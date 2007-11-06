/*
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
#include <jvm.h>
#include <midpAMS.h>
#include <midpStorage.h>
#include <suitestore_common.h>

/**
 * Initializes the midp storage.
 *
 * @param home path to the MIDP working directory.
 * @returns true if the initialization succeeds, false otherwise
 */

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_jump_JumpInit_initMidpStorage) {
    jchar jbuff[1024];
    char cbuff[1024];
    int max = sizeof(cbuff) - 1;
    int len, i, err;
    MIDPError status;

    KNI_StartHandles(1);
    KNI_DeclareHandle(home);
    KNI_GetParameterAsObject(1, home);
                                                                                   
    len = KNI_GetStringLength(home);
    if (len > max) {
        len = max;
    }
                                                                                   
    KNI_GetStringRegion(home, 0, len, jbuff);
    for (i=0; i<len; i++) {
        cbuff[i] = (char)jbuff[i];
    }
    cbuff[len] = 0;

    midpSetHomeDir(cbuff);
    err = storageInitialize(cbuff, cbuff);

    if (err == 0) {
         status = midp_suite_storage_init();
    } else {
         status = OUT_OF_MEMORY;
    }

    KNI_EndHandles();

    if (status != ALL_OK) {
       KNI_ReturnBoolean(KNI_FALSE);
    } else {
       KNI_ReturnBoolean(KNI_TRUE);
    }
}


