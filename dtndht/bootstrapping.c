#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#else
#define SHA_DIGEST_LENGTH 20
#endif

#include "bootstrapping.h"
#include "dtndht.h"

static int dht_has_been_ready = 0;


int bootstrapping_dns(struct dtn_dht_context *ctx, const char* name,
		const char* service) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int error, rc = 0;

	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	switch ((*ctx).type) {
	case BINDBOTH:
		hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
		break;
	case IPV4ONLY:
		hints.ai_family = AF_INET; /* Allow IPv4 */
		break;
	case IPV6ONLY:
		hints.ai_family = AF_INET6; /* Allow IPv6 */
		break;
	case BINDNONE:
		return 0;
		break;
	}
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0; /* Any protocol */
	if (name != NULL) {
		if (service != NULL) {
			rc = getaddrinfo(name, service, &hints, &result);
		} else {
			rc = getaddrinfo(name, BOOTSTRAPPING_SERVICE, &hints, &result);
		}
	} else {
		rc = getaddrinfo(BOOTSTRAPPING_DOMAIN, BOOTSTRAPPING_SERVICE, &hints,
				&result);
	}
	if (rc != 0) {
		return rc;
	}
	/* getaddrinfo() returns a list of address structures. Pinging all.*/
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		error = dht_ping_node((struct sockaddr*) rp->ai_addr, rp->ai_addrlen);
		if (error < 0) {
			int portNumber;
			if (rp->ai_addr->sa_family == AF_INET) {
				struct sockaddr_in *ipv4 = (struct sockaddr_in *) rp->ai_addr;
				portNumber = ipv4->sin_port;
				printf("ERROR %d: host: %s : %d\n", error,
						inet_ntoa(ipv4->sin_addr), portNumber);
			} else {
				char straddr[INET6_ADDRSTRLEN];
				struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) rp->ai_addr;
				portNumber = ipv6->sin6_port;
				inet_ntop(AF_INET6, &ipv6->sin6_addr, straddr, sizeof(straddr));
				printf("ERROR %d: host: %s : %d\n", error, straddr, portNumber);
			}
			rc--;
		}
	}
	freeaddrinfo(result); /* No longer needed */
	return rc;
}

int bootstrapping_load_conf(const char *filename) {
	FILE *fp;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("cannot read the given file");
		return -1;
	}
	char separator[20];
	memset(separator, 0, 20);
	void *inputbuf;
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	memset(in.sin_zero, 0, 8);
	struct sockaddr_in6 in6;
	in6.sin6_family = AF_INET6;
	// 300 * ipv4 + separator + 300 * ipv6
	size_t inputlen = 300 * 6 + 20 + 300 * 18;
	inputbuf = malloc(inputlen);
	memset(inputbuf, 1, inputlen);
	inputlen = fread(inputbuf, 1, inputlen, fp);
	void * psep;
	psep = memchr(inputbuf, 0, inputlen - 19);
	while (psep != NULL) {
		if (memcmp(psep, separator, 20) == 0) {
			int i;
			int numberOfIPv4 = (psep - inputbuf) / 6;
			int numberOfIPv6 = (inputlen - ((psep - inputbuf) + 20)) / 18;
			for (i = 0; i < numberOfIPv4; i++) {
				memcpy(&(in.sin_addr), inputbuf + 6 * i, 4);
				memcpy(&(in.sin_port), inputbuf + 6 * i + 4, 2);
				dht_ping_node((struct sockaddr *) &in, sizeof(in));
			}
			for (i = 0; i < numberOfIPv6; i++) {
				memcpy(&(in6.sin6_addr),
						inputbuf + 18 * i + 20 + 6 * numberOfIPv4, 16);
				memcpy(&(in6.sin6_port),
						inputbuf + 18 * i + 16 + 20 + 6 * numberOfIPv4, 2);
				dht_ping_node((struct sockaddr *) &in6, sizeof(in6));
			}
#ifdef DEBUG_SAVING
			printf("Found %d ipv4 addresses\n", numberOfIPv4);
			printf("Found %d ipv6 addresses\n", numberOfIPv6);
#endif
			break;
		}
		psep = memchr(psep + 1, 0, inputlen - 19 - (psep - inputbuf));
	}
	free(inputbuf);
	return fclose(fp);
}

