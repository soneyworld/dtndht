#include "config.h"
#include "dtndht.h"
#include "dht.h"
/*
 * bootstrapping.h
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */

#ifndef BOOTSTRAPPING_H_
#define BOOTSTRAPPING_H_

#ifndef BOOTSTRAPPING_SEARCH_THRESHOLD
#define BOOTSTRAPPING_SEARCH_THRESHOLD 8
#endif

#define BOOTSTRAPPING_SEARCH_MAX_HASHES 40
#define BOOTSTRAPPING_DOMAIN "dtndht.ibr.cs.tu-bs.de"
#define BOOTSTRAPPING_SERVICE "6881"

#ifndef DHT_READY_THRESHOLD
#define DHT_READY_THRESHOLD 30
#endif


void bootstrapping_start_random_lookup(struct dtn_dht_context *ctx,
		dht_callback *callback);
int bootstrapping_dns(struct dtn_dht_context *ctx, const char* name,
		const char* service);
int bootstrapping_load_conf(const char *filename);
int bootstrapping_save_conf(const char *filename);
#endif /* BOOTSTRAPPING_H_ */
