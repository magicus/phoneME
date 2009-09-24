/*
 * @(#)compute_states.cc	1.8 06/10/10
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
#include <assert.h>
#include "symbol.h"
#include "rule.h"
#include "state.h"
#include "item.h"
#include "transition.h"
#include "debug.h"

extern "C" { int malloc_verify(void); };
extern "C" { extern int _lbound, _ubound; }; // from malloc.o
extern int semantic_error;
//
// DEBUGGING RELATED
int state_to_trace = -1;
extern void printstateinversion();
// END DEBUGGING RELATED
// 

/*
 * give terminals a state "transition"
 */
unsigned static
initialize_terminals(void)
{
	symbollist_iterator s_i = all_symbols;
	symbol *sp;
	terminal_symbol *tsp;
	state * resultp;

	while ((sp = s_i.next() ) != 0){
		if (sp->type() == symbol::terminal){
			tsp = (terminal_symbol *)sp;
			resultp = state::combine( tsp );
			if (DEBUG(COMBINE)){
				if ( state_to_trace < 0 
				    || state_to_trace == resultp->number 
				)
				printf("COMBINE( %s ): %d\n", tsp->name(),
				    resultp->number );
			}
			// link it up...
			tsp->transitions = leaf_transition::newtransition( tsp );
			insert_stateno( tsp->transitions, resultp->number);
		}
	}
	return allstates.n();
}

void static
unary_closure_iteration(
	unary_symbol *usp )
{
	unary_transition *tp;
	state * resultp;
	int lstateno, oldn;
	int curn;

	if (usp->transitions == 0)
		usp->transitions = unary_transition::newtransition( usp );
	tp = usp->transitions;
	oldn = tp->max_stateno();
	curn = usp->lhs_map.max_mapping();
	tp->grow(curn);
	// iterate through all new mapped states.
	for ( lstateno = oldn+1 ; lstateno <= curn ; lstateno ++ ){
		resultp = state::combine( usp, lstateno );
		insert_stateno( tp, lstateno, resultp->number);
	}

}

void static
binary_closure_iteration(
	binary_symbol *bsp )
{
	binary_transition *tp;
	state * resultp;
	int oldrowmax, oldcolmax, colmin, rstateno, lstateno;
	int currowmax, curcolmax;

	if (bsp->transitions == 0)
		bsp->transitions = binary_transition::newtransition( bsp );
	tp = bsp->transitions;
	oldrowmax = tp->row_max_stateno();
	oldcolmax = tp->col_max_stateno();
	currowmax = bsp->lhs_map.max_mapping();
	curcolmax = bsp->rhs_map.max_mapping();
	tp->grow( currowmax, curcolmax );

	// iterate through all mapped states.
	for ( lstateno = 0; lstateno <= currowmax ; lstateno ++ ){
		if ( lstateno <= oldrowmax )
			colmin = oldcolmax+1;
		else
			colmin = 0;
		for ( rstateno = colmin ; rstateno <= curcolmax ; rstateno ++ ){
			resultp = state::combine( bsp, lstateno, rstateno );
			insert_stateno( tp, lstateno, rstateno, resultp->number );
		}
			
	}
}

unsigned static
closure_iteration( int nstates, int nnewstates )
{
	symbollist_iterator s_i		 = all_symbols;
	symbol *	    sp;
	int		    curno	 = allstates.n();
	int		    old_nstates  = nstates - nnewstates;

	/*
	 * We only have to look at states that appeared in the previous
	 * call to closure_iteration, or in initialize_terminals.
	 * To expedite that process, we revv up an iterator to the
	 * current state of the states, RELYING ON THE FACT THAT
	 * ITERATION ORDER IS INSERTION ORDER, IS STATE NUMBER ORDER!!
	 * And since list addition is incompatable with list iteration,
	 * we make a copy to iterate over.
	 */
	assert( curno == nstates );

	while ( (sp = s_i.next() ) != 0){
		switch (sp->type()){
		case symbol::unaryop:
			unary_closure_iteration( (unary_symbol *)sp );
			break;
		case symbol::binaryop:
			binary_closure_iteration( (binary_symbol *)sp );
			break;
		default: // don't care
			break;
		}
	}

	return allstates.n()- curno;
}

