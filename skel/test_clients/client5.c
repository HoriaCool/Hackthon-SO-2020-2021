#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/logmemcache.h"

int main() {
	struct logmemcache_st *client;
	struct client_logline **lines;
	char *stats;
	uint64_t logs, i;
	char time[TIME_SIZE + 1];
	char logline[LINE_SIZE - TIME_SIZE + 1];

	client = logmemcache_create(NULL);
	logmemcache_get_stats(client);

	logmemcache_add_log(client, "this is a log line 0");
	logmemcache_add_log(client, "this is a log line 1");
	logmemcache_add_log(client, "this is a log line 2");
	logmemcache_add_log(client, "this is a log line 3");

	stats = logmemcache_get_stats(client);
	printf("%s\n", stats);
	lines = logmemcache_get_logs(client, 0, 0, &logs);

	for (i = 0; i < logs; i++) {
		logmemcache_get_time(time, lines[i], sizeof(time));
		logmemcache_get_logline(logline, lines[i], sizeof(logline));
		printf("%s: %s\n", time, logline);
	}

	logmemcache_flush(client);
	printf("----------------------flush-----------------------\n");
	logmemcache_disconnect(client);

	client = logmemcache_create(NULL);

	logmemcache_add_log(client, "this is a log line 4");
	logmemcache_add_log(client, "this is a log line 5");
	logmemcache_add_log(client, "this is a log line 6");

	stats = logmemcache_get_stats(client);
	printf("%s\n", stats);
	lines = logmemcache_get_logs(client, 0, 0, &logs);

	for (i = 0; i < logs; i++) {
		logmemcache_get_time(time, lines[i], sizeof(time));
		logmemcache_get_logline(logline, lines[i], sizeof(logline));
		printf("%s: %s\n", time, logline);
	}

	logmemcache_unsubscribe(client);

	client = logmemcache_create(NULL);
	logmemcache_get_stats(client);

	return 0;
}
