#include "blacklist.h"
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

struct blacklist_entry {
	struct blacklist_entry * children[256];
	unsigned char level;
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
	entry->level = level;
	return entry;
}
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);
void printf_addr(const struct sockaddr *sa) {
	char str[INET6_ADDRSTRLEN];
	get_ip_str(sa, str, INET6_ADDRSTRLEN);
	printf("%s", str);
}

void blacklist_blacklist_node(const struct sockaddr *sa, int salen) {
	printf("BLACKLIST ");
	printf_addr(sa);
	printf("\n");
	return;
	struct sockaddr_in * ip4addr;
	struct sockaddr_in6 * ip6addr;
	struct blacklist_entry * prev;
	struct blacklist_entry * entry;
	int i, maxlevel = 0;
	char addr[16];
	unsigned char pos = 0;
	switch (sa->sa_family) {
	case AF_INET:
		maxlevel = 3;
		ip4addr = (struct sockaddr_in*) sa;
		if (IPv4_blacklist == NULL) {
			IPv4_blacklist = create_new_entry(0);
		}
		memcpy(addr, &(ip4addr->sin_addr.s_addr), 4);
		prev = IPv4_blacklist;
		for (i = 0; i < maxlevel; i++) {
			pos = addr[i];
			entry = prev->children[pos];
			if (entry == NULL && i < maxlevel) {
				prev->children[pos] = create_new_entry(i + 1);
				entry = prev->children[pos];
			}
			prev = entry;
		}
		break;
	case AF_INET6:
		maxlevel = 15;
		ip6addr = (struct sockaddr_in6*) sa;
		if (IPv6_blacklist == NULL) {
			IPv6_blacklist = create_new_entry(0);
		}
		memcpy(addr, &(ip6addr->sin6_addr.s6_addr), 16);
		prev = IPv6_blacklist;
		for (i = 0; i < maxlevel; i++) {
			pos = addr[i];
			entry = prev->children[pos];
			if (entry == NULL && i < maxlevel) {
				prev->children[pos] = create_new_entry(i + 1);
				entry = prev->children[pos];
			} else if (entry == NULL) {
				number_of_entries++;
				prev->children[pos] = ip6addr->sin6_port;
			}
			prev = entry;
		}
		break;
	}
}

int blacklist_blacklisted(const struct sockaddr *sa, int salen) {
	struct sockaddr_in * ip4addr;
	struct sockaddr_in6 * ip6addr;
	struct blacklist_entry * entry;
	unsigned char i, pos, level, maxlevel = 0;
	char addr[16];
	switch (sa->sa_family) {
	case AF_INET:
		maxlevel = 3;
		ip4addr = (struct sockaddr_in*) sa;
		memcpy(addr, &(ip4addr->sin_addr.s_addr), 4);
		entry = IPv4_blacklist;
		for (i = 0; i <= maxlevel; i++) {
			pos = addr[i];
			if (entry == NULL) {
				return 0;
			} else {
				level = entry->level;
				entry = entry->children[pos];
			}
		}
		/*		printf("socket is blacklisted: ");
		 printf_addr(sa);
		 printf(" level: %d\n", level);*/
		return 1;
		break;
	case AF_INET6:
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
		break;
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
