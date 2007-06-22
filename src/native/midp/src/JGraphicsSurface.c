/*
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
#include<stdio.h>

#include <JAbstractSurface.h>

#include <KNIUtil.h>

#include <PiscesUtil.h>
#include <PiscesSysutils.h>
#include <commonKNIMacros.h>

#include <midpGraphics.h>

#define SURFACE_NATIVE_PTR 0
#define SURFACE_GRAPHICS 1
#define SURFACE_LAST SURFACE_GRAPHICS

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeSurfaceFieldIds(jobject objectHandle);

static void surface_acquire(AbstractSurface* surface, jobject surfaceHandle);
static void surface_release(AbstractSurface* surface, jobject surfaceHandle);
static void surface_cleanup(AbstractSurface* surface);

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_GraphicsSurface_initialize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    AbstractSurface* surface;

    KNI_GetThisPointer(objectHandle);

    if (surface_initialize(objectHandle)
            && initializeSurfaceFieldIds(objectHandle)) {
        surface = my_malloc(AbstractSurface, 1);
        
        if (surface != NULL) {
            surface->super.offset = 0;
            surface->super.pixelStride = 1;
            surface->super.imageType = TYPE_USHORT_565_RGB;

            surface->acquire = surface_acquire;
            surface->release = surface_release;
            surface->cleanup = surface_cleanup;

            KNI_SetLongField(objectHandle, fieldIds[SURFACE_NATIVE_PTR],
                             PointerToJLong(surface));
        } else {
            KNI_ThrowNew("java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    } else {
        KNI_ThrowNew("java/lang/IllegalStateException", "");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

static jboolean
initializeSurfaceFieldIds(jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },
                { "g", "Ljavax/microedition/lcdui/Graphics;" },
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

    if (initializeFieldIds(fieldIds, classHandle, surfaceFieldDesc)) {
        retVal = KNI_TRUE;
        fieldIdsInitialized = KNI_TRUE;
    }

    KNI_EndHandles();
    return retVal;
}

static void
surface_acquire(AbstractSurface* surface, jobject surfaceHandle) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);

    KNI_GetObjectField(surfaceHandle, fieldIds[SURFACE_GRAPHICS], 
                       graphicsHandle);

    if (!KNI_IsNullHandle(graphicsHandle)) {
        VDC vdc;
        VDC* pVDC;
        
        pVDC = setupVDC(graphicsHandle, &vdc);
        pVDC = getVDC(pVDC);
        
        surface->super.data = pVDC->hdc;
        surface->super.width = pVDC->width;
        surface->super.height = pVDC->height;
        surface->super.scanlineStride = pVDC->width;
    } else {
        /* 
         * This is not a correct error type to be reported here. For correct
         * reporting, Pisces error handling needs to be redesigned.
         */
        setMemErrorFlag();
    }

    KNI_EndHandles();
}

static void
surface_release(AbstractSurface* surface, jobject surfaceHandle) {
    // do nothing
    // IMPL NOTE : to fix warning : unused parameter
    (void)surface;
    (void)surfaceHandle;
}

static void
surface_cleanup(AbstractSurface* surface) {
    (void)surface;
}

