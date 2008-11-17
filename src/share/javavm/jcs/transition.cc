/*
 * @(#)transition.cc	1.8 06/10/10
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

// @(#)transition.cc       2.23     98/07/03 
#include "transition.h"
#include "symbol.h"
#include "globals.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define ENTRIES_PER_LINE 8

int
find_error_state(void);


/*
 * transition matrix management.
 * create em. make em bigger when necessary,
 * print em.
 *
 * A note on the meaning of "maxstates". This is
 * the NUMBER of rows or columns, as opposed to
 * the MAXIMUM INDEX of these rows or columns.
 * If we want to be able to index up to, say, n,
 * then we had better allocate a thing with maxstates = n+1.
 * Zero-based addressing strikes again.
 */

struct leaf_transition *
leaf_transition::newtransition( terminal_symbol * sp )
{
	struct leaf_transition * ltp;
	assert( sp->functional_type() == symbol::terminal );
	ltp = new (leaf_transition);
	ltp->op =   sp;
	ltp->state = BAD_STATENO;
	return ltp;
}

void
leaf_transition::print(const char * statetype, const char * symbol_name,
	    const char * name_template ,FILE *output, FILE *f)
{
	if ( this == 0 ) return;
	assert( op->functional_type() == symbol::terminal );
	print_statenumber( f, state,"");
}

void
leaf_transition::free(void)
{
	return;
}

////////////////////////////////////////////////////////////////

struct unary_transition *
unary_transition::newtransition( unary_symbol * sp )
{
	struct unary_transition *utp;
	assert( (sp->functional_type() == symbol::unaryop) || 
		(sp->functional_type() == symbol::rightunaryop) );
	utp = new (unary_transition);
	utp->op =   sp;
	utp->maxstates = 0;
	utp->state = 0;
	return utp;
}

void
unary_transition::print(
	const char * statetype,
	const char * symbol_name,
	const char * name_template,
	FILE *output,
	FILE *data)
{
	if ( this == 0 ) return;

	register stateno i;
	register stateno maxminus1 = max_stateno();
	register int count = 0;

	assert( (op->functional_type() == symbol::unaryop) ||
		(op->functional_type() == symbol::rightunaryop) );
	fprintf(output, "extern %s\t", statetype);
	fprintf(output, name_template, symbol_name );
	fprintf(output, "[%d];\n", maxstates);
	fprintf(data, "%s\n", statetype);
	fprintf(data, name_template, symbol_name );
	fprintf(data, "[%d] =\n", maxstates);
	fprintf(data, "{ ");
	for (i=0; i<maxminus1; i++) {
	    print_statenumber( data, state[i], ",");
	    count += 1;
	    if (count % 20 == 0) {
		fputs("\n", data);
	    }
	}
	print_statenumber(data, state[i], " };\n\n");
}

void
unary_transition::grow( stateno newmax_index )
{
    stateno *sp, *lastp;
    stateno newmax = newmax_index+1;

    assert( newmax >= maxstates );
    if( newmax == maxstates ){
	// trivial "growth"
	return;
    }
    if (maxstates == 0 ){
	// first allocation
	state = (stateno *)malloc( newmax * sizeof(stateno) );
	maxstates_available = newmax;
    } else if ( newmax > maxstates_available) {
	// already existing, just too small.
	state = (stateno *)realloc( (char *) state, newmax * sizeof(stateno ));
	maxstates_available = newmax;

    }
    assert(state != 0); // make sure no malloc failures.
    // set new parts of vector to illegal value.
    for ( sp = &state[maxstates], lastp= &state[newmax-1]; sp <= lastp; sp++ )
	*sp = BAD_STATENO;
    // install new size
    maxstates = newmax;
}

/*
 * To delete state transitions.
 * A client could do this by manipulating "state[n]"
 * directly, but with less likely success. Will shrink
 * the allowable number of transitions (max_stateno())
 * but not actually deallocate memory.
 * Input is a vector of booleans indicating positions to be
 * deleted. Compacting will be done, basically sliding down
 * the rest of the vector to fill.
 * Usage:
 *	unary_transition * utp;
 *	stateno delete_list[100];
 *	int 	n_deletions;
 *	int	old_maxstate;
 *	old_maxstate = utp->max_stateno();
 *	utp->delete_transitions( delete_list, n_deletions );
 *	assert( old_maxstate == utp->max_stateno()+n_deletions );
 */
