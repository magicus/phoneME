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
#include <stdio.h>



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

JNIEXPORT void JNICALL
Java_com_sun_pisces_GraphicsSurface_initialize(
	JNIEnv *env,
    jobject instance) {

    AbstractSurface* surface;


    printf("@ initialize\n");
    fflush(stdout);
    

	if (surface_initialize(env, instance) && initializeSurfaceFieldIds(env, instance)) {
		surface = my_malloc(AbstractSurface, 1);


		printf("@ initialize: surface=%x\n", surface);
		fflush(stdout);

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
                { "nativePtr", "J" },
//                { "g", "Ljavax/microedition/lcdui/Graphics;" },
                { "g", "Ljava/lang/Object;" },
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
    printf("@ surface_acquire: graphicsHandle=%x\n", graphicsHandle);

    graphicsClassHandle = (*env)->GetObjectClass(env, graphicsHandle);
    printf("@ surface_acquire: graphicsClassHandle=%x\n", graphicsClassHandle);

    {
//        jmethodID mid_Graphics_toString = (*env)->GetMethodID(env, graphicsClassHandle, "toString", "()Ljava/lang/String;");
//        jstring str = (jstring) (*env)->CallObjectMethod(env, graphicsClassHandle, mid_Graphics_toString, NULL);
//        const char* graphicsClassName = (*env)->GetStringUTFChars(env, str, NULL);
//        printf("@ surface_acquire: graphicsClassName=%s\n", graphicsClassName);
	}

	fid_gciDrawingSurface = (*env)->GetFieldID(env, graphicsClassHandle, "gciDrawingSurface", "Lcom/sun/me/gci/surface/GCIDrawingSurface;");
	printf("@ surface_acquire: fid_gciDrawingSurface=%x\n", fid_gciDrawingSurface);
	fprintf(stderr, "@ surface_acquire: fid_gciDrawingSurface=%x\n", fid_gciDrawingSurface);
	fflush(stderr);


	jobj_gciDrawingSurface = (jobject) (*env)->GetObjectField (env, graphicsHandle, fid_gciDrawingSurface);
	printf("jobj_gciDrawingSurface = 0x%x\n", (int)jobj_gciDrawingSurface);

	jcls_GCIDrawingSurface = (*env)->FindClass (env, "com/sun/me/gci/surface/GCIDrawingSurface");
	printf("jcls_GCIDrawingSurface = 0x%x\n", (int)jcls_GCIDrawingSurface);

	jmid_isNativeSurface = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "isNativeSurface", "()Z");
	printf ("jmid_isNativeSurface = 0x%x\n", (int)jmid_isNativeSurface);

	isNativeSurface = (*env)->CallBooleanMethod (env, jobj_gciDrawingSurface, jmid_isNativeSurface);
	printf ("isNativeSurface = 0x%x\n", (int)isNativeSurface);

    // ***********

	jmid_getFormat = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getFormat", "()I");
	printf ("jmid_getFormat = 0x%x\n", (int)jmid_getFormat);
	format = (*env)->CallIntMethod(env, jobj_gciDrawingSurface, jmid_getFormat);
	printf ("format = 0x%x\n", (int)format);

    // ***********


	jmid_getSurfaceInfo = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getSurfaceInfo", "()Lcom/sun/me/gci/surface/GCISurfaceInfo;");
	printf ("jmid_getSurfaceInfo = 0x%x\n", (int)jmid_getSurfaceInfo);

	jobj_surfaceInfo = (*env)->CallObjectMethod (env, jobj_gciDrawingSurface, jmid_getSurfaceInfo);
	printf ("jobj_surfaceInfo = 0x%x\n", (int)jobj_surfaceInfo);

	jcls_GCISurfaceInfo = (*env)->FindClass (env, "com/sun/me/gci/surface/GCISurfaceInfo");
	printf("jcls_GCISurfaceInfo = 0x%x\n", (int)jcls_GCISurfaceInfo);


	jmid_getBasePointer = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getBasePointer", "()J");
	printf ("jmid_getBasePointer = 0x%x\n", (int)jmid_getBasePointer);

	basePointer = (*env)->CallLongMethod (env, jobj_surfaceInfo, jmid_getBasePointer);
	printf ("basePointer = 0x%x\n", (int)basePointer);


	jmid_getXBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getXBitStride", "()I");
	printf ("jmid_getXBitStride = 0x%x\n", (int)jmid_getXBitStride);

	xBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getXBitStride);
	printf ("xBitStride = 0x%x\n", (int)xBitStride);


	jmid_getYBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getYBitStride", "()I");
	printf ("jmid_getYBitStride = 0x%x\n", (int)jmid_getYBitStride);

	yBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getYBitStride);
	printf ("yBitStride = 0x%x\n", (int)yBitStride);


	jmid_getWidth = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getWidth", "()I");
	printf ("jmid_getWidth = 0x%x\n", (int)jmid_getWidth);

	width = (*env)->CallIntMethod (env, jobj_gciDrawingSurface, jmid_getWidth);
	printf ("width = %d\n", (int)width);


	jmid_getHeight = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getHeight", "()I");
	printf ("jmid_getHeight = 0x%x\n", (int)jmid_getHeight);

	height = (*env)->CallIntMethod (env, jobj_gciDrawingSurface, jmid_getHeight);
	printf ("height = %d\n", (int)height);


    fflush(stdout);

    if (!(0 == graphicsHandle)) {
    	printf ("fill surface\n");
    	fflush(stdout);

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

			printf ("done, ok\n");
			fflush(stdout);
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
    printf ("<<<surface_acquire\n");
    fflush(stdout);
}

static void
surface_release(AbstractSurface* surface, JNIEnv* env, jobject surfaceHandle) {
    // do nothing
    // IMPL NOTE : to fix warning : unused parameter
    (void)surface;
    (void)surfaceHandle;
}

static void
surface_cleanup(AbstractSurface* surface) {
    // IMPL NOTE : to fix warning : unused parameter
    (void)surface;
}

