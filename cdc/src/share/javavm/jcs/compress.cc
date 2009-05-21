/*
 * @(#)compress.cc	1.10 06/10/10
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compress.h"
#include "rule.h"
#include "state.h"
#include "symbol.h"
#include "transition.h"
#include "debug.h"
#include "globals.h"

/*
 * @(#)compress.cc	1.9	93/10/25
 * This file defines no classes
 * This file defines routines for massaging
 * statemap and transition tables,
 * for the purpose of producing more compact output.
 *
 * Towards this end, we iteratively remove redundant
 * rows & columns in transition tables, and redundant
 * states.
 * For our purposes, two states are the same ( and thus one
 * of them is redundant ) if they have all the same transitions.
 * This can be true even if they represent different itemsets and
 * thus different partial matches.
 */

extern int uncompress;

extern void delete_inversion_info();
/*
 * Squeeze redundance out of a unary transition table.
 * Simply look for identical row entries, and delete them,
 * while remapping the states.
 * Pretty brute-force sort of algorithm used here.
 *
 * We are provided with temp vectors allocated for our use,
 * since the same can be used for each transition table being
 * compressed, we save a little allocation time.
 */
int
compress_unary_transition(
	unary_symbol *usp,
	char *  delvector )
{
	statemap * 	   map = & usp->lhs_map;
	unary_transition * ut  = usp->transitions;
	stateno *	   row = ut->state;
	int		   n_redundant = 0;
	stateno		   n, m, t, new_n, new_m;
	stateno		   max_transition = ut->max_stateno();

	// first, find redundant entries. List them.
	memset( delvector, 0, (max_transition+1)*sizeof(char) );
	new_n = 0;
	for ( n = 0 ; n < max_transition ; n++ ){
		if ( delvector[n] )
			continue; // already detected as redundant!!
		t = row[n];
		new_m = new_n+1;
		for ( m = n+1; m <= max_transition ; m ++ ){
			if ( delvector[m] )
				continue;
			if (row[m] == t ){
				// the same
				delvector[m] = 1; // mark as redundant
				map->remap( new_m, new_n ); // change mapping.
				n_redundant ++;
			} else {
				new_m += 1; // not the same
			}
		}
		new_n += 1;
	}
	// get rid of those redundant entries
	ut->delete_transitions( delvector );

	return n_redundant;
}

/*
 * See if this is a degenerate unary_symbol: i.e. one with only one
 * non-error transition state. If so, mark it as functionally terminal
 * and give it a terminal_transition table.
 */
void
error_compress_unary_transition(
	unary_symbol *usp )
{
	statemap * 	   map = & usp->lhs_map;
	unary_transition * ut  = usp->transitions;
	stateno *	   row = ut->state;
	int		   found_non_error_state = 0;
	stateno		   n, t, non_error_state = 0; 
	stateno		   max_transition = ut->max_stateno();
	leaf_transition *  lt;

	for ( n = 0 ; n <= max_transition ; n++ ){
	    t = row[n];
	    if ( t == NULL_STATE )
		continue;
	    if ( found_non_error_state ){
		if ( t != non_error_state ){
		    return; // non-degenerate map. Cannot compress
		}
	    } else {
		found_non_error_state = 1;
		non_error_state = t;
	    }
	}
	// made it through the loop finding (at most) one non-error state.
	// this is a degenerate symbol. Mark it so and give it a terminal_transition.
	usp->set_functional_type( symbol::terminal );
	ut->free();
	lt = leaf_transition::newtransition( (terminal_symbol*) usp );
	lt->state = non_error_state;
	usp->transitions = (unary_transition*)lt;
	if (DEBUG(COMPRESS) && n != 0 ){
	    printf("Error compression reduced unary %s to functionally terminal\n",
		usp->name() );
	}

}

