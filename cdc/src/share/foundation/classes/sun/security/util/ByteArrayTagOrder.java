/*
 * @(#)ByteArrayTagOrder.java	1.16 06/10/10
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


/**
 * ByteArrayTagOrder: a class for comparing two DER encodings by the 
 * order of their tags.
 *
 * @version 1.9 02/02/00
 * @author D. N. Hoover
 */

package sun.security.util;

import java.util.Comparator;

public class ByteArrayTagOrder implements Comparator {

    /**
     * Compare two byte arrays, by the order of their tags,
     * as defined in ITU-T X.680, sec. 6.4.  (First compare
     *  tag classes, then tag numbers, ignoring the constructivity bit.)
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

	// tag order is same as byte order ignoring any difference in 
	// the constructivity bit (0x02)
	return (bytes1[0] | 0x20) - (bytes2[0] | 0x20);
    }
	

}
