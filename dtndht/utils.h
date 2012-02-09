/*
 * utils.h
 *
 *  Created on: 08.02.2012
 *      Author: Till Lorentzen
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <sys/socket.h>

int cpyvaluetosocketstorage(struct sockaddr_storage *target, const void *value,
		int type);

#endif /* UTILS_H_ */
