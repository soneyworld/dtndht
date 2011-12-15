#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/rand.h>
#include <netdb.h>
#include "dht.h"
#include "dtndht.h"

#ifndef LOOKUP_THRESHOLD
#define LOOKUP_THRESHOLD 1800
#endif

#ifndef REANNOUNCE_THRESHOLD
#define REANNOUNCE_THRESHOLD 60
#endif

#define DEBUG

struct dhtentry {
	unsigned char md[20];
	unsigned char *eid;
	size_t eidlen;
	unsigned char *cl;
	size_t cllen;
	time_t updatetime;
	int port;
	struct dhtentry *next;
};

static struct list {
	struct dhtentry *head;
} lookuptable, lookupgrouptable, announcetable, announceneigbourtable;

void cleanUpList(struct list *table, int threshold) {
	printf("CLEAN UP LIST\n");
	struct dhtentry *pos = table->head;
	struct dhtentry *prev = NULL;
	while (pos != NULL) {
		printf("--- CLEAN UP LIST \n");
		if (time() > (pos->updatetime + threshold)) {
			printf("--- DOING CLEANUP \n");
			if (pos->next) {
				prev->next = pos->next;
			} else {
				prev->next = NULL;
			}
			free(pos->cl);
			free(pos->eid);
			free(pos);
			pos = prev->next;
		} else {
			printf("--- SKIP CLEANUP \n");
			prev = pos;
			pos = pos->next;
		}
	}
}

int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold) {
	checkList(table);
	int rc = 0, result = 0, i = 0;
	struct dhtentry *pos = table->head;
	while (pos != NULL) {
		printf("REANNOUNCE %d\n", i);
		i++;
		time_t acttime = time();
		//		printf("got time: %d\n", (int) acttime);
		time_t lastupdate = pos->updatetime;
		//		printf("last update: %d\n", (int) lastupdate);
		if ((lastupdate + threshold) < acttime) {
			printf("--- REANNOUNCE\n");
			rc = dtn_dht_search(ctx, pos->md, pos->port);
			if (rc < 0) {
				result--;
			} else {
				pos->updatetime = time();
			}
		} else {
			printf("--- SKIPPED REANNOUNCE\n");
		}
		pos = pos->next;
	}
	return result;
}

void addToList(const unsigned char *key, const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port, struct list *table) {
	printf("Adding DEBUG: \n");
	int listsize = 0;
	struct dhtentry *newentry;
	struct dhtentry *pos;
	newentry = (struct dhtentry*) malloc(sizeof(struct dhtentry));
	memcpy(newentry->md, key, 20);
	newentry->eid = (unsigned char*) malloc(eidlen);
	memcpy(newentry->eid, eid, eidlen);
	newentry->eidlen = eidlen;
	newentry->cl = (unsigned char*) malloc(cllen);
	memcpy(newentry->cl, cltype, cllen);
	newentry->cllen = cllen;
	newentry->port = port;
	newentry->next = NULL;
	newentry->updatetime = time();
	if (table->head == NULL) {
		table->head = newentry;
	} else {
		listsize++;
		pos = table->head;
		while (pos->next != NULL) {
			listsize++;
			printf("Adding DEBUG: (%d)\n", listsize);
			pos = pos->next;
		}
		pos->next = newentry;
	}
	printf("Adding to list (size=%d)\n", listsize);
	checkList(table);
}

void checkList(struct list *table) {
	struct dhtentry *pos;
	pos = table->head;
	int i = 0;
	while (pos != NULL) {
		printf("%d PORT: %d\n", i, pos->port);
		printf("%d TIME: %d\n", i, pos->updatetime);
		pos = pos->next;
		i++;
	}
}

void removeFromList(const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port, struct list *table) {
	printf("Removing from list\n");
	struct dhtentry *pos = table->head;
	struct dhtentry *prev = NULL;
	int n;
	while (pos) {
		if ((strcmp(pos->eid, eid) == 0) && (strcmp(pos->cl, cltype) == 0)
				&& pos->port == port) {
			if (pos->next) {
				prev->next = pos->next;
			} else {
				prev->next = NULL;
			}
			free(pos->cl);
			free(pos->eid);
			free(pos);
			break;
		}
		prev = pos;
		pos = pos->next;
	}
}

struct dhtentry* getFromList(const unsigned char *key, struct list *table) {
	printf("Searching in list\n");
	struct dhtentry *result = table->head;
	int n;
	while (result) {
		n = memcmp(key, result->md, 20);
		if (n == 0) {
			printf("------------------> Result found in list\n");
			return result;
		}
		result = result->next;
	}
	return NULL;
}

/* The call-back function is called by the DHT whenever something
 interesting happens.  Right now, it only happens when we get a new value or
 when a search completes, but this may be extended in future versions. */
