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
#include <stdlib.h>
#include <string.h>

#include "ReferenceTypeImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"


static jboolean 
signature(PacketInputStream *in, PacketOutputStream *out)
{
    char *signature;    
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    signature = classSignature(clazz);

    if (signature == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }

    outStream_writeString(out, signature);
    jdwpFree(signature);
    return JNI_TRUE;
}

static jboolean 
getClassLoader(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jclass clazz = inStream_readClassRef(in);
    jobject loader;
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    loader = classLoader(clazz);
    WRITE_GLOBAL_REF(env, out, loader);
    return JNI_TRUE;
}

static jboolean 
modifiers(PacketInputStream *in, PacketOutputStream *out)
{
    jint modifiers;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    modifiers = classModifiers(clazz);
    outStream_writeInt(out, modifiers);
    return JNI_TRUE;
}

static void 
writeMethodInfo(PacketOutputStream *out, jclass clazz, jmethodID methodID)
{
    char *name;
    char *signature;
    jint modifiers;
    jint error;
    jboolean isSynthetic = JNI_FALSE;
    
    /* 
     * Check for synthetic. If the query is not supported, we assume
     * it is not synthetic.
     */
    error = jvmdi->IsMethodSynthetic(clazz, methodID, &isSynthetic);
    if ((error != JVMDI_ERROR_NONE) && 
        (error != JVMDI_ERROR_NOT_IMPLEMENTED)) {
        outStream_setError(out, error);
        return;
    }

    if ((error = jvmdi->GetMethodModifiers(clazz, methodID, &modifiers)
         != JVMDI_ERROR_NONE) ||
        (error = jvmdi->GetMethodName(clazz, methodID, &name, &signature)
         != JVMDI_ERROR_NONE)) {
        outStream_setError(out, error);
        return;
    }

    if (isSynthetic) {
        modifiers |= MOD_SYNTHETIC;
    }
    outStream_writeMethodID(out, methodID);
    outStream_writeString(out, name);
    outStream_writeString(out, signature);
    outStream_writeInt(out, modifiers);
    jdwpFree(name);
    jdwpFree(signature);
}

static jboolean 
methods(PacketInputStream *in, PacketOutputStream *out)
{
    jint methodCount;
    jmethodID *methods;
    int i;

    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    if (isArrayClass(clazz)) {
        outStream_writeInt(out, 0);   /* no methods in an array */
        return JNI_TRUE;
    }

    if ((classStatus(clazz) & JVMDI_CLASS_STATUS_PREPARED) == 0) {
        outStream_setError(out, JVMDI_ERROR_CLASS_NOT_PREPARED);
        return JNI_TRUE;
    }

    methods = declaredMethods(clazz, &methodCount);
    if (methods == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }

    outStream_writeInt(out, methodCount);
    for (i = 0; (i < methodCount) && !outStream_error(out); i++) {
        writeMethodInfo(out, clazz, methods[i]);
    }

    /* Free methods array */
    jdwpFree(methods);
    return JNI_TRUE;
}

static void
writeFieldInfo(PacketOutputStream *out, jclass clazz, jfieldID fieldID)
{
    char *name;
    char *signature;
    jint modifiers;
    jboolean isSynthetic = JNI_FALSE;
    jint error;
    
    /* 
     * Check for synthetic. If the query is not supported, we assume
     * it is not synthetic.
     */
    error = jvmdi->IsFieldSynthetic(clazz, fieldID, &isSynthetic);
    if ((error != JVMDI_ERROR_NONE) && 
        (error != JVMDI_ERROR_NOT_IMPLEMENTED)) {
        outStream_setError(out, error);
        return;
    }

    if ((error = jvmdi->GetFieldModifiers(clazz, fieldID, &modifiers)
         != JVMDI_ERROR_NONE) ||
        (error = jvmdi->GetFieldName(clazz, fieldID, &name, &signature)
         != JVMDI_ERROR_NONE)) {
        outStream_setError(out, error);
        return;
    }

    if (isSynthetic) {
        modifiers |= MOD_SYNTHETIC;
    }
    outStream_writeFieldID(out, fieldID);
    outStream_writeString(out, name);
    outStream_writeString(out, signature);
    outStream_writeInt(out, modifiers);
    jdwpFree(name);
    jdwpFree(signature);
}

