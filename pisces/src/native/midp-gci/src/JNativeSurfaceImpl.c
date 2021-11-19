/*
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved. 
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

// struct _AbstractSurface
#include <JAbstractSurface.h>

#include <kni.h>
#include <jni.h>


#include <JNIUtil.h>


//#define SVG_DEBUG
#ifdef SVG_DEBUG

#include <stdio.h>
#define debug_print_int_hex(prefix,var) do { printf("%s %s=0x%x\n", prefix, #var, (int)(var)); fflush(stdout); } while(0)
#define debug_print_int(prefix,var) do { printf("%s %s=%d\n", prefix, #var, (int)(var)); fflush(stdout); } while(0)

#else

#define debug_print_int(prefix,var)
#define debug_print_int_hex(prefix,var)

#endif


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



// ==============================================================================================
JNIEXPORT void JNICALL
Java_com_sun_pisces_NativeSurface_clean(
    JNIEnv *env,
    jobject instance) {


    AbstractSurface* surface;

    initializeSurfaceFieldIds(env, instance);

    surface = (AbstractSurface *) (long) KNI_GetLongField(instance, fieldIds[
                                                            SURFACE_NATIVE_PTR])
                                                            ;
    if (surface != NULL) {
        if (surface->super.data != NULL) {
            memset(surface->super.data, 0, surface->super.width *
                                           surface->super.height *
                                           sizeof(jint));
        }
    }
}



/**
 * public native void draw(Graphics g, int x, int y, int w, int h, float alpha);
 * g: destination
 * this: instance of class NativeSurface
 *
 * Copy data from surface[AbstractSurface] -> GCI surface (associated with g:Graphics)
 */
