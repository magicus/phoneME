/*
 * @(#)KeyManagementException.java	1.18 06/10/10
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
 * This is the general key management exception for all operations
 * dealing with key management. Examples of subclasses of 
 * KeyManagementException that developers might create for 
 * giving more detailed information could include:
 *
 * <ul>
 * <li>KeyIDConflictException
 * <li>KeyAuthorizationFailureException
 * <li>ExpiredKeyException
 * </ul>
 *
 * @version 1.11 00/02/02
 * @author Benjamin Renaud
 *
 * @see Key
 * @see KeyException
 */

public class KeyManagementException extends KeyException {

    /**
     * Constructs a KeyManagementException with no detail message. A
     * detail message is a String that describes this particular
     * exception.
     */
    public KeyManagementException() {
	super();
    }

     /**
     * Constructs a KeyManagementException with the specified detail
     * message. A detail message is a String that describes this
     * particular exception.  
     *
     * @param msg the detail message.  
     */
   public KeyManagementException(String msg) {
	super(msg);
    }
}
