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


#define MAX_PATH_LEN 1024
#define APP_DIR "appdb"
#define CONFIG_DIR "lib"

/**
 * Initializes the midp storage.
 *
 * @param home path to the MIDP working directory.
 * @returns true if the initialization succeeds, false otherwise
 */

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_jump_JumpInit_initMidpStorage) {
    /* These need to static for midpSetAppDir and midpSetConfigDir. */
    static char appDir[MAX_PATH_LEN];
    static char configDir[MAX_PATH_LEN];

    jchar jbuff[MAX_PATH_LEN];
    char cbuff[MAX_PATH_LEN];
    int max = MAX_PATH_LEN - strlen(APP_DIR) - 2 /* fsep + terminator */;
    int len, i, err;
    MIDPError status = OUT_OF_MEMORY;

    KNI_StartHandles(1);
    KNI_DeclareHandle(home);
    KNI_GetParameterAsObject(1, home);

    len = KNI_GetStringLength(home);
    if (len <= max) {
        KNI_GetStringRegion(home, 0, len, jbuff);

        for (i = 0; i < len; i++) {
            cbuff[i] = (char)jbuff[i];
        }

        cbuff[len] = (char)storageGetFileSeparator();
        cbuff[len + 1] = 0;

        strcpy(appDir, cbuff);
        strcat(appDir, APP_DIR);

        midpSetAppDir(appDir);

        strcpy(configDir, cbuff);
        strcat(configDir, CONFIG_DIR);

        midpSetConfigDir(configDir);

        err = storageInitialize(configDir, appDir);

        if (err == 0) {
            status = midp_suite_storage_init();
        } else {
            status = OUT_OF_MEMORY;
        }
    }

    KNI_EndHandles();

    if (status != ALL_OK) {
       KNI_ReturnBoolean(KNI_FALSE);
    } else {
       KNI_ReturnBoolean(KNI_TRUE);
    }
}


