/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.xml.rpc;

/**
 * The class <code>Type</code> is a type safe enumeration of allowable
 * types that are used to identify simple types defined in a Web Service's
 * WSDL definition. The set of simple types are:
 * <ul>
 *  <li>boolean</li>
 *  <li>byte</li>
 *  <li>short</li>
 *  <li>int</li>
 *  <li>long</li>
 *  <li>float</li>
 *  <li>double</li>
 *  <li>string</li>
 * </ul>
 *
 * @version 0.1
 * @see javax.xml.rpc.Stub
 * @see javax.microedition.xml.rpc.Operation
 */
public class Type {


    /**
     * Type constant identifying boolean parameters.
     * Given the integer value '0'.
     */
    public static final Type BOOLEAN                  = new Type(0);

    /**
     * Type constant identifying byte parameters.
     * Given the integer value '1'.
     */
    public static final Type BYTE                     = new Type(1);

    /**
     * Type constant identifying short parameters.
     * Given the integer value '2'.
     */
    public static final Type SHORT                    = new Type(2);

    /**
     * Type constant identifying integer parameters.
     * Given the integer value '3'.
     */
    public static final Type INT                      = new Type(3);

    /**
     * Type constant identifying long parameters.
     * Given the integer value '4'.
     */
    public static final Type LONG                     = new Type(4);

    /**
     * Type constant identifying float parameters.
     * Given the integer value '5'.
     */
    public static final Type FLOAT                    = new Type(5);

    /**
     * Type constant identifying double parameters.
     * Given the integer value '6'.
     */
    public static final Type DOUBLE                   = new Type(6);

    /**
     * Type constant identifying String parameters.
     * Given the integer value '7'.
     */
    public static final Type STRING                   = new Type(7);

    /**
     * The internal integer "value" of this Type, [0-8]
     */
    public final int value;

    /**
     * Package-private constructor.
     *
     * @param value the integer id for this Type object.
     */
    Type(int value) {
        this.value = value;
    }

}
