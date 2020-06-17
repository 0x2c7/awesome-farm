#include "animal_farm.h"
#include "test_workers.h"

int main() {
    // Activate which workers to run in this server. Used to scale or reserve workers on purpose.
    AF_ACTIVATE_WORKERS(worker_1,worker_2);

    af_client_result_t *result = af_init("localhost", 6379);
    af_server_configuration configuration = {
        // Host and port of redis server
        .redisHost = "localhost",
        .redisPort = 6379,
        // Default timeout is 15 second. If a job exceeds the timeout, its thread is killed, and restarted
        .job_timeout = 5,
        // Default timeout is 15 second. How long the process waits for its jobs to finish.
        .graceful_timeout = 15,
        // Number of concurrent threads to handle the jobs
        .processors = 10,
        // Whether the server should runs in daemon mode.
        .daemon = 0,
    };

    af_start_server(configuration);
    af_cleanup();
}
