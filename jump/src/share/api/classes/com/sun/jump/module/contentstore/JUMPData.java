/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jump.module.contentstore;

/**
 * <code>JUMPData</code> encapsulates the value stored in the persistant store.
 * This class consists of the type of the value and the actual value.
 */
public class JUMPData {
    public static final int FORMAT_UNKNOWN = -1;
    public static final int FORMAT_STRING  = 0;
    public static final int FORMAT_INT     = 1;
    public static final int FORMAT_FLOAT   = 2;
    public static final int FORMAT_BOOLEAN = 3;
    public static final int FORMAT_BYTES   = 4;
    public static final int FORMAT_SERIALIZABLE   = 5;
    
    protected int format;
    private Object value;
    
    public JUMPData(Object value, int format){
        this.value = value;
        this.format = format;
    }
    
    /**
     * Creates a new instance of JUMPData
     */
    public JUMPData() {
        this(null,FORMAT_UNKNOWN);
    }
    
    public JUMPData(String value){
        this(value, FORMAT_STRING);
    }
    
    public JUMPData(int value){
        this(new Integer(value), FORMAT_INT);
    }
    
    public JUMPData(float value){
        this(new Float(value), FORMAT_FLOAT);
    }
    
    public JUMPData(boolean value){
        this(new Boolean(value), FORMAT_BOOLEAN);
    }
    
    public JUMPData(byte[] value){
        this(value, FORMAT_BYTES);
    }

    public JUMPData(java.io.Serializable value) {
        this(value, FORMAT_SERIALIZABLE);
    }
    
    public int getFormat(){
        return this.format;
    }
    
    public String getStringValue() {
        return (String)value;
    }
    
    public int getIntValue() {
        return ((Integer)value).intValue();
    }
    
    public float getFloatValue() {
        return ((Float)value).floatValue();
    }
    
    public boolean getBooleanValue() {
        return ((Boolean)value).booleanValue();
    }
    
    public byte[] getBytesValue() {
        return (byte[]) this.value;
    }

    /**
     * Returns the value of this JUMPData as an Object.
     */
    public Object getValue() {
        return this.value;
    }

    /**
     * Returns the String summerizing the content of this JUMPData.
     */
    public String toString() {
        return "JUMPData(" + this.value + ",format=" + this.format + ")";
    }
                                                                                 
    /**
     *  Tells whether or not this JUMPData is equal to another object.
     *  Two JUMPDatas are equal if the data format is the same and the value
     *  is <code> equals </code> to one another.
     */
    public boolean equals(Object obj) {
          if (!(obj instanceof JUMPData))
             return false;
          JUMPData other = (JUMPData) obj;
          return (format == other.format && value.equals(other.value));
    }
}
