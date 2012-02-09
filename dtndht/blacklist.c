#include "blacklist.h"
#include <stddef.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

struct blacklist_entry {
	void * children[256];
	unsigned char level;
};

struct blacklist_leaf {
	u_int16_t port;
	char md[SHA_DIGEST_LENGTH];
};
static struct blacklist_entry *IPv4_blacklist = NULL;
static struct blacklist_entry *IPv6_blacklist = NULL;
static int enabled = 1;
static unsigned int number_of_entries_ipv4 = 0;
static unsigned int number_of_entries_ipv6 = 0;

unsigned int blacklist_size(unsigned int *ipv4_return,
		unsigned int *ipv6_return) {
	*ipv4_return = number_of_entries_ipv4;
	*ipv6_return = number_of_entries_ipv6;
	return number_of_entries_ipv4 + number_of_entries_ipv6;
}

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
	memcpy(leaf->md, md, SHA_DIGEST_LENGTH);
	//	printf("creating leaf with port: %d\n", leaf->port);
	return leaf;
}
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);
void printf_addr(const struct sockaddr *sa) {
	char str[INET6_ADDRSTRLEN];
	get_ip_str(sa, str, INET6_ADDRSTRLEN);
	printf("%s", str);
}

void blacklist_blacklist_node(const struct sockaddr *sa, unsigned char * md) {
	if (!enabled)
		return;
	struct sockaddr_in * ip4addr;
	struct sockaddr_in6 * ip6addr;
	struct blacklist_entry * prev;
	int i = 0, maxlevel = 0;
	u_int16_t port = 0;
	unsigned char addr[16];
	unsigned char pos = 0;
	//	printf("BLACKLIST ");
	//	printf_addr(sa);
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
		//		printf(" Port %d", ntohs(ip4addr->sin_port));
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
		//		printf(" Port %d", ntohs(ip6addr->sin6_port));
		break;
	default:
		return;
	}
	//	printf("\n");
	for (i = 1; i <= maxlevel; i++) {
		if (i < maxlevel) {
			if (prev->children[addr[i - 1]] == NULL) {
				prev->children[addr[i - 1]] = create_new_entry(i + 1);
			}
		} else {
			if (prev->children[addr[i - 1]] == NULL) {
				switch (sa->sa_family) {
				case AF_INET:
					number_of_entries_ipv4++;
					break;
				case AF_INET6:
					number_of_entries_ipv6++;
					break;
				}
			}
			prev->children[addr[i - 1]] = create_new_leaf(port, md);
		}
		prev = prev->children[addr[i - 1]];
	}
	//	printf_blacklist();
}

int blacklist_blacklisted(const struct sockaddr *sa) {
	if (!enabled)
		return 0;
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
					if (leaf != NULL) {
						return 1;
					} else {
						return 0;
					}
				} else {
					entry = entry->children[pos];
				}
			}
		}
		//		printf("\nsocket is blacklisted: ");
		//		printf_addr(sa);
		//		printf(" Port: %d", leaf->port);
		//		printf(" level: %d\n", i);
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

void blacklist_printf() {
	if (!enabled)
		return;
	unsigned int pos[16];
	printf("----------BLACKLIST-IPv4-------\n");
	printf_bl_entry(pos, 0, 4, IPv4_blacklist);
	printf("----------BLACKLIST-IPv6-------\n");
	printf_bl_entry(pos, 0, 16, IPv6_blacklist);
	printf("-------------------------------\n");
}

void blacklist_free_tree(void * tree, int level, int maxlevel) {
	if (tree == NULL)
		return;
	if (level < maxlevel) {
		int i;
		struct blacklist_entry * entry = (struct blacklist_entry*) tree;
		for (i = 0; i < 256; i++) {
			if (entry->children[i] != NULL) {
				blacklist_free_tree(entry->children[i], level + 1, maxlevel);
				entry->children[i] = NULL;
			}
		}
		free(entry);
	} else {
		struct blacklist_leaf * leaf = (struct blacklist_leaf*) tree;
		free(leaf);
	}
}
void blacklist_id_free();
void blacklist_free() {
	blacklist_free_tree(IPv4_blacklist, 0, 4);
	blacklist_free_tree(IPv6_blacklist, 0, 16);
	IPv4_blacklist = NULL;
	IPv6_blacklist = NULL;
	blacklist_id_free();
}

void blacklist_enable(int enable) {
	if (enable) {
		enabled = 1;
	} else {
		enabled = 0;
	}
}

int blacklist_is_enabled() {
	return enabled;
}

// Blacklisted ID's as linked list

struct blacklisted_id {
	unsigned char md[SHA_DIGEST_LENGTH];
	struct blacklisted_id *next;
};
static struct blacklisted_id *idblacklist = NULL;

int blacklist_id_blacklisted(const unsigned char *id) {
	struct blacklisted_id * blacklist = idblacklist;
	while (blacklist) {
		if (memcmp(blacklist->md, id, SHA_DIGEST_LENGTH) == 0) {
			return 1;
		}
		blacklist = blacklist->next;
	}
	return 0;
}

void blacklist_blacklist_id(const unsigned char *id) {
	struct blacklisted_id * bid = malloc(sizeof(struct blacklisted_id));
	memcpy(bid->md, id, SHA_DIGEST_LENGTH);
	if (idblacklist) {
		bid->next = idblacklist;
	} else {
		bid->next = NULL;
	}
	idblacklist = bid;
}

void blacklist_id_free() {
	struct blacklisted_id * blacklist = idblacklist;
	struct blacklisted_id * next;
	while (blacklist) {
		next = blacklist->next;
		free(blacklist);
		blacklist = next;
	}
	idblacklist = NULL;
}

