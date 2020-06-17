#ifndef AWESOME_FARM_INCLUDED
#define AWESOME_FARM_INCLUDED

#include <stdlib.h>
#include <stdarg.h>
#include <ffi.h>
#include <msgpack.h>
#include "macro_magic.h"
#include "worker.h"
#include "client.h"
#include "server.h"
#include "serializer.h"
#include "dispatcher.h"

// Global process client to schedule job
af_client_t *af_client;

enum {
    TYPENAME_BOOL,
    TYPENAME_INT,
    TYPENAME_STR,
};

#define typename(x) _Generic((x),\
_Bool:              TYPENAME_BOOL,\
int:                TYPENAME_INT,\
char *:             TYPENAME_STR,\
default:            NULL\
)

#define AF_GLUE(X,Y) X##Y
#define AF_ACTIVATE_WORKER(WORKER_NAME) af_register_worker(&af_worker_registry,#WORKER_NAME, (void *)WORKER_NAME, NULL, NULL)
#define AF_ACTIVATE_WORKERS(...) FOR_EACH(AF_ACTIVATE_WORKER, __VA_ARGS__)
#define AF_DECLARE_WORKER(WORKER_NAME, ...) \
    void AF_GLUE(af_validate_arguments_, WORKER_NAME)(__VA_ARGS__);\
    void WORKER_NAME(__VA_ARGS__);

#define AF_WORKER(WORKER_NAME, ...) \
    void AF_GLUE(af_validate_arguments_, WORKER_NAME)(__VA_ARGS__) {  }\
    void WORKER_NAME(__VA_ARGS__)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#define AF_SERIALIZE(SERIALIZER,VAR) {\
    switch (typename(VAR)) {\
        case TYPENAME_INT:\
            af_serialize_int(SERIALIZER, (int)(VAR));\
            break;\
        case TYPENAME_STR:\
            af_serialize_str(SERIALIZER, (char*)(VAR));\
            break;\
    }\
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#define AF_SCHEDULE(WORKER_NAME, RESULT, ...) {\
    AF_GLUE(af_validate_arguments_, WORKER_NAME)(__VA_ARGS__);\
    int nValues = PP_NARG(__VA_ARGS__); \
    af_serializer_t *af_serializer = af_new_serializer();\
    af_start_serializing(af_serializer,#WORKER_NAME, nValues);\
    FOR_EACH_ONE_FIXED(AF_SERIALIZE, af_serializer, __VA_ARGS__);\
    char *job;\
    size_t job_size;\
    af_end_serializing(af_serializer,&job, &job_size);\
    RESULT = af_client_schedule(af_client,#WORKER_NAME, job, job_size);\
    free(af_serializer);\
}
af_client_result_t *af_init(char *, int);
void af_cleanup();

#endif
