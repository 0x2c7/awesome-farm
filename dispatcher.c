#include <ffi.h>
#include "dispatcher.h"
#include "serializer.h"
#include "worker.h"

af_dispatch_result_t * af_dispatch_job(char *job, size_t job_size) {
    af_dispatch_result_t *result = malloc(sizeof(af_dispatch_result_t));
    deserializer_object_t *obj;

    af_deserializer_result_t *deserializer_result = af_deserialize(job, job_size, &obj);

    if (deserializer_result->status != AF_DESERIALIZER_OK) {
        result->status = AF_DISPATCH_DESERIALIZER_ERROR;
        sprintf(result->msg, "Fail to dispatch job. Reason: %s", deserializer_result->msg);
        free(deserializer_result);
        af_deserializer_free(obj);
        return result;
    }
    free(deserializer_result);

    af_worker_t *worker = af_get_worker(&af_worker_registry, obj->worker_name);
    if (worker == NULL) {
        result->status = AF_DISPATCH_WORKER_NOT_FOUND;
        sprintf(result->msg, "Worker `%s` not found. Maybe you forgot to call `AF_ACTIVATE_WORKERS(%s)`", obj->worker_name, obj->worker_name);
        af_deserializer_free(obj);
        return result;
    }

    ffi_cif cif;
    ffi_type **types = (ffi_type **) malloc(obj->objects_count * sizeof(ffi_type *));
    ffi_arg rc;

    void **values = malloc(sizeof(void *) * obj->objects_count);

    for (int i = 0; i < obj->objects_count; i++) {
        types[i] = &ffi_type_pointer;
        switch (obj->objects[i]->type) {
            case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            case MSGPACK_OBJECT_POSITIVE_INTEGER:
                values[i] = &(obj->objects[i]->via.i64);
                break;
            case MSGPACK_OBJECT_STR:
                values[i] = &obj->objects[i]->via.str.ptr;
                break;
            default:
                result->status = AF_DISPATCH_TYPE_NOT_SUPPORTED;
                sprintf(result->msg, "Worker argument's type not supported");
                free(values);
                free(types);
                af_deserializer_free(obj);
                return result;
                break;
        }
    }

    ffi_status ffi_s = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, obj->objects_count, &ffi_type_void, types);
    if (ffi_s == FFI_OK) {
        ffi_call(&cif, worker->process, &rc, values);
    } else if (ffi_s == FFI_BAD_ABI) {
        result->status = AF_DISPATCH_FFI_ERROR;
        sprintf(result->msg, "Fail to dispatch job. FFI raises Bad ABI error!");
    } else if (ffi_s == FFI_BAD_TYPEDEF) {
        result->status = AF_DISPATCH_FFI_ERROR;
        sprintf(result->msg, "Fail to dispatch job. FFI raises bad typedef error!");
    } else {
        result->status = AF_DISPATCH_FFI_ERROR;
        sprintf(result->msg, "Fail to dispatch job. FFI raises unknown error!");
    }

    free(values);
    free(types);
    af_deserializer_free(obj);

    result->status = AF_DISPATCH_OK;
    return result;
}
