#include <string.h>
#include <stdlib.h>
#include "worker.h"

void af_register_worker(af_worker_registry_t *registry, char *name, void *process, void *serialize, void *deserialize) {
    registry->num++;
    af_worker_t *w = malloc(sizeof(af_worker_t));
    w->name = name;
    w->process = process;
    w->next = NULL;

    if (registry->head == NULL) {
        registry->head = w;
        registry->tail = w;
    } else {
        registry->tail->next = w;
        registry->tail = w;
    }
}

af_worker_t * af_get_worker(af_worker_registry_t *registry, const char * name) {
    if (registry->num == 0) {
        return NULL;
    }
    af_worker_t *w = registry->head;
    while (w != NULL) {
        if (strcmp(name, w->name) == 0) {
            return w;
        }
        w = w->next;
    }
    return NULL;
}

void af_free_worker_registry(af_worker_registry_t * registry) {
    if (registry->head == NULL) {
        registry->num = 0;
        return;
    }
    af_worker_t * worker = registry->head;
    af_worker_t * previousWorker = registry->head;
    while (worker != NULL) {
        previousWorker = worker;
        worker = worker->next;
        free(previousWorker);
    }
}
