/*
 * @(#)ObjectReferenceImpl.c	1.30 06/10/10
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
#include "ObjectReferenceImpl.h"
#include "util.h"
#include "commonRef.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"

static jboolean 
referenceType(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jbyte tag;
    jclass clazz;

    jobject object = inStream_readObjectRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, 1);

    clazz = (*env)->GetObjectClass(env, object);
    tag = referenceTypeTag(clazz);

    outStream_writeByte(out, tag);
    WRITE_LOCAL_REF(env, out, clazz);

    END_WITH_LOCAL_REFS(env);
    return JNI_TRUE;
}

static jboolean 
getValues(PacketInputStream *in, PacketOutputStream *out)
{
    sharedGetFieldValues(in, out, JNI_FALSE);
    return JNI_TRUE;
}


static jint
readFieldValue(JNIEnv *env, PacketInputStream *in, jclass clazz,
               jobject object, jfieldID field, char *signature)
{
    jvalue value;
    jint error = JVMDI_ERROR_NONE;

    switch (signature[0]) {
        case JDWP_Tag_ARRAY:
        case JDWP_Tag_OBJECT:
            value.l = inStream_readObjectRef(in);
            (*env)->SetObjectField(env, object, field, value.l);
            break;
        
        case JDWP_Tag_BYTE:
            value.b = inStream_readByte(in);
            (*env)->SetByteField(env, object, field, value.b);
            break;

        case JDWP_Tag_CHAR:
            value.c = inStream_readChar(in);
            (*env)->SetCharField(env, object, field, value.c);
            break;

        case JDWP_Tag_FLOAT:
            value.f = inStream_readFloat(in);
            (*env)->SetFloatField(env, object, field, value.f);
            break;

        case JDWP_Tag_DOUBLE:
            value.d = inStream_readDouble(in);
            (*env)->SetDoubleField(env, object, field, value.d);
            break;

        case JDWP_Tag_INT:
            value.i = inStream_readInt(in);
            (*env)->SetIntField(env, object, field, value.i);
            break;

        case JDWP_Tag_LONG:
            value.j = inStream_readLong(in);
            (*env)->SetLongField(env, object, field, value.j);
            break;

        case JDWP_Tag_SHORT:
            value.s = inStream_readShort(in);
            (*env)->SetShortField(env, object, field, value.s);
            break;

        case JDWP_Tag_BOOLEAN:
            value.z = inStream_readBoolean(in);
            (*env)->SetBooleanField(env, object, field, value.z);
            break;
    }

    if ((*env)->ExceptionOccurred(env)) {
        error = JVMDI_ERROR_INTERNAL;
    }
    
    return error;
}

static jboolean 
setValues(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint i;
    jfieldID field;
    char *signature;
    jint count;
    jint error = JVMDI_ERROR_NONE;
    jclass clazz;

    jobject object = inStream_readObjectRef(in);
    count = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, count + 1);

    clazz = (*env)->GetObjectClass(env, object);
    
    for (i = 0; (i < count) && 
                (error == JVMDI_ERROR_NONE) &&
                !inStream_error(in); i++) {
        field = inStream_readFieldID(in);

        error = fieldSignature(clazz, field, &signature);
        if (error == JVMDI_ERROR_NONE) {
            error = readFieldValue(env, in, clazz, object, field, signature);
            jdwpFree(signature);
        }

    }

    END_WITH_LOCAL_REFS(env);

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

static jboolean 
monitorInfo(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint error;
    JVMDI_monitor_info info;
    jint i;

    jobject object = inStream_readObjectRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = jvmdi->GetMonitorInfo(object, &info);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    WRITE_GLOBAL_REF(env, out, info.owner);
    outStream_writeInt(out, info.entry_count);
    outStream_writeInt(out, info.waiter_count);
    for (i = 0; i < info.waiter_count; i++) {
        WRITE_GLOBAL_REF(env, out, info.waiters[i]);
    }

    jdwpFree(info.waiters);
    return JNI_TRUE;
}

static jboolean 
invokeInstance(PacketInputStream *in, PacketOutputStream *out)
{
    return sharedInvoke(in, out);
}

static jboolean 
disableCollection(PacketInputStream *in, PacketOutputStream *out)
{
    jlong id = inStream_readObjectID(in);
    jint error;
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = commonRef_pin(id);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }

    return JNI_TRUE;
}

static jboolean 
enableCollection(PacketInputStream *in, PacketOutputStream *out)
{
    jint error;
    jlong id = inStream_readObjectID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = commonRef_unpin(id);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }

    return JNI_TRUE;
}

static jboolean 
isCollected(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jobject ref;
    jlong id = inStream_readObjectID(in);

    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    if (id == NULL_OBJECT_ID) {
        outStream_setError(out, JVMDI_ERROR_INVALID_OBJECT);
        return JNI_TRUE;
    }

    ref = commonRef_idToRef(id);
    outStream_writeBoolean(out, (jboolean)(ref == NULL));

    (*env)->DeleteGlobalRef(env, ref);

    return JNI_TRUE;
}

void *ObjectReference_Cmds[] = { (void *)0x9
    ,(void *)referenceType
    ,(void *)getValues
    ,(void *)setValues
    ,(void *)NULL      /* no longer used */
    ,(void *)monitorInfo
    ,(void *)invokeInstance
    ,(void *)disableCollection
    ,(void *)enableCollection
    ,(void *)isCollected
    };
