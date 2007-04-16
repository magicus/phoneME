/*
 * 
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
 */


#include <JPiscesRenderer.h>

#include <PiscesLibrary.h>
#include <KNIUtil.h>
#include <JAbstractSurface.h>
#include <JTransform.h>

#include <PiscesSysutils.h>

#include <PiscesRenderer.inl>

#include <sni.h>
#include <commonKNIMacros.h>

#define RENDERER_NATIVE_PTR 0
#define RENDERER_SURFACE 1
#define RENDERER_LAST RENDERER_SURFACE

#define CMD_MOVE_TO 0
#define CMD_LINE_TO 1
#define CMD_QUAD_TO 2
#define CMD_CURVE_TO 3
#define CMD_CLOSE 4

// SURFACE_FROM_RENDERER is needed only when the java surface support is enabled
#ifdef PISCES_JAVA_SURFACE_SUPPORT

#define SURFACE_FROM_RENDERER(surface, surfaceHandle, rendererHandle)     \
        KNI_GetObjectField((rendererHandle), fieldIds[RENDERER_SURFACE],  \
                           (surfaceHandle));                              \
        (surface) = &surface_get((surfaceHandle))->super;

#else // PISCES_JAVA_SURFACE_SUPPORT

#define SURFACE_FROM_RENDERER(surface, surfaceHandle, rendererHandle) \
    (void)(surface); \
    (void)(surfaceHandle);

#endif // PISCES_JAVA_SURFACE_SUPPORT


