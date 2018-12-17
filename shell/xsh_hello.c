#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  * xsh_hello - Simple hello command to input string and print a hello message
 *   *------------------------------------------------------------------------
*/
shellcmd xsh_hello(int argc,char* argv[]) {
	if(argc==2){
		//Print the hello message
		printf("Hello %s, Welcome to the world of Xinu!!\n",argv[1]);
	}
	else if(argc==1){
		//Too few arguements
		printf("Too few arguements");
	}
	else if(argc>2)
	{
		//Too many arguements
		printf("Too many arguements");
	}
	return 0;
}
