/*
 * dtndht_test.c
 *
 *  Created on: 20.01.2012
 *      Author: Till Lorentzen
 */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

#include "dtndht/dtndht.h"

#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

static struct dtn_dht_context _context;
static int _foundNodes = 0;
#define MAX_NEIGHBORS 265
static int _neighbors[MAX_NEIGHBORS];
static time_t _neighbors_pinged = 0;
#define MAX_RANDOMLOOKUPS 40
static int _random_lookups = 0;
static int _announced = 0;

static int _interrupt_pipe[2];
static unsigned char _buf[4096];

static volatile sig_atomic_t dumping = 0, searching = 0, _exiting = 0,
		announcing = 0;

static void sigdump(int signo) {
	dumping = 1;
}

static void sigtest(int signo) {
	searching = 1;
}

static void sigexit(int signo) {
	_exiting = 1;
}

static void init_signals(void) {
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&ss);
	sa.sa_handler = sigdump;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&ss);
	sa.sa_handler = sigtest;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGUSR2, &sa, NULL);

	sigemptyset(&ss);
	sa.sa_handler = sigexit;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}

void ping_neighbors() {
	if (_neighbors_pinged < time(NULL)) {
		struct sockaddr_in ip4addr;
		struct sockaddr_in6 ip6addr;
		ip4addr.sin_family = AF_INET;
		ip6addr.sin6_family = AF_INET6;
		inet_pton(AF_INET, "127.0.0.1", &ip4addr.sin_addr);
		inet_pton(AF_INET6, "::1", &ip6addr.sin6_addr);
		int i;
		for (i = 0; i < MAX_NEIGHBORS; i++) {
			if (_neighbors[i] > 0) {
				ip4addr.sin_port = htons(_neighbors[i]);
				ip6addr.sin6_port = htons(_neighbors[i]);
				if (_context.ipv4socket >= 0)
					dtn_dht_ping_node((struct sockaddr*) &ip4addr,
							sizeof ip4addr);
				if (_context.ipv6socket >= 0)
					dtn_dht_ping_node((struct sockaddr*) &ip6addr,
							sizeof ip6addr);
			}
		}
		_neighbors_pinged = time(NULL);
	}
}

// Lookup of an eid was successful
void dtn_dht_handle_lookup_result(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int ipversion,
		struct sockaddr_storage *addr, size_t addrlen, size_t count) {
	if (count <= 0)
		return;
	printf("FOUND %s %s: ", eid, cltype);
	int i, port;
	for (i = 0; i < count; i++) {
		char *buf;
		switch (addr[i].ss_family) {
		case AF_INET:
			buf = (char*) malloc(INET_ADDRSTRLEN);
			inet_ntop(addr[i].ss_family,
					&((struct sockaddr_in *) &addr[i])->sin_addr, buf,
					INET_ADDRSTRLEN);
			port = ((struct sockaddr_in *) &addr[i])->sin_port;
			break;
		case AF_INET6:
			buf = (char*) malloc(INET6_ADDRSTRLEN);
			inet_ntop(addr[i].ss_family,
					&((struct sockaddr_in6 *) &addr[i])->sin6_addr, buf,
					INET6_ADDRSTRLEN);
			port = ((struct sockaddr_in6 *) &addr[i])->sin6_port;
			break;
		}
		printf(" %s %d ", buf, port);
		free(buf);
	}
	printf("\n");
}
// Lookup of a group was successful
void dtn_dht_handle_lookup_group_result(const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen,
		int ipversion, struct sockaddr_storage *addr, size_t addrlen,
		size_t count) {

}

void create_interupt_pipe() {
	int i;
	if (pipe(_interrupt_pipe) < 0)
		abort();
	for (i = 0; i < 2; i++) {
		int opts;
		opts = fcntl(_interrupt_pipe[i], F_GETFL);
		if (opts < 0) {
			abort();
		}
		opts |= O_NONBLOCK;
		if (fcntl(_interrupt_pipe[i], F_SETFL, opts) < 0) {
			abort();
		}
	}
}

