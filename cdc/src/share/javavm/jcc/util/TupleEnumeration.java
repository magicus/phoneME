/*
 * @(#)TupleEnumeration.java	1.4 06/10/10
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

package util;
import java.util.Enumeration;
import java.util.NoSuchElementException;

public
class TupleEnumeration implements Enumeration {
    Enumeration enums[];
    Enumeration cur;
    int i;
    int iMax;

    public TupleEnumeration(Enumeration a, Enumeration b){
	enums = new Enumeration[2];
	enums[0] = a;
	enums[1] = b;
	cur = enums[0];
	i = 0;
	iMax = 1;
    }

    public TupleEnumeration(Enumeration arrayOfE[]){
	enums = arrayOfE;
	cur = enums[0];
	i = 0;
	iMax = enums.length-1;
    }

    public boolean hasMoreElements(){ 
	if (cur.hasMoreElements()){
	    return true;
	}
	while (++i <= iMax){
	    cur = enums[i];
	    if (cur.hasMoreElements()){
		return true;
	    }
	}
	return false;
    }
	
    public Object nextElement(){ 
	NoSuchElementException caught;
	try {
	    return cur.nextElement();
	}catch(NoSuchElementException e){
	    caught = e;
	}
	while (++i <= iMax){
	    cur = enums[i];
	    try {
		return cur.nextElement();
	    }catch(NoSuchElementException e){
		caught = e;
	    }
	}
	throw caught;
    }

}
