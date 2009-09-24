/*
 * @(#)wordlist.cc	1.6 06/10/10
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

// @(#)wordlist.cc       2.6     93/10/25 
#include "wordlist.h"
#include <stdio.h>

wordlist * new_wordlist( const char * p )
{
	wordlist * wp;
	wp = new wordlist;
	wp->add( p );
	return wp;
}

void
print_wordlist( wordlist *wp, FILE * out )
{
	word_iterator wi = *wp;
	const char * p;

	while ( ( p = wi.next() ) != 0 ){
		fprintf(out, "%s ", p);
	}
}

void dispose_wordlist( wordlist *wp )
{
	wp->destruct();
	delete wp;
}
