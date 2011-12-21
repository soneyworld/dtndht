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
#include <openssl/sha.h>

#ifndef LOOKUP_THRESHOLD
#define LOOKUP_THRESHOLD 1800
#endif

#ifndef REANNOUNCE_THRESHOLD
#define REANNOUNCE_THRESHOLD 10
#endif

#define BOOTSTRAPPING_DOMAIN "dtndht.ibr.cs.tu-bs.de"
#define BOOTSTRAPPING_SERVICE "6881"

#define REPORT_HASHES

//#define DEBUG

#ifdef DEBUG
#ifndef REPORT_HASHES
#define REPORT_HASHES
#endif
#endif

struct dhtentry {
	unsigned char md[SHA_DIGEST_LENGTH];
	unsigned char *eid;
	size_t eidlen;
	unsigned char *cl;
	size_t cllen;
	time_t updatetime;
	int port;
	struct dhtentry *next;
};

#ifdef REPORT_HASHES
static void printf_hash(const unsigned char *buf) {
	int i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		printf("%02x", buf[i]);
}
#endif

static struct list {
	struct dhtentry *head;
} lookuptable, lookupgrouptable, announcetable, announceneigbourtable;
#ifdef DEBUG
void checkList(struct list *table) {
	struct dhtentry *pos;
	pos = table->head;
	int i, size = 0;
	while (pos != NULL) {
		printf("%d PORT: %d\n", size, pos->port);
		printf("%d TIME: %d\n", size, (int) pos->updatetime);
		printf("%d EID : %s\n", size, pos->eid);
		printf("%d CL  : %s\n", size, pos->cl);
		printf("%d KEY : ", size);
		printf_hex(pos->md, SHA_DIGEST_LENGTH);
		printf("\n");
		pos = pos->next;
		size++;
	}
}
#endif

int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type);
int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port);

void cleanUpList(struct list *table, int threshold) {
#ifdef DEBUG
	printf("CLEAN UP LIST\n");
	checkList(table);
#endif
	struct dhtentry *head = table->head;
	struct dhtentry *pos = head;
	struct dhtentry *prev = NULL;
	while (pos != NULL) {
#ifdef DEBUG
		printf("--- CLEAN UP LIST \n");
#endif
		if (time(NULL) > (pos->updatetime + threshold)) {
#ifdef DEBUG
			printf("--- DELETE ENTRY \n");
#endif
			if (prev != NULL) {
				if (pos->next) {
					prev->next = pos->next;
				} else {
					prev->next = NULL;
				}
			} else {
				table->head = NULL;
			}
			free(pos->cl);
			free(pos->eid);
			free(pos);
			if (prev != NULL) {
				pos = prev->next;
			} else {
				pos = NULL;
			}
		} else {
#ifdef DEBUG
			printf("--- SKIP ENTRY \n");
#endif
			prev = pos;
			pos = pos->next;
		}
	}
}

int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold) {
#ifdef DEBUG
	checkList(table);
#endif
	int rc = 0, result = 0, i = 0;
	struct dhtentry *pos = table->head;
	while (pos != NULL) {
#ifdef DEBUG
		printf("REANNOUNCE %d\n", i);
#endif
		i++;
		time_t acttime = time(NULL);
		//		printf("got time: %d\n", (int) acttime);
		time_t lastupdate = pos->updatetime;
		//		printf("last update: %d\n", (int) lastupdate);
		if ((lastupdate + threshold) < acttime) {
#ifdef DEBUG
			printf("--- REANNOUNCE PORT %d\n", pos->port);
			printf("--- REANNOUNCE KEY  ");
			printf_hex(pos->md, SHA_DIGEST_LENGTH);
			printf("\n");
			printf("--- REANNOUNCE TIME %d %d\n", (int) pos->updatetime,
					(int) lastupdate);
#endif
			rc = dtn_dht_search(ctx, pos->md, pos->port);
			if (rc < 0) {
#ifdef DEBUG
				printf("--- REANNOUNCE FAILED\n");
#endif
				result--;
			} else {
#ifdef DEBUG
				printf("--- REANNOUNCE DONE: \n");
#endif
				//TODO MEMORY ACCESS IS WRONG!!!!
				pos->updatetime = time(NULL);

			}
		} else {
#ifdef DEBUG
			printf("--- SKIPPED REANNOUNCE\n");
#endif
		}
		pos = pos->next;
	}
	return result;
}

void addToList(const unsigned char *key, const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port, struct list *table) {
#ifdef DEBUG
	printf("Adding: %s with CL %s\n", eid, cltype);
#endif
	struct dhtentry *newentry;
	newentry = (struct dhtentry*) malloc(sizeof(struct dhtentry));
	memcpy(newentry->md, key, SHA_DIGEST_LENGTH);
	newentry->eid = (unsigned char*) malloc(eidlen + 1);
	newentry->eid[eidlen] = '\0';
	strncpy((char*) newentry->eid, (const char*) eid, eidlen);
	newentry->eidlen = eidlen;
	newentry->cl = (unsigned char*) malloc(cllen + 1);
	strncpy((char*) newentry->cl, (const char*) cltype, cllen);
	newentry->cl[cllen] = '\0';
	newentry->cllen = cllen;
#ifdef DEBUG
	printf("   %s\n   %s\n", cltype, newentry->cl);
#endif
	newentry->port = port;
	newentry->next = table->head;
	newentry->updatetime = time(NULL);
	table->head = newentry;
#ifdef DEBUG
	checkList(table);
#endif
}

