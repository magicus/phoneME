/*
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

package java.nio;

/**
 * A <code>Buffer</code> storing <code>int</code> data.
 */
class IntBufferImpl extends IntBuffer {

    IntBufferImpl(ByteBufferImpl parent, int capacity,
                  int[] array, int arrayOffset,
                  boolean isDirect) {
        this.parent = parent;
	this.isDirect = isDirect;
	this.array = array;
	this.arrayOffset = arrayOffset;
	this.capacity = this.limit = capacity;
	this.position = 0;
    }

    public int get() {
        if (position >= limit) {
            throw new BufferUnderflowException();
        }
	return get(position++);
    }

    public int get(int index) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 2);
	if (isDirect) {
            return ByteBufferImpl._getInt(bytePtr);
	} else if (array != null) {
            return array[arrayOffset + index]; 
	} else {
            return parent.getInt(bytePtr);
        }
    }

    public IntBuffer put(int i) {
        if (position >= limit) {
            throw new BufferOverflowException();
        }
	return put(position++, i);
    }

    public IntBuffer put(int index, int i) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 2);
	if (isDirect) {
	    ByteBufferImpl._putInt(bytePtr, i);
	} else if (array != null) {
	    array[arrayOffset + index] = i;
	} else {
            parent.putInt(bytePtr, i);
        }
	return this;
    }

    public IntBuffer slice() {
        int pos = position;
        if (isDirect) {
            pos <<= 2;
        }
        return new IntBufferImpl(parent, limit - position, array,
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
//  	return "IntBufferImpl[" +
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
