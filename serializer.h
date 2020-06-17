#ifndef ANIMAL_FARM_SERIALIZER_INCLUDED
#define ANIMAL_FARM_SERIALIZER_INCLUDED

#include "msgpack.h"

typedef struct af_serializer_t {
    msgpack_sbuffer *buffer;
    msgpack_packer *packer;
} af_serializer_t;

typedef struct af_deserializer_object_t {
    const char * worker_name;
    int objects_count;
    msgpack_object ** objects;
    msgpack_zone zone;
} deserializer_object_t;

enum deserializer_status {
    AF_DESERIALIZER_OK,
    AF_DESERIALIZER_INVALID_JOB
};

typedef struct af_deserializer_result_t {
    enum deserializer_status status;
    char msg[255];
} af_deserializer_result_t;

af_serializer_t * af_new_serializer(void);

// TODO: handle serializer errors
void af_start_serializing(af_serializer_t *, char *, int);
void af_serialize_int(af_serializer_t *, int);
void af_serialize_str(af_serializer_t *, char *);
void af_end_serializing(af_serializer_t *, char **, size_t*);

void af_deserializer_free(deserializer_object_t *);
af_deserializer_result_t * af_deserialize(char * , size_t, deserializer_object_t **);

#endif
