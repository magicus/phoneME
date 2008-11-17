/*
 * @(#)symbol.cc	1.10 06/10/10
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

// @(#)symbol.cc       2.16     99/07/14 
#include <stdio.h>
#include <stdlib.h>
#include "symbol.h"
#include "transition.h"

extern int curlineno;
extern char * goalname; // for checksymbols()
extern int semantic_error;

static int n_nonterm = 0;

symbollist all_symbols;
symbollist binary_symbols;
symbollist nonterminal_symbols;

static const char * typenames[]
	= { "binary", "unary", "terminal", "nonterminal", "rightunary" };
static const char * type_image( symbol::symboltype s );

/*
 * primordial addition of symbol to list.
 * To be called only after you know the symbol isn't already
 * there, as we do no checking.
 */
symbol *
symbol::add_symbol( const char * newname, symboltype newtype )
{
	register symbol * sp;

	// first get the right type, and do type-specific processing...
	switch (newtype){
	case symbol::binaryop:
		sp = new binary_symbol;
		((binary_symbol *)sp)->transitions = 0;
		break;
	case symbol::unaryop:
		sp = new unary_symbol;
		((unary_symbol *)sp)->transitions = 0;
		break;
	case symbol::terminal:
		sp = new terminal_symbol;
		((terminal_symbol *)sp)->transitions = 0;
		break;
	case symbol::nonterminal:
		sp = new nonterminal_symbol;
		((nonterminal_symbol*)sp)->goal_number = ++n_nonterm;
		break;
	}
	// now do general processing.
	sp->stype = newtype;
	sp->ftype = newtype; // default functional type is same as declared type.
	sp->sname = newname;
	sp->direct_map_transitions = 0;
	// rhs and lhs lists initialize themselves, we hope.
	// put on end of list
	all_symbols.add( sp );
	if ( newtype == symbol::binaryop )
	    binary_symbols.add( sp );
	else if ( newtype == symbol::nonterminal )
	    nonterminal_symbols.add( sp );
	return sp;
}

/*
 * Do symbol lookup during rule parsing.
 * If a symbol isn't there, it must be a nonterminal,
 * so insert it as such.
 */

symbol *
symbol::lookup( const char * name )
{
	symbollist_iterator s_iter = all_symbols;
	symbol *  sp;

	// look through existing list
	while( (sp = s_iter.next()) != 0){
		if ( sp->name() == name )
			return sp;
	}
	// didn't find it anywhere.
	// add as a nonterminal.
	return  add_symbol( name, symbol::nonterminal );
}

void
symbol::shrink(void)
{
	closure_set.destruct();
	core_set.destruct();
	lhs_sets.destruct();
	rhs_sets.destruct();
}

symbol *
symbol::newsymbol( const char * name, symboltype s )
{
	symbol * sp;
	symbollist_iterator s_iter = all_symbols;

	// this is not the best: make sure s in range
	if ( s < symbol::binaryop || s > symbol::nonterminal ){
		fprintf(stderr,"\nIllegal symbol type %d for symbol %s\n", s, name);
		semantic_error += 1;
		return 0;
	}
	// make absolutely, positively sure its not on the list.
	while ( (sp = s_iter.next() ) != 0){
		if ( sp->name() == name ){
			/* (hint: cannot use type_image twice in arg string) */
			fprintf(stderr,"line %d: attempt to redefine %s from %s to %s\n",
				curlineno-1, sp->name(),
				typenames[sp->type()], typenames[s]);
			semantic_error += 1;
			return 0;
		}
	}
	// now add it in as appropriate
	return add_symbol( name, s );
}


static int
is_on_rulelist( rule * r, rulelist l )
{
	rulelist_iterator ri( l );
	rule * rp;
	while( (rp = ri.next()) != 0){
		if ( r == rp) return 1;
	}
	return 0;
}

/*
 * unique addition to lists. These are pretty non-optimal. If we had
 * to do it more often, we might change to a unique list representation,
 * such as itemset.
 */
void
symbol::add_rhs( rule * r )
{
	if (! is_on_rulelist( r, rhs_list) )
		rhs_list.add( r );
}

void
symbol::add_lhs( rule * r )
{
	if (! is_on_rulelist( r, lhs_list) )
		lhs_list.add( r );
}


