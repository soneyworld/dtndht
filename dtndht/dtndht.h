
#ifndef DTNDHT_INCLUDED
#define DTNDHT_INCLUDED
#include <dtndht/dht.h>

enum dtn_dht_bind_type{
	BINDNONE=0,IPV4ONLY=1,IPV6ONLY=2,BINDBOTH=3
};

extern FILE *dtn_dht_debug;


struct dtn_dht_context {
	int ipv4socket;
	int ipv6socket;
	unsigned char id[20];
	int port;
	int type;
};

// Loading previous saved buckets for faster bootstrapping
int dtn_dht_load_prev_conf(struct dtn_dht_context *ctx, FILE *f);
// Save acutal buckets to file for faster bootstrapping
int dtn_dht_save_conf(struct dtn_dht_context *ctx, FILE *f);

// Init struct
int dtn_dht_initstruct(struct dtn_dht_context *ctx);

// Initialize the dht
int dtn_dht_init(struct dtn_dht_context *ctx);

int dtn_dht_init_sockets(struct dtn_dht_context *ctx);

// Destroy the dht
int dtn_dht_uninit(void);

// Simple DNS based bootstrapping method
int dtn_dht_dns_bootstrap(struct dtn_dht_context *ctx);

// Asynchronously lookup for the given eid and the given service
int dtn_dht_lookup(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen);

int dtn_dht_lookup_group(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen);

// Join a dtn group with the given gid
int dtn_dht_join_group(const unsigned char *gid, int gidlen, const unsigned char *cltype, int cllen, int port);
// Leave the given dtn group -> stopping the announcing
int dtn_dht_leave_group(const unsigned char *gid, int gidlen, const unsigned char *cltype, int cllen, int port);

// DHT Announce
int dtn_dht_announce(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);
int dtn_dht_announce_neighbour(struct dtn_dht_context *ctx, const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);

// DHT Stop Announcement
int dtn_dht_deannounce(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);
int dtn_dht_deannounce_neighbour(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);

// The main loop of the dht
int dtn_dht_periodic(struct dtn_dht_context *ctx, const void *buf, size_t buflen,
		const struct sockaddr *from, int fromlen, time_t *tosleep);


//int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
//int dtn_dht_ping_node(struct sockaddr *sa, int salen);

// Closes all socket of the context
int dtn_dht_close_sockets(struct dtn_dht_context *ctx);

// callback functions: Must be provided by the user
// Lookup of an eid was successful
void dtn_dht_handle_lookup_result(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int ipversion, struct sockaddr_storage *addr, size_t addrlen, size_t count);
// Lookup of a group was successful
void dtn_dht_handle_lookup_group_result(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int ipversion, struct sockaddr_storage *addr, size_t addrlen, size_t count);
#endif
