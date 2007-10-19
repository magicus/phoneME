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

#define MAX_PROTOCOLNAME_LEN 10
#define MAX_MIMETYPENAME_LEN 100
typedef struct {
    const javacall_media_caps* pCaps;
    unsigned char protocol[MAX_PROTOCOLNAME_LEN];
    int p;
} ListCapsType;
typedef struct {
    javacall_const_utf8_string *protocols;
    int protocolCount;
    int p;
} ListProtocolsType;


KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesOpen) {
    ListCapsType *hdlr = NULL;
    javacall_utf16_string protocolUTF16=NULL;
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
                protocolNameLen = 0;
                javautil_unicode_utf16_ulength(protocolUTF16, &protocolNameLen);
                if (protocolNameLen != 0 && JAVACALL_OK != 
                    javautil_unicode_utf16_to_utf8(protocolUTF16, protocolNameLen+1,
                            hdlr->protocol, MAX_PROTOCOLNAME_LEN, &protocolNameLen)) {
                        FREE(hdlr);
                        hdlr = NULL;
                }
                if (protocolUTF16!=NULL) {
                    FREE(protocolUTF16);
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
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesNext) {
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
                    if(javautil_string_equals((unsigned char *)hdlr->pCaps[hdlr->p].protocols[i], (unsigned char *)hdlr->protocol)) {
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
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesClose) {
    ListCapsType *hdlr = NULL;
    if ((hdlr = (ListCapsType *)KNI_GetParameterAsInt(1))!= NULL) {
        FREE(hdlr);
    }
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsOpen) {
    ListProtocolsType *hdlr = NULL;
    javacall_utf16_string mimeUTF16=NULL;

    javacall_int32 mimeNameLen;
    javacall_media_caps* pCaps;

    unsigned char mime[MAX_MIMETYPENAME_LEN];


    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_GetParameterAsObject(1, stringObj);

    hdlr = MALLOC(sizeof(ListProtocolsType));
    if (hdlr != NULL) {
        pCaps = (javacall_media_caps*)javacall_media_get_caps();
        hdlr->p = 0;
        hdlr->protocolCount = 0;
        hdlr->protocols = NULL;
        if (pCaps != NULL) {
            if (!KNI_IsNullHandle(stringObj)) {
                if (JAVACALL_OK != 
                    jsrop_jstring_to_utf16_string(stringObj, &mimeUTF16)) {
                        FREE(hdlr);
                        hdlr = NULL;
                }
                mimeNameLen = 0;
                javautil_unicode_utf16_ulength(mimeUTF16,&mimeNameLen);
                if (mimeNameLen!= 0 && JAVACALL_OK != 
                    javautil_unicode_utf16_to_utf8(mimeUTF16, mimeNameLen+1,
                    mime, MAX_MIMETYPENAME_LEN, &mimeNameLen)) {
                        FREE(hdlr);
                        hdlr = NULL;
                }
                if (mimeUTF16 != NULL) {
                    FREE(mimeUTF16);
                }
                /* go to the caps for requested mime */
                while(pCaps->mimeType != NULL) {
                    if (javautil_string_equals(mime, (unsigned char *)pCaps->mimeType)) {
                        hdlr->protocols = pCaps->protocols;
                        hdlr->protocolCount = pCaps->protocolCount;
                        break;
                    }
                    pCaps++;
                }
            } else {
                /* go to the last caps for all supported protocols */
                while(pCaps->mimeType != NULL) pCaps++;
                hdlr->protocols = pCaps->protocols;
                hdlr->protocolCount = pCaps->protocolCount;
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
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsNext) {
    int i   = 0;
    ListProtocolsType *hdlr = NULL;
    javacall_utf16 protocolUTF16[MAX_PROTOCOLNAME_LEN];
    int protocolLen;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    hdlr = (ListProtocolsType *)KNI_GetParameterAsInt(1);
    KNI_ReleaseHandle(stringObj);

    if (hdlr != NULL && hdlr->p < hdlr->protocolCount &&
        hdlr->protocols != NULL && hdlr->protocols[hdlr->p] != NULL) {
        protocolLen = 0;
        while(hdlr->protocols[hdlr->p][protocolLen]!=0) protocolLen++;
        if (JAVACALL_OK == javautil_unicode_utf8_to_utf16(hdlr->protocols[hdlr->p], protocolLen, protocolUTF16, MAX_PROTOCOLNAME_LEN, &protocolLen) ) {
            protocolUTF16[protocolLen] = 0;
            jsrop_jstring_from_utf16_string(KNIPASSARGS protocolUTF16, stringObj);
        }
        hdlr->p++;
    }

    KNI_EndHandlesAndReturnObject(stringObj);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsClose) {
    ListProtocolsType *hdlr = NULL;
    if ((hdlr = (ListProtocolsType *)KNI_GetParameterAsInt(1)) != NULL) {
        FREE(hdlr);
    }
}
