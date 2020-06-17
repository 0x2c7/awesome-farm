#include <msgpack/unpack.h>
#include <string.h>
#include "serializer.h"

af_serializer_t * af_new_serializer(void) {
    af_serializer_t * serializer = malloc(sizeof(af_serializer_t));
    serializer->buffer = msgpack_sbuffer_new();
    serializer->packer = msgpack_packer_new(serializer->buffer, msgpack_sbuffer_write);

    return serializer;
}

void af_start_serializing(af_serializer_t * serializer, char * worker_name, int argument_count) {
    msgpack_sbuffer_clear(serializer->buffer);
    msgpack_pack_array(serializer->packer, argument_count + 2);

    int l = strlen(worker_name);
    msgpack_pack_str(serializer->packer, l);
    msgpack_pack_str_body(serializer->packer, worker_name, l);

    msgpack_pack_int(serializer->packer, argument_count);
}

void af_end_serializing(af_serializer_t *serializer, char **job, size_t *job_size) {
  (*job) = serializer->buffer->data;
  (*job_size) = serializer->buffer->size;
}

void af_serialize_int(af_serializer_t *serializer, int var) {
    msgpack_pack_int(serializer->packer, var);\
}

void af_serialize_str(af_serializer_t *serializer, char * str) {
    int l = strlen(str);
    msgpack_pack_str(serializer->packer, l + 1);
    msgpack_pack_str_body(serializer->packer, str, l + 1);
}


void af_deserializer_free(deserializer_object_t *obj) {
    msgpack_zone_clear(&obj->zone);
    msgpack_zone_destroy(&obj->zone);
    free((void *)obj->worker_name);
    free(obj->objects);
    free(obj);
}

af_deserializer_result_t * af_deserialize(char * job, size_t size, deserializer_object_t** obj) {
    *obj = malloc(sizeof(deserializer_object_t));
    af_deserializer_result_t *result = malloc(sizeof(af_deserializer_result_t));

    msgpack_object *msgpack_obj = malloc(sizeof(msgpack_object));
    // TODO: 2048 is too much. Maybe we can save the real size before serializing instead.
    msgpack_zone_init(&(*obj)->zone, 2048);
    msgpack_unpack(job, size, NULL, &(*obj)->zone, msgpack_obj);

    if (msgpack_obj->via.array.ptr == NULL || msgpack_obj->via.array.size < 2) {
        result->status = AF_DESERIALIZER_INVALID_JOB;
        sprintf(result->msg, "Invalid job from Redis");
        free(msgpack_obj);
        af_deserializer_free(*obj);
        return result;
    }

    char *worker_name = malloc(msgpack_obj->via.array.ptr[0].via.str.size + 1);
    worker_name[msgpack_obj->via.array.ptr[0].via.str.size] = '\0';
    memcpy(worker_name, msgpack_obj->via.array.ptr[0].via.str.ptr, msgpack_obj->via.array.ptr[0].via.str.size);
    (*obj)->worker_name = worker_name;
    (*obj)->objects_count = msgpack_obj->via.array.ptr[1].via.i64;
    (*obj)->objects = malloc(sizeof(msgpack_object *) * (*obj)->objects_count);

    for (int i = 0; i < msgpack_obj->via.array.size - 2; i++) {
        (*obj)->objects[i] = &msgpack_obj->via.array.ptr[i+2];
    }

    result->status = AF_DESERIALIZER_OK;
    free(msgpack_obj);
    return result;
}

