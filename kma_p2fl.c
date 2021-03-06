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
	int size;
	kbuffer_t* buffer;
} kfreelist_t;

typedef struct
{
	void* itself;
	int numpages;//from 0 to max, the 1st one is 0
	int numalloc;// 0 means nothing//each page hold one , if 0 then free
	void* freememory;
	kfreelist_t p2fl[10];
} klistheader_t;

/************Global Variables*********************************************/

kpage_t* gentryptr=0;

/************Function Prototypes******************************************/
kma_size_t roundup(kma_size_t size);
int chkfreespace(kma_size_t size);// 0:not enough, >1: enough space
int chkfreelist(kma_size_t size);// find it in list
kpage_t* initial(kpage_t* page);
void* buffermalloc(klistheader_t* page,kma_size_t size);

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
	klistheader_t* mainpage;
	klistheader_t* temppage;
	
	if(!gentryptr){// initialized the entry
		gentryptr=initial(get_page());
	}
	
	mainpage=(klistheader_t*)(gentryptr->ptr);
	
	if((i=chkfreelist(size)))
	{
		i--;
		int listindex=4;
		while((1<<listindex)!=roundup(size))
		{
			listindex++;
		}
		listindex=listindex-4;
		temppage=(klistheader_t*)((long int)mainpage + i * PAGESIZE);
		ret=(void*)(((*temppage).p2fl[listindex]).buffer);
		((*temppage).p2fl[i]).buffer=(kbuffer_t*)((*((kbuffer_t*)ret)).nextbuffer);
		(*temppage).numalloc++;//	current page's number of malloc
		(*mainpage).numalloc++;//	global number of malloc
		return ret;
	}
	else if((i=chkfreespace(size)))
	{
		i--;
		temppage=(klistheader_t*)((long int)mainpage + i * PAGESIZE);
		ret=buffermalloc(temppage,size);// already incease the (*temppage).numalloc++;
		return ret;
	}
	else{
		initial(get_page());
		(*mainpage).numpages++;
		i=(*mainpage).numpages;
		temppage=(klistheader_t*)((long int)mainpage + i * PAGESIZE);
		ret=buffermalloc(temppage,size);// already incease the (*temppage).numalloc++;
		return ret;
	}
	// find in list
	// find for free space
	// create new pages
	
	/*
	i=roundup(size);
	i=chkfreespace(size);
	ret=buffermalloc(page,size);
	return ret;
*/
	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)// when delete, we add the buffer to the gentrylist
{
	kbuffer_t* buffer;
	void* nextbuffer;
	kfreelist_t* freelist;
	buffer = (kbuffer_t*)((long int)ptr-sizeof(kbuffer_t*));
	freelist = (kfreelist_t*)((*buffer).nextbuffer);
	
	nextbuffer=((*freelist).buffer);
	(*buffer).nextbuffer=nextbuffer;
	
	klistheader_t* mainpage=(klistheader_t*)(gentryptr->ptr);
	klistheader_t* temppage;
	int i;
	i=((long int)ptr-(long int)mainpage)/PAGESIZE;
	temppage=(klistheader_t*)((long int)mainpage + i * PAGESIZE);
	(*temppage).numalloc--;
	(*mainpage).numalloc--;
	
	if((*temppage).numalloc==0){
		kpage_t* page0;
/*		for(; (*mainpage).numpages >=0; (*mainpage).numpages--)
		{
			temppage = *((kpage_t**)((long int)mainpage + (*mainpage).numpages * PAGESIZE));
			free_page(temppage);
		}*/
		page0 = *(kpage_t**)(temppage);
		initial(page0);// it is no use, but we must keep all the pages sequential, so just initialize it
		//free_page(page0);
		//(*mainpage).numpages is -1 no
		//gentryptr=0;//all pages are free
	}
	if((*mainpage).numalloc==0){//	there is nothing left, we must free all
		kpage_t* page1;
		page1 = *(kpage_t**)(mainpage);
		for(; (*mainpage).numpages >0; (*mainpage).numpages--)
		{
			page1 = *((kpage_t**)((long int)mainpage + (*mainpage).numpages * PAGESIZE));
			free_page(page1);
		}
		page1 = *((kpage_t**)((long int)mainpage + (*mainpage).numpages * PAGESIZE));
		free_page(page1);
		
		//(*mainpage).numpages is -1 no
		gentryptr=0;//all pages are free
	}
	//free_page(page);
  //we need to clean the  freelist when there is nothing left
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
	*((kpage_t**)page->ptr) = page;
	int i;
	listheader=(klistheader_t*)(page->ptr);
	for(i = 0; i < 10; ++i)
	{
		((*listheader).p2fl[i]).buffer=0;
		((*listheader).p2fl[i]).size=1<<(i+4);
	}
	(*listheader).numpages=0;
	(*listheader).numalloc=0;
	(*listheader).freememory=(void*)((long int)listheader+sizeof(klistheader_t));//	the point to indicate that from this point, we have free space
	return page;
}

int chkfreespace(kma_size_t size){
	klistheader_t* mainpage=(klistheader_t*)(gentryptr->ptr);
	klistheader_t* temppage;
	int i;
	for(i=0; i<=(*mainpage).numpages; i++)
	{
		temppage = (klistheader_t*)((long int)mainpage + i * PAGESIZE);
		if((PAGESIZE+(long int)temppage-(long int)((*temppage).freememory))/(size+sizeof(kbuffer_t)))return i+1;// if there is free space left
	}
	return 0;
}

int chkfreelist(kma_size_t size){// if there is a free buffer in the free list, we just use it
	klistheader_t* mainpage=(klistheader_t*)(gentryptr->ptr);
	klistheader_t* temppage;
	int i;
	int listindex=4;
	while((1<<listindex)!=roundup(size))
	{
		listindex++;
	}
	listindex=listindex-4;
	for(i = 0; i <= (*mainpage).numpages; ++i)
	{
		temppage=(klistheader_t*)((long int)mainpage+i*PAGESIZE);
		if(((*temppage).p2fl[listindex]).buffer!=0)return i+1;
	}
	return 0;
}

void* buffermalloc(klistheader_t* page,kma_size_t size){//point to the main list
	int i=4;
	void* ret;
	klistheader_t* mainpage=(klistheader_t*)(gentryptr->ptr);
	kbuffer_t* buffer;
	kfreelist_t* freelist;
	while((1<<i)!=roundup(size))
	{
		i++;
	}
	i=i-4;
	buffer=(kbuffer_t*)((*page).freememory);
	freelist=(kfreelist_t*)&((*mainpage).p2fl[i]);
	ret=(void*)buffer+sizeof(kbuffer_t);
	(*buffer).nextbuffer=(void*)freelist;//point the list it belonging to
	(*page).freememory=(void*)buffer+size+sizeof(kbuffer_t);
	(*page).numalloc++;
	(*mainpage).numalloc++;
	return ret;
}

#endif // KMA_P2FL
