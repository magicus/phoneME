/*
 * @(#)inStream.h	1.15 06/10/10
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
#ifndef INSTREAM_H
#define INSTREAM_H

#include <jni.h>
#include <jvmdi.h>
#include "transport.h"

struct bag;

typedef struct PacketInputStream {
    jbyte *current;
    struct PacketData *segment;
    jint left;
    jint error;
    struct Packet packet;
    struct bag *refs;
} PacketInputStream;

void inStream_init(PacketInputStream *stream, struct Packet packet);

jint inStream_id(PacketInputStream *stream);
jbyte inStream_command(PacketInputStream *stream);

jboolean inStream_readBoolean(PacketInputStream *stream);
jbyte inStream_readByte(PacketInputStream *stream);
jbyte* inStream_readBytes(PacketInputStream *stream, 
                          int length, jbyte *buf);
jchar inStream_readChar(PacketInputStream *stream);
jshort inStream_readShort(PacketInputStream *stream);
jint inStream_readInt(PacketInputStream *stream);
jlong inStream_readLong(PacketInputStream *stream);
jfloat inStream_readFloat(PacketInputStream *stream);
jdouble inStream_readDouble(PacketInputStream *stream);
jlong inStream_readObjectID(PacketInputStream *stream);
jframeID inStream_readFrameID(PacketInputStream *stream);
jmethodID inStream_readMethodID(PacketInputStream *stream);
jfieldID inStream_readFieldID(PacketInputStream *stream);
jlocation inStream_readLocation(PacketInputStream *stream);

jobject inStream_readObjectRef(PacketInputStream *stream);
jclass inStream_readClassRef(PacketInputStream *stream);
jthread inStream_readThreadRef(PacketInputStream *stream);
jthreadGroup inStream_readThreadGroupRef(PacketInputStream *stream);
jobject inStream_readClassLoaderRef(PacketInputStream *stream);
jstring inStream_readStringRef(PacketInputStream *stream);
jarray inStream_readArrayRef(PacketInputStream *stream);

char *inStream_readString(PacketInputStream *stream);
jvalue inStream_readValue(struct PacketInputStream *in, jbyte *typeKeyPtr);

jint inStream_skipBytes(PacketInputStream *stream, jint count);

jboolean inStream_endOfInput(PacketInputStream *stream);
jint inStream_error(PacketInputStream *stream);

void inStream_destroy(PacketInputStream *stream);

#endif /* INSTREAM_H */
