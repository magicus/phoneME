/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#include <stdio.h>
#include <jsrop_kni.h>
#include <javautil_string.h>

#define MAX_PROTOCOLNAME_LEN 30
#define MAX_MIMETYPENAME_LEN 50

#define MIME_AUDIO_TONE "audio/x-tone-seq"
#define MIME_AUDIO_MIDI "audio/midi"
#define DEVICE_PROTOCOL "device"

typedef struct {
    char *current;
    char list[1];
} ListIterator;

static struct _protocolNames {
    int proto_mask;
    char *proto_name;
} protocolNames[] = {
    JAVACALL_MEDIA_MEMORY_PROTOCOL,     "memory",
    JAVACALL_MEDIA_FILE_LOCAL_PROTOCOL, "file",
    JAVACALL_MEDIA_FILE_REMOTE_PROTOCOL,"file",
    JAVACALL_MEDIA_HTTP_PROTOCOL,       "http",
    JAVACALL_MEDIA_HTTPS_PROTOCOL,      "https",
    JAVACALL_MEDIA_RTP_PROTOCOL,        "rtp",
    JAVACALL_MEDIA_RTSP_PROTOCOL,       "rtsp",
    JAVACALL_MEDIA_CAPTURE_PROTOCOL,    "capture"
};

void mmapi_string_delete_duplicates(char *p);
static javacall_result simple_jcharString_to_asciiString(jchar *jcharString, jsize jcharStringLen, char *asciiStringBuffer, jsize bufferSize);

