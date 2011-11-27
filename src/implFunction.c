/*
 * Implementation of the needed c functions of dht.h
 *
 * 		Author: Till Lorentzen
 */
//#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/signal.h>
#include "dht.h"


#include <openssl/rand.h>
#include <openssl/sha.h>


/* Functions called by the DHT. */

int
dht_blacklisted(const struct sockaddr *sa, int salen)
{
    return 0;
}

void
dht_hash(void *hash_return, int hash_size,
         const void *v1, int len1,
         const void *v2, int len2,
         const void *v3, int len3)
{
    static SHA_CTX ctx;
    static char md[20];
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, v1, len1);
    SHA1_Update(&ctx, v2, len2);
    SHA1_Update(&ctx, v3, len3);
    SHA1_Final(md,&ctx);
    if(hash_size > 20)
        memset((char*)hash_return + 20, 0, hash_size - 20);
    memcpy(hash_return, md, hash_size > 20 ? 20 : hash_size);
}

int
dht_random_bytes(void *buf, size_t size)
{
	return (RAND_bytes(buf, size)==1);
}

