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

//#include "lime.h"
#include "multimedia.h"
#include "mmmididev.h"

//=============================================================================

static javacall_media_caps g_caps[] = 
{
//    mediaFormat,                   contentTypes,           'whole' protocols,              streaming protocols
    { JAVACALL_MEDIA_FORMAT_MS_PCM,  "audio/x-wav audio/wav",             JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_MIDI,    "audio/midi audio/mid audio/x-midi", JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_SP_MIDI, "audio/sp-midi",                     JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_TONE,    "audio/x-tone-seq audio/tone",       JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO, "audio/x-wav",                 JAVACALL_MEDIA_CAPTURE_PROTOCOL, 0 },
#ifdef ENABLE_AMR
    { JAVACALL_MEDIA_FORMAT_AMR,     "audio/amr",                         JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
#endif // ENABLE_AMR
#ifdef ENABLE_MMAPI_LIME   
    { JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3, "audio/mpeg",      JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_MPEG_1,  "video/mpeg",           JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
    { JAVACALL_MEDIA_FORMAT_MOV,     "video/quicktime",      JAVACALL_MEDIA_MEMORY_PROTOCOL, 0 },
#endif /* ENABLE_MMAPI_LIME */    
    { NULL,                          NULL,                   0,                              0 }
};

static javacall_media_configuration g_cfg;

javacall_result javacall_media_get_configuration(const javacall_media_configuration** cfg)
{
    g_cfg.audioEncoding         = "encoding=pcm&rate=22050&bits=16&channels=1";
    g_cfg.videoEncoding         = NULL;
    g_cfg.videoSnapshotEncoding = NULL;

    g_cfg.supportMixing         = JAVACALL_TRUE;
    g_cfg.supportRecording      = JAVACALL_TRUE;
    g_cfg.supportDeviceTone     = JAVACALL_TRUE;
    g_cfg.supportDeviceMIDI     = JAVACALL_TRUE;
    g_cfg.supportCaptureRadio   = JAVACALL_FALSE;

    g_cfg.mediaCaps             = g_caps;

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
    JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO     ,
    //JAVACALL_MEDIA_FORMAT_UNKNOWN excluded, it will be mapped to -1
    JAVACALL_MEDIA_FORMAT_UNSUPPORTED
};

static const int g_fmt_count = sizeof( g_fmt ) / sizeof( g_fmt[ 0 ] );

jc_fmt fmt_str2enum( javacall_media_format_type fmt )
{
    int n;

    JC_MM_ASSERT( JC_FMT_UNSUPPORTED == g_fmt_count - 1 );

    for( n = 0; n < g_fmt_count; n++ )
        if( 0 == strcmp( fmt, g_fmt[ n ] ) ) return (jc_fmt)n;

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
    unsigned int mimelen = strlen( mime );
    const char*  ct;

    for( idx = 0; idx < sizeof( g_caps ) / sizeof( g_caps[ 0 ] ) - 1; idx++ )
    {
        ct = g_caps[ idx ].contentTypes;

        while( NULL != ct && strlen( ct ) >= mimelen )
        {
            if( '\0' == ct[ mimelen ] || ' ' == ct[ mimelen ] )
            {
                if( 0 == _strnicmp( ct, mime, mimelen ) )
                    return g_caps[ idx ].mediaFormat;
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
    for (i = 0; i < sizeof g_caps / sizeof g_caps[0] - 1; i++) {
        if (!strcmp(fmt, g_caps[i].mediaFormat)) {
            char *s = g_caps[i].contentTypes;
            char *p = strchr(s, ' ');
            int len;
            
            if (p == NULL) {
                len = strlen(s);
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

extern media_interface g_audio_itf;
extern media_interface g_qsound_itf;
extern media_interface g_amr_audio_itf;
extern media_interface g_qsound_interactive_midi_itf;
extern media_interface g_video_itf;
//extern media_interface g_tone_itf;
extern media_interface g_camera_itf;
//extern media_interface g_interactive_midi_itf;
extern media_interface g_record_itf;

media_interface* fmt_enum2itf( jc_fmt fmt )
{
    switch( fmt )
    {
#ifdef ENABLE_MMAPI_LIME
    case JC_FMT_MPEG_1:
    case JC_FMT_MPEG_4_SVP:
    case JC_FMT_MPEG_4_AVC:
    case JC_FMT_VIDEO_3GPP:
    case JC_FMT_MOV:
        return &g_video_itf;    // was: VIDEO_MPEG4, VIDEO_3GPP, CAPTURE_VIDEO, VIDEO_MPEG, VIDEO_GIF
 #endif /* ENABLE_MMAPI_LIME */

    case JC_FMT_TONE:
    case JC_FMT_MIDI:
    case JC_FMT_SP_MIDI:
    case JC_FMT_MS_PCM:
        return &g_qsound_itf;   // was: AUDIO_MIDI, AUDIO_WAVE

#ifdef ENABLE_MMAPI_LIME        
    case JC_FMT_MPEG1_LAYER3:
    case JC_FMT_MPEG1_LAYER3_PRO:
    case JC_FMT_MPEG2_AAC:
    case JC_FMT_MPEG4_HE_AAC:
        return &g_audio_itf;    // was: AUDIO_MP3, AUDIO_MPEG4, AUDIO_AAC, AUDIO_MP3_2
#endif /* ENABLE_MMAPI_LIME */

#ifdef ENABLE_AMR
    case JC_FMT_AMR:
    case JC_FMT_AMR_WB:
    case JC_FMT_AMR_WB_PLUS:
        return &g_amr_audio_itf; // was: AUDIO_AMR
#endif // ENABLE_AMR

    //case JC_FMT_TONE:
        //return &g_tone_itf;     // AUDIO_TONE

    default:
        return NULL;
    }

    // &g_record_itf,              // JAVACALL_CAPTURE_AUDIO,     /** Audio capture   */
    // &g_interactive_midi_itf,    // JAVACALL_INTERACTIVE_MIDI,  /** Interactive MIDI */
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
        { L".wav",  JAVACALL_MEDIA_FORMAT_MS_PCM },
        { L".mid",  JAVACALL_MEDIA_FORMAT_MIDI   },
        { L".midi", JAVACALL_MEDIA_FORMAT_MIDI   },
        { L".jts",  JAVACALL_MEDIA_FORMAT_TONE   },
#ifdef ENABLE_AMR
        { L".amr",  JAVACALL_MEDIA_FORMAT_AMR    },
#endif // ENABLE_AMR
#ifdef ENABLE_MMAPI_LIME
        { L".mp3",  JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3 },
        { L".mpg",  JAVACALL_MEDIA_FORMAT_MPEG_1       },
        { L".mov",  JAVACALL_MEDIA_FORMAT_MOV          },
#endif /* ENABLE_MMAPI_LIME */
        { L".gif",  JAVACALL_MEDIA_FORMAT_UNSUPPORTED   },
        { L".wmv",  JAVACALL_MEDIA_FORMAT_UNSUPPORTED   }
    };

    int i, extlen;
    javacall_const_utf16_string tail;

    for( i = 0; i < sizeof( map ) / sizeof( map[ 0 ] ); i++ )
    {
        extlen = wcslen( map[ i ].ext );

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

/**
 * Testing purpose API
 */
javacall_handle javacall_media_create2(int playerId, javacall_media_format_type mediaType,
                                       const javacall_utf16* fileName,
                                       int fileNameLength)
{
    return NULL;
}

/**
 * Native player create.
 * This function create internal information structure that will be used from other native API.
 */

#define AUDIO_CAPTURE_LOCATOR   L"capture://audio"
#define VIDEO_CAPTURE_LOCATOR   L"capture://video"
#define DEVICE_TONE_LOCATOR     L"device://tone"
#define DEVICE_MIDI_LOCATOR     L"device://midi"

javacall_result javacall_media_create(int appId,
                                      int playerId,
                                      javacall_const_utf16_string uri, 
                                      long uriLength,
                                      javacall_handle *handle)
{
    javacall_impl_player* pPlayer = NULL;

    JC_MM_DEBUG_PRINT("javacall_media_create \n");

    pPlayer = MALLOC(sizeof(javacall_impl_player));

    if( NULL == pPlayer ) return JAVACALL_OUT_OF_MEMORY;

    pPlayer->appId            = appId;
    pPlayer->playerId         = playerId;
    pPlayer->uri              = NULL;
    pPlayer->mediaHandle      = NULL;
    pPlayer->mediaItfPtr      = NULL;
    pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_UNKNOWN;
    pPlayer->downloadByDevice = JAVACALL_FALSE;    

    if( NULL != uri )
    {
        pPlayer->uri = MALLOC( (uriLength + 1) * sizeof(javacall_utf16) );
        memcpy( pPlayer->uri, uri, uriLength * sizeof(javacall_utf16) );
        pPlayer->uri[ uriLength ] = (javacall_utf16)0;

        if( 0 == _wcsnicmp( uri, AUDIO_CAPTURE_LOCATOR, 
                           min( (long)wcslen( AUDIO_CAPTURE_LOCATOR ), uriLength ) ) )
        {
            pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO;
            pPlayer->mediaItfPtr      = &g_record_itf;
            pPlayer->downloadByDevice = JAVACALL_TRUE;
        }
        else if( 0 == _wcsnicmp( uri, VIDEO_CAPTURE_LOCATOR, 
                           min( (long)wcslen( VIDEO_CAPTURE_LOCATOR ), uriLength ) ) )
        {
            pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO;
            pPlayer->mediaItfPtr      = &g_camera_itf;
            pPlayer->downloadByDevice = JAVACALL_TRUE;
        }
        else if( 0 == _wcsnicmp( uri, DEVICE_TONE_LOCATOR, 
                           min( (long)wcslen( DEVICE_TONE_LOCATOR ), uriLength ) ) )
        {
            pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_DEVICE_TONE;
            pPlayer->mediaItfPtr      = &g_qsound_itf;
            pPlayer->downloadByDevice = JAVACALL_TRUE;
        }
        else if( 0 == _wcsnicmp( uri, DEVICE_MIDI_LOCATOR, 
                           min( (long)wcslen( DEVICE_MIDI_LOCATOR ), uriLength ) ) )
        {
            pPlayer->mediaType        = JAVACALL_MEDIA_FORMAT_DEVICE_MIDI;
            pPlayer->mediaItfPtr      = &g_qsound_itf;
            pPlayer->downloadByDevice = JAVACALL_TRUE;
        }
        else
        {
            pPlayer->mediaType   = fmt_guess_from_url( uri, uriLength );
            pPlayer->mediaItfPtr = fmt_enum2itf( fmt_str2enum(pPlayer->mediaType) );
        }
    }

    if( NULL != pPlayer->mediaItfPtr )
    {
        JC_MM_ASSERT( QUERY_BASIC_ITF(pPlayer->mediaItfPtr, create) );

        pPlayer->mediaHandle =
            pPlayer->mediaItfPtr->vptrBasic->create( 
                appId, playerId, 
                fmt_str2enum(pPlayer->mediaType), 
                pPlayer->uri );

        if( NULL != pPlayer->mediaHandle )
        {
            *handle = pPlayer;
            return JAVACALL_OK;
        }
        else
        {
            FREE( pPlayer );
            return JAVACALL_FAIL;
        }
    }
    else
    {
        // format still unknown, leave it to realize()
        *handle = pPlayer;
        return JAVACALL_OK;
    }
}

javacall_result javacall_media_realize(javacall_handle handle,
                                       javacall_const_utf16_string mime,
                                       long mimeLength)
{
    javacall_result ret     = JAVACALL_FAIL;
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;
    char* cmime;

    if( 0 == strcmp( JAVACALL_MEDIA_FORMAT_UNKNOWN, pPlayer->mediaType ) )
    {
        if( NULL != mime )
        {
            JC_MM_ASSERT( mimeLength > 0 );

            cmime = MALLOC( mimeLength + 1 );

            if( NULL != cmime )
            {
                /* warning! TODO: unsafe, mime must contain only ASCII chars. */
                int wres = WideCharToMultiByte( CP_ACP, 0, mime, mimeLength,
                                                cmime, mimeLength + 1, NULL, NULL );
                if( wres )
                {
                    cmime[ mimeLength ] = '\0';
                    pPlayer->mediaType = fmt_mime2str( cmime );
                }

                FREE( cmime );
            }
        }

        if( NULL == pPlayer->mediaItfPtr && 
            0 != strcmp( JAVACALL_MEDIA_FORMAT_UNKNOWN, pPlayer->mediaType ) )
        {
            pPlayer->mediaItfPtr = fmt_enum2itf( fmt_str2enum(pPlayer->mediaType) );

            if( NULL != pPlayer->mediaItfPtr )
            {
                JC_MM_ASSERT( QUERY_BASIC_ITF(pPlayer->mediaItfPtr, create) );

                pPlayer->mediaHandle =
                    pPlayer->mediaItfPtr->vptrBasic->create( 
                    pPlayer->appId, pPlayer->playerId, 
                    fmt_str2enum(pPlayer->mediaType),
                    pPlayer->uri );

                if( NULL == pPlayer->mediaHandle )
                {
                    return JAVACALL_FAIL;
                }
            }
            else
            {
                return JAVACALL_FAIL;
            }
        }
    }
    if (NULL == pPlayer->mediaItfPtr) {
        pPlayer->mediaType = JAVACALL_MEDIA_FORMAT_UNSUPPORTED;
    }

    if( QUERY_BASIC_ITF(pPlayer->mediaItfPtr, realize) )
    {
        ret = pPlayer->mediaItfPtr->vptrBasic->realize(
            pPlayer->mediaHandle, mime, mimeLength );
    } else {
        ret = JAVACALL_OK;
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
        ret = pItf->vptrBasic->get_format(pPlayer->mediaHandle,&fmt);
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

/**
 *
 */
javacall_result javacall_media_close(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, close)) {
        ret = pItf->vptrBasic->close(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_destroy(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, destroy)) {
        ret = pItf->vptrBasic->destroy(pPlayer->mediaHandle);
    }

    if( NULL != pPlayer )
    {
        if( NULL != pPlayer->uri ) FREE( pPlayer->uri );
        FREE( pPlayer );
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_acquire_device(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, acquire_device)) {
        ret = pItf->vptrBasic->acquire_device(pPlayer->mediaHandle);
    } else {
        ret = JAVACALL_OK;
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_release_device(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, release_device)) {
        ret = pItf->vptrBasic->release_device(pPlayer->mediaHandle);
    }
    return ret;
}

/**
 * Is this protocol handled by device? If yes return JAVACALL_OK.
 */
javacall_result javacall_media_download_handled_by_device(javacall_handle handle,
                                                  /*OUT*/javacall_bool* isHandled)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;

    *isHandled = pPlayer->downloadByDevice;

    return JAVACALL_OK;
}

javacall_result javacall_media_get_java_buffer_size(javacall_handle handle,
                                 long /*OUT*/*java_buffer_size, 
                                 long /*OUT*/*first_data_size)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_java_buffer_size)) {
        ret = pItf->vptrBasic->get_java_buffer_size(pPlayer->mediaHandle,
                                                    java_buffer_size, 
                                                    first_data_size);
    }

    return ret;
}

javacall_result javacall_media_set_whole_content_size(javacall_handle handle,
                                 long whole_content_size)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_whole_content_size)) {
        ret = pItf->vptrBasic->set_whole_content_size(pPlayer->mediaHandle, 
                                                      whole_content_size);
    }

    return ret;
}

javacall_result javacall_media_get_buffer_address(javacall_handle handle, 
                                 const void** /*OUT*/buffer, 
                                 long /*OUT*/*max_size)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_buffer_address)) {
        ret = pItf->vptrBasic->get_buffer_address(pPlayer->mediaHandle,
                                                  buffer,
                                                  max_size);
    }

    return ret;
}

/**
 * Store media data to temp file (except JTS type)
 */
javacall_result javacall_media_do_buffering(javacall_handle handle, 
                                            const void*     buffer,
                                            long*           length,
                                            javacall_bool*  need_more_data,
                                            long*           min_data_size)
{
    long ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, do_buffering)) {
        ret = pItf->vptrBasic->do_buffering(
            pPlayer->mediaHandle, buffer, length,
            need_more_data, min_data_size);
    }

    return ret;
}

