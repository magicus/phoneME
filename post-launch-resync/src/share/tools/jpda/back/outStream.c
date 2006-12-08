/*
 * @(#)outStream.c	1.26 06/10/10
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

#include <string.h>
#include <assert.h>

#include "util.h"
#include "stream.h"
#include "outStream.h"
#include "inStream.h"
#include "transport.h"
#include "commonRef.h"
#include "bag.h"
#include "JDWP.h"

#define INITIAL_ID_ALLOC  50

static void
commonInit(PacketOutputStream *stream)
{
    stream->current = &stream->initialSegment[0];
    stream->left = sizeof(stream->initialSegment);
    stream->segment = &stream->packet.type.cmd.data;
    stream->segment->length = 0;
    stream->segment->data = &stream->initialSegment[0];
    stream->segment->next = NULL;
    stream->error = JVMDI_ERROR_NONE;
    stream->sent = JNI_FALSE;
    stream->ids = bagCreateBag(sizeof(jlong), INITIAL_ID_ALLOC);
    if (stream->ids == NULL) {
        stream->error = JVMDI_ERROR_OUT_OF_MEMORY;
    }
}

void 
outStream_initCommand(PacketOutputStream *stream, jint id, 
                      jbyte flags, jbyte commandSet, jbyte command)
{
    commonInit(stream);

    /*
     * Command-specific initialization
     */
    stream->packet.type.cmd.id = id;
    stream->packet.type.cmd.cmdSet = commandSet;
    stream->packet.type.cmd.cmd = command;

    stream->packet.type.cmd.flags = flags;
}

void 
outStream_initReply(PacketOutputStream *stream, jint id)
{
    commonInit(stream);

    /*
     * Reply-specific initialization
     */
    stream->packet.type.reply.id = id;
    stream->packet.type.reply.errorCode = REPLY_NoError;
    stream->packet.type.cmd.flags = FLAGS_Reply;
}

jint 
outStream_id(PacketOutputStream *stream)
{
    return stream->packet.type.cmd.id;
}

jbyte 
outStream_command(PacketOutputStream *stream)
{
    /* Only makes sense for commands */
    JDI_ASSERT(!(stream->packet.type.cmd.flags & FLAGS_Reply));
    return stream->packet.type.cmd.cmd;
}

static jint 
writeBytes(PacketOutputStream *stream, void *source, int size)
{
    jbyte *bytes = (jbyte *)source;

    if (stream->error) {
        return stream->error;
    }
    while (size > 0) {
        jint count;
        if (stream->left == 0) {
            jint segSize = MIN(2 * stream->segment->length, MAX_SEGMENT_SIZE);
            jbyte *newSeg = jdwpAlloc(segSize);
            struct PacketData *newHeader = jdwpAlloc(sizeof(*newHeader));
            if ((newSeg == NULL) || (newHeader == NULL)) {
                jdwpFree(newSeg);
                jdwpFree(newHeader);
                stream->error = JVMDI_ERROR_OUT_OF_MEMORY;
                return stream->error;
            }
            newHeader->length = 0;
            newHeader->data = newSeg;
            newHeader->next = NULL;
            stream->segment->next = newHeader;
            stream->segment = newHeader;
            stream->current = newHeader->data;
            stream->left = segSize;
        }
        count = MIN(size, stream->left);
        memcpy(stream->current, bytes, count);
        stream->current += count;
        stream->left -= count;
        stream->segment->length += count;
        size -= count;
        bytes += count;
    }
    return JVMDI_ERROR_NONE;
}

jint 
outStream_writeBoolean(PacketOutputStream *stream, jboolean val)
{
    jbyte byte = (val != 0) ? 1 : 0;
    return writeBytes(stream, &byte, sizeof(byte));
}

