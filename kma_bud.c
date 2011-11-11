/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_bud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_bud.c,v $
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
#ifdef KMA_BUD
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

#define PAGENUM 91

typedef struct
{
	void* nextbuffer;
} kbuffer_t;

typedef struct
{
	int 		size;
	kbuffer_t* 	buffer;//point to the available one or nothing
} klistheader_t;

typedef struct
{
	kpage_t*		ptr;// the origin res return by get page(), we need to free it
	void*			addr;//the ptr.ptr, the start addr of the page
	unsigned char	bitmap[64];// bit map, shows that the resource 
	int				numalloc;
} kpageheader_t;

typedef struct
{
	kpage_t*		itself;
	int				numpages;//from 0 to max, the 1st one is 0
	int				numalloc;// 0 means nothing//each page hold one , if 0 then free
	klistheader_t	freelist[10];
	kpageheader_t	page[PAGENUM];
	void*	nextmainpage;
} kmainheader_t;

/************Global Variables*********************************************/

kmainheader_t* mainpage=0;

/************Function Prototypes******************************************/

kmainheader_t* initial_mainheader(kpage_t* newpage);
void initial_pageheader(kpageheader_t* pageheader, kpage_t* newpage);
kma_size_t roundup(kma_size_t size);
int chkfreelist(kma_size_t size);
kpageheader_t* chkfreepage();
klistheader_t* divi_bud(klistheader_t* bud_list, kma_size_t bud_size);
klistheader_t* combi_bud(klistheader_t* bud_list, kpageheader_t* bud_page);
void insertbuffer(klistheader_t* thefreelist, kbuffer_t* thebuffer);
kbuffer_t* unlinkbuffer(klistheader_t* thefreelist);
kbuffer_t* unlinkbufaddr(klistheader_t* thefreelist, kbuffer_t* thebufaddr);
void fillbitmap(kpageheader_t* pageheader, void* bufferptr, kma_size_t roundsize);
void emptybitmap(kpageheader_t* pageheader, void* bufferptr, kma_size_t roundsize);
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if ((size + sizeof(void*)) > PAGESIZE){ // requested size too large
		return NULL;
	}
	if(!mainpage){// initialized the entry
		mainpage=initial_mainheader(get_page());
	}
	
	int roundsize=roundup(size);
	int i;
	void* ret;

	// if there is not enough page, we create one, the the freelist will be available
	
	if((i=chkfreelist(size))){
		i--;
		klistheader_t* thelist;
		kpageheader_t* thepage=0;
		
		thelist = divi_bud(&((*mainpage).freelist[i]), roundsize);
		ret = unlinkbuffer(thelist);
		
		void* theaddr;
		theaddr=(void*)(((long int)(((long int)ret-(long int)mainpage)/PAGESIZE))*PAGESIZE+(long int)mainpage);
		kmainheader_t* temppage=mainpage;
		// find the page header
		while(!thepage){
			for(i = 0; i < PAGENUM; ++i)
			{
				if((*temppage).page[i].addr==theaddr){
					thepage=&((*temppage).page[i]);
					break;
				}
			}
			if((*temppage).nextmainpage==0)break;// it should find the page
			if(thepage)break;
			temppage=(*temppage).nextmainpage;
		}
		fillbitmap(thepage, ret, roundsize);

		(*mainpage).numalloc++;
		(*thepage).numalloc++;
		return (void*)ret;		
	}
	else{
		kpageheader_t* newpage=chkfreepage();// so we have the newpage. and it is available it freelist[9]
		klistheader_t* thelist;
		
		thelist=divi_bud(&((*mainpage).freelist[9]), roundsize);
		ret=unlinkbuffer(thelist);
		fillbitmap(newpage, ret, roundsize);

		(*mainpage).numalloc++;
		(*newpage).numalloc++;
		return (void*)ret;
	}
  return NULL;
}

