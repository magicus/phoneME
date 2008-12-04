/*
 * @(#)ObjectStreamField.java	1.35 02/01/03
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

package java.io;

import java.lang.reflect.Field;

/* Back-ported JDK 1.4 implementation */

/**
 * A description of a Serializable field from a Serializable class.
 * An array of ObjectStreamFields is used to declare the Serializable
 * fields of a class.
 *
 * @author	Mike Warres
 * @author	Roger Riggs
 * @version 1.38, 01/07/25
 * @see ObjectStreamClass
 * @since 1.2
 */
public class ObjectStreamField implements Comparable {

    /** field name */
    private final String name;
    /** canonical JVM signature of field type */
    private final String signature;
    /** field type (Object.class if unknown non-primitive type) */
    private final Class type;

    // For JDK 1.4 back-port
    private char typeCode;

    /** whether or not to (de)serialize field values as unshared */
    private final boolean unshared;
    /** corresponding reflective field object, if any */
    private final Field field;
    /** offset of field value in enclosing field group */
    private int offset = 0;

    /**
     * Create a Serializable field with the specified type.
     * This field should be documented with a <code>serialField</code>
     * tag. 
     *
     * @param n the name of the serializable field
     * @param clazz the <code>Class</code> object of the serializable field
    */
    public ObjectStreamField(String name, Class type) {
	this(name, type, false);
    }
    
    /**
     * Creates an ObjectStreamField representing a serializable field with the
     * given name and type.  If unshared is false, values of the represented
     * field are serialized and deserialized in the default manner--if the
     * field is non-primitive, object values are serialized and deserialized as
     * if they had been written and read by calls to writeObject and
     * readObject.  If unshared is true, values of the represented field are
     * serialized and deserialized as if they had been written and read by
     * calls to writeUnshared and readUnshared.
     *
     * @param   name field name
     * @param   type field type
     * @param   unshared if false, write/read field values in the same manner
     *          as writeObject/readObject; if true, write/read in the same
     *          manner as writeUnshared/readUnshared
     */
    // Made package-private for JDK 1.4 back-port.
    public ObjectStreamField(String name, Class type, boolean unshared) {
	if (name == null) {
	    throw new NullPointerException();
	}
	this.name = name;
	this.type = type;
	this.unshared = unshared;
	signature = ObjectStreamClass.getClassSignature(type).intern();
	field = null;
	
	// For JDK 1.4 back-port 
	typeCode = signature.charAt(0);
    }

    /**
     * Creates an ObjectStreamField representing a field with the given name,
     * signature and unshared setting.
     */
    ObjectStreamField(String name, String signature, boolean unshared) {
	if (name == null) {
	    throw new NullPointerException();
	}
	this.name = name;
	this.signature = signature.intern();
	this.unshared = unshared;
	field = null;

	// For JDK 1.4 back-port 
	typeCode = signature.charAt(0);
	
	switch (signature.charAt(0)) {
	    case 'Z': type = Boolean.TYPE; break;
	    case 'B': type = Byte.TYPE; break;
	    case 'C': type = Character.TYPE; break;
	    case 'S': type = Short.TYPE; break;
	    case 'I': type = Integer.TYPE; break;
	    case 'J': type = Long.TYPE; break;
	    case 'F': type = Float.TYPE; break;
	    case 'D': type = Double.TYPE; break;
	    case 'L':
	    case '[': type = Object.class; break;
	    default: throw new IllegalArgumentException("illegal signature");
	}
    }
    
    /**
     * Creates an ObjectStreamField representing the given field with the
     * specified unshared setting.  For compatibility with the behavior of
     * earlier serialization implementations, a "showType" parameter is
     * necessary to govern whether or not a getType() call on this
     * ObjectStreamField (if non-primitive) will return Object.class (as
     * opposed to a more specific reference type).
     */
    ObjectStreamField(Field field, boolean unshared, boolean showType) {
	this.field = field;
	this.unshared = unshared;
	name = field.getName();
	Class ftype = field.getType();
	type = (showType || ftype.isPrimitive()) ? ftype : Object.class;
	signature = ObjectStreamClass.getClassSignature(ftype).intern();

	// For JDK 1.4 back-port 
	typeCode = signature.charAt(0);
    }

    /**
     * Get the name of this field.
     *
     * @return a <code>String</code> representing the name of the serializable
     * field 
     */
    public String getName() {
	return name;
    }

    /**
     * Get the type of the field.
     *
     * @return	the <code>Class</code> object of the serializable field 
     */
    public Class getType() {
	return type;
    }

    /** 
     * Returns character encoding of field type.  The encoding is as follows:
     * <blockquote><pre>
     * B            byte
     * C            char
     * D            double
     * F            float
     * I            int
     * J            long
     * L            class or interface
     * S            short
     * Z            boolean
     * [            array
     * </pre></blockquote>
     *
     * @return	the typecode of the serializable field
     */
    // NOTE: deprecate?
    public char getTypeCode() {
	return signature.charAt(0);
    }

    /**
     * Return the JVM type signature.
     *
     * @return	null if this field has a primitive type.
     */
    // NOTE: deprecate?
    public String getTypeString() {
	return isPrimitive() ? null : signature;
    }

    /**
     * Offset of field within instance data.
     *
     * @return	the offset of this field
     * @see #setOffset
     */
    // NOTE: deprecate?
    public int getOffset() {
	return offset;
    }

    /** 
     * Offset within instance data.
     *
     * @param	offset the offset of the field
     * @see #getOffset
     */
    // NOTE: deprecate?
    protected void setOffset(int offset) {
	this.offset = offset;
    }

    /**
     * Return true if this field has a primitive type.
     *
     * @return	true if and only if this field corresponds to a primitive type
     */
    // NOTE: deprecate?
    public boolean isPrimitive() {
	char tcode = signature.charAt(0);
	return ((tcode != 'L') && (tcode != '['));
    }
    
    /**
     * Returns boolean value indicating whether or not the serializable field
     * represented by this ObjectStreamField instance is unshared.
     */
    boolean isUnshared() {  // changed to package-private for JDK 1.4 back-port 
	return unshared;
    }

    /**
     * Compare this field with another <code>ObjectStreamField</code>.
     * Return -1 if this is smaller, 0 if equal, 1 if greater.
     * Types that are primitives are "smaller" than object types.
     * If equal, the field names are compared.
     */
    // NOTE: deprecate?
    public int compareTo(Object obj) {
	ObjectStreamField other = (ObjectStreamField) obj;
	boolean isPrim = isPrimitive();
	if (isPrim != other.isPrimitive()) {
	    return isPrim ? -1 : 1;
	}
	return name.compareTo(other.name);
    }

    /**
     * Return a string that describes this field.
     */
    public String toString() {
	return signature + ' ' + name;
    }
    
    /**
     * Returns field represented by this ObjectStreamField, or null if
     * ObjectStreamField is not associated with an actual field.
     */
    Field getField() {
	return field;
    }
    
    /**
     * Returns JVM type signature of field (similar to getTypeString, except
     * that signature strings are returned for primitive fields as well).
     */
    String getSignature() {
	return signature;
    }
}
