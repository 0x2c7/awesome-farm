#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "animal_farm.h"
#include "ffi.h"

af_client_result_t *af_init(char * host, int port) {
    af_client_result_t *result;
    af_client = af_new_client();
    result = af_init_client(af_client, host, port);
    if (result->status != AF_CLIENT_OK) {
        free(af_client);
        af_client = NULL;
    }
    return result;
}

void af_cleanup() {
    if (af_client != NULL) {
        af_free_client(af_client);
    }
    if (af_worker_registry.head != NULL) {
        af_free_worker_registry(&af_worker_registry);
    }
}
