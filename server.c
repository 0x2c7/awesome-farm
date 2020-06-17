#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "server.h"

static void processor_handle_failure(af_processor_t *processor) {
    af_client_result_t * result = af_client_store_failure(processor->af_client, "Unknown worker", processor->last_job, processor->last_job_size);
    if (result->status == AF_CLIENT_ERROR) {
        printf("[Processor %i] Fail to retore failed job. Senpai, please notice me:\n%.*s\n", processor->process_id, (int)processor->last_job_size, processor->last_job);
    }

    free(result);
}


static void processor_stop(af_processor_t * processor) {
    /* sleep(processor->process_id / 2); */
}

static void processor_dispatch_job(af_processor_t *processor) {
    processor->state = AF_PROCESSOR_STATE_PULLING;
    af_dispatch_result_t *dispatch_result;
    af_client_result_t *client_result = af_client_fetch_job(processor->af_client, &processor->last_job, &processor->last_job_size);

    if (client_result->status == AF_CLIENT_EMPTY) {
        processor->state = AF_PROCESSOR_STATE_IDLE;
        free(client_result);
        nanosleep((const struct timespec[]){{0, (AF_SERVER_TICK + rand() % 500)*1000000L}}, NULL);
        timespec_get(&processor->healthcheck, TIME_UTC);
        return;
    }

    if (client_result->status != AF_CLIENT_OK) {
        processor->state = AF_PROCESSOR_STATE_IDLE;
        printf("[Processor %i] Fail to fetch job. Reason: %s\n", processor->process_id, client_result->msg);
        free(client_result);
        return;
    }
    free(client_result);

    processor->state = AF_PROCESSOR_STATE_PROCESSING;
    timespec_get(&processor->healthcheck, TIME_UTC);

    int jump_code = sigsetjmp(processor->sigjmp_buf, 1);
    switch (jump_code) {
        case 0:
            dispatch_result = af_dispatch_job(processor->last_job, processor->last_job_size);
            if (dispatch_result->status != AF_DISPATCH_OK) {
                processor_handle_failure(processor);
                printf("[Processor %i] Error while processing job. Reason: %s\n", processor->process_id, dispatch_result->msg);
            }
            free(dispatch_result);
            break;
        case -1:
            if (sigismember(&processor->sigset, SIGSEGV)) {
                printf("[Processor %i] Received SIGSEGV (segmentation fault) signal. Ignore and recover thread!\n", processor->process_id);
            } else {
                printf("[Processor %i] Received unknown signal. Ignore and recover thread!\n", processor->process_id);
            }
            processor_handle_failure(processor);
            break;
        default:
            printf("[Processor %i] Received unknown signal. Ignore and recover thread!\n", processor->process_id);
            processor_handle_failure(processor);
            break;
    }
    if (dispatch_result != NULL) {
        /* free(dispatch_result); */
    }
    free(processor->last_job);
    processor->last_job = NULL;
}

static af_processor_t* processor_search(pthread_t thread) {
    for (int i = 0; i < af_server->n_processors; i++) {
        if (pthread_equal(thread, af_server->processors[i].thread)) {
            return &af_server->processors[i];
        }
    }
    return NULL;
}


static void processor_handle_signals(void) {
    af_processor_t *processor = processor_search(pthread_self());
    if (processor == NULL) {
        pthread_exit(NULL);
        return;
    } else {
        siglongjmp(processor->sigjmp_buf, -1);
    }
}

static void processor_init_signals(af_processor_t *processor) {
    sigemptyset(&processor->sigset);
    sigaddset(&processor->sigset, SIGSEGV);
    processor->sigact.sa_flags = 0;
    processor->sigact.sa_mask = processor->sigset;
    processor->sigact.sa_handler = (void*)processor_handle_signals;
    sigaction(SIGSEGV, &processor->sigact, NULL);
}

