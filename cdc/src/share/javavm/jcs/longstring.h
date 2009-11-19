/*
 * @(#)longstring.h	1.8 06/10/10
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

#ifndef __LONGSTRINGH__
#define __LONGSTRINGH__

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

/*
 * "longstring":
 * a class to abstract (specifically from the scanner)
 * the job of collecting aribrarily long strings of C program.
 * Not actually hard, but I want it removed from the scanner so as
 * not to muck up already mucky code.
 * In plain-old-C, this could almost certainly be done
 * using a macro or two. But we have better tools now,
 * so might as well use them.
 */

class longstring {
	/*
	 * The guts of the representation:
	 * a buffer (data), its length (maxlength), and the 
	 * amount of it currently in use (length).
	 */
	unsigned	length;
	unsigned	maxlength;
	char *		data;
public:
	/*
	 * Null constructor.
	 * set the longstring to zero length, with a
	 * zero terminator.
	 */
	inline longstring(void) {
		length = 0;
		maxlength = 0;
		data = (char*)malloc(1);
		data[0] = 0;
	}

	/*
	 * distroy a longstring, returning the actual
	 * data component for use. The allocated memory
	 * is first trimmed to the length being used, to
	 * reduce the amount of garbage in the heap.
	 * No other attempt is made to collect allocated
	 * memory here. In reality, the program fragments
	 * live just about forever anyway, as they aren't
	 * output until all other calculating is done.
	 */
	inline char *
	extract(void) {
		char * t = (char*)realloc(data, length+1);
		length = 0;
		maxlength = 0;
		data = 0;
		return t;
	}

	/*
	 * append a string to the end of the string.
	 * Reallocate if necessary, in (arbitrary) increments
	 * of 80 characters. Simple but effective.
	 * (Of course, we could have overloaded 
	 * operator+=  or something to do this.)
	 */
	inline void
	add( const char *c, size_t l )
	{
		if ( length+l+1 >= maxlength ){
			data = (char*)realloc( data, maxlength += 80 );
			assert( data != 0 );
		} 
		memcpy(data + length, c, l);
		length += l;
		data[ length   ] = 0; // maintain 0 termination
	}
	/*
	 * stick a character at the end of the string.
	 * Reallocate if necessary, in (arbitrary) increments
	 * of 80 characters. Simple but effective.
	 * (Of course, we could have overloaded 
	 * operator+=  or something to do this.)
	 */
	inline void
	add( char c )
	{
	    add(&c, 1);
	}
};

#endif
