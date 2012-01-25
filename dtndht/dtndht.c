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
#include <stdint.h>
//#include "dht.h"
#include "dtndht.h"
#include "blacklist.h"
#include <openssl/sha.h>
#include <netinet/in.h>

#ifndef LOOKUP_THRESHOLD
#define LOOKUP_THRESHOLD 1800
#endif

#ifndef BOOTSTRAPPING_SEARCH_THRESHOLD
#define BOOTSTRAPPING_SEARCH_THRESHOLD 8
#define BOOTSTRAPPING_SEARCH_MAX_HASHES 40

int bootstrapping_hashes = 0;
#endif

#ifndef DHT_READY_THRESHOLD
#define DHT_READY_THRESHOLD 30
#endif

#ifndef REANNOUNCE_THRESHOLD
#define REANNOUNCE_THRESHOLD 600
#endif

#define BOOTSTRAPPING_DOMAIN "dtndht.ibr.cs.tu-bs.de"
#define BOOTSTRAPPING_SERVICE "6881"

//#define REPORT_HASHES
//#define DEBUG_SAVING
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
	for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
		printf("%02x", buf[i]);
	}
}
#endif

static struct list {
	struct dhtentry *head;
} lookuptable, lookupgrouptable, announcetable, announceneigbourtable;

static int dht_has_been_ready = 0;

struct blacklisted_id {
	unsigned char md[SHA_DIGEST_LENGTH];
	struct blacklisted_id *next;
};
static struct blacklisted_id *idblacklist = NULL;

static void callback(void *closure, int event, unsigned char *info_hash,
		void *data, size_t data_len, const struct sockaddr *from, int fromlen);

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
		printf_hash(pos->md);
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

void dtn_dht_start_random_lookup(struct dtn_dht_context *ctx) {
	unsigned char randomhash[20];
	dht_random_bytes(&randomhash, 20);
	struct blacklisted_id * bid = malloc(sizeof(struct blacklisted_id));
	memcpy(bid->md, randomhash, 20);
	if (idblacklist) {
		bid->next = idblacklist;
	} else {
		bid->next = NULL;
	}
	idblacklist = bid;
	if ((*ctx).ipv4socket >= 0) {
		dht_search(randomhash, 0, AF_INET, callback, NULL);
	}
	if ((*ctx).ipv6socket >= 0) {
		dht_search(randomhash, 0, AF_INET6, callback, NULL);
	}
}

void cleanUpList(struct list *table, int threshold) {
	int isempty = 1;
	struct dhtentry *head = table->head;
	struct dhtentry *pos = head;
	struct dhtentry *prev = NULL;
	while (pos != NULL) {
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
			isempty = 0;
		}
		prev = pos;
		if (pos != NULL) {
			pos = pos->next;
		}
	}
	if (isempty) {
		table->head = NULL;
	}
}

int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold) {
	int rc = 0, result = 0, i = 0;
	struct dhtentry *pos = table->head;
	while (pos != NULL) {
		i++;
		time_t acttime = time(NULL);
		time_t lastupdate = pos->updatetime;
		if ((lastupdate + threshold) < acttime) {
#ifdef DEBUG
			printf("--- REANNOUNCE PORT %d\n", pos->port);
			printf("--- REANNOUNCE KEY  ");
			printf_hash(pos->md);
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
#ifdef REPORT_HASHES
				printf(" REANNOUNCE DONE: ");
				printf_hash(pos->md);
				printf("\n");
#endif
				pos->updatetime = time(NULL);

			}
		}
		pos = pos->next;
	}
	return result;
}

void addToList(const unsigned char *key, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen, int port,
		struct list *table) {
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
	newentry->port = port;
	newentry->next = table->head;
	newentry->updatetime = time(NULL);
	table->head = newentry;
}

