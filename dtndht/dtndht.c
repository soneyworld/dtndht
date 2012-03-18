#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#ifdef HAVE_OPENSSL_RAND_H
#include <openssl/rand.h>
#endif
#include <netdb.h>
#include <stdint.h>
#include "dht.h"
#include "dtndht.h"
#include "bootstrapping.h"
#include "utils.h"
#include "list.h"
#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#else
#define SHA_DIGEST_LENGTH 20
#endif
#include <netinet/in.h>
#include <unistd.h>

#ifdef RATING_SUPPORT
#include "rating.h"
static size_t minimum_rating = 1;
#endif

#ifdef BLACKLIST_SUPPORT
#include "blacklist.h"
#endif

#ifdef EVALUATION
#include "evaluation.h"
#endif

#ifndef LOOKUP_THRESHOLD
#define LOOKUP_THRESHOLD 1800
#endif

#ifndef REANNOUNCE_THRESHOLD
#define REANNOUNCE_THRESHOLD 600
#endif

static struct list announcetable;
static struct list lookuptable;

int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port);

/* The call-back function is called by the DHT whenever something
 interesting happens.  Right now, it only happens when we get a new value or
 when a search completes, but this may be extended in future versions. */
static void callback(void *closure, int event, unsigned char *info_hash,
		void *data, size_t data_len, const struct sockaddr *from, int fromlen) {
	if (event == DHT_EVENT_SEARCH_DONE || event == DHT_EVENT_SEARCH_DONE6) {
		struct dhtentry* entry = getFromList(info_hash, &lookuptable);
		if (entry != NULL) {
#ifdef EVALUATION
			printf_evaluation_start();
			printf("SEARCH_DONE_EVENT HASH=");
			printf_hash(info_hash);
			printf("\n");
#endif
			switch (entry->status) {
			case SEARCHING:
				entry->status = DONE;
				break;
			case REPEAT_SEARCH:
				entry->status = DONE_AND_READY_FOR_REPEAT;
				break;
			default:
				break;
			}
		}
#ifdef EVALUATION
		else {
			entry = getFromList(info_hash, &announcetable);
			if (entry) {
				printf_evaluation_start();
				printf("ANNOUNCE_DONE_EVENT HASH=");
				printf_hash(info_hash);
				printf("\n");
			} else {
				printf_evaluation_start();
				printf("UNKNOWN_HASH_DONE_EVENT HASH=");
				printf_hash(info_hash);
				printf("\n");
			}
		}
#endif
		// Inform the user of the lib about the completeness of the search for this info_hash
		dtn_dht_operation_done(info_hash);
		return;
	}
	if (!(event == DHT_EVENT_VALUES || event == DHT_EVENT_VALUES6))
		return;
	int i;
	int count = 0;
	struct sockaddr_storage *ss;
#ifdef RATING_SUPPORT
	int *ratings;
#endif
#ifdef BLACKLIST_SUPPORT
	if (blacklist_is_enabled()) {
		if (blacklist_id_blacklisted(info_hash)) {
			blacklist_blacklist_node(from, info_hash);
			return;
		}
	}
#endif
	if (getFromList(info_hash, &lookuptable) == NULL) {
		return;
	}

	switch (event) {
	case DHT_EVENT_VALUES:
		count = data_len / 6;
		ss = (struct sockaddr_storage*) malloc(
				sizeof(struct sockaddr_storage) * count);
#ifdef RATING_SUPPORT
		ratings = (int *) malloc(sizeof(int) * count);
		memset(ratings, 0, sizeof(int) * count);
#endif
		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 6), AF_INET);
#ifdef RATING_SUPPORT
			ratings[i] = get_rating(info_hash, ss, from, fromlen);
#endif
		}
		break;
	case DHT_EVENT_VALUES6:
		count = data_len / 18;
		ss = malloc(sizeof(struct sockaddr_storage) * count);
#ifdef RATING_SUPPORT
		ratings = (int *) malloc(sizeof(int) * count);
		memset(ratings, 0, sizeof(int) * count);
#endif
		for (i = 0; i < count; i++) {
			ss[i].ss_family = AF_INET;
			cpyvaluetosocketstorage(&ss[i], data + (i * 18), AF_INET6);
#ifdef RATING_SUPPORT
			ratings[i] = get_rating(info_hash, ss, from, fromlen);
#endif
		}
		break;
	}
