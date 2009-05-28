/*
 * @(#)item.cc	1.10 06/10/10
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
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "rule.h"
#include "path.h"
#include "item.h"
#include "POINTERLIST.h"

/*****************************************************************
 * formerly the contents of pattern_implementation.{cc,h}.
 * placed here only for the convenience of the optimizing inliner.
 *
 *****************************************************************/
/*
 * This declares the following classes:
 *	item_body
 * A path plus a rule make an item.
 */

int n_setalloc, n_setfree, max_setactive, n_setactive;
int n_sparsealloc, n_sparsefree, max_sparseactive, n_sparseactive;

#define DID_SETALLOC	{ n_setalloc+=1; n_setactive+= 1; if ( n_setactive > max_setactive ) max_setactive = n_setactive; }
#define SETFREE(p) { free ((char*)(p)); n_setfree++; n_setactive--; }

#define DID_SPARSEALLOC(n) { n_sparsealloc += (n); n_sparseactive += (n); if ( n_sparseactive > max_sparseactive ) max_sparseactive = n_sparseactive; }
#define DID_SPARSE_REALLOC(n) DID_SPARSEALLOC( (n)/2)
#define DID_SPARSEFREE(n) { n_sparseactive-=(n); n_sparsefree+=(n); }

class symbol;

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

/*
 * A item represents a full or partial match of a rule,
 * without cost information. It has two representations:
 * (a) the rule + path + other info struct-sort-of-thing,
 * which we call a item_body, and
 * (b) a cookie which we will call either a item or a
 * item: we'll see.
 * "Other info" includes larger matches for partial rule matches.
 */

class item_body {
	path   			p_path;
	rule * 			p_rule;
	ruletree *		p_subtree;
	item		p_parent_match;
public:
	/*
	 * construct a null item.
	 */
	item_body(void) : p_path()
	    { p_rule = 0; p_parent_match = item_none; }

	/*
	 * construct a item from a path and a rule and a subtree
	 */
	item_body( path p, rule * r, ruletree * );

	/*
	 * add parent match to existing item.
	 * Ensures that item is currently without parent.
	 */
	void add_parent_item( item );

	/*
	 * extract components.
	 */
	rule *		item_rule()   { return p_rule; }
	path   		item_path()   { return p_path; }
	ruletree * 	item_subtree(){ return p_subtree; }
	item	item_parent() { return p_parent_match; }
		

	/*
	 * print a item.
	 * Usage:
	 *	item  pt;
	 *	pt.print(stdout);
	 * Prints out partial rule match only: not other information.
	 */
	void print(FILE * out = stdout);

	/*
	 * determine if a item represents a match of the
	 * full body of a rule, or whether it is on the left
	 * or the right of its parent operator node.
	 * Usage:
	 *	item * ptmp;
	 *	if (ptmp->parent_subpart() == item_table::fullmatch) ...
	 */

	item_table::subgraph_part parent_subpart( void );

	/*
	 * The weak comparison functions.
	 * Usage:
	 *	item * ptmp;
	 *	item * ptmp2;
	 *	if ( ptmp->same_match( *ptmp2 ) )...
	 *	if ( ptmp->lessthan_match( *ptmp2 ) )...
	 */
	// a weaker equality than  ==
	inline int same_match ( register const item_body &other ) const
	{
		return( this->p_rule == other.p_rule
		     && this->p_path == other.p_path );
	}

	/*
	 * Precedence for inequality is:
	 * rule pointer (rules are unique, of course),
	 * then path.
	 */
	inline int lessthan_match ( register const item_body &b ) const
	{
		register int result;
		if ( this->p_rule == b.p_rule ){
			result = this->p_path < b.p_path;
		} else {
			result = this->p_rule < b.p_rule;
		}
		return result;
	}
}; // end class item_body

ruletree*
follow_path( class path &p, ruletree *rtp )
{
	int n;
	int bits;

	for( n=1, bits=p.path_bits; n<= p.path_length; n+=1, bits>>=1)
		if ((bits&1) == p.left)
			rtp = rtp->left;
		else
			rtp = rtp->right;
	return rtp;
}

