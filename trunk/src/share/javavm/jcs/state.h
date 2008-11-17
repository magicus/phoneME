/*
 * @(#)state.h	1.6 06/10/10
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

#ifndef STATEH
#define STATEH
#include <stdio.h>
#include "POINTERLIST.h"
#include "item.h"

class terminal_symbol;
class unary_symbol;
class binary_symbol;

/*
 * This file declares the following classes:
 *	state
 *	statelist
 *	statelist_iterator
 */

/*
 * A "state" represents a FSA state.
 * More pragatically, it represents a set of (partial)
 * matches that can be used to label a tree node in
 * a bottom-up walk. So a state is basically an itemset.
 * But there is a little more meaning to it than that,
 * beause a state also has a name (which is
 * an integer), and is always compared to other states
 * when created, so as to minimize creation of duplicate
 * states. Robert Henry's systems spend LOTS of energy 
 * doing this. We hope to get away with less.
 *
 * This is a state: an itemset and a numerical identifier.
 */

#define NULL_STATE 0 // state of null set.

class state{
	/*
	 * States are linked together so that the newstate function
	 * can compare to all previous states, and use one of
	 * those if necessary.
	 */
	class state * next;
	/*
	 * The primitive state allocator.
	 * Takes an itemset, and turns it into a state.
	 * If there's already a state built around this itemset,
	 * then use it. Does transitive closure, so you don't have to.
	 * In fact, your're better off not: the set "core" is used
	 * for comparisons, but the set "items" is the true contents.
	 *
	 * Assumes that the itemset has been properly massaged already.
	 */
	static state * newstate( const item_costset & ilist, const symbol *sp );
public:
	/*
	 * The important bits. An assigned state number,
	 * and an itemset.
	 */
	/*
	 * State number is assigned by newstate, BUT MAY BE REASSIGNED
	 * IF STATES ARE REORDERED OR DELETED!!. We can use the
	 * convention of number==-1 indicating a deactivated (dead) state.
	 * Thus:
	 */
	int	number;
	void	deactivate() { number = -1; }
	int	is_active() { return (number >= 0 ); }

	const symbol * symp;
	item_costset items;
	item_costset core; // smaller item set: used for comparison.

	/*
	 * A list of the rules to use to produce any
	 * symbol in the terminl_symbols list.
	 * Computed late, used in state comparison and in
	 * output. Rule 0 => can't get there from here.
	 */
	int *	rules_to_use;

	/*
	 * How states are created: the combine functions.
	 * These build states by building itemsets out of symbols
	 * and existing itemsets (found in the symbol's matchsets).
	 * Usage:
	 *	terminal_symbol *tsp;
	 *	unary_symbol    *usp;
	 *	binary_symbol   *bsp;
	 *	state	        *term_state;
	 *	state	        *unary_state;
	 *	state	        *binary_state;
	 *	state	        *empty_state;
	 *	state		*uleft_state;
	 *	state		*uleft_state;
	 *	state		*bleft_state;
	 *	state		*bright_state;
	 *	item_table	*itp;
	 *
	 *	term_state   = state::combine( tsp );
	 *	unary_state  = state::combine( usp, usp->lhs_map[uleft_state->number] );
	 *	binary_state = state::combine( bsp, bsp->lhs_map[ bleft_state->number], bsp->rhs_map[ bright_state->number ] );
	 *	empty_state   = state::null_state( itp );
	 */
	static state * combine( terminal_symbol *s );
	static state * combine( unary_symbol *s, unsigned l_stateno );
	static state * combine( binary_symbol *s, unsigned l_stateno,  unsigned r_stateno );
	static state * null_state( item_table * );

	/*
	 * Compute the rules_to_use vector.
	 */
	void use_rules();

	/*
	 * print a state. Useful only for debugging.
	 */
	void print(int print_core_set = 0, FILE * out = stdout);
};


/*
 * A list of states. Or, to be more exact, a list of
 * pointers to states.
 * Derived from the POINTERLIST class. See which for
 * commentary.
 */
DERIVED_POINTERLIST_CLASS( statelist, state *, statelist_iterator );

/*
 * a list of all states in ascending numerical order.
 */
extern class statelist allstates;
extern class state * nullstate;

/*
 * a routine to print all the states.
 * for debugging.
 * Will print only core sets unless directed.
 */
void printstates(int print_closure_sets);

/*
 * "itemset_trim"
 * Currently, this does only two operations: selftrimming,
 * and normalizing.
 *
 * Selftrim:
 * trim away items looking only at this set.
 * since trimming of duplicate matches of differing
 * costs has been done at itemset insertion, what's
 * left is to trim out full matches with the same lhs
 * but differing costs.
 *
 * Normalize:
 * At the same time, normalize the set by subtracting out
 * the cost of the cheapest match or submatch: thus all
 * normalized sets of items will be zero-based.
 * We discover which the cheapest item when trimming,
 * then normalize with a sweep.
 */

item_costset
itemset_trim( const item_costset & iin );


#endif