/*
 * Squeeze redundance out of a binary transition table.
 * Simply look for identical row or columns, and delete them,
 * while remapping the states.
 * Pretty brute-force sort of algorithm used here.
 *
 * We are provided with temp vectors allocated for our use,
 * since the same can be used for each transition table being
 * compressed, we save a little allocation time.
 */

static int
compress_binary_transition_row(
	binary_symbol *bsp,
	char *  delvector )
{
	statemap * 	   map = & bsp->lhs_map;
	binary_transition * bt  = bsp->transitions;
	binary_transition_row *	   rows = bt->state;
	int		   n_redundant = 0;
	stateno		   n, m, new_n, new_m;
	stateno *	   t;
	stateno		   max_transition = bt->row_max_stateno();
	int		   row_size;

	row_size = (bt->col_max_stateno()+1)*sizeof(stateno);
	// first, find redundant entries. List them.
	memset( delvector, 0, (max_transition+1)*sizeof(char) );
	new_n = 0;
	for ( n = 0 ; n < max_transition ; n++ ){
		if ( delvector[n] != 0 )
			continue; // already detected as redundant!!
		t = rows[n].state;
		new_m = new_n + 1;
		for ( m = n+1; m <= max_transition ; m ++ ){
			if ( delvector[m] )
				continue; // already dead
			if (
			    memcmp( rows[m].state, t, row_size ) == 0
			){
				delvector[m] = 1; // mark as redundant
				// change mapping.
				// (map has already renumbered to
				// compensate for previously removed
				// rows, so we must compensate, too.)
				map->remap( new_m, new_n );
				n_redundant ++;
			} else {
				new_m += 1;
			}
		}
		new_n += 1;
	}
	// get rid of those redundant entries
	bt->delete_transition_rows( delvector );

	return n_redundant;
}

static int
compress_binary_transition_col(
	binary_symbol *bsp,
	char *  delvector )
{
	statemap * 	   map = & bsp->rhs_map;
	binary_transition * bt  = bsp->transitions;
	binary_transition_row *	   rows = bt->state;
	int		   n_redundant = 0;
	stateno		   n, m, new_n, new_m;
	stateno		   max_transition = bt->col_max_stateno();
	register int	   col_size = bt->row_max_stateno();
	int		   col_same;
	register int 	   c;

	// first, find redundant entries. List them.
	memset( delvector, 0, (max_transition+1)*sizeof(char) );
	new_n = 0;
	for ( n = 0 ; n < max_transition ; n++ ){
		if ( delvector[n] != 0 )
			continue; // already detected as redundant!!
		new_m = new_n +1;
		for ( m = n+1; m <= max_transition ; m ++ ){
			if ( delvector[m] )
				continue; // already dead
			col_same = 1;
			for ( c = 0 ; c <= col_size; c++ ){
				// compare the difficult way.
				// NOTE: We might want optimization here.
				if ( rows[c].state[m] != rows[c].state[n] ){
					col_same = 0;
					break;
				}
			}
			if ( col_same ){
				delvector[m] = 1; // mark as redundant
				// change mapping.
				// (map has already renumbered to
				// compensate for previously removed
				// cols, so we must compensate, too.)
				map->remap( new_m, new_n );
				n_redundant ++;
			} else {
				new_m += 1;
			}
		}
		new_n += 1;
	}
	// get rid of those redundant entries
	bt->delete_transition_cols( delvector );

	return n_redundant;
}

static int
map_is_degenerate( statemap *    map )
{
	stateno max_mapped = map->max_ordinate();
	stateno m, t, non_error_value = 0;
	int	found_non_error_value = 0;
	for ( m = 0; m < max_mapped; m++ ){
		t = (*map)[m];
		if ( t == NULL_STATE )
		    continue;
		if ( found_non_error_value ){
		    if ( t != non_error_value )
			return 0; // more than one non-error mapping here.
		} else {
		    found_non_error_value = true;
		    non_error_value = t;
		}
	}
	return 1;
}

