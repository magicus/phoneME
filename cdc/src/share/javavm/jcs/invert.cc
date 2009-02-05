/*
 * @(#)invert.cc	1.7 06/10/10
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

#include <stdio.h>
#include <stdlib.h>
#include "rule.h"
#include "state.h"
#include "wordlist.h"
#include "symbol.h"
#include "transition.h"

// @(#)invert.cc	1.8	93/10/25

static void invert_substate( const state * sp, wordlist * wlp, int depth );
static state ** state_array = 0;
static int n_state = 0;
static int MAXDEPTH = 6;
/*
 * Invert state transition tables
 * to produce a wordlist representation of a tree
 * having the given state labeling its top node.
 * Prefer leaf nodes to operators whenever possible.
 */

static int
is_terminal_state( stateno sno )
{
	return ( state_array[sno]->symp->type() == symbol::terminal );
}

static void
init_state_array()
{
	int cur_n_states = allstates.n();
	statelist_iterator sli = allstates;
	state * sp;
	if ( state_array != 0 ) free( (char *)state_array );
	state_array = (state **) malloc( cur_n_states * sizeof (state*) );
	while( ( sp = sli.next() ) != 0 ){
		if ( sp->is_active() )
			state_array[ sp->number ] = sp;
	}
	n_state = cur_n_states;
}

/* not static */
void
delete_inversion_info()
{
	// could have been called "clear_state_array".
	// delete cached info, so that state renumbering doesn't goof
	// us up too much.
	if ( state_array != 0 ){
		free( (char *)state_array );
		state_array = 0;
	}
	n_state = 0;
}


wordlist
invert_state( const state * sp )
{
	wordlist w;
	if (n_state < allstates.n() )
		init_state_array();
	invert_substate( sp, & w, 0 );
	return w;
}

static void
invert_binary_transition(
	binary_symbol * bsyp,
	register stateno target,
	stateno * lp,
	stateno * rp )
{
	register binary_transition * bsp = bsyp->transitions;
	register stateno *row;
	register statemap * lmap = &bsyp->lhs_map;
	register statemap * rmap = &bsyp->rhs_map;
	stateno l_save =0;
	stateno r_save =0;
	stateno a, b, amax, bmax;
	stateno l, r, lmax, rmax;
	int l_is_terminal, r_is_terminal;

	amax = bsp->row_max_stateno();
	bmax = bsp->col_max_stateno();
	lmax = lmap->max_ordinate();
	rmax = rmap->max_ordinate();
	for( a = 0; a <= amax ; a++ ){
		row = bsp->state[a].state;
		for ( b = 0; b <= bmax ; b++ ){
			if ( row[b] != target ) continue;
			// found a target transition.
			// use brute fource to find state pairs that map here.
			// Prefer terminal states.
			for ( l = 0; l <= lmax ; l++){
				if ( (*lmap)[l] != a ) continue;
				l_is_terminal = is_terminal_state(l);
				for ( r = 0; r <= rmax; r++ ){
					if ( (*rmap)[r] != b ) continue;
					// found one.
					r_is_terminal = is_terminal_state(r);
					switch( l_is_terminal + r_is_terminal ){
					case 2:
						// both terminal: we done.
						*lp = l;
						*rp = r;
						return;
					case 1:
						// save in case we get no
						// better offer.
						l_save = l;
						r_save = r;
						break;
					default:
						// well, at least we tried.
						if ( (l_save == 0)
						    || ( l < l_save )
						    || ( (l==l_save) && (r < r_save) )
						){
							l_save = l;
							r_save = r;
						}
					}
				}
			}
		}
	}
	//
	// didn't find anything with 2 terminal substates.
	// return whatever we did find.
	*lp = l_save;
	*rp = r_save;
}

static void
invert_unary_transition( unary_symbol * usyp, stateno target, stateno * lp )
{
	register unary_transition * usp = usyp->transitions;
	register stateno * row;
	register statemap * lmap = &usyp->lhs_map;
	stateno l_save =0;
	stateno a, amax;
	stateno l, lmax;

	amax = usp->max_stateno();
	lmax = lmap->max_ordinate();
	row = usp->state;
	for( a = 0; a <= amax ; a++ ){
		if ( row[a] != target )continue;
		// found a target transition.
		// use brute fource to find state numbers that map here.
		// Prefer terminal states.
		for ( l = 0; l <= lmax ; l++){
			if ( (*lmap)[l] != a ) continue;
			// found one.
			if( is_terminal_state(l)){
				// terminal: we done.
				*lp = l;
				return;
			} else {
				// save in case we get no
				// better offer.
				if ( l_save == 0 || l < l_save )
					l_save = l;
			}
		}
	}
	//
	// didn't find anything with terminal substate.
	// return whatever we did find.
	*lp = l_save;
}

static void
invert_substate( const state * sp, wordlist * wlp, int depth )
{
	const symbol * ssp = sp->symp;
	if ( ssp == 0 ){
		// null state here.
		return;
	}
	if ( depth > MAXDEPTH ){
		wlp->add( "..." );
		return;
	}
	wlp->add( ssp->name() );
	switch( ssp->type() ){
	case symbol::terminal:
		// we're done. There are no subtrees.
		return;
	case symbol::unaryop:
		stateno substate;
		invert_unary_transition( (unary_symbol *)ssp, sp->number, &substate );
		invert_substate( state_array[ substate ], wlp, depth+1 );
		return;
	case symbol::binaryop:
		stateno l_substate;
		stateno r_substate;
		invert_binary_transition( (binary_symbol *)ssp, sp->number, &l_substate , &r_substate );
		invert_substate( state_array[ l_substate ], wlp, depth+1 );
		invert_substate( state_array[ r_substate ], wlp, depth+1 );
		return;
	default:
		fprintf(stderr,"invert_substate: illegal symbol type\n");
		return;
	}
}
