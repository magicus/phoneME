/*
 * @(#)state.cc	1.9 06/10/10
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
#include <assert.h>
#include "wordlist.h"
#include "symbol.h"
#include "item.h"
#include "rule.h"
#include "debug.h"
#include "state.h"

/*
 * @(#)state.cc	2.24	93/10/25
 *
 * A "state" is just a fancy "itemset",
 * plus a name, plus a uniqueness property.
 */

extern wordlist invert_state( const state * sp );

/*
 * construct a list of items of the form:
 *	[ x -> alpha < s > beta; 0] for terminal s;
 *	[ x -> alpha < s treeL > beta; 0] for unary s;
 * or	[ x -> alpha < s treeL treeR > beta; 0 ] for binary s.
 */

item_costset
allmatch( symbol *s )
{
	/* theoretically, allmatch is used a lot
	 * realistically, it will only be used for leaves.
	 */
	return s->get_core_set();
}

/*
 * Some Predictive (or is it Forward?)  Trimming:
 * Look at the parents of the subtree we've matched in the case
 * of a partial match. If there are two items with the same parent (unary)
 * operator, and the same resultant nonterminal lhs (simple rules only here),
 * then the one with the higher cost of subtree-match-plus-rule-cost will
 * be trimmed out in the next step, so we might as well get rid of it here.
 * Binaries are more complicated.
 * 
 * This trimmer is called by itemset_trim when any item is found
 * which is the descendent of a fullmatch item. We return 1 if
 * the current item is the cheapest one of this sort, or 0 if we find
 * a cheaper one in the set.
 */
static int
predictive_trimmer(
	item this_item,
	item_cost this_cost,
	item parent_item,
	item_table *t,
	const item_costset & iin
)
{
	rule * this_rule;
	symbol * parent_symbol;
	symbol * this_lhs;
	item_costset_iterator i_iter;
	item_table::subgraph_part this_sbgpart;
	ruletree * this_other_subtree;

	item 	  other_item;
	item 	  other_parent;
	item_cost other_cost;
	rule *	  other_rule;


	this_rule = t->item_rule( parent_item );
	parent_symbol =  t->item_subtree( parent_item )->node;
	this_lhs = this_rule->lhs();
	this_sbgpart = t->item_subpart( this_item );
	this_other_subtree = t->item_subtree( parent_item );
	this_cost += this_rule->principle_cost();
	i_iter = iin;
	if ( parent_symbol->type() == symbol::binaryop ){
		switch (this_sbgpart){
		case item_table::leftpart:
			// other is on the right;
			this_other_subtree = this_other_subtree->right;
			break;
		case item_table::rightpart:
			// other is on the left;
			this_other_subtree = this_other_subtree->left;
			break;
		default:
			assert(0);
		}
		if ( this_other_subtree->node->type() != symbol::nonterminal ) {
			// too interesting for us -- don't even try
			return 1;
		}
	}
	while( (other_item = i_iter.next( &other_cost ) ) != item_none ){
		if ( other_item == this_item ) 
			continue;
		if (  t->item_subpart(other_item) != this_sbgpart ){
			continue;
		}
		other_rule  = t->item_rule(other_item);
		other_cost += other_rule->principle_cost();
		if ( other_cost >= this_cost ){
			// don't even do the hard stuff.
			// not worth it.
			// ( if costs will be equal, this is an
			//   ambiguity, and should be reported as such )
			continue;
		}
		other_parent = t->item_parent( other_item );
		if (
		    t->item_isfullmatch( other_parent )
		 && t->item_subtree( other_parent )->node == parent_symbol
		 && other_rule->lhs() == this_lhs
		 ){
			// same parent symbol
			// same side (left/right) of the symbol,
			// same lhs.
			//
			// we are either unary, in which case we
			// can immediately do a comparison,
			// or we are binary, and need to compare
			// the other branch. The only thing we'll
			// accept at first is an identical
			// nonterminal. You can imagine doing a more
			// interesting structural comparison, but
			// I don't see the value in it (yet).

			if ( parent_symbol->type() == symbol::binaryop ){
				ruletree * other_other_subtree;
				other_other_subtree = t->item_subtree( other_parent );
				switch (this_sbgpart){
				case item_table::leftpart:
					// other is on the right;
					other_other_subtree = other_other_subtree->right;
					break;
				case item_table::rightpart:
					// other is on the left;
					other_other_subtree = other_other_subtree->left;
					break;
				default:
					assert(0);
				}
				if ( other_other_subtree->node != this_other_subtree->node ){
					continue; // not the same thing
				}
			}
			// cost is less.
			// structural same.
			// report it.
			return 0;

		}

	}
	// found no contradictions. Must be cheapest
	return 1;
}

