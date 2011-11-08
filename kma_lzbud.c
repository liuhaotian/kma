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
	void* 	prevbuffer;
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
	int				numalloc;// 0 means nothing//each page hold one , if 0 then free
	klistheader_t	freelist[10];
	kpageheader_t	page[PAGENUM];
	void*	nextmainpage;
} kmainheader_t;

/************Global Variables*********************************************/

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	
  return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_LZBUD
