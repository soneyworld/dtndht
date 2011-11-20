/*
 * Checksum.h
 *
 *  Created on: 02.07.2010
 *      Author: Till Lorentzen
 */

#ifndef CHECKSUM_H_
#define CHECKSUM_H_
#include <openssl/evp.h>
#ifndef CHECKSUM
#define CHECKSUM "sha1"
#endif

size_t getChecksumLength();
void calcchecksum(const char* file, unsigned char *md);
void calcchecksum(const char* data, unsigned char *md, size_t length);
#endif /* CHECKSUM_H_ */
