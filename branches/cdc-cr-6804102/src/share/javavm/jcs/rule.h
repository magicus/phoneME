/*
 * @(#)rule.h	1.13 06/10/10
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

#ifndef __RULE_H__
#define __RULE_H__
#include "POINTERLIST.h"
#include <stdio.h>
class symbol;
class wordlist;

/*
 * This file declares these classes:
 *	ruletree
 *	rule
 *	rulelist
 *	rulelist_iterator
 */


/*
 * A ruletree is a little binary tree which
 * is used to represent the body of a rule.
 * A ruletree is built in rule::make, and installed
 * in the rule. Ruletrees are used with paths to
 * form items, thence states. See state.C for common usage.
 * The arity of a ruletree node is implied by the type of
 * the symbol referenced there.
 */
struct ruletree{
	symbol * node;
	struct ruletree * right, *left;
};


/*
 * "rule"
 * This is the internal representation of the rules
 * given busbuild in the input file. A rule is built
 * by a call to rule::make from the parser. It has
 * one component for each component of its input
 * specification, plus a number by which the rule can
 * be referred in the pmatch program produced,
 * and in debugging output.
 *
 * There is also the head of a linked list of items
 * representing matches or partial matches to this
 * rule. I believe this is the fastest way to check for
 * the existence of an item (which are unique). See
 * item::newitem(), which should be the only place
 * this is used.
 */
class rule{
	/*
	 * the components of the rule, corresponding
	 * to the components of the rule during input
	 */
	symbol   *	lhs_symbol;
	ruletree *	rhs_tree;
	int		cost;
	char *		action;
	char *		synthesis_action;
	char *		pre_action;
	wordlist *	macros_list;
	int		number;
	int		arity; // number of non-terminals in rhs_tree
	int		rule_is_reached;
	int		dag_rule;
public:
	/* 
	 * To add a new rule to the dictionary.
	 * It turns the lhs name into a symbol,
	 * turns the rhs wordlist into a ruletree,
	 * and simply installs the other components.
	 * A pointer to the new rule is returned.
	 * The new rule is also added to the rulelist
	 * all_rules, below. Tracing output may be
	 * printed, depending on the debugging flags.
	 * Using "" as a pre_action is in bad taste, as it will cause
	 * rule::has_pre_act to return 1 (i.e. true), when just the
	 * opposite was likely intended, use (char *)0 instead.
	 *
	 * Usage:
	 *	char * lhsname;
	 *	wordlist * rhslist;
	 *	int cost;
	 *	char * c_action;
	 *	rule * rp;
	 *
	 *	rp = rule::make(lhsname, rhslist, cost, c_action,
	 *		(char *)0, (char *)0);
	 */
	static rule * make( char * lhs, wordlist *rhs,
			    int cost, char * code,
			    char *synth_action, char *pre_action,
			    wordlist *macros_list, int is_dag );
	/*
	 * print an existing rule, for debugging purposes.
	 */
	void print(FILE *output = stdout, bool doSummaryOnly = false);
	/*
	 * get a rule's number
	 */
	inline int ruleno(void){ return number; }
	/*
	 * Get the scalar cost of a rule.
	 * NOTE: Could we turn costs into vectors?
	 */
	inline int principle_cost(void){ return cost; }
	/*
	 * find out what symbol is the goal of a given rule
	 */
	inline symbol * lhs(){ return lhs_symbol; }
	/*
	 * Inspect the body of a rule.
	 */
	inline ruletree *rhs(){ return rhs_tree; }
	/*
	 * Inspect a rule's semantic action.
	 */
	inline char *act(){ return action; }
	/*
	 * Gets a rule's macros list.
	 */
	inline wordlist *get_macros_list(){ return macros_list; }

	/*
	 * Discover whether or not a rule has a synthesis_action
	 * Trivial strings, such as "", will not be detected as empty.
	 */
	inline int has_synthesis_act(){ return synthesis_action != 0 ; }
	/*
	 * Inspect the actual synthesis_action.
	 */
	inline char *synthesis_act(){ return synthesis_action; }

	/*
	 * Discover whether or not a rule has a pre_action
	 * Trivial strings, such as "", will not be detected as empty.
	 */
	inline int has_pre_act(){ return pre_action != 0 ; }
	/*
	 * Inspect the actual pre_action.
	 */
	inline char *pre_act(){ return pre_action; }

	/*
	 * retrieve rule's arity
	 */
	inline int get_arity(){ return arity; }

	/*
	 * set/get rule's reachability attribute
	 */
	inline void set_reached(int v){ rule_is_reached = v;}

	inline int  get_reached(){ return rule_is_reached; }

	/*
	 * Inquire as to whether it is marked as a dag rule
	 */
	inline int is_dag(){ return dag_rule; }

	/*
	 * This is the first item, numerically,
	 * numerically, to match this rule. It is always the full
	 * match pattern, since we construct the patterns in this order.
	 */
	int		fullmatch_item;
};

/* yet another instance of a POINTERLIST */
DERIVED_POINTERLIST_CLASS( rulelist, rule *, rulelist_iterator );

/*
 * The list of all rules, in (of course) insertion order = canonical
 * numeric order, I hope.
 */
extern rulelist all_rules;

/*
 * debugging print routine.
 */
void   printrules(void);

#endif