void removeFromList(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port, struct list *table) {
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
	struct dhtentry *result = table->head;
	int n;
	while (result) {
		n = memcmp(key, result->md, SHA_DIGEST_LENGTH);
		if (n == 0) {
#ifdef REPORT_HASHES
			printf("Search in list -> Result found: ");
			printf_hash(key);
			printf("\n");
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
		void *data, size_t data_len, const struct sockaddr *from, int fromlen) {
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
#ifdef REPORT_HASHES
		case DHT_EVENT_SEARCH_DONE6:
		case DHT_EVENT_SEARCH_DONE:
		entry = getFromList(info_hash, &lookuptable);
		if (entry) {

			printf("SEARCH DONE FOR LOOKUP: ");
			printf_hash(info_hash);
			printf(" EID: %s CL: %s PORT: %d\n", entry->eid, entry->cl,
					entry->port);
		} else {
			entry = getFromList(info_hash, &announcetable);
			if (entry) {
				printf("SEARCH DONE FOR ANNOUNCE: ");
				printf_hash(info_hash);
				printf(" EID: %s CL: %s PORT: %d\n", entry->eid, entry->cl,
						entry->port);
			}
		}
#endif
	default:
		return;
	}
	// Find the right informations
	entry = getFromList(info_hash, &lookuptable);
	if (entry) {
#ifdef DEBUG
		printf("Calling lookup result event\n");
#endif
		dtn_dht_handle_lookup_result(entry->eid, entry->eidlen, entry->cl,
				entry->cllen, ipversion, ss, sizeof(struct sockaddr_storage),
				count);
	}
	entry = getFromList(info_hash, &lookupgrouptable);
	if (entry) {
#ifdef DEBUG
		printf("Calling lookup group result event\n");
#endif
		dtn_dht_handle_lookup_group_result(entry->eid, entry->eidlen,
				entry->cl, entry->cllen, ipversion, ss,
				sizeof(struct sockaddr_storage), count);
	}
	struct blacklisted_id *bid;
	bid = idblacklist;
	while (bid) {
		if (memcmp(bid->md, info_hash, 20) == 0) {
			blacklist_blacklist_node(from, info_hash);
			break;
		}
		bid = bid->next;
	}
	free(ss);
}

int dtn_dht_initstruct(struct dtn_dht_context *ctx) {
	(*ctx).port = 9999;
	(*ctx).ipv4socket = -1;
	(*ctx).ipv6socket = -1;
	(*ctx).type = BINDBOTH;
	(*ctx).bind = NULL;
	(*ctx).bind6 = NULL;
	// generate ID
	return (RAND_bytes((*ctx).id, SHA_DIGEST_LENGTH) == 1);
}

int dtn_dht_init(struct dtn_dht_context *ctx) {
	lookuptable.head = NULL;
	lookupgrouptable.head = NULL;
	announcetable.head = NULL;
	announceneigbourtable.head = NULL;
	return dht_init((*ctx).ipv4socket, (*ctx).ipv6socket, (*ctx).id, NULL);
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
		if ((*ctx).bind != NULL) {
			char buf[16];
			rc = inet_pton(AF_INET, (*ctx).bind, buf);
			if (rc == 1) {
				memcpy(&sin.sin_addr, buf, 4);
			} else {
				perror("wrong bind interface: IPv4");
			}
		}
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
			if ((*ctx).bind6 != NULL) {
				char buf[16];
				rc = inet_pton(AF_INET6, (*ctx).bind6, buf);
				if (rc == 1) {
					memcpy(&sin6.sin6_addr, buf, 16);
				} else {
					perror("wrong bind interface: IPv6");
				}
			}
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
	if (dtn_dht_ready_for_work(ctx)) {
		reannounceList(ctx, &announcetable, REANNOUNCE_THRESHOLD);
		reannounceList(ctx, &announceneigbourtable, REANNOUNCE_THRESHOLD);
	}
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

int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx, const char* name,
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
	if (!dtn_dht_ready_for_work(ctx))
		return 0;
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":", 1, eid, eidlen);
#ifdef REPORT_HASHES
	printf("LOOKUP: ");
	printf_hash(key);
	printf("\n");
#endif
	struct dhtentry *entry = getFromList(key, &lookuptable);
	if (entry == NULL) {
		addToList(key, eid, eidlen, cltype, cllen, 0, &lookuptable);
		return dtn_dht_search(ctx, key, 0);
	} else {
		entry->updatetime = time(NULL);
		return dtn_dht_search(ctx, key, 0);
	}
	return 1;
}

