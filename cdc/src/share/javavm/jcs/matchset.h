/*
 * @(#)matchset.h	1.6 06/10/10
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

#ifndef _matchset_h_
#define _matchset_h_
/* @(#)matchset.h	1.9	93/10/25 */
#include "state.h"

/*
 * Herein are defined the classes
 *	matchset
 *	unique_matchset
 *
 * A matchset is a class which holds the result of 
 * item::parent_allmatch for a symbol/{lhs,rhs}/stateno triple.
 * It is attached to the symbol, under the lhs/rhs structure (at least
 * for binaries), and is indexed by stateno. It holds loop invariant 
 * set arithmetic results.
 */

class matchset {

	int	max_state_number;

	sparse_item_costset *
		state_costset;
	
	// private function used to extend as necessary.
	void grow( int new_max );
public:
	// usual empty constructor.
	matchset()
	    { max_state_number= -1; state_costset = 0; }

	int get_max_state_number() const { return max_state_number; }

	item_costset get( int stateno ) const;

	/*
	 * cost_add: put a new item, p, in the item_costset
	 * associated with state number T, 
	 * with cost c, unless it's already in there at lower cost.
	 * Returns:
	 *	1: set was changed, either by adding a missing element,
	 *	   or adding one of lower cost.
	 *	0: nothing happened: was already in set at low cost.
	 */
	int cost_add( int t, item p, item_cost c );

	/*
	 * get rid of allocated space. Try to cut down on
	 * dangling pointers. Does not deallocate the matchset itself:
	 * you may wish to do so.
	 */
	void destruct(void);

};

class unique_matchset {

	int	max_state_number;

	item_costset *
		state_costset;
	
	// private function used to extend as necessary.
	void grow( int new_max );
public:
	// usual empty constructor.
	unique_matchset()
	    { max_state_number= -1; state_costset = 0; }

	int get_max_state_number() const { return max_state_number; }

	item_costset get( int stateno ) const;

	/*
	 * add a whole new item_costset
	 * each new one is always compared with those that preceeded.
	 * this returns as value the index of the item_costset in
	 * the matchset, whether new or old. This number should be
	 * used in building a statemap for the affected transition table.
	 * If the state was already present, it is destructed.
	 * And if a new set was added
	 * that wasn't there previously, *is_set_new is incremented.
	 */
	int add_itemset( item_costset *newset, int * is_set_new );

	/*
	 * get rid of allocated space. Try to cut down on
	 * dangling pointers. Does not deallocate the matchset itself:
	 * you may wish to do so.
	 */
	void destruct(void);
};
	

#endif
