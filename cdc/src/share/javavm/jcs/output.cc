/*
 * @(#)output.cc	1.53 06/10/10
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

// @(#)output.cc	2.25	99/07/14
#include <string.h>
#include <stdio.h>
#include "wordlist.h"
#include "globals.h"
#include "symbol.h"
#include "state.h"
#include "item.h"
#include "path.h"
#include "assert.h"
#include "transition.h"
#include "rule.h"
#include "ARRAY.h"

extern wordlist invert_state( const state * sp );

/*
 * This is not a class -- just a file full of stuff.
 *
 * This is real output: called to make real output files if we get
 *  that far.
 */

DERIVED_ARRAYwoC(path_array, path);

static const char *
small_int_type( unsigned v )
{	// assuming 2's complement representation, and that
	// typecasts properly trim bits..
# define BITS (-1L)
	if ( v <= (unsigned char)BITS )
		return "unsigned char";
	else if ( v <= (unsigned short)BITS )
		return "unsigned short";
	else if ( v <= (unsigned int)BITS ){
		assert (  v <= (unsigned int)BITS );
		return "unsigned int";
	}
# undef BITS
}

static const char *
const_small_int_type( unsigned v )
{	// assuming 2's complement representation, and that
	// typecasts properly trim bits..
# define BITS (-1L)
	if ( v <= (unsigned char)BITS )
		return "const unsigned char";
	else if ( v <= (unsigned short)BITS )
		return "const unsigned short";
	else if ( v <= (unsigned int)BITS ){
		assert (  v <= (unsigned int)BITS );
		return "const unsigned int";
	}
# undef BITS
}

static const char *
stateno_type()
{
	return small_int_type( (unsigned)allstates.n() );
}

static const char *
const_stateno_type()
{
	return const_small_int_type( (unsigned)allstates.n() );
}

/*
 * variable and function names are constructed from a number of
 * things: generally we use the cgname + terminal name + function.
 * Names are sometimes construced in print routines, from templates
 * provided by the caller. Here we consturct some of those templates
 * from cgname, so it doesn't have to be know further.
 */

static char * transition_name_template;
static char * map_name_template;
static char * matchname;
static char * handlename;
static char * rule_to_use;
static char * rule_actions;
static char * action_pairs;
static char * actionname;
static char * debugmac;
static char * printtree;
static char * action_pair_type_name;
static char * rule_action_type_name;
static char * match_state_name;
static char * rule_state_name;
static char * synthesis_phase_name;
static char * rule_is_dag;

static int output_is_setup = 0;

static char *
catiname( const char * proto )
{
	char * name_template;
	// malloc space
	// catinate cgname with the proto.
	// return result.
	name_template = (char*)malloc( strlen(cgname) + strlen(proto) + 1 );
	assert( name_template != 0 );
	strcpy( name_template, cgname );
	strcat( name_template, proto );
	return name_template;
}

static void
defit( char ** loc, const char * default_val )
{
	if (*loc == 0)
		*loc = (char *) default_val;
}

static void
construct_name_templates()
{
	transition_name_template = catiname( "_%s_transition" );
	map_name_template = catiname( "_%s_%s_map" );
	matchname = catiname( "_match"  );
	handlename = catiname( "_thing_p" );
	rule_to_use = catiname( "_rule_to_use" );
	rule_actions = catiname( "_rule_actions" );
	action_pairs = catiname( "_action_pairs" );
	actionname = catiname( "_action" );
	debugmac = catiname( "_DEBUG" );
	printtree = catiname( "_printtree" );
	action_pair_type_name = catiname("_action_pairs_t");
	rule_action_type_name = catiname("_rule_action");
	match_state_name = catiname("_match_computation_state");
	rule_state_name  = catiname("_rule_computation_state");
	synthesis_phase_name = catiname("_synthesis");
	rule_is_dag= catiname("_isdag");
}


static void
setup_output()
{
	if ( output_is_setup != 0 ) 
		return;
	// housekeeping...
	// make sure goalname and nodetype and cgname are defined to
	// something...
	defit( &cgname,   "pmatch");
	defit( &nodetype, "nodep_t");
	defit( &opfn,     "OPCODE" );
	defit( &rightfn,  "RIGHT");
	defit( &leftfn,   "LEFT");
	defit( &getfn,    "GETSTATE");
	defit( &setfn,    "SETSTATE");
	construct_name_templates();
	output_is_setup = 1;
}

/*
 * Special case transition-table output.
 * Largely copied from unary_transition::print
 */
static void
print_direct_map_unary_transition(
	statemap *sm,
	unary_transition *ut,
	const char * statetype,
	const char * symbol_name,
	const char * name_template,
	FILE *output,
	FILE *data )
{
	int maxstates = sm->max_ordinate();
	int i, count = 0;
	stateno * state = ut->state;
	/*
	 * Note: The following assertion will keep us from
	 * getting into trouble in the short run, if
	 * we have DAG management. You can take this assertion
	 * out, but BE SURE that all the places you extract the
	 * state number to use as an index it gets properly
	 * masked using CVMJIT_JCS_STATE_MASK(v).
	 */
	assert( maxstates <= 255 );
	/**************************************************/
	fprintf(output, "extern %s\t", statetype);
	fprintf(output, name_template, symbol_name );
	fprintf(output, "[%d];\n", maxstates+1);
	fprintf(data, "%s\n", statetype);
	fprintf(data, name_template, symbol_name );
	fprintf(data, "[%d] =\n", maxstates+1);
	fprintf(data, "{ ");
	for (i=0; i<maxstates; i++) {
	    print_statenumber( data, state[(*sm)[i]], ",");
	    count += 1;
	    if (count % 20 == 0) {
		fputs("\n", data);
	    }
	}
	print_statenumber(data, state[(*sm)[i]], " };\n\n");
}

