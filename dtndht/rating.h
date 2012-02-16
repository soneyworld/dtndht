/*
 * rating.h
 *
 *  Created on: 06.02.2012
 *      Author: Till Lorentzen
 */

#ifndef RATING_H_
#define RATING_H_
#include "config.h"
#include "bloom.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <openssl/sha.h>

struct dht_result_rating {
	struct sockaddr_storage *ss;
	BLOOM *frombloom;
	int rating;
	struct dht_result_rating *next;
};

struct dht_rating_entry {
	unsigned char key[SHA_DIGEST_LENGTH];
	time_t updated;
	struct dht_result_rating *ratings;
	struct dht_rating_entry *next;
};

/**
 * return the rating of the given sockaddr_storage as target for this id
 * this function adds the sender to a bloom filter to prevent double counting
 * If the head does not exist, it will be created and returned, otherwise the old head is returned
 */
int get_rating(unsigned char *info_hash, const struct sockaddr_storage *target, const struct sockaddr *from,
		int fromlen);
// Destroys the all ratings
void free_ratings();
#endif /* RATING_H_ */
