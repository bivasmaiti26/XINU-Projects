#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
int main(int argv, char *argc[])
{
	pid_t pid;
	int fd[2];
	int ret;
	//Create a pipe
	ret=pipe(fd);
	char buffer[100];
	if(ret==-1)
	{
		//Error in creating pipe
		fprintf(stderr, "Pipe error!\n");
		exit(1);
	}
	//Fork a child process
	pid=fork();
	if(pid<0)
	{
		//Error in Fork
		fprintf(stderr, "Fork error!\n");
		exit(1);
	}
	else if(pid==0)
	{
		//Child Process
		printf("Child PID=%d\n",getpid());
		//Read the message in pipe from Parent
		read(fd[0],buffer,100);
		int estatus;
		if(strcmp(buffer,"/bin/echo")==0)
		{
			//When command is echo
			char* chargv[2] ;
			chargv[0] = buffer ;
			chargv[1] = "Hello World!";
                	estatus=execl(chargv[0],chargv[0],chargv[1],(char *)NULL);
		}
		else
		{
			//When command is anything else
			char* chargv[1] ;
			chargv[0]=buffer;
	                estatus=execl(chargv[0],chargv[0],(char *)NULL);
		}
		if (estatus) {
			printf("Exec error! %s \n", strerror(errno));
      			exit(1) ;
    		}	

	}	
	else{
		//Parent Process
		printf("Parent PID=%d\n",getpid());
		//Write the supplied arguements to pipe
		write(fd[1],argc[1],100);
		//Wait for the child process to complete
		wait(NULL);
	}
}
