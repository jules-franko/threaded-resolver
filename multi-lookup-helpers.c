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
}
