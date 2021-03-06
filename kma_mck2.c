/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the McKusick-Karels
 *              algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_mck2.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_mck2.c,v $
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_MCK2
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

typedef struct
{
	void* 	nextbuffer;
} kbuffer_t;

typedef struct
{
	kbuffer_t* buffer;
} klistheader_t;

typedef struct
{
	void* itself;
	int numpages;//from 0 to max, the 1st one is 0
	int numalloc;// 0 means nothing//each page hold one , if 0 then free
	int size;
	int full;
	klistheader_t p2fl[10];
} kpageheader_t;

/************Global Variables*********************************************/

kpageheader_t* mainpage=0;

/************Function Prototypes******************************************/

kma_size_t roundup(kma_size_t size);
kpageheader_t* initpage(kpage_t* page, kma_size_t roundsize);
void insertbuffer(klistheader_t* thefreelist, kbuffer_t* thebuffer);
kbuffer_t* unlinkbuffer(klistheader_t* thefreelist);
int chkfreelist(kma_size_t roundsize);// we should limite the boundare of the free list
int chkemptypage();
kbuffer_t* unlinkbufaddr(klistheader_t* thefreelist, kbuffer_t* thebufaddr);
//kpageheader_t* chkfreepage();

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if ((size + sizeof(void*)) > PAGESIZE){ // requested size too large
		return NULL;
	}
	if(!mainpage){// initialized the entry
		mainpage=initpage(get_page(), 16);//	16 for the most used page
	}
	//printf("%d\n",size);
	int roundsize=roundup(size);
	int i;
	int listindex=0;
	while((16<<listindex)!=roundsize)
	{
		listindex++;
	}
	klistheader_t* thelist;
	thelist=&((*mainpage).p2fl[listindex]);
	
	kbuffer_t* ret;
	//printf("%lx\n",(long int)thelist);
	if((i=chkfreelist(roundsize)))
	{
		kpageheader_t* thepage;
		ret=unlinkbuffer(thelist);
		
		thepage=(kpageheader_t*)(((long int)(((long int)ret-(long int)mainpage)/PAGESIZE))*PAGESIZE + (long int)mainpage);
		(*mainpage).numalloc++;
		(*thepage).numalloc++;
		return ret;
	}
	else if((i=chkemptypage())){
		kpageheader_t* theexistpage;
		theexistpage=(kpageheader_t*)((long int)mainpage + i * PAGESIZE);
		theexistpage=initpage((*theexistpage).itself, roundsize);
		
		ret=unlinkbuffer(thelist);
		(*mainpage).numalloc++;
		(*theexistpage).numalloc++;
		
		return ret;
	}
	else
	{
		kpageheader_t* newpage=initpage(get_page(), roundsize);
		(*mainpage).numpages++;
		
		ret=unlinkbuffer(thelist);
		(*mainpage).numalloc++;
		(*newpage).numalloc++;
		
		return ret;
	}
  return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	int roundsize=roundup(size);
	int listindex=0;
	while((16<<listindex)!=roundsize)
	{
		listindex++;
	}
	klistheader_t* thelist;
	kpageheader_t* thepage;
	thelist=&((*mainpage).p2fl[listindex]);
	thepage=(kpageheader_t*)(((long int)(((long int)ptr-(long int)mainpage)/PAGESIZE))*PAGESIZE + (long int)mainpage);
	
	insertbuffer(thelist, (kbuffer_t*)ptr);
	(*mainpage).numalloc--;
	(*thepage).numalloc--;
	if((*thepage).numalloc==0){
		(*thepage).full=0;
		
		if(roundsize==8192){// we cannot use all the page to store the data, as the header exist, but all the request are less than 8000, we can still alloc that
			unlinkbufaddr(&((*mainpage).p2fl[9]), (kbuffer_t*)((long int)thepage + sizeof(kpageheader_t)));
		}
		else if(roundsize==4096){// due to the header, we cannot divide the page into two block of 4096
			unlinkbufaddr(&((*mainpage).p2fl[8]), (kbuffer_t*)((long int)thepage + sizeof(kpageheader_t)));
		}
		else{
			kbuffer_t* startbuf;
			kbuffer_t* endbuf;
			kbuffer_t* tempbuf;
			startbuf=(kbuffer_t*)((long int)thepage + sizeof(kpageheader_t));
			endbuf=(kbuffer_t*)((long int)thepage + PAGESIZE - roundsize);
			for(tempbuf = startbuf; tempbuf < endbuf; tempbuf = (kbuffer_t*)((long int)tempbuf + roundsize))
			{
				//(*tempbuf).nextbuffer = (void*)((long int)tempbuf + roundsize);
			}
			tempbuf = (kbuffer_t*)((long int)tempbuf - roundsize);//	back to the last one
			//(*tempbuf).nextbuffer=0;
			unlinkbufaddr(&((*mainpage).p2fl[listindex]), tempbuf);
			(*startbuf).nextbuffer = (*tempbuf).nextbuffer;
			unlinkbufaddr(&((*mainpage).p2fl[listindex]), startbuf);
			//insertbuffer(&((*mainpage).p2fl[listindex]), tempbuf);// insert the last one
			//insertbuffer(&((*mainpage).p2fl[listindex]), startbuf);//	insert the first one
			//(*startbuf).nextbuffer = (void*)((long int)startbuf + roundsize);// so all the buffers are linked
		}
		
	}
	
	kpageheader_t* thelastpage;
	thelastpage=(kpageheader_t*)((long int)mainpage + ((*mainpage).numpages) * PAGESIZE);
	if(thepage==thelastpage)
	{
		while((*thelastpage).numalloc==0){
			(*mainpage).numpages--;
			free_page((*thelastpage).itself);
			if(mainpage==thelastpage){
				mainpage=0;
				break;
			}
			thelastpage=(kpageheader_t*)((long int)mainpage + ((*mainpage).numpages) * PAGESIZE);
		}
	}
  ;
}

