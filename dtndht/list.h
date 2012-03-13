/*
 * list.h
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */
#include <sys/time.h>
#include "dtndht.h"
#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#else
#define SHA_DIGEST_LENGTH 20
#endif
#ifndef LIST_H_
#define LIST_H_

enum dht_lookup_status {
	SEARCHING = 0, DONE = 1, REPEAT_SEARCH = 2, DONE_AND_READY_FOR_REPEAT = 3
};

struct dhtentry {
	unsigned char md[SHA_DIGEST_LENGTH];
	int announce;
	enum dht_lookup_status status;
	time_t updatetime;
	struct dhtentry *next;
};

struct list {
	struct dhtentry *head;
};
// removes all old entries which are update is longer ago than the threshold
void cleanUpList(struct list *table, int threshold);

// For deactivating an announcement, this method should be called.
// A simple remove would lead to problem on adding the same key again later
void deactivateFromList(const unsigned char *key, struct list *table);

// returns the entry from the table with the given key, or NULL if there is no entry with this key
struct dhtentry* getFromList(const unsigned char *key, struct list *table);

// creates a new entry and adds it to the list
struct dhtentry* addToList(struct list *table, const unsigned char *key);

// All entries in the table will be reannounced, if last update is farer as the threshold
int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold);

// All entries in the table with the status DONE_AND_READY_FOR_REPEAT will be searched on the DHT
int relookupList(struct dtn_dht_context *ctx, struct list *table);

// implementation is in dtndht.c
int dtn_dht_search(struct dtn_dht_context *ctx, const unsigned char *id,
		int port);
#endif /* LIST_H_ */