void
path::print(FILE * out) const
{
	int n;
	int bits;

	n = path_length;
	bits = path_bits;
	while( n-->0){
		switch( bits&1){
		case left:  putc('L', out); break;
		case right: putc('R', out); break;
		default:    putc('X', out); break;
		}
		bits >>=1;
	}
	fflush( out );
}

static void
print_tree( struct ruletree * t, path match_path, path cur_path, FILE * out )
{
	symbol * s = t->node;
	int /*Boolean*/ submatch;
	path left_path;


	submatch = match_path == cur_path;
	left_path = cur_path;
	left_path.descend( left_path.left );
	if (submatch)
		fputs("< ", out);
	fprintf(out, "%s ", s->name() );
	switch( s->type() ){
	case symbol::terminal:
	case symbol::nonterminal: 
		// done
		break;
	case symbol::unaryop:
		// print subtree
		print_tree( t->left, match_path, left_path, out );
		break;
	case symbol::binaryop:
		// print subtrees
		print_tree( t->left, match_path, left_path, out );
		cur_path.descend( cur_path.right );
		print_tree( t->right, match_path, cur_path, out );
		break;
	}
	if (submatch)
		fputs("> ", out);
}

void
item_body::print( FILE* out )
{
	path z;
	path p = p_path;
	rule * r = p_rule;
	fprintf( out, "[%3d] %s -> ", r->ruleno(), r->lhs()->name());
	print_tree( r->rhs(), p, z, out );
}

item_body::item_body( path p, rule * r, ruletree * t )
{
	p_path = p;
	p_rule = r;
	p_subtree = t;
	p_parent_match = item_none;
}

void
item_body::add_parent_item( item x )
{
	assert( p_parent_match == item_none);
	assert( x <=item_max );
	p_parent_match = x;
}

item_table::subgraph_part
item_body::parent_subpart()
{
	switch( p_path.last_move() ){
	case path::left:
		return item_table::leftpart;
	case path::right:
		return item_table::rightpart;
	case path::XX:
		return item_table::fullmatch;
	default:
		assert(0);
	}
}
/*
 * end of inlined implementation stuff..
 */
/*****************************************************************/

DERIVED_POINTERLIST_CLASS( plist, item_body *, plist_iterator );

/*
 * item table. 
 * The one real residence of information about items.
 * Created once after rule reading, using create,
 * then probed using other routines. A item_table contains
 * all possible partial and full rule matches, given the
 * rulelist argument to create.
 * Outside this table, items are represented by the item.
 */
item_table all_items;

/*
 * print a item table
 * for debugging 
 */
void
item_table::print()
{
	register int n, max;
	register item_body * p;
	item parent;
	char matchchar;

	max = n_items-1;
	p = items;
	for( n=0; n<=max; n++){
		printf("%d)\t", n );
		p[n].print();
		parent = p[n].item_parent();
		switch ( p[n].parent_subpart() ){
		case fullmatch:
			matchchar = 'M';
			break;
		case leftpart:
			matchchar = 'L';
			break;
		case rightpart:
			matchchar = 'R';
			break;
		default:
			matchchar = 'X';
			break;
		}
		printf("\t%c", matchchar);
		if ( parent != item_none )
			printf(" %d", parent );
		putchar('\n');
	}
	fflush(stdout);
}

/*
 * print a specific item
 */
void 
item_table::item_print( item c, FILE * out  )
{
	assert( c < (unsigned)n_items );
	items[c].print(out);
}

/*
 * Answer more specific questions about a item.
 */

/*
 * extract components.
 */
rule *
item_table::item_rule( item c )
{
	assert( c < (unsigned)n_items );
	return items[c].item_rule();
}

ruletree *
item_table::item_subtree( item c )
{
	assert( c < (unsigned)n_items );
	return items[c].item_subtree();
}

path   		
item_table::item_path( item c )
{
	assert( c < (unsigned)n_items );
	return items[c].item_path();
}

