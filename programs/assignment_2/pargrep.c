#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct file_io
{
   int	partid;
   char *filename;
   char *output;
   int numbytes;
   char *word;
};
int checkNum(char *arg)
{
	char *ptr;
	long ret;
	arg++;
	ret = strtol(arg, &ptr, 10);
	//printf("%d",ret);
	return (int)ret;
}
//File processing Thread function
void* processbytes(void* data)
{
	char *line = NULL;
    size_t len = 0;
	struct file_io *data_struct =(struct file_io*) data;
	long long size=data_struct->numbytes;
	FILE *stream;
	stream = fopen(data_struct->filename, "r");
	if(data_struct->partid!=0)
		fseek(stream,data_struct->partid*data_struct->numbytes-1,SEEK_SET);
	else
		fseek(stream,data_struct->partid*data_struct->numbytes,SEEK_SET);
	int initial_pos=ftell(stream);
	int i=0;
	data_struct->output=malloc(size*2);
	data_struct->output[0]='\0';
	while (getline(&line, &len, stream) != -1) {
		int bytesprocessed=ftell(stream)-initial_pos;
		if(strstr(line,data_struct->word)!=NULL)
		{
			if(!(line[0]!='\n' && i==0&& data_struct->partid!=0)){
				strcat(data_struct->output,line);
			}
		}			
		i++;
		if(bytesprocessed>=data_struct->numbytes)
		{
			fclose(stream);
			free(line);
			pthread_exit(0);
		}
	}
	fclose(stream);
	free(line);
	pthread_exit(0);
}
//process the file taken as input with threads
void process_file(char * filename,int num,char *word){
	//Process file with threads
	FILE *stream;
	//char *final;
	stream = fopen(filename, "r");

	if(stream==NULL)
	{
		puts("Can't open that file!");
		exit(1);
	}
	else{
		int i=0;
		//Goto the end of file
		fseek(stream, 0, SEEK_END);
		//length of the file
		int len = ftell(stream);
		fclose(stream);
		if(num==0)
		{
			num=(len/90000)+1;
		}
		//final=(char *)malloc(len);
		//bytes to be processed per thread
		int bytesperthread=len/num;
		//Struct array for threads
		struct file_io file_io_arr[num];
		//thread ids for threads
		pthread_t tids[num];
		for (i = 0; i < num; i++) {
			//Struct values
			file_io_arr[i].partid=i;
			file_io_arr[i].filename=filename;
			file_io_arr[i].numbytes=bytesperthread;
			file_io_arr[i].word=word;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_create(&tids[i], &attr, processbytes, &file_io_arr[i]);
		}
		for (i = 0; i < num; i++) {
			pthread_join(tids[i], NULL);
			char* ptr=file_io_arr[i].output;
			fputs(file_io_arr[i].output,stdout);
			free(file_io_arr[i].output);
		}
		//printf("%lld,%d",len,num);
	}
	
}
//The main process
int main(int argc, char *argv[])
{
	int num_threads=0;
	char *word;
	char *filename;
    char *line = NULL;
    size_t len = 0;
	ssize_t nread;
	if(argc<2)
	{
		printf("Too few arguements");
		exit(-1);
	}
	else if(argc==2)
	{
		//For cat ./myfile | pargrep word 
	    word=argv[1];
		//int i=0;
		while (getline(&line, &len, stdin) != -1) {
			if((strstr(line,word)!=NULL))
			{
				fputs(line,stdout);
			}
		}
	}
	else if(argc==3)
	{
		//pargrep [-t] word for piped output
		//pargrep word [file]
		if(argv[1][0]=='-')
		{
			num_threads=checkNum(argv[1]);
			if(num_threads<=0)
			{
				printf("invalid arguement, please provide valid number of threads");
				exit(-1);
			}
			//search word is always argv[2]
			word=argv[2];
			while (getline(&line, &len, stdin) != -1) {
			if((strstr(line,word)!=NULL))
				fputs(line,stdout);
			}	
		}
		else{
			word=argv[1];
			filename=argv[2];
			//Do file Stuff
			process_file(filename,num_threads,word);
		}
	}
	else if(argc==4)
	{
		//pargrep [-t] word [file]
		num_threads=checkNum(argv[1]);
		if(num_threads<=0)
		{
			printf("invalid arguement, please provide valid number of threads");
			exit(-1);
		}
		word=argv[2];
		filename=argv[3];
		//Do file stuff
		process_file(filename,num_threads,word);
		
	}
}