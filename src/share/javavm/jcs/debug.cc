/*
 * @(#)debug.cc	1.6 06/10/10
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
#include "debug.h"
#include "rule.h"
#include "item.h"
#include "matchset.h"
#include "symbol.h"
#include "transition.h"

extern int n_setalloc, n_setfree, max_setactive, n_setactive;
extern int n_sparsealloc, n_sparsefree, max_sparseactive, n_sparseactive;

enum item_category {
	state_core=0, state_closure,
	symbol_match_temp, symbol_match_nontemp,
	symbol_core, symbol_closure,
	item_summary,
	N_ITEM_CATEGORY };

struct item_accumulator{
	int 	n_empty_sets;
	int	n_nonempty_sets;
	int	n_items;
};

static
item_accumulator i_counters[ N_ITEM_CATEGORY ];

static void
count_items(
	const item_costset & v,
	item_accumulator * iap )
{
	if ( v.is_empty() ){
		iap->n_empty_sets += 1;
	} else {
		iap->n_nonempty_sets += 1;
		iap->n_items += v.n();
	}
}

static void
count_matchset_items(
	const matchset & s,
	item_accumulator *iap )
{
	item_costset v;
	int i;
	int maxno = s.get_max_state_number();
	for ( i = 0 ; i <= maxno ; i ++ ) {
		v = s.get( i );
		count_items( v, iap );
		v.destruct();
	}
}


static void
count_unique_matchset_items(
	const unique_matchset & s,
	item_accumulator *iap )
{
	item_costset v;
	int i;
	int maxno = s.get_max_state_number();
	for ( i = 0 ; i <= maxno ; i ++ ) {
		v = s.get( i );
		count_items( v, iap );
	}
}


static void
count_items_in_symbol( symbol * sp )
{
	count_matchset_items( sp->lhs_temp_sets,
		&i_counters[ symbol_match_temp ] );
	count_matchset_items( sp->rhs_temp_sets,
		&i_counters[ symbol_match_temp ] );
	count_unique_matchset_items( sp->lhs_sets,
		&i_counters[ symbol_match_nontemp ] );
	count_unique_matchset_items( sp->rhs_sets,
		&i_counters[ symbol_match_nontemp ] );
	count_items( sp->get_core_set(), 
		&i_counters[ symbol_core ] );
	count_items( sp->get_closure(), 
		&i_counters[ symbol_closure ] );
}


static void
count_items_in_state( state * sp )
{
	count_items( sp->core,
	    &i_counters[ state_core ] );
	count_items( sp->items,
	    &i_counters[ state_closure ] );

}

void
printsetclass( const char * uber, const char * sub,
	item_accumulator * iap )
{
	printf("%s\t%s\t%d\t%d\t\t%d\t%f\n",
		uber, sub,
		iap->n_empty_sets, iap->n_nonempty_sets, iap->n_items,
		(double)(iap->n_items)/(double)(iap->n_nonempty_sets) );
}

void
printitemsetstats(void)
{
	int n_items = all_items.n();
	int i;

	for ( i = state_core; i < N_ITEM_CATEGORY ; i ++ ){
		i_counters[i].n_empty_sets =0;
		i_counters[i].n_nonempty_sets =0;
		i_counters[i].n_items =0;
	}

	// count sets in matchsets in symbols.
	symbol * symp;
	symbollist_iterator syml_i = all_symbols;

	while ( ( symp = syml_i.next() ) != 0 ){
		count_items_in_symbol( symp );
	}


	// count sets in states
	state * statp;
	statelist_iterator stl_i = allstates;

	while ( ( statp = stl_i.next() ) != 0 ){
		count_items_in_state( statp );
	}

	// hope thats all.
	// print summary.
	puts("\nITEM SET SUMMARY:");
	printf("    There are %d possible items in each set\n", n_items);
	printf("    %d were allocated, of which %d have been freed\n",
		n_setalloc, n_setfree );
	printf("    %d should now be active, the maximum active at once has been %d\n",
		n_setactive, max_setactive );

	printf("    %d sparse set items were allocated, of which %d have been freed\n",
		n_sparsealloc, n_sparsefree );
	printf("    %d sparse set items should now be active, the maximum active at once has been %d\n",
		n_sparseactive, max_sparseactive );

	puts("\nClass\t\tempty\tnonempty\ttotal\taverage");
	puts("\t\tsets\tsets\t\titems\titems/set");

	printsetclass("Symbol", "temp",
	    &i_counters[ symbol_match_temp ] );

	printsetclass("Symbol", "untemp",
	    &i_counters[ symbol_match_nontemp ] );

	printsetclass("Symbol", "core",
	    &i_counters[ symbol_core ] );

	printsetclass("Symbol", "closure",
	    &i_counters[ symbol_closure ] );

	printsetclass("State", "core",
	    &i_counters[ state_core ] );

	printsetclass("State", "closure",
	    &i_counters[ state_closure ] );

	for ( i = state_core; i < item_summary ; i ++ ){
		i_counters[item_summary].n_empty_sets += i_counters[i].n_empty_sets;
		i_counters[item_summary].n_nonempty_sets += i_counters[i].n_nonempty_sets;
		i_counters[item_summary].n_items += i_counters[i].n_items;
	}
	printsetclass("\nTOTAL","",
	    &i_counters[ item_summary ] );
}

/*
 * Print out a matchset as an array of itemsets.
 */
