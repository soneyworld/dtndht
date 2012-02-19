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
	struct dhtentry *head = table->head;
	struct dhtentry *pos = head;
	struct dhtentry *prev = NULL;
	while (pos != NULL) {
		if (time(NULL) > (pos->updatetime + threshold) && !pos->announce) {
			if (prev != NULL) {
				if (pos->next) {
					prev->next = pos->next;
				} else {
					prev->next = NULL;
				}
			} else {
				table->head = NULL;
			}
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

struct dhtentry * addToList(struct list *table, const unsigned char *key) {
	struct dhtentry *newentry;
	newentry = (struct dhtentry*) malloc(sizeof(struct dhtentry));
	memcpy(newentry->md, key, SHA_DIGEST_LENGTH);
	newentry->next = table->head;
	newentry->updatetime = time(NULL);
	newentry->announce = 0;
	table->head = newentry;
	return newentry;
}

void deactivateFromList(const unsigned char *key, struct list *table) {
	struct dhtentry *pos = getFromList(key,table);
	if(pos){
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

