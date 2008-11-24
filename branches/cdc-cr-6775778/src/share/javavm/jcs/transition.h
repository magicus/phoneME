/*
 * @(#)transition.h	1.6 06/10/10
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

#ifndef __TRANSITIONH__
#define __TRANSITIONH__
#include <stdio.h>
class symbol;
class terminal_symbol;
class unary_symbol;
class binary_symbol;

/*
 * This file declares these classes
 *	generic_transition
 *	leaf_transition
 *	unary_transition
 *	binary_transition
 *
 * The definitions of state transition matrices.
 * In the most general (binary) case,
 * 	transition_table->state[left_child_state][right_child_state] 
 * gives the state number to be associated with the tree node.
 * We subset this in obvious ways for unary (one child) and
 * leaf i.e. nullary (no children) ops.
 *
 * children states are numbered starting at zero, of course,
 * and matricies are always square. All transition table entries
 * (even for the 0-dimensional ones) are initialized to the
 * distinguished value BAD_STATENO. Transition table entries should be
 * given values using the insert_stateno function defined
 * at the end of this file, which checks for the BAD_STATENO value. This
 * is in order to detect stray pointer addressing and re-evaluation
 * of transitions we thought were already evaluated.
 *
 * Transitions matricies are apparently only read by the print routine.
 *
 * Transition matricies are attached to symbol table entries.
 * The specific type of table is determined by looking
 * at the symbol's type, which also gives the symbol-table
 * variant to use.
 */

typedef unsigned short	stateno;
#define BAD_STATENO	0xffff /* immediately obviously bad, for debugging */
#define MAX_STATENO	0xfffe /* depends on type of stateno!! */


struct generic_transition {
	symbol *		op;
	/*
	 * print a transition (matrix or vector)
	 * for the production of pmatch, or for debugging.
	 * Needs a C++ type for the array (usually unsigned short).
	 * Takes a template for name construction, into which is plugged
	 * the string sn.
	 * And needs files to write into which to write a declaration
	 * of the transition data, and its definition, too.
	 *
	 * Usage:
	 *	unary_symbol * usp;
	 *	char * st = "unsigned short";
	 *	char * sn = "mult_s";
	 *	char * template = "pmatch_%s_transition";
	 *	extern FILE * code_file;
	 *	extern FILE * data_file;
	 *	usp->transitions->print(
	 *	    st, sn, template, code_file, data_file );
	 */
	virtual void
	print(
	    const char * statetype, const char * symbol_name,
	    const char * name_template ,FILE *output, FILE *data)
	    = 0;

	/*
	 * free allocated storage for this transition table.
	 * Usage:
	 *	unary_symbol * usp;
	 *	usp->transitions->free();
	 *
	 * Does not deallocate the transition struct itself, only
	 * the allocated tables.
	 * (probably not much used, as we exit the program
	 * after printing.)
	 */
	virtual void free(void) 
	    = 0;
};

/*
 * leaf transition: just add a scalar state number
 * Usage:
 *	terminal_symbol *tsp;
 *	stateno		 thisstate;
 *	insert_stateno( tsp->transitions->state , thisstate );
 */
struct leaf_transition : public generic_transition {
	stateno		state;

	/* 
	 * To create a null-ary transition (matrix).
	 * Usage:
	 *	terminal_symbol *  tsp;
	 *	tsp->transitions = leaf_transition::newtransition( tsp );
	 */
	static leaf_transition * newtransition( terminal_symbol * sp );

	virtual void
	print(
	    const char * statetype, const char * symbol_name,
	    const char * name_template ,FILE *output, FILE *data);

	virtual void free(void); // does nothing
};

/*
 * unary operator transition: has a vector of state numbers,
 * subscripted by state number. Need a size (maxstates) and 
 * a pointer to the actual data, which will be malloc'd using
 * the grow function.
 * Usage:
 *	unary_symbol *usp;
 *	stateno	     childstate, thisstate;
 *	insert_stateno( usp->transitions, childstate, thisstate );
 */
class unary_transition : public generic_transition {
	stateno			maxstates;
	stateno			maxstates_available;
public:
	stateno *		state;

	int			max_stateno(void){ return (int)maxstates-1; }

	/*
	 * To create a unary transition (matrix) having zero size.
	 * Usage:
	 *	unary_symbol *usp;
	 *	usp->transitions = unary_transition::newtransition( usp );
	 */
	static unary_transition * newtransition( unary_symbol * sp );

	/*
	 * To change the size of a transition matrix.
	 * causes the malloc'd part to be realloc'd as necessary.
	 * All newly-allocated vector entries are initialized to BAD_STATENO.
	 * Usage:
	 *	stateno max_state_no;
	 *	unary_symbol *usp;
	 *	usp->transitions->grow(max_state_no);
	 */
	void		grow( stateno newmax ); 

