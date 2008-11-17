/*
 * @(#)InvalidParameterException.java	1.22 06/10/10
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

package java.security;

/**
 * This exception, designed for use by the JCA/JCE engine classes, 
 * is thrown when an invalid parameter is passed 
 * to a method.
 *
 * @author Benjamin Renaud
 * @version 1.15, 00/02/02
 */

public class InvalidParameterException extends IllegalArgumentException {

    /**
     * Constructs an InvalidParameterException with no detail message.
     * A detail message is a String that describes this particular
     * exception.
     */
    public InvalidParameterException() {
	super();
    }

    /**
     * Constructs an InvalidParameterException with the specified
     * detail message.  A detail message is a String that describes
     * this particular exception.
     *
     * @param msg the detail message.  
     */
    public InvalidParameterException(String msg) {
	super(msg);
    }
}
