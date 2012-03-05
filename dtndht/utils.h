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

#endif /* UTILS_H_ */