int
compute_parental_matchsets( int nstates, int nnewstates )
{
	/*
	 * For each new state that's appeared in the last
	 * iteration, determine which can interestingly appear
	 * on the lhs and rhs of any non-leaf terminal, and
	 * add to matchsets accordingly.
	 * This precomputation speeds the binary closure_iterations
	 * by (1) factoring cse's of binary state building, and
	 * (2) allowing early detection of empty sets and consequently
	 * uninteresting transitions.
	 * Returns:
	 *	0: no items were added to any cost set
	 *	non-0: something was added.
	 */
	statelist_iterator sl_i;
	state * st_p;
	symbol * sym_p;
	item_cost cost;
	item_costset itemset;
	register item_table * t;
	item this_item, parent_item;
	item_costset_iterator set_i;
	int did_something = 0;
	int did_something_binary = 0;
	symbollist_iterator sym_i;
	unsigned v, mapped_v;
	int min_state_this_time = nstates - nnewstates;
	int max_state_this_time = nstates - 1;
	// DEBUGGING RELATED
	int tracing;
	int something_new;
	// END DEBUGGING RELATED

	sl_i = allstates;
	while( (st_p = sl_i.next() ) != 0 ){
		tracing = 0;
		if ( st_p->number < min_state_this_time )
			continue; // done before.
		itemset = st_p->items;
		t = itemset.pattab();
		set_i = itemset;
		// DEBUGGING RELATED
		if ( st_p->number == state_to_trace ){
			printf("Compute-parental: tracing state #%d\n", st_p->number);
			tracing = 1;
		}
		// END DEBUGGING RELATED
		while( (this_item = set_i.next( &cost ) ) != item_none ){
			switch( t->item_subpart( this_item ) ){
			case item_table::leftpart:
				parent_item = t->item_parent(this_item);
				sym_p = t->item_subtree( parent_item )->node;
				something_new = sym_p->lhs_temp_sets.cost_add( st_p->number, parent_item, cost );
				did_something += something_new;
				// DEBUGGING RELATED
				if ( tracing ){
					printf( "...on left of %s: %d\n",
					    sym_p->name(), something_new );
				}
				// END DEBUGGING RELATED
				break;
			case item_table::rightpart:
				parent_item = t->item_parent(this_item);
				sym_p = t->item_subtree( parent_item )->node;
				something_new = sym_p->rhs_temp_sets.cost_add( st_p->number, parent_item, cost );
				did_something += something_new;
				// DEBUGGING RELATED
				if ( tracing ){
					printf( "...on right of %s: %d\n",
					    sym_p->name(), something_new );
				}
				// END DEBUGGING RELATED
				break;
			}
		}
	}
	if ( ! did_something )
		return 0;
	/*
	 * now, for all the non-leaf terminal symbols,
	 * take the temp_sets and insert into them into the "real",
	 * unique sets. This also gives us our mapping, which we note.
	 * Don't forget to clean up after.
	 */
	sym_i = all_symbols;
	did_something = 0;
	while ( ( sym_p = sym_i.next() ) != 0 ){
		switch (sym_p->type()){
		case symbol::unaryop:
			sym_p->lhs_map.grow( max_state_this_time );
			for ( v = min_state_this_time;
			      v <= max_state_this_time;
			      v++ ){
				something_new = 0;
				itemset =sym_p->lhs_temp_sets.get( v );
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf("...on left of unary %s",
					    sym_p->name() );
				}
				// END DEBUGGING RELATED
				mapped_v = sym_p->lhs_sets.add_itemset(
					&itemset,
					&something_new
				);
				sym_p->lhs_map.new_mapping( v, mapped_v );
				did_something += something_new;
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf(" maps to %d: %d\n",
					     mapped_v, something_new );
				}
				// END DEBUGGING RELATED
			}
			sym_p->lhs_temp_sets.destruct();
			break;
		case symbol::binaryop:
			sym_p->lhs_map.grow( max_state_this_time );
			sym_p->rhs_map.grow( max_state_this_time );
			for ( v = min_state_this_time;
			      v <= max_state_this_time;
			      v++ ){
				something_new = 0;
				itemset =sym_p->lhs_temp_sets.get( v );
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf("...on left of binary %s",
					    sym_p->name() );
				}
				// END DEBUGGING RELATED
				mapped_v = sym_p->lhs_sets.add_itemset(
					&itemset,
					&something_new
				);
				sym_p->lhs_map.new_mapping( v, mapped_v );
				did_something += something_new;
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf(" maps to %d: %d\n",
					     mapped_v, something_new );
				}
				// END DEBUGGING RELATED

				something_new = 0;
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf("...on right of binary %s",
					    sym_p->name() );
				}
				// END DEBUGGING RELATED
				itemset =sym_p->rhs_temp_sets.get( v );
				mapped_v = sym_p->rhs_sets.add_itemset(
					&itemset,
					&something_new
				);
				sym_p->rhs_map.new_mapping( v, mapped_v );
				did_something += something_new;
				// DEBUGGING RELATED
				if ( v == state_to_trace ){
					printf(" maps to %d: %d\n",
					     mapped_v, something_new );
				}
				// END DEBUGGING RELATED
			}
			sym_p->lhs_temp_sets.destruct();
			sym_p->rhs_temp_sets.destruct();
			break;
		default:
			break;  // others are leaves.
		}
	}

	return did_something;

}