void 
kma_free(void* ptr, kma_size_t size)
{
	int roundsize=roundup(size);
	
	// find its page and its header
	kpageheader_t* thepage=0;
	klistheader_t* thelist=0;
	void* theaddr;
	int i;
	theaddr=(void*)(((long int)(((long int)ptr-(long int)mainpage)/PAGESIZE))*PAGESIZE+(long int)mainpage);
	kmainheader_t* temppage=mainpage;
	kmainheader_t* previouspage=0;
	// find the page header
	while(!thepage){
		for(i = 0; i < PAGENUM; ++i)
		{
			if((*temppage).page[i].addr==theaddr){
				thepage=&((*temppage).page[i]);
				break;
			}
		}
		if((*temppage).nextmainpage==0)break;// it should find the page
		if(thepage)break;
		previouspage=temppage;
		temppage=(*temppage).nextmainpage;
	}
	// find the header
	for(i = 0; i < 10; ++i)
	{
		if((*mainpage).freelist[i].size==roundsize){
			break;
		}
	}
	thelist=&((*mainpage).freelist[i]);
	insertbuffer(thelist,ptr);
	emptybitmap(thepage, ptr, roundsize);
	(*mainpage).numalloc--;
	(*thepage).numalloc--;
	
	
	klistheader_t* otherlist=combi_bud(thelist, thepage);
//	if((*otherlist).size==8192){//the page is empty
	if((*thepage).numalloc==0){
		unlinkbuffer(otherlist);
		free_page((*thepage).ptr);
		(*thepage).ptr=0;
		(*thepage).addr=0;
		(*mainpage).numpages--;
		(*temppage).numpages--;
	}
	if(((*temppage).numpages==0)&&(previouspage!=0))
	{
		(*previouspage).nextmainpage=(*temppage).nextmainpage;
		free_page((*temppage).itself);
	}
	if((*mainpage).numpages==0)
	{
		free_page((*mainpage).itself);
		mainpage=0;
	}
	
	
	/*
	free it
	change bitmap
	according to the bitmap, combain the buddies
	then delete the link from free list
	add the new addr to free list
	current numalloc--
	global numalloc--
	
	if all global numalloc is 0{
		free the pages from the last one to the first one
		
	}
	else if current page empty{
		if the last one{
			free the current page
			numpages--
		}
		else{
			initial it
		}
	}
	*/
  ;
}

kmainheader_t* initial_mainheader(kpage_t* newpage){
	kmainheader_t* ret;
	
	ret=(kmainheader_t*)(newpage->ptr);
	(*ret).itself=newpage;
	(*ret).numpages=0;
	(*ret).numalloc=0;
	(*ret).nextmainpage=0;
	int i;
	for(i = 0; i < 10; ++i)
	{
		(*ret).freelist[i].size=16<<i;
		(*ret).freelist[i].buffer=0;
	}
	for(i = 0; i < PAGENUM; ++i)
	{
		(*ret).page[i].ptr=0;
		(*ret).page[i].addr=0;
		//initial_pageheader(&((*ret).page[i]));
	}
	return ret;
}

void initial_pageheader(kpageheader_t* pageheader, kpage_t* newpage){
	int j;
	(*pageheader).ptr=newpage;
	(*pageheader).addr=(void*)(newpage->ptr);
	(*pageheader).numalloc=0;
	for(j = 0; j < 64; ++j)
	{
		(*pageheader).bitmap[j]=0;
	}
	// add the whole page to free list
	*((kbuffer_t**)((*pageheader).addr))=0;
	insertbuffer(&((*mainpage).freelist[9]), (kbuffer_t*)((*pageheader).addr));
/*	kbuffer_t* tempbuffer;
	tempbuffer=(*mainpage).freelist[9].buffer;
	(*mainpage).freelist[9].buffer=(kbuffer_t*)((*pageheader).addr);
	*((kbuffer_t*)((*pageheader).addr))=tempbuffer;
*/
}

kma_size_t roundup(kma_size_t size){
	kma_size_t ret=16;
	while(size>ret){
		ret=ret<<1;
	}
	return ret;
}

int chkfreelist(kma_size_t size){

	int i;
	int roundsize=roundup(size);
	for(i = 0; i < 10; ++i)
	{
		if((*mainpage).freelist[i].size>=roundsize){
			if((*mainpage).freelist[i].buffer!=0)return i+1;
		}
	}
	return 0;
}

kpageheader_t* chkfreepage(){
	kpageheader_t* ret=0;
	kmainheader_t* temppage=mainpage;
	int i;
	
	// find the available page header
	while(!ret){
		for(i = 0; i < PAGENUM; ++i)
		{
			if((*temppage).page[i].ptr==0){
				ret=&((*temppage).page[i]);
				break;
			}
		}
		if((*temppage).nextmainpage==0)break;
		if(ret)break;
		temppage=(*temppage).nextmainpage;
	}
	
	if(ret!=0)
	{
		initial_pageheader(ret, get_page());
		(*mainpage).numpages++;
		(*temppage).numpages++;
	}
	else{
		(*temppage).nextmainpage=initial_mainheader(get_page());
		temppage=(*temppage).nextmainpage;
		ret=&((*temppage).page[0]);
		initial_pageheader(ret, get_page());
		(*mainpage).numpages++;
		(*temppage).numpages++;
	}
	
	return ret;
}

