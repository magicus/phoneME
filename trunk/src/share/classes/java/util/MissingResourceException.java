/*
 * 
 * @(#)MissingResourceException.java	1.19 06/10/10
 * 
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */

/*
 * (C) Copyright Taligent, Inc. 1996, 1997 - All Rights Reserved
 * (C) Copyright IBM Corp. 1996 - 1998 - All Rights Reserved
 *
 * The original version of this source code and documentation
 * is copyrighted and owned by Taligent, Inc., a wholly-owned
 * subsidiary of IBM. These materials are provided under terms
 * of a License Agreement between Taligent and Sun. This technology
 * is protected by multiple US and International patents.
 *
 * This notice and attribution to Taligent may not be removed.
 * Taligent is a registered trademark of Taligent, Inc.
 *
 */

package java.util;

/**
 * Signals that a resource is missing.
 * @see java.lang.Exception
 * @see ResourceBundle
 * @version     1.12, 01/19/00
 * @author      Mark Davis
 * @since       JDK1.1
 */
public
class MissingResourceException extends RuntimeException {

    /**
     * Constructs a MissingResourceException with the specified information.
     * A detail message is a String that describes this particular exception.
     * @param s the detail message
     * @param className the name of the resource class
     * @param key the key for the missing resource.
     */
    public MissingResourceException(String s, String className, String key) {
        super(s);
        this.className = className;
        this.key = key;
    }

    /**
     * Gets parameter passed by constructor.
     *
     * @return the name of the resource class
     */
    public String getClassName() {
        return className;
    }

    /**
     * Gets parameter passed by constructor.
     *
     * @return the key for the missing resource
     */
    public String getKey() {
        return key;
    }

    //============ privates ============

    /**
     * The class name of the resource bundle requested by the user.
     * @serial
     */
    private String className;

    /**
     * The name of the specific resource requested by the user.
     * @serial
     */
    private String key;
}
