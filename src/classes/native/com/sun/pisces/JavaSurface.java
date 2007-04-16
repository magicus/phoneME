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
 

package com.sun.pisces;

public final class JavaSurface extends AbstractSurface {
    private int[] dataInt;
    private short[] dataShort;
    private byte[] dataByte;
    
    public JavaSurface(int[] data, int width, int height) {
        this(data, TYPE_INT_ARGB, width, height);
    }
    
    public JavaSurface(int[] dataInt, int dataType, int width, int height) {
        this.dataInt = dataInt;

        switch (dataType) {
            case TYPE_INT_RGB:
            case TYPE_INT_ARGB:
            case TYPE_INT_ARGB_PRE:
                break;
            case TYPE_USHORT_565_RGB:
            case TYPE_BYTE_GRAY:            
                throw new IllegalArgumentException("Use different constructor"
                        + " for " + JavaSurface.class.getName() + " to use this"
                        + " data type");
            default:
                throw new IllegalArgumentException("Data type not supported "
                        + " for " + JavaSurface.class.getName());
        }

        initialize(dataType, width, height);
    }
    
    public JavaSurface(short[] dataShort, int dataType, int width, int height) {
        this.dataShort = dataShort;

        switch (dataType) {
            case TYPE_USHORT_565_RGB:
                break;
            case TYPE_INT_RGB:
            case TYPE_INT_ARGB:
            case TYPE_INT_ARGB_PRE:
            case TYPE_BYTE_GRAY:    
                throw new IllegalArgumentException("Use different constructor"
                        + " for " + JavaSurface.class.getName() + " to use this"
                        + " data type");
            default:
                throw new IllegalArgumentException("Data type not supported "
                        + " for " + JavaSurface.class.getName());
        }

        initialize(dataType, width, height);
    }
    
    public JavaSurface(byte[] dataByte, int dataType, int width, int height) {
        this.dataByte = dataByte;

        switch (dataType) {
            case TYPE_BYTE_GRAY:
                break;
            case TYPE_INT_RGB:
            case TYPE_INT_ARGB:
            case TYPE_INT_ARGB_PRE:
            case TYPE_USHORT_565_RGB:
                throw new IllegalArgumentException("Use different constructor"
                        + " for " + JavaSurface.class.getName() + " to use this"
                        + " data type");
            default:
                throw new IllegalArgumentException("Data type not supported "
                        + " for " + JavaSurface.class.getName());
        }

        initialize(dataType, width, height);
    }

    private native void initialize(int dataType, int width, int height);
}
