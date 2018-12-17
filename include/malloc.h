#include<stdio.h>
#include<stdlib.h>

typedef struct{
	void* bufaddr;
	int size;
	struct buffitem* bnext;
	struct buffitem* bprev;
} buffitem;

typedef struct{
	bpid32 pool_id;
	int total_buffers;
	int allocated_buffers;
	int allocated_bytes;
	int fragmented_bytes;
	buffitem* buffhead;

} buffpoolitem;

buffpoolitem bufpoolsnaptab[NBPOOLS];
void xmalloc_init();
void* xmalloc(uint32);
void xfree(void*);
char* xheap_snapshot();