item	
item_table::item_parent( item c )
{
	assert( c < (unsigned)n_items );
	return items[c].item_parent();
}
	
/*
 * determine if a item represents a match of the
 * full body of a rule, or whether it is on the left
 * or the right of its parent operator node.
 * Usage:
 *	item ptmp;
 *	item_table  pt;
 *	if (pt.item_subpart(ptmp) == item_table::fullmatch) ...
 */

item_table::subgraph_part 
item_table::item_subpart( item c )
{
	assert( c < (unsigned)n_items );
	return items[c].parent_subpart();
}

/*
 * Create a item_table by
 * 1) reading each rule and creating all possible matches
 *    and submatches.
 * 2) Constructing parental relationships between them.
 * 3) Attaching information to each rule giving its relations
 *    to the table created, especially a item_costset table attached
 *    to its lhs symbol.
 * A slightly more clever program might be able to do all three in one
 * pass. If performance of this code becomes an issue, we could become
 * more clever.
 */

/*
 * 1) Routines to make a list of all matches for each rule. and
 * 2) Construct parential relationships, almost as a side effect.
 */

static void
find_submatch(
	rule *r,
	path p,
	ruletree *t,
	item parent_cookie,
	item * cur_cookie_p,
	plist *v )
{
	item_body * pbp;
	item my_cookie;
	path rpath;

	// register this match.
	// it is important here that v->add have ADD AT END semantics,
	// otherwise our indexing strategy is hopelessly flawed!

	pbp = new item_body( p, r, t );
	my_cookie = *cur_cookie_p;
	*cur_cookie_p += 1;
	if ( parent_cookie == item_none )
		r->fullmatch_item = my_cookie;
	else
		pbp->add_parent_item( parent_cookie );
	v->add( pbp );

	// descend to find further submatches.
	switch( t->node->type() ){
	case symbol::binaryop:
		rpath = p;
		rpath.descend( rpath.right );
		find_submatch( r, rpath, t->right, my_cookie, cur_cookie_p, v );
		// FALLTHROUGH
	case symbol::unaryop:
		p.descend( p.left );
		find_submatch( r, p, t->left, my_cookie, cur_cookie_p, v );
		break;
	case symbol::terminal:
	case symbol::nonterminal:
		// all done
		break;
	}
}

static void
find_all_submatches( rule *r, plist * v, item * cur_cookie_p )
{
	path p;
	find_submatch( r, p, r->rhs(), item_none, cur_cookie_p, v );
}


static item_body *
make_match_list( rulelist *rlp, int * n_pb_p )
{
	rulelist_iterator rl_i = *rlp;
	rule * rp;
	item_body * pbp;	// the collection of item's we're building
	item_body * pp;
	int i, n_pb;
	item si = 0;
	plist all_pattrns;
	plist_iterator pl_i;

	// for every rule 
	while ( (rp = rl_i.next() ) != 0 ){
		// make all possible item_bodies
		// and add them to my list.
		find_all_submatches( rp, &all_pattrns, &si );
	}
	// number of item bodies is ...
	n_pb = all_pattrns.n();
	assert(n_pb == si );
	* n_pb_p = n_pb;

	// turn my un-indexable list into a table.
	// remember: list was add-at-end, so cookies should work for indexing.
	pbp = (item_body*)malloc( all_pattrns.n()*sizeof(item_body) );
	assert( pbp != 0 );
	i = 0;
	pl_i = all_pattrns;
	while( (pp = pl_i.next() ) != 0 ){
		pbp[i] = *pp;
		delete pp;
		i += 1;
	}
	assert( i == n_pb );
	all_pattrns.destruct();
	return pbp;
}

void
item_table::create( rulelist * rlp )
{
	items = make_match_list( rlp, & n_items );
	assert( n_items > 0 );
	assert( n_items <= item_max );
}

void
printitems(){
	puts("\nITEM TABLE");
	all_items.print();
}

void
item_setup(){
	all_items.create( &all_rules );
}

