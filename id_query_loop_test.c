/* gcc id_query_loop_test.c -o id_query_loop_test $(pkg-config --cflags --libs
 * libmongoc-1.0) */

/* ./id_query_loop_test [CONNECTION_STRING [COLLECTION_NAME]] */

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "id_query_loop_test_opts.h"

long* 
init_ids_array(const char* ids_filepath, size_t* ret_len)
{
	size_t tl = 100; //temp buffer size. Will be doubled repeatedly if necessary.
	long* a = malloc(tl * sizeof(long));
	size_t count = 0;
    long x;

	FILE* fp;
    if ((fp = fopen(ids_filepath, "r")) != NULL) {
       while (!feof(fp)) {
          if (fscanf(fp, "%ld", &x) == 1) {
             a[count++] = x;
          }
          if (tl == count) {
             tl *= 2;
             a = realloc(a, tl * sizeof(long));
          }
       }
    }
    fclose(fp);

	*ret_len = count;
	a = realloc(a, count * sizeof(long));
	return a;
}

int comp_susec(const void * elem1, const void * elem2) 
{
    suseconds_t a = *((suseconds_t*)elem1);
    suseconds_t b = *((suseconds_t*)elem2);
    if (a > b) return  1;
    if (a < b) return -1;
    return 0;
}

void print_usage(FILE* fstr) {
  fprintf(fstr, "Usage: id_query_loop_test [options] <file of ids to query>\n");
}

void print_desc() {
  printf("In a loop will query db.collection.find({ _id: <decimal_id_value>}) \n\
  and output the id, the time, the time elapsed in microsecs, and the matching \n\
  document's byte size.\n\
  The final line of output will be slowest times by several percentiles.\n");
}

int
main (int argc, char *argv[])
{
   int opt_err_flag = 0;
   int nonopt_arg_idx = parse_cmd_options(argc, argv, &opt_err_flag);

   if (help_flag) {
      print_usage(stdout);
      printf("\n");
      print_desc();
      printf("\n");
      print_options_help();
      free_options();
      exit(EXIT_SUCCESS);
   } else if (opt_err_flag || nonopt_arg_idx >= argc) {
      print_usage(stderr);
      exit(EXIT_FAILURE);
   }
//dump_cmd_options();

  if (!conn_uri || !database_name || !collection_name) {
	fprintf(stderr, "Aborting. One or more of the neccesary --conn-uri, --database and --collection arguments was absent.\n");
	fprintf(stderr, "Try --help for options description\n");
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

   size_t ids_array_len;
   long* ids_array = init_ids_array(argv[nonopt_arg_idx], &ids_array_len);
   if (ids_array_len <= 0) {
      fprintf (stderr, "No ids were loaded from file %\n", argv[nonopt_arg_idx]);
      return EXIT_FAILURE;
   }

   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bson_t query;

   mongoc_init();

   client = mongoc_client_new(conn_uri);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   collection = mongoc_client_get_collection (client, database_name, collection_name);

   char iso80601_dt_buf[sizeof "YYYY-mm-ddTHH:MM:SS.mmm+OOOO\0"];

   size_t i;
   if (iteration_count < 1) {
      iteration_count = ids_array_len;
   }
   suseconds_t* elapsed_usecs = calloc(iteration_count, sizeof(suseconds_t));

   for (i = 0; i < iteration_count; ++i) {

      long curr_id = ids_array[i % ids_array_len];
      bson_init (&query);
      bson_append_int64(&query, "_id", -1, curr_id);
   
         struct timeval start_tp;
         gettimeofday(&start_tp, NULL);

      cursor = mongoc_collection_find_with_opts (
         collection,
         &query,
         NULL,  /* additional options */
         NULL); /* read prefs, NULL for default */

      if (mongoc_cursor_next (cursor, &doc)) {
         struct timeval end_tp;
         gettimeofday(&end_tp, NULL);
         elapsed_usecs[i] = ((end_tp.tv_sec - start_tp.tv_sec) * 1000000) + (end_tp.tv_usec - start_tp.tv_usec);
         strftime(iso80601_dt_buf, sizeof(iso80601_dt_buf), "%FT%T", localtime(&start_tp.tv_sec));
         iso80601_dt_buf[sizeof(iso80601_dt_buf)] = '\0';
         int milli = start_tp.tv_usec / 1000;
         sprintf(iso80601_dt_buf + 19, ".%03d", milli);
         strftime(iso80601_dt_buf + 23, 6, "%z\0", localtime(&start_tp.tv_sec));
         fprintf (stdout, "{_id:%ld}\t%s\t%u\t%u\n", curr_id, iso80601_dt_buf, elapsed_usecs[i], doc->len);
      } else {
         elapsed_usecs[i] = -1;
         fprintf (stderr, "No document for _id: %ld was found\n", curr_id);
      }

      if (mongoc_cursor_error (cursor, &error)) {
         fprintf (stderr, "Cursor Failure: %s\n", error.message);
         return EXIT_FAILURE;
      }

      bson_destroy (&query);
      mongoc_cursor_destroy (cursor);

      if (sleep_ms > 0) {
         usleep(sleep_ms * 1000);
      }

   } //end for(;;++i)

   free(ids_array);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   if (iteration_count < 100) {
      fprintf(stderr, "Less than 100 queries executed. Skipping the slowest-times-by-percentile report due to insufficient sample size.\n");
   } else {
      qsort (elapsed_usecs, iteration_count, sizeof(suseconds_t), comp_susec);
      size_t idx1 = (size_t)(0.50 * (float)iteration_count);
      size_t idx2 = (size_t)(0.75 * (float)iteration_count);
      size_t idx3 = (size_t)(0.90 * (float)iteration_count);
      size_t idx4 = (size_t)(0.95 * (float)iteration_count);
      size_t idx5 = (size_t)(0.99 * (float)iteration_count);
      fprintf(stdout, "P50: %u, P75: %u, P90: %u, P95: %u, P99: %u\n", elapsed_usecs[idx1], 
              elapsed_usecs[idx2], elapsed_usecs[idx3], elapsed_usecs[idx4], elapsed_usecs[idx5]);
   }

   free_options();
   return EXIT_SUCCESS;
}
