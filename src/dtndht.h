
enum dtn_dht_event{
	NONE=0,VALUES=1,VALUES6=2,SEARCH_DONE=3,SEARCH_DONE6=4
};

enum dtn_dht_bind_type{
	NONE=0,IPV4ONLY=1,IPV6ONLY=2,BOTH=3
};

typedef void
dtn_dht_callback(void *closure, enum dtn_dht_event event,
             unsigned char *info_hash,
             void *data, size_t data_len);

extern FILE *dtn_dht_debug;


struct dtn_dht_context {
	int ipv4socket;
	int ipv6socket;
	unsigned char id[20];
	int port;
};

// Loading previous saved buckets for faster bootstrapping
int dtn_dht_load_prev_conf(struct dtn_dht_context *ctx, FILE *f);
// Save acutal buckets to file for faster bootstrapping
int dtn_dht_save_conf(struct dtn_dht_context *ctx, FILE *f);

// Init struct
int dtn_dht_initstruct(struct dtn_dht_context *ctx);

// Initialize the dht
int dtn_dht_init(struct dtn_dht_context *ctx);
// Destroy the dht
int dtn_dht_uninit(void);

// Asynchronously lookup for the given eid and the given service
int dtn_dht_lookup(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen);

int dtn_dht_lookup_group(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen);

// Join a dtn group with the given gid
int dtn_dht_join_group(const unsigned char *gid, int gidlen, const unsigned char *cltype, int cllen, int port);
// Leave the given dtn group -> stopping the announcing
int dtn_dht_leave_group(const unsigned char *gid, int gidlen, const unsigned char *cltype, int cllen, int port);

// DHT Announce
int dtn_dht_announce(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);
int dtn_dht_announce_neighbour(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);

// DHT Stop Announcement
int dtn_dht_deannounce(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);
int dtn_dht_deannounce_neighbour(const unsigned char *eid, int eidlen, const unsigned char *cltype, int cllen, int port);

// The main loop of the dht
int dtn_dht_periodic(const void *buf, size_t buflen,
                 const struct sockaddr *from, int fromlen,
                 time_t *tosleep, dht_callback *callback, void *closure);


//int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
//int dtn_dht_ping_node(struct sockaddr *sa, int salen);
