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

#include "lime.h" // Lime has to be the first include for some reason

#include "multimedia.h"
#include "javautil_unicode.h"
#include "javacall_memory.h"
#include <stdio.h>
#include "jpegencoder.h"
#include "pngencoder.h"

#define LIME_MMAPI_PACKAGE      "com.sun.mmedia"
#define LIME_MMAPI_CLASS        "JavaCallBridge"

#define AUDIO_CAPTURE_LOCATOR   L"capture://audio"
#define VIDEO_CAPTURE_LOCATOR   L"capture://video"
#define RADIO_CAPTURE_LOCATOR   L"capture://radio"
#define DEVICE_TONE_LOCATOR     L"device://tone"
#define DEVICE_MIDI_LOCATOR     L"device://midi"
#define RTSP_PROTOCOL_PREFIX    L"rtsp://"

#ifdef ENABLE_EXTRA_CAMERA_CONTROLS
void extra_camera_controls_init( javacall_impl_player * player );
void extra_camera_controls_cleanup( javacall_impl_player * player );
#endif //ENABLE_EXTRA_CAMERA_CONTROLS

static javacall_media_caps g_caps[] = 
{
//    mediaFormat,                   contentTypes,           'whole' protocols,              streaming protocols
    { JAVACALL_MEDIA_FORMAT_MS_PCM,  "audio/x-wav audio/wav",             JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_MIDI,    "audio/midi audio/mid audio/x-midi", JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_SP_MIDI, "audio/sp-midi",                     JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_TONE,    "audio/x-tone-seq audio/tone",       JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
#ifdef ENABLE_JSR_135_DSHOW
    { JAVACALL_MEDIA_FORMAT_FLV,     "video/x-flv",                       0, JAVACALL_MEDIA_MEMORY_PROTOCOL },
#endif //ENABLE_JSR_135_DSHOW
    { JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO, "audio/x-wav",                 JAVACALL_MEDIA_CAPTURE_PROTOCOL, 0 },
#ifdef ENABLE_AMR
    { JAVACALL_MEDIA_FORMAT_AMR,     "audio/amr",                         JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
#endif // ENABLE_AMR
    { JAVACALL_MEDIA_FORMAT_CAPTURE_RADIO, "audio/x-wav",                 JAVACALL_MEDIA_CAPTURE_PROTOCOL, 0 },
    { NULL,                          NULL,                   0,                              0 }
};

static int nCaps;

#ifdef ENABLE_MMAPI_LIME

#include <stdlib.h> // for itoa()
#include "javacall_properties.h"
#include <string.h>

struct _cap_item {
    javacall_media_caps *cap;
    struct _cap_item *next;
};

typedef struct _cap_item cap_item;

#define DEFAULT_VALUE_LEN       0xFF
#define MEDIA_CAPS_SIZE         sizeof(javacall_media_caps)

static javacall_media_format_type streamable_fmt[] = 
{
    JAVACALL_MEDIA_FORMAT_VIDEO_3GPP,
    JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3
};

static javacall_bool is_streamable(javacall_media_format_type fmt)
{
    int i;
    for (i = 0; i < sizeof(streamable_fmt) / sizeof(javacall_media_format_type); i++)
    {
        if (streamable_fmt[i] == fmt)
        {
            return JAVACALL_TRUE;
        }
    }
    return JAVACALL_FALSE;
}

static javacall_media_caps nullCap = { NULL, NULL, 0, 0 };

javacall_media_caps* get_cap_item(const char *strItem) {
    int mediaFormatLen, contentTypesLen;
    javacall_media_caps *cap;
    
    char *mediaFormat;
    char *contentTypes;
    
    char *delimiter = strchr(strItem, ' ');
    
    if (*delimiter == '\0') {
        // malformed string
        return &nullCap;
    }
    
    cap = (javacall_media_caps *) javacall_malloc(MEDIA_CAPS_SIZE);
    mediaFormatLen = delimiter - strItem;
    contentTypesLen = strlen(strItem) - mediaFormatLen - 1;
    mediaFormat = (char *) javacall_malloc(mediaFormatLen + 1);
    contentTypes = (char *) javacall_malloc(contentTypesLen + 1);
    
    strncpy(mediaFormat, strItem, mediaFormatLen);
    strncpy(contentTypes, strItem + mediaFormatLen + 1, contentTypesLen);
    mediaFormat[mediaFormatLen] = '\0';
    contentTypes[contentTypesLen] = '\0';
    
    cap->mediaFormat = mediaFormat;
    cap->contentTypes = contentTypes;
    cap->wholeProtocols = (strstr(mediaFormat, "CAPTURE") != NULL) ?
        JAVACALL_MEDIA_CAPTURE_PROTOCOL : JAVACALL_MEDIA_MEMORY_PROTOCOL;
    cap->streamingProtocols = 0;
    
    return cap;
}

javacall_media_caps* list2array(cap_item *head, const int count) {
    
    int i;
    cap_item *pos;
    
    javacall_media_caps *caps_array = (javacall_media_caps *) 
            javacall_malloc (MEDIA_CAPS_SIZE * (count+1));
    
    for (i = 0; (i < count) && (head != NULL); i++) {
        memcpy(&(caps_array[i]), head->cap, MEDIA_CAPS_SIZE);
        pos = head;
        head = head->next;
        free(pos);
    }
    
    memcpy(&(caps_array[i]), &nullCap, MEDIA_CAPS_SIZE);
    return caps_array;
}

javacall_media_caps *get_capabilities_from_properties() {
    
    static const char* prefix = "mmapi.content";
    
    char key[24]; // 13 for the prefix + 10 for the number + '\0'
    char number[11]; // 10 decimal ciphers (2^32)
    char *value;

    cap_item* item;
    cap_item* head = NULL;
    int i = 0;
    
    strcpy(key, prefix);
    value = (char *) javacall_malloc(DEFAULT_VALUE_LEN);
    while (1) {
        itoa(i, number, 10);
        strcpy(key + strlen(prefix), number);
        key[strlen(prefix) + strlen(number)] = '\0';

        if (JAVACALL_FAIL == 
                 javacall_get_property(key, JAVACALL_INTERNAL_PROPERTY, &value)) {
            break;
        }

        item = (cap_item *) javacall_malloc(sizeof(cap_item));
        item->cap = get_cap_item(value);
        item->next = head;
        head = item;
        i++;
    }

    free(value);
    
    nCaps = i;
    
    return list2array(head, i);
}

#endif // of ENABLE_MMAPI_LIME

void mmSetStatusLine( const char* fmt, ... ) {
    char           str8[ 256 ];
    wchar_t        str16[ 256 ];
    int            str16_len;
    va_list        args;
    javacall_int64 res;

    static LimeFunction* f = NULL;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);
    if (JAVACALL_OK == 
        javautil_unicode_utf8_to_utf16(str8, (javacall_int32)strlen(str8), 
                                        str16, 256, &str16_len)) {

        if( NULL == f ) {
            f = NewLimeFunction( LIME_MMAPI_PACKAGE,
                                 LIME_MMAPI_CLASS,
                                 "put_status_string" );
        }

        f->call( f, &res, str16, str16_len );
    }
}

//=============================================================================

static javacall_media_configuration g_cfg;

javacall_media_caps* get_capabilities() {

    static javacall_media_caps *caps = NULL;

    if (caps == NULL) {
    #ifdef ENABLE_MMAPI_LIME
        caps = get_capabilities_from_properties();
    #else
        caps = g_caps;
        nCaps = sizeof g_caps / sizeof g_caps[0] - 1;
    #endif
    }

    return caps;
}

javacall_result javacall_media_get_configuration(const javacall_media_configuration** cfg)
{
    g_cfg.audioEncoding         = "encoding=pcm&rate=22050&bits=16&channels=1";
    g_cfg.videoEncoding         = "encoding=rgb565";
    g_cfg.videoSnapshotEncoding = "encoding=jpeg&quality=80 encoding=png";

    g_cfg.supportMixing         = JAVACALL_TRUE;
    g_cfg.supportRecording      = JAVACALL_TRUE;
    g_cfg.supportDeviceTone     = JAVACALL_TRUE;
    g_cfg.supportDeviceMIDI     = JAVACALL_TRUE;
    g_cfg.supportCaptureRadio   = JAVACALL_TRUE;

    g_cfg.mediaCaps             = get_capabilities();

    *cfg = &g_cfg;

    return JAVACALL_OK;
}

javacall_result javacall_media_initialize(void)
{
    return JAVACALL_OK;
}

javacall_result javacall_media_finalize(void)
{
    return JAVACALL_OK;
}


//=============================================================================

static javacall_media_format_type g_fmt[] = 
{
    JAVACALL_MEDIA_FORMAT_MPEG1_LAYER2      ,
    JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3      ,
    JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3_PRO  ,
    JAVACALL_MEDIA_FORMAT_MPEG2_AAC         ,
    JAVACALL_MEDIA_FORMAT_MPEG4_HE_AAC      ,
    JAVACALL_MEDIA_FORMAT_ENHANCED_AAC_PLUS ,
    JAVACALL_MEDIA_FORMAT_AMR               ,
    JAVACALL_MEDIA_FORMAT_AMR_WB            ,
    JAVACALL_MEDIA_FORMAT_AMR_WB_PLUS       ,
    JAVACALL_MEDIA_FORMAT_GSM               ,
    JAVACALL_MEDIA_FORMAT_GSM_EFR           ,
    JAVACALL_MEDIA_FORMAT_QCELP             ,
    JAVACALL_MEDIA_FORMAT_MIDI              ,
    JAVACALL_MEDIA_FORMAT_SP_MIDI           ,
    JAVACALL_MEDIA_FORMAT_TONE              ,
    JAVACALL_MEDIA_FORMAT_MS_PCM            ,
    JAVACALL_MEDIA_FORMAT_MS_ADPCM          ,
    JAVACALL_MEDIA_FORMAT_YAMAHA_ADPCM      ,
    JAVACALL_MEDIA_FORMAT_AU                ,
    JAVACALL_MEDIA_FORMAT_OGG_VORBIS        ,
    JAVACALL_MEDIA_FORMAT_REALAUDIO_8       ,
    JAVACALL_MEDIA_FORMAT_AIFF              ,
    JAVACALL_MEDIA_FORMAT_WMA_9             ,
    JAVACALL_MEDIA_FORMAT_MJPEG_DEFAULT     ,
    JAVACALL_MEDIA_FORMAT_H263              ,
    JAVACALL_MEDIA_FORMAT_H264              ,
    JAVACALL_MEDIA_FORMAT_MPEG_1            ,
    JAVACALL_MEDIA_FORMAT_MPEG_2            ,
    JAVACALL_MEDIA_FORMAT_MPEG_4_SVP        ,
    JAVACALL_MEDIA_FORMAT_MPEG_4_AVC        ,
    JAVACALL_MEDIA_FORMAT_REALVIDEO_8       ,
    JAVACALL_MEDIA_FORMAT_WMV_9             ,
    JAVACALL_MEDIA_FORMAT_AUDIO_3GPP        ,
    JAVACALL_MEDIA_FORMAT_VIDEO_3GPP        ,
    JAVACALL_MEDIA_FORMAT_AVI               ,
    JAVACALL_MEDIA_FORMAT_MOV               ,
    JAVACALL_MEDIA_FORMAT_FLV               ,
    JAVACALL_MEDIA_FORMAT_JPEG              ,
    JAVACALL_MEDIA_FORMAT_JPEG2000          ,
    JAVACALL_MEDIA_FORMAT_TIFF              ,
    JAVACALL_MEDIA_FORMAT_PNG               ,
    JAVACALL_MEDIA_FORMAT_GIF               ,
    JAVACALL_MEDIA_FORMAT_RGB888            ,
    JAVACALL_MEDIA_FORMAT_RGBA8888          ,
    JAVACALL_MEDIA_FORMAT_GRAY1             ,
    JAVACALL_MEDIA_FORMAT_GRAY8             ,
    JAVACALL_MEDIA_FORMAT_DEVICE_TONE       ,
    JAVACALL_MEDIA_FORMAT_DEVICE_MIDI       ,
    JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO     ,
    JAVACALL_MEDIA_FORMAT_CAPTURE_RADIO     ,
    JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO     ,
    JAVACALL_MEDIA_FORMAT_RTP_DVI4          ,      
    JAVACALL_MEDIA_FORMAT_RTP_G722          ,
    JAVACALL_MEDIA_FORMAT_RTP_G723          ,
    JAVACALL_MEDIA_FORMAT_RTP_G726_16       ,
    JAVACALL_MEDIA_FORMAT_RTP_G726_24       ,
    JAVACALL_MEDIA_FORMAT_RTP_G726_32       ,
    JAVACALL_MEDIA_FORMAT_RTP_G726_40       ,
    JAVACALL_MEDIA_FORMAT_RTP_G728          ,
    JAVACALL_MEDIA_FORMAT_RTP_G729          ,
    JAVACALL_MEDIA_FORMAT_RTP_G729D         ,
    JAVACALL_MEDIA_FORMAT_RTP_G729E         ,
    JAVACALL_MEDIA_FORMAT_RTP_GSM           ,
    JAVACALL_MEDIA_FORMAT_RTP_GSM_EFR       ,
    JAVACALL_MEDIA_FORMAT_RTP_L8            ,
    JAVACALL_MEDIA_FORMAT_RTP_L16           ,
    JAVACALL_MEDIA_FORMAT_RTP_LPC           ,
    JAVACALL_MEDIA_FORMAT_RTP_MPA           ,
    JAVACALL_MEDIA_FORMAT_RTP_PCMA          ,
    JAVACALL_MEDIA_FORMAT_RTP_PCMU          ,
    JAVACALL_MEDIA_FORMAT_RTP_QCELP         ,
    JAVACALL_MEDIA_FORMAT_RTP_RED           ,
    JAVACALL_MEDIA_FORMAT_RTP_VDVI          ,
    JAVACALL_MEDIA_FORMAT_RTP_BT656         ,
    JAVACALL_MEDIA_FORMAT_RTP_CELB          ,
    JAVACALL_MEDIA_FORMAT_RTP_JPEG          ,
    JAVACALL_MEDIA_FORMAT_RTP_H261          ,
    JAVACALL_MEDIA_FORMAT_RTP_H263          ,
    JAVACALL_MEDIA_FORMAT_RTP_H263_1998     ,
    JAVACALL_MEDIA_FORMAT_RTP_H263_2000     ,
    JAVACALL_MEDIA_FORMAT_RTP_MPV           ,
    JAVACALL_MEDIA_FORMAT_RTP_MP2T          ,
    JAVACALL_MEDIA_FORMAT_RTP_MP1S          ,
    JAVACALL_MEDIA_FORMAT_RTP_MP2P          ,
    JAVACALL_MEDIA_FORMAT_RTP_BMPEG         ,
    JAVACALL_MEDIA_FORMAT_RTP_NV            ,
    //JAVACALL_MEDIA_FORMAT_UNKNOWN excluded, it will be mapped to -1
    JAVACALL_MEDIA_FORMAT_UNSUPPORTED
};

static const int g_fmt_count = sizeof( g_fmt ) / sizeof( g_fmt[ 0 ] );

jc_fmt fmt_str2enum( javacall_media_format_type fmt )
{
    int n;

    JC_MM_ASSERT( JC_FMT_UNSUPPORTED == g_fmt_count - 1 );

    for( n = 0; n < g_fmt_count; n++ )
    {
        if( 0 == strcmp( fmt, g_fmt[ n ] ) )
        { 
            return (jc_fmt)n;
        }
    }

    return JC_FMT_UNKNOWN;
}

javacall_media_format_type fmt_enum2str( jc_fmt fmt )
{
    JC_MM_ASSERT( JC_FMT_UNSUPPORTED == g_fmt_count - 1 );

    if( JC_FMT_UNKNOWN == fmt ) return JAVACALL_MEDIA_FORMAT_UNKNOWN;

    JC_MM_ASSERT( fmt > 0 && fmt < g_fmt_count );

    return g_fmt[ fmt ];
}

javacall_media_format_type fmt_mime2str( const char* mime )
{
    int          idx;
    unsigned int mimelen = (unsigned int)strlen( mime );
    const char*  ct;
    const char*  semicol_pos = strchr( mime, ';' );

    if( NULL != semicol_pos ) mimelen = (unsigned int)(semicol_pos - mime);

    for( idx = 0; idx < nCaps; idx++ )
    {
        ct = get_capabilities()[ idx ].contentTypes;

        while( NULL != ct && strlen( ct ) >= mimelen )
        {
            if( '\0' == ct[ mimelen ] || ' ' == ct[ mimelen ] )
            {
                if( 0 == _strnicmp( ct, mime, mimelen ) )
                    return get_capabilities()[ idx ].mediaFormat;
            }
            ct = strchr( ct, ' ' );
            if( NULL != ct ) ct++;
        }
    }

    return JAVACALL_MEDIA_FORMAT_UNKNOWN;
}

javacall_result fmt_str2mime(
        javacall_media_format_type fmt, char *buf, int buf_len) {
    
    int i;
    for (i = 0; i < nCaps; i++) {
        if (!strcmp(fmt, get_capabilities()[i].mediaFormat)) {
            const char *s = get_capabilities()[i].contentTypes;
            const char *p = strchr(s, ' ');
            int len;
            
            if (p == NULL) {
                len = (int)strlen(s);
            }
            else {
                len = (int)(p - s);
            }
            
            if (len >= buf_len) {
                return JAVACALL_FAIL;
            }
            
            memcpy(buf, s, len);
            buf[len] = '\0';
            return JAVACALL_OK;
        }
    }
    
    return JAVACALL_FAIL;
} 

//=============================================================================

/**
* Get integer parameter from utf16 string ( in the form "param=value" )
*/
javacall_result get_int_param(javacall_const_utf16_string ptr, 
                              javacall_const_utf16_string paramName, 
                              int * value)
{
    javacall_result result = JAVACALL_INVALID_ARGUMENT;

    if ((ptr != NULL) && (paramName != NULL) && (value != NULL)) {

        /// Here is supposed, that sizeof(utf16) == sizeof(wchar_t)
        /// Search position of the first entrance of paramName
        javacall_const_utf16_string pFnd = wcsstr(ptr, paramName);

        if (pFnd) {
            pFnd += wcslen(paramName);
            if (1 == swscanf(pFnd, L"=%i", value))
                result = JAVACALL_OK;
            else 
                result = JAVACALL_FAIL;
        }
    }

    return result;
}

//=============================================================================

#ifdef ENABLE_JSR_135_DSHOW
extern media_interface g_dshow_itf;
#endif // ENABLE_JSR_135_DSHOW

extern media_interface g_qsound_itf;
extern media_interface g_record_itf;
extern media_interface g_fake_radio_itf;
extern media_interface g_fake_camera_itf;
extern media_interface g_rtp_itf;

media_interface* fmt_enum2itf( jc_fmt fmt )
{
    switch( fmt )
    {

#ifdef ENABLE_JSR_135_DSHOW
    case JC_FMT_MPEG1_LAYER3:
    case JC_FMT_MPEG1_LAYER3_PRO:
    case JC_FMT_RTP_MPA:
    case JC_FMT_FLV:
    case JC_FMT_VIDEO_3GPP:
    case JC_FMT_MPEG_4_SVP:
    case JC_FMT_MPEG_1:
    case JC_FMT_AMR:
    case JC_FMT_AMR_WB:
    case JC_FMT_AMR_WB_PLUS:
    case JC_FMT_MS_PCM:
        return &g_dshow_itf;
        break;
#endif // ENABLE_JSR_135_DSHOW

    case JC_FMT_RTP_L16:
        return &g_rtp_itf;
        break;

    case JC_FMT_TONE:
    case JC_FMT_MIDI:
    case JC_FMT_SP_MIDI:
        return &g_qsound_itf;

    default:
        return NULL;
    }
}

/* Media native API interfaces */
/*****************************************************************************/

#define QUERY_BASIC_ITF(_pitf_, _method_)     \
    ( (_pitf_) && (_pitf_)->vptrBasic && (_pitf_)->vptrBasic->##_method_ )

#define QUERY_VOLUME_ITF(_pitf_, _method_)    \
    ( (_pitf_) && (_pitf_)->vptrVolume && (_pitf_)->vptrVolume->##_method_ )

#define QUERY_RATE_ITF(_pitf_, _method_)    \
    ( (_pitf_) && (_pitf_)->vptrRate && (_pitf_)->vptrRate->##_method_ )

#define QUERY_VIDEO_ITF(_pitf_, _method_)     \
    ( (_pitf_) && (_pitf_)->vptrVideo && (_pitf_)->vptrVideo->##_method_  )

#define QUERY_SNAPSHOT_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrSnapshot && (_pitf_)->vptrSnapshot->##_method_ )

#define QUERY_MIDI_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrMidi && (_pitf_)->vptrMidi->##_method_ )

#define QUERY_TONE_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrTone && (_pitf_)->vptrTone->##_method_ )

#define QUERY_METADATA_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrMetaData && (_pitf_)->vptrMetaData->##_method_ )

#define QUERY_PITCH_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrPitch && (_pitf_)->vptrPitch->_method_ )

#define QUERY_TEMPO_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrTempo && (_pitf_)->vptrTempo->_method_ )

#define QUERY_RECORD_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrRecord && (_pitf_)->vptrRecord->##_method_ )

#define QUERY_FPOSITION_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrFposition && (_pitf_)->vptrFposition->##_method_ )

/*****************************************************************************/

javacall_media_format_type fmt_guess_from_url(javacall_const_utf16_string uri, 
                                                  long uriLength)
{
    static const struct
    {
        javacall_const_utf16_string ext;
        javacall_media_format_type  fmt;
    } map[] =
    {
        { L".mid",  JAVACALL_MEDIA_FORMAT_MIDI   },
        { L".midi", JAVACALL_MEDIA_FORMAT_MIDI   },
        { L".jts",  JAVACALL_MEDIA_FORMAT_TONE   },

#if defined(ENABLE_JSR_135_DSHOW)
        { L".amr",  JAVACALL_MEDIA_FORMAT_AMR    },
        { L".wav",  JAVACALL_MEDIA_FORMAT_MS_PCM },
        { L".mp3",  JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3 },
        { L".flv",  JAVACALL_MEDIA_FORMAT_FLV },
        { L".fxm",  JAVACALL_MEDIA_FORMAT_FLV },
        { L".3gp",  JAVACALL_MEDIA_FORMAT_VIDEO_3GPP   },
        { L".3g2",  JAVACALL_MEDIA_FORMAT_VIDEO_3GPP   },
        { L".mp4",  JAVACALL_MEDIA_FORMAT_MPEG_4_SVP   },
        { L".mpeg", JAVACALL_MEDIA_FORMAT_MPEG_1       },
        { L".mpg",  JAVACALL_MEDIA_FORMAT_MPEG_1       },
        { L".mov",  JAVACALL_MEDIA_FORMAT_MOV          },
#endif

        { L".gif",  JAVACALL_MEDIA_FORMAT_UNSUPPORTED   },
        { L".wmv",  JAVACALL_MEDIA_FORMAT_UNSUPPORTED   }
    };

    int i, extlen;
    javacall_const_utf16_string tail;

    // do not guess formats for RTSP, it will be discovered on realize stage
    if( 0 == _wcsnicmp( uri, RTSP_PROTOCOL_PREFIX, 
                        min( (long)wcslen( RTSP_PROTOCOL_PREFIX ), uriLength ) ) )
    {
        return JAVACALL_MEDIA_FORMAT_UNKNOWN;
    }

    for( i = 0; i < sizeof( map ) / sizeof( map[ 0 ] ); i++ )
    {
        extlen = (int)wcslen( map[ i ].ext );

        if( uriLength > extlen )
        {
            tail = uri + uriLength - extlen;

            if( 0 == _wcsnicmp( tail, map[ i ].ext, extlen ) )
            {
                return map[ i ].fmt;
            }
        }
    }

    return JAVACALL_MEDIA_FORMAT_UNKNOWN;
}

javacall_result javacall_media_get_event_data(javacall_handle handle, 
                    int eventType, void *pResult, int numArgs, void *args[])
{
    return JAVACALL_INVALID_ARGUMENT;
}

/* Creation/destruction, format, controls *******************************************/

static void create_player_thread( void* param )
{
    javacall_result       res;
    javacall_impl_player* pPlayer = (javacall_impl_player*)param;

    pPlayer->mediaHandle = NULL;
    res = pPlayer->mediaItfPtr->vptrBasic->create( pPlayer );

    if( JAVACALL_OK != res )
    {
        if( NULL != pPlayer->uri  ) FREE( pPlayer->uri  );
        if( NULL != pPlayer->mime ) FREE( pPlayer->mime );
        FREE( pPlayer );
        pPlayer = NULL;
    }

    javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_CREATE_FINISHED,
                                     p->appId,
                                     p->playerId, 
                                     res, pPlayer );
)

javacall_result javacall_media_create_managed_player(
    javacall_int32              app_id,
    javacall_int32              player_id,
    javacall_int32              locator_len,
    javacall_const_utf16_string locator)
{
    javacall_impl_player* pPlayer 
        = (javacall_impl_player*)MALLOC(sizeof(javacall_impl_player));

    pPlayer->appId            = app_id;
    pPlayer->playerId         = player_id;
    pPlayer->downloadByDevice = JAVACALL_TRUE;
    pPlayer->mime             = NULL;
    pPlayer->streamLen        = -1;

    if( 0 == _wcsnicmp( locator, AUDIO_CAPTURE_LOCATOR, 
                       min( (long)wcslen( AUDIO_CAPTURE_LOCATOR ), locator_len ) ) )
    {
        pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO;
        pPlayer->mediaItfPtr      = &g_record_itf;
    }
    else if( 0 == _wcsnicmp( locator, VIDEO_CAPTURE_LOCATOR, 
                       min( (long)wcslen( VIDEO_CAPTURE_LOCATOR ), locator_len ) ) )
    {
        pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO;
        pPlayer->mediaItfPtr      = &g_fake_camera_itf;

        #ifdef ENABLE_EXTRA_CAMERA_CONTROLS
        pPlayer->pExtraCC         = NULL;
        extra_camera_controls_init( pPlayer );
        #endif //ENABLE_EXTRA_CAMERA_CONTROLS
    }
    else if( 0 == _wcsnicmp( locator, RADIO_CAPTURE_LOCATOR, 
                       min( (long)wcslen( RADIO_CAPTURE_LOCATOR ), locator_len ) ) )
    {
        pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_CAPTURE_RADIO;
        pPlayer->mediaItfPtr      = &g_fake_radio_itf;
    }
    else if( 0 == _wcsnicmp( locator, DEVICE_TONE_LOCATOR, 
                       min( (long)wcslen( DEVICE_TONE_LOCATOR ), locator_len ) ) )
    {
        pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_DEVICE_TONE;
        pPlayer->mediaItfPtr      = &g_qsound_itf;
    }
    else if( 0 == _wcsnicmp( locator, DEVICE_MIDI_LOCATOR, 
                       min( (long)wcslen( DEVICE_MIDI_LOCATOR ), locator_len ) ) )
    {
        pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_DEVICE_MIDI;
        pPlayer->mediaItfPtr      = &g_qsound_itf;
    }

    if( NULL != pPlayer->mediaItfPtr )
    {
        pPlayer->uri = MALLOC( (locator_len + 1) * sizeof(javacall_utf16) );
        memcpy( pPlayer->uri, locator, locator_len * sizeof(javacall_utf16) );
        pPlayer->uri[ locator_len ] = (javacall_utf16)0;

        JC_MM_ASSERT( QUERY_BASIC_ITF(pPlayer->mediaItfPtr, create) );

        _beginthread( create_player_thread, 0, pPlayer );

        return JAVACALL_OK;
    }
    else
    {
        FREE( pPlayer );
        return JAVACALL_FAIL;
    }
}


javacall_result javacall_media_create_unmanaged_player(
    javacall_int32              app_id,
    javacall_int32              player_id,
    javacall_int32              locator_len,
    javacall_const_utf16_string locator,
    javacall_int32              mime_len,
    javacall_const_utf16_string mime,
    javacall_bool               stream_len_known,
    javacall_int64              stream_len )
{
    char* cmime;

    javacall_impl_player* pPlayer 
        = (javacall_impl_player*)MALLOC(sizeof(javacall_impl_player));

    pPlayer->appId            = app_id;
    pPlayer->playerId         = player_id;
    pPlayer->downloadByDevice = JAVACALL_FALSE;
    pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_UNKNOWN;
    pPlayer->mediaItfPtr      = NULL;
    pPlayer->uri              = NULL;
    pPlayer->streamLen        = (JAVACALL_TRUE==stream_len_known) ? stream_len : -1;

    if( NULL != locator )
    {
        pPlayer->uri = MALLOC( (locator_len + 1) * sizeof(javacall_utf16) );
        memcpy( pPlayer->uri, locator, locator_len * sizeof(javacall_utf16) );
        pPlayer->uri[ locator_len ] = (javacall_utf16)0;
    }

    if( NULL != mime )
    {
        pPlayer->mime = MALLOC( (mime_len + 1) * sizeof(javacall_utf16) );
        memcpy( pPlayer->mime, mime, mime_len * sizeof(javacall_utf16) );
        pPlayer->mime[ mime_len ] = (javacall_utf16)0;
    }

    if( NULL != pPlayer->uri )
        pPlayer->mediaType   = fmt_guess_from_url( uri, uriLength );
        pPlayer->mediaItfPtr = fmt_enum2itf( fmt_str2enum(pPlayer->mediaType) );
    }

    if( NULL == pPlayer->mediaItfPtr && NULL != pPlayer->mime )
    {
        JC_MM_ASSERT( mime_len > 0 );

        cmime = MALLOC( mime_len + 1 );

        if( NULL != cmime )
        {
            /* Implementation Note: 
             * unsafe, mime must contain only ASCII chars. 
             * NEED REVISIT
             */
            int wres = WideCharToMultiByte( CP_ACP, 0, mime, mime_len,
                                            cmime, mime_len + 1, NULL, NULL );
            if( wres )
            {
                cmime[ mime_len ] = '\0';

                pPlayer->mediaType   = fmt_mime2str( cmime );
                pPlayer->mediaItfPtr = fmt_enum2itf( fmt_str2enum(pPlayer->mediaType) );
            }

            FREE( cmime );
        }
    }

    if( NULL != pPlayer->mediaItfPtr )
    {
        JC_MM_ASSERT( QUERY_BASIC_ITF(pPlayer->mediaItfPtr, create) );
        _beginthread( create_player_thread, 0, pPlayer );

        return JAVACALL_OK;
    }
    else
    {
        if( NULL != pPlayer->uri  ) FREE( pPlayer->uri  );
        if( NULL != pPlayer->mime ) FREE( pPlayer->mime );
        FREE( pPlayer );
        return JAVACALL_FAIL;
    }
}

javacall_result javacall_media_destroy(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    #ifdef ENABLE_EXTRA_CAMERA_CONTROLS
    if( JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO == pPlayer->mediaType )
    {
        extra_camera_controls_cleanup( pPlayer );
        pPlayer->pExtraCC = NULL;
    }
    #endif //ENABLE_EXTRA_CAMERA_CONTROLS

    if (QUERY_BASIC_ITF(pItf, destroy)) {
        ret = pItf->vptrBasic->destroy(pPlayer->mediaHandle);
    }

    if( NULL != pPlayer )
    {
        if( NULL != pPlayer->uri  ) FREE( pPlayer->uri  );
        if( NULL != pPlayer->mime ) FREE( pPlayer->mime );
        FREE( pPlayer );
    }

    return ret;
}

javacall_result javacall_media_get_format(javacall_handle handle, 
                              javacall_media_format_type /*OUT*/*format)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    jc_fmt fmt = JC_FMT_UNKNOWN;

    if (QUERY_BASIC_ITF(pItf, get_format)) {
        ret = pItf->vptrBasic->get_format(pPlayer->mediaHandle, &fmt);
        if( JAVACALL_OK == ret ) {
            *format = fmt_enum2str( fmt );
        }
    } else {
        *format = pPlayer->mediaType;
        ret = JAVACALL_OK;
    }

    return ret;
}

javacall_result javacall_media_get_player_controls(javacall_handle handle,
                              int /*OUT*/*controls)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_player_controls)) {
        ret = pItf->vptrBasic->get_player_controls(pPlayer->mediaHandle,controls);
    } else {
        *controls = 0;
        if(NULL!=pItf->vptrVolume)    *controls |= JAVACALL_MEDIA_CTRL_VOLUME;
        if(NULL!=pItf->vptrVideo )    *controls |= JAVACALL_MEDIA_CTRL_VIDEO;
        //if(NULL!=pItf->vptrSnapshot )
        if(NULL!=pItf->vptrMidi  )    *controls |= JAVACALL_MEDIA_CTRL_MIDI;
        if(NULL!=pItf->vptrMetaData)  *controls |= JAVACALL_MEDIA_CTRL_METADATA;
        if(NULL!=pItf->vptrRate  )    *controls |= JAVACALL_MEDIA_CTRL_RATE;
        if(NULL!=pItf->vptrTempo)     *controls |= JAVACALL_MEDIA_CTRL_TEMPO;
        if(NULL!=pItf->vptrPitch)     *controls |= JAVACALL_MEDIA_CTRL_PITCH;
        if(NULL!=pItf->vptrRecord)    *controls |= JAVACALL_MEDIA_CTRL_RECORD;
        if(NULL!=pItf->vptrFposition) *controls |= JAVACALL_MEDIA_CTRL_FRAME_POSITIONING;
        //if(NULL!=pItf->vptr) *controls |= JAVACALL_MEDIA_CTRL_STOPTIME;
        //if(NULL!=pItf->vptr) *controls |= JAVACALL_MEDIA_CTRL_TONE;
        return JAVACALL_OK;
    }

    return ret;
}

