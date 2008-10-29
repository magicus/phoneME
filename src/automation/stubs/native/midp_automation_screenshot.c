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
 * Stubs for AutoScreenshotTaker class native methods,
 * used for ports that doesn't use putpixel.
 */

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_takeScreenshot0) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotWidth0) {
    KNI_ReturnInt(0);    
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotHeight0) {
    KNI_ReturnInt(0);    
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_automation_AutoScreenshotTaker_getScreenshotRGB8880) {
    KNI_StartHandles(1); 
    KNI_DeclareHandle(objectHandle); 
 
    /* Set the handle explicitly to null */
    KNI_ReleaseHandle(objectHandle); 
 
    /* Return the null reference to the calling Java method */
    KNI_EndHandlesAndReturnObject(objectHandle); 
}
