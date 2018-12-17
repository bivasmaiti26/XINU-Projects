#include<xinu.h>
#include<stdio.h>
#include<malloc.h>
#include<kv.h>
#include<string.h>
#include<stdlib.h>
int total_hits;
int total_accesses;
int total_set_success;
int cache_size;
int num_keys;
int total_evictions;
int compare(const void * , const void * );
//Initialize the Cache(Key Value Store)
int kv_init(){
	total_hits=0;
	total_accesses=0;
	total_set_success=0;
	cache_size=0;
	num_keys=0;
	total_evictions=0;
	return 0;
}
//Get a value from the store 
char * kv_get(char key[]){
	int i;
	total_accesses+=1;
	for(i=0;i<CACHESIZE;i++){
		if(strncmp(global_cache[i].key,key,MAX_CACHE_KEY)==0 && strlen(global_cache[i].key)!=0){
			total_hits+=1;
			global_cache[i].hits+=1;
			return global_cache[i].value;
		}
	}
	return NULL;
}
//Store a value in the key value store.
int kv_set(char key[],char *value){
	int i;
	if(strlen(value)>MAX_CACHE_VALUE){
		return 1;
	}
	int least_hits=9999999;
	int least_index=-1;
	//if key exists
	for(i=0;i<CACHESIZE;i++){
		if(least_hits>global_cache[i].hits && strlen(global_cache[i].key)!=0){
			least_hits=global_cache[i].hits;
			least_index=i;
		}
		if(strncmp(global_cache[i].key,key,MAX_CACHE_KEY)==0 && strlen(global_cache[i].key)!=0){
			printf("old key=%s,value=%s\n",global_cache[i].key,global_cache[i].value );
			printf("hits=%d\n",global_cache[i].hits);
			xfree(global_cache[i].value);
			char * value_address= xmalloc(strlen(value)+1);
			strncpy(value_address,value,strlen(value)+1);
			global_cache[i].value=value_address;
			cache_size+=(strlen(value)-strlen(global_cache[i].value));
			total_set_success+=1;
			return 0;
		}
	}
	//If key doesn't exist, try to add new key
	for(i=0;i<CACHESIZE;i++){
		if(strlen(global_cache[i].key)==0){
			strncpy(global_cache[i].key,key,MAX_CACHE_KEY);
			char * value_address= xmalloc(strlen(value)+1);
			strncpy(value_address,value,strlen(value));
			global_cache[i].value=value_address;
			total_set_success+=1;
			cache_size+=(strlen(key)+strlen(value));
			global_cache[i].hits=0;
			num_keys+=1;
			return 0;
		}
	}
	//If no space for new item, evict.
	if(least_index>=0){
		char * value_address= xmalloc(strlen(value)+1);
		int max_len;
		int max_key_len;
		if(strlen(value)>strlen(global_cache[least_index].value)){
			max_len=strlen(value);
		}
		else{
			max_len=strlen(global_cache[least_index].value);
			
		}
		if(strlen(key)>strlen(global_cache[least_index].key)){
			max_key_len=strlen(key);
		}
		else{
			max_key_len=strlen(global_cache[least_index].key);
		}
		xfree(global_cache[least_index].value);
		strncpy(value_address,value,max_len+1);
		global_cache[least_index].value=value_address;
		strncpy(global_cache[least_index].key,key,max_key_len);
		total_set_success+=1;
		total_evictions+=1;
		cache_size+=(strlen(key)-strlen(global_cache[least_index].key)+strlen(value)-strlen(global_cache[least_index].value));
		return 0;
	}
	return 1;
}

//Delete a key from the store
bool kv_delete(char key[]){
	int i;
	for(i=0;i<CACHESIZE;i++){
		if(strncmp(global_cache[i].key,key,MAX_CACHE_KEY)==0 && strlen(global_cache[i].key)!=0){
			global_cache[i].hits=0;
			cache_size-=(strlen(key)+strlen(global_cache[i].value));
			strncpy(global_cache[i].key,"",strlen(key));
			xfree(global_cache[i].value);
			num_keys--;
			return TRUE;
		}
	}
	return FALSE;
}

//Reset the cache
void kv_reset(){
	int i;
	for(i=0;i<CACHESIZE;i++){
		if(strlen(global_cache[i].key)!=0){
			global_cache[i].hits=0;
			strncpy(global_cache[i].key,"",strlen(global_cache[i].key));
			xfree(global_cache[i].value);
			global_cache[i].hits=0;

		}
	}
	cache_size=0;
	num_keys=0;
}

//Get k most popular keys from store.
char** most_popular_keys(int k){
	int i;
	char** output=(char **)getmem(k*sizeof(char*));
	qsort((char*)global_cache,num_keys,sizeof(hashunit),(int (*)(void))compare);
	for(i=0;i<k;i++){	
		output[i] = (char *)getmem(MAX_CACHE_KEY+1);
		strncpy(output[i],global_cache[i].key,strlen(global_cache[i].key)+1);
	}
	return output;
}

//Compare function to compare two hash unit structs.
int compare(const void * a, const void * b)
{
 	hashunit *x=(hashunit *)a;
 	hashunit *y=(hashunit *)b;
 	return (y->hits)-(x->hits);
}

//Get accounting information about the cache.
int get_cache_info(char* kind){
	if(strncmp(kind,"total_hits",20)==0)
		return total_hits;
	if(strncmp(kind,"total_accesses",20)==0)
		return total_accesses;
	if(strncmp(kind,"cache_size",20)==0)
		return cache_size;
	if(strncmp(kind,"num_keys",20)==0)
		return num_keys;
	if(strncmp(kind,"total_evictions",20)==0)
		return total_evictions;
	if(strncmp(kind,"total_set_success",20)==0)
		return total_set_success;
	return -1;
	
}
