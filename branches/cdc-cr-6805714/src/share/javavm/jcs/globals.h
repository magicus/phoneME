/*
 * @(#)globals.h	1.9 06/10/10
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

// @(#)globals.h       1.6     93/10/25 
/*
 * file of global variables exported, without control,
 * hither and yon, for use throughout this program, whatever its
 * called.
 * Much (all?) of it is global state set up while parsing input,
 * and used elsewhere. It is of little enough consequence not
 * to seem to require more structure.
 */

extern int	curlineno;	// current input line number: for error reporting.
				// exported by: 	tbl.y
extern char *	cgname;		// %name operand, for output generation
				// exported by:		tbl.y
extern char *	goalname;	// %goal operand, for output generation
				// exported by:		tbl.y
extern char *	nodetype;	// %type operand, for output generation
				// exported by:		tbl.y
extern char *	opfn;		// %opecode operand, for output generation
				// exported by:		tbl.y
extern char *	rightfn;	// %right operand, for output generation
				// exported by:		tbl.y
extern char *	leftfn;		// %left operand, for output generation
				// exported by:		tbl.y
extern char *	getfn;		// %getstate operand, for output generation
				// exported by:		tbl.y
extern char *	setfn;		// %setstate operand, for output generation
				// exported by:		tbl.y
extern int	is_dag;		// non-zero if %dag token seen
				// exported by:		tbl.y
extern int	parse_haderr;	// state of parser: 0=>ok
				// exported by:		tbl.y
extern int	semantic_error;	// state of meaning: 0=>ok
				// exported by:		main.C
extern const char *input_name;	// input file name (for debugging)
				// exported by:		main.C
extern FILE *	output_file;	// file upon which to write program fragment.
				// exported by:		main.C
extern int	error_folding;	// flag signaling whether folding (don't caring)
				// of error states is acceptable
				// exported by:		main.C
extern int	do_attributes;	// flag signaling whether we generate code to
				// manipulate synthetic and inhereted attributes
				// (such as for register targeting)
extern int	max_rule_arity; // max number of nontermials in any rule we see.