/*
 * Print definitions of transition tables, including state maps.
 */
void
print_transition_tables(
    const char * statetype,
    FILE *output,
    FILE *data )
{
	symbollist_iterator s_i = all_symbols;
	symbol *sp;
	statemap *lhs, *rhs;
	unary_transition *ut;
	const char * maptype;

	setup_output();
	while( (sp = s_i.next() ) != 0){
		switch (sp->functional_type()){
		case symbol::unaryop:
		case symbol::rightunaryop:{
			if (sp->type() == symbol::unaryop ){
				unary_symbol * usp = ((unary_symbol *)sp);
				lhs = &usp->lhs_map;
				ut = usp->transitions;
			} else {
				binary_symbol * bsp = ((binary_symbol *)sp);
				lhs = &bsp->lhs_map;
				ut = (unary_transition*)(bsp->transitions);
			}
			maptype = const_small_int_type( lhs->max_mapping() );
			if ( maptype == statetype ){
			    print_direct_map_unary_transition(
				lhs, ut, statetype, sp->name(),
				transition_name_template, output, data );
			    sp->direct_map_transitions = 1;
			} else {
			    lhs->print( 
				maptype, sp->name(), "l", map_name_template,
				output, data );
			    ut->print( statetype, sp->name(),
				transition_name_template, output, data );
			}
			break;
			}
		case symbol::binaryop:{
			binary_symbol * bsp = ((binary_symbol *)sp);
			lhs = &bsp->lhs_map;
			rhs = &bsp->rhs_map;
			lhs->print( 
			    const_small_int_type( lhs->max_mapping() ),
			    sp->name(), "l", map_name_template, output, data );
			rhs->print( 
			    const_small_int_type( rhs->max_mapping() ),
			    sp->name(), "r", map_name_template, output, data );
			bsp->transitions->print( statetype, sp->name(),
			    transition_name_template, output, data );
			break;
			}
		case symbol::nonterminal:
			// no tables
			break;
		case symbol::terminal:
			// don't bother: it's only a scalar...
			break;
		}
	}
}
static void
print_dagrule_array( FILE *output, FILE *data )
{
	int nrules = all_rules.n();
	int item_this_line;
	rulelist_iterator r_iter;
	rule *rp;
	fprintf( output, "extern const char %s[ %d ];\n", rule_is_dag,
		 nrules );
	fprintf( data, "const char %s[] = {\n    0, ", rule_is_dag);
	r_iter = all_rules;
	item_this_line = 1;
	while ( (rp = r_iter.next() ) != 0){
		if ( item_this_line >= 20 ){
			fprintf( data, "\n    ");
			item_this_line = 0;
		}
		fprintf( data, rp->is_dag()? "1, ": "0, ");
		item_this_line += 1;
	}
	fprintf(data, "\n};\n");
}

static void
print_mapped_subscript(
	const char * sym_name,
	const char * designator,
	const char * subscript,
	FILE * out )
{
	putc( '[', out );
	fprintf( out, map_name_template, sym_name, designator );
	fprintf( out, "[%s]]", subscript );
}

/*
 * print out the header file full of shared data types.
 *
static char * action_pair_type_name;
static char * rule_action_type_name;
static char * match_state_name;
static char * rule_state_name;
 */


static void
print_codegen_proto(FILE * output, const char *pass, int with_semi)
{
    static const char *prototype = 
            "int\n%s_%s( %s root, CVMJITCompilationContext* con )%s\n";
    fprintf( output, prototype, cgname, pass, nodetype, with_semi ? ";" : "");
}