static jboolean 
fields(PacketInputStream *in, PacketOutputStream *out)
{
    jint fieldCount;
    jfieldID *fields;
    int i;

    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    if (isArrayClass(clazz)) {
        outStream_writeInt(out, 0);   /* no fields in an array */
        return JNI_TRUE;
    }

    if ((classStatus(clazz) & JVMDI_CLASS_STATUS_PREPARED) == 0) {
        outStream_setError(out, JVMDI_ERROR_CLASS_NOT_PREPARED);
        return JNI_TRUE;
    }

    fields = declaredFields(clazz, &fieldCount);
    if (fields == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }

    outStream_writeInt(out, fieldCount);
    for (i = 0; (i < fieldCount) && !outStream_error(out); i++) {
        writeFieldInfo(out, clazz, fields[i]);
    }

    /* Free fields array */
    jdwpFree(fields);
    return JNI_TRUE;
}

static jboolean 
getValues(PacketInputStream *in, PacketOutputStream *out)
{
    sharedGetFieldValues(in, out, JNI_TRUE);
    return JNI_TRUE;
}

static jboolean 
sourceFile(PacketInputStream *in, PacketOutputStream *out)
{
    char *fileName;    
    jint error;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = jvmdi->GetSourceFileName(clazz, &fileName);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeString(out, fileName);
    jdwpFree(fileName);
    return JNI_TRUE;
}

static jboolean 
sourceDebugExtension(PacketInputStream *in, PacketOutputStream *out)
{
    char *extension;    
    jint error;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = getSourceDebugExtension(clazz, &extension);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeString(out, extension);
    jdwpFree(extension);
    return JNI_TRUE;
}

static jboolean 
nestedTypes(PacketInputStream *in, PacketOutputStream *out)
{
    return JNI_TRUE;
}

static jboolean 
getClassStatus(PacketInputStream *in, PacketOutputStream *out)
{
    jint status;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    status = classStatus(clazz);
    outStream_writeInt(out, status);
    return JNI_TRUE;
}

static jboolean 
interfaces(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint interfaceCount;
    jint i;
    jclass *interfaces;
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    if ((classStatus(clazz) & JVMDI_CLASS_STATUS_PREPARED) == 0) {
        outStream_setError(out, JVMDI_ERROR_CLASS_NOT_PREPARED);
        return JNI_TRUE;
    }

    interfaces = implementedInterfaces(clazz, &interfaceCount);
    if (interfaces == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        outStream_writeInt(out, interfaceCount);
        for (i = 0; i < interfaceCount; i++) {
            WRITE_GLOBAL_REF(env, out, interfaces[i]);
        }
        jdwpFree(interfaces);
    }
    return JNI_TRUE;
}

static jboolean 
classObject(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jclass clazz = inStream_readClassRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * In our implementation, the reference type id is the same as the
     * class object id, so we bounce it right back.
     *
     * (Both inStream and WRITE_GLOBAL_REF) delete their global references
     * so we need to create a new one to write.)
     */
    clazz = (*env)->NewGlobalRef(env, clazz);
    if (clazz == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        WRITE_GLOBAL_REF(env, out, clazz);
    }
    return JNI_TRUE;
}

void *ReferenceType_Cmds[] = { (void *)12
    ,(void *)signature
    ,(void *)getClassLoader
    ,(void *)modifiers
    ,(void *)fields
    ,(void *)methods
    ,(void *)getValues
    ,(void *)sourceFile
    ,(void *)nestedTypes
    ,(void *)getClassStatus
    ,(void *)interfaces
    ,(void *)classObject
    ,(void *)sourceDebugExtension
};