static jfieldID fieldIds[RENDERER_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeRendererFieldIds(jobject objectHandle);

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_staticInitialize() {
    jint xbias = KNI_GetParameterAsInt(1);
    jint ybias = KNI_GetParameterAsInt(2);

    piscesutil_setStrokeBias(xbias, ybias);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    } else {
        if (!pisces_moduleInitialize()) {
            KNI_ThrowNew("java/lang/IllegalStateException", "");
        }
    }

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_initialize() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);

    if (initializeRendererFieldIds(objectHandle)) {
        KNI_GetObjectField(objectHandle, fieldIds[RENDERER_SURFACE], 
                           surfaceHandle);
        surface = &surface_get(surfaceHandle)->super;

        ACQUIRE_SURFACE(surface, surfaceHandle);
        rdr = renderer_create(surface);
        KNI_SetLongField(objectHandle, fieldIds[RENDERER_NATIVE_PTR],
                         PointerToJLong(rdr));
        RELEASE_SURFACE(surface, surfaceHandle);

        //    KNI_registerCleanup(objectHandle, disposeNativeImpl);

        if (KNI_TRUE == readAndClearMemErrorFlag()) {
            KNI_ThrowNew("java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }

    } else {
        KNI_ThrowNew("java/lang/IllegalStateException", "");
    }

    // don't do anything here (see the throw above)!

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_nativeFinalize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    KNI_GetThisPointer(objectHandle);

    renderer_finalize(objectHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_beginRendering__IIIII() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint minX = KNI_GetParameterAsInt(1);
    jint minY = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(3);
    jint height = KNI_GetParameterAsInt(4);
    jint windingRule = KNI_GetParameterAsInt(5);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_beginRendering5(rdr, minX, minY, width, height, windingRule);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_beginRendering__I() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint windingRule = KNI_GetParameterAsInt(1);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_beginRendering1(rdr, windingRule);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_endRendering() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_endRendering(rdr);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setClip() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint minX = KNI_GetParameterAsInt(1);
    jint minY = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(3);
    jint height = KNI_GetParameterAsInt(4);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setClip(rdr, minX, minY, width, height);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_resetClip() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_resetClip(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setTransform() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(transformHandle);

    Renderer* rdr;
    Transform6 transform;

    KNI_GetParameterAsObject(1, transformHandle);
    KNI_GetThisPointer(objectHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    transform_get6(&transform, transformHandle);
    renderer_setTransform(rdr, &transform);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_getTransformImpl() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(transformHandle);

    Renderer* rdr;

    KNI_GetParameterAsObject(1, transformHandle);
    KNI_GetThisPointer(objectHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    transform_set6(transformHandle, renderer_getTransform(rdr));

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setStroke__IIII_3II() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(arrayHandle);

    jint lineWidth = KNI_GetParameterAsInt(1);
    jint capStyle = KNI_GetParameterAsInt(2);
    jint joinStyle = KNI_GetParameterAsInt(3);
    jint miterLimit = KNI_GetParameterAsInt(4);
    jint* dashArray = NULL;
    jint dashArray_length = 0;
    jint dashPhase = KNI_GetParameterAsInt(6);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    KNI_GetParameterAsObject(5, arrayHandle);
    if (!KNI_IsNullHandle(arrayHandle)) {
        jsize length = KNI_GetArrayLength(arrayHandle);
        dashArray = (jint*)PISCESmalloc(length * sizeof(jint));

        //NOTE : dashArray is freed at finalization time by renderer_dispose()

        if (NULL == dashArray) {
            KNI_ThrowNew("java/lang/OutOfMemoryError",
                       "Allocation of renderer memory for stroke data failed.");
        } else {
            KNI_GetRawArrayRegion(arrayHandle, 0, length * sizeof(jint),
                                  (jbyte*)dashArray);
            dashArray_length = length;
        }
    }

    renderer_setStroke6(rdr, lineWidth, capStyle, joinStyle, miterLimit,
                        dashArray, dashArray_length, dashPhase);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setStroke__() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setStroke0(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setFill() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setFill(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setColor() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint red = KNI_GetParameterAsInt(1);
    jint green = KNI_GetParameterAsInt(2);
    jint blue = KNI_GetParameterAsInt(3);
    jint alpha = KNI_GetParameterAsInt(4);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setColor(rdr, red, green, blue, alpha);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setCompositeRule() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint compositeRule = KNI_GetParameterAsInt(1);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setCompositeRule(rdr, compositeRule);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setComposite() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint compositeRule = KNI_GetParameterAsInt(1);
    jfloat alpha = KNI_GetParameterAsFloat(2);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setComposite(rdr, compositeRule, alpha);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setTextureImpl() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(arrayHandle);
    KNI_DeclareHandle(transformHandle);

    //jint imageType = KNI_GetParameterAsInt(1);
    jint width = KNI_GetParameterAsInt(3);
    jint height = KNI_GetParameterAsInt(4);
    jint offset = KNI_GetParameterAsInt(5);
    jint stride = KNI_GetParameterAsInt(6);
    jboolean repeat = KNI_GetParameterAsBoolean(8);

    Renderer* rdr;

    jint* data;
    Transform6 textureTransform;

    KNI_GetParameterAsObject(2, arrayHandle);
    KNI_GetParameterAsObject(7, transformHandle);
    KNI_GetThisPointer(objectHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    data = (jint*)PISCESmalloc((width + 2) * (height + 2) * sizeof(jint));

    //NOTE : data is freed at finalization time by renderer_dispose()

    if (NULL == data) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of renderer memory for texture failed.");
    } else {
        jint sidx = offset * sizeof(jint);
        jint sadd = stride * sizeof(jint);
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
            KNI_GetRawArrayRegion(arrayHandle, sidx, size, (jbyte*)(dest + 1));
            dest[0] = dest[copyToFirstCol + 1];
            dest[width + 1] = dest[copyToLastCol + 1];
            dest += dadd;
            sidx += sadd;
        }

        memcpy(data, data + (copyToFirstRow + 1) * (width + 2),
               size + 2 * sizeof(jint));
        memcpy(dest, data + (copyToLastRow + 1) * (width + 2),
               size + 2 * sizeof(jint));

        transform_get6(&textureTransform, transformHandle);
        renderer_setTexture(rdr, data, width, height, repeat, 
                            &textureTransform);

        if (KNI_TRUE == readAndClearMemErrorFlag()) {
            KNI_ThrowNew("java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setLinearGradientImpl() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(rampHandle);
    KNI_DeclareHandle(transformHandle);

    jint x0 = KNI_GetParameterAsInt(1);
    jint y0 = KNI_GetParameterAsInt(2);
    jint x1 = KNI_GetParameterAsInt(3);
    jint y1 = KNI_GetParameterAsInt(4);
    jint cycleMethod = KNI_GetParameterAsInt(6);

    Renderer* rdr;
    Transform6 gradientTransform;

    jint *ramp;

    KNI_GetParameterAsObject(5, rampHandle);
    KNI_GetParameterAsObject(7, transformHandle);
    KNI_GetThisPointer(objectHandle);

    transform_get6(&gradientTransform, transformHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SNI_BEGIN_RAW_POINTERS;

    ramp = JavaIntArray(rampHandle);

    rdr->_gradient_cycleMethod = cycleMethod;
    renderer_setLinearGradient(rdr, x0, y0, x1, y1,
                               ramp, &gradientTransform);

    SNI_END_RAW_POINTERS;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setRadialGradientImpl() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(rampHandle);
    KNI_DeclareHandle(transformHandle);

    jint cx = KNI_GetParameterAsInt(1);
    jint cy = KNI_GetParameterAsInt(2);
    jint fx = KNI_GetParameterAsInt(3);
    jint fy = KNI_GetParameterAsInt(4);
    jint radius = KNI_GetParameterAsInt(5);
    jint cycleMethod = KNI_GetParameterAsInt(7);

    Renderer* rdr;
    Transform6 gradientTransform;

    jint *ramp;

    KNI_GetParameterAsObject(6, rampHandle);
    KNI_GetParameterAsObject(8, transformHandle);
    KNI_GetThisPointer(objectHandle);

    transform_get6(&gradientTransform, transformHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SNI_BEGIN_RAW_POINTERS;

    ramp = JavaIntArray(rampHandle);

    rdr->_gradient_cycleMethod = cycleMethod;
    renderer_setRadialGradient(rdr, cx, cy, fx, fy, radius,
                               ramp, &gradientTransform);

    SNI_END_RAW_POINTERS;
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setAntialiasing() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jboolean antialiasingOn = KNI_GetParameterAsBoolean(1);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_setAntialiasing(rdr, antialiasingOn);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_pisces_PiscesRenderer_getAntialiasing() {
    jboolean antialiasingOn;

    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    antialiasingOn = renderer_getAntialiasing(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(antialiasingOn);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_moveTo() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint x0 = KNI_GetParameterAsInt(1);
    jint y0 = KNI_GetParameterAsInt(2);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_moveTo(rdr, x0, y0);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_lineJoin() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_lineJoin(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_lineTo() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint x1 = KNI_GetParameterAsInt(1);
    jint y1 = KNI_GetParameterAsInt(2);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_lineTo(rdr, x1, y1);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_quadTo() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint x1 = KNI_GetParameterAsInt(1);
    jint y1 = KNI_GetParameterAsInt(2);
    jint x2 = KNI_GetParameterAsInt(3);
    jint y2 = KNI_GetParameterAsInt(4);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_quadTo(rdr, x1, y1, x2, y2);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_cubicTo() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint x1 = KNI_GetParameterAsInt(1);
    jint y1 = KNI_GetParameterAsInt(2);
    jint x2 = KNI_GetParameterAsInt(3);
    jint y2 = KNI_GetParameterAsInt(4);
    jint x3 = KNI_GetParameterAsInt(5);
    jint y3 = KNI_GetParameterAsInt(6);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_cubicTo(rdr, x1, y1, x2, y2, x3, y3);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_close() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_close(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_end() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    Renderer* rdr;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    renderer_end(rdr);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_clearRect() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_clearRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_drawLine() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x0 = KNI_GetParameterAsInt(1);
    jint y0 = KNI_GetParameterAsInt(2);
    jint x1 = KNI_GetParameterAsInt(3);
    jint y1 = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_drawLine(rdr, x0, y0, x1, y1);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_drawRect() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_drawRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_fillRect() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_fillRect(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_drawOval() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_drawOval(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_fillOval() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_fillOval(rdr, x, y, w, h);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_drawRoundRect() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);
    jint aw = KNI_GetParameterAsInt(5);
    jint ah = KNI_GetParameterAsInt(6);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_drawRoundRect(rdr, x, y, w, h, aw, ah);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_fillRoundRect() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint w = KNI_GetParameterAsInt(3);
    jint h = KNI_GetParameterAsInt(4);
    jint aw = KNI_GetParameterAsInt(5);
    jint ah = KNI_GetParameterAsInt(6);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_fillRoundRect(rdr, x, y, w, h, aw, ah);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_drawArc() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(3);
    jint height = KNI_GetParameterAsInt(4);
    jint startAngle = KNI_GetParameterAsInt(5);
    jint arcAngle = KNI_GetParameterAsInt(6);
    jint arcType = KNI_GetParameterAsInt(7);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_drawArc(rdr, x, y, width, height, startAngle, arcAngle, arcType);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_fillArc() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(surfaceHandle);

    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(3);
    jint height = KNI_GetParameterAsInt(4);
    jint startAngle = KNI_GetParameterAsInt(5);
    jint arcAngle = KNI_GetParameterAsInt(6);
    jint arcType = KNI_GetParameterAsInt(7);

    Renderer* rdr;
    Surface* surface;

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    SURFACE_FROM_RENDERER(surface, surfaceHandle, objectHandle);
    ACQUIRE_SURFACE(surface, surfaceHandle);
    renderer_fillArc(rdr, x, y, width, height, startAngle, arcAngle, arcType);
    RELEASE_SURFACE(surface, surfaceHandle);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_getBoundingBox() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(bbox);

    Renderer* rdr;

    KNI_GetParameterAsObject(1, bbox);
    KNI_GetThisPointer(objectHandle);

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    KNI_SetIntArrayElement(bbox, 0, rdr->_bboxX0);
    KNI_SetIntArrayElement(bbox, 1, rdr->_bboxY0);
    KNI_SetIntArrayElement(bbox, 2, rdr->_bboxX1 - rdr->_bboxX0);
    KNI_SetIntArrayElement(bbox, 3, rdr->_bboxY1 - rdr->_bboxY0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesRenderer_setPathData() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(dataHandle);
    KNI_DeclareHandle(commandsHandle);

    jint nCommands = KNI_GetParameterAsInt(3);

    jint idx;
    Renderer* rdr;
    jfloat* data;
    jbyte* commands;
    jint offset = 0;

    KNI_GetParameterAsObject(1, dataHandle);
    KNI_GetParameterAsObject(2, commandsHandle);

    KNI_GetThisPointer(objectHandle);
    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                                     fieldIds[RENDERER_NATIVE_PTR]));

    SNI_BEGIN_RAW_POINTERS;
    data = (jfloat *) JavaIntArray(dataHandle);
    commands = JavaByteArray(commandsHandle);
    
    
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

    SNI_END_RAW_POINTERS;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

Renderer*
renderer_get(jobject objectHandle) {
    return (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                     fieldIds[RENDERER_NATIVE_PTR]));
}

void
renderer_finalize(jobject objectHandle) {
    Renderer* rdr;

    if (!fieldIdsInitialized) {
        return;
    }

    rdr = (Renderer*)JLongToPointer(KNI_GetLongField(objectHandle,
                                    fieldIds[RENDERER_NATIVE_PTR]));

    if (rdr != (Renderer*)0) {
        renderer_dispose(rdr);
        KNI_SetLongField(objectHandle, fieldIds[RENDERER_NATIVE_PTR],
                         (jlong)0);
    }
}

static jboolean
initializeRendererFieldIds(jobject objectHandle) {
    static const FieldDesc rendererFieldDesc[] = {
                { "nativePtr", "J" },
                { "surface", "Lcom/sun/pisces/AbstractSurface;" },
                { NULL, NULL }
            };

    jboolean retVal;

    if (fieldIdsInitialized) {
        return KNI_TRUE;
    }

    retVal = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(classHandle);

    KNI_GetObjectClass(objectHandle, classHandle);

    if (initializeFieldIds(fieldIds, classHandle, rendererFieldDesc)) {
        retVal = KNI_TRUE;
        fieldIdsInitialized = KNI_TRUE;
    }

    KNI_EndHandles();
    return retVal;
}
