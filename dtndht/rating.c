/*
 * rating.c
 *
 *  Created on: 06.02.2012
 *      Author: Till Lorentzen
 */

#include "rating.h"
#include <time.h>

static struct dht_rating_entry * list = NULL;

unsigned int hash1(const char *key) {
	return (unsigned int) key[0];
}
unsigned int hash2(const char *key) {
	return (unsigned int) key[1];
}
unsigned int hash3(const char *key) {
	return (unsigned int) key[2];
}
unsigned int hash4(const char *key) {
	return (unsigned int) key[3];
}
unsigned int hash5(const char *key) {
	return (unsigned int) key[4];
}
unsigned int hash6(const char *key) {
	return (unsigned int) key[5];
}
unsigned int hash7(const char *key) {
	return (unsigned int) key[6];
}

int sockaddr_storage_equals(const struct sockaddr_storage *ss1,
		const struct sockaddr_storage *ss2) {
	if (ss1->ss_family != ss2->ss_family)
		return 0;
	if (ss1->ss_family == AF_INET) {
		struct sockaddr_in* p1 = (struct sockaddr_in*) ss1;
		struct sockaddr_in* p2 = (struct sockaddr_in*) ss2;
		if (p1->sin_port != p2->sin_port)
			return 0;
		return (memcmp(&p1->sin_addr, &p2->sin_addr, 4) == 0);
	} else if (ss1->ss_family == AF_INET6) {
		struct sockaddr_in6* p1 = (struct sockaddr_in6*) ss1;
		struct sockaddr_in6* p2 = (struct sockaddr_in6*) ss2;
		if (p1->sin6_port != p2->sin6_port)
			return 0;
		return (memcmp(&p1->sin6_addr, &p2->sin6_addr, 16) == 0);
	}
	return 0;
}

struct dht_result_rating * rating_create(const struct sockaddr_storage *ss,
		const struct sockaddr *from, int fromlen) {
	unsigned char md[SHA_DIGEST_LENGTH];
	struct dht_result_rating * result;
	result = (struct dht_result_rating*) malloc(
			sizeof(struct dht_result_rating));
	result->frombloom = bloom_create(96, 7, hash1, hash2, hash3, hash4, hash5,
			hash6, hash7);
	SHA1((const unsigned char*) from, fromlen, md);
	bloom_add(result->frombloom, (const char*) md);
	result->next = NULL;
	result->rating = 0;
	result->ss = malloc(sizeof(struct sockaddr_storage));
	result->ss->ss_family = ss->ss_family;
	if (ss->ss_family == AF_INET) {
		struct sockaddr_in* dest = (struct sockaddr_in*) result->ss;
		struct sockaddr_in* src = (struct sockaddr_in*) ss;
		dest->sin_port = src->sin_port;
		memcpy(&dest->sin_addr, &src->sin_addr, 4);
	} else if (ss->ss_family == AF_INET6) {
		struct sockaddr_in6* dest = (struct sockaddr_in6*) result->ss;
		struct sockaddr_in6* src = (struct sockaddr_in6*) ss;
		dest->sin6_port = src->sin6_port;
		memcpy(&dest->sin6_addr, &src->sin6_addr, 16);
	}
	return result;
}

void rating_create_entry(unsigned char *info_hash,
		const struct sockaddr_storage *target, const struct sockaddr *from,
		int fromlen) {
	struct dht_rating_entry * newentry = (struct dht_rating_entry*) malloc(
			sizeof(struct dht_rating_entry));
	newentry->next = list;
	newentry->updated = time(NULL);
	memcpy(newentry->key, info_hash, SHA_DIGEST_LENGTH);
	newentry->ratings = rating_create(target, from, fromlen);
	list = newentry;
}

int get_rating(unsigned char *info_hash, const struct sockaddr_storage *target,
		const struct sockaddr *from, int fromlen) {
	struct dht_rating_entry * entry = list;
	while (entry) {
		if (memcmp(entry->key, info_hash, SHA_DIGEST_LENGTH) == 0) {
			break;
		}
		entry = entry->next;
	}
	if (entry) {
		struct dht_result_rating * pos = entry->ratings;
		struct dht_result_rating * prev = NULL;
		while (pos) {
			if (sockaddr_storage_equals(pos->ss, target)) {
				entry->updated = time(NULL);
				if (pos->rating < 10) {
					unsigned char md[SHA_DIGEST_LENGTH];
					SHA1((const unsigned char*) from, fromlen, md);
					if (!bloom_check(pos->frombloom, (const char*) md)) {
						bloom_add(pos->frombloom,(const char*) md);
						pos->rating++;
					}
				}
				return pos->rating;
			}
			prev = pos;
			pos = pos->next;
		}
		prev->next = rating_create(target, from, fromlen);
		return 0;
	} else {
		rating_create_entry(info_hash, target, from, fromlen);
		return 0;
	}
}

void free_rating(struct dht_result_rating *head) {
	struct dht_result_rating * next;
	while (head) {
		next = head->next;
		bloom_destroy(head->frombloom);
		free(head->ss);
		free(head);
		head = next;
	}
}

void free_ratings() {
	struct dht_rating_entry * entry = list;
	struct dht_rating_entry * next = NULL;
	while (entry != NULL) {
		next = entry->next;
		free_rating(entry->ratings);
		free(entry);
		entry = next;
	}
}
