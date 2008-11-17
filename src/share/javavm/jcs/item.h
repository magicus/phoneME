/*
 * @(#)item.h	1.6 06/10/10
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

#ifndef _item_h_
#define _item_h_

/*
 * Herein are declared
 *	item
 *	item_max
 *	item_none
 *	item_table
 *	item_cost
 *	item_cost_max
 *	item_cost_none
 *	item_costset
 *	item_costset_iterator
 *	sparse_item_costset
 *
 * A "item" is a full or partial match to a rule,
 * but without cost information.
 * A item is what the item and rule manipulators
 * use to describe a full or partial match.
 * A item is the external representation we
 * use for a item. The real bits are buried inside the
 * item table. It is the latter we query about the properties
 * of the patters we are dealing with.
 * A item_costset is a cost item set associated with each nonterminal symbol.
 * A sparse_item_costset is more compact for very sparse sets, as tend
 * to occur in symbol's temp matchsets. They may not have full semantics,
 * but can be converted to item_costset upon demand.
 */

typedef unsigned short item;
const unsigned int item_max = 0xfffe;
const unsigned int item_none       = 0xffff;

typedef unsigned char item_cost;
const unsigned int item_cost_max  = 0xfe;
const unsigned int item_cost_none = 0xff;

/*
 * item table. 
 * The one real residence of information about items.
 * Created once after rule reading, using create,
 * then probed using other routines. A item_table contains
 * all possible partial and full rule matches, given the
 * symbollist argument to create.
 * Outside this table, items are represented by the item.
 */
class path;
class item_body;
class symbollist;

class item_table {
	int		n_items;
	item_body *	items;
public:
	// usual empty constructor...
	item_table(void){ n_items=0; items=0; }

	/*
	 * Creates a item table from a set of rules.
	 * Call just once after all the rules are read and entered
	 * in the list.
	 */
	void create( rulelist * );

	/*
	 * retrieve item index for given path and rule.
	 */
	item use( path p, rule * r );

	/*
	 * print the whole table
	 */
	void print( void );

	/*
	 * Return size of the table.
	 * Same as the value of the greatest valid item + 1.
	 * Important for sizing arrays
	 */
	item n(void){ return n_items; }

	/*
	 * print a specific item
	 */
	void item_print( item, FILE * out  = stdout );

	/*
	 * Answer more specific questions about a item.
	 */
	/*
	 * extract components.
	 */
	rule *		item_rule( item );
	path   		item_path( item );
	ruletree *   	item_subtree( item );
	item	item_parent( item );
		
	/*
	 * determine if a item represents a match of the
	 * full body of a rule, or whether it is on the left
	 * or the right of its parent operator node.
	 * Usage:
	 *	item ptmp;
	 *	item_table  pt;
	 *	if (pt.item_subpart(ptmp) == item_table::fullmatch) ...
	 */

	enum subgraph_part { fullmatch, leftpart, rightpart };

	subgraph_part item_subpart( item );

	int item_isfullmatch( item cook )
	    { return item_subpart(cook)==item_table::fullmatch; }


};

/*
 * A item_costset is a costset of items associated with each nonterminal symbol.
 * When we are building a state, and we find that the given symbol is the
 * lhs of a rule that has been fully matched in this state, then the
 * item_costset of that symbol is merged into the state's item set. The item_costset
 * can be computed on the fly, but can also be precomputed, given a 
 * item_table and a symbol table, and the item_costsets attached to the
 * symbols.
 *
 * The representation of cost set we're using is an array of costs,
 * indexed by item. item_cost_none means the 
 * item is not in the set.
 * Since items are always relative to a item_table, 
 * we embed the table name in the item_costset.
 */


class item_costset {
	friend class item_costset_iterator;
	item_table * item_costset_table; // should be the same for all item_costsets!
	item_cost  * item_costset_p;     // allocated array of item

	// constructor for private use only.
	item_costset( item_table * pt, item_cost * ary )
		{ item_costset_table = pt; item_costset_p = ary; }

	// to count elements if necessary.
	int private_is_empty() const;
public:
	// constructors for public consumption
	item_costset(){ item_costset_table = 0; item_costset_p = 0; }
	item_costset( item_table * ptp )
	    { item_costset_table = ptp; item_costset_p = 0; }

	// how to make one for a specific symbol.
	static class item_costset compute_item_costset( symbol *, item_table *, item_costset * core_set_pointer = 0);

	// extraction of elements. sort of.
	item_table * pattab() const { return item_costset_table; }

	/*
	 * cost_add: put a new item, p, in the item_costset
	 * with cost c, unless it's already in there at lower cost.
	 * Returns:
	 *	0 - was already in the item_costset with cost <= c;
	 *	1 - wasn't : item_costset changed.
	 */
	int cost_add( item p, item_cost c );

	/*
	 * cost_bias: add a constant +/- item_cost c to the cost of
	 * every item in the set. Absent items, and ininstantiated sets
	 * are uneffected.
	 */
	void cost_bias( int c );

	/*
	 * normalize: find the least item_cost, and subtract it from the
	 * cost of all present items. Works in place.
	 */
	void normalize();

	/*
	 * mash two item_costsets together. Is "like" set union, with
	 * the following difference:
	 * 	for the statement a.cost_union( b, z );
	 *	if a contained the item x at cost y,
	 *	and b contained the item x at cost y-z-1,
	 *	then a' will contain x at cost y-1
	 * Or, generally: take the lowest cost of the existing item
	 * and the one in b, biased by the extra cost parameter.
	 */
	void cost_union( const item_costset &, item_cost bias = 0 );

