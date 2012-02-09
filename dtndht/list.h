/*
 * list.h
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */
#include <sys/time.h>
#include "dtndht.h"
#include <openssl/sha.h>
#ifndef LIST_H_
#define LIST_H_

struct dhtentry {
	unsigned char md[SHA_DIGEST_LENGTH];
	time_t updatetime;
	struct dhtentryresult *resultentries;
	struct dhtentry *next;
};

struct list {
	struct dhtentry *head;
};

void cleanUpList(struct list *table, int threshold);
void removeFromList(const unsigned char *key, struct list *table);
struct dhtentry* getFromList(const unsigned char *key, struct list *table);
void addToList(struct list *table, const unsigned char *key,
		enum dtn_dht_lookup_type type);
int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold);

#endif /* LIST_H_ */
