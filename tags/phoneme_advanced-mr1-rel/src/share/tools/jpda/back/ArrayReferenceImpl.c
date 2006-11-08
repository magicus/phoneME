/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ArrayReferenceImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"

static jboolean 
length(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jsize arrayLength;

    jarray  array = inStream_readArrayRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    arrayLength = (*env)->GetArrayLength(env, array);
    outStream_writeInt(out, arrayLength);
    return JNI_TRUE;
}

static void
writeBooleanComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jboolean);
    jboolean *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetBooleanArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeBoolean(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeByteComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jbyte);
    jbyte *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetByteArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeByte(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeCharComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jchar);
    jchar *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetCharArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeChar(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeShortComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jshort);
    jshort *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetShortArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeShort(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeIntComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jint);
    jint *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetIntArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeInt(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeLongComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jlong);
    jlong *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetLongArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeLong(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeFloatComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jfloat);
    jfloat *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetFloatArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeFloat(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeDoubleComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    jint i;
    jint bufferSize = length * sizeof(jdouble);
    jdouble *components = jdwpAlloc(bufferSize);
    if (components == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        (*env)->GetDoubleArrayRegion(env, array, index, length, components);
        for (i = 0; i < length; i++) {
            outStream_writeDouble(out, components[i]);
        }
        jdwpFree(components);
    }
}

static void
writeObjectComponents(JNIEnv *env, PacketOutputStream *out, 
                    jarray array, jint index, jint length)
{
    int i;
    jobject component;

    WITH_LOCAL_REFS(env, length);

    for (i = 0; i < length; i++) {
        component = (*env)->GetObjectArrayElement(env, array, index + i);
        if ((*env)->ExceptionOccurred(env)) {
            /* cleared by caller */
            break;
        }
        outStream_writeByte(out, specificTypeKey(component));
        WRITE_LOCAL_REF(env, out, component);
    }

    END_WITH_LOCAL_REFS(env);
}

static jboolean 
getValues(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    char *signature;
    char *componentSignature;
    jclass arrayClass;
    jint arrayLength;
    jbyte typeKey;
    jarray array = inStream_readArrayRef(in);
    jint index = inStream_readInt(in);
    jint length = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    arrayLength = (*env)->GetArrayLength(env, array);

    if (length == -1) {
        length = arrayLength - index;
    }

    if ((index < 0) || (index > arrayLength - 1)) {
        outStream_setError(out, JDWP_Error_INVALID_INDEX);
        return JNI_TRUE;
    }

    if ((length < 0) || (length + index > arrayLength)) {
        outStream_setError(out, JDWP_Error_INVALID_LENGTH);
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, 1);

    arrayClass = (*env)->GetObjectClass(env, array);
    signature = classSignature(arrayClass);
    if (signature == NULL) {
        goto err;
    }
    componentSignature = &signature[1];
    typeKey = componentSignature[0];

    outStream_writeByte(out, typeKey);
    outStream_writeInt(out, length);

    if (isObjectTag(typeKey)) {
        writeObjectComponents(env, out, array, index, length);
    } else {
        switch (typeKey) {
            case JDWP_Tag_BYTE:
                writeByteComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_CHAR:
                writeCharComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_FLOAT:
                writeFloatComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_DOUBLE:
                writeDoubleComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_INT:
                writeIntComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_LONG:
                writeLongComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_SHORT:
                writeShortComponents(env, out, array, index, length);
                break;
    
            case JDWP_Tag_BOOLEAN:
                writeBooleanComponents(env, out, array, index, length);
                break;
    
            default:
                outStream_setError(out, JDWP_Error_INVALID_TAG);
                break;
        }
    }

    jdwpFree(signature);

err:
    END_WITH_LOCAL_REFS(env);

    if ((*env)->ExceptionOccurred(env)) {
        outStream_setError(out, JVMDI_ERROR_INTERNAL);
        (*env)->ExceptionClear(env);
    } 

    return JNI_TRUE;
}

static jint 
readBooleanComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jboolean component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readBoolean(in);
        (*env)->SetBooleanArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readByteComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jbyte component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readByte(in);
        (*env)->SetByteArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readCharComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jchar component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readChar(in);
        (*env)->SetCharArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readShortComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jshort component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readShort(in);
        (*env)->SetShortArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readIntComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jint component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readInt(in);
        (*env)->SetIntArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readLongComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jlong component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readLong(in);
        (*env)->SetLongArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readFloatComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jfloat component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readFloat(in);
        (*env)->SetFloatArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}

static jint 
readDoubleComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
{
    int i;
    jdouble component;

    for (i = 0; (i < length) && !inStream_error(in); i++) {
        component = inStream_readDouble(in);
        (*env)->SetDoubleArrayRegion(env, array, index + i, 1, &component);
    }
    return inStream_error(in);
}


static jint 
readObjectComponents(JNIEnv *env, PacketInputStream *in, 
                   jarray array, int index, int length)
                   /* char *componentSignature) */
{
    int i;
    jint error = JVMDI_ERROR_NONE;

    for (i = 0; i < length; i++) {
        jobject object = inStream_readObjectRef(in);

        (*env)->SetObjectArrayElement(env, array, index + i, object);
        if ((*env)->ExceptionOccurred(env)) {
            /* caller will clear */
            break;
        }
    }

    return error;
}


static jboolean 
setValues(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    char *signature;
    char *componentSignature;
    jclass arrayClass;
    jint error = JVMDI_ERROR_NONE;
    int arrayLength;

    jarray array = inStream_readArrayRef(in);
    jint index = inStream_readInt(in);
    jint length = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }
    
    arrayLength = (*env)->GetArrayLength(env, array);

    if ((index < 0) || (index > arrayLength - 1)) {
        outStream_setError(out, JDWP_Error_INVALID_INDEX);
        return JNI_TRUE;
    }

    if ((length < 0) || (length + index > arrayLength)) {
        outStream_setError(out, JDWP_Error_INVALID_LENGTH);
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, 1);

    arrayClass = (*env)->GetObjectClass(env, array);
    signature = classSignature(arrayClass);
    if (signature == NULL) {
        goto err;
    }
    componentSignature = &signature[1];

    switch (componentSignature[0]) {
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY:
            error = readObjectComponents(env, in, array, index, length);
                                       /* componentSignature); */
            break;

        case JDWP_Tag_BYTE:
            error = readByteComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_CHAR:
            error = readCharComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_FLOAT:
            error = readFloatComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_DOUBLE:
            error = readDoubleComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_INT:
            error = readIntComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_LONG:
            error = readLongComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_SHORT:
            error = readShortComponents(env, in, array, index, length);
            break;

        case JDWP_Tag_BOOLEAN:
            error = readBooleanComponents(env, in, array, index, length);
            break;

        default:
            {
                char buf[200];
                sprintf(buf, "Invalid array component signature: %s", componentSignature);
                ERROR_MESSAGE_EXIT(buf);
            }
            break;
    }

    jdwpFree(signature);

err:
    END_WITH_LOCAL_REFS(env);

    if ((*env)->ExceptionOccurred(env)) {
        /*
         * %comment gordonh004
         */
        error = JVMDI_ERROR_TYPE_MISMATCH;
        (*env)->ExceptionClear(env);
    }

    outStream_setError(out, error);
    return JNI_TRUE;
}


void *ArrayReference_Cmds[] = { (void *)0x3
    ,(void *)length
    ,(void *)getValues
    ,(void *)setValues};
