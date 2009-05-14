/*
 * @(#)scan.l	1.16 06/10/10
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

%Start Cexpr
%Start Cstmt
%Start Normal

/* Work around flex 2.5.31 bug */
%array

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "rule.h"
#include "tbl.cc.h"
#include "debug.h"
#include "globals.h"
#include "longstring.h"

/*
 * This is set up for flex. Real lex users may need to change the following
 */
#ifndef input
#define input yyinput
#endif
#ifndef unput
#define unput yyunput
#endif

static char * scan_cstmt( int c, char scanfor, char counterpoise = '\0'  );

hashtab dictionary;

%}

%%

\,	{
	if (DEBUG(SCAN))
	    printf("<,> ");
        return ',';
	}

<Cexpr>:	{
	if (DEBUG(SCAN))
		printf("<C_EXPR(empty)>");
	/* yyless(yyleng); -- this don't seem to work in C++ */
	unput(':');
	yylval.charp = 0;
	BEGIN Normal;
	return C_EXPR;
	}

\:	{
	if (DEBUG(SCAN))
	    printf("<:> ");
	return ':';
	}

<Normal>\n	{
	if (DEBUG(SCAN))
	    printf("<NL>\n");
	return NL;
	}
<Normal>\/\/.*\n {
	if (DEBUG(SCAN))
	    printf("<NL(comment)>\n");
	return NL; /* comment */
	}

\%leaf	{
	if (DEBUG(SCAN))
	    printf("<%%leaf> ");
	return LEAF;
	}

\%term	{
	if (DEBUG(SCAN))
	    printf("<%%term> ");
	return LEAF;
	}

\%unary	{
	if (DEBUG(SCAN))
	    printf("<%%unary> ");
	return UNARY;
	}

\%binary	{
	if (DEBUG(SCAN))
	    printf("<%%binary> ");
	return BINARY;
	}

\%type	{
	if (DEBUG(SCAN))
	    printf("<%%type> ");
	return TYPE;
	}

\%goal	{
	if (DEBUG(SCAN))
	    printf("<%%goal> ");
	return GOAL;
	}

\%name	{
	if (DEBUG(SCAN))
	    printf("<%%name> ");
	return NAME;
	}

\%opcode	{
	if (DEBUG(SCAN))
	    printf("<%%opcode> ");
	return OPCODE;
	}

\%right	{
	if (DEBUG(SCAN))
	    printf("<%%right> ");
	return RIGHT;
	}

\%left	{
	if (DEBUG(SCAN))
	    printf("<%%left> ");
	return LEFT;
	}

\%getstate	{
	if (DEBUG(SCAN))
	    printf("<%%getstate> ");
	return GETSTATE;
	}

\%setstate	{
	if (DEBUG(SCAN))
	    printf("<%%setstate> ");
	return SETSTATE;
	}

\%dag	{
	if (DEBUG(SCAN))
	    printf("<%%dag> ");
	return DAG;
	}


[ \t]*  ;

<Normal>[0-9]+	{
	yylval.intval =  atoi( yytext );
	if (DEBUG(SCAN))
	    printf("<NUMBER:%d> ", yylval.intval);
	return NUMBER;
	}

<Normal>[a-zA-Z][a-zA-Z_0-9]* {
	yylval.charp = dictionary.enhash( yytext );
	if (DEBUG(SCAN))
	    printf("<WORD:%s> ", yylval.charp);
	return WORD;
	}

<Cstmt>.|\n	{
	yylval.charp = scan_cstmt( yytext[0], ';' );
	if (DEBUG(SCAN))
		printf("<CSTMT:%s> ", yylval.charp);
	BEGIN Normal;
	return C_STMT;
	}

<Cexpr>.|\n	{
	yylval.charp = scan_cstmt( yytext[0], ':', '?' );
	// oops. expression has trailing : that we do not want.
	yylval.charp[ strlen( yylval.charp ) - 1 ] = '\0';
	unput(':');
	if (DEBUG(SCAN))
		printf("<CEXPR:%s> ", yylval.charp);
	BEGIN Normal;
	return C_EXPR;
	}


