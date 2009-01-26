/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.jsr239.GLConfiguration;

/**
 * A <code>Buffer</code> storing <code>byte</code> data.
 */
class ByteBufferImpl extends ByteBuffer {
    
    static native int _allocNative(int capacity);

    static native void _copyBytes(int srcAddress,
                                  int dstAddress,
                                  int bytes);
    
    static native byte _getByte(int address);
    static native void _getBytes(int address,
                                 byte[] dst, int offset, int length);
    static native void _putByte(int address, byte value);
    static native void _putBytes(int address,
                                 byte[] dst, int offset, int length);

    static native short _getShort(int address);
    static native void _getShorts(int address,
                                  short[] dst, int offset, int length);
    static native void _putShort(int address, short value);
    static native void _putShorts(int address,
                                  short[] dst, int offset, int length);

    static native int _getInt(int address);
    static native void _getInts(int address,
                                int[] dst, int offset, int length);
    static native void _putInt(int address, int value);
    static native void _putInts(int address,
                                int[] dst, int offset, int length);
    
    static native float _getFloat(int address);
    static native void _getFloats(int address,
                                  float[] dst, int offset, int length);
    static native void _putFloat(int address, float value);
    static native void _putFloats(int address,
                                  float[] dst, int offset, int length);

    native private void finalize();

    ByteBufferImpl(int capacity, byte[] array, int arrayOffset,
                   ByteBuffer directParent) {
        this.array = array;
        this.arrayOffset = arrayOffset;

        this.capacity = capacity;
        this.limit = capacity;
        this.position = 0;

        this.isDirect = array == null;
        this.directParent = directParent;
    }

    public FloatBuffer asFloatBuffer() {
        int pos = position + (isDirect ? arrayOffset : 0);
        return new FloatBufferImpl(this, remaining() >> 2,
                                   null, pos, isDirect);
    }

    public IntBuffer asIntBuffer() {
        int pos = position + (isDirect ? arrayOffset : 0);
        return new IntBufferImpl(this, remaining() >> 2,
                                 null, pos, isDirect);
    }

    public ShortBuffer asShortBuffer() {
        int pos = position + (isDirect ? arrayOffset : 0);
        return new ShortBufferImpl(this, remaining() >> 1,
                                   null, pos, isDirect);
    }
 
    public byte get() {
        if (position >= limit) {
            throw new BufferUnderflowException();
        }
	return get(position++);
    }

