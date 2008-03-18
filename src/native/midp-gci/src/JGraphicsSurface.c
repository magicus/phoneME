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

#include <jni.h>
#include <kni.h>

// *****************************************************************************
#ifdef SVG_DEBUG

#include <stdio.h>
#define debug_print_int_hex(prefix,var) do { printf("%s %s=0x%x\n", prefix, #var, (int)(var)); fflush(stdout); } while(0)
#define debug_print_int(prefix,var) do { printf("%s %s=%d\n", prefix, #var, (int)(var)); fflush(stdout); } while(0)

#else

#define debug_print_int(prefix,var)
#define debug_print_int_hex(prefix,var)

#endif
// *****************************************************************************


// struct _AbstractSurface
// 		void (*acquire)(struct _AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle);
// 		void (*release)(struct _AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle);
// 		void (*cleanup)(struct _AbstractSurface* surface);
// jboolean surface_initialize(JNIEnv* env, jobject surfaceHandle);
#include <JAbstractSurface.h>

// [macro]	my_malloc
#include <PiscesUtil.h>

// setMemErrorFlag
#include <PiscesSysutils.h>

// jboolean initializeFieldIds(jfieldID* dest, JNIEnv* env, jclass classHandle, const FieldDesc* fields);
#include <JNIUtil.h>


// Data
// =============================================================================

#define SURFACE_NATIVE_PTR 0
#define SURFACE_GRAPHICS 1
#define SURFACE_LAST SURFACE_GRAPHICS

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;


// Forward declarations
// =============================================================================

static jboolean initializeSurfaceFieldIds(JNIEnv *env, jobject objectHandle);
static void surface_acquire(AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle);
static void surface_release(AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle);
static void surface_cleanup(AbstractSurface* surface);



// Public functions
// =============================================================================

/**
 * private native void initialize();
 * @param instance  this (instance of class com.sun.pisces.GraphicsSurface)
 */
JNIEXPORT void JNICALL
Java_com_sun_pisces_GraphicsSurface_initialize(
	JNIEnv *env,
    jobject instance) {

    AbstractSurface* surface;


    if (surface_initialize(env, instance) && initializeSurfaceFieldIds(env, instance)) {
        surface = my_malloc(AbstractSurface, 1);


        debug_print_int_hex("@ initialize:", surface);

        if (surface != NULL) {
            surface->super.data = 0;					// pisces should not use it yet
            surface->super.offset = 0;					// pisces should not use it yet
            surface->super.pixelStride = 0;				// pisces should not use it yet
            surface->super.scanlineStride = 0;			// pisces should not use it yet
            surface->super.imageType = TYPE_INT_RGB;	// pisces should not use it yet
            surface->super.alphaData = NULL;			// pisces should not use it yet

            surface->acquire = surface_acquire;
            surface->release = surface_release;
            surface->cleanup = surface_cleanup;

            (*env)->SetLongField(env, instance, fieldIds[SURFACE_NATIVE_PTR], (jlong)surface);
        } else {
            jclass jcls_oome = (*env)->FindClass (env, "java/lang/OutOfMemoryError");
            (*env)->ThrowNew(env, jcls_oome, "Allocation of internal renderer buffer failed.");
        }
    } else {
        jclass jcls_ise = (*env)->FindClass (env, "java/lang/IllegalStateException");
        (*env)->ThrowNew(env, jcls_ise, "");
    }
}



// Helper functions
// =============================================================================


static jboolean
initializeSurfaceFieldIds(JNIEnv *env, jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },           // in class com.sun.pisces.AbstractSurface
//                { "g", "Ljavax/microedition/lcdui/Graphics;" },
                { "g", "Ljava/lang/Object;" },  // in class com.sun.pisces.GraphicsSurface
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


#define FORMAT_INT_RGB_SHIFT    (22)
#define FORMAT_RED_BITS_SHIFT   (4)
#define FORMAT_GREEN_BITS_SHIFT (8)
#define FORMAT_BLUE_BITS_SHIFT  (12)
#define FORMAT_ALPHA_BITS_SHIFT (16)

#define FORMAT_XRGB_8888 (\
        (1 << FORMAT_INT_RGB_SHIFT) | \
        (8 << FORMAT_ALPHA_BITS_SHIFT) | \
        (8 << FORMAT_RED_BITS_SHIFT) | \
        (8 << FORMAT_GREEN_BITS_SHIFT) | \
        (8 << FORMAT_BLUE_BITS_SHIFT))

