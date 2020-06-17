#include "animal_farm.h"
#include "test_workers.h"

int main() {
    AF_ACTIVATE_WORKERS(worker_1,worker_2);

    af_client_result_t *result = af_init("localhost", 6379);

    if (result->status == AF_CLIENT_ERROR) {
        printf("Error: %s\n", result->msg);
        free(result);
        return 1;
    }
    free(result);

    af_server_configuration configuration = {
        .redisHost = "localhost",
        .redisPort = 6379,
        .job_timeout = 5,
        .graceful_timeout = 15,
        .processors = 1,
        .daemon = 0,
    };

    af_server_result_t *server_result = af_start_server(configuration);
    if (result->status == AF_SERVER_ERROR) {
        printf("Error: %s\n", result->msg);
        free(server_result);
        return 1;
    }
    af_cleanup();

    return 0;
}
