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
 * A <code>Buffer</code> storing <code>short</code> data.
 */
class ShortBufferImpl extends ShortBuffer {

    ShortBufferImpl(ByteBufferImpl parent, int capacity,
                    short[] array, int arrayOffset,
                    boolean isDirect) {
        this.parent = parent;
	this.isDirect = isDirect;
	this.array = array;
	this.arrayOffset = arrayOffset;
	this.capacity = this.limit = capacity;
	this.position = 0;
    }

    public short get() {
        if (position >= limit) {
            throw new BufferUnderflowException();
        }
	return get(position++);
    }

    public short get(int index) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 1);
	if (isDirect) {
	    return ByteBufferImpl._getShort(bytePtr);
	} else if (array != null) {
	   return array[arrayOffset + index];
	} else {
            return parent.getShort(bytePtr);
	}
    }

    public ShortBuffer put(short s) {
        if (position >= limit) {
            throw new BufferOverflowException();
        }
	return put(position++, s);
    }

    public ShortBuffer put(int index, short s) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }

        int bytePtr = arrayOffset + (index << 1);
	if (isDirect) {
	    ByteBufferImpl._putShort(bytePtr, s);
	} else if (array != null) {
	    array[arrayOffset + index] = s;
	} else {
            parent.putShort(bytePtr, s);
	}
	return this;
    }

    public ShortBuffer slice() {
        int pos = position;
        if (isDirect) {
            pos <<= 1;
        }
        return new ShortBufferImpl(parent, limit - position, array,
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
//  	return "ShortBufferImpl[" +
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
