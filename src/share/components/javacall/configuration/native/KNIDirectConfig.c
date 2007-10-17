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

#include "KNICommon.h"
#include <jsrop_kni.h>
#include <javautil_string.h>

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DefaultConfiguration_nIsAmrSupported) {
    int res = 0;
    int i   = 0;

    const javacall_media_caps* caps = javacall_media_get_caps();

    while( NULL != caps[ i ].mimeType )
    {
        if( 0 == strcmp( caps[ i ].mimeType, JAVACALL_AUDIO_AMR_MIME ) )
        {
            res = 1;
            break;
        }
        i++;
    }

    KNI_ReturnBoolean( res ); 
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DefaultConfiguration_nIsJtsSupported) {
    int res = 0;
    int i   = 0;

    const javacall_media_caps* caps = javacall_media_get_caps();

    while( NULL != caps[ i ].mimeType )
    {
        if( 0 == strcmp( caps[ i ].mimeType, JAVACALL_AUDIO_TONE_MIME ) )
        {
            res = 1;
            break;
        }
        i++;
    }

    KNI_ReturnBoolean( res ); 
}

#define MAX_PROTOCOLNAME_LEN 10
#define MAX_MIMETYPENAME_LEN 100
typedef struct {
    const javacall_media_caps* pCaps;
    unsigned char protocol[MAX_PROTOCOLNAME_LEN];
    int p;
} ListCapsType;

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListCapsOpen) {
    ListCapsType *hdlr = NULL;
    javacall_utf16_string protocolUTF16;
    javacall_int32 protocolNameLen;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_GetParameterAsObject(1, stringObj);

    hdlr = MALLOC(sizeof(ListCapsType));
    if (hdlr != NULL) {
        hdlr->pCaps = javacall_media_get_caps();
        hdlr->p = 0;
        if (hdlr->pCaps != NULL) {
            if (!KNI_IsNullHandle(stringObj)) {
                if (JAVACALL_OK != 
                    jsrop_jstring_to_utf16_string(stringObj, &protocolUTF16)) {
                        FREE(hdlr);
                        hdlr = NULL;
                }
                if (JAVACALL_OK != 
                    javautil_unicode_utf16_to_utf8(protocolUTF16, javautil_unicode_utf16_ulength(protocolUTF16),
                    hdlr->protocol, MAX_PROTOCOLNAME_LEN, &protocolNameLen)) {
                        FREE(hdlr);
                        hdlr = NULL;
                }
            } else {
                hdlr->protocol[0] = 0;
            }
        } else {
            FREE(hdlr);
            hdlr = NULL;
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt( (jint)hdlr ); 
};

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListCapsNext) {
    int i   = 0;
    ListCapsType *hdlr = NULL;
    javacall_utf16 mimeUTF16[MAX_MIMETYPENAME_LEN];
    int mimeLen;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    hdlr = (ListCapsType *)KNI_GetParameterAsInt(1);
    KNI_ReleaseHandle(stringObj);

    if (hdlr != NULL && hdlr->p >=0) {
        while(hdlr->pCaps[hdlr->p].mimeType != NULL) {
            if (hdlr->protocol[0] == 0) {
                break;
            } else {
                for (i=0; i<hdlr->pCaps[hdlr->p].protocolCount; i++) {
                    if(javautil_string_equals(hdlr->pCaps[hdlr->p].protocols[i], hdlr->protocol)) {
                        break;
                    }
                }
                if (i<hdlr->pCaps[hdlr->p].protocolCount) {
                    break;
                }
            }
            hdlr->p++;
        }
        if (hdlr->pCaps[hdlr->p].mimeType != NULL) {
            mimeLen = 0;
            while(hdlr->pCaps[hdlr->p].mimeType[mimeLen]!=0) mimeLen++;
            if (JAVACALL_OK == javautil_unicode_utf8_to_utf16(hdlr->pCaps[hdlr->p].mimeType, mimeLen, mimeUTF16, MAX_MIMETYPENAME_LEN, &mimeLen) ) {
                mimeUTF16[mimeLen] = 0;
                jsrop_jstring_from_utf16_string(KNIPASSARGS mimeUTF16, stringObj);
            }
            hdlr->p++;
        }
    }

    KNI_EndHandlesAndReturnObject(stringObj);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListCapsClose) {
    ListCapsType *hdlr = NULL;
    hdlr = (ListCapsType *)KNI_GetParameterAsInt(1);
    if (hdlr != NULL) {
        if (hdlr->protocol != NULL) {
            FREE(hdlr->protocol);
            }    
        FREE(hdlr);
    }
}
