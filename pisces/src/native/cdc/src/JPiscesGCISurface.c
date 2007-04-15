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

#include <PiscesUtil.h>
#include <PiscesSysutils.h>

// See PiscesGCISurface.java on details.
#define TYPE_OF_ARRAY_NATIVE       0
#define TYPE_OF_ARRAY_JAVA_INT     1
#define TYPE_OF_ARRAY_JAVA_SHORT   2
#define TYPE_OF_ARRAY_JAVA_BYTE    3

// Indexes to fieldIds[]
#define SURFACE_NATIVE_PTR 0
#define SURFACE_NATIVE_ARRAY 1
#define SURFACE_DATA_INT 2
#define SURFACE_DATA_SHORT 3
#define SURFACE_DATA_BYTE 4
#define SURFACE_TYPE_OF_ARRAY 5
#define SURFACE_OFFSET 6
#define SURFACE_SCANLINE_STRIDE 7
#define SURFACE_PIXEL_STRIDE 8
#define SURFACE_LAST SURFACE_PIXEL_STRIDE

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = JNI_FALSE;

static jboolean initializeSurfaceFieldIds(JNIEnv* env, jobject objectHandle);

static void surface_acquire_static_native(AbstractSurface* surface, JNIEnv* env,
                                          jobject surfaceHandle);
static void surface_release_static_native(AbstractSurface* surface, JNIEnv* env,
                                          jobject surfaceHandle);
static void surface_acquire_static_java(AbstractSurface* surface, JNIEnv* env, 
                                        jobject surfaceHandle);
static void surface_release_static_java(AbstractSurface* surface, JNIEnv* env, 
                                        jobject surfaceHandle);
static void surface_acquire_dynamic(AbstractSurface* surface, JNIEnv* env,
                                    jobject surfaceHandle);
static void surface_release_dynamic(AbstractSurface* surface, JNIEnv* env,
                                    jobject surfaceHandle);
static void surface_cleanup(AbstractSurface* surface);

typedef struct _GCISurface {
    AbstractSurface super;
    jbyte arrayType;
    jfieldID javaArrayFieldID;
    jobject dataHandle;
} GCISurface;

JNIEXPORT void JNICALL
Java_com_sun_pisces_PiscesGCISurface_initialize(JNIEnv* env, 
                                                jobject objectHandle,
                                                jint imageType,
                                                jint width, jint height,
                                                jboolean isDynamic) {
    GCISurface* gciSurface;
    AbstractSurface* surface;
    if (surface_initialize(env, objectHandle) && 
            initializeSurfaceFieldIds(env, objectHandle)) {
        gciSurface = my_malloc(GCISurface, 1);
        if (gciSurface != NULL) {
            surface = &gciSurface->super;
            surface->super.width = width;
            surface->super.height = height;
            surface->super.imageType = imageType;

            surface->cleanup = surface_cleanup;

            if (!isDynamic) {
                jint arrayType;
            
                // get as much info from the surface as possible
                surface->super.offset =
                        (*env)->GetIntField(env, objectHandle, 
                                            fieldIds[SURFACE_OFFSET]);
                surface->super.scanlineStride =
                        (*env)->GetIntField(env, objectHandle, 
                                            fieldIds[SURFACE_SCANLINE_STRIDE]);
                surface->super.pixelStride =
                        (*env)->GetIntField(env, objectHandle, 
                                            fieldIds[SURFACE_PIXEL_STRIDE]);

                arrayType = 
                        (*env)->GetIntField(env, objectHandle, 
                                            fieldIds[SURFACE_TYPE_OF_ARRAY]);

                if (arrayType == TYPE_OF_ARRAY_NATIVE) {
                    surface->super.data = (void*)(*env)->GetLongField(
                            env, 
                            objectHandle,
                            fieldIds[SURFACE_NATIVE_ARRAY]);
                    surface->acquire = surface_acquire_static_native;
                    surface->release = surface_release_static_native;
                } else {
                    // table lookup?
                    switch (arrayType) {
                        case TYPE_OF_ARRAY_JAVA_INT:
                            gciSurface->javaArrayFieldID = 
                                    fieldIds[SURFACE_DATA_INT];
                            break;
                        case TYPE_OF_ARRAY_JAVA_SHORT:
                            gciSurface->javaArrayFieldID = 
                                    fieldIds[SURFACE_DATA_SHORT];
                            break;
                        case TYPE_OF_ARRAY_JAVA_BYTE:
                            gciSurface->javaArrayFieldID = 
                                    fieldIds[SURFACE_DATA_BYTE];
                            break;
                        default:
                            // should not happen
                            gciSurface->javaArrayFieldID = NULL;
                            break;
                    }
                    surface->acquire = surface_acquire_static_java;
                    surface->release = surface_release_static_java;
                }
            } else {
                // we need to get surface parameters in acquire / release
                surface->acquire = surface_acquire_dynamic;
                surface->release = surface_release_dynamic;
            }

            (*env)->SetLongField(env, objectHandle, 
                                 fieldIds[SURFACE_NATIVE_PTR],
                                 PointerToJLong(gciSurface));
        } else {
            JNI_ThrowNew(env, "java/lang/OutOfMemoryError",
                         "Allocation of internal renderer buffer failed.");
        }
    } else {
        JNI_ThrowNew(env, "java/lang/IllegalStateException", 
                     "java/lang/IllegalStateException");
    }
}

