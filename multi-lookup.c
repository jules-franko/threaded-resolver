#include "multi-lookup.h"
#define MIN_EXPECTED_ARGS 6

int initialize_logs(char* req_log, char* res_log);

int main(int argc, char** argv)
{
	struct timeval start, end;
	gettimeofday(&start, NULL);

	/*Take command line args*/
	int requesters = atoi(argv[1]);
	int resolvers = atoi(argv[2]);
	char* res_log = argv[4];
	char* req_log = argv[3];

	/*Initialize mutexes, threads, array*/
	array array;
	array_init(&array);

	pthread_mutex_t data_mutex;
	pthread_mutex_t log_mutex;
	pthread_mutex_t stdout_mutex;

	pthread_mutex_init(&data_mutex, NULL);
	pthread_mutex_init(&log_mutex, NULL);
	pthread_mutex_init(&stdout_mutex, NULL);

	pthread_t req_threads[requesters];
	pthread_t res_threads[resolvers];

	/*Do some checking on args*/
	if (argc < MIN_EXPECTED_ARGS) {
		printf("multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
		return -1;
	}

	if (requesters > MAX_REQUESTER_THREADS || \
			resolvers > MAX_RESOLVER_THREADS)
	{
		fprintf(stderr, "Args out of range\n");
		return -1;
	}

	/*If log file(s) exist, truncate them for new use*/
	if (initialize_logs(req_log, res_log) == -1) {
		return -1;
	}

	/*Put all filenames into an array*/
	int num_data_files = argc - (MIN_EXPECTED_ARGS-1);
	if (num_data_files > MAX_INPUT_FILES) {
		fprintf(stderr, "Too many input files\n");
	}

	char** data_files = malloc(num_data_files * sizeof(char*));

	for (int i = 0; i < num_data_files; i++) {
		char* datafile = argv[MIN_EXPECTED_ARGS+i-1];

		/*Allocate space + 1 for terminating char*/
		data_files[i] = malloc(strlen(datafile) + 1);

		strcpy(data_files[i], datafile);
	}

	for (int i = 0; i < requesters; i++) {
		params *p = malloc(sizeof(params));
		p->log_file = req_log;
		p->data_files = data_files;
		p->data_files_size = num_data_files;
		p->array = &array;
		p->data_mutex = &data_mutex;
		p->log_mutex = &log_mutex;
		p->stdout_mutex = &stdout_mutex;
		pthread_create(&req_threads[i], NULL, request, p);
	}

	for (int i = 0; i < resolvers; i++) {
		params *p = malloc(sizeof(params));
		p->log_file = res_log;
		p->array = &array;
		p->data_mutex = &data_mutex;
		p->log_mutex = &log_mutex;
		p->stdout_mutex = &stdout_mutex;
		pthread_create(&res_threads[i], NULL, resolve, p);
	}

	for (int i = 0; i < requesters; i++) {
		pthread_join(req_threads[i], NULL);
	}

	for (int i = 0; i < resolvers; i++) {
		array_put(&array, "poisonpill");
	}

	for (int i = 0; i < resolvers; i++) {
		pthread_join(res_threads[i], NULL);
	}

	/*Clean up program and exit normally*/
	/*Free data files array*/
	for (int i = 0; i < num_data_files; i++) {
		free(data_files[i]);
	}

	free(data_files);
	array_free(&array);
	pthread_mutex_destroy(&data_mutex);
	pthread_mutex_destroy(&log_mutex);
	pthread_mutex_destroy(&stdout_mutex);

	gettimeofday(&end, NULL);
	double time_taken;
	time_taken = (end.tv_sec - start.tv_sec) * 1e6;
	time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

	printf("%s: total time is %f seconds\n", argv[0], time_taken);
	return 0;
}

void* request(void *arg) {
	params *p = arg;
	int files_serviced = 0;

	FILE* log_fptr = fopen(p->log_file, "a");
	if (log_fptr == NULL) {
		printf("Failed to open file\n");
		return NULL;
	}

	/*event loop*/
	while(1) {

		char next_file[MAX_NAME_LENGTH];

		pthread_mutex_lock(p->data_mutex);
		int status = get_next_file(next_file, p->data_files, p->data_files_size);
		pthread_mutex_unlock(p->data_mutex);

		if (status == 2) {
			break;
		}

		FILE* data_file = fopen(next_file, "r");
		if (data_file == NULL) {
			fprintf(stderr, "invalid file %s\n", next_file);
		}
		else {

			/*Loop through file and write it's contents to the shared array*/
			char hostname[MAX_NAME_LENGTH];

			while(fgets(hostname, MAX_NAME_LENGTH, data_file)) {

				pthread_mutex_lock(p->log_mutex);
				fputs(hostname, log_fptr);
				pthread_mutex_unlock(p->log_mutex);

				//hostname[strlen(hostname) - 1] = '\0';
				hostname[strcspn(hostname, "\n")] = '\0';
				if (array_put(p->array, hostname) != 0) {
					printf("Array put ERROR\n");
				}

			}

			files_serviced++;
			fclose(data_file);
			//free(next_file);
		}
	}

	pthread_t self_id = pthread_self();
	pthread_mutex_lock(p->stdout_mutex);
	printf("thread %ld serviced %d files\n", self_id, files_serviced);
	pthread_mutex_unlock(p->stdout_mutex);

	fclose(log_fptr);
	free(p);

	return NULL;
}

void* resolve(void *arg) {
	params *p = arg;
	int hosts_resolved = 0;

	FILE* log_fptr = fopen(p->log_file, "a");
	if (log_fptr == NULL) {
		printf("Failed to open file\n");
		return NULL;
	}

	while(1) {
		char* hostname = NULL;
		array_get(p->array, &hostname);

		if (strcmp(hostname, "poisonpill") == 0) {
			free(hostname);
			break;
		}

		/*Resolve the host*/
		char* IP = malloc(MAX_IP_LENGTH);
		int dns_fail = dnslookup(hostname, IP, MAX_IP_LENGTH);

		/*Write to log*/
		pthread_mutex_lock(p->log_mutex);
		if (dns_fail == 0) {
			fprintf(log_fptr, "%s, %s\n", hostname, IP);
		}
		if (dns_fail == -1) {
			fprintf(log_fptr, "%s, %s\n", hostname, "NOT_RESOLVED");
		}
		pthread_mutex_unlock(p->log_mutex);

		free(IP);
		free(hostname);
		hosts_resolved++;
	}

	pthread_t self_id = pthread_self();
	pthread_mutex_lock(p->stdout_mutex);
	printf("thread %ld resolves %d hostnames\n", self_id, hosts_resolved);
	pthread_mutex_unlock(p->stdout_mutex);

	free(p);
	fclose(log_fptr);

	return NULL;
}

int get_next_file(char* data, char** data_files, int size) {

	for (int i = 0; i < size; i++) {

		if (strcmp(data_files[i], "done") != 0) {
			strcpy(data, data_files[i]);
			strcpy(data_files[i], "done");
			return 0;
		}

	}

	return 2;

}

int initialize_logs(char* req_log, char* res_log) {
	FILE* req_fptr = fopen(req_log, "w+");
	if (req_fptr == NULL) {
		return -1;
	}
	fclose(req_fptr);

	FILE* res_fptr = fopen(res_log, "w+");
	if (res_fptr == NULL) {
		return -1;
	}
	fclose(res_fptr);

	return 0;
}
