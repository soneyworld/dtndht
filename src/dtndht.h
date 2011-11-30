
enum dtn_dht_event{
	NONE=0,VALUES=1,VALUES6=2,SEARCH_DONE=3,SEARCH_DONE6=4
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
};

int dtn_dht_init(struct dtn_dht_context *ctx);
int dtn_dht_insert_node(const unsigned char *id, struct sockaddr *sa, int salen);
int dtn_dht_ping_node(struct sockaddr *sa, int salen);
int dtn_dht_periodic(const void *buf, size_t buflen,
                 const struct sockaddr *from, int fromlen,
                 time_t *tosleep, dht_callback *callback, void *closure);
int dtn_dht_search(const unsigned char *id, int port, int af,
               dht_callback *callback, void *closure);
int dtn_dht_nodes(int af,
              int *good_return, int *dubious_return, int *cached_return,
              int *incoming_return);
void dtn_dht_dump_tables(FILE *f);
int dtn_dht_get_nodes(struct sockaddr_in *sin, int *num,
                  struct sockaddr_in6 *sin6, int *num6);
int dtn_dht_uninit(void);