static jboolean
initializeSurfaceFieldIds(JNIEnv* env, jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },
                { "nativeArray", "J" },
                { "javaArrayInt", "[I" },
                { "javaArrayShort", "[S" },
                { "javaArrayByte", "[B" },
                { "typeOfArray", "I"},
                { "offset", "I" },
                { "scanlineStride", "I" },
                { "pixelStride", "I" },
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
surface_acquire_static_native(AbstractSurface* surface, JNIEnv* env, 
                              jobject surfaceHandle) {
    // do nothing
}

static void
surface_release_static_native(AbstractSurface* surface, JNIEnv* env, 
                              jobject surfaceHandle) {
    // do nothing
}

static void 
surface_acquire_static_java(AbstractSurface* surface, JNIEnv* env, 
                            jobject surfaceHandle) {
    GCISurface* gciSurface = (GCISurface*)surface;
    jobject dataHandle = (*env)->GetObjectField(env, surfaceHandle, 
                                                gciSurface->javaArrayFieldID);
    if (dataHandle != NULL) {
        void* data = (*env)->GetPrimitiveArrayCritical(env, dataHandle, NULL);
        if (data != NULL) {
            surface->super.data = data;
            gciSurface->dataHandle = dataHandle;
            return;
        }
    }
    
    // failed to get the pointer to java array
    setMemErrorFlag();
}

static void 
surface_release_static_java(AbstractSurface* surface, JNIEnv* env, 
                            jobject surfaceHandle) {
    GCISurface* gciSurface = (GCISurface*)surface;
    (*env)->ReleasePrimitiveArrayCritical(env, gciSurface->dataHandle, 
                                          surface->super.data, 0);
}

static void
surface_acquire_dynamic(AbstractSurface* surface, JNIEnv* env, 
                        jobject surfaceHandle) {
    GCISurface* gciSurface = (GCISurface*)surface;
    jint arrayType;
    
    surface->super.offset =
            (*env)->GetIntField(env, surfaceHandle, 
                                fieldIds[SURFACE_OFFSET]);
    surface->super.scanlineStride = 
            (*env)->GetIntField(env, surfaceHandle, 
                                fieldIds[SURFACE_SCANLINE_STRIDE]);
    surface->super.pixelStride =
            (*env)->GetIntField(env, surfaceHandle, 
                                fieldIds[SURFACE_PIXEL_STRIDE]);

    arrayType = (*env)->GetIntField(env, surfaceHandle, 
                                    fieldIds[SURFACE_TYPE_OF_ARRAY]);
    gciSurface->arrayType = arrayType;

    if (arrayType == TYPE_OF_ARRAY_NATIVE) {
        surface->super.data = 
                (void*)(*env)->GetLongField(env, 
                                            surfaceHandle,
                                            fieldIds[SURFACE_NATIVE_ARRAY]);
        if (surface->super.data == NULL) {
            // The name of the flag shouldn't be MemErrorFlag since the problem
            // is not with memory, it relates to not calling acquireSurface()
            // before rendering operations. More advanced flagging infrastructure 
            // should be introduced.
            setMemErrorFlag();
        }
        return;
    } else {
        jfieldID javaArrayFieldID;
        jobject dataHandle;

        // table lookup?
        switch (arrayType) {
            case TYPE_OF_ARRAY_JAVA_INT:
                javaArrayFieldID = fieldIds[SURFACE_DATA_INT];
                break;
            case TYPE_OF_ARRAY_JAVA_SHORT:
                javaArrayFieldID = fieldIds[SURFACE_DATA_SHORT];
                break;
            case TYPE_OF_ARRAY_JAVA_BYTE:
                javaArrayFieldID = fieldIds[SURFACE_DATA_BYTE];
                break;
            default:
                // should not happen
                javaArrayFieldID = NULL;
                break;
        }
        
        dataHandle = (*env)->GetObjectField(env, surfaceHandle, 
                                            javaArrayFieldID);
        if (dataHandle != NULL) {
            void* data = (*env)->GetPrimitiveArrayCritical(env, dataHandle, 
                                                           NULL);
            if (data != NULL) {
                surface->super.data = data;
                gciSurface->dataHandle = dataHandle;
                return;
            }
        }
    }

    // failed to get the pointer to java array
    setMemErrorFlag();
}

static void
surface_release_dynamic(AbstractSurface* surface, JNIEnv* env, 
                        jobject surfaceHandle) {
    GCISurface* gciSurface = (GCISurface*)surface;
    if (gciSurface->arrayType != TYPE_OF_ARRAY_NATIVE) {
        (*env)->ReleasePrimitiveArrayCritical(env, gciSurface->dataHandle, 
                                              surface->super.data, 0);
    }
}

static void
surface_cleanup(AbstractSurface* surface) {
    // do nothing
}
