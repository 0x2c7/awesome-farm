#include <stdio.h>
#include "test_workers.h"
#include <unistd.h>

AF_WORKER(worker_1, char * a, int b) {
    printf("worker_1: a = %s, b = %i\n", a, b);
}

AF_WORKER(worker_2, int a, int b) {
    printf("worker_2: a = %i, b = %i\n", a, b);
}
