/*
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

import javax.xml.namespace.QName;
import javax.xml.rpc.JAXRPCException;

/**
 * The class <code>Element</code> is a special Object
 * used to represent an xsd:element defined in a Web Service's
 * WSDL definition. An element can have the additional properties
 * of being an array, being nillable, and has minOccurs and
 * maxOccurs values.
 *
 * @version 0.1
 * @see javax.xml.rpc.Stub
 * @see javax.microedition.xml.rpc.Operation
 */
public class Element extends Type {

    /**
     * The QName of this element
     */
    public final QName name;

    /**
     * The Type of this Element's content.
     */
    public final Type contentType;

    /**
     * True if this element is nillable
     */
    public final boolean isNillable;

    /**
     * True if this element is an array
     */
    public final boolean isArray;

    /**
     * True if this element is optional, that is, its minOccurs
     * is defined as being 0.
     */
    public final boolean isOptional;

    /**
     * The 'minOccurs' attribute of this element. -1 if this
     * element has no 'minOccurs' attribute.
     */
    public final int minOccurs;

    /**
     * The 'maxOccurs' attribute of this element. -1 if this
     * element has no 'maxOccurs' attribute.
     */
    public final int maxOccurs;

    /**
     * Constant use to indicate that <code>maxOccurrs</code>
     * is unbounded.
     */
    public final static int UNBOUNDED = -1;

    /**
     * Construct an Element with the given properties.
     * This Type subclass will have an <code>intValue()</code> of 9.
     *
     * @param name the QName of this element
     * @param type the Type of this element's content
     * @param minOccurs indicates the minimum number of times this
     *                  element can occur. A value of '0' indicates
     *                  this element is optional.
     * @param maxOccurs indicates the maximum number of times this
     *                  element can occur. A value > 1, in addition
     *                  to <code>isArray</code> being true, indicates
     *                  this element is an array.
     * @throws IllegalArgumentException
     * <UL>
     * <LI>if <code>minOccurs < 0</code>, or
     * <LI>if <code>name</code> or <code>type</code> are <code>null</code> 
     * <LI>if <code>type</code> is an instance of <code>Element</code>
     * </UL>
     */
    public Element(QName name,
                   Type type,
                   int minOccurs,
                   int maxOccurs,
                   boolean nillable) throws IllegalArgumentException
    {
        super(9);

        if (name == null || type == null || type instanceof Element) {
            throw new IllegalArgumentException();
        }

        this.name = name;
        this.contentType = type;

        if (minOccurs < 0 || (maxOccurs <= 0 && maxOccurs != UNBOUNDED)) {
            throw new IllegalArgumentException("[min|max]Occurs must >= 0");
        } else if (maxOccurs < minOccurs && maxOccurs != UNBOUNDED) {
            throw new IllegalArgumentException("maxOccurs must > minOccurs");
        }
        this.minOccurs = minOccurs;
        this.isOptional = (minOccurs == 0);
        this.maxOccurs = maxOccurs;
        this.isArray = (maxOccurs > 1 || maxOccurs == UNBOUNDED);
        this.isNillable = nillable;
    }

   /**
     * Construct an Element with the given properties. The defaults for
     * the unspecified properties are:
     * <UL>
     * <LI>minOccurs = 1
     * <LI>maxOccurs = 1
     * <LI>isOptional = false
     * <LI>isArray = false
     * <LI>nillable = false
     * </UL>
     *
     * This Type subclass will have an <code>intValue()</code> of 9.
     *
     * @param name the QName of this element
     * @param type the Type of this element's content
     *
     * @throws IllegalArgumentException
     * <UL>
     * <LI>if <code>name</code> or <code>type</code> are <code>null</code> 
     * <LI>if <code>type</code> is an instance of <code>Element</code>
     * </UL>
     */
    public Element(QName name,
                   Type type) throws IllegalArgumentException

    {
        super(9);

        if (name == null || type == null || type instanceof Element) {
            throw new IllegalArgumentException();
        }

        this.name = name;
        this.contentType = type;
        this.minOccurs = 1;
        this.maxOccurs = 1;
        this.isArray = false;
        this.isOptional = false;
        this.isNillable = false;
    }
}
