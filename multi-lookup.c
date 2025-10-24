#include "multi-lookup.h"

#define MIN_EXPECTED_ARGS 6

int get_next_file(char* data, char** data_files, int size) {
	for (int i = 0; i < size; i++) {

		strcpy(data, data_files[i]);

		if ((strcmp(data, "done") != 0)) {
			strcpy(data_files[i], "done");
			return 0;
		}

	}

	return 2;
}

void* request(void *arg) {
	params *p = arg;

	FILE* log_fptr = fopen(p->log_file, "w+");
	if (log_fptr == NULL) {
		printf("Failed to open file\n");
		return NULL;
	}

	while(1) {

		// char* next_file = malloc(MAX_NAME_LENGTH);
		char next_file[MAX_NAME_LENGTH];
		int status = get_next_file(next_file, p->data_files, p->data_files_size);
		if (status == 2) {
			array_put(p->array, "poisonpill");
			break;
		}

		FILE* data_file = fopen(next_file, "r");

		/*Loop through file and write it's contents to the shared array*/
		// char* hostname = malloc(MAX_NAME_LENGTH);
		char hostname[MAX_NAME_LENGTH];

		while(fgets(hostname, MAX_NAME_LENGTH, data_file)) {
			hostname[strlen(hostname) - 1] = '\0';
			if (array_put(p->array, hostname) != 0) {
				printf("Array put ERROR\n");
			}
			fputs(hostname, log_fptr);
			printf("PUT %s\n", hostname);
		}

		fclose(data_file);
	}

	fclose(log_fptr);

	return NULL;
}

void* resolve(void *arg) {
	params *p = arg;

	for (int i = 0; i < 100; i++) {
		char* IP = malloc(MAX_IP_LENGTH);
		array_get(p->array, &IP);
		if (strcmp(IP, "poisonpill") == 0) {
			free(IP);
			return NULL;
		}
		printf("GOT %s\n", IP);
		free(IP);
	}

	return NULL;
}

int main(int argc, char** argv)
{
	/*Take command line args*/
	int requesters = 0;
	int resolvers = 0;
	char* res_log = NULL;
	char* req_log = NULL;

	array array;
	array_init(&array);

	if (argc < MIN_EXPECTED_ARGS) {
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
		return -1;
	}

	if (requesters > MAX_REQUESTER_THREADS || \
			resolvers > MAX_RESOLVER_THREADS)
	{
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
		return -1;
	}

	requesters = atoi(argv[1]);
	resolvers = atoi(argv[2]);
	req_log = argv[3];
	res_log = argv[4];

	/*Put all filenames into an array*/
	int num_data_files = argc - MIN_EXPECTED_ARGS;
	char** data_files = malloc(num_data_files * sizeof(char*));

	/*Fill array of data files*/
	for (int i = 0; i < num_data_files; i++) {
		char* datafile = argv[MIN_EXPECTED_ARGS+i];

		/*Allocate space + 1 for terminating char*/
		data_files[i] = malloc(strlen(datafile) + 1);

		strcpy(data_files[i], datafile);
	}

	pthread_t req_threads[requesters];
	pthread_t res_threads[resolvers];

	for (int i = 0; i < requesters; i++) {
		params *p = malloc(sizeof(params));
		p->log_file = req_log;
		p->data_files = data_files;
		p->data_files_size = num_data_files;
		p->array = &array;
		pthread_create(&req_threads[i], NULL, request, p);
	}

	for (int i = 0; i < resolvers; i++) {
		params *p = malloc(sizeof(params));
		p->log_file = res_log;
		p->array = &array;
		pthread_create(&res_threads[i], NULL, resolve, p);
	}

	for (int i = 0; i < resolvers; i++) {
		pthread_join(res_threads[i], NULL);
	}

	for (int i = 0; i < requesters; i++) {
		pthread_join(req_threads[i], NULL);
	}

	/*Clean up program and exit normally*/
	/*Free data files array*/
	for (int i = 0; i < num_data_files; i++) {
		free(data_files[i]);
	}

	array_free(&array);

	return 0;
}
