/*
 * @(#)tbl.y	1.14 06/10/10
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

/* @(#)tbl.y       2.9     98/01/27 */
%{
#include <stdio.h>
#include "globals.h"
#include "scan.h"
#include "rule.h"
#include "symbol.h"
#include "wordlist.h"

/* Get rid of build failure when using new versions of gcc */
#if defined(__GNUC__) && (3 <= __GNUC__)
#define __attribute__(arglist)
#endif

int curlineno = 1;
char *	cgname;		// %name operand, for output generation
char *	goalname;	// %goal operand, for output generation
char *   nodetype;	// %type operand, for output generation 
char *	opfn;		// %opcode operand, for output generation
char *	rightfn;	// %right operand, for output generation
char *	leftfn;		// %left operand, for output generation
char *	getfn;		// %getstate operand, for output generation
char *	setfn;		// %setstate operand, for output generation
int	is_dag = 0;	// %dag seen, for output generation
int	parse_haderr = 0; // state of parse.

void end_rule(void);

int yylex(void);

void
yyerror( const char * msg ){
	parse_haderr ++;
	fprintf(stderr, "file \"%s\" line %d: %s\n",
		input_name, curlineno, msg);
	fflush(stderr);
}

void
end_rule(){
	curlineno += 1;
	reset_scanner();
}

void
install_state( char * newword, char ** word, const char * operation )
{
	if (*word != 0 ){
		fprintf(stderr, "file \"%s\" line %d: duplicate %s directive:"
			" changing from \"%s\" to \"%s\"\n",
			input_name, curlineno, operation, *word, newword );
		semantic_error ++;
	}
	*word = newword;
}

%}
%term TYPE
%term GOAL
%term NAME

%term LEAF
%term BINARY
%term UNARY

%term OPCODE
%term LEFT
%term RIGHT
%term SETSTATE
%term GETSTATE
%term DAG

%term NL
%term NUMBER
%term WORD
%term C_CODE
%term C_EXPR
%term C_STMT

%type <intval> NUMBER
%type <charp> WORD C_EXPR C_STMT

%type <wlist>   rhs
%type <wlist>   macros_list

%union{
	int	intval;
	char *	charp;
	wordlist *wlist;
};
%%
file: linelist;

linelist: line
|	linelist line;

line:
	opspec
|	statespec
|	rule
|	NL  { curlineno += 1; /*allow blank lines */ }
|	error NL  { end_rule(); /* resynchronize at end of line */ }
|	ctrash
;

opspec:
	LEAF WORD NL	{ (void)symbol::newsymbol( $2, symbol::terminal ); end_rule(); }
|	UNARY WORD NL	{ (void)symbol::newsymbol( $2, symbol::unaryop );  end_rule(); }
|	BINARY WORD NL	{ (void)symbol::newsymbol( $2, symbol::binaryop ); end_rule(); }
;

statespec:
	TYPE WORD NL	{ install_state( $2, &nodetype, "%type");     end_rule(); }
|	GOAL WORD NL	{ install_state( $2, &goalname, "%goal");     end_rule(); }
|	NAME WORD NL	{ install_state( $2, &cgname,   "%name");     end_rule(); }
|	OPCODE WORD NL	{ install_state( $2, &opfn,     "%opcode");   end_rule(); }
|	LEFT WORD NL	{ install_state( $2, &leftfn,   "%left");     end_rule(); }
|	RIGHT WORD NL	{ install_state( $2, &rightfn,  "%right");    end_rule(); }
|	SETSTATE WORD NL { install_state( $2, &setfn,    "%setstate"); end_rule(); }
|	GETSTATE WORD NL { install_state( $2, &getfn,    "%getstate"); end_rule(); }
;

rule:
	WORD ':' rhs ':' NUMBER	':'
	{want_cexpr();} C_EXPR ':' {want_cexpr();} C_EXPR ':' macros_list ':'
	{want_cstmt();} C_STMT NL
	{ (void)rule::make( $1, $3, $5, $16, $8, $11, $13, 0 ); end_rule();}

|	DAG WORD ':' rhs ':' NUMBER	':'
	{want_cexpr();} C_EXPR ':' {want_cexpr();} C_EXPR ':' macros_list ':'
	{want_cstmt();} C_STMT NL
	{ (void)rule::make( $2, $4, $6, $17, $9, $12, $14, 1 );
	  end_rule();
	  is_dag = 1;
	}
;

rhs:
	WORD { $$ = new_wordlist($1); }
|	rhs WORD  { $1->add( $2 ); $$ = $1; }
;

macros_list:
    rhs
|   { $$ = NULL; }
;

ctrash:
	C_CODE NL	{ end_rule(); }
;
%%
