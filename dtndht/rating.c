/*
 * rating.c
 *
 *  Created on: 06.02.2012
 *      Author: Till Lorentzen
 */

#include "rating.h"
#include <openssl/sha.h>

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

struct dhtentryresult* rating_create(const struct sockaddr_storage *ss,
		const struct sockaddr *from, int fromlen) {
	unsigned char md[SHA_DIGEST_LENGTH];
	struct dhtentryresult * result;
	result = (struct dhtentryresult*) malloc(sizeof(struct dhtentryresult));
	result->frombloom = bloom_create(96, 7, hash1, hash2, hash3, hash4, hash5,
			hash6, hash7);
	SHA1((const unsigned char*) from, fromlen, md);
	bloom_add(result->frombloom, md);
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
#ifdef WITH_DOUBLE_LOOKUP
int get_rating_of_sockaddr_storage(struct dhtentryresult *head,
		const struct sockaddr_storage *ss) {
	struct dhtentryresult * result = head;
	while (result) {
		if (sockaddr_storage_equals(result->ss, ss)) {
			return result->rating;
		}
		result = result->next;
	}
	return -1;
}
#endif
struct dhtentryresult* get_rating(int * rating, struct dhtentryresult *head,
		const struct sockaddr_storage *ss, const struct sockaddr *from,
		int fromlen) {
	struct dhtentryresult * result = head;
	if (!result) {
		// Head is empty, create a new one
		result = rating_create(ss, from, fromlen);
		(*rating) = result->rating;
		return result;
	}
	unsigned char md[SHA_DIGEST_LENGTH];
	struct dhtentryresult * prev = NULL;
	while (result) {
		if (sockaddr_storage_equals(result->ss, ss)) {
			if (result->rating < 10) {
				SHA1((const unsigned char*) from, fromlen, md);
				if (!bloom_check(result->frombloom, md)) {
					bloom_add(result->frombloom, md);
					result->rating++;
				}
			}
			(*rating) = result->rating;
			return head;
		}
		prev = result;
		result = result->next;
	}
	// If not found yet, add to list!
	prev->next = rating_create(ss, from, fromlen);
	(*rating) = prev->next->rating;
	return head;
}

void free_rating(struct dhtentryresult *head) {
	struct dhtentryresult * next;
	while (head) {
		next = head->next;
		bloom_destroy(head->frombloom);
		free(head->ss);
		free(head);
		head = next;
	}
}