jint 
outStream_writeByte(PacketOutputStream *stream, jbyte val)
{
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeChar(PacketOutputStream *stream, jchar val)
{
    val = HOST_TO_JAVA_CHAR(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeShort(PacketOutputStream *stream, jshort val)
{
    val = HOST_TO_JAVA_SHORT(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeInt(PacketOutputStream *stream, jint val)
{
    val = HOST_TO_JAVA_INT(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeLong(PacketOutputStream *stream, jlong val)
{
    val = HOST_TO_JAVA_LONG(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeFloat(PacketOutputStream *stream, jfloat val)
{
    val = HOST_TO_JAVA_FLOAT(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeDouble(PacketOutputStream *stream, jdouble val)
{
    val = HOST_TO_JAVA_DOUBLE(val);
    return writeBytes(stream, &val, sizeof(val));
}

jint 
outStream_writeObjectTag(PacketOutputStream *stream, jobject val)
{
    return outStream_writeByte(stream, specificTypeKey(val));
}

jint 
outStream_writeObjectRef(PacketOutputStream *stream, jobject val)
{
    jlong id;
    jlong *idPtr;

    if (stream->error) {
        return stream->error;
    }

    if (val == NULL) {
        id = NULL_OBJECT_ID;
    } else {
        /* Convert the object to an object id */
        id = commonRef_refToID(val);
        if (id == NULL_OBJECT_ID) {
            stream->error = JVMDI_ERROR_OUT_OF_MEMORY;
            return stream->error;
        }

        /* Track the common ref in case we need to release it on a future error */
        idPtr = bagAdd(stream->ids);
        if (idPtr == NULL) {
            commonRef_release(id);
            stream->error = JVMDI_ERROR_OUT_OF_MEMORY;
            return stream->error;
        } else {
            *idPtr = id;
        }

        /* Add the encoded object id to the stream */
        id = HOST_TO_JAVA_LONG(id);
    }

    return writeBytes(stream, &id, sizeof(id));
}

jint 
outStream_writeClassRef(PacketOutputStream *stream, jclass val)
{
    return outStream_writeObjectRef(stream, val);
}

jint 
outStream_writeFrameID(PacketOutputStream *stream, jframeID val)
{
    /*
     * Not good - we're writing a pointer as a jint.  Need
     * to write as a jlong if sizeof(jframeID) == 8.
     */
#ifdef CVM_64
    assert(sizeof(jframeID) == sizeof(jlong));
    return outStream_writeLong(stream, (jlong)val);
#else
    return outStream_writeInt(stream, (jint)val);
#endif
}

jint 
outStream_writeMethodID(PacketOutputStream *stream, jmethodID val)
{
    /*
     * Not good - we're writing a pointer as a jint.  Need
     * to write as a jlong if sizeof(jmethodID) == 8.
     */
#ifdef CVM_64
    assert(sizeof(jmethodID) == sizeof(jlong));
    return outStream_writeLong(stream, (jlong)val);
#else
    return outStream_writeInt(stream, (jint)val);
#endif
}

jint 
outStream_writeFieldID(PacketOutputStream *stream, jfieldID val)
{
    /*
     * Not good - we're writing a pointer as a jint.  Need
     * to write as a jlong if sizeof(jfieldID) == 8.
     */
#ifdef CVM_64
    assert(sizeof(jfieldID) == sizeof(jlong));
    return outStream_writeLong(stream, (jlong)val);
#else
    return outStream_writeInt(stream, (jint)val);
#endif
}

jint 
outStream_writeLocation(PacketOutputStream *stream, jlocation val)
{
    return outStream_writeLong(stream, (jlong)val);
}

jint 
outStream_writeByteArray(PacketOutputStream*stream, jint length, 
                         jbyte *bytes)
{
    outStream_writeInt(stream, length);
    return writeBytes(stream, bytes, length);
}

jint 
outStream_writeString(PacketOutputStream *stream, char *string)
{
    jint length = strlen(string);
    outStream_writeInt(stream, length);
    return writeBytes(stream, (jbyte *)string, length);
}

void 
outStream_writeValue(JNIEnv *env, PacketOutputStream *out, 
                     jbyte typeKey, jvalue value)
{
    if (typeKey == JDWP_Tag_OBJECT) {
        outStream_writeByte(out, specificTypeKey(value.l));
    } else {
        outStream_writeByte(out, typeKey);
    }
    if (isObjectTag(typeKey)) {
        WRITE_GLOBAL_REF(env, out, value.l);
    } else {
        switch (typeKey) {
            case JDWP_Tag_BYTE:
                outStream_writeByte(out, value.b);
                break;
    
            case JDWP_Tag_CHAR:
                outStream_writeChar(out, value.c);
                break;
    
            case JDWP_Tag_FLOAT:
                outStream_writeFloat(out, value.f);
                break;
    
            case JDWP_Tag_DOUBLE:
                outStream_writeDouble(out, value.d);
                break;
    
            case JDWP_Tag_INT:
                outStream_writeInt(out, value.i);
                break;
    
            case JDWP_Tag_LONG:
                outStream_writeLong(out, value.j);
                break;
    
            case JDWP_Tag_SHORT:
                outStream_writeShort(out, value.s);
                break;
    
            case JDWP_Tag_BOOLEAN:
                outStream_writeBoolean(out, value.z);
                break;
    
            case JDWP_Tag_VOID:  /* happens with function return values */   
                /* write nothing */
                break;

            default:
                ERROR_MESSAGE_EXIT("Invalid type key");
        }
    }
}

jint 
outStream_skipBytes(PacketOutputStream *stream, jint count)
{
    int i;
    for (i = 0; i < count; i++) {
        outStream_writeByte(stream, 0);
    }
    return stream->error;
}

jint 
outStream_error(PacketOutputStream *stream)
{
    return stream->error;
}

void 
outStream_setError(PacketOutputStream *stream, jint error)
{
    if (!stream->error) {
        stream->error = error;
    }
}

void 
outStream_sendReply(PacketOutputStream *stream)
{
    jint error;
    if (stream->error) {
        /*
         * Don't send any collected stream data on an error reply
         */
        stream->packet.type.reply.data.length = 0;
        stream->packet.type.reply.errorCode = (jshort)stream->error;
    } 
    error = transport_sendPacket(&stream->packet);
    if (error == 0) {
        stream->sent = JNI_TRUE;
    }
}

void 
outStream_sendCommand(PacketOutputStream *stream)
{
    jint error;
    if (!stream->error) {
        error = transport_sendPacket(&stream->packet);
        if (error == 0) {
            stream->sent = JNI_TRUE;
        }
    } 
}


static jboolean 
releaseID(void *elementPtr, void *arg) 
{
    jlong *idPtr = elementPtr;
    commonRef_release(*idPtr);
    return JNI_TRUE;
}

void 
outStream_destroy(PacketOutputStream *stream)
{
    struct PacketData *next;

    if (stream->error || !stream->sent) {
        bagEnumerateOver(stream->ids, releaseID, NULL);
    }

    next = stream->packet.type.cmd.data.next;
    while (next != NULL) {
        struct PacketData *p = next;
        next = p->next;
        jdwpFree(p->data);
        jdwpFree(p);
    }
    bagDestroyBag(stream->ids);
}

