#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void*(*rmmap)(void*,size_t,int,int,int,long long)=NULL;
int(*rmunmap)(void*,size_t)=NULL;
void*(*rbrk)(const void*)=NULL;
void*(*rsbrk)(int)=NULL;

size_t heap_size=0;

void* mmap(void* addr,size_t len,int prot,int flags,int fd,long long offset)
{
	if(!rmmap)
	{
		rmmap=dlsym(RTLD_NEXT,"mmap");
		const char* emsg=NULL;
		if((emsg=dlerror()))
		{
			fputs("dlsym Error: ",stderr);
			fputs(emsg,stderr);
			fputs("\n",stderr);
			abort();
		}
	}
	void* ret=rmmap(addr,len,prot,flags,fd,offset);
	if(ret)
		heap_size+=len;
	return ret;
}

int munmap(void* addr,size_t len)
{
	if(!rmunmap)
	{
		rmunmap=dlsym(RTLD_NEXT,"munmap");
		const char* emsg=NULL;
		if((emsg=dlerror()))
		{
			fputs("dlsym Error: ",stderr);
			fputs(emsg,stderr);
			fputs("\n",stderr);
			abort();
		}
	}
	int ret=rmunmap(addr,len);
	if(ret==0)
		heap_size-=len;
	return ret;
}

void* brk(const void* addr)
{
	if(!rbrk)
	{
		rbrk=dlsym(RTLD_NEXT,"brk");
		const char* emsg=NULL;
		if((emsg=dlerror()))
		{
			fputs("dlsym Error: ",stderr);
			fputs(emsg,stderr);
			fputs("\n",stderr);
			abort();
		}
	}
	void* ret=rbrk(addr);
	return ret;
}

void* sbrk(int incr)
{
	if(!rsbrk)
	{
		rsbrk=dlsym(RTLD_NEXT,"sbrk");
		const char* emsg=NULL;
		if((emsg=dlerror()))
		{
			fputs("dlsym Error: ",stderr);
			fputs(emsg,stderr);
			fputs("\n",stderr);
			abort();
		}
	}
	void* ret=rsbrk(incr);
	return ret;
}








