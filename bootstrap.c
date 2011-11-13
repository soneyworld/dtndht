#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Just a little program to get bootstrapping nodes for the dht

int main(int argc, char *argv[]) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0; /* Any protocol */

	s = getaddrinfo("dht.transmissionbt.com", "6881", &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.*/
	int ipv4size, ipv6size = 0;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		int portNumber;
		if (rp->ai_addr->sa_family == AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *) rp->ai_addr;
			portNumber = ipv4->sin_port;
			printf("host: %s : %d\n", inet_ntoa(ipv4->sin_addr), portNumber);
			ipv4size++;
		} else {
			char straddr[INET6_ADDRSTRLEN];
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) rp->ai_addr;
			portNumber = ipv6->sin6_port;
			inet_ntop(AF_INET6, &ipv6->sin6_addr, straddr, sizeof(straddr));
			printf("host: %s : %d\n", straddr, portNumber);
			ipv6size++;
		}
	}
	printf("-------------------------------------------------------------\n");
	printf("Number of Results: %d (IPv4: %d and IPv6: %d)\n",
			ipv4size + ipv6size, ipv4size, ipv6size);

	freeaddrinfo(result); /* No longer needed */
	exit(EXIT_SUCCESS);
}
