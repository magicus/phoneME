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
#include <JPiscesRenderer.h>

#include <PiscesLibrary.h>
#include <JNIUtil.h>
#include <JAbstractSurface.h>
#include <JTransform.h>

#include <PiscesSysutils.h>

#include <PiscesRenderer.inl>


#define RENDERER_NATIVE_PTR 0
#define RENDERER_SURFACE 1
#define RENDERER_LAST RENDERER_SURFACE

#define CMD_MOVE_TO 0
#define CMD_LINE_TO 1
#define CMD_QUAD_TO 2
#define CMD_CURVE_TO 3
#define CMD_CLOSE 4

#define SURFACE_FROM_RENDERER(surface, env, surfaceHandle, rendererHandle)     \
        (surfaceHandle) = (*(env))->GetObjectField((env), (rendererHandle),    \
                                                   fieldIds[RENDERER_SURFACE]  \
                                                   );                          \
        (surface) = &surface_get((env), (surfaceHandle))->super;

static jfieldID fieldIds[RENDERER_LAST + 1];
static jboolean fieldIdsInitialized = XNI_FALSE;

static jboolean initializeRendererFieldIds(JNIEnv* env, jobject objectHandle);
static void disposeNativeImpl(JNIEnv* env, jobject objectHandle);

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_staticInitialize(JNIEnv* env,
        jclass classHandle, jint xbias, jint ybias) {
    piscesutil_setStrokeBias(xbias, ybias);
    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    } else {
        if (!pisces_moduleInitialize()) {
            JNI_ThrowNew(env, "java/lang/IllegalStateException", "");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_initialize(JNIEnv* env, 
                                              jobject objectHandle) {
    Renderer* rdr;
    Surface* surface;
    jboolean sfieldsOK;

    sfieldsOK = initializeRendererFieldIds(env, objectHandle);
    if (sfieldsOK) {
        jobject surfaceHandle = (*env)->GetObjectField(env, objectHandle,
                                fieldIds[RENDERER_SURFACE]);
        surface = &surface_get(env, surfaceHandle)->super;

        rdr = renderer_create(surface);

        (*env)->SetLongField(env, objectHandle, fieldIds[RENDERER_NATIVE_PTR],
                             PointerToJLong(rdr));
        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed!!!");
        }

    } else {
        JNI_ThrowNew(env, "java/lang/IllegalStateException", "");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_nativeFinalize(JNIEnv* env,
        jobject objectHandle) {
    disposeNativeImpl(env, objectHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_beginRendering__IIIII(JNIEnv* env,
        jobject objectHandle, jint minX, jint minY, jint width, jint height,
        jint windingRule) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_beginRendering5(rdr, minX, minY, width, height, windingRule);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_beginRendering__I(JNIEnv* env,
        jobject objectHandle, jint windingRule) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_beginRendering1(rdr, windingRule);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_endRendering(JNIEnv* env,
        jobject objectHandle) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_endRendering(rdr);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setClip(JNIEnv* env, jobject objectHandle,
        jint minX, jint minY, jint width, jint height) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setClip(rdr, minX, minY, width, height);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_resetClip(JNIEnv* env,
        jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_resetClip(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setTransform(JNIEnv* env,
        jobject objectHandle, jobject transformHandle) {
    Renderer* rdr;
    Transform6 transform;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    transform_get6(&transform, env, transformHandle);
    renderer_setTransform(rdr, &transform);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_getTransformImpl(JNIEnv* env,
        jobject objectHandle, jobject transformHandle) {

    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    transform_set6(env, transformHandle, renderer_getTransform(rdr));

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setStroke__IIII_3II(JNIEnv* env,
        jobject objectHandle, jint lineWidth, jint capStyle,
        jint joinStyle, jint miterLimit, jintArray arrayHandle, 
        jint dashPhase) {
    jint* dashArray = NULL;
    jint dashArray_length = 0;

    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    if (arrayHandle != NULL) {
        jsize length = (*env)->GetArrayLength(env, arrayHandle);
        dashArray = (jint*)PISCESmalloc(length * sizeof(jint));

        //NOTE : dashArray is freed at finalization time by renderer_dispose()

        if (NULL == dashArray) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                       "Allocation of renderer memory for stroke data failed.");
        } else {
            (*env)->GetIntArrayRegion(env, arrayHandle, 0, length, dashArray);
            dashArray_length = length;
        }
    }

    renderer_setStroke6(rdr, lineWidth, capStyle, joinStyle, miterLimit,
                        dashArray, dashArray_length, dashPhase);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setStroke__(JNIEnv* env,
        jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setStroke0(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setFill(JNIEnv* env, jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setFill(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setColor(JNIEnv* env, jobject objectHandle,
        jint red, jint green, jint blue, jint alpha) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setColor(rdr, red, green, blue, alpha);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}
JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setCompositeRule(JNIEnv* env, 
                                                    jobject objectHandle,
                                                    jint compositeRule) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setCompositeRule(rdr, compositeRule);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}
JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setComposite(JNIEnv* env, 
                                                jobject objectHandle,
                                                jint compositeRule, 
                                                jfloat alpha) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setComposite(rdr, compositeRule, alpha);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}
JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setTextureImpl(JNIEnv* env,
        jobject objectHandle, jint imageType, jintArray arrayHandle,
        jint width, jint height, jint offset, jint stride,
        jobject transformHandle, jboolean repeat) {
    Renderer* rdr;

    jint* data;
    Transform6 textureTransform;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    data = (jint*)PISCESmalloc((width + 2) * (height + 2) * sizeof(jint));

    //NOTE : data is freed at finalization time by renderer_dispose()

    if (NULL == data) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of renderer memory for texture failed.");
    } else {
        jint size = width * sizeof(jint);
        jint dadd = width + 2;
        jint* dest = data + dadd;
        jint h2 = height;

        jint copyToFirstCol;
        jint copyToLastCol;
        jint copyToFirstRow;
        jint copyToLastRow;

        /* prepare additional pixels for interpolation */
        if (repeat) {
            copyToFirstCol = width - 1;
            copyToLastCol = 0;
            copyToFirstRow = height - 1;
            copyToLastRow = 0;
        } else {
            copyToFirstCol = 0;
            copyToLastCol = width - 1;
            copyToFirstRow = 0;
            copyToLastRow = height - 1;
        }

        for (; h2 > 0; --h2) {
            (*env)->GetIntArrayRegion(env, arrayHandle, offset, width, 
                                      dest + 1);
            dest[0] = dest[copyToFirstCol + 1];
            dest[width + 1] = dest[copyToLastCol + 1];
            dest += dadd;
            offset += stride;
        }

        memcpy(data, data + (copyToFirstRow + 1) * (width + 2),
               size + 2 * sizeof(jint));
        memcpy(dest, data + (copyToLastRow + 1) * (width + 2),
               size + 2 * sizeof(jint));

        transform_get6(&textureTransform, env, transformHandle);
        renderer_setTexture(rdr, data, width, height, repeat, 
                            &textureTransform);

        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setLinearGradientImpl(JNIEnv* env,
        jobject objectHandle, jint x0, jint y0, jint x1, jint y1,
        jintArray rampHandle, jint cycleMethod, jobject transformHandle) {
    Renderer* rdr;
    Transform6 gradientTransform;
    jint *ramp;

    ramp = (jint*)(*env)->GetPrimitiveArrayCritical(env, rampHandle, NULL);
    if (ramp != NULL) {
        transform_get6(&gradientTransform, env, transformHandle);

        rdr = (Renderer*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle,
                                       fieldIds[RENDERER_NATIVE_PTR]));
        rdr->_gradient_cycleMethod = cycleMethod;
        renderer_setLinearGradient(rdr, x0, y0, x1, y1,
                                   ramp, &gradientTransform);

        (*env)->ReleasePrimitiveArrayCritical(env, rampHandle, ramp, 0);

        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    } else {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of renderer memory for gradient failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setRadialGradientImpl(JNIEnv* env,
        jobject objectHandle, jint cx, jint cy, jint fx, jint fy, jint radius,
        jintArray rampHandle, jint cycleMethod, jobject transformHandle) {
    Renderer* rdr;
    Transform6 gradientTransform;
    jint *ramp;

    ramp = (jint*)(*env)->GetPrimitiveArrayCritical(env, rampHandle, NULL);
    if (ramp != NULL) {
        transform_get6(&gradientTransform, env, transformHandle);

        rdr = (Renderer*)JLongToPointer(
                  (*env)->GetLongField(env, objectHandle,
                                       fieldIds[RENDERER_NATIVE_PTR]));

        rdr->_gradient_cycleMethod = cycleMethod;
        renderer_setRadialGradient(rdr, cx, cy, fx, fy, radius,
                                   ramp, &gradientTransform);

        (*env)->ReleasePrimitiveArrayCritical(env, rampHandle, ramp, 0);

        if (JNI_TRUE == readAndClearMemErrorFlag()) {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    } else {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of renderer memory for gradient failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setAntialiasing(JNIEnv* env,
        jobject objectHandle, jboolean antialiasingOn) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setAntialiasing(rdr, antialiasingOn);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_sun_pisces_PiscesRenderer_getAntialiasing(JNIEnv* env,
        jobject objectHandle) {
    jboolean antialiasingOn;
    Renderer* rdr;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    antialiasingOn = renderer_getAntialiasing(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    return antialiasingOn;
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_moveTo(JNIEnv* env, jobject objectHandle,
        jint x0, jint y0) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_moveTo(rdr, x0, y0);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_lineJoin(JNIEnv* env, jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_lineJoin(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_lineTo(JNIEnv* env, jobject objectHandle,
        jint x1, jint y1) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_lineTo(rdr, x1, y1);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_quadTo(JNIEnv* env, jobject objectHandle,
        jint x1, jint y1, jint x2, jint y2) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_quadTo(rdr, x1, y1, x2, y2);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_cubicTo(JNIEnv* env, jobject objectHandle,
        jint x1, jint y1, jint x2, jint y2, jint x3, jint y3) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_cubicTo(rdr, x1, y1, x2, y2, x3, y3);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_close(JNIEnv* env, jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_close(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_end(JNIEnv* env, jobject objectHandle) {
    Renderer* rdr;
    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    renderer_end(rdr);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_clearRect(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint w, jint h) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_clearRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_drawLine(JNIEnv* env, jobject objectHandle,
        jint x0, jint y0, jint x1, jint y1) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_drawLine(rdr, x0, y0, x1, y1);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_drawRect(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint w, jint h) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_drawRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_fillRect(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint w, jint h) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));


    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_fillRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_drawOval(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint w, jint h) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_drawOval(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_fillOval(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint w, jint h) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_fillOval(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_drawRoundRect(JNIEnv* env,
        jobject objectHandle, jint x, jint y, jint w, jint h,
        jint aw, jint ah) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_drawRoundRect(rdr, x, y, w, h, aw, ah);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_fillRoundRect(JNIEnv* env,
        jobject objectHandle, jint x, jint y, jint w, jint h,
        jint aw, jint ah) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_fillRoundRect(rdr, x, y, w, h, aw, ah);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_drawArc(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint width, jint height, jint startAngle, jint arcAngle,
        jint arcType) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_drawArc(rdr, x, y, width, height, startAngle, arcAngle, arcType);
    RELEASE_SURFACE(surface, env, surfaceHandle);

    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}


JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_fillArc(JNIEnv* env, jobject objectHandle,
        jint x, jint y, jint width, jint height, jint startAngle, jint arcAngle,
        jint arcType) {
    Renderer* rdr;
    Surface* surface;
    jobject surfaceHandle;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));
    
    SURFACE_FROM_RENDERER(surface, env, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, env, surfaceHandle);
    INVALIDATE_RENDERER_SURFACE(rdr);
    renderer_fillArc(rdr, x, y, width, height, startAngle, arcAngle, arcType);
    RELEASE_SURFACE(surface, env, surfaceHandle);
    if (JNI_TRUE == readAndClearMemErrorFlag()) {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_getBoundingBox(JNIEnv* env,
        jobject objectHandle, jintArray bbox) {
    Renderer* rdr;
    jint bb[4];

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    bb[0] = rdr->_bboxX0;
    bb[1] = rdr->_bboxY0;
    bb[2] = rdr->_bboxX1 - rdr->_bboxX0;
    bb[3] = rdr->_bboxY1 - rdr->_bboxY0;

    (*env)->SetIntArrayRegion(env, bbox, 0, 4, bb);
}

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesRenderer_setPathData(JNIEnv* env,
        jobject objectHandle, jfloatArray dataHandle, jbyteArray commandsHandle,
        jint nCommands) {
    jint idx;
    Renderer* rdr;
    jfloat* data = NULL;
    jbyte* commands = NULL;

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    data = (jfloat*)(*env)->GetPrimitiveArrayCritical(env, dataHandle, NULL);
    commands = (jbyte*)(*env)->GetPrimitiveArrayCritical(env, commandsHandle,
               NULL);

    if ((data != NULL) && (commands != NULL)) {
        jint offset = 0;

        for (idx = 0; idx < nCommands; ++idx) {
            switch (commands[idx]) {
            case CMD_MOVE_TO:
                renderer_moveTo(rdr,
                                (jint)(data[offset] * 65536.0f),
                                (jint)(data[offset + 1] * 65536.0f));
                offset += 2;
                break;
            case CMD_LINE_TO:
                renderer_lineTo(rdr,
                                (jint)(data[offset] * 65536.0f),
                                (jint)(data[offset + 1] * 65536.0f));
                offset += 2;
                break;
            case CMD_QUAD_TO:
                renderer_quadTo(rdr,
                                (jint)(data[offset] * 65536.0f),
                                (jint)(data[offset + 1] * 65536.0f),
                                (jint)(data[offset + 2] * 65536.0f),
                                (jint)(data[offset + 3] * 65536.0f));
                offset += 4;
                break;
            case CMD_CURVE_TO:
                renderer_cubicTo(rdr,
                                 (jint)(data[offset] * 65536.0f),
                                 (jint)(data[offset + 1] * 65536.0f),
                                 (jint)(data[offset + 2] * 65536.0f),
                                 (jint)(data[offset + 3] * 65536.0f),
                                 (jint)(data[offset + 4] * 65536.0f),
                                 (jint)(data[offset + 5] * 65536.0f));
                offset += 6;
                break;
            case CMD_CLOSE:
            default:
                renderer_close(rdr);
                break;
            }
        }
    } else {
        JNI_ThrowNew(env, "java/lang/OutOfMemoryError", "");
    }

    if (data != NULL) {
        (*env)->ReleasePrimitiveArrayCritical(env, dataHandle, data, 0);
    }
    if (commands != NULL) {
        (*env)->ReleasePrimitiveArrayCritical(env, commandsHandle, commands, 0);
    }
}

Renderer*
renderer_get(JNIEnv* env, jobject objectHandle) {
    return (Renderer*)JLongToPointer(
                (*env)->GetLongField(env, objectHandle, 
                                     fieldIds[RENDERER_NATIVE_PTR]));
}

static jboolean
initializeRendererFieldIds(JNIEnv* env, jobject objectHandle) {
    static const FieldDesc rendererFieldDesc[] = {
                { "nativePtr", "J" },
                { "surface", "Lcom/sun/pisces/AbstractSurface;" },
                { NULL, NULL }
            };

    jboolean retVal;
    jclass classHandle;

    if (fieldIdsInitialized) {
        return JNI_TRUE;
    }

    retVal = JNI_FALSE;

    classHandle = (*env)->GetObjectClass(env, objectHandle);

    if (initializeFieldIds(fieldIds, env, classHandle, rendererFieldDesc)) {
        retVal = JNI_TRUE;
        fieldIdsInitialized = JNI_TRUE;
    }

    return retVal;
}

static void
disposeNativeImpl(JNIEnv* env, jobject objectHandle) {
    Renderer* rdr;

    if (!fieldIdsInitialized) {
        return;
    }

    rdr = (Renderer*)JLongToPointer(
              (*env)->GetLongField(env, objectHandle, 
                                   fieldIds[RENDERER_NATIVE_PTR]));

    if (rdr != NULL) {
        renderer_dispose(rdr);
        (*env)->SetLongField(env, objectHandle, fieldIds[RENDERER_NATIVE_PTR],
                             (jlong)0);
    }
}
