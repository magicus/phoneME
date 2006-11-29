/*
 * @(#)StackFrameImpl.c	1.32 06/10/10
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
#include <stdlib.h>
#include <string.h>

#include "StackFrameImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"
#include "threadControl.h"
#include "popFrames.h"

static jint
validateFrame(jthread thread, jframeID frame)
{
    /*
     * Some implementations of JVMDI (notably classic) can crash
     * and burn if the frame id is stale, so we validate the frame
     * beforehand even though it does cost some.
     */
    jframeID candidate;

    jint error = jvmdi->GetCurrentFrame(thread, &candidate);
    if (error != JVMDI_ERROR_NONE) {
        return error;
    }

    while ((error == JVMDI_ERROR_NONE) && (frame != candidate)) {
        error = jvmdi->GetCallerFrame(candidate, &candidate);
    }

    /*
     * End of stack means the frame is not valid
     */
    if (error == JVMDI_ERROR_NO_MORE_FRAMES) {
        error = JVMDI_ERROR_INVALID_FRAMEID;
    }

    /*
     * Note that its possible for some other thread to resume the 
     * thread here and make a validated frame become invalid. 
     * This is a hole, that we can't currently close because
     * JVMDI SuspendThread does not prevent app threads from resuming
     * the threads it suspends. For this reason, front end implementations
     * should avoid using invalid frames as much as possible.
     */
    return error;
}
static jint 
writeVariableValue(PacketOutputStream *out, 
                   jframeID frame, jint slot, jbyte typeKey)
{
    jint error;
    jvalue value;
    JNIEnv *env = getEnv();

    if (isObjectTag(typeKey)) {
        error = jvmdi->GetLocalObject(frame, slot, &value.l);
        if (error != JVMDI_ERROR_NONE) {
            outStream_setError(out, error);
        } else {
            outStream_writeByte(out, specificTypeKey(value.l));
            WRITE_GLOBAL_REF(env, out, value.l);
        }
    } else {
        /*
         * For primitive types, the type key is bounced back as is.
         */
        outStream_writeByte(out, typeKey);
        switch (typeKey) {
            case JDWP_Tag_BYTE: {
                    jint intValue;
                    error = jvmdi->GetLocalInt(frame, slot, &intValue);
                    outStream_writeByte(out, (jbyte)intValue);
                    break;
                }
    
            case JDWP_Tag_CHAR: {
                    jint intValue;
                    error = jvmdi->GetLocalInt(frame, slot, &intValue);
                    outStream_writeChar(out, (jchar)intValue);
                    break;
                }
    
            case JDWP_Tag_FLOAT:
                error = jvmdi->GetLocalFloat(frame, slot, &value.f);
                outStream_writeFloat(out, value.f);
                break;
    
            case JDWP_Tag_DOUBLE:
                error = jvmdi->GetLocalDouble(frame, slot, &value.d);
                outStream_writeDouble(out, value.d);
                break;
    
            case JDWP_Tag_INT:
                error = jvmdi->GetLocalInt(frame, slot, &value.i);
                outStream_writeInt(out, value.i);
                break;
    
            case JDWP_Tag_LONG:
                error = jvmdi->GetLocalLong(frame, slot, &value.j);
                outStream_writeLong(out, value.j);
                break;
    
            case JDWP_Tag_SHORT: {
                jint intValue;
                error = jvmdi->GetLocalInt(frame, slot, &intValue);
                outStream_writeShort(out, (jshort)intValue);
                break;
            }
    
            case JDWP_Tag_BOOLEAN:{
                jint intValue;
                error = jvmdi->GetLocalInt(frame, slot, &intValue);
                outStream_writeBoolean(out, (jboolean)intValue);
                break;
            }
    
            default:
                error = JDWP_Error_INVALID_TAG;
                break;
        }
    }

    return error;
}

