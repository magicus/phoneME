/*
 * @(#)statemap.cc	1.7 06/10/10
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

#include "statemap.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
/*
 * @(#)statemap.cc	1.9	95/03/14
 *
 * statemap: an expandable vector for mapping a large set of
 * states into a smaller one. Used for transition matrix compression.
 * i.e. maps statenumber => statenumber.
 */

// class statemap {
// 	int	max_element;
// 	stateno  *	array;
// 
// make map bigger as number of states grows.
void 
statemap::grow( stateno new_max )
{
	int newsize = (new_max+1)*sizeof(stateno);
	unsigned i;
	if ( max_element < 0 )
		array = (stateno*)malloc( newsize );
	else
		array = (stateno*)realloc( (char *)array, newsize );
	assert( array != 0 );
	for( i= max_element+1; i <= new_max; i++ )
		array[i] = BAD_STATENO;
	max_element = new_max;
}

// add a mapping
void
statemap::new_mapping( stateno index, stateno value )
{
	assert( index <= (unsigned)max_element );
	array[index] = value;
	if ( value > maxv )
		maxv = value;
}

// do lookup.
stateno
statemap::operator[]( stateno index )
{
	assert( index <= (unsigned)max_element );
	return array[index];
}

// squeeze map. When rows or columns are
// removed from a transition table, the map
// has to change, too.
// Remap(m, n) causes all map entries currently referencing
// m to reference n instead. And it causes all map entries currently
// referencing entries BIGGER than m to be diminished by one.
void
statemap::remap( register stateno oldno, register stateno tono )
{
	register stateno * vec = array;
	register int i;
	register int v, vmax;
	register int imax = max_element;
	vmax = 0;
	for( i = 0; i <= max_element; i++){
		v = vec[i];
		if ( v == oldno )
			vec[i]  = v = tono;
		else if ( v > oldno )
			vec[i]  = v = v-1;
		if ( v > vmax )
			vmax = v;
	}
	maxv = vmax; // in case we deleted it.
}

void
statemap::destruct(void)
{
	if (array != 0 ){
		free((char*)array);
		array = 0;
		max_element=-1;
		maxv=0;
	}
}

void
statemap::print(
	    const char * type_name,
	    const char * operator_name,
	    const char * qualifier,
	    const char * name_template,
	    FILE * output,
	    FILE *data )
{
	register int i;
	register int count = 0;

	fprintf( output, "extern %s\t", type_name );
	fprintf( output, name_template, operator_name, qualifier );
	fprintf( output, "[ %d ];\n", max_element+1 );
	fprintf( data, "%s\n", type_name );
	fprintf( data, name_template, operator_name, qualifier );
	fprintf( data, "[ %d ] = {\n", max_element+1 );
	if ( max_element >= 0 ){
		for( i = 0; i < max_element; i++ ){
			print_statenumber( data, array[i], "," );
			count += 1;
			if (count % 20 == 0) {
				fputs( "\n", data );
			}
		}
		print_statenumber( data, array[max_element], "\n" );
	}
	fputs( "};\n", data );
}

// for debugger.
void
print_statemap( statemap * sp)
{
	sp->print( "STATE", "CUROP", "", "%s_map", stdout, stdout );
}