JNIEXPORT void JNICALL
Java_com_sun_pisces_NativeSurface_draw(
    JNIEnv *env,
    jobject instance,
    jobject graphicsHandle,
    jint x,
    jint y,
    jint w,
    jint h,
    jfloat alpha) {


    int offset, xx, yy;
    int soffset;
    int rgb;
    int srgb;
    int r, g, b;
    int dr, dg, db;
    int sr, sg, sb;
    int sa, sam;
    int svalue;

    int xStride;
    int yStride;


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


	jlong surfacePtr;
	AbstractSurface* surface;

	// initialize field id [nativePtr]
	initializeSurfaceFieldIds(env, instance);

	debug_print_int_hex("@ draw", x);
	debug_print_int_hex("@ draw", y);
	debug_print_int_hex("@ draw", w);
	debug_print_int_hex("@ draw", h);


	graphicsClassHandle = (*env)->GetObjectClass(env, graphicsHandle);
	debug_print_int_hex("@ draw", graphicsClassHandle);

	fid_gciDrawingSurface = (*env)->GetFieldID(env, graphicsClassHandle, "gciDrawingSurface", "Lcom/sun/me/gci/surface/GCIDrawingSurface;");
	debug_print_int_hex("@ draw", fid_gciDrawingSurface);


	jobj_gciDrawingSurface = (jobject) (*env)->GetObjectField (env, graphicsHandle, fid_gciDrawingSurface);
	debug_print_int_hex("@ draw", jobj_gciDrawingSurface);

	jcls_GCIDrawingSurface = (*env)->FindClass (env, "com/sun/me/gci/surface/GCIDrawingSurface");
	debug_print_int_hex("@ draw", jcls_GCIDrawingSurface);

	jmid_getSurfaceInfo = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getSurfaceInfo", "()Lcom/sun/me/gci/surface/GCISurfaceInfo;");
	debug_print_int_hex("@ draw", jmid_getSurfaceInfo);

	jobj_surfaceInfo = (*env)->CallObjectMethod (env, jobj_gciDrawingSurface, jmid_getSurfaceInfo);
	debug_print_int_hex("@ draw", jobj_surfaceInfo);

	jcls_GCISurfaceInfo = (*env)->FindClass (env, "com/sun/me/gci/surface/GCISurfaceInfo");
	debug_print_int_hex("@ draw", jcls_GCISurfaceInfo);

	jmid_isNativeSurface = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "isNativeSurface", "()Z");
	debug_print_int_hex("@ draw", jmid_isNativeSurface);

	isNativeSurface = (*env)->CallBooleanMethod (env, jobj_gciDrawingSurface, jmid_isNativeSurface);
	debug_print_int_hex("@ draw", isNativeSurface);

	// ***********
	jmid_getFormat = (*env)->GetMethodID (env, jcls_GCIDrawingSurface, "getFormat", "()I");
	debug_print_int_hex("@ draw", jmid_getFormat);
	format = (*env)->CallIntMethod(env, jobj_gciDrawingSurface, jmid_getFormat);
	debug_print_int_hex("@ draw", format);


    // -----------------------------------------------------------------------------------------------------

    if (isNativeSurface) {

		jmid_getBasePointer = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getBasePointer", "()J");
		debug_print_int_hex("@ draw", jmid_getBasePointer);

		basePointer = (*env)->CallLongMethod (env, jobj_surfaceInfo, jmid_getBasePointer);
		debug_print_int_hex("@ draw", basePointer);
	}
	else {
		jmethodID jmid_getPixelArrayType;
		jint pixelArrayType;
		jmethodID jmid_getPixelArray;
		jintArray pixelArray;


		jmid_getPixelArrayType = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArrayType", "()I");
		debug_print_int_hex("@ draw", jmid_getPixelArrayType);

		pixelArrayType = (*env)->CallIntMethod(env, jobj_surfaceInfo, jmid_getPixelArrayType);
		debug_print_int_hex("@ draw", pixelArrayType);

		jmid_getPixelArray = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getPixelArray", "()Ljava/lang/Object;");
		debug_print_int_hex("@ draw", jmid_getPixelArray);

		pixelArray = (jintArray) ((*env)->CallObjectMethod(env, jobj_surfaceInfo, jmid_getPixelArray));
		debug_print_int_hex("@ draw", pixelArray);

		basePointer = (*env)->GetIntArrayElements  (env, pixelArray, 0);
		debug_print_int_hex("@ draw", basePointer);
	}


	// -----------------------------------------------------------------------------------------------------
	jmid_getXBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getXBitStride", "()I");
	debug_print_int_hex("@ draw", jmid_getXBitStride);

	xBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getXBitStride);
	xStride = xBitStride >> 3 >> 2; // in pixels
	debug_print_int_hex("@ draw", xBitStride);
	debug_print_int_hex("@ draw", xStride);


	jmid_getYBitStride = (*env)->GetMethodID (env, jcls_GCISurfaceInfo, "getYBitStride", "()I");
	debug_print_int_hex("@ draw", jmid_getYBitStride);

	yBitStride = (*env)->CallIntMethod (env, jobj_surfaceInfo, jmid_getYBitStride);
	yStride = yBitStride >> 3 >> 2; // in pixels
	debug_print_int_hex("@ draw", yBitStride);
	debug_print_int_hex("@ draw", yStride);


	// -----------------------------------------------------------------------------------------------------



    // this: instance of NativeSurface extends AbstractSurface: has field nativePtr
	surfacePtr = (*env)->GetLongField (env, instance, fieldIds[SURFACE_NATIVE_PTR]);
    surface = (AbstractSurface *)  surfacePtr;
	debug_print_int_hex("@ draw", surface);
	debug_print_int_hex("@ draw", surface->super.imageType);

    if (surface != NULL && surface->super.data != NULL && alpha != 0 && basePointer != 0) {
//        if (destination has buffer for alpha data) {
	        for(yy = 0 ; yy < surface->super.height; yy++) {
//	        	debug_print_int_hex("@ draw", yy);
        	    for(xx = 0; xx < surface->super.width; xx++) {
    	            soffset = yy * surface->super.width + xx; // in pixels (= units of 4 bytes)

					if (surface->super.alphaData != NULL) {
            	    	svalue = *((int *) surface->super.alphaData + soffset);
            	    	debug_print_int_hex("@ draw", svalue);
            	    }


					svalue = *((int *) surface->super.data + soffset);

            	    debug_print_int_hex("@ draw", svalue);
            	    sa = (int) ((svalue & 0xff000000) >> 24) * alpha + 0.5;
					debug_print_int_hex("@ draw", sa);

                	sam = 255 - sa;

					if (sa != 0) {
                    	offset = (yy + y) * yStride + xx + x;
                    	debug_print_int_hex("@ draw", offset);


                        srgb = svalue;
						debug_print_int_hex("@ draw", srgb);

                        rgb = *((int *)basePointer + offset);
                        debug_print_int_hex("@ draw", rgb);

                        r = (rgb >> 16) & 0xFF;
                        debug_print_int_hex("@ draw", r);
                        g = (rgb >> 8) & 0xFF;
                        debug_print_int_hex("@ draw", g);
                        b = (rgb & 0xFF);
                        debug_print_int_hex("@ draw", b);

                        sr = (srgb >> 16) & 0xFF;
                        debug_print_int_hex("@ draw", sr);
                        sg = (srgb >> 8) & 0xFF;
                        debug_print_int_hex("@ draw", sg);
                        sb = (srgb & 0xFF);
                        debug_print_int_hex("@ draw", sb);

                        dr = (sr * sa + sam * r) / (255);
                        debug_print_int_hex("@ draw", dr);
                        dg = (sg * sa + sam * g) / (255);
                        debug_print_int_hex("@ draw", dg);
                        db = (sb * sa + sam * b) / (255);
                        debug_print_int_hex("@ draw", db);


                        *((int*) basePointer + offset) =
                         (dr << 16) | (dg << 8) | db;
                         // (sa << 24) | 
                    }
				}
	        //}
        }
    }


// TODO: release basePointer for java surface....
}


// ==============================================================================================

// Helper functions
// =============================================================================


/**
 * @param objectHandle (class)
 */
static jboolean
initializeSurfaceFieldIds(JNIEnv *env, jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },           // in class com.sun.pisces.AbstractSurface
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