static void
error_compress_binary_transition(
	binary_symbol *bsp )
{
	binary_transition * bt  = bsp->transitions;
	binary_transition_row *	   rows = bt->state;
	int		   found_non_error_state = 0;
	stateno		   n, m, t, non_error_state =0;
	stateno		   max_transition = bt->col_max_stateno();
	register int	   col_size = bt->row_max_stateno();
	leaf_transition *  lt;
	statemap * 	   map;
	unary_transition * ut;
	stateno		   maxmapped;

	for ( n = 0 ; n <= max_transition ; n++ ){
		for ( m = 0; m <= col_size ; m ++ ){
			t = rows[m].state[n];
			if ( t == NULL_STATE )
			    continue;
			if ( found_non_error_state ){
			    if ( t != non_error_state ){
				goto try_unary; // transitions not totally degenerate.
			    }
			} else {
			    found_non_error_state = 1;
			    non_error_state = t;
			}
		}
	}
	// made it through the loop finding (at most) one non-error state.
	// this degenerates to a terminal.
	bsp->set_functional_type( symbol::terminal );
	bt->free();
	lt = leaf_transition::newtransition( (terminal_symbol*) bsp );
	lt->state = non_error_state;
	bsp->transitions = (binary_transition*)lt;
	if (DEBUG(COMPRESS) && n != 0 ){
	    printf("Error compression reduced binary %s to functionally terminal\n",
		bsp->name() );
	}
	return;
try_unary:
#if 0
	/*
	 * This is not debugged: in fact I think I've confused rows and
	 * columns. In any case, there are few instances where it helps.
	 */
	/*
	 * Here, we know that this is not a totally trivial transition.
	 * It may degenerate to unary though. Look at the mapping vectors, and
	 * if either of them is degenerate, then this symbol is unary in the 'other'
	 * dimension.
	 */
	/* first the RHS map */
	if ( map_is_degenerate( & bsp->rhs_map )){
	    stateno *nsp;
	    stateno  m, mmax;
	    /* all columns are the same. (or error) choose one
	     * to make into a one-row unary transition. use the lhs_map as is.
	     */
	    bsp->set_functional_type( symbol::unaryop );
	    ut = unary_transition::newtransition( (unary_symbol*) bsp );
	    ut->grow( mmax = bsp->lhs_map.max_mapping() );
	    nsp = ut->state;
	    for ( m = 0; m <= mmax; m++ ){
		nsp[m] = bt->state[m].state[1];
	    }
	    bsp->transitions = (binary_transition*)ut;
	    bt->free();
	    bsp->rhs_map.destruct();
	    if (DEBUG(COMPRESS) && n != 0 ){
		printf("Error compression reduced binary %s to functionally left-unary\n",
		    bsp->name() );
	    }
	    return;
	} else if ( map_is_degenerate( & bsp->lhs_map )){
	    stateno *nsp;
	    stateno  m, mmax;
	    /* all rows are the same. (or error) choose one
	     * to make into a one-row unary transition. replace lhs_map with rhs_map;
	     */
	    bsp->set_functional_type( symbol::rightunaryop );
	    ut = unary_transition::newtransition( (unary_symbol*) bsp );
	    ut->grow( mmax = bsp->rhs_map.max_mapping() );
	    nsp = ut->state;
	    for ( m = 0; m <= mmax; m++ ){
		nsp[m] = bt->state[1].state[m];
	    }
	    bsp->transitions = (binary_transition*)ut;
	    bt->free();
	    bsp->lhs_map.destruct();
	    bsp->lhs_map = bsp->rhs_map;
	    if (DEBUG(COMPRESS) && n != 0 ){
		printf("Error compression reduced binary %s to functionally right-unary\n",
		    bsp->name() );
	    }
	    return;
	}
#else
	(void)0;
#endif
}

