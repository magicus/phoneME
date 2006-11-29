/*
 * @(#)ArrayTypeImpl.c	1.22 06/10/10
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

#include "ArrayTypeImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"

/*
 * Determine the component class by looking thru all classes for
 * one that has the signature of the component and the same class loadeer
 * as the array.  See JVM spec 5.3.3:
 *     If the component type is a reference type, C is marked as having
 *     been defined by the defining class loader of the component type.
 */
static jint 
getComponentClass(JNIEnv *env, jclass arrayClass, char *componentSignature, 
                jclass *componentClassPtr) 
{
    jobject arrayClassLoader;
    jclass *classes;
    jint count;
    jclass componentClass = NULL;
    jint error = JVMDI_ERROR_NONE;

    arrayClassLoader = classLoader(arrayClass);
    classes = allLoadedClasses(&count);
    if (classes == NULL) {
        error = JVMDI_ERROR_OUT_OF_MEMORY;
    } else {
        int i;
        for (i = 0; (i < count) && (componentClass == NULL); i++) {
            char *signature;
            jclass clazz = classes[i];
            jboolean match;

            /* signature must match */
            signature = classSignature(clazz);
            if (signature == NULL) {
                error = JVMDI_ERROR_OUT_OF_MEMORY;
                break;
            }
            match = strcmp(signature, componentSignature) == 0;
            jdwpFree(signature);

            /* if signature matches, get class loader to check if
             * it matches 
             */
            if (match) {
                jobject loader = classLoader(clazz);
                match = (*env)->IsSameObject(env, loader, 
                                             arrayClassLoader);
                (*env)->DeleteGlobalRef(env, loader);
            }

            if (match) {
                componentClass = clazz;
            } else {
                (*env)->DeleteGlobalRef(env, clazz);
            }
        }
        /* delete the remaining global refs (classes) */
        for (; i < count; i++) {
            jclass clazz = classes[i];
            (*env)->DeleteGlobalRef(env, clazz);
        }
        jdwpFree(classes);

        *componentClassPtr = componentClass;
    }
    (*env)->DeleteGlobalRef(env, arrayClassLoader);

    if (error == JVMDI_ERROR_NONE && componentClass == NULL) {
        /* per JVM spec, component class is always loaded 
         * before array class, so this should never occur.
         */
        error = JVMDI_ERROR_NOT_FOUND;
    }

    return error;
}

static void
writeNewObjectArray(JNIEnv *env, PacketOutputStream *out, 
                 jclass arrayClass, jint size, char *componentSignature) {

    jarray array;
    jclass componentClass = NULL;

    jint error = getComponentClass(env, arrayClass, 
                                       componentSignature, &componentClass);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return;
    }

    WITH_LOCAL_REFS(env, 1);

    array = (*env)->NewObjectArray(env, size, componentClass, 0);
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        array = NULL;
    }

    if (array == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        outStream_writeByte(out, specificTypeKey(array));
        WRITE_LOCAL_REF(env, out, array);
    }

    (*env)->DeleteGlobalRef(env, componentClass);

    END_WITH_LOCAL_REFS(env);
}

static void
writeNewPrimitiveArray(JNIEnv *env, PacketOutputStream *out, 
                       jclass arrayClass, jint size, char *componentSignature) {

    jarray array = NULL;

    WITH_LOCAL_REFS(env, 1);

    switch (componentSignature[0]) {
        case JDWP_Tag_BYTE:
            array = (*env)->NewByteArray(env, size);
            break;

        case JDWP_Tag_CHAR:
            array = (*env)->NewCharArray(env, size);
            break;

        case JDWP_Tag_FLOAT:
            array = (*env)->NewFloatArray(env, size);
            break;

        case JDWP_Tag_DOUBLE:
            array = (*env)->NewDoubleArray(env, size);
            break;

        case JDWP_Tag_INT:
            array = (*env)->NewIntArray(env, size);
            break;

        case JDWP_Tag_LONG:
            array = (*env)->NewLongArray(env, size);
            break;

        case JDWP_Tag_SHORT:
            array = (*env)->NewShortArray(env, size);
            break;

        case JDWP_Tag_BOOLEAN:
            array = (*env)->NewBooleanArray(env, size);
            break;

        default:
            outStream_setError(out, JVMDI_ERROR_TYPE_MISMATCH);
            break;
    }

    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        array = NULL;
    }

    if (array == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        outStream_writeByte(out, specificTypeKey(array));
        WRITE_LOCAL_REF(env, out, array);
    }

    END_WITH_LOCAL_REFS(env);
}

static jboolean 
newInstance(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    char *arraySignature;
    char *componentSignature;
    jclass arrayClass = inStream_readClassRef(in);
    jint size = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    arraySignature = classSignature(arrayClass);
    componentSignature = &arraySignature[1];

    if ((componentSignature[0] == JDWP_Tag_OBJECT) || 
        (componentSignature[0] == JDWP_Tag_ARRAY)) {
        writeNewObjectArray(env, out, arrayClass, size, componentSignature);
    } else {
        writeNewPrimitiveArray(env, out, arrayClass, size, componentSignature);
    }

    jdwpFree(arraySignature);
    return JNI_TRUE;
}

void *ArrayType_Cmds[] = { (void *)0x1
                          ,(void *)newInstance};