/* State management *****************************************************************/

javacall_result javacall_media_stop(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, stop)) {
        ret = pItf->vptrBasic->stop(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_pause(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, pause)) {
        ret = pItf->vptrBasic->pause(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_run(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, run)) {
        ret = pItf->vptrBasic->run(pPlayer->mediaHandle);
    }

    return ret;
}

/* Data streaming *******************************************************************/

javacall_result javacall_media_stream_length(
    javacall_handle handle,
    javacall_int64 length)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, stream_length)) {
        ret = pItf->vptrBasic->stream_length(pPlayer->mediaHandle,length);
    }

    return ret;
}

javacall_result javacall_media_get_data_request(
    javacall_handle handle,
    /*OUT*/ javacall_int64 *offset,
    /*OUT*/ javacall_int32 *length)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_data_request)) {
        ret = pItf->vptrBasic->get_data_request(pPlayer->mediaHandle,
                                                offset,
                                                length);
    }

    return ret;
}

javacall_result javacall_media_data_ready(
    javacall_handle handle,
    javacall_int32 length,
    /*OUT*/ void **data)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, data_ready)) {
        ret = pItf->vptrBasic->data_ready(pPlayer->mediaHandle,
                                          length,
                                          data);
    }

    return ret;
}

javacall_result javacall_media_data_written(
    javacall_handle handle,
    /*OUT*/ javacall_bool *new_request)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, data_written)) {
        ret = pItf->vptrBasic->data_written(pPlayer->mediaHandle,
                                            new_request);
    }

    return ret;
}

