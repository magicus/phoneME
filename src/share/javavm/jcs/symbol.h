/*
 * @(#)symbol.h	1.8 06/10/10
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

#ifndef __SYMBOLH__
#define __SYMBOLH__
#include "rule.h"
#include "POINTERLIST.h"
#include "item.h"
#include "path.h"
#include "matchset.h"
#include "statemap.h"

/*
 * "symbol" and its derivatives.
 * a symbol is the representation of any named
 * entity -- terminal or nonterminal -- in busbuild.
 * Symbols have the obvious attributs of name and type.
 * Symbols have lists of rules in which they appear as
 * the goal (nonterminals only, we trust), and in which
 * they appear in the rule body (either). Terminal
 * symbols have transition matricies, the dimensionality
 * of which depends on the symbol type. This is achieved
 * using derived classes, given at the end of this file.
 * Like most other things in busbild, symbols can be
 * made into lists, which can be iterated through.
 */

struct leaf_transition;
struct unary_transition;
struct binary_transition;

class symbol{
public:
	/*
	 * The types of symbols. Determines where
	 * a symbol may appear in a rule, and the
	 * dimensionality of its transition table.
	 */
	enum symboltype{
		binaryop,
		unaryop,
		terminal,
		nonterminal,
		rightunaryop };
private:
	/* 
	 * The representation is kept private, but
	 * accessed by the public functions below.
	 * Thus, the type (from the enum above), the
	 * character name, the lists of rules having
	 * this symbol on the rhs (in the body) or on
	 * the lhs (as the goal) respecively,
	 * and the closure.
	 */
	symboltype	stype;
	symboltype	ftype;
	const char *	sname;
	rulelist	rhs_list;
	rulelist	lhs_list;
	item_costset	closure_set;
	item_costset	core_set; // un-closed set of patterns.
	/*
	 * primordial allocation and addition.
	 * pay no attention to the man behind
	 * the curtain...
	 */
	static class symbol * add_symbol( const char * name, symboltype newtype);
public:
	/*
	 * Add a new rule to a symbol's rhs or lhs
	 * lists, respectively, as we see new rules
	 * involving this symbol. A rule is only added
	 * to each list once, as there is a uniqueness
	 * check.
	 * Usage:
	 *	rule * rp;
	 *	symbol * sp;
	 *	sp->add_rhs( rp );
	 */
	void add_rhs( rule * );
	void add_lhs( rule * );
	/*
	 * Attach a closure to the rule. Once only.
	 */
	void add_closure( item_costset c ) { closure_set = c; }
	void add_core_set( item_costset c ) { core_set = c; }

	/*
	 * precomputed state:set mappings when appearing on lhs/rhs,
	 * respectively.
	 * The "matchsets" are temps. The unique_matchsets are for keeps.
	 * and used in conjunction with the statemaps.
	 */
	matchset	lhs_temp_sets;	// may appear under lhs.
	matchset	rhs_temp_sets;	// may appear under rhs.
	unique_matchset	lhs_sets;	// may appear under lhs.
	unique_matchset	rhs_sets;	// may appear under rhs.
	statemap	lhs_map;
	statemap	rhs_map;
	/*
	 * The direct_map_transitions flag is set during output generation
	 * to indicate that the transition table is generated (and should be
	 * accessed) directly, rather than through the indirect vector
	 * lhs_map, as it otherwise would.
	 */
	int		direct_map_transitions;

