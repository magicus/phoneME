/*
 * @(#)main.cc	1.11 06/10/10
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

// @(#)main.cc       2.21     99/07/14 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "debug.h"
#include "globals.h"
#include "scan.h"
#include "compress.h"
#include "wordlist.h"

extern int parse_haderr;
extern int yyparse(void);
extern void compute_states(void);
extern void item_setup(void);
extern void close_symbols(void);
extern void print_closures(int);
extern void print_output_file( FILE * output, FILE *data, FILE *header,
    const char* header_name  );
extern void checksymbols();
extern void printstateinversion();

#define DEFAULT_INPUT_NAME	"<stdin>"
#define DEFAULT_OUTPUT_NAME	"codegen.c"
#define DEFAULT_DATA_NAME	"codegenData.c"
#define DEFAULT_HEADER_NAME	"codegen.h"

int debugging;
int semantic_error;
int error_folding;
int verbose;
int uncompress = 0;
int do_attributes;

wordlist    	input_file_names;
word_iterator	input_file_iterator;
const char *input_name = NULL;
extern FILE * yyin;
static const char *output_name = DEFAULT_OUTPUT_NAME;
FILE * output_file;
static const char *data_name = DEFAULT_DATA_NAME;
static FILE *data_file;
static const char *header_name = DEFAULT_HEADER_NAME;
static FILE *header_file;
char * prog_name;

void cleanup( int exitcode );

main(int argc, char **argv) {
	debugging = 0;
	verbose = 0;
	semantic_error = 0;
	error_folding = 0;
	do_attributes = 0;
	prog_name = *argv;
	while ( argc-- > 1 ){
		// parse our argument
		++argv;
		if (argv[0][0] == '-'){
			switch (argv[0][1]){
			case 'a':
				do_attributes = 1;
				break;
			case 'd':
				debugging += atoi( &argv[0][2] );
				break;
			case 'e':
				error_folding = 1;
				break;
			case 'o':
				--argc;
				++argv;
				output_name = argv[0];
				--argc;
				++argv;
				data_name = argv[0];
				--argc;
				++argv;
				header_name = argv[0];
				break;
			case 'u':
				uncompress += 1;
				break;
			case 'v':
				verbose += 1;
				break;
			default:
				fprintf( stderr,
				    "%s: flag -%c not recognized\n",
				    prog_name, argv[0][1] );
				semantic_error += 1;
				break;
			}
		} else {
			input_file_names.add( argv[0] );
		}
	}
	if (semantic_error != 0 ){
		exit( semantic_error );
	}
	if ( input_file_names.n() == 0){
	    input_name = DEFAULT_INPUT_NAME;
	} else {
	    input_file_iterator = input_file_names;
	    input_name = input_file_iterator.next();
	    yyin = fopen( input_name, "r");
	}

	// open output file BEFORE parsing, so we can pass random junk.
	output_file = fopen( output_name, "w");

	// open a separate file for data.  Tables go here, so that
	// we can compile them with optimization off.  [When optimization
	// is enabled, the SPARC assembler/optimizer builds giant nodelists
	// (>40MB) for large data initialization sections.]
	data_file = fopen( data_name, "w");

	// open output file for data structures we need to share.
	header_file = fopen( header_name, "w");

	if (output_file == NULL){
	    fprintf(stderr, "%s: could not open output file %s\n",
		prog_name, output_name);
	    semantic_error = 1;
	}
	if (data_file == NULL){
	    fprintf(stderr, "%s: could not open output file %s\n",
		prog_name, data_name);
	    semantic_error = 1;
	}
	if (header_file == NULL){
	    fprintf(stderr, "%s: could not open header file %s\n",
		prog_name, header_name);
	    semantic_error = 1;
	}
	if (semantic_error){
	    cleanup(1);
	}

	// parse our input
	reset_scanner();
	yyparse();
	if (parse_haderr != 0){
		cleanup( parse_haderr );
	}
	if (semantic_error != 0 ){
		cleanup( semantic_error );
	}
	checksymbols();
	if (DEBUG(DUMPTABLES) ){
		printrules();
		printsymbols( verbose );
	}
	item_setup();
	close_symbols();
	if (DEBUG(PATTERNS) ){
		printitems();
	}
	if (DEBUG(CLOSURES) ){
		print_closures(verbose);
	}
	if (DEBUG(DUMPTABLES) ){
		printf("\nSTATES:\n");
	}
	compute_states();
	fflush(stderr);
	fflush(stdout);
	if (DEBUG(DUMPTABLES) ){
		printstateinversion();
	}
	if (semantic_error)
		cleanup(semantic_error);
	
	compress_output(); // DEBUG THIS SUKKER
	print_output_file( output_file, data_file, header_file, header_name );

	if (DEBUG(SETS)){
	       printsetstats();
	}
	cleanup(semantic_error);
}

void cleanup( int exitcode )
{
	if (output_file != NULL)
		fclose( output_file );
	if (exitcode != 0) {
		unlink( output_name );
		unlink( data_name );
		unlink( header_name );
	}
	exit( exitcode );
}

/*
 * called by the lexer to switch input file, if there are multiple input files
 * being read.
 */
extern "C" {
int
yywrap(){ 
    char * nextname = input_file_iterator.next();
    if ( nextname == 0 ){
	return 1;
    } else {
	input_name = nextname;
	yyin = fopen( input_name, "r");
	curlineno = 1;
	return 0;
    }
}
}