/*
 * As a side effect of creating the items, we also attached to each
 * rule a cookie giving the name of the first item involving that rule.
 * We will use this fact in finding, for each symbol
 * the list of places it occure in any rule rhs. We will also use
 * find_instance, I suspect.
 */

void
item_costset::print( FILE * out ) const
{
	register item cookie_max;
	register item i;
	register item_cost * p = item_costset_p;

	if ( item_costset_p == 0 ){
		// empty set.
		return;
	}
	cookie_max = item_costset_table->n()-1;
	for( i = 0; i <= cookie_max; i++ ){
		if ( item_costset_p[i] != item_cost_none ){
			fputs( "\t[ ", out);
			item_costset_table->item_print( i, out );
			fprintf( out, " ; %d ]\n", item_costset_p[i] );
		}
	}
}

static int
find_instance(
	item_cost * item_costset_set,
	symbol * symp,
	rule *r,
	item_table * ptp,
	item_cost c )
{
	item v;
	item v_max = ptp->n()-1;
	int changed; // Boolean;
	ruletree * rtp;
	/*
	 * for every item involving this rule,
	 * see if the subtree matched is rooted by symbol s.
	 * If it is, then add it to the item_costset_set, using cost c,
	 * so long as the set doesn't already contain this cookie with
	 * equal or lower cost.
	 */
	changed = 0;
	for( v = r->fullmatch_item;
	    v <= v_max && ptp->item_rule(v) == r;
	    v++ ){
		// if it's already present and cheaper, don't bother.
		if ( item_costset_set[v] <= c )
			continue;
		rtp = ptp->item_subtree(v);
		if ( rtp->node == symp ){
			// found one. add.
			item_costset_set[v] = c;
			changed += 1;
		}
	}
	return changed;
}

static item_cost *
allocate_item_costset_space( unsigned n )
{
	item_cost * item_costset_set;
	item_costset_set = (item_cost*)malloc( n*sizeof(item_cost) );
	assert( item_costset_set != 0 );
	DID_SETALLOC;
	return item_costset_set;
}

static void
free_item_costset_space( item_cost * item_costset_set )
{
	SETFREE( item_costset_set );
}

static item_cost *
allocate_item_costset_array( unsigned n )
{
	unsigned i;
	item_cost * item_costset_set;

	item_costset_set = allocate_item_costset_space(n);
	for( i = 0 ; i < n ; i ++ )
		item_costset_set[i] = item_cost_none;
	return item_costset_set;
}

item_costset
item_costset::compute_item_costset(
	symbol * symp,
	register item_table * ptp, 
	item_costset * core_set_p )
{
	rulelist_iterator	rl_i;
	rule *			r;
	item		i;
	item		cookie_max;
	item_cost *		item_costset_set;
	int			changed; // Boolean
	unsigned		sum_cost;
	int			n_instances;

	// allocate a item_costset array: fill it with absent elements.
	cookie_max = ptp->n()-1;
	item_costset_set = allocate_item_costset_array( ptp->n() );

	/*
	 * start the item_costset_set with all items in which the symbol
	 * appears on the rhs.
	 */
	rl_i = symp->rhs();
	n_instances = 0;
	while( (r = rl_i.next() ) != 0 ){
		n_instances +=
		    find_instance( item_costset_set, symp, r, ptp, 0 );
	}

	if ( n_instances == 0 ){
		/*
		 * this is very boring: there is nothin in the core
		 * set. Give back the storage. Return smaller,
		 * empty sets.
		 */
		free_item_costset_space( item_costset_set );
		if ( core_set_p != 0 ){
			*core_set_p = item_costset( ptp, 0 );
		}
		return item_costset( ptp, 0 );
	}

	if ( core_set_p != 0 ){
		item_cost * core_p = allocate_item_costset_space( ptp->n());
		memcpy(core_p, item_costset_set, ptp->n()*sizeof( item_cost ));
		*core_set_p = item_costset( ptp, core_p );
	}

	changed = 1;
	while( changed ){
		changed = 0;
		/*
		 * for every item in the item_costset_set corresponding
		 * to a fullmatch of a rule, r,
		 * add to the set all items where r's lhs appear
		 * in the (new) item's rhs. Cost is cost of matching r
		 * plus r's cost.
		 * Of course, consider that the set is changed only
		 * if (a) the new item wasn't in the set, or
		 * (b) if it was but at greater cost.
		 */
		for ( i = 0; i <= cookie_max; i++ ){
			if (item_costset_set[i] != item_cost_none
			    && ptp->item_subpart(i)==item_table::fullmatch){
				r = ptp->item_rule(i);
				sum_cost = item_costset_set[i]+r->principle_cost();
				assert( sum_cost <= item_cost_max );
				symp = r->lhs();
				rl_i = symp->rhs();
				while( (r = rl_i.next() ) != 0 ){
					changed += find_instance(
						item_costset_set,
						symp,
						r,
						ptp,
						sum_cost );
				}
			}
		}
	}
	return item_costset( ptp, item_costset_set );
}