/* Media time and duration **********************************************************/

javacall_result javacall_media_get_media_time(javacall_handle handle, long* ms)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_time)) {
        return pItf->vptrBasic->get_time(pPlayer->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

javacall_result javacall_media_set_media_time(javacall_handle handle, long ms)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_time)) {
        return pItf->vptrBasic->set_time(pPlayer->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_duration(javacall_handle handle, long* ms)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_duration)) {
        return pItf->vptrBasic->get_duration(pPlayer->mediaHandle, ms);
    }
    return JAVACALL_FAIL;
}

/* MVM Support **********************************************************************/

javacall_result javacall_media_to_foreground(const javacall_handle handle,
                                             const int appId) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_foreground)) {
        ret = pItf->vptrBasic->switch_to_foreground(pPlayer->mediaHandle, appId);
    }

    return ret;
}

javacall_result javacall_media_to_background(javacall_handle handle,
                                             const int appId) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_background)) {
        ret = pItf->vptrBasic->switch_to_background(pPlayer->mediaHandle, appId);
    }

    return ret;
}

/* VolumeControl Functions ************************************************/

javacall_result javacall_media_get_volume(javacall_handle handle, long* level)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, get_volume)) {
        ret = pItf->vptrVolume->get_volume(pPlayer->mediaHandle,level);
    }

    return ret;
}

