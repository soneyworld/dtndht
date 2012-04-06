/*
 * evaluation.c
 *
 *  Created on: 12.03.2012
 *      Author: Till Lorentzen
 */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#else
#define SHA_DIGEST_LENGTH 20
#endif

void printf_time(void) {
	time_t timer;
	timer = time(NULL);
	char *str = asctime(localtime(&timer));
	int i = 0;
	while(str[i]!='\0'){
		if(str[i]=='\n'){
			str[i] = '\0';
			break;
		}
		i++;
	}
	printf("%s TIME=%ld", str, timer);
}

void printf_evaluation_start(void) {
	printf_time();
	printf(" DHT-EVALUATION ");
}

void printf_hash(const unsigned char *buf) {
	int i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
		printf("%02x", buf[i]);
	}
}

void fflush_evaluation(void){
	printf("\n");
	fflush(stdout);
}

void printf_sockaddr_storage(const struct sockaddr_storage* ss){
	char * str;
	int port;
	switch (((struct sockaddr*) ss)->sa_family) {
	case AF_INET:
		str = (char*) malloc(INET_ADDRSTRLEN);
		inet_ntop(((struct sockaddr*) ss)->sa_family, &((struct sockaddr_in *) ss)->sin_addr,
				str, INET_ADDRSTRLEN);
		port = ntohs(((struct sockaddr_in *) ss)->sin_port);
		break;
	case AF_INET6:
		str = (char*) malloc(INET6_ADDRSTRLEN);
		inet_ntop(((struct sockaddr*) ss)->sa_family, &((struct sockaddr_in6 *) ss)->sin6_addr,
				str, INET6_ADDRSTRLEN);
		port = ntohs(((struct sockaddr_in6 *) ss)->sin6_port);
		break;
	default:
		port = -1;
		str = NULL;
	}
	if (str) {
		printf("IP=%s PORT=%d", str, port);
		free(str);
	}
}