	/*
	 * another way to mash together item_costsets. Is "like" set intersection,
	 * but uses the sum of costs, with the bias cost thrown in.
	 * Thus for the statement a.cost_intersect( b, z ),
	 *	if a contained the item x at cost y,
	 *	and b contained the item x at cost w,
	 *	then a' will contain x at cost w+y+z.
	 */
	void cost_intersect( const item_costset &, item_cost bias = 0 );

	/*
	 * How to count the number of cookies present in one.
	 * This doesn't get called often.
	 */
	int n() const;

	/*
	 * A cheaper test if all you need to know is existance:
	 *     1=> empty set
	 *     0=> non empty set
	 */
	// inline shortcut of common case.
	int is_empty() const
	    {
		return ( item_costset_p == 0 ) ? 1 : private_is_empty();
	    }


	/*
	 * how to copy one.
	 * makes a copy of the array.
	 * Usage:
	 *	item_costset a, b;
	 *	b = a.copy();
	 * Don't forget to distruct copy'd things!
	 */
	item_costset copy( ) const;

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

	item_costset parent_allmatch( item_table::subgraph_part prt, symbol *s ) const;

	/*
	 * simple comparison is pretty trivial.
	 * returns 0 for .ne., 1 for .eq.
	 */
	friend int 
	item_costset_equal( const item_costset &a, const item_costset &b );

	/*
	 * funky comparison: would this set be equal to the other
	 * if both were normalized. Thus ignoring differences of a 
	 * constant cost bias, as this will wash out after minimization.
	 */
	friend int
	item_costset_normalized_equal( const item_costset &a, const item_costset &b );

	// how to get rid of one:
	// only deallocates the allocated array:
	// does not deallocate the item_costset struct itself.
	void destruct();

	// how to print one out.
	void print( FILE * out = stdout ) const;

}; // end class item_costset

/*
 * Iterate over the item's in a item_costset. Use
 * is standard iterator usage, EXCEPT that end-of-iteration is
 * signaled by return of a item of item_none.
 */
class item_costset_iterator{
	item   cli_maxp1;
	item   cli_current;
	item_cost  *  cli_table_p;
public:
	// standard empty constructor.
	item_costset_iterator(){ cli_maxp1 = 0; cli_current = 1; cli_table_p = 0; }

	/*
	 * Setup an iterator by assigning a item_costset to it.
	 * Use:
	 *	item_costset v;
	 *	item_costset_iterator cl_i;
	 *	item cook;
	 *	item_cost   cost;
	 *
	 *	cl_i = v;
	 *	while( ( cook = cl_i.next( &cost ) ) != item_none ){
	 *		use cook and cost
	 *	}
	 */
	void operator=( const item_costset & c )
	    {
		cli_current=0;
		cli_maxp1 = ((cli_table_p = c.item_costset_p) == 0) ? 0
			    : c.item_costset_table->n();
	    }

	// iteration step: see above usage example.
	item next( item_cost * cost_p )
	    {
		    register item_cost   k;
		    register unsigned cur = cli_current;
		    register unsigned maxp1 = cli_maxp1;
		    register item_cost * tabl = cli_table_p;

		    while( cur < maxp1 ){
			    if ( (k=tabl[cur]) != item_cost_none ){
				    *cost_p = k;
				    cli_current = cur+1;
				    return  cur;
			    }
			    cur += 1;
		    }
		    cli_current = cur;
		    return item_none;
	    }

}; // end item_costset_iterator

class sparse_item_costset {
#define SPARSE_ITEM_COSTSET_ARRAY_SIZE	6
	class item_pair { 
	    public:
	    item	ino;
	    item_cost	c;
	};

	item_table *    item_costset_table;
	unsigned	n_pairs;
	unsigned	max_pairs;
	item_pair *	pair_array;
	item_pair	stuff[SPARSE_ITEM_COSTSET_ARRAY_SIZE];

	void private_destruct();

public:
	sparse_item_costset(){ item_costset_table = 0; n_pairs = 0; max_pairs = 0; }
	sparse_item_costset( item_table * ptp )
	    { item_costset_table = ptp; n_pairs = 0; max_pairs = 0; }

	item_table * pattab() const { return item_costset_table; }

	/*
	 * cost_add: put a new item, p, in the item_costset
	 * with cost c, unless it's already in there at lower cost.
	 * Returns:
	 *	0 - was already in the item_costset with cost <= c;
	 *	1 - wasn't : item_costset changed.
	 */
	int cost_add( item p, item_cost c );

	/*
	 * How to count the number of cookies present in one.
	 * This doesn't get called often.
	 */
	int n() const { return n_pairs; }

	/*
	 * A test if all you need to know is existance:
	 *     1=> empty set
	 *     0=> non empty set
	 */
	int is_empty() const
	    { return ( n_pairs == 0 ); }

	/*
	 * You know you really want to see this as a regular item_costset,
	 * so here it is...
	 */
	item_costset get() const;

	/*
	 * free allocated memory, delete all items.
	 */
	void
	destruct(){ if ( max_pairs != 0 ) private_destruct(); };

	// how to print one out.
	void print( FILE * out = stdout ) const;

}; // end sparse_item_costset.
	
	

extern void close_symbols( void );

extern item_table all_items;

#endif
