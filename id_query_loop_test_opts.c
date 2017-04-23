#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "id_query_loop_test_opts.h"

void init_options() {
  conn_uri = malloc(1);
  conn_uri = NULL;
  ids_filepath = malloc(1);
  ids_filepath = NULL;
  collection_name = malloc(1);
  collection_name = NULL;
  iteration_count = 0;
  sleep_ms = 0;
}

void free_options() {
  int i;
  char* p;

  free(conn_uri);
  free(ids_filepath);
  free(collection_name);
}

int parse_cmd_options(int argc, char **argv, int* err_flag) {

  int c;
  char* tmp_str;
  char* p;
  int tnp_len;
  int i;

  init_options();

  while (1) {
    static struct option long_options[] = {
      {"help",        no_argument, &help_flag,    1},
      {"conn-uri",    required_argument, 0, 'm'},
      {"ids-file",    required_argument, 0, 'f'},
      {"collection",  required_argument, 0, 'c'},
      {"count",       required_argument, 0, 'n'},
      {"sleep-ms",    required_argument, 0, 's'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "m:f:c:n:s:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;

        fprintf(stderr, "Unexpected option %s", long_options[option_index].name);
        if (optarg) {
          fprintf(stderr, " with arg %s", optarg);
        }
        fprintf(stderr, "\n");
        *err_flag = 1;
        break;

      case 'm':
        conn_uri = realloc(conn_uri, strlen(optarg) + 1);
        strcpy(conn_uri, optarg);
        //sanity enforcement
        if (conn_uri && strlen(conn_uri) == 0) {
          free(conn_uri);
          conn_uri = NULL;
        }
        break;

      case 'f':
        ids_filepath = realloc(ids_filepath, strlen(optarg) + 1);
        strcpy(ids_filepath, optarg);
        //sanity enforcement
        if (ids_filepath && strlen(ids_filepath) == 0) {
          free(ids_filepath);
          ids_filepath = NULL;
        }
        break;

      case 'c':
        collection_name = realloc(collection_name, strlen(optarg) + 1);
        strcpy(collection_name, optarg);
        //sanity enforcement
        if (collection_name && strlen(collection_name) == 0) {
          free(collection_name);
          collection_name = NULL;
        }
        break;

      case 'n':
        iteration_count = atoi(optarg);
        break;

      case 's':
        sleep_ms = atoi(optarg);
        break;

      case '?':
        /* getopt_long already printed an error message. */
        *err_flag = 1;
        break;

      default:
        *err_flag = 1;
    }
  }

  return optind; //return idx to non-option argv argument (would have been 
    //moved to end by getopt() functions if it wasn't already).
}

void dump_cmd_options() {
  char* p;
  size_t i;

  printf("help        = %s\n", help_flag ? "true" : "false");
  printf("conn-uri    = \"%s\"\n", conn_uri);
  printf("ids-file    = \"%s\"\n", ids_filepath);
  printf("collection  = \"%s\"\n", collection_name);
  printf("count       = \"%d\"\n", iteration_count);
  printf("sleep-ms    = \"%d\"\n", sleep_ms);
}

void print_options_help() {
  printf("Options:\n\
  --help This message\n\
  -m, --conn-uri\n\
    The connection string in mongodb URI format. Use to specify host, port, \n\
    username, password, authentication db, database name, replset name (if \n\
    using one), read preference, and more advance options such as \n\
    localThresholdMS.\n\
    See https://docs.mongodb.com/manual/reference/connection-string/\n\
  -f, --ids-file\n\
    Path to file containing ids in decimal strings, one per line.\n\
  -c, --collection \n\
    Name of the collection to query on. N.b. database name is selected \n\
    with the --conn-uri parameter.\n\
  -n, --count\n\
    The number of queries to run. Optional. By default it will be one \n\
    iteration of all ids in the --ids-file file.\n\
  -s, --sleep-ms\n\
    Time in milliseconds to sleep between finishing one query and beginning \n\
    the next.\n\
");
}
