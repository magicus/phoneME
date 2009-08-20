/*
 * @(#)POINTERLIST.cc	1.6 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include "POINTERLIST.h"

#define INITIAL_ALLOCATION	2
#define POINTERSIZE		(sizeof (void*))

/*---------------------------------------------------------------------*/
/* get some more storage for this list */

void POINTERLIST::expand()
{
	if (pointer_maxn == 0){
		/* initial allocation */
		pointer_maxn = INITIAL_ALLOCATION;
		pointer_array = (void **)malloc(POINTERSIZE*pointer_maxn);
	} else {
		pointer_maxn *= 2;
		pointer_array = (void **)realloc(
			(char *)pointer_array, POINTERSIZE*pointer_maxn);
	}
	assert(pointer_array != NULL);
}

/*---------------------------------------------------------------------*/
/* return any list storage */

void POINTERLIST::destruct()
{
	if (pointer_array != NULL) free((char *)pointer_array);
	pointer_n = 0;
	pointer_maxn = 0;
	pointer_array = (void **) NULL;
}

/*---------------------------------------------------------------------*/
/* copy a list */

POINTERLIST
POINTERLIST::copy( void ) const
{
	POINTERLIST result;

	if (pointer_n == 0){
		/* copy of empty list is cheap */
		result.pointer_n = 0;
		result.pointer_maxn = 0;
		result.pointer_array = (void **) NULL;
	} else {
		/* must actually do copy */
		result.pointer_n = pointer_n;
		result.pointer_maxn = pointer_n;
		result.pointer_array = (void **)malloc(
					POINTERSIZE*pointer_n);
		assert(result.pointer_array != NULL);
		memcpy( result.pointer_array, pointer_array,
					POINTERSIZE*pointer_n );
	}
	return result;
}

/*---------------------------------------------------------------------*/
/* not very efficient subtract, but you shouldn't be using it much anyway */

void POINTERLIST::subtract( void *el )
{
	int i;

	for (i = 0; i < pointer_n; i += 1) {
		if (el == pointer_array[i]) {
			pointer_n -= 1;
			pointer_array[i] = pointer_array[pointer_n];
			break;
		}
	}
}

/*---------------------------------------------------------------------*/
/*
 * Return the first element of the list
 */

void *POINTERLIST::first( void ) const
{
	if (pointer_n <= 0) {
		return (void *) 0;
	} else {
		return pointer_array[0];
	}
}

/*---------------------------------------------------------------------*/
/*
 * Return the last element of the list
 */

void *POINTERLIST::last( void ) const
{
	if (pointer_n <= 0) {
		return (void *) 0;
	} else {
		return pointer_array[ pointer_n - 1 ];
	}
}

/*---------------------------------------------------------------------*/
/*
 * Remove the last element of the list
 */

void POINTERLIST::remove_last( void )
{
	if (pointer_n > 0) {
		pointer_n -= 1;
	}
}

/*---------------------------------------------------------------------*/
/*
 * Replace the current list element with another
 */

void POINTERLIST_ITERATOR::replace_current(void * x)
{
	list_next[-1] = x;
}

/*---------------------------------------------------------------------*/

