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
#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#else
#include "sha1.h"
#define SHA_CTX sha1_context
#define SHA_DIGEST_LENGTH 20
#define SHA1_Init( CTX ) \
        sha1_starts( (CTX) )
#define SHA1( BUF, LEN, OUT) \
		sha1((BUF), (unsigned char *)(LEN), (OUT))
#define SHA1_Update(  CTX, BUF, LEN ) \
        sha1_update( (CTX), (unsigned char *)(BUF), (LEN) )
#define SHA1_Final( OUT, CTX ) \
        sha1_finish( (CTX), (OUT) )
#endif

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
