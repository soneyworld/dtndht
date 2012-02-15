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
#include "dht.h"
#include "config.h"
#include "dtndht.h"
#include "blacklist.h"
#include "rating.h"
#include "bootstrapping.h"
#include "utils.h"
#include "list.h"
#include <openssl/sha.h>
#include <netinet/in.h>
#ifdef WITH_DOUBLE_LOOKUP
#include "doublelookup.h"
#endif
#ifndef LOOKUP_THRESHOLD
#define LOOKUP_THRESHOLD 1800
#endif

#ifndef REANNOUNCE_THRESHOLD
#define REANNOUNCE_THRESHOLD 60
#endif

#define RATING

//#define REPORT_HASHES
//#define DEBUG_SAVING
//#define DEBUG

#ifdef DEBUG
#ifndef REPORT_HASHES
#define REPORT_HASHES
#endif
#endif

#ifdef REPORT_HASHES
static void printf_hash(const unsigned char *buf) {
	int i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
		printf("%02x", buf[i]);
	}
}
#endif

static struct list lookuptable;
static struct list announcetable;

int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port);

/* The call-back function is called by the DHT whenever something
 interesting happens.  Right now, it only happens when we get a new value or
 when a search completes, but this may be extended in future versions. */
static void callback(void *closure, int event, unsigned char *info_hash,
		void *data, size_t data_len, const struct sockaddr *from, int fromlen) {
	if (!(event == DHT_EVENT_VALUES || event == DHT_EVENT_VALUES6))
		return;
	int i;
	int count = 0;
	struct sockaddr_storage *ss;
#ifdef RATING
	int *ratings;
#endif
	struct dhtentry *entry;
	entry = getFromList(info_hash, &lookuptable);
	if (!entry) {
		// Not requested entry found -> blacklist sender
		if (blacklist_is_enabled()) {
			if (blacklist_id_blacklisted(info_hash)) {
				blacklist_blacklist_node(from, info_hash);
			}
		}
		return;
	}
	switch (event) {
	case DHT_EVENT_VALUES:
		count = data_len / 6;
		ss = (struct sockaddr_storage*) malloc(
				sizeof(struct sockaddr_storage) * count);
#ifdef RATING
		ratings = (int *) malloc(sizeof(int) * count);
		memset(ratings, 0, sizeof(int) * count);
#endif
		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 6), AF_INET);
#ifdef RATING
			entry->resultentries = get_rating(&ratings[i],
					entry->resultentries, ss, from, fromlen);
#endif
		}
		break;
	case DHT_EVENT_VALUES6:
		count = data_len / 18;
		ss = malloc(sizeof(struct sockaddr_storage) * count);
#ifdef RATING
		ratings = (int *) malloc(sizeof(int) * count);
		memset(ratings, 0, sizeof(int) * count);
#endif
		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 18), AF_INET6);
#ifdef RATING
			entry->resultentries = get_rating(&ratings[i],
					entry->resultentries, ss, from, fromlen);
#endif
		}
		break;
	}
	i = 0;
#ifdef RATING
	if (entry) {
		struct dhtentryresult * result = entry->resultentries;
		while (result) {
			if (result->rating >= 1) {
#else
				while(i<count) {
#endif
				char * buf;
				int port;
				switch (ss[i].ss_family) {
				case AF_INET:
					buf = (char*) malloc(INET_ADDRSTRLEN);
					inet_ntop(ss[i].ss_family,
							&((struct sockaddr_in *) &ss[i])->sin_addr, buf,
							INET_ADDRSTRLEN);
					port = ((struct sockaddr_in *) &ss[i])->sin_port;
					printf("sending dtn ping to host: %s %d\n", buf,
							ntohs(port));
					free(buf);
					break;
				case AF_INET6:
					buf = (char*) malloc(INET6_ADDRSTRLEN);
					inet_ntop(ss[i].ss_family,
							&((struct sockaddr_in6 *) &ss[i])->sin6_addr, buf,
							INET6_ADDRSTRLEN);
					port = ((struct sockaddr_in6 *) &ss[i])->sin6_port;
					printf("sending dtn ping to host: %s %d\n", buf,
							ntohs(port));
					free(buf);
					break;
				}
				dht_ping_dtn_node((struct sockaddr*) &ss[i], fromlen);
#ifdef RATING
			}
#endif
			i++;
#ifdef RATING
			result = result->next;
		}
	}
	free(ratings);
#else
	}
