#ifndef AWESOME_FARM_WORKER_INCLUDED
#define AWESOME_FARM_WORKER_INCLUDED

#include <stdlib.h>

typedef struct af_worker_t {
  const char *name;
  void *process;
  void *serialize;
  void *deserialize;
  struct af_worker_t *next;
} af_worker_t;

typedef struct af_worker_registry_t {
    int num;
    struct af_worker_t * head;
    struct af_worker_t * tail;
} af_worker_registry_t;

af_worker_registry_t af_worker_registry;

void af_init_registry(void);
void af_register_worker(af_worker_registry_t *, char *, void (*), void (*), void (*));
af_worker_t * af_get_worker(af_worker_registry_t*, const char *);
void af_free_worker_registry(af_worker_registry_t *);

#endif