/*
 * construct a list of all items "c" such that
 * "a" is a member of itemset a, and
 * "b" is a member of itemset b, and
 * "a" and "b" are both (partial) matches of the same rule:
 * ( so "a" is [ x -> alpha <something> beta; ca ] and
 *     "b"  is [ x -> alpha <something> beta; cb ] with
 *  the same corresponding parts, excepting cost ),
 * and "c" has the same rule and (partial) match and a cost
 * which is the sum of the costs of "a" and "b"
 * ( thus "c" is [ x -> alpha <something> beta; ca+cb ] ).
 */

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
 *
 */

item_costset
itemset_trim( const item_costset & iin )
{
	item_costset iout(iin.pattab());
	item_costset_iterator i1;
	item_costset_iterator i2;
	item cook1, cook2;
	item_table *t = iin.pattab();
	symbol *s1;
	item_cost cook1_cost, cook2_cost;
	item_cost total1_cost, total2_cost;
	rule  *   rp1, * rp2;
	unsigned int lowcost;
	int is_cheapest;

	lowcost = item_cost_none;

	i1 = iin;
	// look for duplicate full matches.
	// BRUTE FORCE APPLIED HERE!!
	while( (cook1 = i1.next(&cook1_cost) ) != item_none){
		is_cheapest = 1; // i.e. ip1 is the cheapest seen so far.
		if ( t->item_isfullmatch(cook1) ){
			rp1 = t->item_rule(cook1);
			total1_cost = cook1_cost + rp1->principle_cost();
			s1 = rp1->lhs();
			i2 = iin;
			while( (cook2 = i2.next( &cook2_cost ) ) != item_none ){
				if ( t->item_isfullmatch(cook2) ){
				    rp2 = t->item_rule(cook2);
				    if (rp2->lhs() == s1){
					// two items, two fullmatches, one lhs.
					total2_cost = cook2_cost + rp2->principle_cost();
					if ( total2_cost < total1_cost ){
						is_cheapest = 0; // ip1 not cheapest.
						break; // out of while(cook2) loop.
					}
				    }
				}
			}
			// else it is a full match which is cheapest: fall through.
		} else {
			item parent1;

			parent1 = t->item_parent( cook1 );
			if ( t->item_isfullmatch( parent1 ) ){
				// parent is a fullmatch. Look further.
				is_cheapest = predictive_trimmer( cook1, cook1_cost,
				    parent1, t, iin );
			}
		}
		if (!is_cheapest){
			// this is not the cheapest
			// of its sort. Do not add it to the output set.
			// i.e. trim it.
			if ( DEBUG(TRIMMERS) ){
				printf("trimming same-goal: ");
				t->item_print( cook1 );
				printf(" from itemset\n");
			}
			continue; // to next element of while(ip1)
		}
		// add cheapest full matches, and
		// always add partial matches.
		// find "baseline" cost by which to normalize the whole thing, too.
		if (cook1_cost  < lowcost){
			lowcost = cook1_cost;
		}
		iout.cost_add( cook1, cook1_cost );
	}
	if (( 0 < lowcost ) && ( lowcost != item_cost_none ) ){
		// normalize by negative cost of cheapest item.
		// ( but don't bother if its zero )
		// ( and don't bother if its infinite -- empty set
		if ( DEBUG(TRIMMERS) ){
			printf("trim: normalizing itemset by %d\n", lowcost);
		}
		iout.cost_bias( -lowcost );
	}
	return iout;
}

/*
 * "itemset_minselect_generalize"
 * This routine "generalizes" and itemset. That is, it performs what we
 * might refer to as "transitive closure", given the itemset at hand and
 * the set of rules given in the input, but keeping cost in view.
 * 
 * For each item in the itemset that represents a complete match of the
 * form:
 *      [ X -> < rhs > ; c1 ]
 * where the rule matched has cost c2, look at all rules having X in the
 * rhs, and construct all possible items of the form
 *      [ Y -> alpha < X > beta ; c1+c2 ]
 * (This is the generalization.) Look up this match in the itemset. If
 * there's no item having the same ( partial ) match, then add this item.
 * If there is such an item, but of cost c3 > c1+c2, then replace that
 * item in the itemset with this one.  But if there is already an item of
 * cost c3 <= c1+c2, do not add this item.  (This is the min-selection.)
 * 
 * When all items have been looked at, if any new items were added, then
 * repeat the entire process.
 */

