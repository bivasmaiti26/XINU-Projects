#include <xinu.h>
#include <prodcons.h>
#include<future.h>
sid32 produced, consumed;

shellcmd xsh_prodcons(int nargs, char *args[])
{
  if(nargs==1){
		  
		  //Argument verifications and validations
	  produced = semcreate(0);
	  consumed = semcreate(1);

	  int count = 2000;             //local varible to hold count
	  
	  //check args[1] if present assign value to count
	  
	  //create the process producer and consumer and put them in ready queue.
	  //Look at the definations of function create and resume in exinu/system folder for reference.      
	  resume( create(producer, 1024, 20, "producer", 1, count));
	  resume( create(consumer, 1024, 20, "consumer", 1, count));
	  //return (0);
  }
  else if (nargs==2)
  {
	  if(args[1][0]=='-' && args[1][1]=='f'){
		future_t* f_exclusive,* f_shared,* f_queue;
		f_exclusive = future_alloc(FUTURE_EXCLUSIVE);
		f_shared    = future_alloc(FUTURE_SHARED);
		f_queue     = future_alloc(FUTURE_QUEUE);
		resume( create(future_cons, 1024, 20, "fcons1", 1, f_exclusive) );
		resume( create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, 1) );
		
		resume( create(future_cons, 1024, 20, "fcons2", 1, f_shared) );
	 	resume( create(future_cons, 1024, 20, "fcons3", 1, f_shared) );
	  	resume( create(future_cons, 1024, 20, "fcons4", 1, f_shared) ); 
	 	resume( create(future_cons, 1024, 20, "fcons5", 1, f_shared) );
	  	resume( create(future_prod, 1024, 20, "fprod2", 2, f_shared, 2) );
		
		resume( create(future_cons, 1024, 20, "fcons6", 1, f_queue) );
		resume( create(future_cons, 1024, 20, "fcons7", 1, f_queue) );
		resume( create(future_cons, 1024, 20, "fcons8", 1, f_queue) );
		resume( create(future_cons, 1024, 20, "fcons9", 1, f_queue) );
		resume( create(future_prod, 1024, 20, "fprod3", 2, f_queue, 3) );
		resume( create(future_prod, 1024, 20, "fprod4", 2, f_queue, 4) );
		resume( create(future_prod, 1024, 20, "fprod5", 2, f_queue, 5) );		
		resume( create(future_prod, 1024, 20, "fprod6", 2, f_queue, 6) );
	  	//sleep(5);
	  	//printf("f_exclusive status=[%d]\n",f_exclusive->state);
	  	//printf("f_shared status=[%d]\n",f_shared->state);
	  	//printf("f_queue status=[%d]\n",f_queue->state);
	  	sleep(1);
	  	//Check if nothing is present in the queues
	  	while(!(f_exclusive->get_queue==NULL && f_shared->get_queue==NULL && f_queue->get_queue==NULL  && f_exclusive->set_queue==NULL && f_shared->set_queue==NULL && f_queue->set_queue==NULL)){
	  		sleep(2);
	  	}

		
	  	
	  	
	  }
	  else{
		  printf("invalid arguements");
	  }

  }
  return (0);
}

