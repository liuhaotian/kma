/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the SVR4 lazy budy
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_lzbud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_lzbud.c,v $
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
#ifdef KMA_LZBUD
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
	void* 	nextbuffer;
	int		size;
	int		local;// 	1 means  yes local, 0 means global
} kbuffer_t;

typedef struct
{
	int 		size;
	int			slack;
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
	int				numalloc;// 0 means nothing//each page hold one , if 0 then free, for allocated and local free
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
klistheader_t* divi_bud(kpageheader_t* bud_page, klistheader_t* bud_list, kma_size_t bud_size);
klistheader_t* combi_bud(void* bud_ptr, klistheader_t* bud_list, kpageheader_t* bud_page);
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
	kbuffer_t* ret;

	// if there is not enough page, we create one, the the freelist will be available
	if((i=chkfreelist(size))){
		i--;
		klistheader_t* thelist=&((*mainpage).freelist[i]);
		kpageheader_t* thepage=0;
		
		void* theaddr;
		theaddr=(void*)(((long int)(((long int)(((*mainpage).freelist[i]).buffer)-(long int)mainpage)/PAGESIZE))*PAGESIZE+(long int)mainpage);
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

		thelist = divi_bud(thepage, thelist, roundsize);
		ret = unlinkbuffer(thelist);
		
		
		fillbitmap(thepage, ret, roundsize);

		if((*ret).local == 1)
		{
			(*thelist).slack = (*thelist).slack + 2;
		}
		else{
			(*thelist).slack++;
			(*mainpage).numalloc++;
			(*thepage).numalloc++;
		}
		
		return (void*)ret;		
	}
	else{
		kpageheader_t* newpage=chkfreepage();// so we have the newpage. and it is available it freelist[9]
		klistheader_t* thelist;

		thelist=divi_bud(newpage ,&((*mainpage).freelist[9]), roundsize);
		ret=unlinkbuffer(thelist);
		fillbitmap(newpage, ret, roundsize);

		if((*ret).local == 1)// it must be global free, but we keep this structure
		{
			(*thelist).slack = (*thelist).slack + 2;
		}
		else{
			(*thelist).slack++;
			(*mainpage).numalloc++;
			(*newpage).numalloc++;
		}

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
	insertbuffer(thelist, ptr);
	(*((kbuffer_t*)ptr)).size=roundsize;
	
	klistheader_t* otherlist;
	if((*thelist).slack>=2)
	{
		(*((kbuffer_t*)ptr)).local=1;
		(*thelist).slack=(*thelist).slack-2;
		// do nothing
	}
	else if((*thelist).slack==1)
	{
		(*((kbuffer_t*)ptr)).local=0;
		emptybitmap(thepage, ptr, roundsize);
		(*thelist).slack=0;
		(*mainpage).numalloc--;
		(*thepage).numalloc--;
		otherlist=combi_bud(ptr, thelist, thepage);
		
		if((*thepage).numalloc==0){
			unlinkbufaddr(&((*mainpage).freelist[9]), (*thepage).addr);
			free_page((*thepage).ptr);
			(*thepage).ptr=0;
			(*thepage).addr=0;
			(*mainpage).numpages--;
			(*temppage).numpages--;
		}
		
		if((*temppage).numpages==0){// there is some chance to free a page
			temppage=mainpage;
			previouspage=0;
			while(temppage){
				if(((*temppage).numpages==0)&&(previouspage!=0))
				{
					(*previouspage).nextmainpage=(*temppage).nextmainpage;
					free_page((*temppage).itself);
				}
				else{
					previouspage=temppage;
				}
				temppage=(*temppage).nextmainpage;
			}
			if((*mainpage).numpages==0)
			{
				free_page((*mainpage).itself);
				mainpage=0;
			}
		}
	}
	else
	{
		(*((kbuffer_t*)ptr)).local=0;
		emptybitmap(thepage, ptr, roundsize);
		(*thelist).slack=0;
		(*mainpage).numalloc--;
		(*thepage).numalloc--;
		otherlist=combi_bud(ptr, thelist, thepage);
		
		if((*thepage).numalloc==0){
			unlinkbufaddr(&((*mainpage).freelist[9]), (*thepage).addr);
			free_page((*thepage).ptr);
			(*thepage).ptr=0;
			(*thepage).addr=0;
			(*mainpage).numpages--;
			(*temppage).numpages--;
		}

		// try to free a new buffer from locally to globally, and try to free it
		kpageheader_t* thatpage=0;
		kmainheader_t* thattemppage=mainpage;
		kmainheader_t* thatpreviouspage=0;
		klistheader_t* thatlist=thelist;
		kbuffer_t* thatptr=0;
		kbuffer_t* tempptr;
		tempptr=(kbuffer_t*)((*thatlist).buffer);
		while(tempptr){
			if((*tempptr).local==1){
				thatptr=tempptr;
			}
			tempptr=(*tempptr).nextbuffer;
		}
		
		if(thatptr==0){;}// do nothing when nothing happens
		else{// we try to free that page
			theaddr=(void*)(((long int)(((long int)thatptr-(long int)mainpage)/PAGESIZE))*PAGESIZE+(long int)mainpage);
			// find that page header
			while(!thatpage){
				for(i = 0; i < PAGENUM; ++i)
				{
					if((*thattemppage).page[i].addr==theaddr){
						thatpage=&((*thattemppage).page[i]);
						break;
					}
				}
				if((*thattemppage).nextmainpage==0)break;// it should find the page
				if(thatpage)break;
				thatpreviouspage=thattemppage;
				thattemppage=(*thattemppage).nextmainpage;
			}
			//unlinkbufaddr(thatlist, thatptr);// it is already a free block
			(*thatptr).local=0;
			(*thatlist).slack=0;
			//insertbuffer(thatlist, thatptr);
			(*thatptr).size=(*thatlist).size;
			emptybitmap(thatpage, thatptr, (*thatptr).size);
			(*mainpage).numalloc--;
			(*thatpage).numalloc--;
			combi_bud(thatptr, thatlist, thatpage);
			
			if((*thatpage).numalloc==0){
				unlinkbufaddr(&((*mainpage).freelist[9]), (*thatpage).addr);
				free_page((*thatpage).ptr);
				(*thatpage).ptr=0;
				(*thatpage).addr=0;
				(*mainpage).numpages--;
				(*thattemppage).numpages--;
			}
		}
		if(((*temppage).numpages==0)||((*thattemppage).numpages==0)){
			temppage=mainpage;
			previouspage=0;
			while(temppage){
				if(((*temppage).numpages==0)&&(previouspage!=0))
				{
					(*previouspage).nextmainpage=(*temppage).nextmainpage;
					free_page((*temppage).itself);
				}
				else{
					previouspage=temppage;
				}
				temppage=(*temppage).nextmainpage;
			}
			if((*mainpage).numpages==0)
			{
				free_page((*mainpage).itself);
				mainpage=0;
			}
		}

	}
	
/*
	(*ptr).local
	emptybitmap(thepage, ptr, roundsize);
	(*mainpage).numalloc--;
	(*thepage).numalloc--;


	klistheader_t* otherlist=combi_bud(ptr, thelist, thepage);
//	if((*otherlist).size==8192){//the page is empty
	if((*thepage).numalloc==0){
		unlinkbufaddr(otherlist, (*thepage).addr);
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
	*/
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
		(*ret).freelist[i].slack=0;
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
	kbuffer_t* tempbuffer;
	tempbuffer=(*pageheader).addr;
	(*tempbuffer).local=0;
	(*tempbuffer).size=8192;
	insertbuffer(&((*mainpage).freelist[9]), tempbuffer);
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

klistheader_t* divi_bud(kpageheader_t* bud_page, klistheader_t* bud_list, kma_size_t bud_size){
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
	
	(*tempbuffer0).local=(*tempbuffer).local;//it is the same
	(*tempbuffer0).size=(*ret).size;
	(*tempbuffer1).local=(*tempbuffer).local;
	(*tempbuffer1).size=(*ret).size;
	
	if((*tempbuffer).local == 1)
	{
		(*bud_list).slack = (*bud_list).slack + 1;//N=N-1, L=L-1
		(*ret).slack = (*ret).slack - 2;// N=N+2, L=L+2  so N-2L-G  -2
		(*bud_page).numalloc++;
	}
	else{
		//(*bud_list).slack++;
		//(*ret).slack = (*ret).slack;//no change
	}
	
	insertbuffer(ret, tempbuffer1);
	insertbuffer(ret, tempbuffer0);

	if(bud_size < (*ret).size)ret=divi_bud(bud_page, ret, bud_size);
	return ret;
}

klistheader_t* combi_bud(void* bud_ptr, klistheader_t* bud_list, kpageheader_t* bud_page){// we only accept global free blocks, so nothing changes.
	if(8192==((*bud_list).size))return bud_list;
	
	klistheader_t* ret;
	ret=(klistheader_t*)((long int)bud_list + sizeof(klistheader_t));
	unsigned char* bitmap=(*bud_page).bitmap;
	kma_size_t bud_size = (*bud_list).size;
	
	int i, offset, endbit;
	kbuffer_t* tempbuffer;
	kbuffer_t* tempbuffer0;
	kbuffer_t* tempbuffer1;
	offset=(int)(bud_ptr-(void*)((*bud_page).addr));
	if(offset%(bud_size*2)==0){// the next one is the buddy
		tempbuffer0=(kbuffer_t*)bud_ptr;
		tempbuffer1=(kbuffer_t*)(bud_ptr + bud_size);

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
	}
	else{// the pevirous one it the buddy
		tempbuffer0=(kbuffer_t*)(bud_ptr - bud_size);
		tempbuffer1=(kbuffer_t*)bud_ptr;

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
	}
	// now the buddy is free, we need to combine them
	tempbuffer1=unlinkbufaddr(bud_list,tempbuffer1);
	tempbuffer0=unlinkbufaddr(bud_list,tempbuffer0);
	tempbuffer=tempbuffer0;
	insertbuffer(ret, tempbuffer);
	
	(*tempbuffer).size=(*ret).size;

	if(bud_size < 8192)ret=combi_bud(tempbuffer, ret, bud_page);
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


#endif // KMA_LZBUD
