#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include "dht.h"
#include "dtndht.h"



int dtn_dht_initstruct(struct dtn_dht_context *ctx){
	(*ctx).port=9999;
	(*ctx).ipv4socket = -1;
	(*ctx).ipv6socket = -1;
	(*ctx).type = BINDBOTH;
	//TODO generate ID
	return 0;
}


int dtn_dht_init(struct dtn_dht_context *ctx){
	return dht_init((*ctx).ipv4socket,(*ctx).ipv6socket,(*ctx).id,(unsigned char*)"JC\0\0");
}

int dtn_dht_init_sockets(struct dtn_dht_context *ctx){
	if((*ctx).port <= 0 || (*ctx).port >= 0x10000){
		return -3;
	}
	int rc = 0;
	switch((*ctx).type){
		case BINDNONE:
			(*ctx).ipv4socket = -1;
			(*ctx).ipv6socket = -1;
			break;
		case IPV4ONLY:
			(*ctx).ipv4socket = socket(PF_INET, SOCK_DGRAM, 0);
			if((*ctx).ipv4socket<0){
				perror("socket(IPv4)");
				rc = -1;
			}
			(*ctx).ipv6socket = -1;
			break;
		case IPV6ONLY:
			(*ctx).ipv4socket = -1;
			(*ctx).ipv6socket = socket(PF_INET6, SOCK_DGRAM, 0);
			if((*ctx).ipv6socket<0){
				perror("socket(IPv6)");
				rc = -2;
			}
			break;
		case BINDBOTH:
			(*ctx).ipv4socket = socket(PF_INET, SOCK_DGRAM, 0);
			if((*ctx).ipv4socket<0){
				perror("socket(IPv4)");
				rc = -1;
			}
			(*ctx).ipv6socket = socket(PF_INET6, SOCK_DGRAM, 0);
			if((*ctx).ipv6socket<0){
				perror("socket(IPv6)");
				rc = -2;
			}
			break;
	}
	if(rc<0){
		return rc;
	}
	struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
	struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;

	if((*ctx).ipv4socket>=0){
		sin.sin_port = htons((*ctx).port);
		rc = bind((*ctx).ipv4socket, (struct sockaddr*)&sin, sizeof(sin));
		if(rc < 0) {
			perror("bind(IPv4)");
			return rc;
		}else{
			rc=0;
		}
	}
	if((*ctx).ipv6socket>=0){
        int val = 1;
        rc = setsockopt((*ctx).ipv6socket, IPPROTO_IPV6, IPV6_V6ONLY,
                        (char *)&val, sizeof(val));
        if(rc < 0) {
            perror("setsockopt(IPV6_V6ONLY)");
            return rc;
        }else{

        /* BEP-32 mandates that we should bind this socket to one of our
           global IPv6 addresses.  In this function, this only
           happens if the user used the bind option. */

        sin6.sin6_port = htons((*ctx).port);
        rc = bind((*ctx).ipv6socket, (struct sockaddr*)&sin6, sizeof(sin6));
        if(rc < 0) {
            perror("bind(IPv6)");
            return rc;
        	}else{
            	rc=0;
        	}
        }
	}
	return rc;
}

int dtn_dht_uninit(void){
	return dht_uninit();
}


int dtn_dht_periodic(const void *buf, size_t buflen,
                 const struct sockaddr *from, int fromlen,
                 time_t *tosleep, dht_callback *callback, void *closure){
	return dht_periodic(buf,buflen,from,fromlen,tosleep,callback,closure);
}

