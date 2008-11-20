/*
 * @(#)ByteArrayLexOrder.java	1.16 06/10/10
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


package sun.security.util;

import java.util.Comparator;

/**
 * Compare two byte arrays in lexicographical order.
 *
 * @version 1.9 02/02/00
 * @author D. N. Hoover
 */
public class ByteArrayLexOrder implements Comparator {

    /**
     * Perform lexicographical comparison of two byte arrays,
     * regarding each byte as unsigned.  That is, compare array entries 
     * in order until they differ--the array with the smaller entry 
     * is "smaller". If array entries are 
     * equal till one array ends, then the longer array is "bigger".
     *
     * @param  obj1 first byte array to compare.
     * @param  obj2 second byte array to compare.
     * @return negative number if obj1 < obj2, 0 if obj1 == obj2, 
     * positive number if obj1 > obj2.  
     *
     * @exception <code>ClassCastException</code> 
     * if either argument is not a byte array.
     */
    public final int compare(Object obj1, Object obj2) {

	byte[] bytes1 = (byte[]) obj1;
	byte[] bytes2 = (byte[]) obj2;

	int diff;
	for (int i = 0; i < bytes1.length && i < bytes2.length; i++) {
	    diff = (bytes1[i] & 0xFF) - (bytes2[i] & 0xFF);
	    if (diff != 0) {
		return diff;
	    }
	}
	// if array entries are equal till the first ends, then the
	// longer is "bigger"
	return bytes1.length - bytes2.length;
    }
	

}
