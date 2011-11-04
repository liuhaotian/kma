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

typedef struct
{
	void* list_or_next;
} kbufferheader_t;

typedef struct
{
	kbufferheader_t* buffer;
} klistheader_t;

typedef struct
{
	int map[16];
} kbitmap_t;

typedef struct
{
	void* itself;
	int numpages;//from 0 to max, the 1st one is 0
	int numalloc;// 0 means nothing//each page hold one , if 0 then free
	kbitmap_t bitmap;
	klistheader_t bud[10];
	void* nothing[2];
} kpageheader_t;

/************Global Variables*********************************************/

kpageheader_t* mainpage;

/************Function Prototypes******************************************/
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if too large return NULL
	
	initial the entry only once
	
	check the free list for allocation
	if true {
		find the roundsize
		if true{
			allocate
			current numalloc++
			global numalloc++
			change bitmap
			return
		}
		else{
			divide the large block
			allocate
			current numalloc++
			global numalloc++
			change bitmap
			link the new small block to the free list
			
		}	
	}
	else{
		create new page
		initial the page
		allocate
		current numalloc++
		global numalloc++
		change bitmap
		link the new small block to the current free list
	}
	
  return NULL;
}

void 
kma_free(void* ptr, kma_size_t size)
{
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
  ;
}

#endif // KMA_BUD
