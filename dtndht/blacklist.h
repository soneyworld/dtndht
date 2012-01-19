#ifndef BLACKLIST_INCLUDED
#define BLACKLIST_INCLUDED
#include <sys/socket.h>
// Adds a socket to the blacklist
void blacklist_blacklist_node(const struct sockaddr *sa, unsigned char * md);
// Return 1 if socket has been blacklisted
int blacklist_blacklisted(const struct sockaddr *sa, int salen);

#endif