#ifdef EVALUATION
	printf_evaluation_start();
	printf("VALUES_FOUND_EVENT HASH=");
	printf_hash(info_hash);
	printf(" COUNT=%d FROM=", count);
	char * str;
	int fromport;
	switch (from->sa_family) {
	case AF_INET:
		str = (char*) malloc(INET_ADDRSTRLEN);
		inet_ntop(from->sa_family, &((struct sockaddr_in *) from)->sin_addr,
				str, INET_ADDRSTRLEN);
		fromport = ntohs(((struct sockaddr_in *) from)->sin_port);
		break;
	case AF_INET6:
		str = (char*) malloc(INET6_ADDRSTRLEN);
		inet_ntop(from->sa_family, &((struct sockaddr_in6 *) from)->sin6_addr,
				str, INET6_ADDRSTRLEN);
		fromport = ntohs(((struct sockaddr_in6 *) from)->sin6_port);
		break;
	default:
		fromport = -1;
		str = NULL;
	}
	if (str) {
		printf("%s PORT=%d\n", str, fromport);
		free(str);
	}
#endif
	for (i = 0; i < count; i++) {
#ifdef EVALUATION
		char * buf;
		int port;
		switch (ss[i].ss_family) {
		case AF_INET:
			buf = (char*) malloc(INET_ADDRSTRLEN);
			inet_ntop(ss[i].ss_family,
					&((struct sockaddr_in *) &ss[i])->sin_addr, buf,
					INET_ADDRSTRLEN);
			port = ((struct sockaddr_in *) &ss[i])->sin_port;
			printf("----> %s %d\n", buf, ntohs(port));
			free(buf);
			break;
		case AF_INET6:
			buf = (char*) malloc(INET6_ADDRSTRLEN);
			inet_ntop(ss[i].ss_family,
					&((struct sockaddr_in6 *) &ss[i])->sin6_addr, buf,
					INET6_ADDRSTRLEN);
			port = ((struct sockaddr_in6 *) &ss[i])->sin6_port;
			printf("----> %s %d\n", buf, ntohs(port));
			free(buf);
			break;
		}
#endif
#ifdef RATING_SUPPORT
		if (ratings[i] > 0 && (size_t) ratings[i] >= minimum_rating) {
#endif
			dht_ping_dtn_node((struct sockaddr*) &ss[i], fromlen);
#ifdef RATING_SUPPORT
	}
#endif
	}
#ifdef RATING_SUPPORT
	free(ratings);
#endif
	free(ss);
#ifdef EVALUATION
	printf("-------------------------------------------------------\n");
#endif
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
	return dht_random_bytes((*ctx).id, SHA_DIGEST_LENGTH);
}

int dtn_dht_init(struct dtn_dht_context *ctx) {
#ifdef EVALUATION
	printf_evaluation_start();
	printf("DHT_INIT IPV4=%d IPV6=%d PORT=%d ID=",(*ctx).ipv4socket, (*ctx).ipv6socket,(*ctx).port);
	printf_hash((*ctx).id);
#ifdef RATING_SUPPORT
	printf(" MINIMUM_RATING=%d",(*ctx).minimum_rating);
#endif
	printf("\n");
#endif
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	announcetable.head = NULL;
	lookuptable.head = NULL;
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
#ifdef EVALUATION
	printf_evaluation_start();
	printf("DHT_UNINIT\n");
#endif
#ifdef RATING_SUPPORT
	free_ratings();
#endif
	// switch all announcements off
	struct dhtentry* entry = announcetable.head;
	while (entry) {
		entry->announce = 0;
		entry = entry->next;
	}
	cleanUpList(&announcetable, -1);
	cleanUpList(&lookuptable, -1);
#ifdef BLACKLIST_SUPPORT
	blacklist_free();
#endif
	return dht_uninit();
}