javacall_result javacall_media_set_volume(javacall_handle handle, long* level)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_volume)) {
        ret = pItf->vptrVolume->set_volume(pPlayer->mediaHandle, level);
    }

    return ret;
}

javacall_result javacall_media_is_mute(javacall_handle handle, javacall_bool* mute )
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, is_mute)) {
        ret = pItf->vptrVolume->is_mute(pPlayer->mediaHandle, mute);
    }

    return ret;
}

javacall_result javacall_media_set_mute(javacall_handle handle, javacall_bool mute)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_mute)) {
        ret = pItf->vptrVolume->set_mute(pPlayer->mediaHandle, mute);
    }

    return ret;
}

/* VideoControl Functions ************************************************/

javacall_result javacall_media_set_video_color_key(javacall_handle handle,
                                               javacall_bool on,
                                               javacall_pixel color) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_alpha)) {
        ret = pItf->vptrVideo->set_video_alpha(pPlayer->mediaHandle, on, color);
    }

    return ret;
}

javacall_result javacall_media_get_video_size(javacall_handle handle,
                                              long* width, long* height)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, get_video_size)) {
        ret = pItf->vptrVideo->get_video_size(pPlayer->mediaHandle, width, height);
    } else {
        *width = 0;
        *height = 0;
    }

    return ret;
}

