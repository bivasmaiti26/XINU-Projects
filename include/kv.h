#include<xinu.h>
#define CACHESIZE 32
#define MAX_CACHE_KEY 64
#define MAX_CACHE_VALUE 1024 
//Struct definition for a unit hash in the cache table
typedef struct {
	char key[65];
	int hits;
	char *value;
	
} hashunit;

/*extern int total_hits;
extern int total_accesses;
extern uint32 total_set_success;
extern int cache_size;
extern int num_keys;
extern int total_evictions;*/

//Hash table implemented as a cache
hashunit global_cache[CACHESIZE];

//Key Val store functions
char * kv_get(char[]);
int kv_set(char[],char*);
bool kv_delete(char[]);
void kv_reset();
int kv_init();
int get_cache_info(char*);
char** most_popular_keys(int);