const int caps_sanity_size_limit = 256;

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_DefaultConfiguration_nIsRadioSupported) {
    javacall_media_configuration *cfg;
    jboolean res = KNI_FALSE;
    if (javacall_media_get_configuration(&cfg) != JAVACALL_OK) {
        KNI_ThrowNew(jsropRuntimeException, "Couldn't get MMAPI configuration");
    }
    else if( JAVACALL_TRUE == cfg->supportCaptureRadio )
    {
        javacall_media_caps *caps = cfg->mediaCaps;
        int i = 0;
        if( NULL != cfg->mediaCaps )
        {
            for( i = 0; NULL != cfg->mediaCaps[i].mediaFormat && 
                    i < caps_sanity_size_limit; i++ )
            {
                if( JAVACALL_MEDIA_FORMAT_CAPTURE_RADIO == 
                    cfg->mediaCaps[i].mediaFormat )
                {
                    res = KNI_TRUE;
                    break;
                }
            }
            if( i >= caps_sanity_size_limit )
            {
                KNI_ThrowNew(jsropRuntimeException, 
                    "MMAPI configuration caps list is not null-terminated");
            }
        }
    }
    
    KNI_ReturnBoolean( res );
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesOpen) {
    javacall_int32 proto_mask = 0;
    javacall_bool deviceProtocol = JAVACALL_FALSE;
    javacall_media_configuration *cfg;
    javacall_media_caps *caps;
    int len;
    ListIterator *iterator = NULL;
    char *p;
    
    /* stack buffers. Trying to avoid malloc if a string is not big */
    jchar stack_string16_buffer[MAX_PROTOCOLNAME_LEN], *string16 = NULL;
    char stack_string_buffer[MAX_PROTOCOLNAME_LEN], *proto = NULL;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_GetParameterAsObject(1, stringObj);

    do {
        if (KNI_IsNullHandle(stringObj)) {
            proto = NULL;
        } else {
            len = KNI_GetStringLength(stringObj);
            /* if the string is longer than the stack buffer try to malloc it */
            if (len >= sizeof stack_string16_buffer / sizeof stack_string16_buffer[0]) {
                string16 = MMP_MALLOC((len + 1) * sizeof *string16);
                if (string16 == NULL) {
                    KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                    break;
                }
            } else {
                string16 = stack_string16_buffer;
            }
            if (len >= sizeof stack_string_buffer) {
                proto = MMP_MALLOC(len + 1);
                if (proto == NULL) {
                    KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                    break;
                }
            } else {
                proto = stack_string_buffer;
            }
            KNI_GetStringRegion(stringObj, 0, len, string16);
            if (simple_jcharString_to_asciiString(string16, len, proto, len + 1) != JAVACALL_OK) {
                KNI_ThrowNew(jsropIllegalArgumentException, "Illegal character in protocol name");
                break;
            }
        }
        if (proto != NULL) {
            int i;
            
            /* trying to find protocol by name */
            for (i = 0; i < sizeof protocolNames / sizeof protocolNames[0]; i++) {
                if (protocolNames[i].proto_name != NULL && !javautil_stricmp(protocolNames[i].proto_name, proto)) {
                    proto_mask |= protocolNames[i].proto_mask;
                }
            }
            if (!javautil_stricmp(proto, DEVICE_PROTOCOL)) {
                deviceProtocol = JAVACALL_TRUE;
            }
        }
        if (proto != NULL && proto_mask == 0 && !deviceProtocol) {
            /* Requested protocol wasn't found. Return 0 */
            break;
        }
        
        if (javacall_media_get_configuration(&cfg) != JAVACALL_OK) {
            KNI_ThrowNew(jsropRuntimeException, "Couldn't get MMAPI configuration");
            break;
        }
        
        /* how long will be list of content types? */
        len = 0;
        for (caps = cfg->mediaCaps; caps != NULL && caps->mediaFormat != NULL; caps++) {
            if (proto == NULL || (caps->wholeProtocols & proto_mask) != 0
                    || (caps->streamingProtocols & proto_mask) != 0) {
                len += strlen(caps->contentTypes) + 1; /* +1 for space char */
            }
        }
        if (proto == NULL || deviceProtocol) {
            if (cfg->supportDeviceMIDI) {
                len += strlen(MIME_AUDIO_MIDI) + 1;
            }
            if (cfg->supportDeviceTone) {
                len += strlen(MIME_AUDIO_TONE) + 1;
            }
        }
     
        if (len == 0) {
            /* No MIME types were found for provided protocol. Return 0 */
            break;
        }
     
        iterator = (ListIterator*)MMP_MALLOC(
            sizeof *iterator + len); /* zero terminator instead of last space */
        if (iterator == NULL) {
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
            break;
        }
    
        /* initialize the iterator */
        iterator->current = iterator->list;
    
        /* filling the list of content types */
        p = iterator->list;
        for (caps = cfg->mediaCaps; caps != NULL && caps->mediaFormat != NULL; caps++) {
            if (proto == NULL || (caps->wholeProtocols & proto_mask) != 0
                    || (caps->streamingProtocols & proto_mask) != 0) {
                int types_len = strlen(caps->contentTypes);
                memcpy(p, caps->contentTypes, types_len);
                p += types_len;
                *p++ = ' ';
            }
        }
        if (proto == NULL || deviceProtocol) {
            int types_len;
            if (cfg->supportDeviceMIDI) {
                types_len = strlen(MIME_AUDIO_MIDI);
                memcpy(p, MIME_AUDIO_MIDI, types_len);
                p += types_len;
                *p++ = ' ';
            }
            if (cfg->supportDeviceTone) {
                types_len = strlen(MIME_AUDIO_TONE);
                memcpy(p, MIME_AUDIO_TONE, types_len);
                p += types_len;
                *p++ = ' ';
            }
        }
        p--; *p = '\0'; /* replace last space with zero */
        
        mmapi_string_delete_duplicates(iterator->list);
    } while (0);

    /* freeing buffers */
    if (proto != NULL && proto != stack_string_buffer) {
        MMP_FREE(proto);
    }
    if (string16 != NULL && string16 != stack_string16_buffer) {
        MMP_FREE(string16);
    }
    KNI_EndHandles();
    KNI_ReturnInt((jint)iterator); 
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesNext) {
    ListIterator *iterator;
    char *p;
    int len;
    char stack_string_buffer[MAX_MIMETYPENAME_LEN], *mime = NULL;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    iterator = (ListIterator *)KNI_GetParameterAsInt(1);
    KNI_ReleaseHandle(stringObj);

    do {
        if (iterator == NULL || iterator->current == NULL) { /* Wrong parameter */
            KNI_ThrowNew(jsropIllegalArgumentException, "Illegal iterator");
            break;
        }
        /* finding next item in the list */
        if ((p = strchr(iterator->current, ' ')) != NULL) {
            len = (int)(p - iterator->current);
        } else {
            len = strlen(iterator->current);
        }
        if (len == 0) { /* End of list */
            break;
        }
        /* is the stack buffer enough for the item? */
        if (len >= sizeof stack_string_buffer / sizeof stack_string_buffer[0]) {
            mime = MMP_MALLOC(len + 1);
            if (mime == NULL) {
                KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                break;
            }
        } else {
            mime = stack_string_buffer;
        }

        memcpy(mime, iterator->current, len);
        mime[len] = '\0';
        
        /* shift to next item in the list */
        iterator->current += len;
        while (*iterator->current == ' ') {
            iterator->current++;
        }
    } while (0);
    
    if (mime != NULL) {
        KNI_NewStringUTF(mime, stringObj);
        if (mime != stack_string_buffer) {
            MMP_FREE(mime);
        }
    }
    KNI_EndHandlesAndReturnObject(stringObj);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListContentTypesClose) {
    ListIterator *iterator;
    if ((iterator = (ListIterator *)KNI_GetParameterAsInt(1)) != NULL) {
        MMP_FREE(iterator);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsOpen) {
    javacall_media_configuration *cfg;
    javacall_media_caps *caps;
    ListIterator *iterator = NULL;
    javacall_int32 proto_mask = 0;
    javacall_bool supportDeviceProtocol = JAVACALL_FALSE;
    char *p;
    
    /* stack buffers. Trying to avoid malloc if a string is not big */
    jchar stack_string16_buffer[MAX_PROTOCOLNAME_LEN], *string16 = NULL;
    char stack_string_buffer[MAX_PROTOCOLNAME_LEN], *mime = NULL;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_GetParameterAsObject(1, stringObj);

    do {
        if (KNI_IsNullHandle(stringObj)) {
            mime = NULL;
        } else {
            int len = KNI_GetStringLength(stringObj);
            /* if the string is longer than the stack buffer try to malloc it */
            if (len >= sizeof stack_string16_buffer / sizeof stack_string16_buffer[0]) {
                string16 = MMP_MALLOC((len + 1) * sizeof *string16);
                if (string16 == NULL) {
                    KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                    break;
                }
            } else {
                string16 = stack_string16_buffer;
            }
            if (len >= sizeof stack_string_buffer / sizeof stack_string_buffer[0]) {
                mime = MMP_MALLOC(len + 1);
                if (mime == NULL) {
                    KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                    break;
                }
            } else {
                mime = stack_string_buffer;
            }
            KNI_GetStringRegion(stringObj, 0, len, string16);
            if (simple_jcharString_to_asciiString(string16, len, mime, len + 1) != JAVACALL_OK) {
                KNI_ThrowNew(jsropIllegalArgumentException, "Illegal character in MIME type name");
                break;
            }
        }
        if (javacall_media_get_configuration(&cfg) != JAVACALL_OK) {
            KNI_ThrowNew(jsropRuntimeException, "Couldn't get MMAPI configuration");
            break;
        }
        
        if ((mime == NULL || !javautil_stricmp(mime, MIME_AUDIO_MIDI)) && 
                    cfg->supportDeviceMIDI) {
            supportDeviceProtocol = JAVACALL_TRUE;
        }
        if (!supportDeviceProtocol && 
                    (mime == NULL || !javautil_stricmp(mime, MIME_AUDIO_TONE)) && 
                    cfg->supportDeviceTone) {
            supportDeviceProtocol = JAVACALL_TRUE;
        }
        /* trying to find given MIME type among caps->contentTypes */
        for (caps = cfg->mediaCaps; caps != NULL && caps->mediaFormat != NULL; caps++) {
            if (caps->wholeProtocols != 0 || caps->streamingProtocols != 0) {
                if (mime != NULL) {
                    char *s;
                    int m_len = strlen(mime);
                    
                    for (p = (char *)caps->contentTypes; p != NULL; p = s) {
                        int p_len;
                        
                        while (*p == ' ') {
                            p++;
                        }
                        if ((s = strchr(p, ' ')) != NULL) {
                            p_len = (int)(s - p);
                        } else {
                            p_len = strlen(p);
                        }
                        if (p_len == m_len && !javautil_strnicmp(mime, p, p_len)) {
                            break;
                        }
                    }
                }
                if (mime == NULL || p != NULL) {
                    proto_mask |= caps->wholeProtocols;
                    proto_mask |= caps->streamingProtocols;
                }
            }
        }
        
        if (proto_mask != 0 || supportDeviceProtocol) {
            /* some protocols were found */
            int i;
            int len = 0;
            
            /* trying to resolve protocol names: calculating needed memory */
            for (i = 0; i < sizeof protocolNames / sizeof protocolNames[0]; i++) {
                if ((protocolNames[i].proto_mask & proto_mask) != 0 && protocolNames[i].proto_name != NULL) {
                    len += strlen(protocolNames[i].proto_name) + 1; /* +1 for space char */
                }
            }
            if (supportDeviceProtocol) {
                len += strlen(DEVICE_PROTOCOL) + 1;
            }
            if (len == 0) {
                /* Protocol wasn't found in the protocol name table */
                KNI_ThrowNew(jsropRuntimeException, "Incorrect MMAPI configuration: missing protocol name");
                break;
            }
         
            iterator = (ListIterator*)MMP_MALLOC(
                sizeof *iterator + len); /* zero terminator instead of last space */
            if (iterator == NULL) {
                KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                break;
            }
        
            /* initialize the iterator */
            iterator->current = iterator->list;
        
            /* building the list of protocols */
            p = iterator->list;
            for (i = 0; i < sizeof protocolNames / sizeof protocolNames[0]; i++) {
                if ((protocolNames[i].proto_mask & proto_mask) != 0 && protocolNames[i].proto_name != NULL) {
                    int proto_len = strlen(protocolNames[i].proto_name);
                    memcpy(p, protocolNames[i].proto_name, proto_len);
                    p += proto_len;
                    *p++ = ' ';
                }
            }
            if (supportDeviceProtocol) {
                int proto_len = strlen(DEVICE_PROTOCOL);
                memcpy(p, DEVICE_PROTOCOL, proto_len);
                p += proto_len;
                *p++ = ' ';
            }
            p--; *p = '\0'; /* replace last space with zero */
        } else {
            /* No protocols were found for provided MIME type. Return 0 */
            break;
        }
        mmapi_string_delete_duplicates(iterator->list);
    } while (0);

    /* freeing buffers */
    if (mime != NULL && mime != stack_string_buffer) {
        MMP_FREE(mime);
    }
    if (string16 != NULL && string16 != stack_string16_buffer) {
        MMP_FREE(string16);
    }
    KNI_EndHandles();
    KNI_ReturnInt((jint)iterator); 
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsNext) {
    ListIterator *iterator;
    char *p;
    int len;
    char stack_string_buffer[MAX_PROTOCOLNAME_LEN], *proto = NULL;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    iterator = (ListIterator *)KNI_GetParameterAsInt(1);
    KNI_ReleaseHandle(stringObj);

    do {
        if (iterator == NULL || iterator->current == NULL) { /* Wrong parameter */
            KNI_ThrowNew(jsropIllegalArgumentException, "Illegal iterator");
            break;
        }
        /* finding next item in the list */
        if ((p = strchr(iterator->current, ' ')) != NULL) {
            len = (int)(p - iterator->current);
        } else {
            len = strlen(iterator->current);
        }
        if (len == 0) { /* End of list */
            break;
        }
        
        /* is the stack buffer enough for the item? */
        if (len >= sizeof stack_string_buffer / sizeof stack_string_buffer[0]) {
            proto = MMP_MALLOC(len + 1);
            if (proto == NULL) {
                KNI_ThrowNew(jsropOutOfMemoryError, NULL);
                break;
            }
        } else {
            proto = stack_string_buffer;
        }

        /* shift to next item in the list */
        memcpy(proto, iterator->current, len);
        proto[len] = '\0';
        iterator->current += len;
        while (*iterator->current == ' ') {
            iterator->current++;
        }
    } while (0);
    
    if (proto != NULL) {
        KNI_NewStringUTF(proto, stringObj);
        if (proto != stack_string_buffer) {
            MMP_FREE(proto);
        }
    }
    KNI_EndHandlesAndReturnObject(stringObj);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DefaultConfiguration_nListProtocolsClose) {
    ListIterator *iterator;
    if ((iterator = (ListIterator *)KNI_GetParameterAsInt(1)) != NULL) {
        MMP_FREE(iterator);
    }
    KNI_ReturnVoid();
}

/* Delete duplicates */
void mmapi_string_delete_duplicates(char *p) {
    do {
        char *s, *s0;
        int p_len, s_len;
        
        while (*p == ' ') {
            p++;
        }
        if (*p == '\0' || (s = strchr(p, ' ')) == NULL || (p_len = (int)(s - p)) == 0) {
            break;
        }
        s++;
        do {
            s0 = s;
            if ((s = strchr(s0, ' ')) != NULL) {
                s_len = (int)(s - s0);
                while (*s == ' ') {
                    s++;
                }
            } else {
                s_len = strlen(s0);
            }
            if (s_len == p_len && !javautil_strnicmp(p, s0, s_len)) {
                s0--;
                s_len++;
                if (s == NULL) {
                    *s0 = 0;
                } else {
                    s = s0;
                    memmove(s0, s0+s_len, strlen(s0) - s_len + 1);
                }
            }
        } while (s != NULL && *s != '\0');
        p += p_len;
    } while (1);
}

/* Convert 16-bit string into 8-bit string. 
   Source string must contain only ASCII chars */
static javacall_result simple_jcharString_to_asciiString(
                    jchar *jcharString, 
                    jsize jcharStringLen, 
                    char *asciiStringBuffer, 
                    jsize bufferSize) {

    bufferSize--;
    while (bufferSize-- > 0 && jcharStringLen-- > 0) {
        if ((javacall_int32)*jcharString > 0x7F) {
            return JAVACALL_FAIL;
        }
        *asciiStringBuffer++ = (char)*jcharString++;
    }
    *asciiStringBuffer++ = '\0';
    return JAVACALL_OK;
}