void removeFromList(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port, struct list *table) {
#ifdef DEBUG
	printf("Removing from list\n");
#endif
	struct dhtentry *pos = table->head;
	struct dhtentry *prev = NULL;
	while (pos) {
		if (pos->port == port && pos->eidlen == eidlen && pos->cllen == cllen) {
			if ((strcmp((char*) pos->eid, (const char*) eid) == 0) && (strcmp(
					(char*) pos->cl, (const char*) cltype) == 0)) {
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
		}
		prev = pos;
		pos = pos->next;
	}
}

struct dhtentry* getFromList(const unsigned char *key, struct list *table) {
#ifdef DEBUG
	printf("Searching in list\n");
#endif
	struct dhtentry *result = table->head;
	int n;
	while (result) {
		n = memcmp(key, result->md, SHA_DIGEST_LENGTH);
		if (n == 0) {
#ifdef DEBUG
			printf("------------------> Result found in list\n");
#endif
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
	int i;
	int count = 0;
	struct sockaddr_storage *ss;
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
#ifdef DEBUG
	printf("Incoming information\n");
#endif
	// Find the right informations
	entry = getFromList(info_hash, &lookuptable);
	if (entry) {
#ifdef DEBUG
		printf("Calling event\n");
#endif
		dtn_dht_handle_lookup_result(entry->eid, entry->eidlen, entry->cl,
				entry->cllen, ipversion, ss, sizeof(struct sockaddr_storage),
				count);
	}
	entry = getFromList(info_hash, &lookupgrouptable);
	if (entry) {
#ifdef DEBUG
		printf("Calling group event\n");
#endif
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
	return (RAND_bytes((*ctx).id, SHA_DIGEST_LENGTH) == 1);
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
	cleanUpList(&lookuptable, -1);
	cleanUpList(&lookupgrouptable, -1);
	cleanUpList(&announcetable, -1);
	cleanUpList(&announceneigbourtable, -1);
	return dht_uninit();
}

int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep) {
	cleanUpList(&lookuptable, LOOKUP_THRESHOLD);
	cleanUpList(&lookupgrouptable, LOOKUP_THRESHOLD);
	reannounceList(ctx, &announcetable, REANNOUNCE_THRESHOLD);
	reannounceList(ctx, &announceneigbourtable, REANNOUNCE_THRESHOLD);
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

	rc = getaddrinfo(BOOTSTRAPPING_DOMAIN, BOOTSTRAPPING_SERVICE, &hints,
			&result);
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
	switch (ctx->type) {
	case BINDBOTH:
		rc = dht_search(id, port, AF_INET, callback, NULL);
		if (rc < 0) {
			return rc;
		}
		return dht_search(id, port, AF_INET6, callback, NULL);
		break;
	case IPV4ONLY:
#ifdef DEBUG
		printf("SEARCHING IPV4\n");
#endif
		return dht_search(id, port, AF_INET, callback, NULL);
		break;
	case IPV6ONLY:
		return dht_search(id, port, AF_INET6, callback, NULL);
		break;
	}
	return rc;
}

int dtn_dht_lookup(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen) {
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":", 1, eid, eidlen);
#ifdef REPORT_HASHES
	printf("LOOKUP: ");
	printf_hash(key);
	printf("\n");
#endif
	addToList(key, eid, eidlen, cltype, cllen, 0, &lookuptable);
#ifdef DEBUG
	printf("LOOKUP saved\n");
#endif
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_lookup_group(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen) {
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":g:", 3, eid, eidlen);
#ifdef REPORT_HASHES
	printf("LOOKUP GROUP: ");
	int i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		printf("%02x", key[i]);
	printf("\n");
#endif
	struct dhtentry *entry = getFromList(key, &lookupgrouptable);
	if (entry == NULL) {
		addToList(key, eid, eidlen, cltype, cllen, 0, &lookupgrouptable);
	} else {
		entry->updatetime = time(NULL);
	}
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen, int port) {
	unsigned char key[SHA_DIGEST_LENGTH];
	struct dhtentry *entry;
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":", 1, eid, eidlen);
#ifdef REPORT_HASHES
	int i;
	printf("ANNOUNCE: ");
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		printf("%02x", key[i]);
	printf("\n");
#endif
	entry = getFromList(key, &announcetable);
	if (entry == NULL) {
		addToList(key, eid, eidlen, cltype, cllen, port, &announcetable);
	} else {
		entry->updatetime = time(NULL);
	}
	return dtn_dht_search(ctx, key, port);
}

int dtn_dht_announce_neighbour(struct dtn_dht_context *ctx,
		const unsigned char *eid, size_t eidlen, const unsigned char *cltype,
		size_t cllen, int port) {
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":n:", 3, eid, eidlen);
	if (getFromList(key, &announceneigbourtable) == NULL)
		addToList(key, eid, eidlen, cltype, cllen, port, &announceneigbourtable);
	return dtn_dht_search(ctx, key, port);
}

int dtn_dht_deannounce(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port) {
	removeFromList(eid, eidlen, cltype, cllen, port, &announcetable);
	return 0;
}

int dtn_dht_deannounce_neighbour(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port) {
	removeFromList(eid, eidlen, cltype, cllen, port, &announceneigbourtable);
	return 0;
}

void dtn_dht_build_id_from_str(unsigned char *target, const char *s, size_t len) {
	dht_hash(target, SHA_DIGEST_LENGTH, (const unsigned char*) s, len, "", 0,
			"", 0);
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

/**
 * DIRECTLY WRAPPED FUNCTIONS
 */

int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen) {
	return dht_insert_node(id, sa, salen);
}

int dtn_dht_ping_node(struct sockaddr *sa, int salen) {
	return dht_ping_node(sa, salen);
}