javacall_result javacall_media_set_video_visible(javacall_handle handle,
                                                 javacall_bool visible)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_visible)) {
        ret = pItf->vptrVideo->set_video_visible(pPlayer->mediaHandle, visible);
    }

    return ret;
}

javacall_result javacall_media_set_video_location(javacall_handle handle,
                                                  long x, long y, long w, long h)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_location)) {
        ret = pItf->vptrVideo->set_video_location(pPlayer->mediaHandle, x, y, w, h);
    }

    return ret;
}

javacall_result javacall_media_set_video_full_screen_mode(javacall_handle handle, javacall_bool fullScreenMode)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_fullscreenmode)) {
        ret = pItf->vptrVideo->set_video_fullscreenmode(pPlayer->mediaHandle, fullScreenMode);
    }

    return ret;
}

javacall_result javacall_media_start_video_snapshot(javacall_handle handle,
                                                    const javacall_utf16* imageType,
                                                    long length)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, start_video_snapshot)) {
        ret = pItf->vptrSnapshot->start_video_snapshot(
            pPlayer->mediaHandle, imageType, length);
    }

    return ret;
}

javacall_result javacall_media_get_video_snapshot_data_size(javacall_handle handle,
                                                            /*OUT*/ long* size)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, get_video_snapshot_data_size)) {
        ret = pItf->vptrSnapshot->get_video_snapshot_data_size(
            pPlayer->mediaHandle, size);
    }

    return ret;
}

