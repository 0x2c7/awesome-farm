#ifndef ANIMAL_FARM_CLIENT_INCLUDED
#define ANIMAL_FARM_CLIENT_INCLUDED

#include "hiredis/hiredis.h"

static const char AF_CLIENT_JOBS_KEY[] = "af_jobs";
static const char AF_CLIENT_FAILURE_KEY[] = "af_failures";

typedef struct af_client_t {
    char * redisHost;
    int redisPort;
    redisContext *redisContext;
} af_client_t;

enum af_client_status {
    AF_CLIENT_OK,
    AF_CLIENT_ERROR,
    AF_CLIENT_EMPTY
};

typedef struct af_client_result_t {
    enum af_client_status status;
    char msg[255];
} af_client_result_t;

af_client_t *af_new_client(void);
af_client_result_t * af_init_client(af_client_t * client, char *, int);
void af_free_client(af_client_t *);
af_client_result_t * af_client_schedule(af_client_t *, char *, char*, size_t);
af_client_result_t * af_client_store_failure(af_client_t *, char *, char*, size_t);
af_client_result_t* af_client_fetch_job(af_client_t *, char**, size_t *);
#endif
