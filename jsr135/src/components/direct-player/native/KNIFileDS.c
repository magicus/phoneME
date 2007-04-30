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

#include "KNICommon.h"
#include "javacall_file.h"

/*  private native boolean nCheckFileExist ( String path ) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_mmedia_protocol_FileDS_nCheckFileExist() {
#define MAX_FILENAME_SIZE 100

    jboolean returnValue = KNI_FALSE;
    jsize length;
    jchar uFileName[MAX_FILENAME_SIZE + 1] = {0};

    KNI_StartHandles(1);
    KNI_DeclareHandle(pathHandle);

    /*
    KNI_GetParameterAsObject(1, pathHandle);
    length = KNI_GetStringLength(pathHandle);
    if (length < MAX_FILENAME_SIZE) {
        KNI_GetStringRegion(pathHandle, 0, length, uFileName);
        if (JAVACALL_OK == javacall_file_exist(uFileName, length)) {
            returnValue = KNI_TRUE;
        }
    } 
    */
 
    KNI_EndHandles();
    KNI_ReturnBoolean(returnValue);
}

/*  private native int nGetFileSize ( String path ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_protocol_FileDS_nGetFileSize() {
    jint returnValue = 0;

    jsize length;
    jchar uFileName[MAX_FILENAME_SIZE + 1] = {0};

    KNI_StartHandles(1);
    KNI_DeclareHandle(pathHandle);

    /*
    KNI_GetParameterAsObject(1, pathHandle);
    length = KNI_GetStringLength(pathHandle);
    if (length < MAX_FILENAME_SIZE) {
        KNI_GetStringRegion(pathHandle, 0, length, uFileName);
        returnValue = (jint)javacall_file_sizeof(uFileName, length);
    } 
    */
   
    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}