klistheader_t* divi_bud(klistheader_t* bud_list, kma_size_t bud_size){
	if(bud_size==((*bud_list).size))return bud_list;
	
	klistheader_t* ret;
	ret=(klistheader_t*)((long int)bud_list-sizeof(klistheader_t));
	
	kbuffer_t* tempbuffer;
	kbuffer_t* tempbuffer0;
	kbuffer_t* tempbuffer1;	
	// dealing with the large block free list
	tempbuffer=unlinkbuffer(bud_list);
	// dealing with the small block free list
	tempbuffer0=tempbuffer;
	tempbuffer1=(kbuffer_t*)((long int)tempbuffer0 + (*ret).size);
	insertbuffer(ret, tempbuffer1);
	insertbuffer(ret, tempbuffer0);

	if(bud_size < (*ret).size)ret=divi_bud(ret, bud_size);
	return ret;
}

klistheader_t* combi_bud(klistheader_t* bud_list, kpageheader_t* bud_page){
	if(8192==((*bud_list).size))return bud_list;
	
	klistheader_t* ret;
	ret=(klistheader_t*)((long int)bud_list + sizeof(klistheader_t));
	unsigned char* bitmap=(*bud_page).bitmap;
	kma_size_t bud_size = (*bud_list).size;
	
	int i, offset, endbit;
	kbuffer_t* tempbuffer;
	kbuffer_t* tempbuffer0;
	kbuffer_t* tempbuffer1;
	offset=(int)((void*)((*bud_list).buffer)-(void*)((*bud_page).addr));
	
	if(offset%(bud_size*2)==0){// the next one is the buddy
		tempbuffer0=(*bud_list).buffer;
		tempbuffer1=(kbuffer_t*)((void*)((*bud_list).buffer) + bud_size);
		
		offset += bud_size;
		endbit = offset + bud_size;
		offset /= 16;
		endbit /= 16;

		for( i = offset; i < endbit; ++i)
		{
			if(bitmap[i/8] & (1<<(i%8))){
				return bud_list;//it is not free
			}
		}
		tempbuffer1=unlinkbufaddr(bud_list,tempbuffer1);
		tempbuffer0=unlinkbuffer(bud_list);
	}
	else{// the pevirous one it the buddy
		tempbuffer0=(kbuffer_t*)((void*)((*bud_list).buffer) - bud_size);
		tempbuffer1=(*bud_list).buffer;
		
		offset -= bud_size;
		endbit = offset + bud_size;
		offset /= 16;
		endbit /= 16;

		for( i = offset; i < endbit; ++i)
		{
			if(bitmap[i/8] & (1<<(i%8))){
				return bud_list;//it is not free
			}
		}
		tempbuffer0=unlinkbufaddr(bud_list,tempbuffer0);
		tempbuffer1=unlinkbuffer(bud_list);
	}
	// now the buddy is free, we need to combine them
	
	tempbuffer=tempbuffer0;
	insertbuffer(ret, tempbuffer0);

	if(bud_size < 8192)ret=combi_bud(ret, bud_page);
	return ret;
}

void insertbuffer(klistheader_t* thefreelist, kbuffer_t* thebuffer){// insert the free buffer to the list it belonging to
	kbuffer_t* temp;
	temp=(*thefreelist).buffer;
	(*thefreelist).buffer=thebuffer;
	(*thebuffer).nextbuffer=temp;
}

kbuffer_t* unlinkbuffer(klistheader_t* thefreelist){// just unlink the very first free buffer in the free list
	kbuffer_t* ret;
	ret=(*thefreelist).buffer;
	(*thefreelist).buffer=(*ret).nextbuffer;
	return (kbuffer_t*)ret;
}

kbuffer_t* unlinkbufaddr(klistheader_t* thefreelist, kbuffer_t* thebufaddr){// unlink the buffer for a certain address
	kbuffer_t* ret = 0;
	void* thenextbuffer;
	thenextbuffer=(*thefreelist).buffer;
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

void fillbitmap(kpageheader_t* pageheader, void* bufferptr, kma_size_t roundsize){
	unsigned char* bitmap=(*pageheader).bitmap;
	int i, offset, endbit;
	offset=(int)(bufferptr-(*pageheader).addr);
	endbit = offset + roundsize;
	offset /= 16;
	endbit /= 16;
	
	for( i = offset; i < endbit; ++i)
	{
		bitmap[i/8] |= (1<<(i%8));
	}
}

void emptybitmap(kpageheader_t* pageheader, void* bufferptr, kma_size_t roundsize){
	unsigned char* bitmap=(*pageheader).bitmap;
	int i, offset, endbit;
	offset=(int)(bufferptr-(*pageheader).addr);
	endbit = offset + roundsize;
	offset /= 16;
	endbit /= 16;
	for( i = offset; i < endbit; ++i)
	{
		bitmap[i/8] &= (~(1<<(i%8)));
	}
}

#endif // KMA_BUD