int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep) {
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	if (dtn_dht_ready_for_work(ctx) >= 2) {
		reannounceList(ctx, &announcetable, REANNOUNCE_THRESHOLD);
		relookupList(ctx, &lookuptable);
	}
	cleanUpList(&lookuptable, LOOKUP_THRESHOLD);
	cleanUpList(&announcetable, LOOKUP_THRESHOLD);
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
#ifdef EVALUATION
	printf_evaluation_start();
	if (port > 0) {
		printf("DHT_SEARCH_AND_ANNOUNCE PORT=%d HASH=", port);
	} else {
		printf("DHT_SEARCH HASH=");
	}
	printf_hash(id);
	printf("\n");
#endif
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

int dtn_dht_lookup(struct dtn_dht_context *ctx, const char *eid, size_t eidlen) {
	if (!dtn_dht_ready_for_work(ctx))
		return 0;
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	struct dhtentry *entry;
	struct dhtentry *announce;
	entry = getFromList(key, &lookuptable);
	if (entry == NULL) {
		addToList(&lookuptable, key);
	} else {
		// Do not relookup, if the last lookup isn't finished
		if (entry->status == SEARCHING || entry->status == REPEAT_SEARCH) {
			entry->status = REPEAT_SEARCH;
			return 1;
		}
		entry->updatetime = time(NULL);
		entry->status = SEARCHING;
	}
	announce = getFromList(key, &announcetable);
	if (announce == NULL || !announce->announce) {
#ifdef EVALUATION
		printf_evaluation_start();
		printf("START_LOOKUP EID=%s HASH=", eid);
		printf_hash(key);
		printf("\n");
#endif
		return dtn_dht_search(ctx, key, 0);
	} else {
		return 1;
	}
}

int dtn_dht_announce(struct dtn_dht_context *ctx, const char *eid,
		size_t eidlen, enum dtn_dht_lookup_type type) {
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	dht_add_dtn_eid(eid, eidlen, type);
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	struct dhtentry *entry;
	entry = getFromList(key, &announcetable);
	if (entry == NULL) {
		entry = addToList(&announcetable, key);
		entry->announce = 1;
		if (!dtn_dht_ready_for_work(ctx)) {
			entry->updatetime = 0;
			return 0;
		} else {
#ifdef EVALUATION
			printf_evaluation_start();
			printf("START_ANNOUNCE EID=%s HASH=", eid);
			printf_hash(key);
			printf("\n");
#endif
			return dtn_dht_search(ctx, key, ctx->port);
		}
	} else {
		entry->announce = 1;
		return 0;
	}
}

int dtn_dht_deannounce(const char *eid, size_t eidlen) {
	dht_remove_dtn_eid(eid, eidlen);
	unsigned char key[SHA_DIGEST_LENGTH];
	dht_hash(key, SHA_DIGEST_LENGTH, eid, eidlen, "", 0, "", 0);
	deactivateFromList(key, &announcetable);
	return 0;
}

void dtn_dht_blacklist(int enable) {
#ifdef BLACKLIST_SUPPORT
	blacklist_enable(enable);
#endif
}

/**
 * DIRECTLY WRAPPED FUNCTIONS
 */

int dht_blacklisted(const struct sockaddr *sa, int salen) {
#ifdef BLACKLIST_SUPPORT
	return blacklist_blacklisted(sa);
#else
	return 0;
#endif
}

unsigned int dtn_dht_blacklisted_nodes(unsigned int *ipv4_return,
		unsigned int *ipv6_return) {
#ifdef BLACKLIST_SUPPORT
	return blacklist_size(ipv4_return, ipv6_return);
#else
	(*ipv4_return) = 0;
	(*ipv6_return) = 0;
	return 0;
#endif
}

int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx, const char* name,
		const char* service) {
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	return bootstrapping_dns(ctx, name, service);
}

void dtn_dht_start_random_lookup(struct dtn_dht_context *ctx) {
#ifdef RATING_SUPPORT
	minimum_rating = (*ctx).minimum_rating;
#endif
	bootstrapping_start_random_lookup(ctx, callback);
}

int dtn_dht_save_conf(const char *filename) {
	return bootstrapping_save_conf(filename);
}

int dtn_dht_load_prev_conf(const char *filename) {
	return bootstrapping_load_conf(filename);
}

int dtn_dht_nodes(int af, int *good_return, int *dubious_return) {
	return dht_nodes(af, good_return, dubious_return, NULL, NULL);
}