void
unary_transition::delete_transitions(
	const char * delete_list )
{
	// this is the easiest of the choices, given our
	// current representation.
	register stateno * row;
	register int src, dest;
	register int max;
	int ndel;
	row = state;
	max = maxstates-1;

	// iterate over all "src" transitions, deciding which to copy.
	ndel = 0;
	for ( src = 0, dest = 0; src <= max ; src ++ ){
		if ( delete_list[ src ] == 0 ){
			// simply copy
			row[ dest++ ] = row[ src ];
		} else {
			// don't copy, but tally deletions
			ndel += 1;
		}
	}
	maxstates -= ndel;
}

void
unary_transition::free(void)
{
	if (state)
		::free((char*)state);
	state = 0;
}

////////////////////////////////////////////////////////////////

struct binary_transition *
binary_transition::newtransition( binary_symbol * sp )
{
	struct binary_transition *btp;
	assert( sp->type() == symbol::binaryop );
	btp = new (binary_transition);
	btp->op =   sp;
	btp->nrows = 0;
	btp->ncols = 0;
	btp->state = 0;
	return btp;
}

void
binary_transition::print(
	const char * statetype,
	const char * symbol_name,
	const char * name_template,
	FILE *output,
	FILE *data)
{
	if ( this == 0 ) return;

	register stateno i,j;
	register int count;

	assert( op->type() == symbol::binaryop );
	fprintf(output, "extern %s\t", statetype);
	fprintf(output, name_template, symbol_name );
	fprintf(output, "[%d][%d];\n", nrows, ncols);
	fprintf(data, "%s\n", statetype);
	fprintf(data, name_template, symbol_name );
	fprintf(data, "[%d][%d] =\n", nrows, ncols);
	fprintf( data, "{");
	for (i=0; i<nrows; i++){
	    fprintf(data, "{");
	    count = 0;
	    for (j=0; j<(unsigned)ncols-1; j++){
		print_statenumber( data, state[i].state[j], ",");
		count += 1;
		if (count % 20 == 0) {
		    fputs("\n", data);
		}
	    }
	    if (i<(unsigned)nrows-1)
		print_statenumber( data, state[i].state[j], "},\n");
	    else
		print_statenumber( data, state[i].state[j], "}\n");
	}
	fprintf( data, "};\n\n");
}

void
binary_transition::grow( stateno newmax_rows, stateno newmax_cols )
{
    stateno i;
    stateno *sp, *lastp;
    stateno new_nrows = newmax_rows + 1;
    stateno new_ncols = newmax_cols + 1;

    assert( new_nrows >= nrows );
    assert( new_ncols >= ncols );
    if (nrows == 0 ){
	// first allocation
	state = (binary_transition_row*)malloc( new_nrows * sizeof(binary_transition_row) );
	assert(state != 0); // make sure no malloc failures.
	for (i=0; i<new_nrows; i++){
	    state[i].state = (stateno *)malloc( new_ncols * sizeof(stateno) );
	    assert( state[i].state != 0);
	}
	nrows_available = new_nrows;
	ncols_available = new_ncols;
    } else if ( new_nrows > nrows_available || new_ncols > ncols_available ){
	// already existing, but too small.
	state = (binary_transition_row*)realloc( (char *) state, new_nrows * sizeof(binary_transition_row));
	assert(state != 0); // make sure no malloc failures.
	for (i=0; i<new_nrows; i++){
	    if (i<nrows ){
		// preexisting--realloc
		state[i].state = (stateno *)realloc( (char *)(state[i].state), new_ncols*sizeof(stateno) );
	    } else {
		// new -- malloc
		state[i].state = (stateno *)malloc(  new_ncols*sizeof(stateno) );
	    }
	    assert( state[i].state != 0);
	}
	nrows_available = new_nrows;
	ncols_available = new_ncols;
    }
    // initialize the new parts of the vectors.
    for (i=0; i<new_nrows; i++){
	if (i<nrows){
	    // illegalize end of old vector.
	    sp=&(state[i].state[ncols]);
	} else {
	    // brand new vector -- zap it altogether
	    state[i].max_col_inserted = -1;
	    sp=&(state[i].state[0]);
	}
	lastp = &(state[i].state[new_ncols-1]);
	for (sp, lastp ; sp<=lastp; sp++)
	    *sp = BAD_STATENO;
    }
    // install new size
    nrows = new_nrows;
    ncols = new_ncols;
}


