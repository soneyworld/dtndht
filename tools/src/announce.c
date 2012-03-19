/*
 * This little program can be used to announce an EID by using the dtndht
 *
 * options:
 *  -4: Open IPv4 socket for DHT
 *  -6: Open IPv6 socket for DHT
 *  -c: Do not exit after successful announcement
 *  -p <int>: Set the udp port, which should be used by the DHT
 *  -a <EID>: The EID, which should be announced
 *
 *
 *  Created on: 19.03.2012
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

// announce_done is 1, if the dht has be called to announce the given EID, otherwise 0
static volatile sig_atomic_t exiting = 0, announce_done = 0;
// forces the DHT to shutdown and exits the program
static void sigexit(int signo) {
	exiting = 1;
}

/**
 * Initializes the Signals, send to the program
 *
 * SIGINT: exits the DHT and after a clean exit, the program exits too
 *
 */
static void init_signal(void) {
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&ss);
	sa.sa_handler = sigexit;
	sa.sa_mask = ss;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
}
// Last timestamp when bootstrapping has been done
static int _bootstrapped = 0;
// Flag, if the program should shutdown after a complete search
static int _shutdown_after_successful_announcement = 1;
// Context of the DHT, which is necessary for the library
static struct dtn_dht_context _context;

/**
 * Bootstraps the DHT
 */
void bootstrapping() {
	// Bootstrap only, if last time has been more the 30 seconds ago
	if (_bootstrapped > time(NULL) + 30) {
		return;
	}
	_bootstrapped = time(NULL);
	// Bootstrap by using a DNS request
	dtn_dht_dns_bootstrap(&_context, NULL, NULL);
}
/**
 * This function is called, if the search is done and marks to exit,
 * if a simple request and exit has been wished.
 */
void dtn_dht_operation_done(const unsigned char *info_hash) {
	if (_shutdown_after_successful_announcement)
		exiting = 1;
	return;
}
/**
 * Prints all received results
 */
void dtn_dht_handle_lookup_result(const struct dtn_dht_lookup_result *result) {
	return;
}

int main(int argc, char *argv[]) {
	int c, ipv4 = 0, ipv6 = 0;
	int port = 9999;
	opterr = 0;
	char *announcestr = NULL;
	// Reading the options from command line
	while ((c = getopt(argc, argv, "46cp:a:")) != -1) {
		switch (c) {
		case 'a':
			// Reading the EID, which should be announced
			announcestr = optarg;
			break;
		case 'p':
			// Reading port information
			port = atoi(optarg);
			if (port < 1) {
				fprintf(stderr, "Illegal port number %s\n", optarg);
				return -1;
			}
			break;
		case 'c':
			// If this flag is set, the program doesn't exit after a finished announcement
			_shutdown_after_successful_announcement = 0;
			break;
		case '4':
			// Enable IPv4 socket for the DHT
			ipv4 = 1;
			break;
		case '6':
			// Enable IPv6 socket for the DHT
			ipv6 = 1;
			break;
		case '?':
			// Prints out error message, if a wrong usage has been found
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
	// If no EID has been given, shutdown the program
	if (!announcestr) {
		fprintf(stderr, "Usage: dhtannounce [options] -a <eid>\n\n");
		return -1;
	}
	// Initializes the main needed variables
	// return code
	int rc = 0;
	// Shows, if the DHT is ready for requests
	int ready = 0;
	// Buffer for incoming udp messages
	unsigned char _buf[4096];
	// Initializes the dht context struct
	dtn_dht_initstruct(&_context);
	_context.port = port;
	// If IPv4 or IPv6 is set, only use this for the DHT, otherwise use both
	if (ipv4 && !ipv6) {
		_context.type = IPV4ONLY;
	} else if (!ipv4 && ipv6) {
		_context.type = IPV6ONLY;
	}
	// Open UDP sockets
	if (dtn_dht_init_sockets(&_context) != 0) {
		dtn_dht_close_sockets(&_context);
		return -1;
	}
	// Initialize the DHT with the given sockets and port
	dtn_dht_init(&_context);
	// Register for signals
	init_signal();

	// DHT main loop
	time_t tosleep = 0;
	struct sockaddr_storage from;
	socklen_t fromlen;
	int high_fd = MAX(_context.ipv4socket, _context.ipv6socket);
	while (!exiting) {
		// the # of nodes are necessary to know, if bootstrapping should be done
		int numberOfNodes = dtn_dht_nodes(AF_INET, NULL, NULL) + dtn_dht_nodes(
				AF_INET6, NULL, NULL);
		if (numberOfNodes == 0) {
			// Call bootstrapping function for getting contact to the DHT
			bootstrapping();
		}
		// Is the DHT ready for lookup request?
		ready = dtn_dht_ready_for_work(&_context);
		if ((ready && !announce_done)) {
			dtn_dht_announce(&_context, announcestr, strlen(announcestr), SINGLETON);
			announce_done = 1;
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
		// Start selecting on sockets
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
	// Shutdown the DHT
	dtn_dht_uninit();
	// Close all opened sockets
	dtn_dht_close_sockets(&_context);
	return 0;
}
