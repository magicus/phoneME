/*
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
 */

package java.nio;

/**
 * A <code>Buffer</code> storing <code>float</code> data.
 */
class FloatBufferImpl extends FloatBuffer {

    FloatBufferImpl(ByteBufferImpl parent, int capacity,
                    float[] array, int arrayOffset,
                    boolean isDirect) {
        this.parent = parent;
	this.isDirect = isDirect;
	this.array = array;
	this.arrayOffset = arrayOffset;
	this.capacity = this.limit = capacity;
	this.position = 0;
    }

    public float get() {
        if (position >= limit) {
            throw new BufferUnderflowException();
        }
	return get(position++);
    }

    public float get(int index) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 2);
	if (isDirect) {
	    return ByteBufferImpl._getFloat(bytePtr);
	} else if (array != null) {
	   return array[arrayOffset + index]; 
	} else {
            return parent.getFloat(bytePtr);
	}
    }

    public FloatBuffer put(float f) {
        if (position >= limit) {
            throw new BufferOverflowException();
        }
	return put(position++, f);
    }

    public FloatBuffer put(int index, float f) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 2);
	if (isDirect) {
	    ByteBufferImpl._putFloat(bytePtr, f);
	} else if (array != null) {
	    array[arrayOffset + index] = f;
	} else {
            parent.putFloat(bytePtr, f);
	}
	return this;
    }

    public FloatBuffer slice() {
        int pos = position;
        if (isDirect) {
            pos <<= 2;
        }
        return new FloatBufferImpl(parent, limit - position, array,
                                   arrayOffset + pos,
                                   isDirect);
    }

    public boolean isDirect() {
	return isDirect;
    }

    public int nativeAddress() {
	return arrayOffset;
    }

//     public String toString() {
//  	return "FloatBufferImpl[" +
//  	    "parent=" + parent +
//  	    ",array=" + array +
//  	    ",arrayOffset=" + arrayOffset +
//  	    ",capacity=" + capacity +
//  	    ",limit=" + limit +
//  	    ",position=" + position +
//   	    ",isDirect=" + isDirect +
//   	    ",disposed=" + disposed + "]";
//     }

    public void dispose() {
        // Need revisit
        this.disposed = true;
    }
}
