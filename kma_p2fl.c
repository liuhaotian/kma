/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_p2fl.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_p2fl.c,v $
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
#ifdef KMA_P2FL
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
	void* nextbuffer;
} kbuffer_t;

typedef struct
{
	kbuffer_t* buffer;// if it equals the nestlist, it is used
	int size;
} kfreelist_t;

typedef struct
{
	void* itself;
	void* nextpage;
	kfreelist_t p2fl[10];
	void* freememory;
} klistheader_t;

/************Global Variables*********************************************/

kpage_t* gentryptr=0;

/************Function Prototypes******************************************/
kma_size_t roundup(kma_size_t size);
int chkfreespace(klistheader_t* page,kma_size_t size);// 0:not enough, >1: enough space
kpage_t* initial(kpage_t* page);
void* addbuffer(klistheader_t* page,kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if ((size + sizeof(void*)) > PAGESIZE){ // requested size too large
		return NULL;
	}
	int i;
	void* ret;
	klistheader_t* page;
	
	if(!gentryptr){// initialized the entry
		gentryptr=initial(get_page());
		page=(klistheader_t*)(gentryptr->ptr);
	}
	page=(klistheader_t*)(gentryptr->ptr);
	
	i=roundup(size);
	i=chkfreespace(page,size);
	printf("%ld\n",(long int)((*page).freememory));
	ret=addbuffer(page,roundup(size));
	long int pagestartaddr=(long int)(page);
	long int pageretstartaddr=(long int)(ret);
	
	printf("%ld\n",pagestartaddr);
	printf("%ld\n",pageretstartaddr);
	printf("%ld\n",(long int)((*page).freememory));

	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	
  ;//we need to clean the  freelist when there is nothing left
}

kma_size_t roundup(kma_size_t size){
	kma_size_t ret=16;
	while(size>ret){
		ret=ret<<1;
	}
	return ret;
}

kpage_t* initial(kpage_t* page){
	klistheader_t* listheader;
	int i;
	*((kpage_t**)page->ptr) = page;
	
	listheader=(klistheader_t*)(page->ptr);
	for(i = 0; i < 10; ++i)
	{
		((*listheader).p2fl[i]).buffer=0;
		((*listheader).p2fl[i]).size=1<<(i+4);
	}
	(*listheader).freememory=(void*)((long int)listheader+sizeof(klistheader_t));
	return page;
}

int chkfreespace(klistheader_t* page,kma_size_t size){
	return (bool)(PAGESIZE+(long int)page-(long int)((*page).freememory))/(size+sizeof(kbuffer_t));
}

void* addbuffer(klistheader_t* page,kma_size_t size){
	int i=4;
	void* ret;
	kbuffer_t* buffer;
	kfreelist_t* freelist;
	while((1<<i)!=size)
	{
		i++;
	}
	i=i-4;
	buffer=(kbuffer_t*)((*page).freememory);
	freelist=(kfreelist_t*)&((*page).p2fl[i]);
	ret=(void*)buffer+sizeof(kbuffer_t);
	*(kfreelist_t**)(buffer)=freelist;//point the list it belonging to
	(*page).freememory=(void*)(buffer+size+sizeof(kbuffer_t));
	return ret;
/*	
//	((*page).p2fl[i]).buffer=(*page).freememory;
	(void*)(*(void**)(((*page).freememory)))=(void*)((*page).p2fl[i]);//point the list it belonging to
	ret=(void*)((long int)((*page).freememory)+sizeof(kbuffer_t));
	(*page).freememory=(void*)((long int)((*page).freememory)+size+sizeof(kbuffer_t));// point to the next available memory
	return ret;
*/
}

#endif // KMA_P2FL
