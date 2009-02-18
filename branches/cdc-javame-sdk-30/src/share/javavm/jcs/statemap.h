/*
 * @(#)statemap.h	1.6 06/10/10
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

#ifndef __statemap_h_
#define __statemap_h_
#include <stdio.h>
#include <assert.h>
#include "transition.h"
/*
 * statemap: an expandable vector for mapping a large set of
 * states into a smaller one. Used for transition matrix compression.
 * i.e. maps statenumber => statenumber.
 */

class statemap {
	int		max_element;
	stateno		maxv;
	stateno  *	array;

public:
	// null constructor.
	statemap(){ max_element = -1; array = 0; maxv=0; }
	
	// make map bigger as number of states grows.
	void grow( stateno new_max );

	// add a mapping
	void new_mapping( stateno index, stateno value );

	// find max abcissa value
	stateno max_mapping(void) { return maxv; }

	// find max ordinate value
	stateno max_ordinate(void) { return max_element; }

	// do lookup.
	stateno operator[]( stateno index );

	// squeeze map. When rows or columns are
	// removed from a transition table, the map
	// has to change, too.
	// Remap(m, n) causes all map entries currently referencing
	// m to reference n instead.
	void remap( stateno oldno, stateno tono );

	//
	// free allocated storage. Does not deallocate the
	// statemap itself.
	void destruct(void);

	void print(
	    const char * type_name,
	    const char * symbolname,
	    const char * qualifier,
	    const char * name_template,
	    FILE * output,
	    FILE * data);
};

// for debugger.
void print_statemap( statemap * );

#endif
