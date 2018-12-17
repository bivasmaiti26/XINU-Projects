#include<xinu.h>
#include<future.h>
#include<stdio.h>
#include<stdlib.h>
//Implement all future functions
//Allocates a new future (in the FUTURE_EMPTY state) with the given mode.
// We will use the getmem() call to allocate space to new future
future_t* future_alloc(future_mode_t mode){
	future_t* fut=(future_t *)getmem(sizeof(future_t));
	if(fut==(future_t*)SYSERR){
			return (future_t*)SYSERR;
	}

	fut-> mode=mode;
	fut-> state=FUTURE_EMPTY;
	fut->set_queue=NULL;
	fut->get_queue=NULL;
	return fut; 
}
//Frees the allocated future. Use the freemem() system call to free the space.
syscall future_free(future_t* f){
	if(f->state==FUTURE_EMPTY){
			freemem((char*)(f),sizeof(future_t));
			return OK;
	}
	else{
		//Resume all waiting queues and free the memory
		while(f->get_queue!=NULL){
			resume(f->get_queue->pid);
			future_item *fi=f->get_queue;
			f->get_queue=(future_item *)f->get_queue->fqnext;
			freemem((char*)(fi),sizeof(future_item));
		}
		while(f->set_queue!=NULL){
			resume(f->set_queue->pid);
			future_item *fi=f->set_queue;
			f->set_queue=(future_item *)f->set_queue->fqnext;
			freemem((char*)(fi),sizeof(future_item));
		}
		freemem((char*)(f),sizeof(future_t));
		return OK;
	}
}
//Get the value of a future set by an operation and may change the state of future.
syscall future_get(future_t* f,int* address){
	
	future_item *fi;
	future_item *fn;
	pid32 pid;
	//Disable interrupts
	intmask mask;
	mask=disable();
	//For Future Empty, Enqueue in get_queue, suspend the process and change status to FUTURE_WAITING
	if(f->state==FUTURE_EMPTY){ 
		fi=(future_item *)getmem(sizeof(future_item));
		if(fi==(future_item*)SYSERR){
			restore(mask);
			return SYSERR;
		}
		fi->pid=getpid();
		fi->fqnext=NULL;
		f->get_queue=fi;
		f->state=FUTURE_WAITING;
		suspend(getpid());
		if(f->state==FUTURE_EXCLUSIVE){
			f->get_queue=NULL;
		}
		*address=f->value;
		restore(mask);
		return OK;

	}
	//For FUTURE_WAITING, Enqueue in get_queue and suspend the process 
	if(f->state==FUTURE_WAITING){
		//Insert the process at the end of the queue
		switch(f->mode){
			//Dont enqueue, since only 1 process can be in the queue
			case FUTURE_EXCLUSIVE:
				restore(mask);
				return SYSERR;
				break;
			case FUTURE_SHARED:

			case FUTURE_QUEUE:
				fn=(future_item *)getmem(sizeof(future_item));
				if(fn==(future_item*)SYSERR){
					restore(mask);
					return SYSERR;
				}
				if(f->get_queue!=NULL){
					fi=f->get_queue;
					while(1){
						if(fi->fqnext==NULL){
							fn->pid=getpid();
							fn->fqnext=NULL;
							fi->fqnext=(struct future_item *)fn;
							suspend(getpid());
							break;
						}
						fi=(future_item *)fi->fqnext;

					}
					fi=f->get_queue;
					while(fi!=NULL){
						fi=(future_item *)fi->fqnext;
					}
					//After Suspension, for future_queue mode
					if(f->value!=NULL){
						*address=f->value;
						restore(mask);
						return OK;
					}
				}
				break;
		}
	}
	if(f->state== FUTURE_READY){
		//Get the value from future
		//Set the value at the address
		//If mode=FUTURE_EXCLUSIVE, set status to FUTURE_EMPTY
		switch(f->mode){
			case FUTURE_EXCLUSIVE:
				*address=f->value;
				f->state=FUTURE_EMPTY;
				if(f->set_queue==NULL && f->get_queue==NULL){
					f->state=FUTURE_EMPTY;
				}
				restore(mask);
				return OK;
			case FUTURE_SHARED:
				*address=f->value;
				restore(mask);	
				return OK;
			case FUTURE_QUEUE:
				*address=f->value;
				if(f->set_queue!=NULL){
					fi=f->set_queue;
					f->set_queue=(future_item *)fi->fqnext;
					if(fi->fqnext==NULL){
						f->state=FUTURE_EMPTY;
					}
					pid=fi->pid;
					freemem((char*)(fi),sizeof(future_item));
					resume(pid);
				}
				else
				{
						f->state=FUTURE_EMPTY;
				}	
		}
	}
	restore(mask);
	return OK;
}