\%\{	{
	/* random C code to embed in output. Write it directly out */
	int c;
	int col=2;

	if (DEBUG(SCAN))
	    printf("<C_CODE> ");
	fprintf(output_file, "#line %d \"%s\"\n", curlineno, input_name);
	for(;;){
		c = input();
		switch( c ){
		case 0:
			/* lex eof */
			unput(0);
			return C_CODE;
		case '\n':
			col = 0;
			curlineno += 1;
			break;
		case '%':
			if (col == 1){
				c = input();
				if ( c == '}' )
					return C_CODE;
				fputc('%', output_file);
				col += 1;
			}
			break;
		}
		fputc( c, output_file );
		col += 1;
	}
	}

<Normal>. {
	if (DEBUG(SCAN))
	    printf("<NL(illegal characters)>\n");

	fprintf(stderr, "Illegal characters at #line %d \"%s\":\n   \"%c",
		curlineno, input_name, yytext[0]);
	fflush(stderr);
	/* Consume the whole line of text after this: */
	for(;;) {
	    int c = input();
	    if (c == '\n') {
		break;
	    }
	    fputc(c, stderr);
	}
	fprintf(stderr, "\"\n");
	fflush(stderr);

	/* Note: to abort instead of continuing with the parsing, call
	   cleanup(1).  For now, we want to allow JCS to continue parsing. */
	return NL; /* illegal characters */
	}

%%

void
reset_scanner(void){
	if (DEBUG(SCAN))
		printf("{Normal} ");
	BEGIN Normal;
}

void
want_cexpr(void){
	if (DEBUG(SCAN))
		printf("{Cexpr} ");
	BEGIN Cexpr;
}

void
want_cstmt(void){
	if (DEBUG(SCAN))
		printf("{Cstmt} ");
	BEGIN Cstmt;
}

extern void cleanup(int exitcode);

static void
cstmt_error(const char *errorMessage, int lineNo)
{
	fprintf(stderr, "file \"%s\" line %d: %s\n",
		input_name, lineNo, errorMessage);
	fflush(stderr);
	cleanup(1);
}

static char *
scan_cstmt( int c, char scanfor, char counterpoise )
{
	/* looking for a C statement. Found something that might look like one.
	 * Note that I believe a C statement cannot begin with a %, a , or a %,
	 * so I have not protected with a start condition rules beginning with
	 * one of those characters. 
	 * NOTE: If I am wrong, this would be trivial to fix up later.
	 *
	 * Our job here is to count {, (, ) and } until we see an unqualified ;,
	 * save the junk up, then return a pointer to it. We are not real smart
	 * about quoted strings or macros, so any of these characters -- including
	 * ; -- that ought not to cound must be backslash protected!
	 */
	int nbrace = 0;
	int nparen = 0;
	int ncounter = 0;
	longstring  statement;
	int lastc;
	int startingLineNumberOfStatement = curlineno;
	int startingLineNumberOfComment = 0;
	bool inComment = false;

	{
	    char buf[32];
	    sprintf(buf, "%d", curlineno);
	    statement.add( "#line ", 6 );
	    statement.add( buf, strlen(buf));
	    statement.add( " \"", 2);
	    statement.add( input_name, strlen(input_name));
	    statement.add( "\"\n", 2);
	}
	lastc = ' '; /* harmless character */
	while (c!=EOF) { 	// lex EOF: prevents finding a zero otherwise!
		statement.add( c );
		if ( c == scanfor ){
			if (nbrace==0 && nparen==0 && ncounter<=0 && lastc!='\\')
				break; // from for (;;) loop
			else if (lastc!='\\' && ncounter>0 )
				ncounter -=1;
		} else if ( c == counterpoise ){
			ncounter +=1;
		} else switch (c){
		case '{':
			if (lastc!='\\')
				nbrace+=1;
			break;
		case '}':
			if (lastc!='\\')
				nbrace-=1;
			if (nbrace < 0) {
			    cstmt_error("'}' without a '{'", curlineno);
			}
			break;
		case '(':
			if (lastc!='\\')
				nparen+=1;
			break;
		case ')':
			if (lastc!='\\')
				nparen-=1;
			if (nparen < 0) {
			    cstmt_error("')' without a '('", curlineno);
			}
			break;
		/* Check for comments: */
		case '/': {
			    char next = input();
			    startingLineNumberOfComment = curlineno;
			    /* Check for C comments: */
			    if (next == '*') {
			        statement.add(next);
			        /* C comment found.  Go consume the comment: */
			        inComment = true;
			        c = input();
			        while (c!=EOF && inComment) {
			            statement.add(c);
			            /* Look for end of comment: */
			            switch(c) {
			            case '*':
			                next = input();
			                if (next == '/') {
			                    /* End of comment found: */
			                    statement.add(next);
			                    inComment = false;
			                } else {
			                    unput(next);
			                }
			                break;
			            case '\n':
			                curlineno += 1;
			                break;
			            default:
			                break;
			            }
			            lastc = c;
			            c = input();
			        }
			        unput(c);
			    } else if (next == '/') {
			        statement.add(next);
			        /* C++ comment found.  Go consume the comment: */
			        inComment = true;
			        c = input();
			        fprintf(stderr,
			            "Warning! C++ style comment found at line %d\n",
			            startingLineNumberOfComment);
			        while (c!=EOF && inComment) {
			            statement.add(c);
			            /* Look for end of comment: */
			            switch(c) {
			            case '\n':
			                curlineno += 1;
			                inComment = false;
			                break;
			            default:
			                break;
			            }
			            lastc = c;
			            c = input();
			        }
			        unput(c);
			    } else {
			        unput(next);
			    }
			}
			break;
		case '\n':
			curlineno += 1;
			break;
		default:
			break;
		}
		lastc = c;
		c = input();
	}
	if (c == EOF) {
	    fprintf(stderr, "line %d: Unexpected EOF in statement\n",
	            startingLineNumberOfStatement);
	    if (nbrace > 0) {
	        fprintf(stderr, "    Number of '{' exceeds '}' by %d\n", nbrace);
	    }
	    if (nparen > 0) {
	        fprintf(stderr, "    Number of '(' exceeds ')' by %d\n", nparen);
	    }
	    if (inComment) {
	        fprintf(stderr, "    Unterminated comment at line %d\n",
	                startingLineNumberOfComment);
	    }
	    fflush(stderr);
	    cleanup(1);
	}
	return statement.extract();
}
