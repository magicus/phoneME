/*
 * @(#)GetPropertyAction.java	1.13 06/10/10
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

package sun.security.action;

/**
 * A convenience class for retrieving the string value of a system
 * property as a privileged action.
 *
 * <p>An instance of this class can be used as the argument of
 * <code>AccessController.doPrivileged</code>.
 *
 * <p>The following code retrieves the value of the system
 * property named <code>"prop"</code> as a privileged action: <p>
 *
 * <pre>
 * String s = (String) java.security.AccessController.doPrivileged(
 *                         new GetPropertyAction("prop"));
 * </pre>
 *
 * @author Roland Schemers
 * @version 1.6, 02/02/00
 * @see java.security.PrivilegedAction
 * @see java.security.AccessController
 * @since JDK1.2
 */

public class GetPropertyAction implements java.security.PrivilegedAction {
    private String theProp;
    private String defaultVal;

    /**
     * Constructor that takes the name of the system property whose
     * string value needs to be determined.
     *
     * @param theProp the name of the system property.
     */
    public GetPropertyAction(String theProp) {
	this.theProp = theProp;
    }

    /**
     * Constructor that takes the name of the system property and the default
     * value of that property.
     *
     * @param theProp the name of the system property.
     * @param defaulVal the default value.
     */
    public GetPropertyAction(String theProp, String defaultVal) {
	this.theProp = theProp;
	this.defaultVal = defaultVal;
    }

    /**
     * Determines the string value of the system property whose
     * name was specified in the constructor.
     *
     * @return the string value of the system property,
     *         or the default value if there is no property with that key.
     */
    public Object run() {
	String value = System.getProperty(theProp);
	return (value == null) ? defaultVal : value;
    }
}
