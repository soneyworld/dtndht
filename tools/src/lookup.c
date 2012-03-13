/*
 * lookup.c
 *
 *  Created on: 12.03.2012
 *      Author: Till Lorentzen
 */

#include "dtndht/dtndht.h"
#include <sys/signal.h>
#include <stdio.h>
#include <time.h>

#ifndef MAX
	#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

static volatile sig_atomic_t exiting = 0;
static void sigexit(int signo) {
	exiting = 1;
}

static void init_signal(void) {
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&ss);
	sa.sa_handler = sigexit;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

static int _bootstrapped = 0;
static struct dtn_dht_context _context;
void bootstrapping() {
	if (_bootstrapped > time(NULL) + 30) {
		return;
	}
	_bootstrapped = time(NULL);
	dtn_dht_dns_bootstrap(&_context, NULL, NULL);
}

void dtn_dht_handle_lookup_result(const struct dtn_dht_lookup_result *result) {
	unsigned int i;
	for (i = 0; i < result->eid->eidlen; i++) {
		printf("%c",result->eid->eid[i]);
	}
	printf(" ");

	struct dtn_eid * group = result->groups;
	while(group){
		group = group->next;
	}
	printf("\n");
	exiting = 1;
	return;
}

int main(void) {
	int rc = 0;
	unsigned char _buf[4096];
	dtn_dht_initstruct(&_context);
	if (dtn_dht_init_sockets(&_context) != 0) {
		dtn_dht_close_sockets(&_context);
		return -1;
	}
	dtn_dht_init(&_context);

	init_signal();
	// DHT main loop
	time_t tosleep = 0;
	struct sockaddr_storage from;
	socklen_t fromlen;
	int high_fd = MAX(_context.ipv4socket, _context.ipv6socket);
	while (!exiting) {
		int numberOfNodes = dtn_dht_nodes(AF_INET, NULL, NULL) + dtn_dht_nodes(
				AF_INET6, NULL, NULL);
		if (numberOfNodes == 0) {
			bootstrapping();
		}
		struct timeval tv;
		fd_set readfds;
		tv.tv_sec = tosleep;
		tv.tv_usec = random() % 1000000;
		FD_ZERO(&readfds);
		rc = select(high_fd + 1, &readfds, NULL, NULL, &tv);
		if (exiting)
			break;
		if (rc > 0) {
			fromlen = sizeof(from);
			if (_context.ipv4socket >= 0 && FD_ISSET(_context.ipv4socket,
					&readfds))
				rc = recvfrom(_context.ipv4socket, _buf, sizeof(_buf) - 1, 0,
						(struct sockaddr*) &from, &fromlen);
			else if (_context.ipv6socket >= 0 && FD_ISSET(_context.ipv6socket,
					&readfds))
				rc = recvfrom(_context.ipv6socket, _buf, sizeof(_buf) - 1, 0,
						(struct sockaddr*) &from, &fromlen);
		}
		if (rc > 0) {
			_buf[rc] = '\0';
			rc = dtn_dht_periodic(&_context, _buf, rc,
					(struct sockaddr*) &from, fromlen, &tosleep);
		} else {
			rc = dtn_dht_periodic(&_context, NULL, 0, NULL, 0, &tosleep);
		}
	}
	dtn_dht_uninit();
	dtn_dht_close_sockets(&_context);
	return 0;
}
