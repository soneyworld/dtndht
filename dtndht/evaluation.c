/*
 * evaluation.c
 *
 *  Created on: 12.03.2012
 *      Author: Till Lorentzen
 */
#include <time.h>
#include <stdio.h>
#include <string.h>
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
