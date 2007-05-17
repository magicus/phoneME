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

#include <string.h>
#include "midpEvents.h"
#include "KNICommon.h"
#include "commonKNIMacros.h"

/* KNI Implementation **********************************************************************/

/*  protected native int nInit (int isolatedId, int playerId, String mimeType, String URI, long contentLength ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectPlayer_nInit) {
    jint  isolateId = KNI_GetParameterAsInt(1);
    jint  playerId = KNI_GetParameterAsInt(2);
    jlong contentLength = KNI_GetParameterAsLong(5);
    jint  returnValue = 0;
    jint   mimeLength, URILength;
    jchar* pszMimeType = NULL;
    jchar* pszURI = NULL;
    KNIPlayerInfo* pKniInfo;
    javacall_int64 uniqueId;

    MMP_DEBUG_STR2("+nInit isolate=%d, player=%d\n", isolateId, playerId);

    /* Build 64 bit unique id by using isolate id and player id */
    uniqueId = isolateId;
    uniqueId = (uniqueId << 32 | playerId);

    KNI_StartHandles(2);
    KNI_DeclareHandle(mimeType);
    KNI_DeclareHandle(URI);
    
    /* Get mimeType and URI object parameter */
    KNI_GetParameterAsObject(3, mimeType);
    KNI_GetParameterAsObject(4, URI);

    /* Get mime type java string */
    mimeLength = KNI_GetStringLength(mimeType);
    pszMimeType = MMP_MALLOC(mimeLength * sizeof(jchar));
    if (pszMimeType) {
        KNI_GetStringRegion(mimeType, 0, mimeLength, pszMimeType);
    }

    /* Get URI java string */
    if (-1 == (URILength = KNI_GetStringLength(URI))) {
        pszURI = NULL;
    } else {
        pszURI = MMP_MALLOC(URILength * sizeof(jchar));
        if (pszURI) {
            KNI_GetStringRegion(URI, 0, URILength, pszURI);
        }
    }

    pKniInfo = (KNIPlayerInfo*)MMP_MALLOC(sizeof(KNIPlayerInfo));
            
    if (pKniInfo && pszMimeType /* && pszURI */) {
        /* prepare kni internal information */
        pKniInfo->uniqueId = uniqueId;
        pKniInfo->contentLength = (long)contentLength;
        pKniInfo->isAcquire = 0;
        pKniInfo->offset = 0;
        pKniInfo->hBuffer = 0;
        pKniInfo->isDirectFile = JAVACALL_FALSE;
        pKniInfo->isForeground = -1;
        pKniInfo->recordState = RECORD_CLOSE;
        pKniInfo->pNativeHandle = 
            javacall_media_create(uniqueId, pszMimeType, mimeLength, pszURI, URILength, (long)contentLength); 
        if (NULL == pKniInfo->pNativeHandle) {
            MMP_FREE(pKniInfo);
        } else {
            returnValue = (int)pKniInfo;
        }
    } else {
        if (pKniInfo) { MMP_FREE(pKniInfo); }
    }

    if (pszMimeType) { MMP_FREE(pszMimeType); }
    if (pszURI)      { MMP_FREE(pszURI); }
    
    MMP_DEBUG_STR1("-nInit return=%d\n", returnValue);

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

