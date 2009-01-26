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

package com.sun.jsr239;

import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.*;
import java.nio.*;

public class GL11Impl extends GL10Impl
    implements GL11, GL11Ext, GL11ExtensionPack {

    /**
     * Utility for common error checking.
     * 
     * @exception <code>UnsupportedOperationException</code> if the
     * underlying engine does not support OpenGL ES 1.1.
     */
    void check_1_1() {
        if (!GLConfiguration.supportsGL11) {
            throw new UnsupportedOperationException(Errors.GL_GL11_UNSUPPORTED);
        }
    }

    // Begin GL methods

    // VBO Methods

    public synchronized void glGenBuffers(int n, int[] buffers, int offset) {
        checkThread();
        check_1_1();
        checkLength(buffers, n, offset);

        qflush();
        IglGenBuffers(n, buffers, offset);
    }

    public synchronized void glGenBuffers(int n, IntBuffer buffers) {
        checkThread();
        check_1_1();
        checkLength(buffers, n);

        qflush();
        if (isDirect(buffers)) {
            int[] b = new int[n];

            int pos = buffers.position();

            // Copy data out in case we don't write everything
            buffers.get(b);

            IglGenBuffers(n, b, 0);
            
            // Write new data to buffer
            buffers.position(pos);
            buffers.put(b);

            // Restore buffer position
            buffers.position(pos);
        } else {
            IglGenBuffers(n, buffers.array(), offset(buffers));
        }
    }

    void IglGenBuffers(int n, int[] buffers, int offset) {
        grabContext();
        _glGenBuffers(n, buffers, offset);

        // Need revisit - what if out of memory?
        // Then buffers not really created :-(

        // Record valid buffer IDs
        for (int i = 0; i < n; i++) {
            addBuffer(buffers[offset + i]);
        }
    }

    public synchronized void glDeleteBuffers(int n,
                                             int[] buffers, int offset) {
        checkThread();
        check_1_1();
        checkLength(buffers, n, offset);

        IglDeleteBuffers(n, buffers, offset);
    }

    public synchronized void glDeleteBuffers(int n, IntBuffer buffers) {
        checkThread();
        check_1_1();
        checkLength(buffers, n);

        if (buffers.isDirect()) {
            int[] b = new int[n];
            buffers.get(b, 0, n);
            IglDeleteBuffers(n, b, 0);
        } else {
            IglDeleteBuffers(n, buffers.array(), offset(buffers));
        }
    }

    void IglDeleteBuffers(int n, int[] buffers, int offset) {
        q(CMD_DELETE_BUFFERS, n + 1);
        q(n);
        for (int i = 0; i < n; i++) {
            q(buffers[offset + i]);
        }
        qflush();
        
        // Remove deleted buffer IDs
        for (int i = 0; i < n; i++) {
            removeBuffer(buffers[offset + i]);
        }
    }

    public synchronized void glBindBuffer(int target, int buffer) {
        checkThread();
        check_1_1();

        q(CMD_BIND_BUFFER, 2);
        q(target);
        q(buffer);

        qflush();

        if (target == GL_ARRAY_BUFFER) {
            VBOArrayBufferBound = buffer;
        } else if (target == GL_ELEMENT_ARRAY_BUFFER) {
            VBOElementArrayBufferBound = buffer;
        }
    }

    public synchronized void glBufferData(int target, int size,
                                          Buffer data, int usage) {
        checkThread();
        check_1_1();
        if (data != null && !isDirect(data)) {
            throw new IllegalArgumentException(Errors.GL_NOT_DIRECT);
        }

        if ((target == GL_ARRAY_BUFFER) && (VBOArrayBufferBound != 0)) {
            setBufferSize(target, size);
        } else if ((target == GL_ELEMENT_ARRAY_BUFFER) &&
                   (VBOElementArrayBufferBound != 0)) {
            setBufferSize(target, size);
            bufferIndexData(data, 0, size, true);
        }

        q(CMD_BUFFER_DATA, 4);
        q(target);
        q(size);
        q(data == null ? 0 : pointer(data));
        q(usage);
    
        qflush();
    }

    public synchronized void glBufferSubData(int target, int offset, int size,
                                             Buffer data) {
        checkThread();
        check_1_1();
        if (!isDirect(data)) {
            throw new IllegalArgumentException(Errors.GL_NOT_DIRECT);
        }

        if ((target == GL_ELEMENT_ARRAY_BUFFER) &&
            (VBOElementArrayBufferBound != 0)) {
            bufferIndexData(data, offset, size, false);
        }

        q(CMD_BUFFER_SUB_DATA, 4);
        q(target);
        q(offset);
        q(size);
        q(data);
    
        qflush();
    }

    public synchronized void glGetBufferParameteriv(int target, int pname,
                                                    int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetBufferParametervNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetBufferParameteriv(target, pname, params, offset, length);
    }

    public synchronized void glGetBufferParameteriv(int target, int pname,
                                                    IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetBufferParametervNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetBufferParameteriv(target, pname,
                                    params.array(), offset(params),
                                    length);
        } else {
            IglGetBufferParameteriv(target, pname,
                                    null, pointer(params), length);
        }
    }

    void IglGetBufferParameteriv(int target, int pname,
                                 int[] params, int offset,
                                 int length) {
        grabContext();
        _glGetBufferParameteriv(target, pname, params, offset, length);
    }

    public synchronized void glColorPointer(int size, int type,
                                            int stride, int offset) {
        checkThread();
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glColorPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        // Only record details if this is a legal operation
        if ((size == 4) && 
            (type == GL_UNSIGNED_BYTE ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[COLOR_POINTER]);
            
            pointerBuffer[COLOR_POINTER] = null;
            pointerSize[COLOR_POINTER] = size;
            pointerType[COLOR_POINTER] = type;
            pointerStride[COLOR_POINTER] = stride;
            pointerRemaining[COLOR_POINTER] = -1;
            pointerOffset[COLOR_POINTER] = offset;
        }

        q(CMD_COLOR_POINTER_VBO, 4);
        q(size);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    public synchronized void glNormalPointer(int type, int stride,
                                             int offset) {
        checkThread();
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glNormalPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        if ((type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[NORMAL_POINTER]);
            
            pointerBuffer[NORMAL_POINTER] = null;
            pointerSize[NORMAL_POINTER] = 3;
            pointerType[NORMAL_POINTER] = type;
            pointerStride[NORMAL_POINTER] = stride;
            pointerRemaining[NORMAL_POINTER] = -1;
            pointerOffset[NORMAL_POINTER] = offset;
        }

        q(CMD_NORMAL_POINTER_VBO, 3);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    public synchronized void glTexCoordPointer(int size, int type,
                                               int stride, int offset) {
        checkThread();
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glTexCoordPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        if ((size >= 2 && size <= 4) && 
            (type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[TEX_COORD_POINTER]);

            pointerBuffer[TEX_COORD_POINTER] = null;
            pointerSize[TEX_COORD_POINTER] = size;
            pointerType[TEX_COORD_POINTER] = type;
            pointerStride[TEX_COORD_POINTER] = stride;
            pointerRemaining[TEX_COORD_POINTER] = -1;
            pointerOffset[TEX_COORD_POINTER] = offset;
        }

        q(CMD_TEX_COORD_POINTER_VBO, 4);
        q(size);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    public synchronized void glVertexPointer(int size, int type,
                                             int stride, int offset) {
        checkThread();
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glVertexPointer:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        // Only record details if this is a legal operation
        if ((size >= 2 && size <= 4) && 
            (type == GL_BYTE ||
             type == GL_SHORT ||
             type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[VERTEX_POINTER]);

            pointerBuffer[VERTEX_POINTER] = null;
            pointerSize[VERTEX_POINTER] = size;
            pointerType[VERTEX_POINTER] = type;
            pointerStride[VERTEX_POINTER] = stride;
            pointerRemaining[VERTEX_POINTER] = -1;
            pointerOffset[VERTEX_POINTER] = offset;
        }

        q(CMD_VERTEX_POINTER_VBO, 4);
        q(size);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    // Point Size Array Extension

    public synchronized void glPointSizePointerOES(int type, int stride,
                                                   Buffer pointer) {
        checkThread();
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glPointSizePointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (pointer == null) {
            throwIAE(Errors.GL_POINTER_NULL);
        }
        if (!isDirect(pointer)) {
            throwIAE(Errors.GL_NOT_DIRECT);
        }

        // Only record details if this is a legal operation
        if ((type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[POINT_SIZE_POINTER]);
            BufferManager.useBuffer(pointer);

            pointerBuffer[POINT_SIZE_POINTER] = pointer;
            pointerSize[POINT_SIZE_POINTER] = 1;
            pointerType[POINT_SIZE_POINTER] = type;
            pointerStride[POINT_SIZE_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[POINT_SIZE_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[POINT_SIZE_POINTER] = 0;
        }

        q(CMD_POINT_SIZE_POINTER, 3);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glPointSizePointerOES(int type, int stride,
                                                   int offset) {
        checkThread();
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glPointSizePointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        BufferManager.releaseBuffer(pointerBuffer[POINT_SIZE_POINTER]);

        pointerBuffer[POINT_SIZE_POINTER] = null;
        pointerSize[POINT_SIZE_POINTER] = 1;
        pointerType[POINT_SIZE_POINTER] = type;
        pointerStride[POINT_SIZE_POINTER] = stride;
        pointerRemaining[POINT_SIZE_POINTER] = -1;
        pointerOffset[POINT_SIZE_POINTER] = offset;

        q(CMD_POINT_SIZE_POINTER_VBO, 3);
        q(type);
        q(stride);
        q(offset);
    }

    public synchronized void glDrawElements(int mode, int count,
                                            int type, int offset) {
        checkThread();
        if (VBOElementArrayBufferBound == 0) {
            throw new IllegalStateException("glDrawElements:" +
                                      Errors.VBO_ELEMENT_ARRAY_BUFFER_UNBOUND);
        }

        // No need to bounds check indices if there will be a type error
        if (type == GL_UNSIGNED_BYTE ||
            type == GL_UNSIGNED_SHORT) {

            byte[] bufferData = getBufferIndices();
            int nbytes = (type == GL_UNSIGNED_BYTE) ? 1 : 2;
            
//             System.out.println("offset = " + offset);
//             System.out.println("count = " + count);
//             System.out.println("nbytes = " + nbytes);
//             System.out.println("bufferData.length = " + bufferData.length);

            if (offset < 0 ||
                offset + count*nbytes > bufferData.length) {
                throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
            }

//             System.out.println("bufferData = ");
//             for (int i = 0; i < bufferData.length; i++) {
//                 System.out.print((bufferData[i] & 0xff) + " ");
//             }
//             System.out.println();
            
            int[] indexArray = new int[count];
            boolean isBigEndian = GLConfiguration.IS_BIG_ENDIAN;

            if (type == GL_UNSIGNED_BYTE) {
                for (int i = 0; i < count; i++) {
                    indexArray[i] = bufferData[i + offset] & 0xff;
                }
            } else if (type == GL_UNSIGNED_SHORT) {
                for (int i = 0; i < count; i++) {
                    int b0 = bufferData[2*i + offset] & 0xff;
                    int b1 = bufferData[2*i + offset + 1] & 0xff;
                    if (isBigEndian) {
                        indexArray[i] = (b0 << 8) | b1;
                    } else {
                        indexArray[i] = (b1 << 8) | b0;
                    }
                }
            }

            checkIndices(indexArray);
        }

        q(CMD_DRAW_ELEMENTS_VBO, 4);
        q(mode);
        q(count);
        q(type);
        q(offset);
    }

    // Other Methods

    public synchronized void glClipPlanef(int plane,
                                          float[] equation, int offset) {
        checkThread();
        check_1_1();
        checkLength(equation, 4, offset);

        IglClipPlanef(plane, equation, offset);
    }

    public synchronized void glClipPlanef(int plane, FloatBuffer equation) {
        checkThread();
        check_1_1();
        checkLength(equation, 4);

        if (!equation.isDirect()) {
            IglClipPlanef(plane, equation.array(), offset(equation));
            return;
        }

        q(CMD_CLIP_PLANEFB, 2);
        q(plane);
        q(equation);

        qflush();
    }

    void IglClipPlanef(int plane, float[] equation, int offset) {
        q(CMD_CLIP_PLANEF, 5);
        q(plane);
        q(equation[offset]);
        q(equation[offset + 1]);
        q(equation[offset + 2]);
        q(equation[offset + 3]);
    }

    public synchronized void glClipPlanex(int plane,
                                          int[] equation, int offset) {
        checkThread();
        check_1_1();
        checkLength(equation, 4, offset);

        IglClipPlanex(plane, equation, offset);
    }

    public synchronized void glClipPlanex(int plane, IntBuffer equation) {
        checkThread();
        check_1_1();
        checkLength(equation, 4);

        if (!equation.isDirect()) {
            IglClipPlanex(plane, equation.array(), offset(equation));
            return;
        }

        q(CMD_CLIP_PLANEXB, 2);
        q(plane);
        q(equation);

        qflush();
    }

    void IglClipPlanex(int plane, int[] equation, int offset) {
        q(CMD_CLIP_PLANEX, 5);
        q(plane);
        q(equation[offset]);
        q(equation[offset + 1]);
        q(equation[offset + 2]);
        q(equation[offset + 3]);
    }

    public synchronized void glGetClipPlanef(int pname,
                                             float[] equation, int offset) {
        checkThread();
        check_1_1();
        checkLength(equation, 4, offset);

        qflush();
        IglGetClipPlanef(pname, equation, offset);
    }

    public synchronized void glGetClipPlanef(int pname, FloatBuffer equation) {
        checkThread();
        check_1_1();
        checkLength(equation, 4);

        qflush();
        if (!equation.isDirect()) {
            IglGetClipPlanef(pname, equation.array(), offset(equation));
        } else {
            IglGetClipPlanef(pname, null, pointer(equation));
        }
    }

    void IglGetClipPlanef(int pname, float[] equation, int offset) {
        grabContext();
        _glGetClipPlanef(pname, equation, offset);
    }

    public synchronized void glGetClipPlanex(int pname,
                                             int[] equation, int offset) {
        checkThread();
        check_1_1();
        checkLength(equation, 4, offset);

        qflush();
        IglGetClipPlanex(pname, equation, offset);
    }

    public synchronized void glGetClipPlanex(int pname, IntBuffer equation) {
        checkThread();
        check_1_1();
        checkLength(equation, 4);

        qflush();
    
        if (!equation.isDirect()) {
            IglGetClipPlanex(pname, equation.array(), offset(equation));
        } else {
            IglGetClipPlanex(pname, null, pointer(equation));
        }
    }

    void IglGetClipPlanex(int pname, int[] equation, int offset) {
        grabContext();
        _glGetClipPlanex(pname, equation, offset);
    }

    public synchronized void glGetFixedv(int pname, int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetFixedv(pname, params, offset, length);
    }

    public synchronized void glGetFixedv(int pname, IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetFixedv(pname, params.array(), offset(params), length);
        } else {
            IglGetFixedv(pname, null, pointer(params), length);
        }
    }

    void IglGetFixedv(int pname, int[] params, int offset, int length) {
        grabContext();
        _glGetFixedv(pname, params, offset, length);
    }

    public synchronized void glGetFloatv(int pname,
                                         float[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetFloatv(pname, params, offset, length);
    }

    public synchronized void glGetFloatv(int pname, FloatBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glGetNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetFloatv(pname, params.array(), offset(params), length);
        } else {
            IglGetFloatv(pname, null, pointer(params), length);
        }
    }

    void IglGetFloatv(int pname, float[] params, int offset, int length) {
        grabContext();
        _glGetFloatv(pname, params, offset, length);
    }
    
    public synchronized void glGetLightfv(int light, int pname,
                                          float[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetLightfv(light, pname, params, offset, length);
    }

    public synchronized void glGetLightfv(int light, int pname,
                                          FloatBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetLightfv(light, pname,
                          params.array(), offset(params), length);
        } else {
            IglGetLightfv(light, pname, null, pointer(params), length);
        }
    }

    void IglGetLightfv(int light, int pname,
                       float[] params, int offset, int length) {
        grabContext();
        _glGetLightfv(light, pname, params, offset, length);
    }

    public synchronized void glGetLightxv(int light, int pname,
                                          int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetLightxv(light, pname, params, offset, length);
    }

    public synchronized void glGetLightxv(int light, int pname,
                                          IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glLightNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetLightxv(light, pname,
                          params.array(), offset(params), length);
        } else {
            IglGetLightxv(light, pname, null, pointer(params), length);
        }
    }

    void IglGetLightxv(int light, int pname,
                       int[] params, int offset, int length) {
        grabContext();
        _glGetLightxv(light, pname, params, offset, length);
    }

    public synchronized void glGetMaterialfv(int face, int pname,
                                             float[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetMaterialfv(face, pname, params, offset, length);
    }

    public synchronized void glGetMaterialfv(int face, int pname,
                                             FloatBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetMaterialfv(face, pname,
                             params.array(), offset(params), length);
        } else {
            IglGetMaterialfv(face, pname, null, pointer(params), length);
        }
    }

    void IglGetMaterialfv(int face, int pname,
                          float[] params, int offset, int length) {
        grabContext();
        _glGetMaterialfv(face, pname, params, offset, length);
    }

    public synchronized void glGetMaterialxv(int face, int pname,
                                             int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetMaterialxv(face, pname, params, offset, length);
    }

    public synchronized void glGetMaterialxv(int face, int pname,
                                             IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glMaterialNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetMaterialxv(face, pname,
                             params.array(), offset(params), length);
        } else {
            IglGetMaterialxv(face, pname, null, pointer(params), length);
        }
    }

    void IglGetMaterialxv(int face, int pname,
                          int[] params, int offset, int length) {
        grabContext();
        _glGetMaterialxv(face, pname, params, offset, length);
    }

    public synchronized void glGetPointerv(int pname, Buffer[] params) {
        if (params == null || params.length < 1) {
            throw new IllegalArgumentException();
        }
        
        int pointer = -1;
        switch (pname) {
        case GL_VERTEX_ARRAY_POINTER:
            pointer = VERTEX_POINTER;
            break;
        case GL_COLOR_ARRAY_POINTER:
            pointer = COLOR_POINTER;
            break;
        case GL_NORMAL_ARRAY_POINTER:
            pointer = NORMAL_POINTER;
            break;
        case GL_TEXTURE_COORD_ARRAY_POINTER:
            pointer = TEX_COORD_POINTER;
            break;
        case GL11.GL_POINT_SIZE_ARRAY_POINTER_OES:
            pointer = POINT_SIZE_POINTER;
            break;
        case GL11Ext.GL_MATRIX_INDEX_ARRAY_POINTER_OES:
            pointer = MATRIX_INDEX_POINTER;
            break;
        case GL11Ext.GL_WEIGHT_ARRAY_POINTER_OES:
            pointer = WEIGHT_POINTER;
            break;
        }

        if (pointer != -1) {
            params[0] = pointerBuffer[pointer];
        } else {
            qflush();
            _glGenerateError(GL_INVALID_ENUM);
        }
    }

    public synchronized void glGetTexEnvfv(int env, int pname,
                                           float[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexEnvfv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexEnvfv(int env, int pname,
                                           FloatBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexEnvfv(env, pname, params.array(), offset(params), length);
        } else {
            IglGetTexEnvfv(env, pname, null, pointer(params), length);
        }
    }

    void IglGetTexEnvfv(int env, int pname,
                        float[] params, int offset, int length) {
        grabContext();
        _glGetTexEnvfv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexEnviv(int env, int pname,
                                           int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexEnviv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexEnviv(int env, int pname,
                                           IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexEnviv(env, pname, params.array(), offset(params), length);
        } else {
            IglGetTexEnviv(env, pname, null, pointer(params), length);
        }
    }

    void IglGetTexEnviv(int env, int pname,
                        int[] params, int offset, int length) {
        grabContext();
        _glGetTexEnviv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexEnvxv(int env, int pname,
                                           int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexEnvxv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexEnvxv(int env, int pname,
                                           IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexEnvNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexEnvxv(env, pname, params.array(), offset(params), length);
        } else {
            IglGetTexEnvxv(env, pname, null, pointer(params), length);
        }
    }

    void IglGetTexEnvxv(int env, int pname,
                        int[] params, int offset, int length) {
        grabContext();
        _glGetTexEnvxv(env, pname, params, offset, length);
    }

    public synchronized void glGetTexParameterfv(int target, int pname,
                                                 float[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexParameterfv(target, pname, params, offset, length);
    }

    public synchronized void glGetTexParameterfv(int target, int pname,
                                                 FloatBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexParameterfv(target, pname, params.array(), offset(params),
                                 length);
        } else {
            IglGetTexParameterfv(target, pname, null, pointer(params), length);
        }
    }

    void IglGetTexParameterfv(int target, int pname,
                              float[] params, int offset, int length) {
        grabContext();
        _glGetTexParameterfv(target, pname, params, offset, length);
    }

    public synchronized void glGetTexParameteriv(int target, int pname,
                                                 int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexParameteriv(target, pname, params, offset, length);
    }

    public synchronized void glGetTexParameteriv(int target, int pname,
                                                 IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexParameteriv(target, pname, params.array(), offset(params),
                                 length);
        } else {
            IglGetTexParameteriv(target, pname, null, pointer(params), length);
        }
    }

    void IglGetTexParameteriv(int target, int pname,
                              int[] params, int offset, int length) {
        grabContext();
        _glGetTexParameteriv(target, pname, params, offset, length);
    }

    public synchronized void glGetTexParameterxv(int target, int pname,
                                                 int[] params, int offset) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexParameterxv(target, pname, params, offset, length);
    }

    public synchronized void glGetTexParameterxv(int target, int pname,
                                                 IntBuffer params) {
        checkThread();
        check_1_1();
        int length = GLConfiguration.glTexParameterNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexParameterxv(target, pname, params.array(), offset(params),
                                 length);
        } else {
            IglGetTexParameterxv(target, pname, null, pointer(params), length);
        }
    }

    void IglGetTexParameterxv(int target, int pname,
                              int[] params, int offset, int length) {
        grabContext();
        _glGetTexParameterxv(target, pname, params, offset, length);
    }

    // Draw Texture Extension

    public synchronized void glDrawTexsOES(short x, short y, short z,
                                           short width, short height) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        q(CMD_DRAW_TEXS, 5);
        q((int)x);
        q((int)y);
        q((int)z);
        q((int)width);
        q((int)height);
    }

    public synchronized void glDrawTexiOES(int x, int y, int z,
                                           int width, int height) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        q(CMD_DRAW_TEXI, 5);
        q(x);
        q(y);
        q(z);
        q(width);
        q(height);
    }

    public synchronized void glDrawTexfOES(float x, float y, float z,
                                           float width, float height) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        q(CMD_DRAW_TEXF, 5);
        q(x);
        q(y);
        q(z);
        q(width);
        q(height);
    }

    public synchronized void glDrawTexxOES(int x, int y, int z,
                                           int width, int height) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        q(CMD_DRAW_TEXX, 5);
        q(x);
        q(y);
        q(z);
        q(width);
        q(height);
    }

    public synchronized void glDrawTexsvOES(short[] coords, int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5, offset);

        IglDrawTexsvOES(coords, offset);
    }

    public synchronized void glDrawTexsvOES(ShortBuffer coords) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5);

        if (!coords.isDirect()) {
            IglDrawTexsvOES(coords.array(), offset(coords));
            return;
        }

        q(CMD_DRAW_TEXSB, 1);
        q(coords);

        qflush();
    }

    void IglDrawTexsvOES(short[] coords, int offset) {
        q(CMD_DRAW_TEXS, 5);
        q((int)coords[offset]);
        q((int)coords[offset + 1]);
        q((int)coords[offset + 2]);
        q((int)coords[offset + 3]);
        q((int)coords[offset + 4]);
    }

    public synchronized void glDrawTexivOES(int[] coords, int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5, offset);

        IglDrawTexivOES(coords, offset);
    }

    public synchronized void glDrawTexivOES(IntBuffer coords) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5);

        if (!coords.isDirect()) {
            IglDrawTexivOES(coords.array(), offset(coords));
            return;
        }

        q(CMD_DRAW_TEXIB, 1);
        q(coords);

        qflush();
    }

    void IglDrawTexivOES(int[] coords, int offset) {
        q(CMD_DRAW_TEXI, 5);
        q(coords[offset]);
        q(coords[offset + 1]);
        q(coords[offset + 2]);
        q(coords[offset + 3]);
        q(coords[offset + 4]);
    }

    public synchronized void glDrawTexxvOES(int[] coords, int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5, offset);

        IglDrawTexxvOES(coords, offset);
    }

    public synchronized void glDrawTexxvOES(IntBuffer coords) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5);

        if (!coords.isDirect()) {
            IglDrawTexxvOES(coords.array(), offset(coords));
            return;
        }

        q(CMD_DRAW_TEXXB, 1);
        q(coords);

        qflush();
    }

    void IglDrawTexxvOES(int[] coords, int offset) {
        q(CMD_DRAW_TEXX, 5);
        q(coords[offset]);
        q(coords[offset + 1]);
        q(coords[offset + 2]);
        q(coords[offset + 3]);
        q(coords[offset + 4]);
    }

    public synchronized void glDrawTexfvOES(float[] coords, int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5, offset);

        IglDrawTexfvOES(coords, offset);
    }

    public synchronized void glDrawTexfvOES(FloatBuffer coords) {
        checkThread();
        if (!GLConfiguration.supports_OES_draw_texture) {
            throw new UnsupportedOperationException(
                                           Errors.GL_DRAW_TEXTURE_UNSUPPORTED);
        }
        checkLength(coords, 5);

        if (!coords.isDirect()) {
            IglDrawTexfvOES(coords.array(), offset(coords));
            return;
        }

        q(CMD_DRAW_TEXFB, 1);
        q(coords);

        qflush();
    }

    void IglDrawTexfvOES(float[] coords, int offset) {
        q(CMD_DRAW_TEXF, 5);
        q(coords[offset]);
        q(coords[offset + 1]);
        q(coords[offset + 2]);
        q(coords[offset + 3]);
        q(coords[offset + 4]);
    }

    // Matrix Palette Extension

    public synchronized void
        glCurrentPaletteMatrixOES(int matrixpaletteindex) {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        q(CMD_CURRENT_PALETTE_MATRIX, 1);
        q(matrixpaletteindex);
    }

    public synchronized void glLoadPaletteFromModelViewMatrixOES() {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        q(CMD_LOAD_PALETTE_FROM_MODEL_VIEW_MATRIX, 0);
    }

    public synchronized void glMatrixIndexPointerOES(int size, int type,
                                                     int stride,
                                                     Buffer pointer) {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glMatrixIndexPointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (!isDirect(pointer)) {
            throw new ArrayIndexOutOfBoundsException(Errors.GL_NOT_DIRECT);
        }

        if ((size <= GLConfiguration.MAX_VERTEX_UNITS) &&
            (type == GL_UNSIGNED_BYTE) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[MATRIX_INDEX_POINTER]);
            BufferManager.useBuffer(pointer);
            
            pointerBuffer[MATRIX_INDEX_POINTER] = pointer;
            pointerSize[MATRIX_INDEX_POINTER] = size;
            pointerType[MATRIX_INDEX_POINTER] = type;
            pointerStride[MATRIX_INDEX_POINTER] = stride;
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[MATRIX_INDEX_POINTER] =
                pointer.remaining()*nbytes;
            pointerOffset[MATRIX_INDEX_POINTER] = 0;
        }

        q(CMD_MATRIX_INDEX_POINTER, 4);
        q(size);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glMatrixIndexPointerOES(int size, int type,
                                                     int stride, int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glMatrixIndexPointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new ArrayIndexOutOfBoundsException(Errors.VBO_OFFSET_OOB);
        }

        if ((size > 0) &&
            (size <= GLConfiguration.MAX_VERTEX_UNITS) &&
            (type == GL_UNSIGNED_BYTE) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[MATRIX_INDEX_POINTER]);
            
            pointerBuffer[MATRIX_INDEX_POINTER] = null;
            pointerSize[MATRIX_INDEX_POINTER] = size;
            pointerType[MATRIX_INDEX_POINTER] = type;
            pointerStride[MATRIX_INDEX_POINTER] = stride;
            pointerRemaining[MATRIX_INDEX_POINTER] = -1;
            pointerOffset[MATRIX_INDEX_POINTER] = offset;
        }

        q(CMD_MATRIX_INDEX_POINTER_VBO, 4);
        q(size);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    public synchronized void glWeightPointerOES(int size, int type, int stride,
                                                Buffer pointer) {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        if (VBOArrayBufferBound != 0) {
            throw new IllegalStateException("glWeightPointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_BOUND);
        }
        if (!isDirect(pointer)) {
            throw new IllegalArgumentException(Errors.GL_NOT_DIRECT);
        }

        if ((size <= GLConfiguration.MAX_VERTEX_UNITS) && 
            (type == GL_FIXED || type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[WEIGHT_POINTER]);
            BufferManager.useBuffer(pointer);

            pointerBuffer[WEIGHT_POINTER] = pointer;
            pointerSize[WEIGHT_POINTER] = size;
            pointerType[WEIGHT_POINTER] = type;
            pointerStride[WEIGHT_POINTER] = stride; 
            int nbytes = bufferTypeSize(pointer);
            pointerRemaining[WEIGHT_POINTER] = pointer.remaining()*nbytes;
            pointerOffset[WEIGHT_POINTER] = 0;
        }

        q(CMD_WEIGHT_POINTER, 4);
        q(size);
        q(type);
        q(stride);
        q(pointer);

        qflush();
    }

    public synchronized void glWeightPointerOES(int size, int type, int stride,
                                                int offset) {
        checkThread();
        if (!GLConfiguration.supports_OES_matrix_palette) {
            throw new UnsupportedOperationException(
                                         Errors.GL_MATRIX_PALETTE_UNSUPPORTED);
        }
        if (VBOArrayBufferBound == 0) {
            throw new IllegalStateException("glWeightPointerOES:" +
                                            Errors.VBO_ARRAY_BUFFER_UNBOUND);
        }

        int bufferSize = getBufferSize(GL11.GL_ARRAY_BUFFER);
        if (offset < 0 || offset >= bufferSize) {
            throw new IllegalArgumentException(Errors.VBO_OFFSET_OOB);
        }

        if ((size >= 0) &&
            (size <= GLConfiguration.MAX_VERTEX_UNITS) && 
            (type == GL_FIXED ||
             type == GL_FLOAT) &&
            (stride >= 0)) {
            BufferManager.releaseBuffer(pointerBuffer[WEIGHT_POINTER]);

            pointerBuffer[WEIGHT_POINTER] = null;
            pointerSize[WEIGHT_POINTER] = size;
            pointerType[WEIGHT_POINTER] = type;
            pointerStride[WEIGHT_POINTER] = stride;
            pointerRemaining[WEIGHT_POINTER] = -1;
            pointerOffset[WEIGHT_POINTER] = offset;
        }

        q(CMD_WEIGHT_POINTER_VBO, 4);
        q(size);
        q(type);
        q(stride);
        q(offset);

        qflush();
    }

    // OES_texture_cube_map

    /**
     * Utility for common error checking.
     * 
     * @exception <code>UnsupportedOperationException</code> if the
     * underlying engine does not support the
     * <code>OES_texture_cube_map</code> extension.
     */
    void check_texture_cube_map() {
        if (!GLConfiguration.supports_OES_texture_cube_map) {
            throw new UnsupportedOperationException(
                                       Errors.GL_TEXTURE_CUBE_MAP_UNSUPPORTED);
        }
    }

    public synchronized void glTexGenf(int coord, int pname, float param) {
        checkThread();
        check_texture_cube_map();
    
        q(CMD_TEX_GENF, 3);
        q(coord);
        q(pname);
        q(param);
    }

    public synchronized void glTexGeni(int coord, int pname, int param) {
        checkThread();
        check_texture_cube_map();

        q(CMD_TEX_GENI, 3);
        q(coord);
        q(pname);
        q(param);
    }

    public synchronized void glTexGenx(int coord, int pname, int param) {
        checkThread();
        check_texture_cube_map();

        q(CMD_TEX_GENX, 3);
        q(coord);
        q(pname);
        q(param);
    }

    public synchronized void glTexGenfv(int coord, int pname,
                                        float[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);

        IglTexGenfv(coord, pname, params, offset);
    }

    public synchronized void glTexGenfv(int coord, int pname,
                                        FloatBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            glTexGenfv(coord, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_GENFB, 3);
        q(coord);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexGenfv(int coord, int pname, float[] params, int offset) {
        int n = GLConfiguration.glTexGenNumParams(pname);
      
        q(CMD_TEX_GENFV, n + 3);
        q(n);
        q(coord);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[offset + i]);
        }
    }

    public synchronized void glTexGeniv(int coord, int pname,
                                        int[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);
    }

    public synchronized void glTexGeniv(int coord, int pname,
                                        IntBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            glTexGeniv(coord, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_GENIB, 3);
        q(coord);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexGeniv(int coord, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexGenNumParams(pname);
      
        q(CMD_TEX_GENIV, n + 3);
        q(n);
        q(coord);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[offset + i]);
        }
    }

    public synchronized void glTexGenxv(int coord, int pname,
                                        int[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);
    }

    public synchronized void glTexGenxv(int coord, int pname,
                                        IntBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        if (!params.isDirect()) {
            glTexGenxv(coord, pname, params.array(), offset(params));
            return;
        }

        q(CMD_TEX_GENXB, 3);
        q(coord);
        q(pname);
        q(params);

        qflush();
    }

    void IglTexGenxv(int coord, int pname, int[] params, int offset) {
        int n = GLConfiguration.glTexGenNumParams(pname);
      
        q(CMD_TEX_GENXV, n + 3);
        q(n);
        q(coord);
        q(pname);
        for (int i = 0; i < n; i++) { 
            q(params[offset + i]);
        }
    }

    public synchronized void glGetTexGenfv(int coord, int pname,
                                           float[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexGenfv(coord, pname, params, offset, length);
    }

    public synchronized void glGetTexGenfv(int coord, int pname,
                                           FloatBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexGenfv(coord, pname,
                           params.array(), offset(params), length);
        } else {
            IglGetTexGenfv(coord, pname, null, pointer(params), length);
        }
    }

    void IglGetTexGenfv(int coord, int pname,
                        float[] params, int offset, int length) {
        grabContext();
        _glGetTexGenfv(coord, pname, params, offset, length);
    }

    public synchronized void glGetTexGeniv(int coord, int pname,
                                           int[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexGeniv(coord, pname, params, offset, length);
    }

    public synchronized void glGetTexGeniv(int coord, int pname,
                                           IntBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexGeniv(coord, pname,
                           params.array(), offset(params), length);
        } else {
            IglGetTexGeniv(coord, pname, null, pointer(params), length);
        }
    }

    void IglGetTexGeniv(int coord, int pname,
                        int[] params, int offset, int length) {
        grabContext();
        _glGetTexGeniv(coord, pname, params, offset, length);
    }

    public synchronized void glGetTexGenxv(int coord, int pname,
                                           int[] params, int offset) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetTexGenxv(coord, pname, params, offset, length);
    }

    public synchronized void glGetTexGenxv(int coord, int pname,
                                           IntBuffer params) {
        checkThread();
        check_texture_cube_map();
        int length = GLConfiguration.glTexGenNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetTexGenxv(coord, pname,
                           params.array(), offset(params), length);
        } else {
            IglGetTexGenxv(coord, pname, null, pointer(params), length);
        }
    }

    void IglGetTexGenxv(int coord, int pname,
                        int[] params, int offset, int length) {
        grabContext();
        _glGetTexGenxv(coord, pname, params, offset, length);
    }

    // OES_blend_subtract

    public synchronized void glBlendEquation(int mode) {
        checkThread();
        if (!GLConfiguration.supports_OES_blend_subtract) {
            throw new UnsupportedOperationException(
                                         Errors.GL_BLEND_SUBTRACT_UNSUPPORTED);
        }

        q(CMD_BLEND_EQUATION, 1);
        q(mode);
    }

    // OES_blend_func_separate

    public synchronized void glBlendFuncSeparate(int srcRGB, int dstRGB,
                                                 int srcAlpha, int dstAlpha) {
        checkThread();
        if (!GLConfiguration.supports_OES_blend_func_separate) {
            throw new UnsupportedOperationException(
                                    Errors.GL_BLEND_FUNC_SEPARATE_UNSUPPORTED);
        }

        q(CMD_BLEND_FUNC_SEPARATE, 4);
        q(srcRGB);
        q(dstRGB);
        q(srcAlpha);
        q(dstAlpha);
    }

    // OES_blend_equation_separate

    public synchronized void glBlendEquationSeparate(int modeRGB,
                                                     int modeAlpha) {
        checkThread();
        if (!GLConfiguration.supports_OES_blend_equation_separate) {
            throw new UnsupportedOperationException(
                                Errors.GL_BLEND_EQUATION_SEPARATE_UNSUPPORTED);
        }

        q(CMD_BLEND_EQUATION_SEPARATE, 2);
        q(modeRGB);
        q(modeAlpha);
    }

    // OES_framebuffer_object

    /**
     * Utility for common error checking.
     * 
     * @exception <code>UnsupportedOperationException</code> if the
     * underlying engine does not support the
     * <code>OES_framebuffer_object</code> extension.
     */
    void check_fbo() {
        if (!GLConfiguration.supports_OES_framebuffer_object) {
            throw new UnsupportedOperationException(
                                     Errors.GL_FRAMEBUFFER_OBJECT_UNSUPPORTED);
        }
    }

    public synchronized boolean glIsRenderbufferOES(int renderbuffer) {
        checkThread();
        check_fbo();

        qflush();

        grabContext();
        boolean retval = GL_TRUE == _glIsRenderbufferOES(renderbuffer);

        return retval;
    }
  
    public synchronized void glBindRenderbufferOES(int target,
                                                   int renderbuffer) {
        checkThread();
        check_fbo();

        q(CMD_BIND_RENDERBUFFER, 2);
        q(target);
        q(renderbuffer);
    }

    public synchronized void glDeleteRenderbuffersOES(int n,
                                                      int[] renderbuffers,
                                                      int offset) {
        checkThread();
        check_fbo();
        checkLength(renderbuffers, n, offset);
    
        IglDeleteRenderbuffersOES(n, renderbuffers, offset);
    }

    public synchronized void
        glDeleteRenderbuffersOES(int n, IntBuffer renderbuffers) {
        checkThread();
        check_fbo();
        checkLength(renderbuffers, n);

        if (!renderbuffers.isDirect()) {
            glDeleteRenderbuffersOES(n, renderbuffers.array(),
                                     offset(renderbuffers));
            return;
        }

        q(CMD_DELETE_RENDERBUFFERSB, 2);
        q(n);
        q(renderbuffers);

        qflush();
    }

    void IglDeleteRenderbuffersOES(int n, int[] renderbuffers, int offset) {
        q(CMD_DELETE_RENDERBUFFERS, n + 1);
        q(n);
        for (int i = 0; i < n; i++) {
            q(renderbuffers[offset + i]);
        }
    }

    public synchronized void glGenRenderbuffersOES(int n,
                                                   int[] renderbuffers,
                                                   int offset) {
        checkThread();
        check_fbo();
        checkLength(renderbuffers, n, offset);

        qflush();
        IglGenRenderbuffersOES(n, renderbuffers, offset);
    }

    public synchronized void glGenRenderbuffersOES(int n,
                                                   IntBuffer renderbuffers) {
        checkThread();
        check_fbo();
        checkLength(renderbuffers, n);

        if (!renderbuffers.isDirect()) {
            IglGenRenderbuffersOES(n, renderbuffers.array(),
                                   offset(renderbuffers));
            return;
        }

        q(CMD_GEN_RENDERBUFFERSB, 2);
        q(n);
        q(renderbuffers);

        qflush();
    }

    void IglGenRenderbuffersOES(int n, int[] renderbuffers, int offset) {
        grabContext();
        _glGenRenderbuffersOES(n, renderbuffers, offset);
    }

    public synchronized void glRenderbufferStorageOES(int target,
                                                      int internalformat,
                                                      int width, int height) {
        checkThread();
        check_fbo();

        q(CMD_RENDERBUFFER_STORAGE, 4);
        q(target);
        q(internalformat);
        q(width);
        q(height);
    }

    public synchronized void glGetRenderbufferParameterivOES(int target,
                                                             int pname,
                                                             int[] params,
                                                             int offset) {
        checkThread();
        check_fbo();
        int length = GLConfiguration.glRenderbufferParameterNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetRenderbufferParameterivOES(target, pname,
                                         params, offset, length);
    }

    public synchronized void
        glGetRenderbufferParameterivOES(int target, int pname,
                                        IntBuffer params) {
        checkThread();
        check_fbo();
        int length = GLConfiguration.glRenderbufferParameterNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetRenderbufferParameterivOES(target, pname, params.array(),
                                             offset(params), length);
        } else {
            IglGetRenderbufferParameterivOES(target, pname, null,
                                             pointer(params), length);
        }
    }

    void IglGetRenderbufferParameterivOES(int target,
                                          int pname,
                                          int[] params,
                                          int offset,
                                          int length) {
        grabContext();
        _glGetRenderbufferParameterivOES(target, pname, params,
                                         offset, length);
    }

    public synchronized boolean glIsFramebufferOES(int framebuffer) {
        checkThread();
        check_fbo();

        qflush();

        grabContext();
        boolean retval = GL_TRUE == _glIsFramebufferOES(framebuffer);
        return retval;
    }

    public synchronized void glBindFramebufferOES(int target,
                                                  int framebuffer) {
        checkThread();
        check_fbo();
    }

    public synchronized void glDeleteFramebuffersOES(int n,
                                                     int[] framebuffers,
                                                     int offset) {
        checkThread();
        check_fbo();
        checkLength(framebuffers, n, offset);

        IglDeleteFramebuffersOES(n, framebuffers, offset);
    }

    public synchronized void glDeleteFramebuffersOES(int n,
                                                     IntBuffer framebuffers) {
        checkThread();
        check_fbo();
        checkLength(framebuffers, n);

        if (!framebuffers.isDirect()) {
            glDeleteFramebuffersOES(n, framebuffers.array(),
                                    offset(framebuffers));
            return;
        }

        q(CMD_DELETE_FRAMEBUFFERSB, 2);
        q(n);
        q(framebuffers);

        qflush();
    }

    void IglDeleteFramebuffersOES(int n, int[] framebuffers, int offset) {
        q(CMD_DELETE_FRAMEBUFFERS, n + 1);
        q(n);
        for (int i = 0; i < n; i++) {
            q(framebuffers[offset + i]);
        }
    }

    public synchronized void glGenFramebuffersOES(int n,
                                                  int[] framebuffers,
                                                  int offset) {
        checkThread();
        check_fbo();
        checkLength(framebuffers, n, offset);

        qflush();
        IglGenFramebuffersOES(n, framebuffers, offset);
    }

    public synchronized void glGenFramebuffersOES(int n,
                                                  IntBuffer framebuffers) {
        checkThread();
        check_fbo();
        checkLength(framebuffers, n);

        if (!framebuffers.isDirect()) {
            IglGenRenderbuffersOES(n, framebuffers.array(),
                                   offset(framebuffers));
            return;
        }

        q(CMD_GEN_FRAMEBUFFERSB, 2);
        q(n);
        q(framebuffers);

        qflush();
    }

    void IglGenFramebuffersOES(int n, int[] framebuffers, int offset) {
        grabContext();
        _glGenFramebuffersOES(n, framebuffers, offset);
    }

    public synchronized int glCheckFramebufferStatusOES(int target) {
        checkThread();
        check_fbo();

        qflush();

        grabContext();
        int retval = _glCheckFramebufferStatusOES(target);
        return retval;
    }

    public synchronized void glFramebufferTexture2DOES(int target,
                                                       int attachment,
                                                       int textarget,
                                                       int texture,
                                                       int level) {
        checkThread();
        check_fbo();
    
        q(CMD_FRAMEBUFFER_TEXTURE2D, 5);
        q(target);
        q(attachment);
        q(textarget);
        q(textarget);
        q(level);
    }

    public synchronized void
        glFramebufferRenderbufferOES(int target,
                                     int attachment,
                                     int renderbuffertarget,
                                     int renderbuffer) {
        checkThread();
        check_fbo();

        q(CMD_FRAMEBUFFER_RENDERBUFFER, 4);
        q(target);
        q(attachment);
        q(renderbuffertarget);
        q(renderbuffer);
    }

    public synchronized void
        glGetFramebufferAttachmentParameterivOES(int target,
                                                 int attachment,
                                                 int pname,
                                                 int[] params,
                                                 int offset) {
        checkThread();
        check_fbo();
        int length =
            GLConfiguration.glFramebufferAttachmentParameterNumParams(pname);
        checkLength(params, length, offset);

        qflush();
        IglGetFramebufferAttachmentParameterivOES(target, attachment, pname,
                                                  params, offset, length);
    }

    public synchronized void
        glGetFramebufferAttachmentParameterivOES(int target,
                                                 int attachment,
                                                 int pname,
                                                 IntBuffer params) {
        checkThread();
        check_fbo();
        int length =
            GLConfiguration.glFramebufferAttachmentParameterNumParams(pname);
        checkLength(params, length);

        qflush();
        if (!params.isDirect()) {
            IglGetFramebufferAttachmentParameterivOES(target, attachment,
                                                      pname, params.array(),
                                                      offset(params), length);
        } else {
            IglGetFramebufferAttachmentParameterivOES(target, attachment,
                                                      pname, null,
                                                      pointer(params),
                                                      length);
        }
    }

    void IglGetFramebufferAttachmentParameterivOES(int target,
                                                   int attachment,
                                                   int pname,
                                                   int[] params,
                                                   int offset,
                                                   int length) {
        grabContext();
        _glGetFramebufferAttachmentParameterivOES(target, attachment,
                                                  pname, params,
                                                  offset, length);
    }

    public synchronized void glGenerateMipmapOES(int target) {
        checkThread();
        check_fbo();

        q(CMD_GENERATE_MIPMAP, 1);
        q(target);
    }

    public GL11Impl(EGLContext context) {
        super(context);
    }
}