/*
 * generalize_symbol is the inner loop: once we've found a full match
 * we pass the incurred cost and the lhs symbol to generalize_symbol,
 * which constructs items and looks them up. minselect_generalize
 * is the outer loop.
 */

static inline void
generalize_symbol( symbol *s, int cost_so_far, item_costset * Newset )
{

	Newset->cost_union( s->get_closure(), cost_so_far );
}

static item_costset
itemset_minselect_generalize( const item_costset & Input )
{
	item_cost cost;
	item_costset_iterator Ilist;
	item olditem;
	item_table *t = Input.pattab();
	item_costset result(t);

	Ilist = Input;
	result = Input.copy();
	while( ( olditem = Ilist.next( &cost ) ) != item_none ){
		if ( t->item_subpart(olditem) == item_table::fullmatch ){
			/* found a full match. 
			 * find all the places the lhs symbol
			 * appear in any rule, and consider
			 * adding those partial matches to 
			 * Inew.
			 */
			cost += t->item_rule( olditem )->principle_cost();
			generalize_symbol(
				    t->item_rule( olditem )->lhs(),
				    cost,
				    &result );
		}
	}
	return result;
}

/* "combine" function for empty state */
state *
state::null_state( item_table* itp )
{
	if (nullstate==0){
		nullstate = state::newstate( item_costset(itp), 0 );
		assert( nullstate->number == NULL_STATE );
	}
	return nullstate;
}

/* "combine" function for leaves */
state *
state::combine( terminal_symbol *s )
{
	item_costset tempa;
	state * v;
	tempa = allmatch( s );
	v = state::newstate( tempa, s );
	// tempa is not ours to destruct.
	// it belongs to s.
	return v;
}

/* "combine" function for unary operators */
state *
state::combine( unary_symbol *s, unsigned l_stateno )
{
	item_costset tempa;

	tempa = s->lhs_sets.get( l_stateno );
	if ( tempa.is_empty() )
		return nullstate;
	return state::newstate( tempa, s );
	// tempa is not ours to destruct.
	// it belongs to s.
}

/* "combine" function for binary operators */
state *
state::combine( binary_symbol *s, unsigned l_stateno, unsigned r_stateno )
{
	item_costset tempa, tempb, tempc;
	state * v;
	tempa = s->lhs_sets.get( l_stateno );
	if ( tempa.is_empty() )
		return nullstate;
	// tempa is not ours to destruct.
	// it belongs to s.
	tempb = s->rhs_sets.get( r_stateno );
	if ( tempb.is_empty() )
		return nullstate;
	// tempb is not ours to destruct, or to intersect into.
	// it belongs to s.
	tempc = tempb.copy();
	tempc.cost_intersect( tempa, 0);
	v = state::newstate( tempc, s );
	tempc.destruct();
	return v;
}

/*
 *  Stuff for making new states. States are kept in a pool, which is
 *  an array of linked lists of states. The array is indexed by some
 *  function of the state. We'll first try the rule number of the
 * first item on the itemset. Since itemsets are sorted by something
 * (rule number?) the first one should always be the same for two
 * itemsets that are the same.
 * We'll also keep a statelist of states in ascending numerical order.
 */

class statelist allstates; // the list of all states in ascending order.
class state * nullstate = 0;

static unsigned nlists = 0;
static state ** statelists = 0;

class state *
state::newstate( const item_costset & ilist, const symbol * syp )
{
	item_costset_iterator iter;
	item_costset	temp_set;
	item ip;
	item_cost c;
	int ruleno;
	state * sp;
	/*
	 * compare this state with all that have gone before.
	 */
	// getting the first rule number is suprizingly awkward.
	temp_set = itemset_trim( ilist );
	iter = temp_set;
	ip = iter.next( &c );
	ruleno = (ip==item_none) ? 0 : temp_set.pattab()->item_rule( ip )->ruleno();
	if (nlists== 0){
		// first time through
		nlists = all_rules.n()+1; // 0th state is null!!
		statelists = (state **)calloc( nlists, sizeof(state *) );
		assert(statelists!=0);
	} else {
		// look for preexisting state.
		sp = statelists[ ruleno ];
		while (sp != 0){
			if (item_costset_equal( sp->core ,  temp_set ) ){
				temp_set.destruct();
				return sp;
			} else {
				sp = sp->next;
			}
		}

	}
	// No such state currently exists. Make up a new one.
	sp = new state;
	sp->number = allstates.n();
	sp->core = temp_set;
	sp->symp = syp;
	sp->rules_to_use = 0;
	temp_set = itemset_minselect_generalize( sp->core );
	sp->items = itemset_trim( temp_set );
	temp_set.destruct();
	// put new state on lists.
	sp->next = statelists[ ruleno ];
	statelists[ ruleno ] = sp;
	allstates.add( sp );
	// debug printing
	if (DEBUG(NSTATES) ){
		printf("New state: #%d has %d core, %d total items\n",
		    sp->number, sp->core.n(), sp->items.n() );
		fflush(stdout);
	}
	if (DEBUG(DUMPTABLES) ){
		sp->print( verbose );
		fflush(stdout);
	}

	return sp;
}

