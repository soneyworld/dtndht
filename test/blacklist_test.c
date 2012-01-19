/*
 * blacklist_test.c
 *
 *  Created on: 19.01.2012
 *      Author: Till Lorentzen
 */
#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "dtndht/blacklist.h"

int main(int argc, char **argv) {
	char buffer[50];
	int i, j, k, l;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 1; j++) {
			for (k = 0; k < 1; k++) {
				for (l = 1; l < 256; l++) {
					unsigned char md[20];
					struct sockaddr_in ip4addr;
					ip4addr.sin_family = AF_INET;
					ip4addr.sin_port = htons(3490 + i);
					sprintf(buffer, "%d.%d.%d.%d", i, j, k, l);
					inet_pton(AF_INET, buffer, &ip4addr.sin_addr);
					if (blacklist_blacklisted((struct sockaddr*) &ip4addr)) {
						printf("%s should not be blacklisted yet!\n", buffer);
						return -1;
					}
					blacklist_blacklist_node((struct sockaddr*) &ip4addr, md);
					if (!blacklist_blacklisted((struct sockaddr*) &ip4addr)) {
						printf("%s should be blacklisted yet!\n", buffer);
						return -2;
					}
				}
			}
		}
	}
	return 0;
}
