/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_rm.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_rm.c,v $
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
#ifdef KMA_RM
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
	int size;
	void* nextbuffer;
	void* prevbuffer;
	void* whichpage;
} kfreelist_t;

typedef struct
{
	void* itself;
	int numpages; // from 0 to max, 1st page is 0
	int numalloc; // if 0 then we can free page, number of allocated blocks per page
	kfreelist_t* header; // pointer to header of free list
} klistheader_t;

	
/************Global Variables*********************************************/

kpage_t* gentryptr=0; // our global entry pointer

/************Function Prototypes******************************************/

kpage_t* initial(kpage_t* page, int first); // function to initialize page
void* findfit(int size); // returns pointer to free space based on our list, and fixes the list
void add(void* ptr, int size); // adds pointer to free space based on our list
void resolve(void); // resolves list, looking for pages being used with no allocs and freeing them
void remove(void* ptr); // removes pointer from list

/************External Declaration*****************************************/

/**************Implementation***********************************************/

/* The goal:
	To include a linked list data structure that will be maintained in the beginning of each free block. The header pointer will be at the beginning of the first page. The structure consists of free block <base, pairs> and we use first-fit as the algorithm for searching our list for a fit. */

void*
kma_malloc(kma_size_t size)
{
	if ((size + sizeof(void*)) > PAGESIZE) // requested size too large
		return NULL;

	// initializations for malloc stack vars
	void* ret;
	klistheader_t* mainpage;

	if (!gentryptr) // initialize the first page if we have no entry pointer and set it to it
		initial(get_page(), 1);

	mainpage = (klistheader_t*)(gentryptr->ptr); // set our mainpage listheader struct before looking for first fit

	// Now we call our findfit function that will search list and return ptr to freelist struct that fits, if not in list it will also make a new page and add a new freelist struct and return that. In addition, it will do some tricks to save some space
	ret = findfit(size);

	//mainpage->numalloc++;
	
	klistheader_t* thispage;

	thispage=(*(kfreelist_t*)ret).whichpage;
	((*thispage).numalloc)++; // increase number allocated on the page we added
	return ret;
	
}

void
kma_free(void* ptr, kma_size_t size)
{
	// decrease number allocated
	//(((klistheader_t*)(gentryptr->ptr))->numalloc)--;
	
	// function to add ptr with size to list
	add(ptr, size);
	(((klistheader_t*)(((kfreelist_t*)ptr)->whichpage))->numalloc)--;
	
	// function to check pages and free/release a page if it has no mallocs
	resolve();
}

kpage_t* initial(kpage_t* page, int first)
{
	klistheader_t* listheader;
	*((kpage_t**)page->ptr) = page;

	listheader=(klistheader_t*)(page->ptr); // set our listheader to the base of the page

	/*kfreelist_t* firstresource; // initialize the first resource
        firstresource->size = (PAGESIZE - sizeof(klistheader_t)); // how much space we will have
	firstresource->nextbuffer = NULL; // end of our free list for now
	firstresource->prevbuffer = NULL; // beginning of free list */

	// Add first free resource to list
	listheader->header = (kfreelist_t*)((long int)listheader + sizeof(klistheader_t)); // set our header to right after the initial data struct

	if (first)
	{
	  gentryptr = page;
	}
	
	// Add new resource which is the full page to the linked list
	add( ((void*)(listheader->header)), (PAGESIZE - sizeof(klistheader_t)));

	(*listheader).numpages = 0;
	(*listheader).numalloc = 0;
	
	return page;
}

void* findfit(int size)
{
	if (size < sizeof(kfreelist_t))
		size = sizeof(kfreelist_t); // minimum size allowed for free resource
  klistheader_t* mainpage;
  mainpage = (klistheader_t*)(gentryptr->ptr);
  kfreelist_t* temp = ((kfreelist_t*)(mainpage->header));
	int blocksize;
  
  while (temp != NULL)
  {
		blocksize = temp->size;
    if (blocksize >= size) // found our fit
    {
      if (blocksize == size || (blocksize - size) < sizeof(kfreelist_t) ) // perfect fit or not enough space to perserve, remove whole thing from free resource list
      {
				remove(temp);
				return ((void*)temp);
      }

      // else, we have to save some free space and make a new entry in our list
			add((void*)((long int)temp + size), (blocksize - size));
			remove(temp);
      return ((void*)temp);  
    }
    temp = temp->nextbuffer; // continue on with list loop
  }
  
  // didn't find a fit, allocate a new page, call initial to initialize it and add it to free list, return it
  // then, return the new ptr after sectioning off additional space
  //kpage_t* newpage;
  //newpage = initial(get_page(), 0);
	initial(get_page(), 0);
	// update numpages on main page
	mainpage->numpages++;
	
	return findfit(size);
  
  // remove the new page's initial resource from free list since initial made it
  //remove( ((kfreelist_t*)((newpage->ptr) + sizeof(klistheader_t))) );
  //add( ( ((kfreelist_t*)((newpage->ptr) + sizeof(klistheader_t))) + size), ( (((kfreelist_t*)((newpage->ptr) + sizeof(klistheader_t)))->size) - size ) ); 
  //return ((kfreelist_t*)((newpage->ptr) + sizeof(klistheader_t)));  
}	