static void
printmatchset( const char * name, unique_matchset & ums )
{
	int i, max;
	item_costset ics;
	max = ums.get_max_state_number();
	for( i = 0; i <= max ; i ++ ){
		printf("<%s:%d>:\n", name, i );
		ums.get( i ).print();
	}
}

/*
 * To print each symbol's matchsets.
 * Do this only if you really mean it: it will be verbose.
 */
void
printsymbolmatchsets(void)
{
	symbol * symp;
	symbollist_iterator syml_i = all_symbols;

	printf("\n================================\nSYMBOL MATCH SETS:\n");
	while ( ( symp = syml_i.next() ) != 0 ){
		printf("SYMBOL %s:\n", symp->name() );
		switch( symp->type() ){
		case symbol::unaryop:
			printmatchset( "left", symp->lhs_sets );
			break;
		case symbol::binaryop:
			printmatchset( "left", symp->lhs_sets );
			printmatchset( "right", symp->rhs_sets );
			break;
		case symbol::terminal:
		case symbol::nonterminal:
		default:
			printf("\tnone\n");
			break;
		}
	}

}


/*
 * For reasons of extreme laziness, I print itemset stats and 
 * transition table stats together, ...
 */

enum symbol_categories {
	sym_nonterminal = 0,
	sym_leaf,
	sym_unary,
	sym_binary,
	sym_total,
	N_SYM_CATEGORY };

struct symbol_accumulator {
	int	n_symbols;
	int	size_statemaps;
	int	size_transitions;
};

static struct symbol_accumulator s_counters[ N_SYM_CATEGORY ];

static void
count_statemaps( symbol * sp, struct symbol_accumulator * sap )
{
	sap->size_statemaps += sp->lhs_map.max_mapping();
	sap->size_statemaps += sp->rhs_map.max_mapping();
}

static void
count_binary_symbol_transitions( binary_symbol * bsp )
{
	s_counters[ sym_binary ].n_symbols += 1;
	count_statemaps( bsp, &s_counters[ sym_binary ]);
	if ( bsp->transitions != 0 ){
		s_counters[ sym_binary ].size_transitions +=
		      (bsp->transitions->col_max_stateno() + 1)
		    * (bsp->transitions->row_max_stateno() + 1);
	}
}

static void
count_unary_symbol_transitions( unary_symbol * usp )
{
	s_counters[ sym_unary ].n_symbols += 1;
	count_statemaps( usp, &s_counters[ sym_unary ]);
	if ( usp->transitions != 0 ){
		s_counters[ sym_unary ].size_transitions +=
			usp->transitions->max_stateno()+1;
	}
}

static void
count_leaf_symbol_transitions( terminal_symbol * tsp )
{
	s_counters[ sym_leaf ].n_symbols += 1;
	count_statemaps( tsp, &s_counters[ sym_leaf ] );
}

static void
count_nonterminal_symbol_transitions( nonterminal_symbol * ntsp )
{
	s_counters[ sym_nonterminal ].n_symbols += 1;
	count_statemaps( ntsp, &s_counters[ sym_nonterminal ] );
}


static void
printsymbolclass( const char * name,
	struct symbol_accumulator * sap )
{
	printf( "%-16s%-8d%-16d%-8d\n", name,
	    sap->n_symbols,
	    sap->size_statemaps,
	    sap->size_transitions );
}

static void
printtransitionstats(void)
{
	// count maps and transitions
	symbol * symp;
	symbollist_iterator syml_i = all_symbols;
	int i;
	for ( i = 0; i < N_SYM_CATEGORY; i++){
		s_counters[i].n_symbols = 0;
		s_counters[i].size_statemaps = 0;
		s_counters[i].size_transitions = 0;
	}

	while ( ( symp = syml_i.next() ) != 0 ){
		switch ( symp->type() ){
		case symbol::binaryop:
			count_binary_symbol_transitions( (binary_symbol*)symp );
			break;
		case symbol::unaryop:
			count_unary_symbol_transitions( (unary_symbol*)symp );
			break;
		case symbol::terminal:
			count_leaf_symbol_transitions( (terminal_symbol*)symp );
			break;
		case symbol::nonterminal:
			count_nonterminal_symbol_transitions( (nonterminal_symbol*)symp );
			break;
		}
	}

	// print it out.
	puts("\nSYMBOL AND TRANSITION SUMMARY");
	puts("Type\t\tnumber\tstatemap\ttransition");
	puts("\t\t\tentries\t\tentries");

	printsymbolclass("Unary",       &s_counters[sym_unary] );
	printsymbolclass("Binary",      &s_counters[sym_binary] );
	printsymbolclass("Leaf",        &s_counters[sym_leaf] );
	printsymbolclass("Nonterminal", &s_counters[sym_nonterminal] );

	printf("\n");
	for ( i = 0; i < sym_total; i++){
		s_counters[sym_total].n_symbols += s_counters[i].n_symbols;
		s_counters[sym_total].size_statemaps += s_counters[i].size_statemaps;
		s_counters[sym_total].size_transitions += s_counters[i].size_transitions;
	}

	printsymbolclass("TOTAL",       &s_counters[sym_total] );
}

void
printsetstats(void)
{
	if ( verbose )
	    printsymbolmatchsets();
	printitemsetstats();
	printtransitionstats();
}
