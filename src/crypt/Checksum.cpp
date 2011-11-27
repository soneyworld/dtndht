/*
 * Checksum.cpp
 *
 *  Created on: 07.07.2010
 *      Author: Till Lorentzen
 */
#include "Checksum.h"
void calcchecksum(const char* data, unsigned char *md, size_t length) {
	unsigned int md_len;
	EVP_MD_CTX mdctx;
	const EVP_MD *digit;
	OpenSSL_add_all_digests();
	digit = EVP_get_digestbyname(CHECKSUM);
	if (!digit) {
		printf("Unknown message digest\n");
		exit(1);
	}
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, digit, NULL);
	EVP_DigestUpdate(&mdctx, data, length);
	EVP_DigestFinal_ex(&mdctx, md, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);
}
size_t getChecksumLength() {
	const EVP_MD *digit;
	OpenSSL_add_all_digests();
	digit = EVP_get_digestbyname(CHECKSUM);
	if (!digit) {
		printf("Unknown message digest\n");
		exit(1);
	}
	return digit->md_size;
}