/*
 * To delete state transitions.
 * A client could do this by manipulating "state[n]"
 * directly, but with less likely success. Will shrink
 * the allowable number of transitions (row_max_stateno() or
 * col_max_stateno() ) but not actually deallocate memory.
 * Input is a vector of booleans indicating positions to be
 * deleted. Compacting will be done, basically sliding down
 * the rest of the vector to fill.
 * Usage:
 *	binary_transition * btp;
 *	stateno col_delete_list[100];
 *	stateno row_delete_list[100];
 *	int 	n_col_deletions;
 *	int 	n_row_deletions;
 *	int	old_maxcol;
 *	int	old_maxrow;
 *	old_maxcol = btp->col_max_stateno();
 *	old_maxrow = btp->row_max_stateno();
 *	btp->delete_transition_cols(
 *		col_delete_list, n_col_deletions );
 *	btp->delete_transition_rows(
 *		row_delete_list, n_row_deletions );
 *	assert( old_maxcol == btp->col_max_stateno()+n_col_deletions );
 *	assert( old_maxrow == btp->row_max_stateno()+n_row_deletions );
 */

void
binary_transition::delete_transition_cols(
	const char * delete_list )
{
	// delete given columns from each row.
	binary_transition_row * row_vector;
	stateno * row;
	int nrow, rowmax;
	register int src_col, dest_col;
	register int colmax;
	int ndel;
	row_vector = state;
	rowmax = nrows-1;
	colmax = ncols-1;
	// iterate over all rows.
	for( nrow = 0 ; nrow <= rowmax; nrow ++ ){
		// iterate over all "src" cols
		ndel = 0;
		row = row_vector[nrow].state;
		for ( src_col = 0, dest_col = 0; src_col <= colmax ; src_col ++ ){
			if ( delete_list[ src_col ] == 0 ){
				// simply copy
				row[ dest_col++ ] = row[ src_col ];
			} else {
				// do not copy this row.
				// instead, tally our deletions
				ndel += 1;
			}
		}
	}
	ncols -= ndel;
}

void
binary_transition::delete_transition_rows(
	register const char * delete_list )
{
	// this is the easier of the choices, given our
	// current representation.
	register binary_transition_row * row_vector;
	register int src_row, dest_row;
	register int rowmax;
	int ndel;
	row_vector = state;
	rowmax = nrows-1;

	// iterate over all "src" rows, deciding which to copy.
	// those dropped must be deleted, unless they're at the end.
	ndel = 0;
	for ( src_row = 0, dest_row = 0; src_row <= rowmax ; src_row ++ ){
		if ( delete_list[ src_row ] == 0 ){
			// simply copy
			row_vector[ dest_row++ ] = row_vector[ src_row ];
		} else {
			// do not copy this row.
			// instead, delete it, tally it.
			::free( (char*)row_vector[ src_row ].state );
			ndel += 1;
		}
	}
	// just for safety. zero out all the state pointers at end.
	for( src_row = nrows-ndel; src_row < nrows ; src_row++)
		row_vector[ src_row ].state = 0;
	nrows -= ndel;
}

void
binary_transition::free(void)
{
	if (state){
		register unsigned i;
		for ( i = 0 ; i < nrows_available ; i ++ ){
			if ( state[i].state )
				::free( (char*)state[i].state );
		}
		::free((char*)state);
	}
	state = 0;
}

/*
 * Debugging check on transition table assignment.
 * Make sure the entry we're assigning to hasn't been
 * assigned to before, and try to assure that its in
 * a transition table.
 * See above examples of usage
 */
void
insert_stateno(
	register unary_transition *target,
	register int subscript,
	register int value )
{
	assert( (unsigned)subscript <= target->max_stateno() );
	assert( target->state[subscript] == BAD_STATENO);
	assert( (0 <= value ) && ( value <= MAX_STATENO));
	target->state[subscript] = value;
}

void
insert_stateno(
	register leaf_transition *target,
	register int value )
{
	assert( target->state == BAD_STATENO);
	assert( (0 <= value ) && ( value <= MAX_STATENO));
	target->state = value;
}

void
insert_stateno(
	register binary_transition *target,
	register int subscript1,
	register int subscript2,
	register int value )
{
	assert( (unsigned)subscript1 <= target->row_max_stateno() );
	assert( (unsigned)subscript2 <= target->col_max_stateno() );
	assert( target->state[subscript1].state[subscript2] == BAD_STATENO);
	assert( (0 <= value ) && ( value <= MAX_STATENO));
	if ( subscript2 > target->state[subscript1].max_col_inserted)
		target->state[subscript1].max_col_inserted = subscript2;
	target->state[subscript1].state[subscript2] = value;
}

void
print_statenumber( FILE *f, stateno n, const char * trailer )
{
	if (n == BAD_STATENO)
		fprintf(f, "BAD%s", trailer);
	else
		fprintf(f, "%d%s",n, trailer);
}
