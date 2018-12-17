# -*- MakeFile -*-
#  #
#  target: dependencies
#  #       action 
pargrep: pargrep.o
	gcc pargrep.o -o pargrep -pthread
pargrep.o: pargrep.c
	gcc pargrep.c -c 
clean: 
	rm -f *.o pargrep

