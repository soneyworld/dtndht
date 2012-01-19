#ifndef BLACKLIST_INCLUDED
#define BLACKLIST_INCLUDED
#include <sys/socket.h>
// Adds a socket to the blacklist
void blacklist_blacklist_node(const struct sockaddr *sa, unsigned char * md);
// Return 1 if socket has been blacklisted
int blacklist_blacklisted(const struct sockaddr *sa);
void blacklist_printf();
unsigned int blacklist_size(unsigned int *ipv4_return, unsigned int *ipv6_return);
#endif