#endif
	free(ss);
}

int dtn_dht_initstruct(struct dtn_dht_context *ctx) {
	(*ctx).port = 9999;
	(*ctx).ipv4socket = -1;
	(*ctx).ipv6socket = -1;
	(*ctx).type = BINDBOTH;
	(*ctx).bind = NULL;
	(*ctx).bind6 = NULL;
	(*ctx).minimum_rating = 0;
	(*ctx).clayer = NULL;
	// generate ID
	return (RAND_bytes((*ctx).id, SHA_DIGEST_LENGTH) == 1);
}

int dtn_dht_init(struct dtn_dht_context *ctx) {
	lookuptable.head = NULL;
	announcetable.head = NULL;
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

int reannounceLists(struct dtn_dht_context *ctx) {
	reannounceList(ctx, &announcetable, REANNOUNCE_THRESHOLD);
}

int dtn_dht_uninit(void) {
	cleanUpList(&lookuptable, -1);
	cleanUpList(&announcetable, -1);
	blacklist_free();
	return dht_uninit();
}

int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep) {
	cleanUpList(&lookuptable, LOOKUP_THRESHOLD);
	if (dtn_dht_ready_for_work(ctx)) {
		reannounceLists(ctx);
	}
	return dht_periodic(buf, buflen, from, fromlen, tosleep, callback, NULL,
			ctx);
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
		return dht_search(id, port, AF_INET, callback, NULL);
		break;
	case IPV6ONLY:
		return dht_search(id, port, AF_INET6, callback, NULL);
		break;
	}
	return rc;
}

int dtn_dht_lookup(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, enum dtn_dht_lookup_type type) {
	if (!dtn_dht_ready_for_work(ctx))
		return 0;
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	struct dhtentry *entry = getFromList(key, &lookuptable);
	if (entry == NULL) {
		addToList(&lookuptable, key, type);
		return dtn_dht_search(ctx, key, 0);
	} else {
		entry->updatetime = time(NULL);
		return dtn_dht_search(ctx, key, 0);
	}
	return 1;
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, enum dtn_dht_lookup_type type) {
	dht_add_dtn_eid(eid, eidlen, type);
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	struct dhtentry *entry;
	entry = getFromList(key, &announcetable);
	if (entry == NULL) {
		addToList(&announcetable, key, type);
		if (!dtn_dht_ready_for_work(ctx))
			return 0;
		else {
			return dtn_dht_search(ctx, key, ctx->port);
		}
	} else {
		return 0;
	}
	return 1;
}

int dtn_dht_deannounce(const unsigned char *eid, size_t eidlen) {
	dht_remove_dtn_eid(eid, eidlen);
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	removeFromList(key, &announcetable);
	return 0;
}

void dtn_dht_blacklist(int enable) {
	blacklist_enable(enable);
}

/**
 * DIRECTLY WRAPPED FUNCTIONS
 */

int dht_blacklisted(const struct sockaddr *sa, int salen) {
	return blacklist_blacklisted(sa);
}

unsigned int dtn_dht_blacklisted_nodes(unsigned int *ipv4_return,
		unsigned int *ipv6_return) {
	return blacklist_size(ipv4_return, ipv6_return);
}

int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx, const char* name,
		const char* service) {
	return bootstrapping_dns(ctx, name, service);
}

void dtn_dht_start_random_lookup(struct dtn_dht_context *ctx) {
	bootstrapping_start_random_lookup(ctx, callback);
}

int dtn_dht_save_conf(const char *filename) {
	return bootstrapping_save_conf(filename);
}

int dtn_dht_load_prev_conf(struct dtn_dht_context *ctx, const char *filename) {
	return bootstrapping_load_conf(ctx, filename);
}

int dtn_dht_nodes(int af, int *good_return, int *dubious_return) {
	return dht_nodes(af, good_return, dubious_return, NULL, NULL);
}

