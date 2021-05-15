#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/logmemcache.h"


int main() {
	struct logmemcache_st *client;

	client = logmemcache_create(NULL);

	printf("client = %p\n", client);

	if (client != NULL)
		fprintf(stderr, "Client was created\n");
	else
		fprintf(stderr, "Client was not created\n");

	// logmemcache_add_log(client, "this is a log line 0");
	// char *recv = logmemcache_get_stats(client);
	// printf("|%s|\n", recv);

	// logmemcache_add_log(client, "this is a log line 1");
	// recv = logmemcache_get_stats(client);
	// printf("|%s|\n", recv);

	// logmemcache_add_log(client, "this is a log line 2");
	// recv = logmemcache_get_stats(client);
	// printf("|%s|\n", recv);

	// logmemcache_add_log(client, "this is a log line 3");
	// recv = logmemcache_get_stats(client);
	// printf("|%s|\n", recv);

	// logmemcache_add_log(client, "this is a log line 4");
	// recv = logmemcache_get_stats(client);
	// printf("|%s|\n", recv);

	// logmemcache_disconnect(client);



	
	
	// client = logmemcache_create(NULL);
	// logmemcache_unsubscribe(client);

	// client = logmemcache_create(NULL);
	// logmemcache_unsubscribe(client);

	// client = logmemcache_create(NULL);
	// logmemcache_disconnect(client);

	// client = logmemcache_create(NULL); // found
	// logmemcache_unsubscribe(client);


	return 0;
}