javacall_result javacall_media_get_video_snapshot_data(javacall_handle handle,
                                                       /*OUT*/ char* buffer,
                                                       long size)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, get_video_snapshot_data)) {
        ret = pItf->vptrSnapshot->get_video_snapshot_data(
            pPlayer->mediaHandle, buffer, size);
    }

    return ret;
}

/* Simple Tone Play Functions */
/*****************************************************************************/

extern int mmaudio_tone_note(long isolateId, long note, long duration, long volume);

javacall_result javacall_media_play_tone(int appId, long note, long duration, long volume){
    return mmaudio_tone_note(appId, note, duration, volume);
}

javacall_result javacall_media_play_dualtone(int appId, long noteA, long noteB, long duration, long volume)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_media_stop_tone(int appId){
    return JAVACALL_OK;
}

/* MIDIControl functions */
/*****************************************************************************/

javacall_result javacall_media_get_channel_volume(javacall_handle handle,
                                                  long channel,
                                                  /*OUT*/ long* volume) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_channel_volume)) {
        ret = pItf->vptrMidi->get_channel_volume(pPlayer->mediaHandle, channel, volume);
    }

    return ret;
}

javacall_result javacall_media_set_channel_volume(javacall_handle handle,
                                                  long channel, long volume) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_channel_volume)) {
        ret = pItf->vptrMidi->set_channel_volume(pPlayer->mediaHandle, channel, volume);
    }

    return ret;
}

