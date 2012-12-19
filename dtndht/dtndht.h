#ifndef DTNDHT_INCLUDED
#define DTNDHT_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/socket.h>

enum dtn_dht_bind_type {
	BINDNONE = 0, IPV4ONLY = 1, IPV6ONLY = 2, BINDBOTH = 3
};

enum dtn_dht_lookup_type {
	SINGLETON = 0, NEIGHBOUR = 1, GROUP = 2
};

extern FILE *dtn_dht_debug;

struct dtn_convergence_layer_arg {
	char * key;
	size_t keylen;
	char * value;
	size_t valuelen;
	struct dtn_convergence_layer_arg * next;
};

struct dtn_convergence_layer {
	char * clname;
	size_t clnamelen;
	struct dtn_convergence_layer_arg * args;
	struct dtn_convergence_layer * next;
};

struct dtn_dht_context {
	int ipv4socket;
	int ipv6socket;
	unsigned char id[20];
	int port;
	int type;
	const char *bind;
	const char *bind6;
	size_t minimum_rating;
	struct dtn_convergence_layer * clayer;
};

struct dtn_eid {
	char * eid;
	size_t eidlen;
	struct dtn_eid * next;
};

struct dtn_dht_lookup_result {
	struct dtn_eid * eid;
	struct dtn_eid * groups;
	struct dtn_eid * neighbours;
	struct dtn_convergence_layer * clayer;
};

// Loading previous saved buckets for faster bootstrapping
int dtn_dht_load_prev_conf(const char *filename);
// Save actual buckets to file for faster bootstrapping
int dtn_dht_save_conf(const char *filename);

// Generates an ID from given string. This produces a deterministic ID.
// Implementation is in the utils.c file
void dtn_dht_build_id_from_str(unsigned char *target, const char *s, size_t len);

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
int dtn_dht_lookup(struct dtn_dht_context *ctx, const char *eid,
		size_t eidlen);

// DHT Announce
int dtn_dht_announce(struct dtn_dht_context *ctx, const char *eid,
		size_t eidlen, enum dtn_dht_lookup_type type);

// DHT Stop Announcement
int dtn_dht_deannounce(const char *eid, size_t eidlen);

// The main loop of the dht
int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf,
		size_t buflen, const struct sockaddr *from, int fromlen,
		time_t *tosleep);

// Closes all socket of the context
int dtn_dht_close_sockets(struct dtn_dht_context *ctx);

// Frees the convergence layer list
void dtn_dht_free_convergence_layer_struct(struct dtn_convergence_layer *clayer);

// If blacklist is enabled (default) the number of blocked addresses is returned
unsigned int dtn_dht_blacklisted_nodes(unsigned int *ipv4_return,
		unsigned int *ipv6_return);

// Returns the number of nodes available, and also the good and dubious nodes
int dtn_dht_nodes(int af, int *good_return, int *dubious_return);

// callback functions: Must be provided by the user
// Lookup of an eid was successful
void dtn_dht_handle_lookup_result(const struct dtn_dht_lookup_result *result);

// Is called if a DHT Search is finished. This could have multiple reasons
// First: A Lookup is done
// Second: A Announcement is done
// Third: A Search for a random hash is done
void dtn_dht_operation_done(const unsigned char *info_hash);

// functions for self implemented bootstrapping methods
// inserting a known dht node (use this only, if you know the node.
// Should be used carefully)
int
dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
// pinging a possible dht node (normal way to add a possible dht node
// method is better/softer than inserting a node and should be used)
int dtn_dht_ping_node(struct sockaddr *sa, int salen);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
