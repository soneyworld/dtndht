/*
 * list.c
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */
#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

void cleanUpList(struct list *table, int threshold) {
	int isempty = 1;
	struct dhtentry *pos = table->head;
	struct dhtentry *prev = NULL;
	struct dhtentry *next = NULL;
	while (pos != NULL) {
		if (time(NULL) > (pos->updatetime + threshold) && !pos->announce) {
			// Delete pos by changing pointer
			next = pos->next;
			if (prev != NULL) {
				prev->next = next;
			} else {
				table->head = next;
			}
			free(pos);
			pos = NULL;
		} else {
			isempty = 0;
		}
		if (prev)
			prev = prev->next;
		if (pos != NULL) {
			pos = pos->next;
		} else {
			pos = next;
		}
	}
	if (isempty) {
		table->head = NULL;
	}
}

int reannounceList(struct dtn_dht_context *ctx, struct list *table,
		int threshold) {
	int rc, result = 0;
	struct dhtentry *pos = table->head;
	while (pos != NULL) {
		time_t acttime = time(NULL);
		time_t lastupdate = pos->updatetime;
		if (pos->announce && (lastupdate + threshold) < acttime) {
			rc = dtn_dht_search(ctx, pos->md, ctx->port);
			if (rc < 0) {
				result--;
			} else {
				pos->updatetime = time(NULL);
			}
		}
		pos = pos->next;
	}
	return result;
}

int relookupList(struct dtn_dht_context *ctx, struct list *table) {
	int rc, result = 0;
	struct dhtentry *pos = table->head;
	while (pos != NULL) {
		if (pos->status == DONE_AND_READY_FOR_REPEAT) {
			rc = dtn_dht_search(ctx, pos->md, 0);
			if (rc < 0) {
				result--;
			} else {
				pos->status = SEARCHING;
				pos->updatetime = time(NULL);
			}
		}
		pos = pos->next;
	}
	return result;
}

struct dhtentry * addToList(struct list *table, const unsigned char *key) {
	struct dhtentry *newentry;
	newentry = (struct dhtentry*) malloc(sizeof(struct dhtentry));
	memcpy(newentry->md, key, SHA_DIGEST_LENGTH);
	newentry->next = table->head;
	newentry->updatetime = time(NULL);
	newentry->announce = 0;
	newentry->status = SEARCHING;
	table->head = newentry;
	return newentry;
}

void deactivateFromList(const unsigned char *key, struct list *table) {
	struct dhtentry *pos = getFromList(key, table);
	if (pos) {
		pos->announce = 0;
	}
}

struct dhtentry* getFromList(const unsigned char *key, struct list *table) {
	struct dhtentry *result = table->head;
	int n;
	while (result) {
		n = memcmp(key, result->md, SHA_DIGEST_LENGTH);
		if (n == 0) {
			return result;
		}
		result = result->next;
	}
	return NULL;
}

