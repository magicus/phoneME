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


#include <JAbstractSurface.h>

#include <KNIUtil.h>

#include <PiscesUtil.h>
#include <PiscesSysutils.h>

#include <commonKNIMacros.h>

#define SURFACE_NATIVE_PTR 0
#define SURFACE_DATA_INT 1
#define SURFACE_DATA_SHORT 2
#define SURFACE_DATA_BYTE 3 
#define SURFACE_LAST SURFACE_DATA_BYTE

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeSurfaceFieldIds(jobject objectHandle);

static void surface_acquire(AbstractSurface* surface, jobject surfaceHandle);
static void surface_release(AbstractSurface* surface, jobject surfaceHandle);
static void surface_cleanup(AbstractSurface* surface);

typedef struct _JavaSurface {
    AbstractSurface super;
    jfieldID javaArrayFieldID;
    jint javaArrayFieldSize;
} JavaSurface;

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_JavaSurface_initialize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    jint dataType = KNI_GetParameterAsInt(1);
    jint width = KNI_GetParameterAsInt(2);
    jint height = KNI_GetParameterAsInt(3);

    JavaSurface* jSurface;
    AbstractSurface* surface;

    KNI_GetThisPointer(objectHandle);

    if (surface_initialize(objectHandle)
            && initializeSurfaceFieldIds(objectHandle)) {
        jSurface = my_malloc(JavaSurface, 1);
        surface = &jSurface->super;
        if (surface != NULL) {
            surface->super.width = width;
            surface->super.height = height;
            surface->super.offset = 0;
            surface->super.scanlineStride = width;
            surface->super.pixelStride = 1;
            surface->super.imageType = dataType;

            surface->acquire = surface_acquire;
            surface->release = surface_release;
            surface->cleanup = surface_cleanup;

            switch(surface->super.imageType){
                case TYPE_INT_RGB:
                case TYPE_INT_ARGB:
                case TYPE_INT_ARGB_PRE:
                    jSurface->javaArrayFieldID = fieldIds[SURFACE_DATA_INT];
                    jSurface->javaArrayFieldSize = sizeof(jint);            
                    break;
                case TYPE_USHORT_565_RGB:
                    jSurface->javaArrayFieldID = fieldIds[SURFACE_DATA_SHORT];
                    jSurface->javaArrayFieldSize = sizeof(jshort);
                    break;
                case TYPE_BYTE_GRAY:
                    jSurface->javaArrayFieldID = fieldIds[SURFACE_DATA_BYTE];
                    jSurface->javaArrayFieldSize = sizeof(jbyte);
                    break;
                default: //errorneous - should never happen
                    jSurface->javaArrayFieldID = NULL;
            }

            KNI_SetLongField(objectHandle, fieldIds[SURFACE_NATIVE_PTR],
                             PointerToJLong(surface));
            //    KNI_registerCleanup(objectHandle, disposeNativeImpl);
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
                { "dataInt", "[I" },
                { "dataShort", "[S" },
                { "dataByte", "[B" },
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
    KNI_DeclareHandle(dataHandle);

    KNI_GetObjectField(surfaceHandle, ((JavaSurface *) surface)->javaArrayFieldID, dataHandle);
    switch(((JavaSurface*)surface)->javaArrayFieldSize) {
        case sizeof(jint):
            surface->super.data = JavaIntArray(dataHandle);
            break;
        case sizeof(jshort):
            surface->super.data = JavaShortArray(dataHandle);
            break;
        case sizeof(jbyte):
            surface->super.data = JavaByteArray(dataHandle);
            break;
        default:
            //shouldn't happen        
            break;
    }
    KNI_EndHandles();
}

static void
surface_release(AbstractSurface* surface, jobject surfaceHandle) {
    // do nothing
}

static void
surface_cleanup(AbstractSurface* surface) {
    // do nothing
}