syscall future_set(future_t* f,int value){
	future_item* fi;
	future_item* fn;
	pid32 pid;
	intmask mask;
	mask=disable();
	//Set State as FUTURE_READY, set value in future and return.
	if(f->state==FUTURE_EMPTY){ 
		f->value=value;
		f->state=FUTURE_READY;
		restore(mask);
		return OK;
	}
	//FUTURE_READY: Enqueu in set_queue if FUTURE_QUEUE, else return SYSERR
	if(f->state==FUTURE_READY){ 
		switch(f->mode){
			case FUTURE_EXCLUSIVE:
			case FUTURE_SHARED:
				restore(mask);
				return SYSERR;
			case FUTURE_QUEUE:
				fi=(future_item *)getmem(sizeof(future_item));
				fn=(future_item *)getmem(sizeof(future_item));
				if(fi==(future_item*)SYSERR){
					restore(mask);
					return SYSERR;
				}
				if(fn==(future_item*)SYSERR){
					restore(mask);
					return SYSERR;
				}
				//If set queue has values, traverse and enqueue, then suspend
				if(f->set_queue!=NULL){
			
					fi=f->set_queue;
					while(1)
					{
						if(fi->fqnext==NULL)
						{
							fn->pid=currpid;
							fn->fqnext=NULL;
							fi->fqnext=(struct future_item *)fn;
							suspend(getpid());
							break;
						}
						fi=(future_item *)fi->fqnext;
					}
				}
				else{
					fi->pid=getpid();
					fi->fqnext=NULL;
					f->set_queue=fi;
					suspend(getpid());
				}
				f->value=value;
				f->state=FUTURE_READY;
				restore(mask);
				return OK;
				
		}
	}
	//Resume processes waiting in the get queue, change status according to mode and data in set queue
	if(f->state==FUTURE_WAITING){ 
		switch(f->mode){
			case FUTURE_EXCLUSIVE:
				f->value=value;
				f->state=FUTURE_READY;
				pid=f->get_queue->pid;
				freemem((char*)(f->get_queue),sizeof(future_item));
				resume(pid);
			case FUTURE_SHARED:
			//Dequeue all processes from get queue and resume them. Also free their memory. When all done, set state to 
			//FUTURE_READY, since we have value
				fi=f->get_queue;
				if(f->get_queue!=NULL){
					f->value=value;
					f->state=FUTURE_READY;
					while(1){
						fn=(future_item *)fi->fqnext;
						pid=fi->pid;
						freemem((char*)(fi),sizeof(future_item));

						resume(pid);
						if(fn==NULL){
							f->get_queue=NULL;
							f->state=FUTURE_READY;
							break;
						}
						fi=fn;
						
					}
				}
				break;
				
			case FUTURE_QUEUE:
			//Dequeue 1 process from get_queue, if last, set status to .FUTURE_EMPTY
				if(f->get_queue!=NULL){
					f->value=value;
					fi=f->get_queue;
					pid=fi->pid;
					f->get_queue=(future_item *)fi->fqnext;
					if(f->get_queue==NULL){
						f->state=FUTURE_EMPTY;
					}
					resume(pid);
				}
				else{
					f->value=value;
					f->state=FUTURE_READY;
				}
		}
	}
	restore(mask);
	return OK;
}