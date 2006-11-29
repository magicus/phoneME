/*
 * @(#)outStream.h	1.18 06/10/10
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
#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include "typedefs.h"
#undef GetMonitorInfo
#include <jni.h>
#include <jvmdi.h>
#include "transport.h"

struct bag;

#define INITIAL_SEGMENT_SIZE   300
#define MAX_SEGMENT_SIZE     10000

typedef struct PacketOutputStream {
    jbyte *current;
    jint left;
    struct PacketData *segment;
    jint error;
    jboolean sent;
    struct Packet packet;
    jbyte initialSegment[INITIAL_SEGMENT_SIZE];
    struct bag *ids;
} PacketOutputStream;

void outStream_initCommand(PacketOutputStream *stream, jint id, 
                           jbyte flags, jbyte commandSet, jbyte command);
void outStream_initReply(PacketOutputStream *stream, jint id);

jint outStream_id(PacketOutputStream *stream);
jbyte outStream_command(PacketOutputStream *stream);

jint outStream_writeBoolean(PacketOutputStream *stream, jboolean val);
jint outStream_writeByte(PacketOutputStream *stream, jbyte val);
jint outStream_writeChar(PacketOutputStream *stream, jchar val);
jint outStream_writeShort(PacketOutputStream *stream, jshort val);
jint outStream_writeInt(PacketOutputStream *stream, jint val);
jint outStream_writeLong(PacketOutputStream *stream, jlong val);
jint outStream_writeFloat(PacketOutputStream *stream, jfloat val);
jint outStream_writeDouble(PacketOutputStream *stream, jdouble val);
jint outStream_writeObjectRef(PacketOutputStream *stream, jobject val);
jint outStream_writeObjectTag(PacketOutputStream *stream, jobject val);
jint outStream_writeClassID(PacketOutputStream *stream, jclass val);
jint outStream_writeFrameID(PacketOutputStream *stream, jframeID val);
jint outStream_writeMethodID(PacketOutputStream *stream, jmethodID val);
jint outStream_writeFieldID(PacketOutputStream *stream, jfieldID val);
jint outStream_writeLocation(PacketOutputStream *stream, jlocation val);

#define WRITE_LOCAL_REF(env, stream, ref) \
    outStream_writeObjectRef(stream, ref)
#define WRITE_GLOBAL_REF(env, stream, ref) \
    do {                                   \
        outStream_writeObjectRef(stream, ref); \
        if (ref) (*env)->DeleteGlobalRef(env, ref); \
        ref = (jobject)(intptr_t)-1;  \
    } while (0)

jint outStream_writeByteArray(PacketOutputStream*stream, jint length, jbyte *bytes);
jint outStream_writeString(PacketOutputStream *stream, char *string);
void outStream_writeValue(JNIEnv *env, struct PacketOutputStream *out, 
                          jbyte typeKey, jvalue value);
jint outStream_skipBytes(PacketOutputStream *stream, jint count);

jint outStream_error(PacketOutputStream *stream);
void outStream_setError(PacketOutputStream *stream, jint error);

void outStream_sendReply(PacketOutputStream *stream);
void outStream_sendCommand(PacketOutputStream *stream);

void outStream_destroy(PacketOutputStream *stream);

#endif /* OUTSTREAM_H */
