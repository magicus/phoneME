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
 *
 */


#include <JAbstractSurface.h>

#include <JNIUtil.h>
#include <PiscesSysutils.h>

#include <PiscesSurface.inl>

#define SURFACE_NATIVE_PTR 0
#define SURFACE_LAST SURFACE_NATIVE_PTR

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = JNI_FALSE;

static jboolean initializeSurfaceFieldIds(JNIEnv* env, jobject objectHandle);
static void disposeNativeImpl(JNIEnv* env, jobject objectHandle);

AbstractSurface*
surface_get(JNIEnv* env, jobject surfaceHandle) {
    return (AbstractSurface*)JLongToPointer(
               (*env)->GetLongField(env, surfaceHandle, 
                                    fieldIds[SURFACE_NATIVE_PTR]));
}

jboolean
surface_initialize(JNIEnv* env, jobject surfaceHandle) {
    return initializeSurfaceFieldIds(env, surfaceHandle);
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_AbstractSurface_nativeFinalize(JNIEnv* env,
        jobject objectHandle) {
    disposeNativeImpl(env, objectHandle);
}

JNIEXPORT jint JNICALL
Java_com_sun_pisces_AbstractSurface_getWidth(JNIEnv* env,
        jobject objectHandle) {
    Surface* surface;

    surface = (Surface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    return surface->width;
}

JNIEXPORT jint JNICALL
Java_com_sun_pisces_AbstractSurface_getHeight(JNIEnv* env,
        jobject objectHandle) {
    Surface* surface;

    surface = (Surface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    return surface->height;
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_AbstractSurface_getRGB(JNIEnv* env, jobject objectHandle,
        jintArray arrayHandle, jint offset, jint scanLength,
        jint x, jint y, jint width, jint height) {
    jint dstX = 0;
    jint dstY = 0;

    Surface* surface;

    surface = (Surface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    CORRECT_DIMS(surface, x, y, width, height, dstX, dstY);

    if ((width > 0) && (height > 0)) {
        jint* dstData = (jint*)(*env)->GetPrimitiveArrayCritical(env, 
                                                                 arrayHandle,
                                                                 NULL);
        if (dstData != NULL) {
            jint* src;
            jint* dst;
            jint srcScanRest = surface->width - width;
            jint dstScanRest = scanLength - width;

            ACQUIRE_SURFACE(surface, env, objectHandle);
            src = (jint*)surface->data + y * surface->width + x;
            dst = dstData + offset + dstY * scanLength + dstX;
            for (; height > 0; --height) {
                jint w2 = width;
                for (; w2 > 0; --w2) {
                    *dst++ = *src++;
                }
                src += srcScanRest;
                dst += dstScanRest;
            }
            RELEASE_SURFACE(surface, env, objectHandle);

            if (JNI_TRUE == readAndClearMemErrorFlag()) {
                JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                             "Allocation of internal renderer buffer failed.");
            }

            (*env)->ReleasePrimitiveArrayCritical(env, arrayHandle, dstData, 0);
        } else {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                      "Allocation of temporary renderer memory buffer failed.");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_AbstractSurface_setRGB(JNIEnv* env, jobject objectHandle,
        jintArray arrayHandle, jint offset, jint scanLength,
        jint x, jint y, jint width, jint height) {
    jint srcX = 0;
    jint srcY = 0;

    Surface* surface;
    surface = (Surface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    CORRECT_DIMS(surface, x, y, width, height, srcX, srcY);

    if ((width > 0) && (height > 0)) {
        jint* srcData = (jint*)(*env)->GetPrimitiveArrayCritical(env, 
                                                                 arrayHandle,
                                                                 NULL);
        if (srcData != NULL) {
            jint* src;

            ACQUIRE_SURFACE(surface, env, objectHandle);
            src = srcData + offset + srcY * scanLength + srcX;
            surface_setRGB(surface, x, y, width, height, src, scanLength);
            RELEASE_SURFACE(surface, env, objectHandle);

            (*env)->ReleasePrimitiveArrayCritical(env, arrayHandle, srcData, 0);

            if (JNI_TRUE == readAndClearMemErrorFlag()) {
                JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                             "Allocation of internal renderer buffer failed.");
            }
        } else {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                      "Allocation of temporary renderer memory buffer failed.");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_AbstractSurface_drawSurfaceImpl(JNIEnv* env,
        jobject objectHandle, jobject surfaceHandle, jint srcX, jint srcY,
        jint dstX, jint dstY, jint width, jint height, jfloat opacity) {
    Surface* dstSurface;
    Surface* srcSurface;

    dstSurface = (Surface*)JLongToPointer(
                     (*env)->GetLongField(env, objectHandle, 
                                          fieldIds[SURFACE_NATIVE_PTR]));
    srcSurface = (Surface*)JLongToPointer(
                     (*env)->GetLongField(env, surfaceHandle, 
                                          fieldIds[SURFACE_NATIVE_PTR]));

    CORRECT_DIMS(dstSurface, dstX, dstY, width, height, srcX, srcY);
    CORRECT_DIMS(srcSurface, srcX, srcY, width, height, dstX, dstY);

    if ((width > 0) && (height > 0) && (opacity > 0)) {
        ACQUIRE_SURFACE(dstSurface, env, objectHandle);
        ACQUIRE_SURFACE(srcSurface, env, surfaceHandle);
        surface_drawSurface(dstSurface, dstX, dstY, width, height,
                            srcSurface, srcX, srcY, opacity);
        RELEASE_SURFACE(srcSurface, env, surfaceHandle);
        RELEASE_SURFACE(dstSurface, env, objectHandle);

        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_AbstractSurface_drawRGBImpl(JNIEnv* env,
        jobject objectHandle, jintArray arrayHandle,
        jint offset, jint scanLength, jint x, jint y,
        jint width, jint height, jfloat opacity) {
    jint srcX = 0;
    jint srcY = 0;

    Surface* surface;
    surface = (Surface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    CORRECT_DIMS(surface, x, y, width, height, srcX, srcY);

    if ((width > 0) && (height > 0)) {
        jint* srcData = (jint*)(*env)->GetPrimitiveArrayCritical(env, 
                                                                 arrayHandle,
                                                                 NULL);
        if (srcData != NULL) {
            jint* src;

            ACQUIRE_SURFACE(surface, env, objectHandle);
            src = srcData + offset + srcY * scanLength + srcX;
            surface_drawRGB(surface, x, y, width, height, src, scanLength, 
                            opacity);
            RELEASE_SURFACE(surface, env, objectHandle);

            (*env)->ReleasePrimitiveArrayCritical(env, arrayHandle, srcData, 0);

            if (JNI_TRUE == readAndClearMemErrorFlag()) {
                JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                             "Allocation of internal renderer buffer failed.");
            }
        } else {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                      "Allocation of temporary renderer memory buffer failed.");
        }
    }
}

static jboolean
initializeSurfaceFieldIds(JNIEnv* env, jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },
                { NULL, NULL }
            };

    jboolean retVal;
    jclass classHandle;

    if (fieldIdsInitialized) {
        return JNI_TRUE;
    }

    retVal = JNI_FALSE;

    classHandle = (*env)->GetObjectClass(env, objectHandle);

    if (initializeFieldIds(fieldIds, env, classHandle, surfaceFieldDesc)) {
        retVal = JNI_TRUE;
        fieldIdsInitialized = JNI_TRUE;
    }

    return retVal;
}

static void
disposeNativeImpl(JNIEnv* env, jobject objectHandle) {
    AbstractSurface* surface;

    if (!fieldIdsInitialized) {
        return;
    }

    surface = (AbstractSurface*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle, 
                                       fieldIds[SURFACE_NATIVE_PTR]));

    if (surface != NULL) {
        surface->cleanup(surface);
        surface_dispose(&surface->super);
        (*env)->SetLongField(env, objectHandle, fieldIds[SURFACE_NATIVE_PTR],
                             (jlong)0);

        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    }
}
