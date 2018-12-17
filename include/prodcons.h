#include<future.h>
/*Global variable for producer consumer*/
extern int n; /*this is just declaration*/

/*function Prototype*/
void consumer(int);
void producer(int);
uint future_prod(future_t* ,int);
uint future_cons(future_t*);
/* Semaphore Declarations */
extern sid32 produced, consumed;
