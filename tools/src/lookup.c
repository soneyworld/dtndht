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
#include <unistd.h>
#include <string.h>

#ifndef MAX
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

static volatile sig_atomic_t exiting = 0, relookup = 0;
static void sigexit(int signo) {
	exiting = 1;
}

static void sigrelookup(int signo) {
	relookup = 1;
}

static void init_signal(void) {
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&ss);
	sa.sa_handler = sigrelookup;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&ss);
	sa.sa_handler = sigexit;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

static int _bootstrapped = 0;
static int _shutdown_after_successful_lookup = 1;
static struct dtn_dht_context _context;
void bootstrapping() {
	if (_bootstrapped > time(NULL) + 30) {
		return;
	}
	_bootstrapped = time(NULL);
	dtn_dht_dns_bootstrap(&_context, NULL, NULL);
}

void dtn_dht_operation_done(const unsigned char *info_hash) {
	if (_shutdown_after_successful_lookup)
		exiting = 1;
	return;
}

void dtn_dht_handle_lookup_result(const struct dtn_dht_lookup_result *result) {
	unsigned int i, j;
	for (i = 0; i < result->eid->eidlen; i++) {
		printf("%c", result->eid->eid[i]);
	}
	printf(" groups={");
	// Group Support
	struct dtn_eid * group = result->groups;
	while (group) {
		for (i = 0; i < group->eidlen; i++) {
			printf("%c", group->eid[i]);
		}
		if (group->next != NULL)
			printf(" ");
		group = group->next;
	}
	printf("} neighbours={");
	// Group Support
	struct dtn_eid * neighbour = result->neighbours;
	while (neighbour) {
		for (i = 0; i < neighbour->eidlen; i++) {
			printf("%c", neighbour->eid[i]);
		}
		if (neighbour->next != NULL)
			printf(" ");
		neighbour = neighbour->next;
	}
	printf("} clayers={");
	struct dtn_convergence_layer * clayer = result->clayer;
	while (clayer) {
		printf("name=");
		for (i = 0; i < clayer->clnamelen; i++) {
			printf("%c", clayer->clname[i]);
		}
		printf(" args={");
		struct dtn_convergence_layer_arg *arg = clayer->args;
		while (arg) {
			for (j = 0; j < arg->keylen; j++) {
				printf("%c", arg->key[j]);
			}
			printf("=");
			for (j = 0; j < arg->valuelen; j++) {
				printf("%c", arg->value[j]);
			}
			if (arg->next)
				printf(" ");
			arg = arg->next;
		}
		printf("}");
		if (clayer->next)
			printf(" ");
		clayer = clayer->next;
	}
	printf("}\n");
	return;
}

int main(int argc, char *argv[]) {
	int c, ipv4 = 0, ipv6 = 0;
	int port = 9999;
	opterr = 0;
	char *lookupstr = NULL;
	while ((c = getopt(argc, argv, "46p:l:")) != -1) {
		switch (c) {
		case 'l':
			lookupstr = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			if (port < 1) {
				fprintf(stderr, "Illegal port number %s\n", optarg);
				return -1;
			}
			break;
		case '4':
			ipv4 = 1;
			break;
		case '6':
			ipv6 = 1;
			break;
		case '?':
			if (optopt == 'l' || optopt == 'p')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			return -1;
		}
	}
	if (!lookupstr) {
		fprintf(stderr, "Usage: dhtlookup [options] -l <eid>\n\n");
		return -1;
	}
	int rc = 0;
	int ready = 0;
	int lookup_done = 0;
	unsigned char _buf[4096];
	dtn_dht_initstruct(&_context);
	_context.port = port;
	if (ipv4 && !ipv6) {
		_context.type = IPV4ONLY;
	} else if (!ipv4 && ipv6) {
		_context.type = IPV6ONLY;
	}
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
		ready = dtn_dht_ready_for_work(&_context);
		if ((ready && !lookup_done) || relookup) {
			dtn_dht_lookup(&_context, lookupstr, strlen(lookupstr));
			lookup_done = 1;
			relookup = 0;
		}
		struct timeval tv;
		fd_set readfds;
		tv.tv_sec = tosleep;
		tv.tv_usec = random() % 1000000;
		FD_ZERO(&readfds);
		if (_context.ipv4socket >= 0)
			FD_SET(_context.ipv4socket, &readfds);
		if (_context.ipv6socket >= 0)
			FD_SET(_context.ipv6socket, &readfds);
		rc = select(high_fd + 1, &readfds, NULL, NULL, &tv);
		if (exiting)
			break;
		if (rc > 0) {
			fromlen = sizeof(from);
			if (_context.ipv4socket >= 0 && FD_ISSET(_context.ipv4socket,
					&readfds)) {
				rc = recvfrom(_context.ipv4socket, _buf, sizeof(_buf) - 1, 0,
						(struct sockaddr*) &from, &fromlen);
			} else if (_context.ipv6socket >= 0 && FD_ISSET(
					_context.ipv6socket, &readfds)) {
				rc = recvfrom(_context.ipv6socket, _buf, sizeof(_buf) - 1, 0,
						(struct sockaddr*) &from, &fromlen);
			}
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
