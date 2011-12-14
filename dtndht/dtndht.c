#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/rand.h>
#include <netdb.h>
#include "dht.h"
#include "dtndht.h"

/* The call-back function is called by the DHT whenever something
 interesting happens.  Right now, it only happens when we get a new value or
 when a search completes, but this may be extended in future versions. */
static void callback(void *closure, int event, unsigned char *info_hash,
		void *data, size_t data_len) {
	int ipversion;
	int addr_len;
	switch (event) {
	case DHT_EVENT_VALUES:
		ipversion = 4;
		break;
	case DHT_EVENT_VALUES6:
		ipversion = 6;
		break;
	default:
		return;
	}
	dtn_dht_handle_lookup_result("test", 5, "tcp", 4, ipversion, NULL, 0);
}

int dtn_dht_initstruct(struct dtn_dht_context *ctx) {
	(*ctx).port = 9999;
	(*ctx).ipv4socket = -1;
	(*ctx).ipv6socket = -1;
	(*ctx).type = BINDBOTH;
	// generate ID
	return (RAND_bytes((*ctx).id, 20) == 1);
}

int dtn_dht_init(struct dtn_dht_context *ctx) {
	return dht_init((*ctx).ipv4socket, (*ctx).ipv6socket, (*ctx).id,
			(unsigned char*) "JC\0\0");
}

int dtn_dht_init_sockets(struct dtn_dht_context *ctx) {
	if ((*ctx).port <= 0 || (*ctx).port >= 0x10000) {
		return -3;
	}
	int rc = 0;
	switch ((*ctx).type) {
	case BINDNONE:
		(*ctx).ipv4socket = -1;
		(*ctx).ipv6socket = -1;
		break;
	case IPV4ONLY:
		(*ctx).ipv4socket = socket(PF_INET, SOCK_DGRAM, 0);
		if ((*ctx).ipv4socket < 0) {
			perror("socket(IPv4)");
			rc = -1;
		}
		(*ctx).ipv6socket = -1;
		break;
	case IPV6ONLY:
		(*ctx).ipv4socket = -1;
		(*ctx).ipv6socket = socket(PF_INET6, SOCK_DGRAM, 0);
		if ((*ctx).ipv6socket < 0) {
			perror("socket(IPv6)");
			rc = -2;
		}
		break;
	case BINDBOTH:
		(*ctx).ipv4socket = socket(PF_INET, SOCK_DGRAM, 0);
		if ((*ctx).ipv4socket < 0) {
			perror("socket(IPv4)");
			rc = -1;
		}
		(*ctx).ipv6socket = socket(PF_INET6, SOCK_DGRAM, 0);
		if ((*ctx).ipv6socket < 0) {
			perror("socket(IPv6)");
			rc = -2;
		}
		break;
	}
	if (rc < 0) {
		return rc;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;

	if ((*ctx).ipv4socket >= 0) {
		sin.sin_port = htons((*ctx).port);
		rc = bind((*ctx).ipv4socket, (struct sockaddr*) &sin, sizeof(sin));
		if (rc < 0) {
			perror("bind(IPv4)");
			return rc;
		} else {
			rc = 0;
		}
	}
	if ((*ctx).ipv6socket >= 0) {
		int val = 1;
		rc = setsockopt((*ctx).ipv6socket, IPPROTO_IPV6, IPV6_V6ONLY,
				(char *) &val, sizeof(val));
		if (rc < 0) {
			perror("setsockopt(IPV6_V6ONLY)");
			return rc;
		} else {

			/* BEP-32 mandates that we should bind this socket to one of our
			 global IPv6 addresses.  In this function, this only
			 happens if the user used the bind option. */

			sin6.sin6_port = htons((*ctx).port);
			rc
					= bind((*ctx).ipv6socket, (struct sockaddr*) &sin6,
							sizeof(sin6));
			if (rc < 0) {
				perror("bind(IPv6)");
				return rc;
			} else {
				rc = 0;
			}
		}
	}
	return rc;
}

int dtn_dht_uninit(void) {
	return dht_uninit();
}

int dtn_dht_periodic(const void *buf, size_t buflen,
		const struct sockaddr *from, int fromlen, time_t *tosleep) {
	//TODO perform announcements of eid and group hashes
	return dht_periodic(buf, buflen, from, fromlen, tosleep, callback, NULL);
}

int dtn_dht_close_sockets(struct dtn_dht_context *ctx) {
	if ((*ctx).ipv4socket >= 0) {
		close((*ctx).ipv4socket);
	}
	if ((*ctx).ipv6socket >= 0) {
		close((*ctx).ipv6socket);
	}
	return 0;
}

int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx) {
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

	rc = getaddrinfo("dht.transmissionbt.com", "6881", &hints, &result);
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

int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port) {
	int rc = 0;
	switch ((*ctx).type) {
	case BINDBOTH:
		rc = dht_search(id, port, AF_INET, callback, NULL);
		if (rc < 0) {
			return rc;
		}
		return dht_search(id, port, AF_INET6, callback, NULL);
		break;
	case IPV4ONLY:
		return dht_search(id, port, AF_INET, callback, NULL);
		break;
	case IPV6ONLY:
		return dht_search(id, port, AF_INET6, callback, NULL);
		break;
	}
	return rc;
}

int dtn_dht_lookup(struct dtn_dht_context *ctx, const unsigned char *eid,
		int eidlen, const unsigned char *cltype, int cllen) {
	char key[20];
	dht_hash(key, 20, cltype, cllen, ":", 1, eid, eidlen);
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_lookup_group(struct dtn_dht_context *ctx, const unsigned char *eid,
		int eidlen, const unsigned char *cltype, int cllen) {
	char key[20];
	dht_hash(key, 20, cltype, cllen, ":g:", 3, eid, eidlen);
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port) {
	char key[20];
	dht_hash(key, 20, cltype, cllen, ":", 1, eid, eidlen);
	// TODO Save the announced stuff to reannounce storage
	return dtn_dht_search(ctx,key,port);
}

int dtn_dht_announce_neighbour(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port) {
	char key[20];
	dht_hash(key, 20, cltype, cllen, ":n:", 3, eid, eidlen);
	// TODO Save the announced stuff to reannounce storage
	return dtn_dht_search(ctx,key,port);
}

int dtn_dht_deannounce(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port){
	// TODO remove from reannounce storage
	return 0;
}

int dtn_dht_deannounce_neighbour(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port){
	// TODO remove from reannounce storage
	return 0;
}


