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

/* Native functions for ..\src\java\nio\Buffer.java and BufferManager.java */

#include <kni.h>
#include "JSR239-KNIInterface.h"
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

/*  static native int _allocNative ( int capacity ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_java_nio_ByteBufferImpl__1allocNative() {

    jint capacity = KNI_GetParameterAsInt(1);

    jint returnValue;

    returnValue = (jint)JSR239_malloc(capacity); // assumes 4 byte alignment
#ifdef DEBUG
    printf("_allocNative allocated %d bytes at %p\n", capacity, returnValue);
    fflush(stdout);
#endif

    KNI_ReturnInt(returnValue);
}

/*  static native void _copyBytes ( int srcAddress , int dstAddress , int bytes ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1copyBytes() {

    jint srcAddress = KNI_GetParameterAsInt(1);
    jint dstAddress = KNI_GetParameterAsInt(2);
    jint bytes = KNI_GetParameterAsInt(3);

#ifdef DEBUG
    printf("Copying %d bytes of data from %p to %p\n",
	   bytes, srcAddress, dstAddress);
    fflush(stdout);
#endif
    JSR239_memcpy((void *) dstAddress, (void *) srcAddress, bytes);

    KNI_ReturnVoid();
}

/*  static native byte _getByte ( int address ) ; */
KNIEXPORT KNI_RETURNTYPE_BYTE
Java_java_nio_ByteBufferImpl__1getByte() {

    jint address = KNI_GetParameterAsInt(1);

    jbyte returnValue = *((jbyte *) address);
#ifdef DEBUG
    printf("Reading a byte from %p -> %d\n", address, returnValue);
    fflush(stdout);
#endif

    KNI_ReturnByte(returnValue);
}

/*  static native void _getBytes ( int address , byte [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1getBytes() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Reading %d bytes from %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);
    KNI_SetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native void _putByte ( int address , byte value ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putByte() {

    jint address = KNI_GetParameterAsInt(1);
    jbyte value = KNI_GetParameterAsByte(2);

#ifdef DEBUG
    printf("Writing a byte (%d) to %p\n", value, address);
    fflush(stdout);
#endif
    *((jbyte *) address) = value;

    KNI_ReturnVoid();
}

/*  static native void _putBytes ( int address , byte [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putBytes() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Writing %d bytes to %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    KNI_GetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native short _getShort ( int address ) ; */
KNIEXPORT KNI_RETURNTYPE_SHORT
Java_java_nio_ByteBufferImpl__1getShort() {

    jint address = KNI_GetParameterAsInt(1);

    jshort returnValue = *((jshort *) address);
#ifdef DEBUG
    printf("Reading a short from %p -> %d\n", address, returnValue);
    fflush(stdout);
#endif

    KNI_ReturnShort(returnValue);
}

/*  static native void _getShorts ( int address , short [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1getShorts() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Reading %d shorts from %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jshort);
    length *= sizeof(jshort);
    KNI_SetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native void _putShort ( int address , short value ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putShort() {

    jint address = KNI_GetParameterAsInt(1);
    jshort value = KNI_GetParameterAsShort(2);

    *((jshort *) address) = value;
#ifdef DEBUG
    printf("Writing a short (%d) to %p\n", value, address);
    fflush(stdout);
#endif

    KNI_ReturnVoid();
}

/*  static native void _putShorts ( int address , short [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putShorts() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Writing %d shorts to %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jshort);
    length *= sizeof(jshort);
    KNI_GetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native int _getInt ( int address ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_java_nio_ByteBufferImpl__1getInt() {

    jint address = KNI_GetParameterAsInt(1);

    jint returnValue = *((jint *) address);
#ifdef DEBUG
    printf("Reading an int from %p -> %d\n", address, returnValue);
    fflush(stdout);
#endif

    KNI_ReturnInt(returnValue);
}

/*  static native void _getInts ( int address , int [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1getInts() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Reading %d ints from %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jint);
    length *= sizeof(jint);
    KNI_SetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native void _putInt ( int address , int value ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putInt() {

    jint address = KNI_GetParameterAsInt(1);
    jint value = KNI_GetParameterAsInt(2);

    *((jint *) address) = value;
#ifdef DEBUG
    printf("Writing an int (%d) to %p\n", value, address);
    fflush(stdout);
#endif

    KNI_ReturnVoid();
}

/*  static native void _putInts ( int address , int [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putInts() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Writing %d ints to %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jint);
    length *= sizeof(jint);
    KNI_GetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native float _getFloat ( int address ) ; */
KNIEXPORT KNI_RETURNTYPE_FLOAT
Java_java_nio_ByteBufferImpl__1getFloat() {

    jint address = KNI_GetParameterAsInt(1);

    jfloat returnValue = *((jfloat *) address);
#ifdef DEBUG
    printf("Reading a float from %p -> %f\n", address, returnValue);
    fflush(stdout);
#endif

    KNI_ReturnFloat(returnValue);
}

/*  static native void _getFloats ( int address , float [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1getFloats() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Reading %d floats from %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jfloat);
    length *= sizeof(jfloat);
    KNI_SetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  static native void _putFloat ( int address , float value ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putFloat() {

    jint address = KNI_GetParameterAsInt(1);
    jfloat value = KNI_GetParameterAsFloat(2);

#ifdef DEBUG
    printf("Writing a float (%f) to %p\n", value, address);
    fflush(stdout);
#endif
    *((jfloat *) address) = value;

    KNI_ReturnVoid();
}

/*  static native void _putFloats ( int address , float [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl__1putFloats() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Writing %d floats to %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);

    offset *= sizeof(jfloat);
    length *= sizeof(jfloat);
    KNI_GetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* BufferManager */

/*  static native void _getBytes ( int address , byte [ ] dst , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_jsr239_BufferManager__1getBytes() {

    jint address = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(dstHandle);

#ifdef DEBUG
    printf("Reading %d bytes from %p\n", length, address);
    fflush(stdout);
#endif

    KNI_GetParameterAsObject(2, dstHandle);
    KNI_SetRawArrayRegion(dstHandle, offset, length, (jbyte *) address);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/* native private void finalize();*/
/* Macro to retrieve C structure representation of an Object */
typedef struct Java_java_nio_ByteBufferImpl _byte_buffer_impl;

#define getByteBuffer(handle) (unhand(_byte_buffer_impl,(handle)))

KNIEXPORT KNI_RETURNTYPE_VOID
Java_java_nio_ByteBufferImpl_finalize() {
    _byte_buffer_impl* buffer;
    void* native_buffer_address;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    buffer = getByteBuffer(thisObject);
    native_buffer_address = (void*)buffer->arrayOffset;

    if (native_buffer_address && !buffer->directParent) {        
        JSR239_free(native_buffer_address);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
#undef getByteBuffer


