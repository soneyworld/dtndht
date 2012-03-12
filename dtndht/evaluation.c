/*
 * evaluation.c
 *
 *  Created on: 12.03.2012
 *      Author: Till Lorentzen
 */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

void printf_time(void) {
	time_t timer;
	timer = time(NULL);
	char *str = asctime(localtime(&timer));
	str[strlen(str) - 1] = '\0';
	printf("%s", str);
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
