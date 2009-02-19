/*
 * @(#)MalformedInputException.java	1.16 06/10/10
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

package sun.io;


/**
* The input string or input byte array to a character conversion
* contains a malformed sequence of characters or bytes.
*
* @author Asmus Freytag
*/
public class MalformedInputException
    extends java.io.CharConversionException
{
    /**
     * Constructs a MalformedInputException with no detail message.
     * A detail message is a String that describes this particular exception.
     */
    public MalformedInputException() {
	super();
    }

    /**
     * Constructs a MalformedInputException with the specified detail message.
     * A detail message is a String that describes this particular exception.
     * @param s the String containing a detail message
     */
    public MalformedInputException(String s) {
	super(s);
    }
}
