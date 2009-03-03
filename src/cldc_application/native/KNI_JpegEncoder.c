/*
 * 
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#include "KNICommon.h"
#include "jsrop_exceptions.h"
#include "javacall_multimedia.h"


/*  private static native byte[] encode0(byte[] rgbData, int w, int h, int qaulity); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_JPEGEncoder_encode0) {
    jint w =  KNI_GetParameterAsInt(2);
    jint h =  KNI_GetParameterAsInt(3);
    jint q =  KNI_GetParameterAsInt(4);
    jint rgbLen;
    KNI_StartHandles(2);
    KNI_DeclareHandle(rgb);
    KNI_DeclareHandle(jpeg);
    KNI_GetParameterAsObject(1, rgb);
    rgbLen = KNI_GetArrayLength(rgb);
    do {
        if (rgbLen > 0) {
            jbyte* rgbData;
            jbyte* jpegData;
            jint jpegLen;
            javacall_result res;
            
            rgbData = (jbyte*) JAVAME_MALLOC(rgbLen);
            if (NULL == rgbData) {
                KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                break;
            }
            KNI_GetRawArrayRegion(rgb, 0, rgbLen, (jbyte*)rgbData);

            res = javacall_media_encode(rgbData, w, h, JAVACALL_JPEG_ENCODER,
                                        q, &jpegData, &jpegLen);
            
            if (JAVACALL_OK == res) {
                SNI_NewArray(SNI_BYTE_ARRAY, jpegLen, jpeg);
                if (!KNI_IsNullHandle(jpeg)) {
                    KNI_SetRawArrayRegion(jpeg, 0, jpegLen, jpegData);
                } else {
                    KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                }
                javacall_media_release_data(jpegData, jpegLen);
            } else if (JAVACALL_OUT_OF_MEMORY == res) {
                KNI_ThrowNew(jsropOutOfMemoryError, NULL);
            }
            JAVAME_FREE(rgbData);
        }
    } while ( 0 );
    KNI_EndHandlesAndReturnObject(jpeg);
}
