/* gcc id_query_loop_test.c -o id_query_loop_test $(pkg-config --cflags --libs
 * libmongoc-1.0) */

/* ./id_query_loop_test [CONNECTION_STRING [COLLECTION_NAME]] */

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int* 
init_ids_array(size_t* ret_len)
{
	size_t tl = 100; //temp buffer size. Will be doubled repeatedly if necessary.
	int* a = calloc(tl, sizeof(int));
	size_t count = 0;
	size_t i;
	for (i = 1; i <= 200; i++) {  //this sequence is just a dummy one for early development
		a[count++] = i;
		if (count == tl) {
			tl *= 2;
			a = realloc(a, tl * sizeof(int));
		}
	}
	*ret_len = count;
	a = realloc(a, count * sizeof(int));
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

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   const char *uristr = "mongodb://127.0.0.1:27017/productpersistdb?appname=client-example";
   const char *collection_name = "product";
   bson_t query;

   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   if (argc > 2) {
      collection_name = argv[2];
   }

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   collection = mongoc_client_get_collection (client, "productpersistdb", collection_name);

   size_t ids_array_len;
   int* ids_array = init_ids_array(&ids_array_len);
   suseconds_t* elapsed_usecs = calloc(ids_array_len, sizeof(suseconds_t));
   char iso80601_dt_buf[sizeof "YYYY-mm-ddTHH:MM:SS.mmm+OOOO\0"];

   size_t i;
   for (i = 0; i < ids_array_len; ++i) {

      bson_init (&query);
      bson_append_int32(&query, "_id", -1, ids_array[i]);
   
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
		 sprintf(iso80601_dt_buf + 19, ".%d", milli);
		 strftime(iso80601_dt_buf + 23, 6, "%z\0", localtime(&start_tp.tv_sec));
         fprintf (stdout, "_id:%d\t%s\t%u\t%u\n", ids_array[i], iso80601_dt_buf, elapsed_usecs[i], doc->len);
      } else {
         elapsed_usecs[i] = -1;
		 fprintf (stderr, "No document for _id: %d was found\n", ids_array[i]);
	  }
   
      if (mongoc_cursor_error (cursor, &error)) {
         fprintf (stderr, "Cursor Failure: %s\n", error.message);
         return EXIT_FAILURE;
      }

   } //end while(ids_array[i])

   free(ids_array);
   bson_destroy (&query);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   if (ids_array_len < 100) {
      fprintf(stderr, "Less than 100 queries executed. Skipping the slowest times by percentile report due to lack of significant sample size.\n");
   } else {
      qsort (elapsed_usecs, ids_array_len, sizeof(suseconds_t), comp_susec);
      size_t idx1 = (size_t)(0.50 * (float)ids_array_len);
      size_t idx2 = (size_t)(0.75 * (float)ids_array_len);
      size_t idx3 = (size_t)(0.90 * (float)ids_array_len);
      size_t idx4 = (size_t)(0.95 * (float)ids_array_len);
      size_t idx5 = (size_t)(0.99 * (float)ids_array_len);
      fprintf(stdout, "P50: %u, P75: %u, P90: %u, P95: %u, P99: %u\n", elapsed_usecs[idx1], 
              elapsed_usecs[idx2], elapsed_usecs[idx3], elapsed_usecs[idx4], elapsed_usecs[idx5]);
   }

   return EXIT_SUCCESS;
}
