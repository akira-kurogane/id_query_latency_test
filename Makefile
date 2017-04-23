#libmongoc_flags=$(pkg-config --cflags --libs libmongoc-1.0)
libmongoc_flags=-I/usr/include/libmongoc-1.0 -I/usr/include/libbson-1.0 -lmongoc-1.0 -lsasl2 -lssl -lcrypto -lrt -lbson-1.0

id_query_loop_test:	id_query_loop_test.c id_query_loop_test_opts.c
	gcc -c -ggdb id_query_loop_test_opts.c
	gcc -c -ggdb id_query_loop_test.c ${libmongoc_flags}
	gcc -ggdb -o id_query_loop_test id_query_loop_test.o id_query_loop_test_opts.o ${libmongoc_flags}

clean:
	rm -f id_query_loop_test *.o
