#ifndef ANIMAL_FARM_DISPATCHER_INCLUDED
#define ANIMAL_FARM_DISPATCHER_INCLUDED

#include "serializer.h"

enum af_dispatch_status {
    AF_DISPATCH_OK,
    AF_DISPATCH_TYPE_NOT_SUPPORTED,
    AF_DISPATCH_WORKER_NOT_FOUND,
    AF_DISPATCH_DESERIALIZER_ERROR,
    AF_DISPATCH_FFI_ERROR
};

typedef struct af_dispatch_result_t {
    enum af_dispatch_status status;
    char msg[255];
} af_dispatch_result_t;

af_dispatch_result_t *af_dispatch_job(char *, size_t);

#endif