/* Purpose: Dump the macros list for all the rules that are defined. */
static void
print_rules_macros_list(FILE* header_file)
{
    rule *rp;
    rulelist_iterator r_iter;
    int rule_is_reached;

    r_iter = all_rules;
    while ( (rp = r_iter.next() ) != 0){
        wordlist *macros_list = rp->get_macros_list();
        if (macros_list == NULL) {
            continue;
        }

        rule_is_reached = rp->get_reached();
        fprintf(header_file, "/* Macros list for rule ");
        rp->print(header_file, true);
        fprintf(header_file, ": */\n");
        if ( ! rule_is_reached ){
            fprintf(header_file, "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
        }

        /* Dump the macros list: */
        {
            word_iterator wi = *macros_list;
            const char * p;
            static const char macro_declaration[] = 
                "#ifndef %s\n"
                "#define %s\n"
                "#endif\n";

            while ( ( p = wi.next() ) != 0 ){
                fprintf(header_file, macro_declaration, p, p);
            }
        }

        if ( ! rule_is_reached ){
            fprintf(header_file, "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
        }
       fprintf(header_file, "\n");
    }
}

static void
print_header(
	FILE * output,
	FILE * data,
	FILE* header_file,
	const char* header_name)
{
	static const char * prototype1 =
"#ifndef INCLUDED_%s\n\
#define INCLUDED_%s\n\
	/* MACHINE GENERATED -- DO NOT EDIT DIRECTLY! */ \n\
/* Possible error states returned by CVMJITCompileExpression. */\n\
#define JIT_IR_SYNTAX_ERROR -1\n\
#define JIT_RESOURCE_NONE_ERROR -2\n\n\
#ifndef IN_%s_DATA\n\
struct %s { \n\
	int	opcode; \n\
	%s	p; \n\
	%s	subtrees[2]; \n\
	int	which_submatch; \n\
	int	n_submatch; \n\
}; \n\
\n\
struct %s { \n\
	%s	p; \n\
	int	ruleno; \n\
	int	subgoals_todo; \n\
	const struct %s*	app; \n";
static const char * prototype2 =
"}; \n\n";
static const char * prototype3 = 
"\n#endif\n\
\n\
struct %s { \n\
	unsigned short pathno;\n\
	unsigned short subgoal;\n\
}; \n\
\n\
struct %s {\n\
	unsigned short	n_subgoals;\n\
	unsigned short	action_pair_index;\n\
};\n\
\n\
#endif\n\
\n";
    
	fprintf(header_file, prototype1,
	    cgname, cgname, cgname,			/* the ifdef parts */
	    match_state_name, nodetype, nodetype,	/* the match state */
	    rule_state_name, nodetype, action_pair_type_name /* rule state */
	);
	fprintf(header_file, "#define %s_MAX_ARITY %d\n",
	    cgname, max_rule_arity);
	fprintf(header_file, 
	    "	CVMJITIRNodePtr	* curr_submatch_root;\n",
	    cgname, cgname );
	fprintf(header_file, 
	    "	CVMJITIRNodePtr	submatch_roots[%s_MAX_ARITY];\n",
	    cgname, cgname );
	if (do_attributes){
	    fprintf(header_file, 
		"	%s_attribute *	curr_attribute;\n", cgname );
	    fprintf(header_file, 
		"	%s_attribute	attributes[%s_MAX_ARITY];\n",
		cgname, cgname );
	}

	fprintf(header_file, prototype2);
	print_codegen_proto(header_file, "match", 1);
	if (do_attributes) {
	    print_codegen_proto(header_file, "synthesis", 1);
	}
	print_codegen_proto(header_file, "action", 1);
	fprintf(header_file, prototype3,
	    action_pair_type_name,
	    rule_action_type_name
	);
	fprintf(output, "#include \"%s\"\n", header_name);
	fprintf(data, "#define IN_%s_DATA\n", cgname);
	fprintf(data, "#include \"%s\"\n", header_name);
}

/*
 * Print the bottom up pattern match
 * machinery. Avoid recursion in the generated program
 * by managing our own stack.
 */
static void
print_match( const char * statetype, FILE * output)
{
	symbollist_iterator s_i = all_symbols;
	symbol *symp;

	/* dag management assumes that there will be no more than
	 * 4000 states!
	 */
	static const char * dag_management =
"#define CVMJIT_JCS_MATCH_DAG\n\
#define	CVMJIT_JCS_STATE_MASK(v)	(v & 0x0fff)\n\
#define	CVMJIT_JCS_STATE_MATCHED	0x1000\n\
#define CVMJIT_JCS_STATE_SYNTHED	0x2000\n\
#define CVMJIT_JCS_STATE_SUBACTED 0x4000\n\
#define CVMJIT_JCS_STATE_ACTED	0x8000\n\
#define CVMJIT_JCS_SET_STATE(p, flag)	%s(p, %s(p)|flag)\n\
#define CVMJIT_JCS_DID_PHASE(v, flag)	(((v) & (flag)) != 0)\n\
#define CVMJIT_DID_SEMANTIC_ACTION(p)	(CVMJIT_JCS_DID_PHASE(%s(p), CVMJIT_JCS_STATE_ACTED))\n\
";

	static const char * nondag_management = "\
#define	CVMJIT_JCS_STATE_MASK(v)	v\n\
#define	CVMJIT_JCS_STATE_MATCHED	0x0\n\
#define CVMJIT_DID_SEMANTIC_ACTION(p)	(CVM_FALSE)\n";

	static const char * prologue =
"\nint\n\
%s( %s p, CVMJITCompilationContext* con)\n\
{\n\
	%s l = 255, r = 255, result;\n\
	%s left_p, right_p;\n\
	int opcode, n_operands;\n\
#ifdef CVMJIT_JCS_MATCH_DAG\n\
	int stateno;\n\
#endif\n\
	struct %s * top_state;\n\
	INITIALIZE_MATCH_STACK;\n\
    new_node:\n\
#ifdef CVMJIT_JCS_MATCH_DAG\n\
	stateno = %s(p);\n\
	if (stateno >= CVMJIT_JCS_STATE_MATCHED){\n\
		goto skip_match;\n\
	}\n\
#endif\n\
	opcode = %s(con, p);\n\
	switch( opcode ){\n";
	
	static const char * binarycase =
"		left_p = %s(p);\n\
		right_p = %s(p);\n\
		MATCH_PUSH( p, opcode, left_p, right_p, 0, 2 );\n\
		break;\n";

	static const char * unarycase =
"		left_p = %s(p);\n\
		right_p = NULL;\n\
		MATCH_PUSH( p, opcode, left_p, NULL, 0, 1 );\n\
		break;\n\
	default:\n\
		goto assign_state; /* leaves need not worry */\n\
	}\n\
	top_state = GET_MATCH_STACK_TOP;\n\
    continue_processing:\n\
	if ( top_state->which_submatch < top_state->n_submatch ){\n\
		p = top_state->subtrees[top_state->which_submatch++];\n\
		goto new_node;\n\
	}\n\
	/* done with subtrees, go assign state.*/\n\
	MATCH_POP( p, opcode, left_p, right_p, n_operands )\n\
	switch ( n_operands ){\n\
	    case 2:\n\
		r = CVMJIT_JCS_STATE_MASK(%s( right_p ));\n\
		/* FALLTHRU */\n\
	    case 1:\n\
		l = CVMJIT_JCS_STATE_MASK(%s( left_p ));\n\
	}\n\
    assign_state:\n\
	/* IN THE FUTURE, DO THIS ALL DIFFERENT */\n\
	/* now use appropriate transition function */\n\
	switch ( opcode ){\n";

	static const char * switch_end =
"	default:\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
		con->errNode = p;\n\
#endif\n\
		return -1;\n\
        }\n";

	static const char * err_check =
"	if (result == 0){\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
		con->errNode = p;\n\
#endif\n\
		return -1;\n\
	}\n";

	static const char * epilogue =
"	%s(p, result|CVMJIT_JCS_STATE_MATCHED);\n\
#ifdef CVMJIT_JCS_MATCH_DAG\n\
    skip_match:\n\
#endif\n\
	if ( MATCH_STACK_EMPTY ){\n\
	    return 0;\n\
	}\n\
	top_state = GET_MATCH_STACK_TOP;\n\
	goto continue_processing;\n\
}\n\n";

	static const char * switch_label = "\tcase %s:\n\t\tresult = ";

	static const char * finish_transition =
		";\n\t\tbreak;\n";

	if (is_dag){
	    fprintf(output, dag_management,
		setfn, getfn, getfn );
	}else{
	    fprintf(output, nondag_management);
	}
	fprintf(output, prologue,
	    matchname,
	    nodetype,
	    statetype,
	    nodetype,
	    match_state_name,
	    getfn,
	    opfn);
	// write out names of all binary operators
	while ( (symp = s_i.next() )  != 0){
		if (symp->type() == symbol::binaryop)
			fprintf( output, "\tcase %s:\n", symp->name() );
	}
	fprintf(output, binarycase, leftfn, rightfn);
	// write out names of all unary operators
	s_i = all_symbols;
	while ( (symp = s_i.next() )  != 0){
		if (symp->type() == symbol::unaryop)
			fprintf( output, "\tcase %s:\n", symp->name() );
	}
	fprintf(output, unarycase, leftfn, getfn, getfn);

	s_i = all_symbols;
	while ( (symp = s_i.next() )  != 0){
		switch (symp->functional_type()){
		case symbol::binaryop:
			fprintf( output, switch_label, symp->name());
			fprintf(output, transition_name_template, symp->name());
			print_mapped_subscript( symp->name(), "l", "l", output );
			print_mapped_subscript( symp->name(), "r", "r", output );
			fprintf(output, finish_transition );
			break;
		case symbol::unaryop:
			fprintf( output, switch_label, symp->name());
			fprintf(output, transition_name_template, symp->name());
			if (symp->direct_map_transitions){
				fputs( "[l]", output );
			} else {
				print_mapped_subscript( symp->name(), "l", "l",
							output );
			}
			fprintf(output, finish_transition );
			break;
		case symbol::rightunaryop:
			fprintf( output, switch_label, symp->name());
			fprintf(output, transition_name_template, symp->name());
			if (symp->direct_map_transitions){
				fputs( "[r]", output );
			} else {
				print_mapped_subscript( symp->name(), "l", "r",
							output );
			}
			fprintf(output, finish_transition );
			break;
		case symbol::terminal:{
			leaf_transition *lt;
			fprintf( output, switch_label, symp->name());
			switch (symp->type()){
			case symbol::terminal:
				lt = ((terminal_symbol *)symp) -> transitions;
				break;
			case symbol::unaryop:
				lt = (leaf_transition*)
					((unary_symbol *)symp) -> transitions;
				break;
			case symbol::binaryop:
				lt = (leaf_transition*)
					((binary_symbol *)symp) -> transitions;
				break;
			}
			fprintf(output, "%d", lt->state );
			fprintf(output, finish_transition );
			break;
			}
		case symbol::nonterminal:
			break; // don't appear in trees.
		}
	}
	fprintf( output, switch_end );
	if (!error_folding){
	    fprintf( output, err_check );
	}
	fprintf( output, epilogue, setfn );
}