static int
compress_transitions()
{

	char * bool_temp;
	symbollist_iterator  sl_i;
	symbol * sp;
	int maxstate = allstates.n() -1;
	int n;
	int n_sum;

	// allocate up temporary for use by all callees.
	// can never need more that total number of states.
	bool_temp = (char *)malloc( allstates.n() );
	assert( bool_temp != 0 );

	n_sum = 0;
	sl_i = all_symbols;
	while ( ( sp = sl_i.next() ) != 0 ){
		switch ( sp->type() ){
		case symbol::unaryop:
			n = compress_unary_transition(
			    (unary_symbol *) sp,
			    bool_temp
			);
			if (DEBUG(COMPRESS) && n != 0 ){
			    printf("Compressed %d transitions from unary %s\n",
				n, sp->name() );
			}
			n_sum += n;
			break;
		case symbol::binaryop:
			n = compress_binary_transition_row(
			    (binary_symbol *) sp,
			    bool_temp
			);
			if (DEBUG(COMPRESS) && n != 0 ){
			    printf("Compressed %d rows from binary %s\n",
				    n, sp->name() );
			}
			n_sum += n;
			n = compress_binary_transition_col(
			    (binary_symbol *) sp,
			    bool_temp
			);
			if (DEBUG(COMPRESS) && n != 0 ){
			    printf("Compressed %d cols from binary %s\n",
				    n, sp->name() );
			}
			n_sum += n;
			break;
		}
	}

	free( bool_temp );
	return n_sum;
}

/*
 * Detecting redundant states is easier than it sounds.
 * All you really need to do is make sure that they always
 * mean the same thing. This means looking at the transition
 * maps to see if they map to the same entry/row/column.
 * Also, make sure they represent the same full matches of the
 * same rules, if any. After all, we need them to cause the same
 * semantic actions.
 */

static int
are_states_redundant( state * s1, state * s2 )
{
	int * r1, * r2;
	int n_r;
	int sn1, sn2;
	symbollist_iterator sym_i;
	symbol * symp;

	// first, compare production semantics.
	n_r = nonterminal_symbols.n();
	if ( s1->rules_to_use == 0 )
		s1->use_rules();
	r1 = s1->rules_to_use;
	if ( s2->rules_to_use == 0 )
		s2->use_rules();
	r2 = s2->rules_to_use;
	while( n_r -- > 0 ){
		if (*(r1++) != *(r2++) )
			return 0; // immediate difference
	}
	// ok, so they use all the same rules.
	// but do the act the same in the transition tables??
	sn1 = s1->number;
	sn2 = s2->number;
	sym_i = all_symbols;
	while ( ( symp = sym_i.next() ) != 0 ){
		switch ( symp->type() ){
		case symbol::binaryop:
			if (symp->rhs_map[sn1] != symp->rhs_map[sn2] )
				return 0; // don't map the same.
			// FALL THRU
		case symbol::unaryop:
			if (symp->lhs_map[sn1] != symp->lhs_map[sn2] )
				return 0; // don't map the same.
			break;
		}
	}
	//
	// they have the same rules.
	// they make the same transitions.
	// they are the same.
	return 1;
}

static void
whack_state_list( const int * renumber_vector )
{
	int new_number;
	state * sp;
	statelist_iterator sli;
	statelist new_list;

	// renumber_vector is here just for sanity check.
	sli = allstates;
	new_number = 0;
	while( (sp = sli.next() ) != 0 ){
		if ( sp->is_active() ){
			// renumber and put on new list.
			assert( new_number == renumber_vector[ sp->number ] );
			sp->number = new_number;
			new_number += 1;
			new_list.add( sp );
		}
		// else: we could reap dead states but DO NOT.
	}
	// REPLACE OLD LIST WITH NEW
	allstates.destruct();
	allstates = new_list;

}

static void
renumber_transition_vector(
	register stateno *svector,
	register int max_subscript,
	register const int * renumber_vector )
{
	register int i;

	for( i = 0; i <= max_subscript ; i ++ ){
		svector[i] = renumber_vector[ svector[i] ];
	}
}

