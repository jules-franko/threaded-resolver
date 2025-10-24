/*
 * Shared array module implementation
 * @license GPLv3
 * @author Julian Franko
 * @version 10-5-2025
 */
#ifndef ARRAY_H
#define ARRAY_H
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#ifdef ARRAY_IMPORT
#define EXTERN
#else
#define EXTERN extern
#endif
#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 253 /*Biggest possible hostname*/
typedef struct array {
	char *array[ARRAY_SIZE];
	int front;
	int size;
	sem_t mutex;
	sem_t space_avail;
	sem_t items_avail;
} array;

EXTERN int array_init(array *s); // initialize the array
EXTERN int array_put (array *s, char *hostname); // place element into the array, block when full
EXTERN int array_get (array *s, char **hostname); // remove element from the array, block when empty
EXTERN void array_free(array *s); // free the array's resources

#undef ARRAY_IMPORT
#undef EXTERN
#endif

