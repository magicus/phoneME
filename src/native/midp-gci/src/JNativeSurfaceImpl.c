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

//#include <JAbstractSurface.h>

//#include <kni.h>
#include <jni.h>


#include <JNIUtil.h>
#include <stdio.h>



static jboolean fieldIdsInitialized = JNI_FALSE;



static jboolean initializeSurfaceFieldIds(JNIEnv *env, jobject objectHandle);



// ==============================================================================================
JNIEXPORT void JNICALL
Java_com_sun_pisces_NativeSurface_clean(
    JNIEnv *env,
    jobject instance) {

	printf ("Java_com_sun_pisces_NativeSurface_clean\n");
    fflush(stdout);

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

static jboolean
initializeSurfaceFieldIds(JNIEnv *env, jobject objectHandle) {
    static const FieldDesc surfaceFieldDesc[] = {
                { "nativePtr", "J" },
                { NULL, NULL }
            };

    jboolean retVal;
    jclass classHandle;

    if (fieldIdsInitialized) {
        return JNI_TRUE;
    }
    retVal = JNI_FALSE;



    classHandle = (*env)->GetObjectClass(env, objectHandle);
//    if (initializeFieldIds(fieldIds, classHandle, surfaceFieldDesc)) {
//        retVal = KNI_TRUE;
//        fieldIdsInitialized = KNI_TRUE;
//    }


    return retVal;
}