/*
 * Manipulate item_costsets:
 * first: add a new item with cost to a item_costset.
 */
int
item_costset::cost_add( item cook, item_cost c )
{
	if ( c == item_cost_none )
		return 0; // not the best move.
	if (item_costset_p == 0 ){
		// instantiate it.
		item_costset_p = allocate_item_costset_array( item_costset_table->n() );
	}
	assert( cook < item_costset_table->n() );
	if ( item_costset_p[cook] == item_cost_none
	||   item_costset_p[cook] > c ){
		item_costset_p[cook] = c;
		return 1;
	} else {
		return 0;
	}
}


/*
 * cost_bias: add a constant +/- item_cost c to the cost of
 * every item in the set. Absent items, and ininstantiated sets
 * are uneffected.
 */
void 
item_costset::cost_bias( int c )
{
	item i, cookie_max = item_costset_table->n()-1;
	item_cost * cp;

	if ( c == 0 )
		return;
	if (item_costset_p == 0 )
		return;
	for( i = 0, cp = item_costset_p ; i <= cookie_max ; i++, cp++ ){
		if ( *cp != item_cost_none ){
			*cp += c;
		}
	}
}

/*
 * normalize: find the least item_cost, and subtract it from the
 * cost of all present items. Works in place.
 */
void 
item_costset::normalize()
{
	unsigned ncosts;
	int lowcost = item_cost_none;
	register unsigned i;
	register item_cost * cp;
	register int v;

	ncosts = item_costset_table->n();
	cp = item_costset_p;
	if ( cp == 0 )
		return;
	for( i = 0; i < ncosts ; i++ ){
		v = cp[i];
		if ( v < lowcost ){
			lowcost = v;
		}
	}
	if ( lowcost != item_cost_none )
		cost_bias( -lowcost );
}

/*
 * Union together two item_costsets, with cost bias
 */
void
item_costset::cost_union( const item_costset & other, item_cost bias )
{
	unsigned i, maxp1;
	unsigned sumcost;
	int	is_it_empty;

	if ( other.item_costset_p == 0 ){
		// other set empty. Nothing to do.
		return;
	}
	if (item_costset_p == 0 ){
		// instantiate it.
		item_costset_p = allocate_item_costset_array( item_costset_table->n() );
	}
	is_it_empty = 1;
	assert( bias <= item_cost_max );
	maxp1 = item_costset_table->n();
	for ( i = 0; i < maxp1; i++){
		if ( (sumcost=other.item_costset_p[i]) != item_cost_none ){	
			sumcost += bias;
			if ( item_costset_p[i] == item_cost_none
			||   item_costset_p[i] > sumcost ){
				item_costset_p[i] = sumcost;
			}
			is_it_empty = 0;
		} else if (item_costset_p[i] != item_cost_none){
			is_it_empty = 0;
		}
	}
	if (is_it_empty!=0){
		/*
		 * its an empty set.
		 * deallocate storage.
		 */
		free_item_costset_space( item_costset_p );
		item_costset_p = 0;
	}
	return;
}


