/*
 * @(#)Set.java	1.14 06/10/10
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

/*
 * Set-of-objects
 * Add, delete, query, enumerate.
 * Simple.
 * This set implementation is best for small, often
 * empty sets, as used by the dependenceAnalysis classes.
 * It appears that a good fraction ( 1/3 to 1/2 ) of those
 * sets are empty, and most of the rest are pretty small.
 * That is the target use of this implementation.
 * 
 */
package util;
import java.util.Enumeration;
import java.util.Vector;

public class Set {

    static private final int defaultInitial = 10;

    int	  ninitial; 	// initial allocation for this one.
    Vector setData;

    public Set( int nin ){
	ninitial = nin;
    }

    public Set(){
	ninitial = defaultInitial;
    }

    public boolean isIn( Object o ){
	return (setData==null)?false:setData.contains( o );
    }

    public void addElement( Object o ){
	// see if its already in. If not,
	// add at end.
	if ( isIn( o ) ) return;

	// now add, extending if necessary.
	if ( setData == null ){
	    setData = new Vector( ninitial );
	}
	setData.addElement( o );
    }

    public void add( Object o ){ addElement( o ); }

    public Enumeration elements(){
	return (setData==null)?EmptyEnumeration.instance:setData.elements();
    }

    public void deleteAllElements(){
	setData = null;
    }

    public int size(){ return (setData==null)?0:setData.size(); } 

}