javacall_result javacall_media_set_program(javacall_handle handle,
                                           long channel, long bank, long program) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_program)) {
        ret = pItf->vptrMidi->set_program(pPlayer->mediaHandle, channel, bank, program);
    }

    return ret;
}

javacall_result javacall_media_short_midi_event(javacall_handle handle,
                                                long type, long data1, long data2) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    if (QUERY_MIDI_ITF(pItf, short_midi_event)) {
        ret = pItf->vptrMidi->short_midi_event(pPlayer->mediaHandle, type, data1, data2);
    }

    return ret;
}

javacall_result javacall_media_long_midi_event(javacall_handle handle,
                                               const char* data,
                                               long offset,
                                               /*INOUT*/ long* length) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, long_midi_event)) {
        ret = pItf->vptrMidi->long_midi_event(pPlayer->mediaHandle, data, offset, length);
    }

    return ret;
}

/* Tone Control functions */
/*****************************************************************************/

javacall_result javacall_media_tone_alloc_buffer(javacall_handle handle, int length, void** ptr)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_TONE_ITF(pItf, tone_alloc_buffer)) {
        ret = pItf->vptrTone->tone_alloc_buffer(pPlayer->mediaHandle, length, ptr);
    }

    return ret;
}

javacall_result javacall_media_tone_sequence_written(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_TONE_ITF(pItf, tone_sequence_written)) {
        ret = pItf->vptrTone->tone_sequence_written(pPlayer->mediaHandle);
    }

    return ret;
}

/* Record Control functions */
/*****************************************************************************/

javacall_result
javacall_media_recording_handled_by_native(javacall_handle handle,
                                                           const javacall_utf16* locator,
                                           long locatorLength)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, recording_handled_by_native)) {
        ret = pItf->vptrRecord->recording_handled_by_native(
            pPlayer->mediaHandle, locator);
    }

    return ret;
}

javacall_result javacall_media_set_recordsize_limit(javacall_handle handle,
                                                    /*INOUT*/ long* size) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, set_recordsize_limit)) {
        ret = pItf->vptrRecord->set_recordsize_limit(pPlayer->mediaHandle, size);
    }

    return ret;
}

javacall_result javacall_media_start_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;


    if (QUERY_RECORD_ITF(pItf, start_recording)) {
        ret = pItf->vptrRecord->start_recording(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_pause_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, pause_recording)) {
        ret = pItf->vptrRecord->pause_recording(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_stop_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, stop_recording)) {
        ret = pItf->vptrRecord->stop_recording(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_reset_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, reset_recording)) {
        ret = pItf->vptrRecord->reset_recording(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_commit_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, commit_recording)) {
        ret = pItf->vptrRecord->commit_recording(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_get_recorded_data_size(javacall_handle handle,
                                                      /*OUT*/ long* size) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, get_recorded_data_size)) {
        ret = pItf->vptrRecord->get_recorded_data_size(pPlayer->mediaHandle, size);
    }

    return ret;
}

javacall_result javacall_media_get_recorded_data(javacall_handle handle,
                                                 /*OUT*/ char* buffer,
                                                 long offset, long size) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, get_recorded_data)) {
        ret = pItf->vptrRecord->get_recorded_data(
            pPlayer->mediaHandle, buffer, offset, size);
    }

    return ret;
}

javacall_result javacall_media_get_record_content_type_length(javacall_handle handle,
                                                              int* length) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};

    *length = sizeof(contentType) / sizeof(*contentType);
    return JAVACALL_OK;
}

javacall_result javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           /*INOUT*/ int* length) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};
    memcpy(contentTypeBuf, contentType, sizeof(contentType));
    *length = sizeof(contentType) / sizeof(*contentType);
    return JAVACALL_OK;
}

javacall_result javacall_media_close_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, close_recording)) {
        ret = pItf->vptrRecord->close_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/* Meta data functions ***********************************************************/

javacall_result javacall_media_get_metadata_key_counts(javacall_handle handle,
                                                       long* keyCounts)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_METADATA_ITF(pItf, get_metadata_key_counts)) {
        ret = pItf->vptrMetaData->get_metadata_key_counts(pPlayer->mediaHandle, keyCounts);
    }

    return ret;
}

javacall_result javacall_media_get_metadata_key(javacall_handle handle,
                                                long index,
                                                long bufLength,
                                                /*OUT*/ javacall_utf16* buf)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_METADATA_ITF(pItf, get_metadata_key)) {
        ret = pItf->vptrMetaData->get_metadata_key(pPlayer->mediaHandle, index, bufLength, buf);
    }

    return ret;
}

javacall_result javacall_media_get_metadata(javacall_handle handle,
                                            const javacall_utf16* key,
                                            long bufLength,
                                            javacall_utf16* buf)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_METADATA_ITF(pItf, get_metadata)) {
        ret = pItf->vptrMetaData->get_metadata(pPlayer->mediaHandle, key, bufLength, buf);
    }

    return ret;
}

/* RateControl functions ***********************************************************/

javacall_result javacall_media_get_max_rate(javacall_handle handle, long* maxRate)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_max_rate)) {
        ret = pItf->vptrRate->get_max_rate(pPlayer->mediaHandle, maxRate);
    }

    return ret;
}

javacall_result javacall_media_get_min_rate(javacall_handle handle, long* minRate)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_min_rate)) {
        ret = pItf->vptrRate->get_min_rate(pPlayer->mediaHandle, minRate);
    }

    return ret;
}

javacall_result javacall_media_set_rate(javacall_handle handle, long rate)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, set_rate)) {
        ret = pItf->vptrRate->set_rate(pPlayer->mediaHandle, rate);
    }

    return ret;
}

javacall_result javacall_media_get_rate(javacall_handle handle, long* rate)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_rate)) {
        ret = pItf->vptrRate->get_rate(pPlayer->mediaHandle, rate);
    }

    return ret;
}

/* TempoControl functions **************************************************/

javacall_result javacall_media_get_tempo(javacall_handle handle,
                                         /*OUT*/ long* tempo)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;


    if (QUERY_TEMPO_ITF(pItf, get_tempo)) {
        ret = pItf->vptrTempo->get_tempo(pPlayer->mediaHandle, tempo);
    }

    return ret;
}

javacall_result javacall_media_set_tempo(javacall_handle handle, long tempo)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_TEMPO_ITF(pItf, set_tempo)) {
        ret = pItf->vptrTempo->set_tempo(pPlayer->mediaHandle, tempo);
    }

    return ret;
}