/*
 * Intersect two item_costsets, with cost bias
 */
void
item_costset::cost_intersect( const item_costset & other, item_cost bias )
{
	unsigned i, maxp1;
	unsigned sumcost;
	int is_it_empty;

	if (item_costset_p == 0 ){
		// this set empty. Intersection is empty.
		return;
	}
	assert( bias <= item_cost_max );
	maxp1 = item_costset_table->n();

	if ( other.item_costset_p == 0 ){
		// other set empty. Intersection is empty.
		free_item_costset_space( item_costset_p );
		item_costset_p = 0;
		return;
	}
	// away we go
	is_it_empty = 1;
	for ( i = 0; i < maxp1; i++){
		if ( (sumcost=other.item_costset_p[i]) == item_cost_none ){	
			// not in other set: not in intersection.
			item_costset_p[i] = item_cost_none;
		} else {
			sumcost += bias;
			if ( item_costset_p[i] != item_cost_none ){
				item_costset_p[i] += sumcost;
				is_it_empty = 0;
			}
		}
	}
	if (is_it_empty != 0 ){
		/*
		 * empty result.
		 */
		free_item_costset_space( item_costset_p );
		item_costset_p = 0;
	}
	return;
}

int
item_costset::n()
const
{
	// count items present in item_costset
	unsigned i, maxp1;
	int nmembers;

	if (item_costset_p == 0 ){
		// empty set
		return 0;
	} else {
		nmembers = 0;
		maxp1 = item_costset_table->n();
		for( i = 0; i < maxp1; i++ )
			if ( item_costset_p[i] != item_cost_none )
				nmembers += 1;
		return nmembers;
	}
}
/*
 * A cheaper test if all you need to know is existance:
 *     1=> empty set
 *     0=> non empty set
 */
int
item_costset::private_is_empty() const
{
	unsigned i, maxp1;

	if (item_costset_p != 0 ){
		maxp1 = item_costset_table->n();
		for( i = 0; i < maxp1; i++ )
			if ( item_costset_p[i] != item_cost_none )
				return 0;
	}
	return 1;
}

item_costset
item_costset::copy()
const
{
	item_costset result( item_costset_table );
	unsigned ncosts;
	if (item_costset_p == 0 ){
		// empty set
		return result;
	} else {
		// allocate array. Bcopy.
		ncosts = item_costset_table->n();
		result.item_costset_p = allocate_item_costset_space( ncosts );
		memcpy(result.item_costset_p, item_costset_p, ncosts*sizeof(item_cost));
		return result;
	}
}

/* 
 * For prt == parent_table::leftpart
 * construct a item_costset of all cookies "b", such that
 *	item "a" is a member of item_costset l;
 *	and  "a" is of the form [ x -> alpha s < treeL > beta ; c ]
 *					for unary symbol s or 
 *				[ x -> alpha s < treeL > treeR  beta ; c ]
 *					for binary s
 *      then "b" is		[ x -> alpha < s treeL > beta; c]
 *			     or [ x-> alpha <s treeL treeR > beta; c ]
 *      correspondingly.
 */

item_costset
item_costset::parent_allmatch(
	item_table::subgraph_part prt,
	symbol *s ) const
{
	register item_table *t = item_costset_table;
	register item_cost * p;
	register item v, vmaxp1;
	item vparent;
	item_cost c;
	item_costset result(t);

	p = item_costset_p;
	if ( p == 0 ){
		// empty set in, empty set out.
		return result;
	}
	vmaxp1 = t->n();
	for( v = 0; v < vmaxp1; v++ ){
		if ( (c = p[v] ) == item_cost_none ){
			// v not in set.
			continue;
		}
		if (t->item_subpart(v) == prt ){
			vparent = t->item_parent( v );
			if ( t->item_subtree(vparent)->node == s )
				result.cost_add( vparent, c );
		}
	}
	return result;
}