static void
renumber_unary_transition(
	unary_transition * usp,
	register const int * renumber_vector )
{

	renumber_transition_vector(
	    usp->state,
	    usp->max_stateno(),
	    renumber_vector );
}

static void
renumber_binary_transition(
	binary_transition * bsp,
	register const int * renumber_vector )
{
	int i;
	int max_ro_no = bsp->row_max_stateno();
	int max_col_no = bsp->col_max_stateno();
	binary_transition_row * rowvec = bsp->state;

	for( i = 0; i <= max_ro_no ; i ++ ){
		renumber_transition_vector(
		    rowvec[i].state, max_col_no, renumber_vector );
	}
}

static void
renumber_statemap(
	register statemap * smp,
	register const char * delvector,
	int new_state_max,
	int old_state_max )
{
	statemap newmap;
	register stateno old_i, new_i;
	register stateno oldmax;
	// if state numbers are changing, then
	// map ordinates are, too. Deal with it.
	newmap.grow( new_state_max );
	oldmax = smp->max_ordinate();
	for( old_i = 0, new_i = 0 ; old_i <= oldmax ; old_i++ ){
		if ( ! delvector[ old_i ] ){
			// keep a state.
			newmap.new_mapping( new_i, (*smp)[ old_i ] );
			new_i += 1;
		}
	}
	assert( new_i == new_state_max+1 );
	assert( old_i <= old_state_max+1 );
	smp->destruct();
	*smp = newmap;

}

static void
renumber_transitions(
	const int * renumber_vector,
	const char * delvector,
	int new_state_max,
	int old_state_max )
{
	symbollist_iterator  sl_i;
	symbol * sp;

	sl_i = all_symbols;
	while ( ( sp = sl_i.next() ) != 0 ){
		switch ( sp->type() ){
		case symbol::unaryop:
			renumber_unary_transition(
			    ((unary_symbol *) sp)->transitions,
			    renumber_vector
			);
			renumber_statemap(
			    & sp->lhs_map, delvector, new_state_max, old_state_max );
			break;
		case symbol::binaryop:
			renumber_binary_transition(
			    ((binary_symbol *) sp)->transitions,
			    renumber_vector
			);
			renumber_statemap(
			    & sp->rhs_map, delvector, new_state_max, old_state_max );
			renumber_statemap(
			    & sp->lhs_map, delvector, new_state_max, old_state_max );
			break;
		case symbol::terminal: {
			// the easy case.
			leaf_transition * ltp = ((terminal_symbol *)sp)->transitions;
			ltp->state = renumber_vector[ ltp->state ];
			break;
		    }
		}
	}

}

