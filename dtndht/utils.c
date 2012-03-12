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
#include <openssl/rand.h>
#include <openssl/sha.h>
#include "dht.h"

int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type) {
	if (type == AF_INET) {
		struct sockaddr_in* sa_in = (struct sockaddr_in*) target;
		sa_in->sin_port = 0;
		memcpy(&(sa_in->sin_addr), value, 4);
		memcpy(&(sa_in->sin_port), value + 4, 2);
	} else if (type == AF_INET6) {
		struct sockaddr_in6* sa_in = (struct sockaddr_in6*) target;
		sa_in->sin6_port = 0;
		memcpy(&(sa_in->sin6_addr), value, 16);
		memcpy(&(sa_in->sin6_port), value + 16, 2);
	}
	return 0;
}

void dtn_dht_build_id_from_str(unsigned char *target, const char *s, size_t len) {
	dht_hash(target, SHA_DIGEST_LENGTH, (const unsigned char*) s, len, "", 0,
			"", 0);
}

// Map the function available by using the API to the internal used function
void dtn_dht_free_convergence_layer_struct(struct dtn_convergence_layer *clayer) {
	free_convergence_layer(clayer);
}

struct dtn_dht_lookup_result * create_dtn_dht_lookup_result(void) {
	struct dtn_dht_lookup_result * result =
			(struct dtn_dht_lookup_result*) malloc(
					sizeof(struct dtn_dht_lookup_result));
	result->clayer = NULL;
	result->eid = NULL;
	result->groups = NULL;
	result->neighbours = NULL;
	return result;
}

void free_dtn_dht_lookup_result(struct dtn_dht_lookup_result * result) {
	if (result) {
		free_dtn_eid(result->eid);
		free_dtn_eid(result->groups);
		free_dtn_eid(result->neighbours);
		free_convergence_layer(result->clayer);
		free(result);
	}
}

struct dtn_eid * create_dtn_eid(void) {
	struct dtn_eid * result = (struct dtn_eid*) malloc(sizeof(struct dtn_eid));
	result->eid = NULL;
	result->eidlen = 0;
	result->next = NULL;
	return result;
}
void free_dtn_eid(struct dtn_eid * eid) {
	if (eid) {
		// recursive call of this function for the next element in list
		free_dtn_eid(eid->next);
		if (eid->eid)
			free(eid->eid);
		free(eid);
		eid = NULL;
	}
}

struct dtn_convergence_layer_arg * create_convergence_layer_arg(void) {
	struct dtn_convergence_layer_arg * result =
			(struct dtn_convergence_layer_arg*) malloc(
					sizeof(struct dtn_convergence_layer_arg));
	result->key = NULL;
	result->keylen = 0;
	result->value = NULL;
	result->valuelen = 0;
	result ->next = NULL;
	return result;
}

void free_convergence_layer_arg(struct dtn_convergence_layer_arg * arg) {
	if (arg) {
		// recursive call of this function for the next element in list
		free_convergence_layer_arg(arg->next);
		if (arg->key)
			free(arg->key);
		if (arg->value)
			free(arg->value);
		free(arg);
		arg = NULL;
	}
}

struct dtn_convergence_layer * create_convergence_layer(void) {
	struct dtn_convergence_layer * result =
			(struct dtn_convergence_layer*) malloc(
					sizeof(struct dtn_convergence_layer));
	result->args = NULL;
	result->clname = NULL;
	result->clnamelen = 0;
	result->next = NULL;
	return result;
}

void free_convergence_layer(struct dtn_convergence_layer * layer) {
	if (layer) {
		// recursive call of this function for the next layer in list
		free_convergence_layer(layer->next);
		if (layer->clname)
			free(layer->clname);
		// frees all arguments of this convergence layer
		free_convergence_layer_arg(layer->args);
		free(layer);
		layer = NULL;
	}
}

/* Functions called by the DHT. */

void dht_hash(void *hash_return, int hash_size, const void *v1, int len1,
		const void *v2, int len2, const void *v3, int len3) {
	static SHA_CTX ctx;
	static unsigned char md[SHA_DIGEST_LENGTH];
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, v1, len1);
	SHA1_Update(&ctx, v2, len2);
	SHA1_Update(&ctx, v3, len3);
	SHA1_Final(md, &ctx);
	if (hash_size > SHA_DIGEST_LENGTH)
		memset((char*) hash_return + SHA_DIGEST_LENGTH, 0,
				hash_size - SHA_DIGEST_LENGTH);
	memcpy(hash_return, md,
			hash_size > SHA_DIGEST_LENGTH ? SHA_DIGEST_LENGTH : hash_size);
}

int dht_random_bytes(void *buf, size_t size) {
	return (RAND_bytes(buf, size) == 1);
}