static jint
readVariableValue(JNIEnv *env, PacketInputStream *in, 
                  jframeID frame, jint slot, jbyte typeKey)
{
    jint error;
    jvalue value;

    if (isObjectTag(typeKey)) {
        value.l = inStream_readObjectRef(in);
        error = jvmdi->SetLocalObject(frame, slot, value.l);
    } else {
        switch (typeKey) {
            case JDWP_Tag_BYTE:
                value.b = inStream_readByte(in);
                error = jvmdi->SetLocalInt(frame, slot, value.b);
                break;
    
            case JDWP_Tag_CHAR:
                value.c = inStream_readChar(in);
                error = jvmdi->SetLocalInt(frame, slot, value.c);
                break;
    
            case JDWP_Tag_FLOAT:
                value.f = inStream_readFloat(in);
                error = jvmdi->SetLocalFloat(frame, slot, value.f);
                break;
    
            case JDWP_Tag_DOUBLE:
                value.d = inStream_readDouble(in);
                error = jvmdi->SetLocalDouble(frame, slot, value.d);
                break;
    
            case JDWP_Tag_INT:
                value.i = inStream_readInt(in);
                error = jvmdi->SetLocalInt(frame, slot, value.i);
                break;
    
            case JDWP_Tag_LONG:
                value.j = inStream_readLong(in);
                error = jvmdi->SetLocalLong(frame, slot, value.j);
                break;
    
            case JDWP_Tag_SHORT:
                value.s = inStream_readShort(in);
                error = jvmdi->SetLocalInt(frame, slot, value.s);
                break;
    
            case JDWP_Tag_BOOLEAN:
                value.z = inStream_readBoolean(in);
                error = jvmdi->SetLocalInt(frame, slot, value.z);
                break;
    
            default:
                error = JDWP_Error_INVALID_TAG;
                break;
        }
    }

    return error;
}

static jboolean 
getValues(PacketInputStream *in, PacketOutputStream *out)
{
    jint i;
    jint error;

    jthread thread = inStream_readThreadRef(in);
    jframeID frame = inStream_readFrameID(in);
    jint variableCount = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * Validate the frame id
     */
    error = validateFrame(thread, frame);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeInt(out, variableCount);
    for (i = 0; (i < variableCount) && !outStream_error(out); i++) {
        jint error;
        jint slot = inStream_readInt(in);
        jbyte typeKey = inStream_readByte(in);

        error = writeVariableValue(out, frame, slot, typeKey);
        if (error != JVMDI_ERROR_NONE) {
            outStream_setError(out, error);
            return JNI_TRUE;
        }
    }
    return JNI_TRUE;
}

static jboolean 
setValues(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint i;
    jint error = JVMDI_ERROR_NONE;

    jthread thread = inStream_readThreadRef(in);
    jframeID frame = inStream_readFrameID(in);
    jint variableCount = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * Validate the frame id
     */
    error = validateFrame(thread, frame);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    for (i = 0; (i < variableCount) &&
                (error == JVMDI_ERROR_NONE) &&
                !inStream_error(in); i++) {
        jint slot = inStream_readInt(in);
        jbyte typeKey = inStream_readByte(in);

        error = readVariableValue(env, in, frame, slot, typeKey);
    }

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }

    return JNI_TRUE;
}

static jboolean 
thisObject(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint error;
    jclass clazz;
    jmethodID method;
    jlocation location;
    jint modifiers;
    jobject thisObject;

    jthread thread = inStream_readThreadRef(in);
    jframeID frame = inStream_readFrameID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * Validate the frame id
     */
    error = validateFrame(thread, frame);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    /* 
     * Find out if the given frame is for a static or native method.
     */
    error = threadControl_getFrameLocation(thread, frame, &clazz, &method, &location);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    error = jvmdi->GetMethodModifiers(clazz, method, &modifiers);

    (*env)->DeleteGlobalRef(env, clazz);

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    /*
     * Return null for static or native methods; otherwise, the JVM
     * spec guarantees that "this" is in slot 0
     */
    if (modifiers & (MOD_STATIC | MOD_NATIVE)) {
        thisObject = NULL;
    } else {
        error = jvmdi->GetLocalObject(frame, 0, &thisObject);
        if (error != JVMDI_ERROR_NONE) {
            outStream_setError(out, error);
            return JNI_TRUE;
        }
    }

    outStream_writeByte(out, specificTypeKey(thisObject));
    WRITE_GLOBAL_REF(env, out, thisObject);
    return JNI_TRUE;
}

static jboolean 
popFrames(PacketInputStream *in, PacketOutputStream *out) 
{
    jint error;
    jthread thread;
    jframeID frame;

    thread = inStream_readThreadRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    frame = inStream_readFrameID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * Validate the frame id
     */
    error = validateFrame(thread, frame);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    if (threadControl_isDebugThread(thread)) {
        outStream_setError(out, JVMDI_ERROR_INVALID_THREAD);
        return JNI_TRUE;
    }

    error = popFrames_pop(thread, frame);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

void *StackFrame_Cmds[] = { (void *)0x4
    ,(void *)getValues
    ,(void *)setValues
    ,(void *)thisObject
    ,(void *)popFrames
};
