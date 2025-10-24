#include "multi-lookup.h"

#define MIN_EXPECTED_ARGS 6

int main(int argc, char** argv)
{
	/*Take command line args*/
	int requesters = 0;
	int resolvers = 0;
	char* res_log = NULL;
	char* req_log = NULL;
	char* data_file = NULL;

	FILE* req_fptr = NULL;
	FILE* res_fptr = NULL;
	FILE* data_fptr = NULL;


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

	req_fptr = fopen(req_log, "w+");
	if (req_fptr == NULL) {
		printf("Failed to open requester log file.\n");
		return -1;
	}
	res_fptr = fopen(res_log, "w+");
	if (res_fptr == NULL) {
		printf("Failed to open resolver log file.\n");
		return -1;
	}

	/*Begin Resolving DNS*/
	int num_data_files = argc - MIN_EXPECTED_ARGS;

	for (int i = 0; i < num_data_files; i++) {
		data_file = argv[MIN_EXPECTED_ARGS+i];
		data_fptr = fopen(data_file, "r");

		if (data_fptr == NULL) {
			printf("Failed to open data file(s): %s\n", data_file);
			return -1;
		}

		char *hostname = malloc(MAX_NAME_LENGTH);
		char* ip = malloc(MAX_IP_LENGTH);

		while (fgets(hostname, MAX_NAME_LENGTH, data_fptr)) {
			fprintf(req_fptr,"%s", hostname);

			array_put(&array, hostname);
			array_get(&array, &hostname);

			hostname[strlen(hostname)-1] = '\0';

			if (dnslookup(hostname, ip, MAX_IP_LENGTH) == -1) {
				fprintf(res_fptr,"%s, %s", hostname, ", DNS ERROR");
			}
			else {
				fprintf(res_fptr,"%s, %s", hostname, ip);
				printf("%s\n", ip);
			}
		};

		//free(buf);
		free(ip);
		fclose(data_fptr);
	}

	/*Clean up program and exit normally*/
	fclose(req_fptr);
	fclose(res_fptr);
	array_free(&array);

	return 0;
}
