/*
 * @(#)ClassTypeImpl.c	1.32 06/10/10
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

#include "ClassTypeImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"

static jboolean 
superclass(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jclass superclass;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, 1);

    superclass = (*env)->GetSuperclass(env,clazz);
    WRITE_LOCAL_REF(env, out, superclass);

    END_WITH_LOCAL_REFS(env);
    return JNI_TRUE;
}

static jint
readStaticFieldValue(JNIEnv *env, PacketInputStream *in, jclass clazz,
                     jfieldID field, char *signature)
{
    jvalue value;
    jint error = JVMDI_ERROR_NONE;

    switch (signature[0]) {
        case JDWP_Tag_ARRAY:
        case JDWP_Tag_OBJECT:
            value.l = inStream_readObjectRef(in);
            (*env)->SetStaticObjectField(env, clazz, field, value.l);
            break;
        
        case JDWP_Tag_BYTE:
            value.b = inStream_readByte(in);
            (*env)->SetStaticByteField(env, clazz, field, value.b);
            break;

        case JDWP_Tag_CHAR:
            value.c = inStream_readChar(in);
            (*env)->SetStaticCharField(env, clazz, field, value.c);
            break;

        case JDWP_Tag_FLOAT:
            value.f = inStream_readFloat(in);
            (*env)->SetStaticFloatField(env, clazz, field, value.f);
            break;

        case JDWP_Tag_DOUBLE:
            value.d = inStream_readDouble(in);
            (*env)->SetStaticDoubleField(env, clazz, field, value.d);
            break;

        case JDWP_Tag_INT:
            value.i = inStream_readInt(in);
            (*env)->SetStaticIntField(env, clazz, field, value.i);
            break;

        case JDWP_Tag_LONG:
            value.j = inStream_readLong(in);
            (*env)->SetStaticLongField(env, clazz, field, value.j);
            break;

        case JDWP_Tag_SHORT:
            value.s = inStream_readShort(in);
            (*env)->SetStaticShortField(env, clazz, field, value.s);
            break;

        case JDWP_Tag_BOOLEAN:
            value.z = inStream_readBoolean(in);
            (*env)->SetStaticBooleanField(env, clazz, field, value.z);
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

    jclass clazz = inStream_readClassRef(in);
    count = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, count);

    for (i = 0; (i < count) && (error == JVMDI_ERROR_NONE); i++) {
        field = inStream_readFieldID(in);

        error = fieldSignature(clazz, field, &signature);
        if (error == JVMDI_ERROR_NONE) {
            error = readStaticFieldValue(env, in, clazz, 
                                         field, signature);
            jdwpFree(signature);
        }
    }

    END_WITH_LOCAL_REFS(env);
    return JNI_TRUE;
}

static jboolean 
invokeStatic(PacketInputStream *in, PacketOutputStream *out)
{
    return sharedInvoke(in, out);
}

/* Now done on front end 
static jboolean 
validateObjectAssignments(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint i;

    jclass clazz = inStream_readClassRef(in);
    jint count = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    outStream_writeInt(out, count);
    for (i = 0; i < count; i++) {
        jint result;
        jobject object;
        char *signature = inStream_readString(in);
        if (signature == NULL) {
            outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
            return JNI_TRUE;
        }
        object = inStream_readObjectRef(in);
        if (inStream_error(in)) {
            return JNI_TRUE;
        }
        result = validateAssignment(clazz, object, signature);
        outStream_writeInt(out, result);
        jdwpFree(signature);
    }

    return JNI_TRUE;
} */

void *ClassType_Cmds[] = { (void *)0x4
    ,(void *)superclass
    ,(void *)setValues
    ,(void *)invokeStatic
    ,(void *)invokeStatic
};