/* PitchControl functions ******************************************************/

javacall_result javacall_media_get_max_pitch(javacall_handle handle,
                                             /*OUT*/ long* maxPitch)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_max_pitch)) {
        ret = pItf->vptrPitch->get_max_pitch(pPlayer->mediaHandle, maxPitch);
    }

    return ret;
}

javacall_result javacall_media_get_min_pitch(javacall_handle handle,
                                             /*OUT*/ long* minPitch)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_min_pitch)) {
        ret = pItf->vptrPitch->get_min_pitch(pPlayer->mediaHandle, minPitch);
    }

    return ret;
}

javacall_result javacall_media_set_pitch(javacall_handle handle, long pitch) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, set_pitch)) {
        ret = pItf->vptrPitch->set_pitch(pPlayer->mediaHandle, pitch);
    }

    return ret;
}

javacall_result javacall_media_get_pitch(javacall_handle handle,
                                         /*OUT*/ long* pitch)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_pitch)) {
        ret = pItf->vptrPitch->get_pitch(pPlayer->mediaHandle, pitch);
    }

    return ret;
}

/* MIDI Bank Query functions *******************************************/

javacall_result javacall_media_is_midibank_query_supported(javacall_handle handle, 
                                                           /*OUT*/ long* supported)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, is_bank_query_supported)) {
        ret = pItf->vptrMidi->is_bank_query_supported(
            pPlayer->mediaHandle, supported);
    }

    return ret;
}

javacall_result javacall_media_get_midibank_list(javacall_handle handle, 
                                                 long custom, 
                                                 /*OUT*/short* banklist, 
                                                 /*INOUT*/ long* numlist) 
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_bank_list)) {
        ret = pItf->vptrMidi->get_bank_list(
            pPlayer->mediaHandle, custom, banklist, numlist);
    }

    return ret;
}

javacall_result javacall_media_get_midibank_key_name(javacall_handle handle, 
                                                     long bank, long program, 
                                                     long key, 
                                                     /*OUT*/char* keyname, 
                                                     /*INOUT*/ long* keynameLen) 
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_key_name)) {
        ret = pItf->vptrMidi->get_key_name(
            pPlayer->mediaHandle, bank, program, key, keyname, keynameLen);
    }

    return ret;
}

javacall_result 
javacall_media_get_midibank_program_name(javacall_handle handle, 
                                         long bank, long program, 
                                         /*OUT*/char* progname, 
                                         /*INOUT*/ long* prognameLen) 
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_program_name)) {
        ret = pItf->vptrMidi->get_program_name(
            pPlayer->mediaHandle, bank, program, progname, prognameLen);
    }

    return ret;
}

javacall_result 
javacall_media_get_midibank_program_list(javacall_handle handle, 
                                         long bank, 
                                         /*OUT*/char* proglist, 
                                         /*INOUT*/ long* proglistLen) 
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_program_list)) {
        ret = pItf->vptrMidi->get_program_list(
            pPlayer->mediaHandle, bank, proglist, proglistLen);
    }

    return ret;
}

javacall_result javacall_media_get_midibank_program(javacall_handle handle, 
                                                    long channel, 
                                                    /*OUT*/long* prog) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_program)) {
        ret = pItf->vptrMidi->get_program(pPlayer->mediaHandle, channel, prog);
    }

    return ret;
}

/* Frame Position functions *******************************************/

javacall_result javacall_media_map_frame_to_time(javacall_handle handle,
                                                 long frameNum, /*OUT*/ long* ms) {
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, map_frame_to_time)) {
        ret = pItf->vptrFposition->map_frame_to_time(pPlayer->mediaHandle, frameNum, ms);
    }

    return ret;
}

javacall_result javacall_media_map_time_to_frame(javacall_handle handle,
                                                 long ms, /*OUT*/ long* frameNum) {
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, map_time_to_frame)) {
        ret = pItf->vptrFposition->map_time_to_frame(pPlayer->mediaHandle, ms, frameNum);
    }

    return ret;
}

javacall_result javacall_media_seek_to_frame(javacall_handle handle,
                                             long frameNum, /*OUT*/ long* actualFrameNum) {
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, seek_to_frame)) {
        ret = pItf->vptrFposition->seek_to_frame(pPlayer->mediaHandle, frameNum, actualFrameNum);
    }

    return ret;
}

javacall_result javacall_media_skip_frames(javacall_handle handle, /*INOUT*/ long* nFrames) {
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, skip_frames)) {
        ret = pItf->vptrFposition->skip_frames(pPlayer->mediaHandle, nFrames);
    }

    return ret;
}


#define JFIF_HEADER_MAXIMUM_LENGTH 1024
javacall_result javacall_media_encode_start(javacall_uint8* rgb888, 
                                      javacall_uint8 width, 
                                      javacall_uint8 height,
                                      javacall_encoder_type encode,
                                      javacall_uint8 quality,
                                      javacall_uint8** result_buffer,
                                            javacall_uint32* result_buffer_len,
                                            javacall_handle* context) {
    if (JAVACALL_JPEG_ENCODER == encode) {
        /// It's hard to suppose, how large will be jpeg image 
        int nWidth = ((width+7)&(~7));
        int nHeight = ((height+7)&(~7));
        int jpegLen = nWidth*nHeight*5 + JFIF_HEADER_MAXIMUM_LENGTH;
        *result_buffer = javacall_malloc(jpegLen);
        if (NULL != *result_buffer) {
            *result_buffer_len = RGBToJPEG(rgb888, width, height, quality, 
                                           *result_buffer, JPEG_ENCODER_COLOR_RGB);
            return (*result_buffer_len > 0) ? JAVACALL_OK : JAVACALL_FAIL;
        }
    } else if (JAVACALL_PNG_ENCODER == encode) {
        int pngLen = javautil_media_get_png_size(width, height);
        *result_buffer = javacall_malloc(pngLen);
        if (NULL != *result_buffer) {
            *result_buffer_len = javautil_media_rgb_to_png(rgb888, *result_buffer, 
                                                               width, height);
            return (*result_buffer_len > 0) ? JAVACALL_OK : JAVACALL_FAIL;
        }
    }

    return JAVACALL_OUT_OF_MEMORY;
}
#undef JFIF_HEADER_MAXIMUM_LENGTH

javacall_result javacall_media_encode_finish(javacall_handle context,
                                             javacall_uint8** result_buffer, javacall_uint32* result_buffer_len) {
    // should never be called
    return JAVACALL_FAIL;
}

void javacall_media_release_data(javacall_uint8* result_buffer, javacall_uint32 result_buffer_len) {
    javacall_free(result_buffer);
}

javacall_result javacall_media_get_system_volume(/*OUT*/ javacall_int32 *volume) {
    return JAVACALL_NO_DATA_AVAILABLE;
}