#define FORMAT_ARGB_8888 (\
        (8 << FORMAT_ALPHA_BITS_SHIFT) |\
        (8 << FORMAT_RED_BITS_SHIFT) |\
        (8 << FORMAT_GREEN_BITS_SHIFT) |\
        (8 << FORMAT_BLUE_BITS_SHIFT))



/**
 * Must(?) populate 'surface' with proper data, describing the GCI surface passed in 'surfaceHandle'
 * @param surfaceHandle
 */
static void
surface_acquire(AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle) {
    jobject graphicsHandle;
    jclass graphicsClassHandle;

	jfieldID fid_gciDrawingSurface;
	jobject jobj_gciDrawingSurface;
	jclass jcls_GCIDrawingSurface;

	jmethodID jmid_isNativeSurface;
	jboolean isNativeSurface;

	jmethodID jmid_getFormat;
	jint format;

	jmethodID jmid_getSurfaceInfo;
	jobject jobj_surfaceInfo;
	jclass jcls_GCISurfaceInfo;
	jmethodID jmid_getBasePointer;
	jmethodID jmid_getXBitStride;
	jmethodID jmid_getYBitStride;
	jlong basePointer;
	jint xBitStride;
	jint yBitStride;
    jmethodID jmid_getWidth;
    jmethodID jmid_getHeight;
    jint width;
    jint height;


    graphicsHandle = (*env)->GetObjectField(env, surfaceHandle, fieldIds[SURFACE_GRAPHICS]);
    debug_print_int_hex("@ surface_acquire", graphicsHandle);

    graphicsClassHandle = (*env)->GetObjectClass(env, graphicsHandle);
    debug_print_int_hex("@ surface_acquire", graphicsClassHandle);

    {
//        jmethodID mid_Graphics_toString = (*env)->GetMethodID(env, graphicsClassHandle, "toString", "()Ljava/lang/String;");
//        jstring str = (jstring) (*env)->CallObjectMethod(env, graphicsClassHandle, mid_Graphics_toString, NULL);
//        const char* graphicsClassName = (*env)->GetStringUTFChars(env, str, NULL);
//        debug_print_int_hex("@ surface_acquire", graphicsClassName=%s\n", graphicsClassName);
	}

	fid_gciDrawingSurface = (*env)->GetFieldID(env, graphicsClassHandle, "gciDrawingSurface", "Lcom/sun/me/gci/surface/GCIDrawingSurface;");
	debug_print_int_hex("@ surface_acquire", fid_gciDrawingSurface);


	jobj_gciDrawingSurface = (jobject) (*env)->GetObjectField (env, graphicsHandle, fid_gciDrawingSurface);
	debug_print_int_hex("@ surface_acquire", jobj_gciDrawingSurface);

	jcls_GCIDrawingSurface = (*env)->FindClass (env, "com/sun/me/gci/surface/GCIDrawingSurface");
	debug_print_int_hex("@ surface_acquire", jcls_GCIDrawingSurface);

	jmid_getSurfaceInfo = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getSurfaceInfo", "()Lcom/sun/me/gci/surface/GCISurfaceInfo;");
	debug_print_int_hex("@ surface_acquire", jmid_getSurfaceInfo);

	jobj_surfaceInfo = (*env)->CallObjectMethod (env, jobj_gciDrawingSurface, jmid_getSurfaceInfo);
	debug_print_int_hex("@ surface_acquire", jobj_surfaceInfo);

	jcls_GCISurfaceInfo = (*env)->FindClass (env, "com/sun/me/gci/surface/GCISurfaceInfo");
	debug_print_int_hex("@ surface_acquire", jcls_GCISurfaceInfo);

	jmid_isNativeSurface = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "isNativeSurface", "()Z");
	debug_print_int_hex("@ surface_acquire", jmid_isNativeSurface);

	isNativeSurface = (*env)->CallBooleanMethod (env, jobj_gciDrawingSurface, jmid_isNativeSurface);
	debug_print_int_hex("@ surface_acquire", isNativeSurface);

	// ***********
	jmid_getFormat = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getFormat", "()I");
	debug_print_int_hex("@ surface_acquire", jmid_getFormat);
	format = (*env)->CallIntMethod(env, jobj_gciDrawingSurface, jmid_getFormat);
	debug_print_int_hex("@ surface_acquire", format);
	// ***********

	// -------------------------------------------------------------------------------------------------
    if (isNativeSurface) {

		jmid_getBasePointer = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getBasePointer", "()J");
		debug_print_int_hex("@ surface_acquire", jmid_getBasePointer);

		basePointer = (*env)->CallLongMethod (env, jobj_surfaceInfo, jmid_getBasePointer);
		debug_print_int_hex("@ surface_acquire", basePointer);
	}
	else {
		jmethodID jmid_getPixelArrayType;
		jint pixelArrayType;
		jmethodID jmid_getPixelArray;
		jintArray pixelArray;


		jmid_getPixelArrayType = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArrayType", "()I");
		debug_print_int_hex("@ surface_acquire", jmid_getPixelArrayType);

		pixelArrayType = (*env)->CallIntMethod(env, jobj_surfaceInfo, jmid_getPixelArrayType);
		debug_print_int_hex("@ surface_acquire", pixelArrayType);

		jmid_getPixelArray = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArray", "()Ljava/lang/Object;");
		debug_print_int_hex("@ surface_acquire", jmid_getPixelArray);

		pixelArray = (jintArray) ((*env)->CallObjectMethod(env, jobj_surfaceInfo, jmid_getPixelArray));
		debug_print_int_hex("@ surface_acquire", pixelArray);

		basePointer = (*env)->GetIntArrayElements  (env, pixelArray, 0);
		debug_print_int_hex("@ surface_acquire", basePointer);
	}


	jmid_getXBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getXBitStride", "()I");
	debug_print_int_hex("@ surface_acquire", jmid_getXBitStride);

	xBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getXBitStride);
	debug_print_int_hex("@ surface_acquire", xBitStride);


	jmid_getYBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getYBitStride", "()I");
	debug_print_int_hex("@ surface_acquire", jmid_getYBitStride);

	yBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getYBitStride);
	debug_print_int_hex("@ surface_acquire", yBitStride);


	jmid_getWidth = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getWidth", "()I");
	debug_print_int_hex("@ surface_acquire", jmid_getWidth);

	width = (*env)->CallIntMethod (env, jobj_gciDrawingSurface, jmid_getWidth);
	debug_print_int_hex("@ surface_acquire", width);


	jmid_getHeight = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getHeight", "()I");
	debug_print_int_hex("@ surface_acquire", jmid_getHeight);

	height = (*env)->CallIntMethod (env, jobj_gciDrawingSurface, jmid_getHeight);
	debug_print_int_hex("@ surface_acquire", height);


    if (!(0 == graphicsHandle)) {
    	if (format == FORMAT_XRGB_8888 || format == FORMAT_ARGB_8888) {
			surface->super.data = (void*)basePointer;

			surface->super.width = width;
			surface->super.height = height;
			surface->super.pixelStride = xBitStride >> 3 >> 2;
			surface->super.scanlineStride = yBitStride >> 3 >> 2;

//			surface->super.imageType = TYPE_USHORT_565_RGB;
			if (format == FORMAT_XRGB_8888) {
				surface->super.imageType = TYPE_INT_RGB;
			}
			else if (format == FORMAT_ARGB_8888) {
			    surface->super.imageType = TYPE_INT_ARGB;
			}

			surface->super.alphaData = NULL;
		}
		else {
			// unsupported format
			setMemErrorFlag();
		}
    } else {
        /*
         * This is not a correct error type to be reported here. For correct
         * reporting, Pisces error handling needs to be redesigned.
         */
        setMemErrorFlag();
    }
}