static void processor_loop(void * arg) {
    af_processor_t *processor = (af_processor_t*) arg;
    af_server_t *server = af_server;

    for (;;) {
        if (server->state == AF_SERVER_STATE_STOPPING || server->state == AF_SERVER_STATE_FORCED_STOPPING) {
            break;
        }
        if (server->state == AF_SERVER_STATE_RUNNING) {
            break;
        }
        sleep(1);
    }

    processor_init_signals(processor);
    for (;;) {
        if (server->state != AF_SERVER_STATE_RUNNING) {
            processor->state = AF_PROCESSOR_STATE_STOPPING;
            processor_stop(processor);

            processor->state = AF_PROCESSOR_STATE_STOPPED;
            printf("[Processor %i] Processor stopped!\n", processor->process_id);

            return;
        }
        processor_dispatch_job(processor);
        processor->state = AF_PROCESSOR_STATE_IDLE;
    }
}

static af_server_result_t * processor_init(af_server_t *server, int i) {
    af_processor_t *processor = &server->processors[i];

    processor->state = AF_PROCESSOR_STATE_IDLE;
    processor->process_id = i;
    processor->state = AF_PROCESSOR_STATE_IDLE;
    processor->af_client = af_new_client();
    processor->thread = 0;
    timespec_get(&processor->healthcheck, TIME_UTC);

    af_client_result_t *client_result = af_init_client(processor->af_client, server->configuration.redisHost, server->configuration.redisPort);
    af_server_result_t *processor_result = malloc(sizeof(af_server_result_t));

    if (client_result->status == AF_CLIENT_ERROR) {
        processor_result->status = AF_SERVER_ERROR;
        strcpy(processor_result->msg, client_result->msg);
    } else {
        pthread_attr_setdetachstate(&processor->thread_attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setinheritsched(&processor->thread_attr, PTHREAD_INHERIT_SCHED);
        pthread_attr_setscope(&processor->thread_attr, PTHREAD_SCOPE_PROCESS);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        /* pthread_attr_setstacksize(processor->thread_attr size_t __stacksize); */

        pthread_create(&processor->thread, &processor->thread_attr, (void*)&processor_loop, processor);
    }

    free(client_result);
    return processor_result;
}

static int server_calculate_processors(void) {
    FILE *f = popen("nproc", "r");

    int n_proc;
    fscanf(f, "%i", &n_proc);

    fclose(f);
    return n_proc * 4;
}


static void server_handle_signal() {
    if (af_server->state == AF_SERVER_STATE_STOPPING) {
        af_server->state = AF_SERVER_STATE_FORCED_STOPPING;
    } else if (af_server->state != AF_SERVER_STATE_FORCED_STOPPING) {
        af_server->state = AF_SERVER_STATE_STOPPING;
    }
}

static void server_init_signals(af_server_t *server) {
    server->sigaction.sa_flags = 0;
    sigemptyset(&server->sigaction.sa_mask);
    server->sigaction.sa_handler = server_handle_signal;

    sigaction(SIGINT, &server->sigaction, NULL);
}

static void processor_free(af_processor_t * processor) {
    af_free_client(processor->af_client);
    if (processor->last_job != NULL) {
        free(processor->last_job);
    }
}

static void server_check_processor_timeout(af_server_t *server) {
    struct timespec now;
    long time_diff = 0;

    timespec_get(&now, TIME_UTC);

    for (int i = 0; i < server->n_processors; i++) {
        if (server->processors[i].healthcheck.tv_sec == 0 && server->processors[i].healthcheck.tv_nsec == 0) {
            continue;
        }
        time_diff = now.tv_sec - server->processors[i].healthcheck.tv_sec;
        if (time_diff > AF_SERVER_DEFAULT_JOB_TIMEOUT) {
            printf("[Server] Processor %i timeout while processing a job. Force reset!\n", server->processors[i].process_id);
            pthread_cancel(server->processors[i].thread);
            pthread_join(server->processors[i].thread, NULL);
            processor_free(&server->processors[i]);
            af_server_result_t *result = processor_init(server, server->processors[i].process_id);
            // TODO: Handle fail to re-initialize processor here.
            free(result);
        }
    }
}

static void server_wait(af_server_t *server) {
    server->state = AF_SERVER_STATE_RUNNING;

    for (;;) {
        if (server->state != AF_SERVER_STATE_RUNNING) {
            printf("[Server] Stopping job server. Graceful timeout %i seconds...\n", server->configuration.graceful_timeout);

            char processor_all_stopped = 0;
            int tick = 1;
            struct timespec wait_begin_at;
            struct timespec now;
            timespec_get(&wait_begin_at, TIME_UTC);
            for (;;) {
                timespec_get(&now, TIME_UTC);
                if (server->state == AF_SERVER_STATE_FORCED_STOPPING) {
                    printf("[Server] Force stopped! All processors will be killed ungracefully!\n");
                    break;
                }

                if (now.tv_sec - wait_begin_at.tv_sec > server->configuration.graceful_timeout) {
                    printf("[Server] Exceed graceful timeout! Force stop all processors!\n");
                    break;
                }

                processor_all_stopped = 1;
                for (int i = 0; i < server->n_processors; i++) {
                    if (server->processors[i].state != AF_SERVER_STATE_STOPPED) {
                        processor_all_stopped = 0;
                        break;
                    }
                }
                if (processor_all_stopped) {
                    break;
                } else {
                    nanosleep((const struct timespec[]){{0, AF_SERVER_TICK}}, NULL);
                }
            }

            server->state = AF_SERVER_STATE_STOPPED;
            return;
        }
        server_check_processor_timeout(server);
        nanosleep((const struct timespec[]){{0, AF_SERVER_TICK}}, NULL);
    }
}

static void server_free(af_server_t *server) {
    signal(SIGINT, SIG_DFL);
    for (int i = 0; i < server->n_processors; i++) {
        pthread_cancel(server->processors[i].thread);
        if (server->processors[i].af_client != NULL) {
            processor_free(&server->processors[i]);
        }
    }
    free(server->processors);
    free(server);
}

af_server_result_t *af_start_server(af_server_configuration configuration) {
    srand(time(0));

    af_server = malloc(sizeof(af_server_t));
    af_server->state = AF_SERVER_STATE_STARTING;
    af_server->configuration = configuration;
    if (configuration.graceful_timeout == 0) {
        af_server->configuration.graceful_timeout = AF_SERVER_DEFAULT_GRACEFUL_TIMEOUT;
    }
    if (configuration.job_timeout == 0) {
        af_server->configuration.job_timeout = AF_SERVER_DEFAULT_JOB_TIMEOUT;
    }
    if (configuration.processors <= 0) {
        af_server->configuration.processors = server_calculate_processors();
    }
    af_server->n_processors = af_server->configuration.processors;

    if (af_server->configuration.daemon == 1) {
        int server_pid = fork();
        if (server_pid != 0) {
            printf("[Server] Starting job server at pid %i.\n", server_pid);
            printf("[Server] Number of processors: %i.\n", af_server->n_processors);
            printf("[Server] Running as daemon mode\n");
            exit(0);
        }
    } else {
        printf("[Server] Starting job server at pid %i.\n", getpid());
        printf("[Server] Number of processors: %i.\n", af_server->n_processors);
    }

    af_server->processors = malloc(sizeof(af_processor_t) * af_server->n_processors);
    af_server_result_t *result;

    for (int i = 0; i < af_server->n_processors; i++) {
        result = processor_init(af_server, i);

        if (result->status == AF_SERVER_ERROR) {
            printf("[Server] Fail to initialize processor.\nReason: %s\n", result->msg);
            for (int j = 0; j <= i; j++) {
                processor_stop(&af_server->processors[i]);
            }
            server_free(af_server);
            af_server = NULL;

            return result;
        }
        free(result);
    }

    server_init_signals(af_server);
    server_wait(af_server);

    printf("[Server] Job server stopped!\n");
    server_free(af_server);

    af_server = NULL;

    result->status = AF_SERVER_OK;
    return result;
}
