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

struct dhtentryresult {
	struct sockaddr_storage *ss;
	BLOOM *frombloom;
	int rating;
	struct dhtentryresult *next;
};
#ifdef WITH_DOUBLE_LOOKUP
// Returns the rating of the given socket information
// If no information was found, it returns -1
int get_rating_of_sockaddr_storage(struct dhtentryresult *head,
		const struct sockaddr_storage *ss);
#endif
/**
 * writes the rating of the given sockaddr_storage
 * this function adds the sender to a bloom filter to prevent double counting
 * If the head does not exist, it will be created and returned, otherwise the old head is returned
 */
struct dhtentryresult* get_rating(int * rating, struct dhtentryresult *head,
		const struct sockaddr_storage *ss, const struct sockaddr *from,
		int fromlen);
// Destroys the rating
void free_rating(struct dhtentryresult *head);
#endif /* RATING_H_ */
