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

// struct _AbstractSurface
#include <JAbstractSurface.h>

#include <kni.h>
#include <jni.h>


#include <JNIUtil.h>
#include <stdio.h>



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

	printf ("Java_com_sun_pisces_NativeSurface_clean\n");
	fflush(stdout);

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

	        printf ("Java_com_sun_pisces_NativeSurface_draw\n");
            fflush(stdout);

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