/**
 * Delete temp file
 */
javacall_result javacall_media_clear_buffer(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, clear_buffer)) {
        ret = pItf->vptrBasic->clear_buffer(pPlayer->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_prefetch(javacall_handle handle){
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, prefetch)) {
        ret = pItf->vptrBasic->prefetch(pPlayer->mediaHandle);
    } else {
        ret = JAVACALL_OK;
    }

    return ret;
}

/**
 * Start playing (except JTS type)
 */
javacall_result javacall_media_start(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, start)) {
        ret = pItf->vptrBasic->start(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * Stop playing
 */
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

/**
 * Pause playing
 */
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

/**
 * Resume playing
 */
javacall_result javacall_media_resume(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, resume)) {
        ret = pItf->vptrBasic->resume(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * Get current position
 */
javacall_result javacall_media_get_time(javacall_handle handle, long* ms)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_time)) {
        return pItf->vptrBasic->get_time(pPlayer->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

/**
 * Set current position
 */
javacall_result javacall_media_set_time(javacall_handle handle, long* ms)
{
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_time)) {
        return pItf->vptrBasic->set_time(pPlayer->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

/**
 * Get media duration
 */
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

/**
 * This function called by JVM when this player goes to foreground.
 * There is only one foreground midlets but,
 * multiple player can be exits at this midlets.
 * So, there could be multiple players from JVM.
 * Device resource handling policy is not part of Java implementation.
 * It is totally depends on native layer's implementation.
 *
 * @param handle    Handle to the native player
 * @param option    MVM options.
 * Check about javacall_media_mvm_option type definition.
 *
 * @retval JAVACALL_OK    Something happened
 * @retval JAVACALL_FAIL  Nothing happened
 */
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

/**
 * This function called by JVM when this player goes to background.
 * There could be multiple background midlets.
 * Also, multiple player can be exits at this midlets.
 * Device resource handling policy is not part of Java implementation.
 * It is totally depends on
 * native layer's implementation.
 *
 * @param handle    Handle to the native player
 * @param option    MVM options.
 * Check about javacall_media_mvm_option type definition.
 *
 * @retval JAVACALL_OK    Something happened
 * @retval JAVACALL_FAIL  Nothing happened
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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

javacall_result javacall_media_set_video_fullscreenmode(javacall_handle handle, javacall_bool fullScreenMode)
{
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_fullscreenmode)) {
        ret = pItf->vptrVideo->set_video_fullscreenmode(pPlayer->mediaHandle, fullScreenMode);
    }

    return ret;
}

/**
 *
 */
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

/**
 *
 */
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

/**
 *
 */
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
/**
 * Tone to MIDI short message converter
 */
javacall_result javacall_media_play_tone(int appId, long note, long duration, long volume){
    mmaudio_tone_note(appId, note, duration, volume);
    return JAVACALL_OK;
}

javacall_result javacall_media_play_dualtone(int appId, long noteA, long noteB, long duration, long volume)
{
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 * MIDI note off
 */
javacall_result javacall_media_stop_tone(int appId){
    return JAVACALL_OK;
}

//=============================================================================

#if 0

typedef struct {
    volatile UINT       uID;
    volatile DWORD      msg;
    LONG                isLocked; /// used for simple spin-lock synchronization
    HMIDIOUT            hmo;
} tone_data_type;

#define G_IS_FREE    0
#define G_IS_LOCKED  1
#define G_SLEEP_LOCK_TIME 50

static tone_data_type _tone = {0, 0, G_IS_FREE, 0};
/*
 * To synchronize access tone_timer_callback and javacall_media_play_tone to
 * struct _tone, spin-lock synchronization is used.
 * Global initialization of critical section is avoided.
 * Another way was to use TIME_KILL_SYNCHRONOUS flag in timeSetEvent,
 * but this is not supported by Win95 and firsts Win98
 */

/**
 * MIDI note off callback
 */

static int tryEnterLong(LONG* pValue) {
    LONG oldValue;
    /// In VC 6.0 and earlier InterlockedCompareExchange works with pointers
#if (WINVER <= 0x400)
    oldValue = (LONG)InterlockedCompareExchange(
        (void**)pValue, (void*)G_IS_LOCKED, (void*)G_IS_FREE);
#else
    oldValue = InterlockedCompareExchange(pValue, G_IS_LOCKED, G_IS_FREE);
#endif
    return (oldValue == G_IS_FREE);
}

static void CALLBACK FAR
    tone_timer_callback(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    while (!tryEnterLong(&_tone.isLocked)) {
        Sleep(G_SLEEP_LOCK_TIME);
    }

    if (_tone.uID) {
        midiOutShortMsg(_tone.hmo, _tone.msg);
        _tone.msg = 0;

        javacall_close_midi_out(&_tone.hmo);

        timeKillEvent(_tone.uID);
        _tone.uID = 0;
    }

    _tone.isLocked = G_IS_FREE;
}

/**
 * Tone to MIDI short message converter
 */

javacall_result javacall_media_play_tone(int appId, long note, long duration, long volume)
{
    javacall_result ret = JAVACALL_OK;

    // force the duration be at least 200ms. This is a workaround
    // for broken synthesizers, which can not render the very short
    // tones properly.
    if (duration < 200) {
        duration = 200;
    }

    if (_tone.msg != 0) {
        ret = JAVACALL_FAIL;
    } else {
        ret = javacall_open_midi_out(&_tone.hmo, JAVACALL_TRUE);
    }

    if (JAVACALL_SUCCEEDED(ret)) {
        _tone.msg = (((volume & 0xFF) << 16) | (((note & 0xFF) << 8) | 0x90));
        /* Note on at channel 0 */
        midiOutShortMsg(_tone.hmo, _tone.msg);
        _tone.msg &= 0xFFFFFF80;

        #if WINVER >= 0x0501
            _tone.uID = timeSetEvent(duration, 100, tone_timer_callback, 0,
                TIME_ONESHOT | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
        #else
            _tone.uID = timeSetEvent(duration, 100, tone_timer_callback, 0,
                TIME_ONESHOT | TIME_CALLBACK_FUNCTION);
        #endif// WINVER >= 0x0501

        if (0 == _tone.uID) {
            midiOutShortMsg(_tone.hmo, _tone.msg);
            _tone.msg = 0;
            javacall_close_midi_out(&_tone.hmo);
            ret = JAVACALL_FAIL;
        }

    }

    return ret;
}

/**
 * MIDI note off
 */
javacall_result javacall_media_stop_tone(void)
{

    /// this call is ok, because tone_timer_callback use synchronization
    tone_timer_callback(_tone.uID, 0, 0, 0, 0);

    return JAVACALL_OK;
}

#endif

/* MIDIControl functions */
/*****************************************************************************/

/**
 * Get volume for the given channel.
 * The return value is independent of the master volume,
  which is set and retrieved with VolumeControl.
 *
 * @param handle    Handle to the library
 * @param channel   0-15
 * @param volume    channel volume, 0-127, or -1 if not known
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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

/**
 * Set volume for the given channel. To mute, set to 0.
 * This sets the current volume for the channel and may be overwritten
*  during playback by events in a MIDI sequence.
 *
 * @param handle    Handle to the library
 * @param channel   0-15
 * @param volume    channel volume, 0-127
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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

/**
 * Set program of a channel.
 * This sets the current program for the channel and may be overwritten
 * during playback by events in a MIDI sequence.
 *
 * @param handle    Handle to the library
 * @param channel   0-15
 * @param bank      0-16383, or -1 for default bank
 * @param program   0-127
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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

/**
 * Sends a short MIDI event to the device.
 *
 * @param handle    Handle to the library
 * @param type      0x80..0xFF, excluding 0xF0 and 0xF7,
 * which are reserved for system exclusive
 * @param data1     for 2 and 3-byte events: first data byte, 0..127
 * @param data2     for 3-byte events: second data byte, 0..127
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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

/**
 * Sends a long MIDI event to the device, typically a system exclusive message.
 *
 * @param handle    Handle to the library
 * @param data      array of the bytes to send.
 *                  This memory buffer will be freed after this function returned.
 *                  So, you should copy this data to the other internal memory buffer
 *                  if this function needs data after return.
 * @param offset    start offset in data array
 * @param length    number of bytes to be sent
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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

/* Record Control functions */
/*****************************************************************************/

/**
 * Is this recording transaction is handled by native layer or Java layer?
 *
 * @param handle    Handle to the library
 * @param locator   URL locator string for recording data (ex: file:///root/test.wav)
 *
 * @retval JAVACALL_OK      This recording transaction will be handled by native layer
 * @retval JAVACALL_FAIL    This recording transaction should be handled by Java layer
 */
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

/**
 * Specify the maximum size of the recording including any headers.
 * If a size of -1 is passed then the record size limit should be removed.
 *
 * @param handle    Handle to the library
 * @param size      The maximum size bytes of the recording requested as input parameter.
 *                  The supported maximum size bytes of the recording which is less than or
 *                  equal to the requested size as output parameter.
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
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

/**
 * Starts the recording. records all the data of the player ( video / audio )
 *
 * @param handle  Handle to the library
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_start_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;


    if (QUERY_RECORD_ITF(pItf, start_recording)) {
        ret = pItf->vptrRecord->start_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * Pause the recording. this should enable a future call
 * to javacall_media_start_recording. Another call to
 * javacall_media_start_recording after pause has been
 * called will result in recording the new data
 * and concatenating it to the previously recorded data.
 *
 * @param handle  Handle to the library
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_pause_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, pause_recording)) {
        ret = pItf->vptrRecord->pause_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * Stop the recording.
 *
 * @param handle  Handle to the library
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_stop_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, stop_recording)) {
        ret = pItf->vptrRecord->stop_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * The recording that has been done so far should be discarded. (deleted)
 * Recording will be paused before this method is called.
 * If javacall_media_start_recording is called after this method is called,
 * recording should resume. Calling reset after javacall_media_finish_recording
 * will have no effect on the current recording. If the Player that
 * is associated with this RecordControl is closed, javacall_media_reset_recording
 * will be called implicitly.
 *
 * @param handle  Handle to the library
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_reset_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, reset_recording)) {
        ret = pItf->vptrRecord->reset_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * The recording should be completed;
 * this may involve updating the header,flushing buffers and closing
 * the temporary file if it is used by the implementation.
 * javacall_media_pause_recording will be called before this method is called.
 *
 * @param handle  Handle to the library
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_commit_recording(javacall_handle handle) {
    javacall_result ret = JAVACALL_FAIL;
    javacall_impl_player* pPlayer = (javacall_impl_player*)handle;
    media_interface* pItf = pPlayer->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, commit_recording)) {
        ret = pItf->vptrRecord->commit_recording(pPlayer->mediaHandle);
    }

    return ret;
}

/**
 * Get how much data was returned.
 * This function can be called after a successful call to
 * javacall_media_finish_recording.
 *
 * @param handle    Handle to the library
 * @param size      How much data was recorded
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
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

/**
 * Gets the recorded data.
 * This function can be called after a successful call to
 * javacall_media_finish_recording.
 * It receives the data recorded from offset till the size.
 *
 * @param handle    Handle to the library
 * @param buffer    Buffer will contains the recorded data
 * @param offset    An offset to the start of the required recorded data
 * @param size      How much data will be copied to buffer
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
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

/**
 * Get the current recording data content type mime string length
 *
 * @return  If success return length of string else return 0
 */
javacall_result javacall_media_get_record_content_type_length(javacall_handle handle,
                                                              int* length) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};

    *length = sizeof(contentType) / sizeof(*contentType);
    return JAVACALL_OK;
}

/**
 * Get the current recording data content type mime string length
 * For example : 'audio/x-wav' for audio recording
 *
 * @param handle                Handle of native player
 * @param contentTypeBuf        Buffer to return content type Unicode string
 * @param contentTypeBufLength  Length of contentTypeBuf buffer (in Unicode metrics)
 * @param actualLength          Length of content type string stored in contentTypeBuf
 * @return                      
 */
javacall_result javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           /*INOUT*/ int* length) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};
    memcpy(contentTypeBuf, contentType, sizeof(contentType));
    *length = sizeof(contentType) / sizeof(*contentType);
    return JAVACALL_OK;
}

/**
 * Close the recording. Delete all resources related with this recording.
 *
 * @param handle    Handle to the library
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
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
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_metadata_key(javacall_handle handle,
                                                long index,
                                                long bufLength,
                                                /*OUT*/ javacall_utf16* buf)
{
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_metadata(javacall_handle handle,
                                            const javacall_utf16* key,
                                            long bufLength,
                                            javacall_utf16* buf)
{
    return JAVACALL_FAIL;
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