static void
print_with_substitution( const char * msg, const char * subst, FILE * output )
{
	char c;
	
	while( (c = *(msg++)) != 0 ){
		if ((c=='$') && (*msg == '$')){
			fputs( subst, output );
			msg++;
		} else {
			putc( c, output );
		}
	}
}

static void
print_dorule( FILE * output )
{
	rule *rp;
	rulelist_iterator r_iter;
	int	rule_is_reached;

	static const char * prologue =
"	switch( ruleno ){\n\
	case 0:\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
		con->errNode = %s;\n\
#endif\n\
error_return_branch_island:\n\
		return -1;\n";

	fprintf( output, prologue, handlename );
	r_iter = all_rules;
	while ( (rp = r_iter.next() ) != 0){
		rule_is_reached = rp->get_reached();
		if ( ! rule_is_reached ){
		    fprintf( output, "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
		}
		fprintf( output, "	case %d:\n", rp->ruleno() );
		fprintf(output, "	CVMJITdoStartOfCodegenRuleAction(con, ruleno, \"");
		rp->print(output, true);
		fprintf(output, "\", %s);\n", handlename);
		print_with_substitution( rp->act(), handlename, output );
		fprintf(output, "\n	CVMJITdoEndOfCodegenRuleAction(con);\n");
		fputs("	break;\n", output );
		if ( ! rule_is_reached ){
		    fprintf( output, "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
		}
	}
	fputs("	}\n", output);
}

static int
is_chain_rule(rule *rp){
    return rp->get_arity() == 1 && 
	   rp->rhs()->node->type() == symbol::nonterminal;
}

/*
 * Emit case arms for synthetic and inherited attribute manipulation
 */
static void write_actions(
	FILE*	output,
	int	(rule::*has_action)(),
	char*	(rule::*get_action)(),
	const char* default_word)
{
	rule *rp;
	rulelist_iterator r_iter;
	int * defaults_found = (int*)calloc( sizeof(int), max_rule_arity + 1);
	int this_arity;
	int chain_rules_found = 0;
	int rule_is_reached;

	r_iter = all_rules;
	while ( (rp = r_iter.next() ) != 0){
		if ( (rp->*has_action)() ){
		    rule_is_reached = rp->get_reached();
		    if ( ! rule_is_reached ){
		        fprintf( output, "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
		    }
		    fprintf( output, "	case %d:\n", rp->ruleno() );
		    print_with_substitution( (rp->*get_action)(), handlename, output );
		    fputs("\n	break;\n", output );
		    if ( ! rule_is_reached ){
		        fprintf( output, "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
		    }
		} else {
		    defaults_found[ rp->get_arity() ] = 1;
		}
	}
	// now go back and find all the defaults. Sort by arity.
	// treat unary rules separately, so we can detect chain rules
	// and give them separate defaults.
	if (max_rule_arity >= 1 && defaults_found[1]){
		r_iter = all_rules;
		while ( (rp = r_iter.next() ) != 0){
		    if ( (rp->get_arity() == 1) && 
			!((rp->*has_action)() )){
			    if (is_chain_rule(rp)){
				    chain_rules_found += 1;
			    } else {
				    rule_is_reached = rp->get_reached();
				    if (!rule_is_reached) {
				        fprintf(output, 
				                "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
				    }
				    fprintf( output, "\tcase %d:\n",
					    rp->ruleno() );
				    if (!rule_is_reached) {
				        fprintf(output,
				                "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
				    }
			    }
		    }
		}
		fprintf(output, "\t\tDEFAULT_%s1(con, %s);\n\t\tbreak;\n",
		    default_word, handlename );
	}
	if (chain_rules_found > 0){
		r_iter = all_rules;
		while ( (rp = r_iter.next() ) != 0){
		    if ( (rp->get_arity() == 1) && 
			!((rp->*has_action)() )){
			    rule_is_reached = rp->get_reached();
			    if (!rule_is_reached) {
			        fprintf(output, "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
			    }
			    if (is_chain_rule(rp)){
				    fprintf( output, "\tcase %d:\n",
					    rp->ruleno() );
			    }
			    if (!rule_is_reached) {
			        fprintf(output,
			                "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
			    }
		    }
		}
		fprintf(output, "\t\tDEFAULT_%s_CHAIN(con, %s);\n\t\tbreak;\n",
		    default_word, handlename );
	}
	for ( this_arity = 2; this_arity <= max_rule_arity; this_arity ++){
		if ( !defaults_found[this_arity])
		    continue;
		r_iter = all_rules;
		while ( (rp = r_iter.next() ) != 0){
		    if ( (rp->get_arity() == this_arity) && 
			!((rp->*has_action)() )){
			    rule_is_reached = rp->get_reached();
			    if (!rule_is_reached) {
			        fprintf(output, "#ifdef CVM_CG_EXPAND_UNREACHED_RULE\n");
			    }
			    fprintf( output, "\tcase %d:\n", rp->ruleno() );
			    if (!rule_is_reached) {
			        fprintf(output,
			                "#endif /*CVM_CG_EXPAND_UNREACHED_RULE*/\n");
			    }
		    }
		}
		fprintf(output, "\t\tDEFAULT_%s%d(con, %s);\n\t\tbreak;\n",
		    default_word, this_arity, handlename );
	}
	free(defaults_found);
		
}

void do_synthesis(FILE * output)
{
	/* emit code inline to manipulate synthetic attributes.
	 */

	const static char * prologue =
"	switch( ruleno ){\n\
	case 0:\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
		con->errNode = %s;\n\
#endif\n\
		return -1;\n";

	fprintf( output, prologue, handlename );

	write_actions(output, &rule::has_synthesis_act, &rule::synthesis_act,
			"SYNTHESIS");
	fprintf(output,"	}\n");
		
}

void print_inheritance(FILE * output)
{
	/* emit code inline to manipulate inherited attributes.
	 */

	const static char * prologue =
"	switch( ruleno ){\n\
	case 0:\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
		con->errNode = %s;\n\
#endif\n\
		goto error_return_branch_island;\n";

	if ( !do_attributes){
	    return;
	}
	fprintf( output, prologue, handlename );

	write_actions(output, &rule::has_pre_act, &rule::pre_act,
			"INHERITANCE");
	fprintf(output,"	}\n");
}

static int
print_action_matrix( FILE *output, FILE *data )
{
	int ngoals, goalval;
	symbol *sp;
	symbollist_iterator s_iter;
	int 	n_nonterms = nonterminal_symbols.n();
	int	i;
	state *stp;
	statelist_iterator st_iter;
	int goalnumber = -1;
	int gotgoal    =  0;


	/* 
	 * first off, print out the goal values. This isn't for us,
	 * but for a human.
	 */
	fputs("enum goals { goal_BAD ", output );
	ngoals = 0;
	s_iter = nonterminal_symbols;
	while( (sp = s_iter.next() ) != 0){
		goalval = ((nonterminal_symbol *)sp)->goal_number;
		ngoals += 1;
		assert( goalval == ngoals );
		fprintf( output, ", goal_%s", sp->name() );
		if (!gotgoal){
			if (goalname==0 || goalname==sp->name() ){
				// first or named one.
				goalnumber = goalval;
				gotgoal = 1;
			}
		}
	}
	fputs(" };\n\n", output );
	assert( ngoals == nonterminal_symbols.n() );

	/* 
	 * now print the actual matrix.
	 * Brute force at its finest
	 */

	fprintf( output, "extern const short\t%s[%d][%d];\n",
	      rule_to_use, allstates.n(), ngoals+1 );
	fprintf( data, "const short\n%s[%d][%d] = {\n",
	      rule_to_use, allstates.n(), ngoals+1 );
	st_iter = allstates;
	for(int state = 0; (stp = st_iter.next()) != 0; state++) {
		if ( stp->rules_to_use == 0 )
			stp->use_rules();
		// for all states...
		fprintf(data, "    /* State %4d */ { 0,", state); // 0th goal
		for ( i = 0; i < n_nonterms; i++ ){
			fprintf( data, " %d,", stp->rules_to_use[i] );
		}
		fputs(" },\n", data );
	}
	fputs("};\n\n", data );

	assert( gotgoal );
	return goalnumber;
}

/*
 * the hard one.
 * synthesize a routine that, given a subtree and a goal:
 * 1) figures out which rule we want to apply;
 * 2) causes appropriate subgoals to be satisfied;
 * 3) applies the rule's semantic action.
 * we use the matrix rule_to_use to satisfy 1),
 * and write lots of yeucchy code to do 2, then
 * simply call dorule to do 3. We will not try
 * to factor the code generated, even though it's likely
 * to be real redundant.
 */
/*
 * but first, some subroutines & data structs we'll need.
 */
struct subgoal{ path p; nonterminal_symbol *sp; struct subgoal * next; };

static void
print_down_path( FILE * output, path p, const char *ptrname )
{
	int level = 0;
	int done = 0;

	do {
		switch( p.last_move()){
		case path::left:
			fprintf( output, "%s(", leftfn );
			level ++;
			p.ascend();
			break;
		case path::right:
			fprintf( output, "%s(", rightfn );
			level ++;
			p.ascend();
			break;
		default:
			fputs(ptrname, output );
			done = 1;
			break;
		}
	} while ( !done );
	while ( level > 0 ){
		putc( ')', output );
		level --;
	}
}

static path_array paths;
static int max_path_index = -1;

static int
find_path_index( const path &p ){
        int arraymax = max_path_index;
	int i;
	for ( i = 0; i <= arraymax; i++ ){
	    if ( p == paths[i] ){
		return i;
	    }
	}
	paths[i] = p;
	max_path_index = i;
	return i;
}


void
find_nonterminals( struct ruletree *rtp, struct subgoal **sgpp, path p )
{
	path rightpath, leftpath;
	struct subgoal * sgp;

	switch( rtp->node->type() ){
	case symbol::binaryop:
		// look down right subtree...
		rightpath = p;
		rightpath.descend( path::right );
		find_nonterminals( rtp->right, sgpp, rightpath );
		/* FALLTHROUGH */
	case symbol::unaryop:
		// look down left subtree...
		leftpath = p;
		leftpath.descend( path::left );
		find_nonterminals( rtp->left, sgpp, leftpath );
		break;
	case symbol::terminal:
		// not here.
		break;
	case symbol::nonterminal:
		// found one.
		sgp = new (struct subgoal);
		sgp->p = p;
		sgp->sp = (nonterminal_symbol *)(rtp->node);
		sgp->next = *sgpp;
		*sgpp = sgp;
		break;
	}
}

void
print_subgoal_description( FILE * output, FILE * data )
{
	rule * rp;
	rulelist_iterator r_i;
	struct subgoal * gp, *gp2;
	path q;
	int rulecost;
	int * first_action;
	int rule_index;
	int max_rule_index;
	int next_action_index;

	first_action = (int*)malloc( sizeof(int)*(all_rules.n()+1) );
	assert( first_action != NULL );

	fprintf(output, "extern const struct %s %s[];\n",
	    action_pair_type_name, action_pairs);
	fprintf(output, "extern const struct %s %s[];\n",
	    rule_action_type_name, rule_actions);

	fprintf(data, "const struct %s %s[]={\n",
	    action_pair_type_name, action_pairs);

	r_i = all_rules;
	rule_index = 0;
	next_action_index = 0;
	int apIdx = 0;
	while ( (rp = r_i.next() ) != 0){
		gp = NULL;
		first_action[rule_index++] = next_action_index;
		q.zero();
		find_nonterminals( rp->rhs(), & gp, q );
		while ( gp != 0 ){
			// enumerate subgoals
			fprintf(data, "    /* Action Pair %4d */ { %d, %d /* %s */ },\n",
			    apIdx++, find_path_index( gp->p ), gp->sp->goal_number,
			    gp->sp->name());
			// loop housekeeping
			next_action_index += 1;
			gp2 = gp;
			gp = gp->next;
			delete gp2;
		}
	}
	// the one off-the-end, so we know how many goals in the last rule.
	// (see below)
	first_action[rule_index] = next_action_index;

	fprintf(data, "};\nconst struct %s %s[]={\n"
	    "    /* Rule Action    0 */ { 0, 0 },\n",
	    rule_action_type_name, rule_actions);
	max_rule_index = rule_index;
	for (rule_index = 0; rule_index < max_rule_index; rule_index++){
		fprintf(data, "    /* Rule Action %4d */ { %d, %d },\n", rule_index+1,
		    first_action[rule_index+1]-first_action[rule_index],
		    first_action[rule_index] );
	}
	fputs("};\n", data);
	free( first_action );
}

static void
do_submatch_roots( FILE * output )
{
	path q;
	int  pathno, maxpaths;

	static const char * submatch_root_rule =
"	submatch_roots = goal_top->submatch_roots;\n\
	goal_top->curr_submatch_root = submatch_roots;\n\
	for (i=0; i<subgoals_todo; i++){\n\
	    switch (app[i].pathno ){\n";

	fprintf(output, submatch_root_rule);
	maxpaths = max_path_index;
	for ( pathno = 0; pathno <= maxpaths; pathno ++ ){
		q = paths[pathno];
		fprintf(output,"\t\tcase %d:\n\t\t    submatch_roots[i]=",
		    pathno );
		print_down_path( output, q, handlename );
		fputs( ";\n\t\t    break;\n", output );
	}
	fputs("		}\n	}\n", output);
}

void
print_synthesis( FILE * output )
{
	rule * rp;
	rulelist_iterator r_i;
	struct subgoal * gp, *gp2;
	path q;
	int  pathno, maxpaths;
	int rulecost;

	static const char * prologue =
"int\n\
%s( %s %s, CVMJITCompilationContext* con)\n\
{\n\
	INITIALIZE_STACK_STATS(SYNTHESIS)\n\
	int ruleno, subgoals_todo, goal, i;\n\
	unsigned int stateno;\n\
	const struct %s *app = NULL;\n\
	struct %s*  goal_top;\n\
	%s *submatch_roots = NULL;\n\
	INITIALIZE_GOAL_STACK;\n\
	goal = goal_%s;\n";

	static const char * start_rule =
"start_rule:\n\
	stateno = %s(%s);\n\
	ruleno = %s[ CVMJIT_JCS_STATE_MASK(stateno) ][ goal ];\n\
#ifdef CVMJIT_JCS_MATCH_DAG\n\
	if (CVMJIT_JCS_DID_PHASE(stateno, CVMJIT_JCS_STATE_SYNTHED)){\n\
	    if (%s[ruleno]){\n\
		/*\n\
		CVMconsolePrintf(\">Synth: skipping node %%d rule %%d\\n\", CVMJITCompileExpression_thing_p->nodeID, ruleno);\n\
		*/\n\
		goto skip_synth;\n\
	    }\n\
	}\n\
#endif\n\
	if (ruleno == 0 ) {\n\
#if defined(CVM_DEBUG) || defined(CVM_TRACE_JIT)\n\
	    con->errNode = %s;\n\
#endif\n\
	    return -1;\n\
	}\n\
	subgoals_todo = %s[ruleno].n_subgoals;\n\
	app = &%s[%s[ruleno].action_pair_index];\n\
	goal_top->p = %s;\n\
	goal_top->ruleno = ruleno;\n\
	goal_top->subgoals_todo = subgoals_todo;\n\
	goal_top->app = app;\n";

	static const char * continue_rule =
"continue_rule:\n\
	if ( goal_top->subgoals_todo > 0 ){\n\
		app = (goal_top->app)++;\n\
		goal_top->subgoals_todo -= 1;\n\
		%s = *goal_top->curr_submatch_root++;\n";

	static const char * finish_continue_rule =
"		goal = app->subgoal;\n\
		goal_top += 1;\n\
		statsPushStack(SYNTHESIS);\n\
		validateStack((goal_top < GOAL_STACK_TOP), Synthesis);\n\
		goto start_rule;\n\
	}\n\
	%s = goal_top->p;\n\
	submatch_roots = goal_top->submatch_roots;\n\
	ruleno = goal_top->ruleno;\n";

	static const char * finish_rule =
"#ifdef CVMJIT_JCS_MATCH_DAG\n\
    skip_synth:\n\
#endif\n";

static const char * epilogue =
"#ifdef CVMJIT_JCS_MATCH_DAG\n\
	CVMJIT_JCS_SET_STATE(%s, CVMJIT_JCS_STATE_SYNTHED);\n\
#endif\n\
	if ( !GOAL_STACK_EMPTY ){\n\
		statsPopStack(SYNTHESIS);\n\
		goal_top -= 1;\n\
		goto continue_rule;\n\
	}\n\
	return 0;\n\
}\n\n";

	fprintf(output, prologue,
	    synthesis_phase_name,
	    nodetype, handlename,
	    action_pair_type_name,
	    rule_state_name,
	    nodetype,
	    goalname );

	fprintf(output, start_rule,
	    getfn, handlename,
	    rule_to_use,
	    rule_is_dag,
	    handlename,
	    rule_actions,
	    action_pairs, rule_actions,
	    handlename );

	do_submatch_roots(output);
	    
	fprintf(output, continue_rule, handlename );
	fprintf(output, finish_continue_rule, handlename );
	fprintf(output, finish_rule);

	do_synthesis( output );

	fprintf(output, epilogue, handlename);
}

void
print_satisfy_subgoals( FILE * output, int do_attributes )
{
	rule * rp;
	rulelist_iterator r_i;
	struct subgoal * gp, *gp2;
	path q;
	int  pathno, maxpaths;
	int rulecost;

	static const char * prologue =
"int\n\
%s( %s %s, CVMJITCompilationContext* con)\n\
{\n\
	INITIALIZE_STACK_STATS(ACTION)\n\
	int ruleno, subgoals_todo = 0, goal, i;\n\
	unsigned int stateno;\n\
	const struct %s *app = NULL;\n\
	struct %s*  goal_top;\n\
	%s *submatch_roots = NULL;\n\
	INITIALIZE_GOAL_STACK;\n\
	goal = goal_%s;\n";

	static const char * start_rule =
"start_rule:\n\
	/* Get the rule which can convert the current IRNode from its current\n\
	   state into a form that is expected by the desired goal: */\n\
	stateno = %s(%s);\n\
	ruleno = %s[ CVMJIT_JCS_STATE_MASK(stateno) ][ goal ];\n\
#ifdef CVMJIT_JCS_MATCH_DAG\n\
	if (CVMJIT_JCS_DID_PHASE(stateno, CVMJIT_JCS_STATE_SUBACTED)){\n\
	    if (%s[ruleno]){\n\
		/*\n\
		CVMconsolePrintf(\">Action: skipping node %%d rule %%d\\n\", CVMJITCompileExpression_thing_p->nodeID, ruleno);\n\
		*/\n\
		goto skip_action_recursion;\n\
	    }\n\
	}\n\
#endif\n\
	if (ruleno != 0 ) {\n\
            /* Initialize the current goal: */\n\
	    subgoals_todo = %s[ruleno].n_subgoals;\n\
	    app = &%s[%s[ruleno].action_pair_index];\n\
	    goal_top->p = %s;\n\
	    goal_top->ruleno = ruleno;\n\
	    goal_top->subgoals_todo = subgoals_todo;\n\
	    goal_top->app = app;\n\
	}\n";

	static const char * continue_rule =
"continue_rule:\n\
	if ( goal_top->subgoals_todo > 0 ){\n\
		app = (goal_top->app)++;\n\
		goal_top->subgoals_todo -= 1;\n\
		%s = *goal_top->curr_submatch_root++;\n";


	static const char * finish_continue_rule =
"		goal = app->subgoal;\n\
		goal_top += 1;\n\
		statsPushStack(ACTION);\n\
		validateStack((goal_top < GOAL_STACK_TOP), Action);\n\
		goto start_rule;\n\
	}\n\
	%s = goal_top->p;\n\
	submatch_roots = goal_top->submatch_roots;\n\
	ruleno = goal_top->ruleno;\n";

	static const char * finish_rule =
"#ifdef CVMJIT_JCS_MATCH_DAG\n\
	CVMJIT_JCS_SET_STATE(%s, CVMJIT_JCS_STATE_SUBACTED);\n\
    skip_action_recursion:\n\
#endif\n\
#ifdef %s\n\
	if (%s)\n\
	    fprintf(stderr, \"Applying rule %%d to node %%d\\n\",\n\
		ruleno, id(%s) );\n\
#endif\n\n";


static const char * epilogue =
"#ifdef CVMJIT_JCS_MATCH_DAG\n\
	CVMJIT_JCS_SET_STATE(%s, CVMJIT_JCS_STATE_ACTED);\n\
#endif\n\
	if ( !GOAL_STACK_EMPTY ){\n\
		statsPopStack(ACTION);\n\
		goal_top -= 1;\n\
		%s\n\
		goto continue_rule;\n\
	}\n\
	return 0;\n\
}\n\n";

	fprintf(output, prologue,
	    actionname,
	    nodetype, handlename,
	    action_pair_type_name,
	    rule_state_name,
	    nodetype,
	    goalname );

	fprintf(output, start_rule,
	    getfn, handlename, rule_to_use, rule_is_dag,
	    rule_actions,
	    action_pairs, rule_actions,
	    handlename );

	do_submatch_roots(output);

	if ( do_attributes ){
		fprintf( output, "	    goal_top->curr_attribute = goal_top->attributes;\n");
		print_inheritance( output );
	}

	fprintf(output, continue_rule, handlename );
	fprintf(output, finish_continue_rule, handlename );

	fprintf(output, finish_rule,
	    handlename,debugmac, debugmac, 
	    handlename );
	print_dorule( output );

	fprintf(output, epilogue,
	    handlename,
	    do_attributes ?
		"goal_top->curr_attribute +=1;" : ""
	);
}

/*
 * aid to debugging program (and thus grammar and generator )
 */
static void
print_printtree( FILE * output )
{
	/*
	 * write out a data structure that corresponds to
	 * our symbol table. It must give:
	 * 	correspondence between opcode numbers and names;
	 *	unary/binary/terminal type info.
	 * Given that, we can traverse the tree, printing out
	 * node id's and opcodes and state #s, which is what we're
	 * really after.
	 */

	symbol * sp;
	symbollist_iterator s_i = all_symbols;
	const char * typeword;

	static const char * prologue =
"#ifdef %s\n\
enum type { t_binary, t_unary, t_leaf, t_eot };\n\n\
/* symbol table of node types */\n\
struct symbol { int symval; char * symname; enum type t; };\n\
\n\
static struct symbol debug_symboltable[] = {\n";

	/* end of symbol table and lookup code */
	static const char * lookup = 
"	{ 0, 0, t_eot} };\n\n\
void\n\
%s( %s p )\n\
{\n\
	struct symbol * sp;\n\
	char * symname;\n\
	char buf[10];\n\
\n\
	/* look up symbol */\n\
	for ( sp = debug_symboltable; sp->t != t_eot; sp ++ )\n\
		if (%s(con, p) == sp->symval) break;\n\
	if ( sp->t == t_eot ){\n\
		sprintf( buf, \"<%%d>\", %s(con, p) );\n\
		symname = buf;\n\
	} else \n\
		symname = sp->symname;\n\
	/* now print out what we got */\n\
	fprintf( stderr, \"(%%s [%%d] state %%d \", symname, id(p), %s(p));\n\
	switch (sp->t){\n\
	case t_binary:\n\
		%s( %s(p)  );\n\
		%s( %s(p) );\n\
		break;\n\
	case t_unary:\n\
		%s( %s(p)  );\n\
		break;\n\
	}\n\
	fputs( \" ) \", stderr );\n\
}\n\
#endif\n\n";
	

	fprintf( output, prologue, debugmac );
	/* print out symbol table */
	while ( ( sp = s_i.next() ) != 0 ){
		switch ( sp->type() ){
		case symbol::binaryop: typeword = "t_binary"; break;
		case symbol::unaryop:  typeword = "t_unary";  break;
		case symbol::terminal: typeword = "t_leaf";   break;
		case symbol::nonterminal:
			continue; // nonterminals don't appear in trees.
		}
		fprintf( output, "\t{ %s, \"%s\", %s },\n",
			 sp->name(), sp->name(), typeword );
	}
	fprintf( output, lookup,
		printtree, nodetype,
		opfn, opfn, getfn,
		printtree, leftfn, printtree, rightfn,
		printtree, leftfn );
}

void
print_output_file(
    FILE* output,
    FILE* data,
    FILE* header,
    const char* header_name )
{
	int goalno;

	setup_output();
	print_header(output, data, header, header_name);
	print_transition_tables(const_stateno_type(), output, data );
	if (is_dag)
	    print_dagrule_array( output, data );
	print_match(stateno_type(), output );
	goalno = print_action_matrix( output, data );

	/* We have to wait till after print_action_matrix() to print the rules
	   macros list because print_rules_macros_list() depends on
	   print_action_matrix() to set the is_reached flag in each rule: */
	print_rules_macros_list(header);
	fclose(header);

	print_subgoal_description( output, data );
	if ( do_attributes ){
	    print_synthesis( output );
	}
	print_satisfy_subgoals( output, do_attributes );
	print_printtree( output );
}