static void
surface_release(AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle) {
	jobject graphicsHandle;
    jclass graphicsClassHandle;

	jfieldID fid_gciDrawingSurface;
	jobject jobj_gciDrawingSurface;
	jclass jcls_GCIDrawingSurface;

	jmethodID jmid_isNativeSurface;
	jboolean isNativeSurface;

	jmethodID jmid_getFormat;
	jint format;

	jmethodID jmid_getSurfaceInfo;
	jobject jobj_surfaceInfo;
	jclass jcls_GCISurfaceInfo;
	jmethodID jmid_getBasePointer;
	jmethodID jmid_getXBitStride;
	jmethodID jmid_getYBitStride;
	jlong basePointer;
	jint xBitStride;
	jint yBitStride;
    jmethodID jmid_getWidth;
    jmethodID jmid_getHeight;
    jint width;
    jint height;


    graphicsHandle = (*env)->GetObjectField(env, surfaceHandle, fieldIds[SURFACE_GRAPHICS]);
    debug_print_int_hex("@ surface_release", graphicsHandle);

    graphicsClassHandle = (*env)->GetObjectClass(env, graphicsHandle);
    debug_print_int_hex("@ surface_release", graphicsClassHandle);

    {
//        jmethodID mid_Graphics_toString = (*env)->GetMethodID(env, graphicsClassHandle, "toString", "()Ljava/lang/String;");
//        jstring str = (jstring) (*env)->CallObjectMethod(env, graphicsClassHandle, mid_Graphics_toString, NULL);
//        const char* graphicsClassName = (*env)->GetStringUTFChars(env, str, NULL);
//        debug_print_int_hex("@ surface_acquire", graphicsClassName=%s\n", graphicsClassName);
	}

	fid_gciDrawingSurface = (*env)->GetFieldID(env, graphicsClassHandle, "gciDrawingSurface", "Lcom/sun/me/gci/surface/GCIDrawingSurface;");
	debug_print_int_hex("@ surface_release", fid_gciDrawingSurface);

	jobj_gciDrawingSurface = (jobject) (*env)->GetObjectField (env, graphicsHandle, fid_gciDrawingSurface);
	debug_print_int_hex("@ surface_release", jobj_gciDrawingSurface);

	jcls_GCIDrawingSurface = (*env)->FindClass (env, "com/sun/me/gci/surface/GCIDrawingSurface");
	debug_print_int_hex("@ surface_release", jcls_GCIDrawingSurface);

	jmid_getSurfaceInfo = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getSurfaceInfo", "()Lcom/sun/me/gci/surface/GCISurfaceInfo;");
	debug_print_int_hex("@ surface_release", jmid_getSurfaceInfo);

	jobj_surfaceInfo = (*env)->CallObjectMethod (env, jobj_gciDrawingSurface, jmid_getSurfaceInfo);
	debug_print_int_hex("@ surface_release", jobj_surfaceInfo);

	jcls_GCISurfaceInfo = (*env)->FindClass (env, "com/sun/me/gci/surface/GCISurfaceInfo");
	debug_print_int_hex("@ surface_release", jcls_GCISurfaceInfo);

	jmid_isNativeSurface = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "isNativeSurface", "()Z");
	debug_print_int_hex("@ surface_release", jmid_isNativeSurface);

	isNativeSurface = (*env)->CallBooleanMethod (env, jobj_gciDrawingSurface, jmid_isNativeSurface);
	debug_print_int_hex("@ surface_release", isNativeSurface);

	// ***********
	jmid_getFormat = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getFormat", "()I");
	debug_print_int_hex("@ surface_release", jmid_getFormat);
	format = (*env)->CallIntMethod(env, jobj_gciDrawingSurface, jmid_getFormat);
	debug_print_int_hex("@ surface_release", format);
	// ***********

	// -------------------------------------------------------------------------------------------------

	if (isNativeSurface) {

		jmid_getBasePointer = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getBasePointer", "()J");
		debug_print_int_hex("@ surface_release", jmid_getBasePointer);

		basePointer = (*env)->CallLongMethod (env, jobj_surfaceInfo, jmid_getBasePointer);
		debug_print_int_hex("@ surface_release", basePointer);
	}
	else {
		jmethodID jmid_getPixelArrayType;
		jint pixelArrayType;
		jmethodID jmid_getPixelArray;
		jintArray pixelArray;


		jmid_getPixelArrayType = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArrayType", "()I");
		debug_print_int_hex("@ surface_release", jmid_getPixelArrayType);

		pixelArrayType = (*env)->CallIntMethod(env, jobj_surfaceInfo, jmid_getPixelArrayType);
		debug_print_int_hex("@ surface_release", pixelArrayType);

		jmid_getPixelArray = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArray", "()Ljava/lang/Object;");
		debug_print_int_hex("@ surface_release", jmid_getPixelArray);

		pixelArray = (jintArray) ((*env)->CallObjectMethod(env, jobj_surfaceInfo, jmid_getPixelArray));
		debug_print_int_hex("@ surface_release", pixelArray);
		
		//basePointer = (*env)->GetIntArrayElements  (env, pixelArray, 0);
		(*env)->ReleaseIntArrayElements  (env, pixelArray, (jint*)surface->super.data, 0);
	}
}


static void
surface_cleanup(AbstractSurface* surface) {
    // IMPL NOTE : to fix warning : unused parameter
    (void)surface;
}