	virtual void
	print(const char * statetype, const char * symbol_name,
	    const char * name_template ,FILE *output, FILE *data);

	/*
	 * To delete state transitions.
	 * A client could do this by manipulating "state[n]"
	 * directly, but with less likely success. Will shrink
	 * the allowable number of transitions (max_stateno())
	 * but not actually deallocate memory.
	 * Input is a vector of booleans indicating the positions to be
	 * deleted. Compacting will be done, basically sliding down
	 * the rest of the vector to fill. Input vector same size
	 * as transition.
	 * Usage:
	 *	unary_transition * utp;
	 *	char *  delete_list; // 1=> delete, 0=>leave
	 *	int	old_maxstate;
	 *	int	n_deletions;
	 *	old_maxstate = utp->max_stateno();
	 *	utp->delete_transitions( delete_list );
	 *	assert( old_maxstate == utp->max_stateno()+n_deletions );
	 */
	void
	delete_transitions( const char * delete_list );

	virtual void free(void);
};

/*
 * binary operator transition: has a vector of vector of state numbers,
 * subscripted by state number. Need two size indicators
 * (nrows, ncols), since it need not be a square matrix.
 * There's also a pointer to the vector of
 * pointers to vectors of state numbers. All vectors are malloc'd.
 * Usage:
 *	binary_symbol *bsp;
 *	stateno	       thisstate, leftchild, rightchild;
 *	insert_stateno( tsp->transitions, leftchild, rightchild , thisstate );
 *	
 */
struct binary_transition_row {
	int			max_col_inserted;
	stateno *		state;
};

class binary_transition : public generic_transition {
	stateno			nrows;
	stateno			ncols;
	stateno			nrows_available;
	stateno			ncols_available;
public:
	binary_transition_row *	state;
	/*
	 * To create a unary transition (matrix) having zero size.
	 * Usage:
	 *	binary_symbol *bsp;
	 *	bsp->transitions = binary_transition::newtransition( bsp );
	 */
	static binary_transition * newtransition( binary_symbol * sp );

	/*
	 * To change the size of a transition matrix.
	 * Causes the malloc'd parts to be realloc'd and initialized as necessary.
	 * All newly-allocated stateno vector entries are initialized to BAD_STATENO.
	 * Usage:
	 *	int newsize;
	 *	binary_symbol *bsp;
	 *	bsp->transitions->grow(newsize);
	 */
	void		grow( stateno newrows, stateno newcols ); 

	/*
	 * To access the size of a rectangular transition matrix.
	 * This is to avoid the ever-prevailant confusion about
	 * number of rows/cols versus max index for a row/col.
	 * Usage:
	 *	int max_row_index;
	 *	binary_symbol *bsp
	 *	max_row_index = bsp->transitions->row_max_stateno();
	 *	if (max_row_index < 0 ){
	 *		... // empty matrix!
	 *	}
	 * And similarly for col_max_stateno.
	 */
	int row_max_stateno(void) { return nrows-1;}
	int col_max_stateno(void) { return ncols-1;}

	virtual void
	print(const char * statetype, const char * symbol_name,
	    const char * name_template ,FILE *output, FILE *f);

	/*
	 * To delete state transitions.
	 * A client could do this by manipulating "state[n]"
	 * directly, but with less likely success. Will shrink
	 * the allowable number of transitions (row_max_stateno() or
	 * col_max_stateno() ) but not actually deallocate memory.
	 * Input is a vector of booleans indicating the positions to be
	 * deleted. Compacting will be done, basically sliding down
	 * the rest of the vector to fill. Input vector same size
	 * as transition.
	 * Usage:
	 *	binary_transition * btp;
	 *	char *  col_delete_list[100]; // 1=> delete, 0=>leave
	 *	char *  row_delete_list[100]; // 1=> delete, 0=>leave
	 *	int 	n_col_deletions;
	 *	int 	n_row_deletions;
	 *	int	old_maxcol;
	 *	int	old_maxrow;
	 *	old_maxcol = btp->col_max_stateno();
	 *	old_maxrow = btp->row_max_stateno();
	 *	btp->delete_transition_cols( col_delete_list );
	 *	btp->delete_transition_rows( row_delete_list );
	 *	assert( old_maxcol == btp->col_max_stateno()+n_col_deletions );
	 *	assert( old_maxrow == btp->row_max_stateno()+n_row_deletions );
	 */
	void
	delete_transition_cols( const char * delete_list );
	void
	delete_transition_rows( const char * delete_list );

	virtual void free(void);
};

void
insert_stateno(
	unary_transition *target,
	int subscript,
	int value );

void
insert_stateno(
	leaf_transition *target,
	int value );

void
insert_stateno(
	binary_transition *target,
	int subscript1,
	int subscript2,
	int value );

void
print_statenumber( FILE *f, stateno n, const char * trailer );

#endif
