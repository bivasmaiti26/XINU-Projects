#include<xinu.h>
#include<malloc.h>
#include <string.h>
//xmalloc_test

void xmalloc_init(){
		
}
//update the snapshot table when a buffer is allocated, this is called from  xmalloc
void update_snaptab(bpid32 pool_id,void* bufaddr,uint32 size){
	//printf("update_to_snaptab,pool_id=%d\n",pool_id);
	struct bpentry *bpptr;
	bpptr = &buftab[pool_id];
	bufpoolsnaptab[pool_id].allocated_buffers+=1;
	bufpoolsnaptab[pool_id].allocated_bytes+=size;
	bufpoolsnaptab[pool_id].fragmented_bytes+=bpptr->bpsize-size;
	//Keep track of the actual size of the xmalloc request and the address
	buffitem* bufftracker;
	buffitem* buffnew=(buffitem*)getmem(sizeof(buffitem));
	bufftracker=bufpoolsnaptab[pool_id].buffhead;
	buffnew->size=size;
	buffnew->bufaddr=bufaddr;
	buffnew->bnext=NULL;
	while(bufftracker->bnext!=NULL){
		bufftracker=(buffitem*)bufftracker->bnext;
	}

	buffnew->bprev=(struct buffitem*)bufftracker;
	if(bufpoolsnaptab[pool_id].buffhead==NULL){
		bufpoolsnaptab[pool_id].buffhead=(struct buffitem*)buffnew;
	}
	else{
		bufftracker->bnext=(struct buffitem*)buffnew;
		bufftracker=(buffitem*)bufftracker->bnext;
		bufftracker->bnext=NULL;
	}
	buffitem *t=bufpoolsnaptab[pool_id].buffhead;
	buffitem *t1=t->bnext;
	//printf("After update_snaptab: buffhead=%d,next=%d,pool_id=%d\n",t->bufaddr,t1->bufaddr,pool_id );	
}
//update the snapshot table when a buffer is made free, this is called from  xfree
void update_snaptab_free(bpid32 pool_id,void* bufaddr,uint32 size){
	struct bpentry *bpptr;
	bpptr = &buftab[pool_id];
	bufpoolsnaptab[pool_id].allocated_buffers-=1;
	bufpoolsnaptab[pool_id].allocated_bytes-=size;
	bufpoolsnaptab[pool_id].fragmented_bytes-=bpptr->bpsize-size;
}
//insert new row to the snapshot table when a new buffer pool is created and 1 buffer is allocated from its buffers
void insert_to_snaptab(bpid32 pool_id,uint32 total_buffers,void* bufaddr,uint32 size){
	//printf("insert_to_snaptab,pool_id=%d\n",pool_id);
	struct bpentry *bpptr;
	bpptr = &buftab[pool_id];
	bufpoolsnaptab[pool_id].total_buffers=total_buffers;
	bufpoolsnaptab[pool_id].allocated_buffers=1;
	bufpoolsnaptab[pool_id].allocated_bytes=size;
	bufpoolsnaptab[pool_id].fragmented_bytes=bpptr->bpsize-size;
	//Keep track of the actual size of the xmalloc request and the address
	buffitem *t=bufpoolsnaptab[0].buffhead;
	buffitem *t1=t->bnext;
	//printf("before insert_to_snaptab: buffhead=%d,next=%d,pool_id=%d\n",t->bufaddr,t1->bufaddr,pool_id );
	
	buffitem* bufftracker=(buffitem*)getmem(sizeof(buffitem));
	bufftracker->size=size;
	bufftracker->bprev=NULL;
	bufftracker->bnext=NULL;
	bufftracker->bufaddr=bufaddr;
	//printf("pool_id=%d\n",pool_id );
	bufpoolsnaptab[2].buffhead=bufftracker;
	t=bufpoolsnaptab[0].buffhead;
	t1=t->bnext;
	//printf("after insert_to_snaptab: buffhead=%d,next=%d,pool_id=%d\n",t->bufaddr,t1->bufaddr,pool_id );
	

}
//Xmalloc call. This allocates memory from a buffer pool and returns the pointer to the location 
void* xmalloc(uint32 size_t){
	int i;
	bpid32 buffpoolId;
	int corrected_size=size_t;
	if(size_t < BP_MINB){
		corrected_size=BP_MINB;
	}
	else if(size_t > BP_MAXB){
		return (void*)SYSERR;
	}
	//Check if no buffer pools are present
	if(nbpools==0){
		//Create a buffer pool with rounded size and max size of buf pools
		if(size_t < BP_MINB){
			buffpoolId=mkbufpool(corrected_size, BP_MAXN);	
		}
		else{
			buffpoolId=mkbufpool(size_t, BP_MAXN);		
		}
		
		void * buffpooladdress= (void*)getbuf(buffpoolId);
		insert_to_snaptab(buffpoolId,BP_MAXN,buffpooladdress,size_t);
		return buffpooladdress;

	}
	struct bpentry *bpptr; /* Pointer to entry in buftab */
	int size=999999;
	int maxbufsize=0;
	uint32 fittingpoolid=-1;
	for (i=0;i<nbpools;i++){
		bpptr = &buftab[i];	
		if(bpptr->bpsize<=size && bpptr->bpsize>=corrected_size && (bpptr->bpsize/2<corrected_size) ){
			fittingpoolid=i;
			size=bpptr->bpsize;
		}
		if(maxbufsize<=bpptr->bpsize)
			maxbufsize=bpptr->bpsize;
	}
	if(fittingpoolid!=-1){
		void * buffpooladdress= (void*)getbuf(fittingpoolid);
		update_snaptab(fittingpoolid,buffpooladdress,size_t);
		return buffpooladdress;
	}
	else{
		if(maxbufsize<corrected_size && (maxbufsize*2>=corrected_size)){
			corrected_size=maxbufsize*2;
		}
		buffpoolId=mkbufpool(corrected_size, BP_MAXN);	
		void * buffpooladdress= (void*)getbuf(buffpoolId);
		insert_to_snaptab(buffpoolId,BP_MAXN,buffpooladdress,size_t);
		return buffpooladdress;
	}
}
//xfree- This frees up the location from the buffers allocated. Takes in a void pointer as arguement.
void xfree(void* ptr_to_free){
	//printf("xfree call on %d\n", ptr_to_free);

	ptr_to_free -= sizeof(bpid32);
	bpid32 pool_id = *(bpid32 *)ptr_to_free;
	buffitem *t,*t1;
	t=bufpoolsnaptab[pool_id].buffhead;
	t1=t->bnext;
	//printf("xfree: buffhead=%d,next=%d,pool_id=%d\n",t->bufaddr,t1->bufaddr,pool_id );
	//Correct the ptr_to_free address since it is decremented above
	ptr_to_free+=sizeof(bpid32);
	buffitem* bufftracker=bufpoolsnaptab[pool_id].buffhead;
	int size=-1;
	int count=0;
	//Check the buffer item from the buffer tracker queue to get the size of the item.
	while(bufftracker!=NULL){
		count++;
		//printf("xfree,bufftracker=%d,pool_id=%d\n",bufftracker,pool_id );
		//printf("bufaddr=%d\n",bufftracker->bufaddr );
		if(bufftracker->bufaddr==ptr_to_free){
			//printf("count=%d\n", count);
			size=bufftracker->size;
			buffitem* prev=(buffitem*)bufftracker->bprev;
			buffitem* next=(buffitem*)bufftracker->bnext;
			prev->bnext=(struct buffitem*)next;
			//printf("bufftracker=%d,prev=%d,next=%d\n",bufftracker,prev,next );
			//If this is not already the last item
			if(next!=NULL)
				next->bprev=(struct buffitem*)prev;
			if(next==NULL && prev==NULL){
				//printf("Empty\n");
				bufpoolsnaptab[pool_id].buffhead=NULL;
			}
			//free the item memory from the linked list too!
			freemem((char*)bufftracker,sizeof(buffitem));
			break;
		}
		
		bufftracker=(buffitem*)bufftracker->bnext;

	}
	//printf("allocated buffers=%d\n",bufpoolsnaptab[pool_id].allocated_buffers);
	if(size>0){
		update_snaptab_free(pool_id,ptr_to_free,size);
	}
	freebuf(ptr_to_free);
	t=bufpoolsnaptab[pool_id].buffhead;
	t1=t->bnext;
	//printf("After xfree: buffhead=%d,next=%d,pool_id=%d\n",t->bufaddr,t1->bufaddr,pool_id );
	//printf("After xfree bufftracker=%d,next=%d\n",bufftracker,bufftracker->bnext );
}
//This returns a character pointer to a string generated which tells us about the current state of the buffer pools. 
char * xheap_snapshot(){
	char * retstring=getmem(nbpools*200);
	char * snaprow=getmem(200);
	int pool_id;
	for(pool_id=0;pool_id<nbpools;pool_id++){
		struct bpentry *bpptr;
		bpptr = &buftab[pool_id];	
		sprintf(snaprow, "poolid=%d, buffer_size=%d, total_buffers=%d, allocated_bytes=%d, allocated_buffers=%d, fragmented_bytes=%d \n",pool_id, bpptr->bpsize, bufpoolsnaptab[pool_id].total_buffers, bufpoolsnaptab[pool_id].allocated_bytes, bufpoolsnaptab[pool_id].allocated_buffers, bufpoolsnaptab[pool_id].fragmented_bytes);
		strncat(retstring,snaprow,200);
	}
	freemem(snaprow,200);
	return retstring;
}