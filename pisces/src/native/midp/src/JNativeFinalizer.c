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


#include <KNIUtil.h>

#include <JPiscesRenderer.h>
#include <JAbstractSurface.h>

#define FINALIZER_GUARDED_OBJ 0
#define FINALIZER_LAST FINALIZER_GUARDED_OBJ

static jfieldID fieldIds[FINALIZER_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeFinalizerFieldIds(jobject objectHandle);

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_NativeFinalizer_initialize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    KNI_GetThisPointer(objectHandle);

    if (!initializeFinalizerFieldIds(objectHandle)) {
        KNI_ThrowNew("java/lang/IllegalStateException", "");
    }

    // don't do anything here (see the throw above)!

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_NativeFinalizer_0004SurfaceNativeFinalizer_finalize() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(guardedHandle);

    KNI_GetThisPointer(objectHandle);
    KNI_GetObjectField(objectHandle, fieldIds[FINALIZER_GUARDED_OBJ],
                       guardedHandle);

    surface_finalize(guardedHandle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_NativeFinalizer_0004RendererNativeFinalizer_finalize() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(guardedHandle);

    KNI_GetThisPointer(objectHandle);
    KNI_GetObjectField(objectHandle, fieldIds[FINALIZER_GUARDED_OBJ],
                       guardedHandle);

    renderer_finalize(guardedHandle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_NativeFinalizer_00024SurfaceNativeFinalizer_finalize() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(guardedHandle);

    KNI_GetThisPointer(objectHandle);
    KNI_GetObjectField(objectHandle, fieldIds[FINALIZER_GUARDED_OBJ],
                       guardedHandle);

    surface_finalize(guardedHandle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_NativeFinalizer_00024RendererNativeFinalizer_finalize() {
    KNI_StartHandles(2);
    KNI_DeclareHandle(objectHandle);
    KNI_DeclareHandle(guardedHandle);

    KNI_GetThisPointer(objectHandle);
    KNI_GetObjectField(objectHandle, fieldIds[FINALIZER_GUARDED_OBJ],
                       guardedHandle);

    renderer_finalize(guardedHandle);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

static jboolean
initializeFinalizerFieldIds(jobject objectHandle) {
    static const FieldDesc finalizerFieldDesc[] = {
                { "guardedObject", "Lcom/sun/pisces/NativeFinalization;"
                },
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

    if (initializeFieldIds(fieldIds, classHandle, finalizerFieldDesc)) {
        retVal = KNI_TRUE;
        fieldIdsInitialized = KNI_TRUE;
    }

    KNI_EndHandles();
    return retVal;
}