static void
resize_a_map( statemap * smp, int max_stateno )
{
	int stateno = smp->max_ordinate();

	if ( max_stateno <= stateno ) return;
	smp->grow( max_stateno );
	for( stateno +=1; stateno <= max_stateno; stateno ++ )
		smp->new_mapping( stateno, 0 );
}

static void
resize_all_maps( int max_stateno )
{
	symbollist_iterator s_i		 = all_symbols;
	symbol *	    sp;
	/*
	 * We have no more legal transformations to add,
	 * but we should make sure to represent the illegal ones
	 * without problem.
	 */
	while ( (sp = s_i.next() ) != 0){
		switch (sp->type()){
		case symbol::binaryop:
			resize_a_map( & sp->rhs_map, max_stateno );
			// FALL THRU
		case symbol::unaryop:
			resize_a_map( & sp->lhs_map, max_stateno );
			break;
		default: // don't care
			break;
		}
	}

}

void
compute_states(void){
	// boot with empty state and with terminals.
	// then do a small number of closure iterations;
	int nstates;
	int nnewstates;
	int niters;
	state * sp;
	int parental_sets_augmented;

	// this must be first!!
	sp = state::null_state( &all_items );

	nstates = initialize_terminals();
	nnewstates = nstates;

	for (niters=1; niters<10; niters +=1){
		parental_sets_augmented = compute_parental_matchsets( nstates, nnewstates );
		if ( ! parental_sets_augmented ){
			/*
			 * none of the states recently added had any
			 * effect on the transition matricies for any of
			 * the non-leaf terminals. So don't even bother
			 * doing another closure, because there will be
			 * no new legal transitions. However, we should
			 * make sure that all our state maps are long enough
			 * to represent the illegal transitions that may
			 * occur.
			 */
			resize_all_maps( nstates-1 );
			return;
		}
		nnewstates = closure_iteration( nstates, nnewstates );
		if (nnewstates == 0){
			// we've iterated till done. we're done.
			return;
		}
		nstates += nnewstates;
	}
	printf("\n\nITERATION TIMEOUT\n");
	semantic_error += 1;
	
}
