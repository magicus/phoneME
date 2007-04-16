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

#include <kni.h>

#include <midpStorage.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpUtilKni.h>
#include <midpError.h>

/**
 * Checks if the file exists.
 *
 * @return true if the file is available, otherwise return false
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_MIDletAppImageGenerator_removeAppImage() {
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, fileName)    
    char* pszError;
    storage_delete_file(&pszError, &fileName);
    if (pszError != NULL) {
        /* There may not be an app image. */
        storageFreeError(pszError);
    }
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}
