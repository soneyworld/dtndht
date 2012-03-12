/*
 * utils.h
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 *
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <sys/socket.h>
/**
 * This function copies the given value returned from the dht into the target sockaddr_storage
 * This is very helpful, because all normal socket functions can handle the sockaddr_storage,
 * but not the format, in which the dht answers
 */
int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type);

// Creates a empty struct for dtn message results
struct dtn_dht_lookup_result * create_dtn_dht_lookup_result(void);
// Frees the dtn message result struct
void free_dtn_dht_lookup_result(struct dtn_dht_lookup_result * result);

// Creates a empty dtn_eid struct
struct dtn_eid * create_dtn_eid(void);
// Frees the dtn_eid struct and all of it's next elements
void free_dtn_eid(struct dtn_eid * eid);

// Creates a empty dtn_convergence_layer struct
struct dtn_convergence_layer * create_convergence_layer(void);
// Frees a given dtn_convergence_layer struct with all next elements and all the arguments
void free_convergence_layer(struct dtn_convergence_layer * layer);

// Creates a empty dtn_convergence_layer_arg struct
struct dtn_convergence_layer_arg * create_convergence_layer_arg(void);

#endif /* UTILS_H_ */
