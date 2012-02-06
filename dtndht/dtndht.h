#ifndef DTNDHT_INCLUDED
#define DTNDHT_INCLUDED
#include <dtndht/dht.h>

enum dtn_dht_bind_type {
	BINDNONE = 0, IPV4ONLY = 1, IPV6ONLY = 2, BINDBOTH = 3
};

enum dtn_dht_request_type {
	SINGLETON = 0, GROUP = 1, NEIGHBOUR = 2
};

extern FILE *dtn_dht_debug;

struct dtn_dht_context {
	int ipv4socket;
	int ipv6socket;
	unsigned char id[20];
	int port;
	int type;
	const char *bind;
	const char *bind6;
};

// Loading previous saved buckets for faster bootstrapping
int dtn_dht_load_prev_conf(struct dtn_dht_context *ctx, const char *filename);
// Save acutal buckets to file for faster bootstrapping
int dtn_dht_save_conf(struct dtn_dht_context *ctx, const char *filename);

// Generates an ID from given string. This produces a deterministic ID.
void
dtn_dht_build_id_from_str(unsigned char *target, const char *s, size_t len);

// Initialize struct
int dtn_dht_initstruct(struct dtn_dht_context *ctx);

// Initialize the dht
int dtn_dht_init(struct dtn_dht_context *ctx);

// Initialize sockets based on context
int dtn_dht_init_sockets(struct dtn_dht_context *ctx);

// Destroy the dht
int dtn_dht_uninit(void);

// Simple DNS based bootstrapping method
int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx, const char* name,
		const char* service);

// Sending a simple request for peers for a random node to speeding up bootstrapping
void dtn_dht_start_random_lookup(struct dtn_dht_context *ctx);

// Switches blacklisting of weird acting nodes on and off
// Default is on
void dtn_dht_blacklist(int enable);

// Returns true, if the DHT has more then seven nodes found
// You should wait with announcements and lookups, until this returns true
int dtn_dht_ready_for_work(struct dtn_dht_context *ctx);

// Asynchronously lookup for the given eid and the given service
int dtn_dht_lookup(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen,
		enum dtn_dht_request_type type);

// DHT Announce
int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid,
		size_t eidlen, const unsigned char *cltype, size_t cllen, int port,
		enum dtn_dht_request_type type);

// DHT Stop Announcement
int dtn_dht_deannounce(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int port,
		enum dtn_dht_request_type type);

// The main loop of the dht
int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep);

// Closes all socket of the context
int dtn_dht_close_sockets(struct dtn_dht_context *ctx);

// If blacklist is enabled (default) the number of blocked addresses is returned
unsigned int dtn_dht_blacklisted_nodes(unsigned int *ipv4_return,
		unsigned int *ipv6_return);

// callback function: Must be provided by the user
// Lookup of an eid was successful
void dtn_dht_handle_lookup_result(const unsigned char *eid, size_t eidlen,
		const unsigned char *cltype, size_t cllen, int ipversion,
		struct sockaddr_storage *addr, size_t addrlen, size_t count,
		enum dtn_dht_request_type type, int *rating);

// functions for self implemented bootstrapping methods
// inserting a known dht node (use this only, if you know the node.
// Should be used carefully)
int
dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
// pinging a possible dht node (normal way to add a possible dht node
// method is better/softer than inserting a node and should be used)
int dtn_dht_ping_node(struct sockaddr *sa, int salen);

#endif
