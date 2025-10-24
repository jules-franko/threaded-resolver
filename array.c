#define ARRAY_IMPORT
#include "array.h"
int array_init(array *s) {
	s->size = 0;
	s->front = 0;
	sem_init(&s->mutex, 0, 1);
	sem_init(&s->space_avail, 0, ARRAY_SIZE);
	sem_init(&s->items_avail, 0, 0);
	//Initialize all pointers to NULL for later convenience when freeing
	for (int i = 0; i < ARRAY_SIZE; i++) {
		s->array[i] = NULL;
	}
	return 0;
}
int array_put(array *s, char *hostname) {
	sem_wait(&s->space_avail);
	sem_wait(&s->mutex);
	/*Begin Critical Section*/
	int put = (s->front+s->size) % ARRAY_SIZE;
	//If there is already an item here, free it before allocating another chunk
	if (s->array[put] != NULL) {
		free(s->array[put]);
	}
	//Check if string is too long, it so, terminate with error
	if (strlen(hostname) > MAX_NAME_LENGTH) {
		array_free(s);
		return -1;
	}
	char* alloc_hostname = malloc(strlen(hostname) + 1);
	strcpy(alloc_hostname, hostname);
	s->array[put] = alloc_hostname;
	s->size++;
	/*End Critical Section*/
	sem_post(&s->mutex);
	sem_post(&s->items_avail);
	return 0;
}
int array_get(array *s, char **hostname) {
	sem_wait(&s->items_avail);
	sem_wait(&s->mutex);
	/*Begin Critical Section*/
	*hostname = s->array[s->front];
	s->front = (s->front+1)%ARRAY_SIZE;
	s->size--;
	/*End Critical Section*/
	sem_post(&s->mutex);
	sem_post(&s->space_avail);
	return 0;
}
void array_free(array *s) {
	//De-allocate remaining pointers
	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (s->array[i] != NULL) {
			free(s->array[i]);
		}
	}
	sem_destroy(&s->mutex);
	sem_destroy(&s->space_avail);
	sem_destroy(&s->items_avail);
	return;
}