    public byte get(int index) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }
	if (isDirect) {
	    return _getByte(arrayOffset + index);
	} else {
	   return array[arrayOffset + index]; 
	}
    }

    public ByteBuffer put(byte b) {
        if (position >= limit) {
            throw new BufferOverflowException();
        }
	return put(position++, b);
    }

    public ByteBuffer put(int index, byte b) {
        if (index < 0 || index >= limit) {
            throw new IndexOutOfBoundsException();
        }
        if (isDirect) {
	    _putByte(arrayOffset + index, b);
	} else {
	    array[arrayOffset + index] = b;
	}
	return this;
    }
    
    public float getFloat() {
        return Float.intBitsToFloat(getInt());
    }
    
    public float getFloat(int index) {
        return Float.intBitsToFloat(getInt(index));
    }

    public int getInt() {
        if (position >= limit - 3) {
            throw new BufferUnderflowException();
        }
        int x = getInt(position);
        position += 4;
        return x;
    }

    public int getInt(int index) {
        if (index < 0 || index >= limit - 3) {
            throw new IndexOutOfBoundsException();
        }
        int x0, x1, x2, x3;
	if (isDirect) {
            index += arrayOffset;
	    x0 = _getByte(index++);
	    x1 = _getByte(index++);
	    x2 = _getByte(index++);
	    x3 = _getByte(index);
	} else {
            index += arrayOffset;
            x0 =  array[index++]; 
            x1 =  array[index++]; 
            x2 =  array[index++]; 
            x3 =  array[index++]; 
	}

        if (GLConfiguration.IS_BIG_ENDIAN) {
            return (( x0         << 24) |
                    ((x1 & 0xff) << 16) |
                    ((x2 & 0xff) <<  8) |
                     (x3 & 0xff));
        } else {
            return (( x3         << 24) |
                    ((x2 & 0xff) << 16) |
                    ((x1 & 0xff) <<  8) |
                     (x0 & 0xff));
        }
    }

    public short getShort() {
        if (position >= limit - 1) {
            throw new BufferUnderflowException();
        }
        short s = getShort(position);
        position += 2;
        return s;
    }

    public short getShort(int index) {
        if (index < 0 || index >= limit - 1) {
            throw new IndexOutOfBoundsException();
        }
        int x0, x1;
	if (isDirect) {
            index += arrayOffset;
	    x0 = _getByte(index++);
	    x1 = _getByte(index++);
	} else {
            index += arrayOffset;
            x0 =  array[index++]; 
            x1 =  array[index++]; 
	}

        if (GLConfiguration.IS_BIG_ENDIAN) {
            return (short)(((x0 & 0xff) << 8) | (x1 & 0xff));
        } else {
            return (short)(((x1 & 0xff) << 8) | (x0 & 0xff));
        }
    }

    public ByteBuffer putFloat(float value) {
        return putInt(Float.floatToIntBits(value));
    }

    public ByteBuffer putFloat(int index, float value) {
        return putInt(index, Float.floatToIntBits(value));
    }

    public ByteBuffer putInt(int value) {
        if (position >= limit - 3) {
            throw new BufferOverflowException();
        }
        putInt(position, value);
        position += 4;
        return this;
    }
    
    public ByteBuffer putInt(int index, int value) {
        if (index < 0 || index >= limit - 3) {
            throw new IndexOutOfBoundsException();
        }

        byte x0, x1, x2, x3;
        if (GLConfiguration.IS_BIG_ENDIAN) {
            x0 = (byte)(value >> 24);
            x1 = (byte)(value >> 16);
            x2 = (byte)(value >> 8);
            x3 = (byte)(value);
        } else {
            x3 = (byte)(value >> 24);
            x2 = (byte)(value >> 16);
            x1 = (byte)(value >> 8);
            x0 = (byte)(value);
        }

	if (isDirect) {
            index += arrayOffset;
	    _putByte(index++, x0);
	    _putByte(index++, x1);
	    _putByte(index++, x2);
	    _putByte(index,   x3);
	} else {
            index += arrayOffset;
            array[index++] = x0;
            array[index++] = x1;
            array[index++] = x2;
            array[index  ] = x3;
	}
        
        return this;
    }
    
    public ByteBuffer putShort(short value) {
        if (position >= limit - 1) {
            throw new BufferOverflowException();
        }
        putShort(position, value);
        position += 2;
        return this;
    }
    
    public ByteBuffer putShort(int index, short value) {
        if (index < 0 || index >= limit - 1) {
            throw new IndexOutOfBoundsException();
        }

        byte x0, x1;
        if (GLConfiguration.IS_BIG_ENDIAN) {
            x0 = (byte)(value >> 8);
            x1 = (byte)(value);
        } else {
            x1 = (byte)(value >> 8);
            x0 = (byte)(value);
        }

	if (isDirect) {
            index += arrayOffset;
	    _putByte(index++, x0);
	    _putByte(index,   x1);
	} else {
            index += arrayOffset;
            array[index++] = x0;
            array[index  ] = x1;
	}
        
        return this;
    }
    
    public ByteBuffer slice() {
        return new ByteBufferImpl(limit - position, array,
                arrayOffset + position, this);
    }

    public boolean isDirect() {
	return isDirect;
    }

    public int nativeAddress() {
	return arrayOffset;
    }

    public static boolean isBigEndian() {
        return GLConfiguration.IS_BIG_ENDIAN;
    }

//     public String toString() {
// 	return "ByteBufferImpl[" +
//  	    "array=" + array +
// 	    ",arrayOffset=" + arrayOffset +
//  	    ",capacity=" + capacity +
//   	    ",limit=" + limit +
//   	    ",position=" + position +
//   	    ",isBigEndian=" + GLConfiguration.IS_BIG_ENDIAN +
//   	    ",isDirect=" + isDirect +
//   	    ",disposed=" + disposed + "]";
//     }

    public void dispose() {
        // Need revisit
        this.disposed = true;
    }
}
