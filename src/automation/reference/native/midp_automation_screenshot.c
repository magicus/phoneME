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


/**
 * AutoScreenshotTaker native methods implementation for
 * putpixel graphic library.
 */

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <sni.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <midpError.h>
#include <string.h>
#include <commonKNIMacros.h>

/** Taken screenshot width */
static int gsScreenshotWidth = 0;

/** Taken screenshot height */
static int gsScreenshotHeight = 0;

/** Taken screenshot pixels data */
static gxj_pixel_type* gsScreenshotData = NULL;

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_takeScreenshot0) {
    int oldw = gsScreenshotWidth;
    int oldh = gsScreenshotHeight;
    int sz;
    
    gsScreenshotWidth = gxj_system_screen_buffer.width;
    gsScreenshotHeight = gxj_system_screen_buffer.height;

    /* the dimensions of the screen may have changed */
    if (oldw != gsScreenshotWidth || oldh != gsScreenshotHeight) {
        if (gsScreenshotData != NULL) {
            midpFree(gsScreenshotData);
            gsScreenshotData = NULL;
        }
    }

    /* allocate memory for pixels data, if needed */
    sz = sizeof(gxj_pixel_type) * gsScreenshotWidth * gsScreenshotHeight;
    if (gsScreenshotData == NULL) {
        gsScreenshotData = (gxj_pixel_type*)midpMalloc(sz);
        if (gsScreenshotData == NULL) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    }

    /* and save pixels data */
    if (gsScreenshotData != NULL) {
        memcpy(gsScreenshotData, gxj_system_screen_buffer.pixelData, sz);
    }

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotWidth0) {
    KNI_ReturnInt(gsScreenshotWidth);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotHeight0) {
    KNI_ReturnInt(gsScreenshotHeight);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotRGB8880) {
    int totalPixels;
    int i;
    jbyte* rgb888;
    int rgb888Off;
    jbyte r, g, b;

    KNI_StartHandles(1); 
    KNI_DeclareHandle(returnArray); 
 
    do {
        totalPixels = gsScreenshotWidth * gsScreenshotHeight;

        /* create Java array we are going to return */
        SNI_NewArray(SNI_BYTE_ARRAY, totalPixels * 3, returnArray);
        if (KNI_IsNullHandle(returnArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        /* and fill it with pixels data */
        rgb888 = JavaByteArray(returnArray); 
        for (i = 0, rgb888Off = 0; i < totalPixels; ++i, rgb888Off += 3) {
            gxj_pixel_type pixel = gsScreenshotData[i];
            r = GXJ_GET_RED_FROM_PIXEL(pixel);
            g = GXJ_GET_GREEN_FROM_PIXEL(pixel);
            b = GXJ_GET_BLUE_FROM_PIXEL(pixel);

            rgb888[rgb888Off] = r;
            rgb888[rgb888Off + 1] = g;
            rgb888[rgb888Off + 2] = b;
        }
        
    } while (0);
 
    KNI_EndHandlesAndReturnObject(returnArray); 
}