kma_size_t roundup(kma_size_t size){
	kma_size_t ret=16;
	while(size>ret){
		ret=ret<<1;
	}
	return ret;
}

void insertbuffer(klistheader_t* thefreelist, kbuffer_t* thebuffer){
	if((*thefreelist).buffer==0)// the first one
	{
		(*thefreelist).buffer=thebuffer;
		(*thebuffer).nextbuffer=0;
	}
	else if((*thefreelist).buffer>thebuffer){
		(*thebuffer).nextbuffer=(*thefreelist).buffer;
		(*thefreelist).buffer=thebuffer;
	}
	else{
		void* thenextbuffer;
		kbuffer_t* temp;
		thenextbuffer=(*thefreelist).buffer;
		while((*(kbuffer_t*)thenextbuffer).nextbuffer){
			if((*(kbuffer_t*)thenextbuffer).nextbuffer>(void*)thebuffer)break;
			thenextbuffer=(*(kbuffer_t*)thenextbuffer).nextbuffer;
		}
		temp=(*(kbuffer_t*)thenextbuffer).nextbuffer;
		(*(kbuffer_t*)thenextbuffer).nextbuffer=thebuffer;
		(*thebuffer).nextbuffer=temp;
	}
}

kbuffer_t* unlinkbuffer(klistheader_t* thefreelist){
	kbuffer_t* ret;
	ret=(*thefreelist).buffer;
	(*thefreelist).buffer=(*ret).nextbuffer;
	return (kbuffer_t*)ret;
}

kpageheader_t* initpage(kpage_t* page, kma_size_t roundsize){
	kpageheader_t* ret;
	*((kpage_t**)page->ptr) = page;
	
	ret=(kpageheader_t*)((*page).ptr);
	if(mainpage==0)mainpage=ret;
	
	int i;
	for(i = 0; i < 10; ++i)
	{
		((*ret).p2fl[i]).buffer=0;
		//((*ret).p2fl[i]).size=1<<(i+4);
	}
	(*ret).numpages=0;
	(*ret).numalloc=0;
	(*ret).size=roundsize;
	(*ret).full=1;
	
	if(roundsize==8192){
		insertbuffer(&((*mainpage).p2fl[9]), (kbuffer_t*)((long int)ret + sizeof(kpageheader_t)));
	}
	else if(roundsize==4096){
		insertbuffer(&((*mainpage).p2fl[8]), (kbuffer_t*)((long int)ret + sizeof(kpageheader_t)));
	}
	else{
		kbuffer_t* startbuf;
		kbuffer_t* endbuf;
		kbuffer_t* tempbuf;
		startbuf=(kbuffer_t*)((long int)ret + sizeof(kpageheader_t));
		endbuf=(kbuffer_t*)((long int)ret + PAGESIZE - roundsize);
		for(tempbuf = startbuf; tempbuf < endbuf; tempbuf = (kbuffer_t*)((long int)tempbuf + roundsize))
		{
			(*tempbuf).nextbuffer = (void*)((long int)tempbuf + roundsize);
		}
		tempbuf = (kbuffer_t*)((long int)tempbuf - roundsize);//	back to the last one
		(*tempbuf).nextbuffer=0;
		int listindex=0;
		while((16<<listindex)!=roundsize)
		{
			listindex++;
		}
		insertbuffer(&((*mainpage).p2fl[listindex]), tempbuf);// insert the last one
		insertbuffer(&((*mainpage).p2fl[listindex]), startbuf);//	insert the first one
		(*startbuf).nextbuffer = (void*)((long int)startbuf + roundsize);// so all the buffers are linked
	}
	
	return ret;
}

int chkfreelist(kma_size_t roundsize){
	void* endptr;
	endptr=(void*)((long int)mainpage + ((*mainpage).numpages + 1) * PAGESIZE);
	
	int listindex=0;
	while((16<<listindex)!=roundsize)
	{
		listindex++;
	}
	
	kbuffer_t* thebuffer;
	thebuffer=(*mainpage).p2fl[listindex].buffer;
	
	if((thebuffer != 0) && ((void*)thebuffer < (void*)endptr))return listindex+1;
	return 0;
}

int chkemptypage(){
	int ret;
	kpageheader_t* temppage;
	for(ret = 0; ret < (*mainpage).numpages; ++ret)
	{
		temppage=(kpageheader_t*)((long int)mainpage + ret * PAGESIZE);
		if((*temppage).full==0)return ret;
	}
	return 0;
}

kbuffer_t* unlinkbufaddr(klistheader_t* thefreelist, kbuffer_t* thebufaddr){
	kbuffer_t* ret = 0;
	void* thenextbuffer;
	thenextbuffer=(*thefreelist).buffer;
	if(thebufaddr==thenextbuffer)
	{
		ret=thebufaddr;
		(*thefreelist).buffer=(*ret).nextbuffer;
		return ret;
	}
	while(thenextbuffer){
		if((*(kbuffer_t*)thenextbuffer).nextbuffer==thebufaddr){// we find it!
			ret=thebufaddr;
			(*(kbuffer_t*)thenextbuffer).nextbuffer=(*ret).nextbuffer;
			return ret;
		}
		thenextbuffer=(*(kbuffer_t*)thenextbuffer).nextbuffer;
	}
	return 0; // it should be free
}

#endif // KMA_MCK2
