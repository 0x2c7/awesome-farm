#include "animal_farm.h"
#include "test_workers.h"

int main() {
    AF_ACTIVATE_WORKERS(worker_1,worker_2);

    af_client_result_t *result = af_init("localhost", 6379);

    af_server_configuration configuration = {
        .redisHost = "localhost",
        .redisPort = 6379,
        .job_timeout = 5,
        .graceful_timeout = 15,
        .processors = 1,
        .daemon = 0,
    };

    af_start_server(configuration);
    af_cleanup();
}