static int
delete_redundant_states()
{
	statelist_iterator a, b;
	state * q, *r;
	int n_states = allstates.n();
	int * renumber_vector = (int *)malloc( n_states*sizeof(int) );
	int delvector_size= n_states*sizeof(char);
	char * delvector = (char *)malloc( delvector_size );
	int new_state_number;
	int n_redundant = 0;
	int i;

	/*
	 * delvector : an entry per state.
	 *	0 =>still active
	 *	1 =>deactivated, will be removed.
	 * This is redundant with is_active() information in the state
	 * table, but is more convenient for us.
	 *
	 * renumber_vector: an entry per state.
	 *	indexed by old state number, gives new state number.
	 *	for use when whacking transition tables.
	 */
	a = allstates;
	memset( delvector, 0, delvector_size );

	new_state_number = 0;
	while( ( q = a.next() ) != 0 ){
		if ( ! q->is_active() ) continue;
		assert( delvector[ q->number ] == 0 );

		renumber_vector[ q->number ] = new_state_number;
		b = a; // copy iterator state.
		while ( ( r = b.next() ) != 0 ){
			if ( r == q ) continue; // oops.
			if ( ! r->is_active() ) continue; // more oops.
			assert( delvector[ r->number ] == 0 );

			if ( are_states_redundant( q, r ) ){
				if (DEBUG(COMPRESS)){
				    printf("Redundant states %d and %d detected\n",
					q->number, r->number );
				}
				delvector[ r->number ] = 1;
				assert ( q->number < r->number ); // this matters
				renumber_vector[ r->number ] = new_state_number;
				r->deactivate();
				n_redundant ++;
			}
		}
		new_state_number += 1;
	}
	/*
	 * An example is wanted here. Let us say that we had 9 states,
	 * and have found that state 4 is redundant with state 2, and
	 * that state 7 is redundant with state 5. Notice that the above
	 * assertion of ( q->number < r->number ) ensures us that we delete
	 * the HIGHER numbered redundant state. This is what our data look
	 * like in this case:
	 *
	 * i 			0  1  2  3  4  5  6  7  8
	 * delvector[i]		0  0  0  0  1  0  0  1  0
	 * renumber_vector[i]	0  1  2  3  2  4  5  4  6
	 * 
	 * Notice in this case that renumber_vector[7] gives the NEW state
	 * number (4) corresponding to the redundant state (was 5).
	 */

	if ( n_redundant == 0 ){
		// boring: didn't find anything redundant
		free( (char *)delvector );
		free( (char *)renumber_vector );
		return 0;
	}

	/*
	 * Just to be really sure that things look as we expect ....
	 */
	new_state_number = -1;
	for ( i = 0; i < n_states; i ++ ){
		if ( delvector[i] ){
			assert( renumber_vector[i] <= new_state_number );
		} else {
			assert( renumber_vector[i] == ++new_state_number );
		}
	}

	/*
	 * Delete deactivated states from the state table.
	 * and renumber the survivors.
	 * We don't really need to do this, but I feel uncomfortable
	 * leaving inactive states in the table.
	 */
	whack_state_list( renumber_vector );

	/*
	 * Now for the big work: traverse ALL the transition tables,
	 * renumbering ALL the state numbers.
	 */
	renumber_transitions( renumber_vector, delvector, allstates.n()-1, n_states-1 );

	// done
	free( (char *)delvector );
	free( (char *)renumber_vector );

	if ( n_redundant != 0 )
		delete_inversion_info();
	return n_redundant;
}

// DEBUG
extern void print_transition_tables(
    const char * statetype,
    FILE *output,
    FILE *data );
// DEBUG
int
compress_output()
{
	int n_states = 0;
	int n;
	// DEBUG
	int iterno;
	if ( uncompress!= 0 ) return 0;

	iterno = 1;
	// DEBUG
	if (DEBUG(COMPRESS) && DEBUG(DUMPTABLES)){
		printf("Before compress_transition\n" );
		print_transition_tables( "INT", stdout, stdout );
	}
	// DEBUG
	while( compress_transitions()){
		// DEBUG
		if (DEBUG(COMPRESS) && DEBUG(DUMPTABLES)){
			printf("After compress_transition #%d\n", iterno );
			print_transition_tables( "INT", stdout, stdout );
		}
		// DEBUG
		n = delete_redundant_states();
		// DEBUG
		if ( n != 0 ){
			if (DEBUG(COMPRESS) && DEBUG(DUMPTABLES)){
			    printf("After delete_redundant_states #%d\n", iterno );
			    print_transition_tables( "INT", stdout, stdout );
			}
		}
		// DEBUG
		n_states += n;
		if ( n == 0 ) break;
		iterno += 1;
	}
	if ( n_states != 0 && DEBUG(DUMPTABLES) )
		printstates(0);
	if (error_folding){
		symbollist_iterator  sl_i;
		symbol * sp;

		sl_i = all_symbols;
		while ( ( sp = sl_i.next() ) != 0 ){
			switch ( sp->type() ){
			case symbol::unaryop:
				error_compress_unary_transition( (unary_symbol *) sp );
				break;
			case symbol::binaryop:
				error_compress_binary_transition( (binary_symbol *) sp );
				break;
			}
		}
	}
	return n_states;
}