void
printsymbols( int with_transitions )
{
	extern void safeprint( const char *, FILE * output = stdout);

	symbollist_iterator si = all_symbols;
	symbol * sp;

	printf("\nSYMBOLS:\n");

	while( (sp = si.next()) != 0 ){
		rulelist_iterator list;
		rule * rp;

		safeprint( sp->name() );
		printf(":\t%s\n", type_image( sp->type() ) );
		printf("\tappears in rhsides: ");
		list = sp->rhs();
		while( (rp = list.next()) != 0){
			printf("%d ", rp->ruleno());
		}
		printf("\n\tappears in lhsides: ");
		list = sp->lhs();
		while( (rp = list.next()) != 0){
			printf("%d ", rp->ruleno());
		}
		printf("\n");
		if ( with_transitions ){
		    generic_transition * tp;
		    switch (sp->type()){
		    case symbol::nonterminal:
		    default:
			    tp = 0;
			    break;
		    case symbol::terminal:
			    tp = ((terminal_symbol *)sp)->transitions;
			    break;
		    case symbol::unaryop:
			    ((unary_symbol *)sp)->lhs_map.print(
				"map_data", sp->name(), "",
				"unary_map", stdout, stdout );
			    tp = ((unary_symbol *)sp)->transitions;
			    break;
		    case symbol::binaryop:
			    ((binary_symbol *)sp)->lhs_map.print(
				"map_data", sp->name(), "",
				"left_binary_map", stdout, stdout );
			    ((binary_symbol *)sp)->rhs_map.print(
				"map_data", sp->name(), "",
				"right_binary_map", stdout, stdout );
			    tp = ((binary_symbol *)sp)->transitions;
			    break;
		    }
		    if ( tp ){
			    tp->print( 
				"","","transition_table",stdout, stdout );
			    printf("\n");
		    }
		}
	}
	fflush( stdout );

}

static const char *
type_image( symbol::symboltype s ){

	static char buf[20];

	if ( s >= symbol::binaryop || s <= symbol::nonterminal ){
		return typenames[s];
	} else {
		// strange value
		sprintf(buf,"(%u)", (unsigned)s );
		return buf;
	}
}

int
number_of_nonterminals(void){
	return n_nonterm;
}

static void
symbol_rule_usage_message(
	const char * fmt, const char * sname, const rulelist & rules )
{
	rulelist_iterator list = rules;
	rule * rp;
	fprintf( stderr, fmt, sname );

	while( (rp = list.next()) != 0){
		fprintf( stderr, "%d ", rp->ruleno());
	}
	fprintf( stderr, "\n");
}

/*
 * Post-parse symantic consistency check.
 * There are only two things here now:
 * 1) nonterminals never defined.
 *    This yields a warning
 * 2) terminals on lhs.
 *    This yields a hard error.
 * 3) goal a terminal or appearing on rhs
 *    A warning, but highly dubious.
 * 4) terminal never used.
 *    A warning, of course.
 */
void
checksymbols()
{
	symbollist_iterator si = all_symbols;
	symbol * sp;
	const char * sname;
	int n_rhs_usage;

	while( (sp = si.next()) != 0 ){
		sname = sp->name();
		switch (sp->type()){
		case symbol::nonterminal:
			if (sp->lhs().n() == 0 ){
				fprintf( stderr,
				    "Warning: nonterminal \"%s\" is never produced\n",
				    sname
				);

			}
			n_rhs_usage = sp->rhs().n();
			if ( (n_rhs_usage == 0 ) && (sname != goalname )  ){
				fprintf( stderr,
				    "Warning: nonterminal \"%s\" is never consumed\n",
				    sname
				);

			}
			if ( ( sname == goalname ) && ( n_rhs_usage != 0 ) ){
				symbol_rule_usage_message(
				    "Warning: goal \"%s\" is consumed by these rules: ",
				    sname,
				    sp->rhs()
				);
			}
			break;
		case symbol::terminal:
		case symbol::unaryop:
		case symbol::binaryop:
			if ( sp->lhs().n() != 0 ){
				symbol_rule_usage_message(
				    "Symbol \"%s\" is defined like a nonterminal by these rules: ",
				    sname,
				    sp->lhs()
				);
				semantic_error += 1;
			}
			if ( sname == goalname ){
				fprintf(stderr,
				    "Warning: goal \"%s\" is a terminal symbol\n",
				    sname
				);
			}
			break;
		}
	}
}
