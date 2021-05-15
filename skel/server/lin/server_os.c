/**
 * Hackathon SO: LogMemCacher
 * (c) 2020-2021, Operating Systems
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <math.h>

#include "../../include/server.h"
#include "../../include/thpool.h"

extern char *logfile_path;

#define MAX_ALLOCATED_PAGES 5


// static void* client_function(SOCKET *client_sock)
static void client_function(void* client_sock)
{
	int rc;
	struct logmemcache_client_st *client;

	client = logmemcache_create_client(*(SOCKET*) client_sock);

	while (1) {
		rc = get_command(client);
		printf("rc:%d\n", rc);
		if (rc == -1)
			break;
	}

	close(*(SOCKET*) client_sock);
	free(client);

	printf("client out\n");
}

void logmemcache_init_server_os(int *socket_server)
{
	threadpool thpool = thpool_init(1);

	int sock, client_size, client_sock;
	struct sockaddr_in server, client;

	memset(&server, 0, sizeof(struct sockaddr_in));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return;

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = inet_addr(SERVER_IP);

	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Could not bind");
		exit(1);
	}

	if (listen(sock, DEFAULT_CLIENTS_NO) < 0) {
		perror("Error while listening");
		exit(1);
	}

	*socket_server = sock;

	while (1) {
		memset(&client, 0, sizeof(struct sockaddr_in));
		client_size = sizeof(struct sockaddr_in);
		client_sock = accept(sock, (struct sockaddr *)&client,
				(socklen_t *)&client_size);

		if (client_sock < 0) {
			perror("Error while accepting clients");
		}

		thpool_add_work(thpool, client_function, &client_sock);
		// client_function(client_sock);
	}

	// useless:
	thpool_wait(thpool);
	thpool_destroy(thpool);
}

int logmemcache_init_client_cache(struct logmemcache_cache *cache)
{
	cache->ptr = mmap(NULL, getpagesize() * MAX_ALLOCATED_PAGES,
		PROT_READ | PROT_WRITE,	MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (cache->ptr == MAP_FAILED) {
		printf("mmap failed\n");
		return -1;
	}
	cache->pages = 0;
	cache->logs = 0;
	cache->file_logs = 0;

	// init logs file:
	char buff_path[1000];
	snprintf(buff_path, sizeof(buff_path), "%s/%s.log", logfile_path, cache->service_name);

	cache->fd = open(buff_path, O_RDWR | O_APPEND | O_CREAT, 0777);
	if (cache->fd < 0)
		printf("open failed\n");

	return 0;
}

int logmemcache_add_log_os(struct logmemcache_client_st *client,
	struct client_logline *log)
{
	char *dst = (char*) client->cache->ptr
		+ client->cache->logs * sizeof(*log);
	char *end = (char*) client->cache->ptr +
				MAX_ALLOCATED_PAGES * getpagesize();


	if (dst + sizeof(*log) > end) {
		// flush the logs:
		if (logmemcache_flush_os(client))
			return -1;

		if (munmap(client->cache->ptr, getpagesize() * MAX_ALLOCATED_PAGES)) {
			printf("munmap failed\n");
			return -1;
		}
		client->cache->ptr = mmap(NULL, getpagesize() * MAX_ALLOCATED_PAGES,
			PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (client->cache->ptr == MAP_FAILED)
			return -1;
		
		client->cache->pages = 1;
	} else {
		client->cache->pages = ceil(1.0 * (unsigned int) (dst -
				(char*) client->cache->ptr +
				sizeof(*log)) / getpagesize());
	}
	memcpy(dst, log, sizeof(*log));
	++client->cache->logs;
	printf("|%s| |%s|\n",
		((struct client_logline *) dst)->time, ((struct client_logline *) dst)->logline);

	return 0;
}

int logmemcache_send_loglines_os(struct logmemcache_client_st *client)
{
	int err;
	char *src;
	size_t total_logs = client->cache->file_logs + client->cache->logs;
	err = send_data(client->client_sock, &total_logs, sizeof(size_t), SEND_FLAGS);
	if (err < 0)
		return -1;

	
	// sends the logs from file:
	lseek(client->cache->fd, 0, SEEK_SET);
	char buff[sizeof(struct client_logline)];

	for (int i = 0; i < client->cache->file_logs; ++i) {
		if (xread(client->cache->fd, buff, sizeof(buff)) != sizeof(buff)) {
			printf("xread failed\n");
			return -1;
		}

		err = send_data(client->client_sock, buff, sizeof(buff), SEND_FLAGS);
		if (err < 0)
			return -1;
	}

	// sends the logs from cache:
	for (int i = 0; i < client->cache->logs; ++i) {
		src = client->cache->ptr + i * sizeof(struct client_logline);

		err = send_data(client->client_sock, src, sizeof(struct client_logline), SEND_FLAGS);
		if (err < 0)
			return -1;
	}

	return 0;
}

int logmemcache_send_stats_os(struct logmemcache_client_st *client)
{
	char stats[STATUS_MAX_SIZE];
	int err;
	size_t total_logs = client->cache->file_logs + client->cache->logs;

	sprintf(stats, STATS_FORMAT, client->cache->service_name,
			client->cache->pages * getpagesize(), total_logs);
	err = send_data(client->client_sock, stats, STATUS_MAX_SIZE, SEND_FLAGS);
	if (err < 0)
		return -1;
	
	return 0;
}

int logmemcache_flush_os(struct logmemcache_client_st *client)
{
	xwrite(client->cache->fd, client->cache->ptr,
				client->cache->logs * sizeof(struct client_logline));
	
	client->cache->file_logs += client->cache->logs;
	client->cache->logs = 0;
	return 0;
}

int logmemcache_unsubscribe_os(struct logmemcache_client_st *client)
{
	char buff_path[1000];
	snprintf(buff_path, sizeof(buff_path), "%s/%s.log", logfile_path, client->cache->service_name);

	free(client->cache->service_name);
	if (munmap(client->cache->ptr, getpagesize() * MAX_ALLOCATED_PAGES)) {
		printf("munmap failed\n");
	}
	close(client->cache->fd);
	free(client->cache);

	if (remove(buff_path)) {
		printf("remove fail\n");
		return -1;
	}

	return 0;
}



void mutex_init_os(pthread_mutex_t *lock)
{
	if (pthread_mutex_init(lock, NULL)) {
		printf("Error init mutex\n");
	}
}

void mutex_destroy_os(pthread_mutex_t *lock)
{
	if (pthread_mutex_destroy(lock)) {
		printf("Error destroy mutex\n");
	}	
}

void mutex_lock_os(pthread_mutex_t *lock)
{
	if (pthread_mutex_lock(lock)) {
		printf("Error lock mutex\n");
	}
}

void mutex_unlock_os(pthread_mutex_t *lock)
{
	if (pthread_mutex_unlock(lock)) {
		printf("Error unlock mutex\n");
	}
}