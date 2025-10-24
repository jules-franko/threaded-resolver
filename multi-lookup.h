/*Julian Franko*/
/*Threaded DNS Resolver*/

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include "util.h"
#include "array.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define ARRAY_SIZE 8
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct params {
    FILE* log;
    FILE* data;
    int next_data;
} params;

#endif
