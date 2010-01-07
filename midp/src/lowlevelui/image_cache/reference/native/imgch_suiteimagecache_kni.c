/*
 *
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

#include <stddef.h>
#include <string.h>

#include <midpUtilKni.h>
#include <midpMalloc.h>

#include <imgapi_image.h>
#include <imgdcd_image_util.h>

#if ENABLE_IMAGE_CACHE
#include <imageCache.h>
#endif

#include <img_imagedata_load.h>


/**
 * Loads a native image data from image cache into ImageData..
 * <p>
 * Java declaration:
 * <pre>
 *     boolean loadAndCreateImmutableImageDataFromCache0(ImageData imageData,
 *                                                       int suiteId, 
 *                                                       String resName);
 * </pre>
 *
 * @param imageData The ImageData to be populated
 * @param suiteId   The suite Id
 * @param resName   The name of the image resource
 * @return true if a cached image was loaded, false otherwise
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_SuiteImageCacheImpl_loadAndCreateImmutableImageDataFromCache0) {
#if ENABLE_IMAGE_CACHE
    int len;
    SuiteIdType suiteId;
    jboolean status = KNI_FALSE;
    unsigned char *rawBuffer = NULL;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(3, resName)

    KNI_DeclareHandle(imageData);
    KNI_GetParameterAsObject(1, imageData);

    suiteId = KNI_GetParameterAsInt(2);

    len = loadImageFromCache(suiteId, &resName, &rawBuffer);
    if (len != -1 && rawBuffer != NULL) {
        /* image is found in cache */
        status = img_load_imagedata_from_raw_buffer(KNIPASSARGS
            imageData, rawBuffer, len);
    }

    midpFree(rawBuffer);

    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
#else
    KNI_ReturnBoolean(KNI_FALSE);
#endif
}