int bootstrapping_save_conf(const char *filename) {
	FILE *fp;
	fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("cannot open file to save actual nodes!");
		return -1;
	}
	struct sockaddr_in sins[300];
	struct sockaddr_in6 sins6[300];
	char separator[20];
	memset(separator, 0, 20);
	int i, written, to_be_written, num = 300, num6 = 300;
#ifndef DEBUG_SAVING
	dht_get_nodes(sins, &num, sins6, &num6);
#else

	int n = dht_get_nodes(sins, &num, sins6, &num6);
	printf("Saving %d (%d + %d) nodes\n", n, num, num6);
#endif
	for (i = 0; i < num; i++) {
		to_be_written = 4;
		written = fwrite(&(sins[i].sin_addr), 1, to_be_written, fp);
		if (written != to_be_written) {
			perror("failed to save actual ipv4 node");
			fclose(fp);
			return -1;
		}
		to_be_written = 2;
		written = fwrite(&(sins[i].sin_port), 1, to_be_written, fp);
		if (written != to_be_written) {
			perror("failed to save actual ipv4 port");
			fclose(fp);
			return -1;
		}
	}
	to_be_written = 20;
	written = fwrite(separator, 1, 20, fp);
	if (written != to_be_written) {
		perror("failed to write separator between nodes");
		fclose(fp);
		return -1;
	}
	for (i = 0; i < num6; i++) {
		to_be_written = 16;
		written = fwrite(&(sins6[i].sin6_addr), 1, to_be_written, fp);
		if (written != to_be_written) {
			perror("failed to save actual ipv6 node");
			fclose(fp);
			return -1;
		}
		to_be_written = 2;
		written = fwrite(&(sins6[i].sin6_port), 1, to_be_written, fp);
		if (written != to_be_written) {
			perror("failed to save actual ipv6 port");
			fclose(fp);
			return -1;
		}
	}
	return fclose(fp);
}

int dtn_dht_ready_for_work(struct dtn_dht_context *ctx) {
	int good = 0, good6 = 0, rc;
	rc = 0;
	if (dht_has_been_ready > 0) {
		rc = 1;
	}
	if ((*ctx).ipv4socket >= 0) {
		dht_nodes(AF_INET, &good, NULL, NULL, NULL);
		if (good >= BOOTSTRAPPING_SEARCH_THRESHOLD && good
				< DHT_READY_THRESHOLD) {
			rc = 2;
		}
	}
	if ((*ctx).ipv6socket >= 0) {
		dht_nodes(AF_INET6, &good6, NULL, NULL, NULL);
		if (good6 >= BOOTSTRAPPING_SEARCH_THRESHOLD && good6
				< DHT_READY_THRESHOLD) {
			rc = 2;
		}
	}
	if (good >= DHT_READY_THRESHOLD || good6 >= DHT_READY_THRESHOLD) {
		dht_has_been_ready = 1;
		return 3;
	} else {
		return rc;
	}
}

void bootstrapping_start_random_lookup(struct dtn_dht_context *ctx,
		dht_callback *callback) {
	unsigned char randomhash[SHA_DIGEST_LENGTH];
	dht_random_bytes(&randomhash, SHA_DIGEST_LENGTH);
#ifdef BLACKLIST_SUPPORT
	if (blacklist_is_enabled()) {
		blacklist_blacklist_id(randomhash);
	}
#endif

	if ((*ctx).ipv4socket >= 0) {
		dht_search(randomhash, 0, AF_INET, callback, NULL);
	}
	if ((*ctx).ipv6socket >= 0) {
		dht_search(randomhash, 0, AF_INET6, callback, NULL);
	}
}

int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen) {
	return dht_insert_node(id, sa, salen);
}

int dtn_dht_ping_node(struct sockaddr *sa, int salen) {
	return dht_ping_node(sa, salen);
}
