/*
 * utils.c
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */
#include "utils.h"
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>


int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type) {
	if (type == AF_INET) {
		struct sockaddr_in* sa_in = (struct sockaddr_in*) target;
		sa_in->sin_port = 0;
		memcpy(&(sa_in->sin_addr), value, 4);
		memcpy(&(sa_in->sin_port), value + 4, 2);
		sa_in->sin_port = ntohs(sa_in->sin_port);
	} else if (type == AF_INET6) {
		struct sockaddr_in6* sa_in = (struct sockaddr_in6*) target;
		memcpy(&(sa_in->sin6_addr), value, 16);
		memcpy(&(sa_in->sin6_port), value + 16, 2);
		sa_in->sin6_port = ntohs(sa_in->sin6_port);
	}
	return 0;
}

void dtn_dht_build_id_from_str(unsigned char *target, const char *s, size_t len) {
	dht_hash(target, SHA_DIGEST_LENGTH, (const unsigned char*) s, len, "", 0,
			"", 0);
}
