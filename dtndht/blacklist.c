#include "blacklist.h"
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

struct blacklist_entry {
	void * children[256];
	unsigned char level;
};

struct blacklist_leaf {
	u_int16_t port;
	char md[20];
};
static struct blacklist_entry *IPv4_blacklist = NULL;
static struct blacklist_entry *IPv6_blacklist = NULL;
static unsigned long number_of_entries = 0l;

struct blacklist_entry * create_new_entry(unsigned char level) {
	unsigned int i;
	struct blacklist_entry * entry;
	entry = (struct blacklist_entry*) malloc(sizeof(struct blacklist_entry));
	for (i = 0; i < 256; i++) {
		entry->children[i] = NULL;
	}
	return entry;
}

struct blacklist_leaf * create_new_leaf(u_int16_t port, char * md) {

	struct blacklist_leaf *leaf = malloc(sizeof(struct blacklist_leaf));
	leaf->port = ntohs(port);
	memcpy(leaf->md, md, 20);
	printf("creating leaf with port: %d\n", leaf->port);
	return leaf;
}
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);
void printf_blacklist();
void printf_addr(const struct sockaddr *sa) {
	char str[INET6_ADDRSTRLEN];
	get_ip_str(sa, str, INET6_ADDRSTRLEN);
	printf("%s", str);
}

void blacklist_blacklist_node(const struct sockaddr *sa, unsigned char * md) {
	struct sockaddr_in * ip4addr;
	struct sockaddr_in6 * ip6addr;
	struct blacklist_entry * prev;
	int i = 0, maxlevel = 0;
	u_int16_t port = 0;
	unsigned char addr[16];
	unsigned char pos = 0;
	printf("BLACKLIST ");
	printf_addr(sa);
	switch (sa->sa_family) {
	case AF_INET:
		maxlevel = 4;
		ip4addr = (struct sockaddr_in*) sa;
		if (IPv4_blacklist == NULL) {
			IPv4_blacklist = create_new_entry(0);
		}
		prev = IPv4_blacklist;
		memcpy(addr, &(ip4addr->sin_addr.s_addr), maxlevel);
		port = ip4addr->sin_port;
		printf(" Port %d", ntohs(ip4addr->sin_port));
		break;
	case AF_INET6:
		maxlevel = 16;
		ip6addr = (struct sockaddr_in6*) sa;
		memcpy(addr, &(ip6addr->sin6_addr.s6_addr), maxlevel);
		if (IPv6_blacklist == NULL) {
			IPv6_blacklist = create_new_entry(0);
		}
		prev = IPv6_blacklist;
		port = ip6addr->sin6_port;
		printf(" Port %d", ntohs(ip6addr->sin6_port));
		break;
	default:
		return;
	}
	printf("\n");
	for (i = 0; i <= maxlevel; i++) {
		if (i < maxlevel) {
			if (prev->children[addr[i]] == NULL) {
				prev->children[addr[i]] = create_new_entry(i + 1);
			}
		} else {
			prev->children[addr[i]] = create_new_leaf(port, md);
		}
		prev = prev->children[addr[i]];
	}
	printf_blacklist();
}

int blacklist_blacklisted(const struct sockaddr *sa, int salen) {
	//printf_blacklist();
	struct sockaddr_in * ip4addr;
	struct sockaddr_in6 * ip6addr;
	struct blacklist_entry * entry;
	struct blacklist_leaf * leaf;
	unsigned char i, pos, level, maxlevel = 0;
	unsigned char addr[16];
	switch (sa->sa_family) {
	case AF_INET:
		maxlevel = 4;
		ip4addr = (struct sockaddr_in*) sa;
		memcpy(addr, &(ip4addr->sin_addr.s_addr), 4);
		entry = IPv4_blacklist;
		for (i = 0; i < maxlevel; i++) {
			pos = addr[i];
			if (entry == NULL) {
				return 0;
			} else {
				if (i == maxlevel - 1) {
					leaf = entry->children[pos];
				} else {
					entry = entry->children[pos];
				}
			}
		}
		printf("\nsocket is blacklisted: ");
		printf_addr(sa);
		printf(" Port: %d", leaf->port);
		printf(" level: %d\n", i);
		return 1;
		break;
		/*	case AF_INET6:
		 maxlevel = 15;
		 ip6addr = (struct sockaddr_in6*) sa;
		 memcpy(addr, &(ip6addr->sin6_addr.s6_addr), 16);
		 entry = IPv6_blacklist;
		 for (i = 0; i <= maxlevel; i++) {
		 pos = addr[i];
		 if (entry == NULL) {
		 return 0;
		 } else {
		 entry = entry->children[pos];
		 }
		 }
		 printf("socket is blacklisted: ");
		 printf_addr(sa);
		 printf("\n");
		 return 1;
		 break;*/
	}
	return 0;
}

// Helper function found: http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html

//Convert a struct sockaddr address to a string, IPv4 and IPv6:
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
	switch (sa->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *) sa)->sin_addr), s, maxlen);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) sa)->sin6_addr), s,
				maxlen);
		break;

	default:
		strncpy(s, "Unknown AF", maxlen);
		return NULL;
	}

	return s;
}

void printf_bl_entry(unsigned int * pos, int level, int maxlevel, void * entry) {
	if (entry != NULL) {
		if (level == maxlevel) {
			struct blacklist_leaf * leaf = entry;
			if (level == 4) {
				printf(" %d.%d.%d.%d : %d\n", pos[0], pos[1], pos[2], pos[3],
						leaf->port);
			} else if (level == 16) {
				printf("IPv6 blocked on port: %d\n", leaf->port);
			}
		} else {
			int j;
			for (j = 0; j < 256; j++) {
				pos[level] = j;
				struct blacklist_entry * node = (struct blacklist_entry*) entry;
				printf_bl_entry(pos, level + 1, maxlevel, node->children[j]);
			}
		}
	}
}

void printf_blacklist() {
	unsigned int pos[16];
	printf("----------BLACKLIST-IPv4-------\n");
	printf_bl_entry(pos, 0, 4, IPv4_blacklist);
	printf("----------BLACKLIST-IPv6-------\n");
	printf_bl_entry(pos, 0, 16, IPv6_blacklist);
	printf("-------------------------------\n");
}
