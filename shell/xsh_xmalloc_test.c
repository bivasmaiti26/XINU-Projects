#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include<malloc.h>
shellcmd xsh_xmalloc_test(int argc,char* argv[]) {
	char *a = xmalloc(19);
	char *b = xmalloc(63);
	char *c = xmalloc(43);
	char *d = xmalloc(11);
	char *e = xmalloc(1);
	char *f = xmalloc(4);
	char *g = xmalloc(8192);
	printf("Address of a=%d for size 19\nAddress of b=%d for size 63\nAddress of c=%d for size 43\nAddress of d=%d for size 11\nAddress of e=%d for size 1\nAddress of f=%d for size 4\nAddress of g=%d for size 8197\n",a,b,c,d,e,f,g );
	char * snap=xheap_snapshot();
	printf("%s\n",snap);
	xfree(a);
	xfree(b);
	xfree(d);
	xfree(f);
	printf("a,b,d,f freed\n");
	snap=xheap_snapshot();
	printf("New Snap:\n%s\n",snap);
	freemem(snap,nbpools*200);
}