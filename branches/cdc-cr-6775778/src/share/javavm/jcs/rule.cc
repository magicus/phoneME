/*
 * @(#)rule.cc	1.12 06/10/10
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

// @(#)rule.cc       2.10     93/10/25 
#include "rule.h"
#include "wordlist.h"
#include "symbol.h"
#include "debug.h"
#include "globals.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// max number of nontermials in any rule we see.
int		max_rule_arity = 0;
static int	this_rule_arity;

static int    rulenumber = 1;
struct ruletree * make_tree( class rule *, wordlist *wl );
void print_tree( struct ruletree *, FILE *output = stdout);

rulelist all_rules;

void
safeprint( const char * cp, FILE * output = stdout )
{
	if (cp == 0)
		fprintf(output,"<bad address> ");
	else
		fprintf(output,"%s ", cp);
}


rule *
rule::make( char * lhs, wordlist *rhs, int cost, char *code,
	char *synth_action, char *pre_action,
	wordlist *macros_list, int is_rule_dag )
{
	rule *  rp;

	if (DEBUG(RULEWORDS) || DEBUG(RULETREE)){
		printf("NEW RULE: ");
		if (is_rule_dag)
		    printf("%DAG ");
		printf("%s: ", lhs);
		printf("%d :", cost );
		printf("%s :", (synth_action==0)?"":synth_action);
		printf("%s :", (pre_action==0)?"":pre_action);
		if (macros_list == NULL) {
		    print_wordlist( macros_list );
		}
		printf(" :");
		printf(" %s\n", code);
	}
	if (DEBUG(RULEWORDS)){
		printf("\t(in words):");
		print_wordlist( rhs );
		printf("\n");
		fflush(stdout);
	}
	rp = new rule;
	rp->cost = cost;
	rp->action = code;
	rp->synthesis_action = synth_action;
	rp->pre_action = pre_action;
	rp->macros_list = macros_list;
	rp->number = rulenumber ++ ;
	rp->dag_rule = is_rule_dag;
	// calculate "real" form of rule.
	rp->rhs_tree = make_tree( rp, rhs );
	rp->arity    = this_rule_arity;
	rp->rule_is_reached = false;
	dispose_wordlist( rhs ); // throw away wordlist
	rp->lhs_symbol = symbol::lookup( lhs );
	// add to xref for lhs
	rp->lhs_symbol->add_lhs( rp );
	if (DEBUG(RULETREE)){
		printf("\t(as tree):");
		print_tree( rp->rhs_tree );
		printf("\n");
		fflush(stdout);
	}
	// now add to list
	all_rules.add( rp );
	return rp;
}

void
rule::print(FILE * output, bool doSummaryOnly)
{

    fprintf(output,"[%3d] %s: ", number, lhs_symbol->name());
    print_tree( rhs_tree, output );
    if (!doSummaryOnly) {
        fprintf(output,": %d : ", cost);
        fprintf(output,"%s : ", has_synthesis_act()?synthesis_action:"" );
        fprintf(output,"%s : %s\n", has_pre_act()?pre_action:"", action );
    }
    fflush(output);
}

void
printrules()
{
	rule * p;
	rulelist_iterator r_iter;

	r_iter = all_rules;
	while( (p = r_iter.next() ) != 0)
		p->print(stdout);
}

static void
rule_error( const char * few_or_many, wordlist * rhs )
{
	fprintf(stderr, "line %d: this rule has too %s words: ",
	    curlineno, few_or_many );
	print_wordlist( rhs, stderr );
	fprintf(stderr,"\n");
	fflush(stderr);
	semantic_error += 1;
}

static struct ruletree *
sub_make_tree( rule * thisrule, word_iterator *wli, wordlist * rhs )
{
	struct ruletree * tp;
	symbol * s;
	const char * stringname;

	stringname = wli->next();
	if( stringname == 0 ){
		rule_error( "few", rhs );
		return 0;
	}

	tp = new ruletree;
	s  = symbol::lookup( stringname );
	tp->node = s;
	s->add_rhs( thisrule );

	switch( s->type() ){
	case symbol::nonterminal:
		// this is a leaf
		this_rule_arity += 1;
		break;
	case symbol::terminal:
		// this is a leaf
		break;
	case symbol::unaryop:
		// this is a unary operator: do one subtree
		tp->left = sub_make_tree( thisrule, wli, rhs );
		break;
	case symbol::binaryop:
		// this is a binary operator: do two subtree
		tp->left = sub_make_tree( thisrule, wli, rhs );
		tp->right = sub_make_tree( thisrule, wli, rhs );
		break;
	default:
		fprintf( stderr, "sub_make_tree: bad symbol type %d\n", s->type());
		semantic_error += 1;
	}
	return tp;
}

struct ruletree *
make_tree( rule * thisrule, wordlist *wl )
{
	struct ruletree * v;
	word_iterator wli = *wl;
	this_rule_arity = 0;
	v = sub_make_tree( thisrule, &wli, wl );
	// make sure entire word list consumed by 
	if ( wli.next() != 0 )
		rule_error( "many", wl );
	// recursive descent traversal
	if ( this_rule_arity > max_rule_arity)
	    max_rule_arity = this_rule_arity;
	return v;
}

void
print_tree( struct ruletree * t, FILE * output )
{
	symbol * s = t->node;

	fprintf(output, "%s ", s->name() );
	switch( s->type() ){
	case symbol::terminal:
	case symbol::nonterminal: 
		// done
		break;
	case symbol::unaryop:
		// print subtree
		print_tree( t->left, output );
		break;
	case symbol::binaryop:
		// print subtrees
		print_tree( t->left, output );
		print_tree( t->right, output );
		break;
	}
}
