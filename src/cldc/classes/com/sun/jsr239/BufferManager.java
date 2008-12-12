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

package com.sun.jsr239;

import java.nio.*;
import java.lang.ref.WeakReference;
import java.util.Enumeration;
import java.util.Hashtable;

public class BufferManager {

    // All buffers in use by the native API engine are listed in
    // this table, along with a reference count.  When a reference
    // count drops to 0, the buffer is removed from the table.
    //
    // Buffer -> Integer(# implementation references)
    static Hashtable bufferStrongReferences = new Hashtable();

    // All buffers are listed in this table.  If a WeakReference
    // becomes null, free the associated native heap data.
    //
    // WeakReference(Buffer) -> Integer(native heap pointer)
    static Hashtable bufferWeakReferences = new Hashtable();

    // Record the native address of this buffer along with a
    // weak reference.
    public static synchronized void newBuffer(Buffer b, int pointer) {
        bufferWeakReferences.put(new WeakReference(b), new Integer(pointer));
    }
    
    // GL has a new pointer to the buffer
    public static synchronized void useBuffer(Buffer b) {
        if (b == null) {
            return;
        }

        Object o = bufferStrongReferences.get(b);
        int refs = (o == null) ? 0 : ((Integer)o).intValue();
        bufferStrongReferences.put(b, new Integer(refs + 1));
    }

    // GL has released a pointer to the buffer
    public static synchronized void releaseBuffer(Buffer b) {
        if (b == null) {
            return;
        }

        Object o = bufferStrongReferences.get(b);
        int refs = (o == null) ? 0 : ((Integer)o).intValue();
        if (refs > 1) {
            bufferStrongReferences.put(b, new Integer(refs - 1));
        } else {
            bufferStrongReferences.remove(b);
        }
    }

    public static native void _getBytes(int address,
                                        byte[] dst, int offset, int length);

    // Offsets are in bytes regardless of the buffer's datatype
    public static void getBytes(Buffer buf, int boffset,
                         byte[] dst, int offset, int length) {
        int address = GL10Impl._getNativeAddress(buf, 0);
        int capacity = buf.capacity();
        
        if (buf instanceof ByteBuffer) {
            ByteBuffer bbi = (ByteBuffer)buf;
            if (!bbi.isDirect()) {
                throw new IllegalArgumentException("!isDirect");
            }
        } else if (buf instanceof ShortBuffer) {
            ShortBuffer sbi = (ShortBuffer)buf;
            if (!sbi.isDirect()) {
                throw new IllegalArgumentException("!isDirect");
            }
            capacity *= 2;
        } else if (buf instanceof IntBuffer) {
            IntBuffer ibi = (IntBuffer)buf;
            if (!ibi.isDirect()) {
                throw new IllegalArgumentException("!isDirect");
            }
            capacity *= 4;
        } else if (buf instanceof FloatBuffer) {
            FloatBuffer fbi = (FloatBuffer)buf;
            if (!fbi.isDirect()) {
                throw new IllegalArgumentException("!isDirect");
            }
            capacity *= 4;
        } else {
            throw new IllegalArgumentException("Unknown buffer type!");
        }
        
        if (boffset < 0 || boffset + length > capacity) {
            throw new IllegalArgumentException("boffset out of bounds");
        }
        if (offset < 0 || offset + length > dst.length) {
            throw new IllegalArgumentException("offset out of bounds");
        }

        _getBytes(address + boffset, dst, offset, length);
    }
}
