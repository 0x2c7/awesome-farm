#include <pthread.h>
#include <time.h>
#include <client.h>
#include <signal.h>
#include <setjmp.h>
#include "worker.h"
#include "dispatcher.h"

static const int AF_SERVER_TICK = 500000000L;
static const int AF_SERVER_DEFAULT_GRACEFUL_TIMEOUT = 15;
static const int AF_SERVER_DEFAULT_JOB_TIMEOUT = 15;

enum af_server_status {
    AF_SERVER_OK,
    AF_SERVER_ERROR
};

typedef struct af_server_result_t {
    enum af_server_status status;
    char msg[255];
} af_server_result_t;

enum af_processor_state {
    AF_PROCESSOR_STATE_IDLE = 1,
    AF_PROCESSOR_STATE_PULLING,
    AF_PROCESSOR_STATE_PROCESSING,
    AF_PROCESSOR_STATE_STOPPING,
    AF_PROCESSOR_STATE_STOPPED
};

typedef struct af_processor_t {
    int process_id;

    enum af_processor_state state;
    af_client_t *af_client;

    char * last_job;
    size_t last_job_size;
    struct timespec healthcheck;

    pthread_t thread;
    pthread_attr_t thread_attr;

    struct sigaction sigact;
    sigset_t sigset;
    sigjmp_buf sigjmp_buf;
} af_processor_t;

typedef struct af_server_configuration {
    char * redisHost;
    int redisPort;
    int graceful_timeout;
    int job_timeout;
    int processors;
    int daemon;
} af_server_configuration;

enum af_server_state {
    AF_SERVER_STATE_STARTING = 1,
    AF_SERVER_STATE_RUNNING,
    AF_SERVER_STATE_STOPPING,
    AF_SERVER_STATE_FORCED_STOPPING,
    AF_SERVER_STATE_STOPPED
};

typedef struct af_server_t {
    enum af_server_state state;
    struct sigaction sigaction;

    int n_processors;
    af_processor_t *processors;

    af_server_configuration configuration;
} af_server_t;

static af_server_t *af_server;
af_server_result_t * af_start_server(af_server_configuration);