int 
item_costset_equal( const item_costset &a, const item_costset &b )
{
	// look for the special cases:
	// expanded and unexpanded empty sets.
	if ( a.item_costset_p == 0 ){
		// "a" is empty.
		return ( b.is_empty() );
	} else if ( b.item_costset_p == 0 ){
		// "b" is empty, and even though "a" is allocated,
		// it might be empty anyway.
		// this case is unlikely.
		return ( a.is_empty() );
	}
	// else neither is empty.
	// do byte-by-byte equality check.
	return memcmp( a.item_costset_p, b.item_costset_p, a.item_costset_table->n()*sizeof(item_cost) ) == 0;
}

/*
 * funky comparison: would this set be equal to the other
 * if both were normalized. Thus ignoring differences of a 
 * constant cost bias, as this will wash out after minimization.
 * This costs lots and lots more than operator==, but if applied
 * earlier in the state building process, may shortcut other
 * more expensive operations.
 */

int
item_costset_normalized_equal( const item_costset &a, const item_costset &b )
{
	// look for the special cases:
	// expanded and unexpanded empty sets.
	// (starts like item_costset_equal, doesn't it?)
	if ( a.item_costset_p == 0 ){
		return ( b.n() == 0 );
	} else if ( b.item_costset_p == 0 ){
		// other is empty
		return ( a.n() == 0 );
	}

	register unsigned i, maxp1;
	register int  bias = item_cost_none;
	register item_cost this_v, other_v;
	register item_cost * this_p, * other_p;
	maxp1 = a.item_costset_table->n();
	this_p = a.item_costset_p;
	other_p = b.item_costset_p;
	// first, compare as much of the set as possible
	// for simple emptyness.
	for (i=0; i<maxp1; i++ ){
		if (this_p[i] != item_cost_none )
			break;
		if (other_p[i] != item_cost_none )
			break;
	}
	if ( i >= maxp1 ){
		// both entirely empty
		return 1;
	}
	// found the first case where one or the other is not missing.
	this_v = this_p[i];
	other_v = other_p[i];
	if ( this_v == item_cost_none || other_v == item_cost_none ){
		// one not missing, but one is missing.
		return 0;
	}
	// neither missing. Compute the bias only once.
	bias = this_v - other_v;

	// continue comparison more carefully.
	for (i=i+1; i<maxp1; i++ ){
		this_v = this_p[i];
		other_v = other_p[i];
		if ( this_v == item_cost_none ){
			if ( other_v == item_cost_none ){
				// element i is missing from both
				continue;
			} else {
				// element i is missing from this
				// but in other: not equal
				return 0;
			}
		} else if ( other_v == item_cost_none ) {
			// element i is missing from other
			return 0;
		} else {
			// make sure cost difference is same.
			if ( this_v - other_v != bias )
					return 0;
		}
	}
	// end of for loop: found no unequal elements
	return 1;
}

// how to get rid of one:
// only deallocates the allocated array:
// does not deallocate the item_costset struct itself.
void
item_costset::destruct()
{
	if ( item_costset_p != 0 ){
		free_item_costset_space( item_costset_p);
		item_costset_p = 0;
	}
}

/*
 * Operations on sparse item cost sets. These are used in 
 * "compute_parental_matchsets", where even the non-empty sets have,
 * generally, < 3 things in them. These are lots smaller, and can
 * easily support the operational subset required.
 */