static void
ambiguous_state( state * stp, symbol * goal_p )
{
	item_costset_iterator c_i;
	item pc;
	item_cost cst;
	item_table *t;
	int firstrule = -1;
	wordlist example;

	fprintf( stderr, "Warning: grammar is ambiguous (state %d)\n", stp->number );
	fprintf( stderr, "	cannot tell which of these rules to use to get a %s:\n", 
		 goal_p->name() );
	c_i = stp->items;
	t = stp->items.pattab();

	while( (pc = c_i.next( &cst ) ) != item_none ){
		if ( t->item_isfullmatch( pc ) 
		    && t->item_rule( pc )->lhs() == goal_p ){
			fputs("\t", stderr);
			t->item_rule( pc )->print(stderr, false);
			if (firstrule < 0 )
				firstrule = t->item_rule( pc )->ruleno();
		}
	}
	example = invert_state( stp );
	fprintf( stderr,"\texample ambiguous input tree: ");
	print_wordlist( &example, stderr );
	example.destruct();

	fprintf( stderr, "\nUsing rule %d\n\n", firstrule );
}

/*
 * Compute the rules_to_use vector.
 */
void
state::use_rules()
{
	int *   rp;     // the output vector, of length nonterms.n().
	int nfullmatch;
	symbol *sp;
	symbollist_iterator s_iter;
	item   pc;
	item_table *  t;
	item_costset_iterator i_iter;
	item_cost     cst;

	rules_to_use = (int *) malloc( nonterminal_symbols.n() * sizeof(int) );
	assert( rules_to_use != 0 );
	rp = rules_to_use;

	s_iter = nonterminal_symbols;
	while ( (sp = s_iter.next() ) != 0) {
		// for all goals...
		nfullmatch = 0;
		i_iter = items;
		t = items.pattab();
		while ( (pc = i_iter.next(&cst) ) != item_none ){
			// find a full match resulting in that goal.
			if ( t->item_rule(pc)->lhs() == sp
			  && t->item_isfullmatch(pc) ){
				// and record its rule number
				if (nfullmatch == 0 ){
					*(rp++ ) =
					    t->item_rule(pc)->ruleno();
					t->item_rule(pc)->set_reached(true);
				} else{
					ambiguous_state( this, sp );
				}
				nfullmatch += 1;
			}
		}
		if ( nfullmatch == 0 ){
			*(rp++ ) = 0 ; // oops.
		}
	}
}

/*
 * print out a state, print out all states.
 */

void
state::print(int verbose, FILE * out)
{
	fprintf( out, "%d core: (", number );
	core.print(out);
	fprintf(out, ")\n");
	if (verbose >= 2 ){
		// print closure set.
		fprintf(out, "%d closure: (", number );
		items.print(out);
		fprintf(out, ")\n");
	}

}

void
printstates(int print_closure_sets)
{
	statelist_iterator s_i;
	state *sp;
	s_i = allstates;
	printf("\nSTATES:\n");
	while( (sp = s_i.next() ) != 0){
		sp->print(!print_closure_sets);
	}
	fflush(stdout);
}

void
printstateinversion()
{
	statelist_iterator s_i;
	state *sp;
	wordlist example;
	static int maxprinted=-1;
	int stateno;

	s_i = allstates;
	fflush(stderr);
	printf("\nSTATE INPUT EXAMPLES:\n");
	while( (sp = s_i.next() ) != 0){
		stateno = sp->number;
		if ( stateno <= maxprinted ) 
			continue;
		example = invert_state( sp );
		printf( "state %d example input tree:\n\t", stateno );
		print_wordlist( &example, stdout );
		example.destruct();
		printf("\n");
	}
	maxprinted = stateno;
	fflush(stdout);
}

// find the state (if there is one) that has zero items
// useful for folding with other states during compression

int
find_error_state(void)
{
	return NULL_STATE;
}
