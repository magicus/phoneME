/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include <sni.h>
#include <midpError.h>

#include <javacall_multimedia.h>

static javacall_result 
    createStringArray(javacall_const_utf8_string header,
        javacall_const_utf8_string* pStrings, int number, 
        jobject jStrings) 
{
    javacall_result crResult = JAVACALL_OK;

    KNI_StartHandles(1);
    KNI_DeclareHandle(jtype);

    number++;

    SNI_NewArray(SNI_STRING_ARRAY, number, jStrings);
    if (KNI_IsNullHandle(jStrings)) {
        crResult = JAVACALL_OUT_OF_MEMORY;
    } else {
        int i;
        for (i = -1; i < number; i++) {
            javacall_const_utf8_string curStr;
            if (i < 0)
                curStr = header;
            else
                curStr = pStrings[i];
            KNI_NewStringUTF(curStr, jtype);
            if (KNI_IsNullHandle(jtype)) {
                 crResult = JAVACALL_OUT_OF_MEMORY;
                 break;
            }
            KNI_SetObjectArrayElement(jStrings, i, jtype);
        }
    }

    KNI_EndHandles();
    return crResult;
}

static void checkAndRaiseException(javacall_result jcresult)
{
    if (!JAVACALL_SUCCEEDED(jcresult))
        switch (jcresult) {
            case JAVACALL_OUT_OF_MEMORY: 
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
            case JAVACALL_NOT_IMPLEMENTED:
            case JAVACALL_INVALID_ARGUMENT: 
                KNI_ThrowNew(midpIllegalArgumentException, NULL); 
                break;
            default:
                KNI_ThrowNew(midpRuntimeException, NULL);
                break;
    }
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_mmedia_DefaultConfiguration_nGetMediaCaps()
{
    javacall_result jcresult = JAVACALL_OK;
    KNI_StartHandles(2);
    KNI_DeclareHandle(mediaCaps);
    KNI_DeclareHandle(mediaCapsItem);

    int count = 0;
    const javacall_media_caps* pMP = javacall_media_get_caps();
    const javacall_media_caps* pCur = pMP;

    if (pCur) {
        while (pCur->mimeType != NULL) {
            if (pCur->protocolCount > 0)
                count++;
            pCur++;
        }
    }

    SNI_NewArray(SNI_OBJECT_ARRAY, count, mediaCaps);
    if (KNI_IsNullHandle(mediaCaps)) {
        jcresult = JAVACALL_OUT_OF_MEMORY;
    } else {
        pCur = pMP;
        count = 0;

        while (pCur->mimeType != NULL) {
            if (pCur->protocolCount > 0) {
                jcresult = createStringArray(pCur->mimeType, 
                    pCur->protocols, pCur->protocolCount, 
                    mediaCapsItem);
                if (!JAVACALL_SUCCEEDED(jcresult))
                    break;
                KNI_SetObjectArrayElement(mediaCaps, count, mediaCapsItem);
                count++;
            }
            pCur++;
        }
    }

    checkAndRaiseException(jcresult);

    KNI_EndHandlesAndReturnObject(mediaCaps);
}
