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
	struct dhtentry *next;
};

struct list {
	struct dhtentry *head;
};

void cleanUpList(struct list *table, int threshold);
void removeFromList(const unsigned char *key, struct list *table);
struct dhtentry* getFromList(const unsigned char *key, struct list *table);
void addToList(struct list *table, const unsigned char *key);
int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold);
// implementation is in dtndht.c
int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port);
#endif /* LIST_H_ */