	/*
	 * inspect the lists of rules in which this
	 * symbol appears in body or as goal, respectively.
	 * Usage:
	 *	rulelist rl;
	 *	rulelist_iterator rl_i;
	 *	symbol *sp;
	 *	rl = sp->rhs();
	 *	rl_i = rl;
	 */
	inline rulelist rhs(void){ return rhs_list; }
	inline rulelist lhs(void){ return lhs_list; }
	/*
	 * Retrieve the symbol's name
	 */
	inline const char *   name(void) const { return sname; }
	/*
	 * Retrieve a symbol's type.
	 */
	inline symboltype type(void) const { return stype; }
	/*
	 * Manipulate a symbol's functional type.
	 * By default this is the same as the symbol's type, but when we find
	 * a degenerate case (only when compressing out error cases), then
	 * a unaryop or binaryop can become functionally a terminal, and
	 * a binaryop can become functionally a (left) unaryop or rightunaryop.
	 */
	inline symboltype functional_type(void) const { return ftype; }
	inline void set_functional_type(symboltype t) { ftype = t; }
	/*
	 * Retrieve the symbol's closure
	 */
	inline item_costset get_closure(void) { return closure_set; }
	inline item_costset get_core_set(void) { return core_set; }

	/*
	 * Instantiate a new symbol, of a given name and
	 * type. Checks first to see if the named symbol
	 * is already present. If it is, then increment
	 * the variable "semantic_error" and fprintf(stderr...)
	 * a warning, then return zero. But in the normal
	 * case, enter a new symbol in the table and 
	 * return a pointer to it. Also checks the symboltype
	 * value, but that should never be a problem, should
	 * it?
	 * N.B. The symbol allocated is not really
	 * a "symbol", but is one of the derived types
	 * below, depending on the symboltype given.
	 */
	static class symbol *
	newsymbol( const char * name, symboltype s);

	/*
	 * Look up a symbol believed to be in the
	 * symboltable, and return a pointer to it.
	 * If the name is not found, then a new symbol is
	 * added, of type symbol::nonterminal, as this
	 * is busbuild's default symbol type.
	 * N.B. The pointer returned is not
	 * to a "symbol", but to one of the derived types
	 * below, depending on the symboltype given.
	 */
	static class symbol * lookup( const char * name );

	/*
	 * Reduce the amount of storage associated with a symbol,
	 * by getting rid of the various bits of intermediate state
	 * associated with it. Can only be done after all state
	 * computation, but may be done before compression. This
	 * allows more swap space for the latter.
	 */
	void shrink(void);
};

/*
 * Specific sorts of symbols. They're really all the same,
 * except for the transition matricies they reference.
 * I suppose I could have pushed this differentiation down
 * to the transition level. But I didn't.
 */

/*
 * Leaf symbols have a pointer to a leaf_transition
 * ( which is itself pretty boring,  being only a
 * scalar c.f.).
 */
struct terminal_symbol: public symbol {
	leaf_transition *	transitions;
};

/* 
 * Nonterminal symbols don't have transitions,
 * but are numbered. This was for the use of the
 * generated code, but we currently make up an
 * enumeration instead, so the goal_number is very
 * little used.
 */
struct nonterminal_symbol: public symbol {
	int	goal_number;
};

/*
 * Unary operator symbols have a pointer to a unary_transition
 * which is a vector.
 */
struct unary_symbol: public symbol {
	unary_transition *	transitions;
};

/*
 * Binary operator symbols have a pointer to a binary_transition
 * which is a matrix.
 */
struct binary_symbol: public symbol {
	binary_transition *	transitions;
};

/*
 * yet another instance of a POINTERLIST
 * see POINTERLIST.h for commentary.
 */

DERIVED_POINTERLIST_CLASS( symbollist, symbol *, symbollist_iterator );

/*
 * The global symbol table. You can iterate through it,
 * but please don't add to it except by way of symbol::newsymbol
 */
extern symbollist all_symbols;
extern symbollist binary_symbols;
extern symbollist nonterminal_symbols;

/*
 * Print the symbol table, for debugging purposes.
 * If with_tables != 0, then transition tables are
 * printed, too.
 */
void printsymbols(int with_tables);

/*
 * Check for symbol-table semantic consistency:
 * Especially, see if there are any "nonterminal"
 * symbols not defined, and warn about them.
 */
void checksymbols();

/*
 * count the symbols of type symbol::nonterminal in the symbol table.
 * Not really very amusing.
 */
extern int  number_of_nonterminals(void);


#endif 