int
sparse_item_costset::cost_add( item p, item_cost c )
{
	register unsigned	i;
	register item_pair *	pp;

	assert( p < item_costset_table->n() );
	if ( c == item_cost_none )
		return 0; // very dopy thing to do.
	if (max_pairs <= SPARSE_ITEM_COSTSET_ARRAY_SIZE)
		pp = stuff;
	else
		pp = pair_array;
	/*
	 * iterate through table, looking for match.
	 */
	for ( i = 0 ; i < n_pairs ; i ++ ){
		if ( pp[i].ino == p ){
			// found
			if ( c < pp[i].c ){
				pp[i].c = c;
				return 1;
			} else
				return 0; // was already there cheaper.
		}
	}
	/*
	 * didn't find. add at end.
	 */
	if ( max_pairs <= SPARSE_ITEM_COSTSET_ARRAY_SIZE ){
		// allocate at end of builtin array if it fits...
		max_pairs = SPARSE_ITEM_COSTSET_ARRAY_SIZE;
		if ( n_pairs < SPARSE_ITEM_COSTSET_ARRAY_SIZE ){
			// will fit
			pp = &stuff[n_pairs];
		} else { 
			// won't fit. Will allocate and copy.
			max_pairs *=2;
			pair_array = (item_pair *) malloc(
			    max_pairs*sizeof(item_pair) );
			DID_SPARSEALLOC( max_pairs );
			assert( pair_array != 0 );
			memcpy(pair_array, stuff, (SPARSE_ITEM_COSTSET_ARRAY_SIZE)*sizeof(item_pair) );
			pp = &pair_array[n_pairs];
		}
	} else {
		// allocate at end of allocated array if it fits...
		if ( n_pairs >= max_pairs ){
			// must reallocate.
			max_pairs *=2;
			pair_array = (item_pair *) realloc(
			    (char*)pair_array, max_pairs*sizeof(item_pair) );
			DID_SPARSE_REALLOC( max_pairs );
			assert( pair_array != 0 );
		}
		pp = &pair_array[n_pairs];
	} 
	pp->ino = p;
	pp->c = c;
	n_pairs +=1;
	return 1;
}

item_costset
sparse_item_costset::get() const
{
	item_costset v( item_costset_table ); // construct an empty one.
	register unsigned		i;
	register const item_pair *	pp;

	if (max_pairs <= SPARSE_ITEM_COSTSET_ARRAY_SIZE)
		pp = stuff;
	else
		pp = pair_array;
	for ( i = 0 ; i < n_pairs ; i += 1 ){
		v.cost_add( pp[i].ino, pp[i].c );
	}
	return v;
}

void
sparse_item_costset::private_destruct()
{
	if ( max_pairs > SPARSE_ITEM_COSTSET_ARRAY_SIZE ){
		// free allocated memory
		free( (char *)pair_array );
		DID_SPARSEFREE( max_pairs );
	}
	n_pairs = 0;
	max_pairs = 0;
}

void
sparse_item_costset::print( FILE * out ) const
{
	unsigned i;

	for( i = 0; i <= n_pairs; i++ ){
		fputs( "\t[ ", out);
		item_costset_table->item_print( pair_array[i].ino, out );
		fprintf( out, " ; %d ]\n", pair_array[i].c );
	}
}

/****************************************************************************/

void
close_symbols()
{
	symbollist_iterator sl_i = all_symbols;
	symbol * sp;
	item_costset core_set;

	/*
	 * compute item_costset for nonterminals. And while we're at it,
	 * compute itemsets for leaf terminals. Others should have
	 * empty item_costset sets.
	 */
	while( ( sp = sl_i.next() ) != 0 ){
		switch( sp->type() ){
		case symbol::binaryop:
		case symbol::unaryop:
			// build an empty one.
			sp->add_closure( item_costset( &all_items ) );
			break;
		case symbol::terminal:
		case symbol::nonterminal:
			sp->add_closure(
			    item_costset::compute_item_costset( sp, &all_items, &core_set ) );
			sp->add_core_set( core_set );
		}
	}
}

void
print_closures( int print_core_sets )
{
	symbollist_iterator sl_i = all_symbols;
	symbol *s;
	puts("\nCLOSURE SETS");

	while( (s=sl_i.next() ) != 0 ){
		printf("%s: {\n", s->name());
		s->get_closure().print(stdout);
		puts("}");
		if (print_core_sets ){
			printf("core of %s: {\n", s->name());
			s->get_core_set().print(stdout);
			puts("}");
		}
	}
}

void
print_item_costset( const item_costset * cp )
{
	cp->print( stdout );
	fflush(stdout);
}