void add(void* ptr, int size) // adds pointer to free space based on our list
{
  klistheader_t* mainpage;
  mainpage = (klistheader_t*)(gentryptr->ptr);
	void* temp = (void*)(mainpage->header);
	
	klistheader_t* temppage;
	kfreelist_t*	thisptr;
	long int i = ((long int)ptr - (long int)mainpage) / PAGESIZE; // how far our page is by number of pages
	temppage = (klistheader_t*)((long int)mainpage + i * PAGESIZE); // the page we are adding a free resource to
	thisptr = (kfreelist_t*)ptr;
	(*thisptr).whichpage = temppage;

	((kfreelist_t*)ptr)->size = size;
	((kfreelist_t*)ptr)->prevbuffer = NULL;
	
	// case 1: adding the first one
	if (temp == ptr)
	{
		((kfreelist_t*)ptr)->nextbuffer = NULL;
		return;
	}

	/* Add to beginning of list, bad algorithm leads to page limit problems
	// case 2: adding any others
  ((kfreelist_t*)(mainpage->header))->prevbuffer = (kfreelist_t*)ptr; // change previous header's prevbuffer to new resource 
  
  // make the new header the ptr
  ((kfreelist_t*)ptr)->nextbuffer = ((kfreelist_t*)(mainpage->header));
  mainpage->header = (kfreelist_t*)ptr; */


	// Add to the free list in order of increasing ptr, so we will always start our search from front contiguously
	// case 2: adding any others
	if (temp > ptr) // if our new one comes before our header contiguously
	{
		((kfreelist_t*)(mainpage->header))->prevbuffer = (kfreelist_t*)ptr; // change prev header's prevbuffer to new
		
		// make the new header the ptr
		((kfreelist_t*)ptr)->nextbuffer = ((kfreelist_t*)(mainpage->header));
		mainpage->header = (kfreelist_t*)ptr;

		return;
	}

	// else we need to find where to add by searching list and inserting between
	kfreelist_t* tempnext;
	while (((kfreelist_t*)temp)->nextbuffer)
	{
		if (temp > ptr)
			break; // we found it, where ours needs to be inserted after

		temp = ((void*)(((kfreelist_t*)temp)->nextbuffer));
	}
	tempnext = ((kfreelist_t*)temp)->nextbuffer;

	((kfreelist_t*)temp)->nextbuffer = ptr;
	((kfreelist_t*)ptr)->nextbuffer = tempnext;
	((kfreelist_t*)ptr)->prevbuffer = temp;
	if (tempnext)
		tempnext->prevbuffer = ptr;
	
}

void resolve(void) // resolves list, looking for pages being used with no allocs and freeing them
{
	/* SIMPLE ALGORITHM PLUS EVEN IF IT DID TEST 5 WILL FAIL TOO MANY ALLOC'D PAGES
	klistheader_t* mainpage = (klistheader_t*)(gentryptr->ptr);
	kpage_t* temppage;
	
	//kpage_t* emptypage;

	if (mainpage->numalloc == 0)
	{
		for (; mainpage->numpages > 0; (mainpage->numpages)--) // free all pages and remove all free resources
		{
			temppage = ((*(kpage_t**)((long int)mainpage + mainpage->numpages * PAGESIZE)));

			//emptypage = ((kpage_t*)((long int)mainpage + mainpage->numpages * PAGESIZE));
			free_page(temppage);
		}
		temppage = ((*(kpage_t**)((long int)mainpage + mainpage->numpages * PAGESIZE)));
		gentryptr = 0;
		free_page(temppage);
	} */

	
	
	// THIS NEEDS WORK...BETTER ALGORITHM FOR DYNAMICALLY FREEING PAGES BUT DOES NOT WORK, SEGFAULTS
	klistheader_t* mainpage;
	mainpage = (klistheader_t*)(gentryptr->ptr);
	
	klistheader_t* temppage;
	int i;
	i = mainpage->numpages; // find the max page and go there first
	int cont = 1;
	do 
	{
		cont = 0;
		temppage = (((klistheader_t*)((long int)mainpage + i * PAGESIZE)));
		// if that page has no malloc's, then first loop through free list and remove each one with page ptr, then free the page
		kfreelist_t* temp = mainpage->header;
		kfreelist_t* temp2;
		if ( ((klistheader_t*)temppage)->numalloc == 0) // free this page!
		{
			while (temp != NULL)
			{
				temp2 = temp->nextbuffer;
				if (temp->whichpage == temppage)
					remove(temp);
				temp = temp2;
			}
			cont = 1; //could be more free pages in descending order from end, let it continue
			if (temppage == mainpage)
			{
				gentryptr = 0;
				cont = 0;
			}

			free_page((*temppage).itself);
			if (gentryptr)
				(mainpage->numpages)--;
			i--;
		}
	} while(cont);
}

void remove(void* ptr) // removes pointer from list
{
  //4 cases, removing head of list with more, head of list by itself, tail of list, or anywhere else

	kfreelist_t* temp = (kfreelist_t*)ptr;
	kfreelist_t* tempnext = temp->nextbuffer;
	kfreelist_t* tempprev = temp->prevbuffer;

  // case 1: head by itself
  if ( tempnext == NULL && tempprev == NULL)
  {
		klistheader_t* mainpage = (klistheader_t*)(gentryptr->ptr);
		mainpage->header = NULL;
    gentryptr = 0;
    return;
  }
    
  // case 2: head with more
  if ( tempprev == NULL)
  {
    klistheader_t* mainpage = (klistheader_t*)(gentryptr->ptr);
    tempnext->prevbuffer = NULL;
		// and set new header
		mainpage->header = tempnext;
    return;
  }
  
  // case 3: tail
  if ( tempnext == NULL)
  {
    tempprev->nextbuffer = NULL;
    return;
  }
  
  // case 4: anywhere else
  tempnext->prevbuffer = tempprev;
	tempprev->nextbuffer = tempnext;
  return;
}

#endif // KMA_RM