static void callback(void *closure, int event, unsigned char *info_hash,
		void *data, size_t data_len) {
	int ipversion;
	int addr_len, i;
	int count = 0;
	struct sockaddr_storage *ss;
	struct sockaddr_in6 *ipv6;
	struct sockaddr_in *ipv4;
	struct dhtentry *entry;
	switch (event) {
	case DHT_EVENT_VALUES:
		ipversion = 4;
		count = data_len / 6;
		ss = (struct sockaddr_storage*) malloc(
				sizeof(struct sockaddr_storage) * count);

		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 6), AF_INET);
		}
		break;
	case DHT_EVENT_VALUES6:
		ipversion = 6;
		count = data_len / 18;
		ss = malloc(sizeof(struct sockaddr_storage) * count);
		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 18), AF_INET6);
		}
		break;
	default:
		return;
	}
	printf("Incomming information");
	// Find the right informations
	entry = getFromList(info_hash, &lookuptable);
	if (entry) {
		printf("Calling event");
		dtn_dht_handle_lookup_result(entry->eid, entry->eidlen, entry->cl,
				entry->cllen, ipversion, ss, sizeof(struct sockaddr_storage),
				count);
	}
	entry = getFromList(info_hash, &lookupgrouptable);
	if (entry) {
		printf("Calling group event");
		dtn_dht_handle_lookup_group_result(entry->eid, entry->eidlen,
				entry->cl, entry->cllen, ipversion, ss,
				sizeof(struct sockaddr_storage), count);
	}
	free(ss);
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
	lookuptable.head = NULL;
	lookupgrouptable.head = NULL;
	announcetable.head = NULL;
	announceneigbourtable.head = NULL;
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
	cleanUpList(&lookuptable, 0);
	cleanUpList(&lookupgrouptable, 0);
	// TODO FREE ALL LISTS
	return dht_uninit();
}

int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep) {
	int rc = 0;
	cleanUpList(&lookuptable, LOOKUP_THRESHOLD);
	cleanUpList(&lookupgrouptable, LOOKUP_THRESHOLD);
	rc = reannounceList(ctx, &announcetable, REANNOUNCE_THRESHOLD);
	rc = reannounceList(ctx, &announceneigbourtable, REANNOUNCE_THRESHOLD);
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
	unsigned char key[20];
	dht_hash(key, 20, cltype, cllen, ":", 1, eid, eidlen);
#ifdef DEBUG
	printf("LOOKUP: ");
	int i, rc;
	for (i = 0; i < 20; i++)
		printf("%02x", key[i]);
	printf("\n");
#endif
	addToList(key, eid, eidlen, cltype, cllen, 0, &lookuptable);
	printf("LOOKUP saved\n");
	rc = dtn_dht_search(ctx, key, 0);
	printf("LOOKUP DONE\n");
	return rc;
}

int dtn_dht_lookup_group(struct dtn_dht_context *ctx, const unsigned char *eid,
		int eidlen, const unsigned char *cltype, int cllen) {
	unsigned char key[20];
	dht_hash(key, 20, cltype, cllen, ":g:", 3, eid, eidlen);
	addToList(key, eid, eidlen, cltype, cllen, 0, &lookupgrouptable);
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid,
		int eidlen, const unsigned char *cltype, int cllen, int port) {
	unsigned char key[20];
	int i;
	dht_hash(key, 20, cltype, cllen, ":", 1, eid, eidlen);
#ifdef DEBUG
	printf("----------------\n");
	for (i = 0; i < cllen; i++) {
		printf("%c", cltype[i]);
	}
	printf(":");
	for (i = 0; i < eidlen; i++) {
		printf("%c", eid[i]);
	}
	printf("\n----------------\n");
	printf("ANNOUNCE:\n");
	for (i = 0; i < 20; i++)
		printf("%02x", key[i]);
	printf("\n");
#endif
	addToList(key, eid, eidlen, cltype, cllen, port, &announcetable);
	return dtn_dht_search(ctx, key, port);
}

int dtn_dht_announce_neighbour(struct dtn_dht_context *ctx,
		const unsigned char *eid, int eidlen, const unsigned char *cltype,
		int cllen, int port) {
	unsigned char key[20];
	dht_hash(key, 20, cltype, cllen, ":n:", 3, eid, eidlen);
	addToList(key, eid, eidlen, cltype, cllen, port, &announceneigbourtable);
	return dtn_dht_search(ctx, key, port);
}

int dtn_dht_deannounce(const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port) {
	removeFromList(eid, eidlen, cltype, cllen, port, &announcetable);
	return 0;
}

int dtn_dht_deannounce_neighbour(const unsigned char *eid, int eidlen,
		const unsigned char *cltype, int cllen, int port) {
	removeFromList(eid, eidlen, cltype, cllen, port, &announceneigbourtable);
	return 0;
}

int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type) {
	if (type == AF_INET) {
		memcpy(&((struct sockaddr_in*) target)->sin_addr, value, 4);
		memcpy(&((struct sockaddr_in*) target)->sin_port, value + 4, 2);
	} else if (type == AF_INET6) {
		memcpy(&((struct sockaddr_in6*) target)->sin6_addr, value, 16);
		memcpy(&((struct sockaddr_in6*) target)->sin6_port, value + 16, 2);
	}
	return 0;
}