int main(int argc, char **argv) {
	int i, rc;
	int opt;
	int quiet = 0, ipv4 = 1, ipv6 = 1;
	char buf[16];
	while (1) {
		opt = getopt(argc, argv, "q46");
		if (opt < 0)
			break;
		switch (opt) {
		case 'q':
			quiet = 1;
			break;
		case '4':
			ipv6 = 0;
			break;
		case '6':
			ipv4 = 0;
			break;
		default:
			goto usage;
		}
	}
	if (argc < 2)
		goto usage;
	i = optind;
	if (argc < i + 1)
		goto usage;
	// Creating interrupt pipe
	create_interupt_pipe();
	rc = dtn_dht_initstruct(&_context);
	_context.port = atoi(argv[i++]);
	if (_context.port <= 0 || _context.port >= 0x10000)
		abort();
	/* If you set dht_debug to a stream, every action taken by the DHT will
	 be logged. */
	if (!quiet)
		dht_debug = stdout;
	_context.bind = "127.0.0.1";
	_context.bind6 = "::1";
	if (!ipv4 && ipv6) {
		_context.type = IPV6ONLY;
	} else if (!ipv6 && ipv4) {
		_context.type = IPV4ONLY;
	} else if (!ipv4 && !ipv6) {
		abort();
	}
	if (dtn_dht_init_sockets(&_context) != 0) {
		dtn_dht_close_sockets(&_context);
		abort();
	}
	if (dtn_dht_init(&_context) < 0)
		abort();
	// Pinging all known neighbors
	int j;
	for (j = 0; j < MAX_NEIGHBORS; j++)
		_neighbors[j] = 0;
	j = 0;
	while (i < argc && j < MAX_NEIGHBORS) {
		_neighbors[j] = atoi(argv[i]);
		i++;
		j++;
	}
	init_signals();
	time_t tosleep = 0;
	struct sockaddr_storage from;
	socklen_t fromlen;

	// Main Loop
	while (!_exiting) {
		struct timeval tv;
		fd_set readfds;
		tv.tv_sec = tosleep;
		tv.tv_usec = random() % 1000000;
		int high_fd = _interrupt_pipe[0];
		FD_ZERO(&readfds);
		FD_SET(_interrupt_pipe[0], &readfds);
		if (_context.ipv4socket >= 0) {
			FD_SET(_context.ipv4socket, &readfds);
			high_fd = max(high_fd, _context.ipv4socket);
		}
		if (_context.ipv6socket >= 0) {
			FD_SET(_context.ipv6socket, &readfds);
			high_fd = max(high_fd, _context.ipv6socket);
		}
		rc = select(high_fd + 1, &readfds, NULL, NULL, &tv);
		if (rc < 0) {
			if (errno != EINTR) {
				sleep(1);
			}
		}
		if (FD_ISSET(_interrupt_pipe[0], &readfds)) {
			// this was an interrupt with the self-pipe-trick
			char buf[2];
			if (read(_interrupt_pipe[0], buf, 2) <= 2 || _exiting)
				break;
		}
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
		int numberOfHosts = 0;
		int numberOfGoodHosts = 0;
		int numberOfGood6Hosts = 0;
		unsigned int numberOfBlocksHosts = 0;
		unsigned int numberOfBlocksHostsIPv4 = 0;
		unsigned int numberOfBlocksHostsIPv6 = 0;
		if (_context.ipv4socket >= 0)
			numberOfHosts = dht_nodes(AF_INET, &numberOfGoodHosts, NULL, NULL,
					NULL);
		if (_context.ipv6socket >= 0)
			numberOfHosts += dht_nodes(AF_INET6, &numberOfGood6Hosts, NULL,
					NULL, NULL);
		numberOfBlocksHosts = dtn_dht_blacklisted_nodes(
				&numberOfBlocksHostsIPv4, &numberOfBlocksHostsIPv6);
		if (_foundNodes != numberOfHosts) {
			printf(
					"DHT Nodes available: %d (Good:%d+%d) Blocked: %d (%d+%d)\n",
					numberOfHosts, numberOfGoodHosts, numberOfGood6Hosts,
					numberOfBlocksHosts, numberOfBlocksHostsIPv4,
					numberOfBlocksHostsIPv6);
			_foundNodes = numberOfHosts;
		}
		if (_foundNodes <= 8 && _foundNodes >= 0) {
			ping_neighbors();
		}
		if (dtn_dht_ready_for_work(&_context) == 2 && _random_lookups
				< MAX_RANDOMLOOKUPS) {
			dtn_dht_start_random_lookup(&_context);
			_random_lookups++;
		}
		if (dtn_dht_ready_for_work(&_context)) {
			if (!_announced) {
				dtn_dht_announce(&_context, "dtn://test", 10, "TCP", 3,
						_context.port);
				dtn_dht_lookup(&_context, "dtn://test", 10, "TCP", 3);
				_announced = 1;
			}
			if (searching) {

				searching = 0;
			}
		}
		if (dumping) {
			dht_dump_tables(stdout);
			dumping = 0;
		}
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				printf("dtn_dht_periodic failed");
				if (rc == EINVAL || rc == EFAULT) {
					printf("DHT failed -> stopping DHT");
					break;
				}
				tosleep = 1;
			}
		}
	}
	if (_announced) {
		dtn_dht_deannounce("dtn://test", 10, "TCP", 3, _context.port);
	}
	dtn_dht_uninit();
	dtn_dht_close_sockets(&_context);
	return 0;

	usage: printf("Usage: dtndhttest [-q] [-4] [-6] port [port]...\n");
	exit(1);
}

