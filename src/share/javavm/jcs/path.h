/*
 * @(#)path.h	1.8 06/10/10
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

#ifndef _path_h__
#define _path_h__
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This file declares the following classes:
 *	path
 * A path is part of an pattern. It describes the series of moves
 * down from the root to a subtree.
 * A path plus a rule make a pattern.
 */


/*
 * A "path" is used for navigating in a binary tree.
 * It is used as part of an item, and names the subtree match
 * to which the item corresponds. (See below)
 * Think of a path as a string of L's and R's. Thus
 * the path "LLR" corresponds to a subtree that, starting
 * from the tree's root, can be found by descending via the
 * left link, then another left link, then a right link.
 * A zero-length path is allowed.
 * Path comparison is used when comparing items, and
 * in defining an item collating sequence.
 */
class path {
	int path_length;
	int path_bits;
public:
	enum path_segment { left = 0, right = 1, XX = 2 };

	/* 
	 * Dynamically set a variable of type path to zero.
	 * Usage:
	 *	path pth;
	 *	pth.zero();
	 * often causes a compiler complaint about
	 * use before set. Ignore it.
	 */
	inline void zero(){
		path_length = 0;
		path_bits = 0;
	}
	/*
	 * Initialize a path variable to zero.
	 */
	inline path(void){
		path_length = 0;
		path_bits = 0;
	}

	/*
	 * Path comparison operators.
	 */
	inline int operator==( const struct path &other ) const
		{ return (path_length == other.path_length)
		      && (path_bits == other.path_bits);
		}
	inline int operator>( const struct path &other ) const
		{
			return (path_length == other.path_length)
				?  ( path_bits > other.path_bits)
				: (path_length > other.path_length);
		}
	inline int operator<( const struct path &other ) const
		{
			return (path_length == other.path_length)
				?  ( path_bits < other.path_bits)
				: (path_length < other.path_length);
		}
	inline int operator>=( const struct path &other ) const
		{
			return (path_length == other.path_length)
				?  ( path_bits >= other.path_bits)
				: (path_length >= other.path_length);
		}

	/*
	 * Append a new path component (path::left or path::right) to
	 * the end of an existing path.
	 * Usage:
	 *	path pth;
	 *	pth.descend( path::left );
	 */
	inline void descend( path_segment s )
		{
			assert(path_length<32);
			/*
			 * Cfront does not handle auto-increment
			 * expressions correctly in inlined member
			 * functions.  This code used to read
			 * 	path_bits |= s << (path_length++);
			 * but was changed to work around the bug.
			 *
			 * 1e+4932 on the vomit meter.
			 */
			path_bits |= s << (path_length);
			path_length += 1;
		}
	/*
	 * Delete the last component from a path. This makes the
	 * path shorter.
	 * Usage:
	 *	path pth;
	 *	pth.ascend();
	 */
	inline void ascend()
		{	assert(path_length > 0);
			path_length -= 1;
			path_bits &= (1<<path_length)-1;
		}
	/*
	 * Find out the last component of a path, either path::left or
	 * path::right. This is important in the {left|right}parent_allmatch
	 * routines.
	 * Usage:
	 *	path pth;
	 *	if ( pth.last_move() == path::right ) ...
	 */
	inline path_segment last_move(void) const
		{
			return (enum path_segment)((path_length>0)
					? ((path_bits>>(path_length-1))&1)
					: XX);
		}

	/*
	 * Find out if a path has length zero.
	 * This used to be done by comparing last_move() to path::XX
	 * Usage:
	 *	path pth;
	 *	if (pth.empty_path() ) ...
	 */
	inline int empty_path(void) const { return path_length <= 0; }

	/* 
	 * print a path. The form used here is only interesting for
	 * debugging purposes.
	 * Usage:
	 *	path pth;
	 *	pth.print();
	 */
	void print(FILE * out = stdout)const ;

	/*
	 * allow this routine to look directly at the representation
	 * of a path. This is a performance optimization only.
	 */
	friend struct ruletree* follow_path( class path &p, struct ruletree *rtp );
};

#endif
