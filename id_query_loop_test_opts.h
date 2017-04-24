int help_flag;
char* conn_uri;
char* database_name;
char* collection_name;
int iteration_count;
int sleep_ms;

void init_options();
void free_options();
int  parse_cmd_options(int argc, char **argv, int* err_flag);
void dump_cmd_options();
void print_options_help();

