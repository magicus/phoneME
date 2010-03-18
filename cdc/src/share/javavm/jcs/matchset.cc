/*
 * @(#)matchset.cc	1.6 06/10/10
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

/* @(#)matchset.cc	1.10	98/07/03 */
#include "path.h"
#include "rule.h"
#include "matchset.h"
#include <stdlib.h>

/*
 * Herein is defined the class
 *	matchset
 *	unique_matchset
 *
 * A matchset is a class which holds the result of 
 * item::parent_allmatch for a symbol/{lhs,rhs}/stateno triple.
 * It is attached to the symbol, under the lhs/rhs structure (at least
 * for binaries), and is indexed by stateno. It holds loop invariant 
 * set arithmetic results.
 */

void 
unique_matchset::grow( int new_max )
{
	int n;
	item_costset empty(&all_items);
	n = (new_max+1)*sizeof(item_costset);
	if ( state_costset == 0 )
		state_costset = (item_costset *)malloc( n );
	else
		state_costset = (item_costset *)realloc( (char*)state_costset,  n );
	assert( state_costset != 0 );
	for ( n = max_state_number+1; n <= new_max; n++ )
		state_costset[n] = empty;
	max_state_number = new_max;
}


void 
matchset::grow( int new_max )
{
	int n;
	sparse_item_costset empty(&all_items);
	n = (new_max+1)*sizeof(sparse_item_costset);
	if ( state_costset == 0 )
		state_costset = (sparse_item_costset *)malloc( n );
	else
		state_costset = (sparse_item_costset *)realloc( (char*)state_costset,  n );
	assert( state_costset != 0 );
	for ( n = max_state_number+1; n <= new_max; n++ )
		state_costset[n] = empty;
	max_state_number = new_max;
}

item_costset
unique_matchset::get( int stateno ) const
{
	if ( stateno > max_state_number )
		return item_costset(&all_items);
	else
		return state_costset[ stateno ];
}

void
unique_matchset::destruct(void)
{
	int i;
	if ( state_costset != 0 ){
		for ( i = 0; i <= max_state_number; i++ )
			state_costset[i].destruct();
		free( (char*)state_costset );
		state_costset = 0;
		max_state_number = -1;
	}
}

item_costset
matchset::get( int stateno ) const
{
	if ( stateno > max_state_number )
		return item_costset(&all_items);
	else
		return state_costset[ stateno ].get();
}

void
matchset::destruct(void)
{
	int i;
	if ( state_costset != 0 ){
		for ( i = 0; i <= max_state_number; i++ )
			state_costset[i].destruct();
		free( (char*)state_costset );
		state_costset = 0;
		max_state_number = -1;
	}
}

int
matchset::cost_add( int t, item p, item_cost c )
{
	if ( t > max_state_number )
		grow( t );
	return state_costset[t].cost_add(p, c );
}

/*
 * add a whole new item_costset
 * each new one is always compared with those that preceeded.
 * this returns as value the index of the item_costset in
 * the matchset, whether new or old. This number should be
 * used in building a statemap for the affected transition table.
 * And if a new set was added
 * that wasn't there previously, *is_set_new is incremented.
 */
int
unique_matchset::add_itemset( item_costset *newset, int * is_set_new )
{
	/*
	 * second implememtation is pretty trivial:
	 * (Normalize first.) Then compare to  all preceeding, and add
	 * if not already there. 
	 * Remember, all will be normalized.
	 */
	int i;

	newset->normalize();
	for ( i = 0; i <= max_state_number; i++ ){
		if ( item_costset_equal( state_costset[i], *newset ) ){
			newset->destruct();
			return i;
		}
	}
	// wasn't there. add at end.
	grow(i);
	state_costset[i] = *newset;
	*is_set_new += 1;
	return i;
}