int dtn_dht_lookup_group(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen) {
	if (!dtn_dht_ready_for_work(ctx))
		return 0;
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":g:", 3, eid, eidlen);
	struct dhtentry *entry = getFromList(key, &lookupgrouptable);
	if (entry == NULL) {
		addToList(key, eid, eidlen, cltype, cllen, 0, &lookupgrouptable);
	} else {
		entry->updatetime = time(NULL);
	}
#ifdef REPORT_HASHES
	printf("LOOKUP GROUP: ");
	printf_hash(key);
	printf("\n");
#endif
	return dtn_dht_search(ctx, key, 0);
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen, int port) {
	unsigned char key[SHA_DIGEST_LENGTH];
	struct dhtentry *entry;
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":", 1, eid, eidlen);
	entry = getFromList(key, &announcetable);
	if (entry == NULL) {
		addToList(key, eid, eidlen, cltype, cllen, port, &announcetable);
		if (!dtn_dht_ready_for_work(ctx))
			return 0;
		else {
#ifdef REPORT_HASHES
			int i;
			printf("ANNOUNCE: ");
			printf_hash(key);
			printf("\n");
#endif
			return dtn_dht_search(ctx, key, port);
		}
	} else {
		entry->updatetime = time(NULL);
		if (!dtn_dht_ready_for_work(ctx))
			return 0;
		else {
#ifdef REPORT_HASHES
			int i;
			printf("ANNOUNCE: ");
			printf_hash(key);
			printf("\n");
#endif
			return dtn_dht_search(ctx, key, port);
		}
	}
	return 1;
}

int dtn_dht_announce_neighbour(struct dtn_dht_context *ctx,
		const unsigned char *eid, size_t eidlen, const unsigned char *cltype,
		size_t cllen, int port) {
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, cltype, cllen, ":n:", 3, eid, eidlen);
	if (getFromList(key, &announceneigbourtable) == NULL)
		addToList(key, eid, eidlen, cltype, cllen, port, &announceneigbourtable);
	if (!dtn_dht_ready_for_work(ctx))
		return 0;
	else
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
		struct sockaddr_in* sa_in = (struct sockaddr_in*) target;
		sa_in->sin_port = 0;
		memcpy(&(sa_in->sin_addr), value, 4);
		memcpy(&(sa_in->sin_port), value + 4, 2);
		sa_in->sin_port = ntohs(sa_in->sin_port);
	} else if (type == AF_INET6) {
		struct sockaddr_in6* sa_in = (struct sockaddr_in6*) target;
		memcpy(&(sa_in->sin6_addr), value, 16);
		memcpy(&(sa_in->sin6_port), value + 16, 2);
		sa_in->sin6_port = ntohs(sa_in->sin6_port);
	}
	return 0;
}

int dtn_dht_save_conf(struct dtn_dht_context *ctx, const char *filename) {
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
	int i, written, to_be_written, n, num = 300, num6 = 300;
	n = dht_get_nodes(sins, &num, sins6, &num6);
#ifdef DEBUG_SAVING
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

int dtn_dht_load_prev_conf(struct dtn_dht_context *ctx, const char *filename) {
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
			struct sockaddr sa;
			int salen, i;
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

/**
 * DIRECTLY WRAPPED FUNCTIONS
 */

int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen) {
	return dht_insert_node(id, sa, salen);
}

int dtn_dht_ping_node(struct sockaddr *sa, int salen) {
	return dht_ping_node(sa, salen);
}

int dht_blacklisted(const struct sockaddr *sa, int salen) {
	return blacklist_blacklisted(sa);
}

unsigned int dtn_dht_blacklisted_nodes(unsigned int *ipv4_return,
		unsigned int *ipv6_return) {
	return blacklist_size(ipv4_return, ipv6_return);
}
