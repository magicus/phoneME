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

/**
 * - NOTICE -
 *
 * This is a very simple implementation of basic multimedia porting layer
 * by using Win32 APIs. It could be used as a sample codes but,
 * do not guarantee any of problems might be happen by this codes.
 * You can use same buffering method shown here but, can choose another
 * option like heap memory. You can use same non-blocking play method
 * shown here but, can choose another option like H/W async playing.
 *
 * This sample files shows:
 *      - How OEM could use native handle structure to implement MMAPI native porting layer
 *      - Buffering implementation by using temporary file
 *      - Non-blocking media playing
 *      - How stop and start function related each other (re-start from stopped position)
 *      - How send media event to Java (End of media)
 *      - How to implement non-blocking JTS player (mmtone.c file)
 *      - How to implement camera capture interfaces - capture://video (mmcamera.c file)
 *      - How to implement audio recording - capture://audio (mmrecord.c file)
 */

/**
 * - IMPL_NOTE -
 * 1. Implement direct video playing by using DirectX library
 * 2. Implement optional controls by using Win32 multimedia library
 * 3. Fix JTS player => Stop and start, pause, resume,
 *          get duration, set time, get time and etc..
 */

#include "lime.h"
#include "multimedia.h"
#include "mmmididev.h"

//=============================================================================

static javacall_media_caps g_caps[] = 
{
    {
        "",                               //mediaFormat
        "",                               //contentTypes
        0x00000000,                        //whole_protocols
        0x00000000                         //streaming_protocols
    },{
        "",                               //mediaFormat
        "",                               //contentTypes
        0x00000000,                        //whole_protocols
        0x00000000                         //streaming_protocols
    }
};

static javacall_media_configuration g_cfg;

javacall_result
javacall_media_get_configuration(const javacall_media_configuration** cfg)
{
    *cfg = &g_cfg;
    return JAVACALL_OK;
}

javacall_result javacall_media_initialize(void)
{
    g_cfg.audioEncoding         = "";
    g_cfg.videoEncoding         = "";
    g_cfg.videoSnapshotEncoding = "";

    g_cfg.supportMixing         = JAVACALL_TRUE;
    g_cfg.supportRecording      = JAVACALL_TRUE;
    g_cfg.supportDeviceTone     = JAVACALL_TRUE;
    g_cfg.supportDeviceMIDI     = JAVACALL_TRUE;
    g_cfg.supportCaptureRadio   = JAVACALL_FALSE;

    g_cfg.mediaCaps             = g_caps;

    return JAVACALL_OK;
}

javacall_result javacall_media_finalize(void)
{
    javacall_result ret = JAVACALL_FAIL;
    return ret;
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
    //JAVACALL_MEDIA_FORMAT_UNKNOWN excluded, it will be mapped to -1
    JAVACALL_MEDIA_FORMAT_UNSUPPORTED
};

static const int g_fmt_count = sizeof( g_fmt ) / sizeof( g_fmt[ 0 ] );

jc_fmt fmt_str2enum( javacall_media_format_type fmt )
{
    int n;

    JC_MM_ASSERT( JC_FMT_UNSUPPORTED == g_fmt_count - 1 );

    for( n = 0; n < g_fmt_count; n++ )
        if( !strcmp( fmt, g_fmt[ n ] ) ) return (jc_fmt)n;

    return JC_FMT_UNKNOWN;
}

javacall_media_format_type fmt_enum2str( jc_fmt fmt )
{
    JC_MM_ASSERT( JC_FMT_UNSUPPORTED == g_fmt_count - 1 );

    if( JC_FMT_UNKNOWN == fmt ) return JAVACALL_MEDIA_FORMAT_UNKNOWN;

    JC_MM_ASSERT( fmt > 0 && fmt < g_fmt_count );

    return g_fmt[ fmt ];
}

//=============================================================================

extern media_interface g_audio_itf;
extern media_interface g_qsound_itf;
extern media_interface g_amr_audio_itf;
extern media_interface g_qsound_interactive_midi_itf;
extern media_interface g_video_itf;
extern media_interface g_tone_itf;
extern media_interface g_camera_itf;
extern media_interface g_interactive_midi_itf;
extern media_interface g_record_itf;

media_interface* fmt_enum2itf( jc_fmt fmt )
{
    switch( fmt )
    {
    case JC_FMT_MPEG_4_SVP:
    case JC_FMT_MPEG_4_AVC:
    case JC_FMT_VIDEO_3GPP:
        return &g_video_itf;    // was: VIDEO_MPEG4, VIDEO_3GPP, CAPTURE_VIDEO, VIDEO_MPEG, VIDEO_GIF

    case JC_FMT_MIDI:
    case JC_FMT_SP_MIDI:
    case JC_FMT_MS_PCM:
        return &g_qsound_itf;   // was: AUDIO_MIDI, AUDIO_WAVE

    case JC_FMT_MPEG1_LAYER3:
    case JC_FMT_MPEG1_LAYER3_PRO:
    case JC_FMT_MPEG2_AAC:
    case JC_FMT_MPEG4_HE_AAC:
        return &g_audio_itf;    // was: AUDIO_MP3, AUDIO_MPEG4, AUDIO_AAC, AUDIO_MP3_2

    case JC_FMT_AMR:
    case JC_FMT_AMR_WB:
    case JC_FMT_AMR_WB_PLUS:
        return &g_amr_audio_itf; // was: AUDIO_AMR

    case JC_FMT_TONE:
        return &g_tone_itf;     // AUDIO_TONE

    default:
        return NULL;
    }

    // &g_record_itf,              // JAVACALL_CAPTURE_AUDIO,     /** Audio capture   */
    // &g_interactive_midi_itf,    // JAVACALL_INTERACTIVE_MIDI,  /** Interactive MIDI */
}

//=============================================================================

/*
static javacall_media_caps _media_caps[] = {
    {JAVACALL_AUDIO_TONE_MIME,      3, {"device", "http", "file"}},
    {JAVACALL_AUDIO_MIDI_MIME,      2, {"http", "file"}},
    {JAVACALL_AUDIO_MIDI_MIME_2,    2, {"http", "file"}},
    {JAVACALL_AUDIO_WAV_MIME,       3, {"capture", "http", "file"}},
    {JAVACALL_AUDIO_MP3_MIME,       3, {"http", "file", "rtsp"}},
    {JAVACALL_VIDEO_MPEG4_MIME,     3, {"http", "file", "rtsp"}},
    {JAVACALL_VIDEO_3GPP_MIME,      3, {"http", "file", "rtsp"}},
    {JAVACALL_AUDIO_MP3_MIME_2,     2, {"http", "file", "rtsp"}},
    {JAVACALL_AUDIO_AMR_MIME,       2, {"http", "file"}},
    {JAVACALL_AUDIO_AAC_MIME,       2, {"http", "file"}},
    {JAVACALL_VIDEO_MPEG_MIME,      3, {"capture", "http", "file"}},
    {JAVACALL_VIDEO_GIF_MIME,       2, {"http", "file"}},


    // End of caps => mimeType should be NULL and list all of
    // protocols from here !
    {NULL, 5, {"device", "capture", "http" , "file", "rtsp"}}
};
*/


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

/**
 * Checks, that second string contains first as prefix
 */
static int check_prefix(javacall_const_utf16_string left,
                        javacall_const_utf16_string right)
{
    int length = strlen((const char *)left);
    return strncmp((const char *)left, (const char *)right, length);
}

/**
 * Convert mime string to media type constants value
 */
/*
static javacall_media_format_type _detrmine_url_format(const javacall_utf16* mime, long length)
{
    javacall_media_format_type ret = JAVACALL_END_OF_TYPE;
    char* cMime = MALLOC(length + 1);

    if (cMime) {
        int wres = WideCharToMultiByte(CP_ACP, 0, mime, length,
            cMime, length + 1, NULL, NULL);

        if (0 != wres) {
            cMime[length] = 0;
            JC_MM_DEBUG_PRINT1("javautil_media_mime_to_type %s\n", cMime);

            if (0 == strcmp(JAVACALL_VIDEO_GIF_MIME, cMime)) {
                JC_MM_DEBUG_PRINT1("gif type %s\n", cMime);
                ret = JAVACALL_VIDEO_GIF;
            } else if (0 == strcmp(JAVACALL_AUDIO_MIDI_MIME, cMime)) {
                ret = JC_FMT_MIDI;
            } else if (0 == strcmp(JAVACALL_AUDIO_WAV_MIME, cMime)) {
                ret = JC_FMT_MS_PCM;
            } else if (0 == strcmp(JAVACALL_AUDIO_MP3_MIME, cMime)) {
                ret = JAVACALL_AUDIO_MP3;
            } else if (0 == strcmp(JAVACALL_AUDIO_MP3_MIME_2, cMime)) {
                ret = JAVACALL_AUDIO_MP3_2;
            } else if (0 == strcmp(JAVACALL_AUDIO_TONE_MIME, cMime)) {
                ret = JC_FMT_TONE;
            } else if (0 == strcmp(JAVACALL_DEVICE_TONE_MIME, cMime)) {
                ret = JC_FMT_TONE;
            } else if (0 == strcmp(JAVACALL_DEVICE_MIDI_MIME, cMime)) {
                ret = JAVACALL_INTERACTIVE_MIDI;
            } else if (0 == strcmp(JAVACALL_VIDEO_MPEG4_MIME_2, cMime)) {
                ret = JAVACALL_VIDEO_MPEG4;
            } else if (0 == strcmp(JAVACALL_AUDIO_MPEG4_MIME_2, cMime)) {
                ret = JAVACALL_AUDIO_MPEG4;
            } else if (0 == strcmp(JAVACALL_AUDIO_AMR_MIME, cMime)) {
                ret = JC_FMT_AMR;
            } else if (0 == strcmp(JAVACALL_AUDIO_AAC_MIME, cMime)) {
                ret = JAVACALL_AUDIO_AAC;
            }else if (0 == strcmp(JAVACALL_VIDEO_3GPP_MIME, cMime)) {
                ret = JAVACALL_VIDEO_3GPP;
            } else if (0 == strcmp(JAVACALL_VIDEO_3GPP_MIME_2, cMime)) {
                ret = JAVACALL_VIDEO_3GPP;
            } else if (0 == strcmp(JAVACALL_VIDEO_MPEG_MIME, cMime)) {
                ret = JAVACALL_VIDEO_MPEG;
            } else if (0 == strncmp(JAVACALL_CAPTURE_VIDEO_MIME, cMime, strlen(JAVACALL_CAPTURE_VIDEO_MIME))) {
                ret = JAVACALL_CAPTURE_VIDEO;
            } else if (0 == strncmp(JAVACALL_CAPTURE_AUDIO_MIME, cMime, strlen(JAVACALL_CAPTURE_AUDIO_MIME))) {
                ret = JAVACALL_CAPTURE_AUDIO;
            }
        }
        FREE(cMime);
    }
    JC_MM_DEBUG_PRINT1("javautil_media_mime_to_type media_type is %d\n", ret);
    return ret;
}
*/

/**
 * Convert media type constants value to mime strings
 */
/*
char * javautil_media_type_to_mime(javacall_media_format_type media_type) {
    JC_MM_DEBUG_PRINT("javautil_media_type_to_mime\n");
    switch (media_type) {
        case JAVACALL_VIDEO_GIF: return JAVACALL_VIDEO_GIF_MIME;
        case JC_FMT_MIDI: return JAVACALL_AUDIO_MIDI_MIME;
        case JC_FMT_MS_PCM: return JAVACALL_AUDIO_WAV_MIME;
        case JAVACALL_AUDIO_MP3: return JAVACALL_AUDIO_MP3_MIME;
        case JAVACALL_AUDIO_MP3_2: return JAVACALL_AUDIO_MP3_MIME_2;
        case JC_FMT_TONE: return JAVACALL_AUDIO_TONE_MIME;
        case JAVACALL_INTERACTIVE_MIDI: return JAVACALL_DEVICE_MIDI_MIME;
        case JAVACALL_VIDEO_MPEG4: return JAVACALL_VIDEO_MPEG4_MIME_2;
        case JC_FMT_AMR: return JAVACALL_AUDIO_AMR_MIME;
        case JAVACALL_AUDIO_AAC: return JAVACALL_AUDIO_AAC_MIME;
        case JAVACALL_VIDEO_3GPP: return JAVACALL_VIDEO_3GPP_MIME;
        case JAVACALL_VIDEO_MPEG: return JAVACALL_VIDEO_MPEG_MIME;
        case JAVACALL_CAPTURE_VIDEO: return JAVACALL_CAPTURE_VIDEO_MIME;
        case JAVACALL_CAPTURE_AUDIO: return JAVACALL_CAPTURE_AUDIO_MIME;
        case JAVACALL_AUDIO_MPEG4: return JAVACALL_AUDIO_MPEG4_MIME_2;

        default: return NULL;
    }
}
*/
/* Native Implementation Functions */
/*****************************************************************************/

/**
 * Send event to external event queue
 * This function is a sample implementation for Win32
 */
//static void jmmpSendEvent(int type, int param1, int param2)
//{
//#if 0
    // This memory SHOULD be deallocated from event handler
    //int* pParams = (int*)pcsl_mem_malloc(sizeof(int) * 2);

    //if (pParams) {
    //    pParams[0] = param1;
    //    pParams[1] = param2;
    //    PostMessage(GET_MCIWND_HWND(), WM_MEDIA, type, (LPARAM)pParams);
    //}
//#endif
//}


javacall_media_format_type fmt_determine_from_url(javacall_const_utf16_string uri, 
                                                  long uriLength)
{
    return JAVACALL_MEDIA_FORMAT_MS_PCM;
}

/**
 * Native player create.
 * This function create internal information structure that will be used from other native API.
 */
javacall_result javacall_media_create(int appId,
                                      int playerId,
                                      javacall_const_utf16_string uri, 
                                      long uriLength,
                                      javacall_handle *handle)
{
    javacall_media_format_type mediaType;
    native_handle* pHandle = NULL;
    media_interface* pItf;
    JC_MM_DEBUG_PRINT("javacall_media_create \n");

    pHandle = MALLOC(sizeof(native_handle));
    if (NULL == pHandle) return JAVACALL_OUT_OF_MEMORY;

    /* Mime type string to type constants */
    mediaType = fmt_determine_from_url( uri, uriLength );

    if (JAVACALL_MEDIA_FORMAT_UNKNOWN == mediaType) {
        JC_MM_DEBUG_PRINT1("javacall_media_create fail %d\n", mediaType);
        FREE(pHandle);
        return JAVACALL_FAIL;
    }


    /* Query interface table */
    pItf = fmt_enum2itf( fmt_str2enum(mediaType) );
    if (NULL == pItf) {
        FREE(pHandle);
        return JAVACALL_FAIL;
    }

    JC_MM_DEBUG_PRINT2("javacall_media_create %d %x\n", mediaType, pItf);

    if (QUERY_BASIC_ITF(pItf, create)) {
        javacall_handle handle = pItf->vptrBasic->create( 
            appId, playerId, fmt_str2enum(mediaType), uri, uriLength);
        if (NULL == handle) {
            FREE(pHandle);
            return JAVACALL_FAIL;
        }
        pHandle->mediaType = mediaType;
        pHandle->mediaHandle = handle;
        pHandle->mediaItfPtr = pItf;
    } else {
        JC_MM_DEBUG_PRINT("QUERY_BASIC_ITF FAIL\n");
    }

    *handle = (javacall_handle)pHandle;

    return JAVACALL_OK;
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

javacall_result javacall_media_get_format(javacall_handle handle, 
                              javacall_media_format_type /*OUT*/*format)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    jc_fmt fmt = JC_FMT_UNKNOWN;

    if (QUERY_BASIC_ITF(pItf, get_format)) {
        ret = pItf->vptrBasic->get_format(pHandle->mediaHandle,&fmt);
        if( JAVACALL_OK == ret ) {
            *format = fmt_enum2str( fmt );
        }
    } else {
        *format = pHandle->mediaType;
        ret = JAVACALL_OK;
    }

    return ret;
}

javacall_result javacall_media_get_player_controls(javacall_handle handle,
                              int /*OUT*/*controls)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_player_controls)) {
        ret = pItf->vptrBasic->get_player_controls(pHandle->mediaHandle,controls);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, close)) {
        ret = pItf->vptrBasic->close(pHandle->mediaHandle);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_destroy(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, destroy)) {
        ret = pItf->vptrBasic->destroy(pHandle->mediaHandle);
    }

    if (pHandle) {
        pHandle->mediaItfPtr = NULL;
        pHandle->mediaHandle = NULL;
        FREE(pHandle);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_acquire_device(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, acquire_device)) {
        ret = pItf->vptrBasic->acquire_device(pHandle->mediaHandle);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_release_device(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, release_device)) {
        ret = pItf->vptrBasic->release_device(pHandle->mediaHandle);
    }
    return ret;
}

/**
 * Is this protocol handled by device? If yes return JAVACALL_OK.
 */
javacall_result javacall_media_download_handled_by_device(javacall_handle handle,
                                                  /*OUT*/javacall_bool* isHandled)
{
    native_handle* pHandle = (native_handle*)handle;

    *isHandled = JAVACALL_FALSE;

    return JAVACALL_OK;
}

javacall_result javacall_media_get_java_buffer_size(javacall_handle handle,
                                 long /*OUT*/*java_buffer_size, 
                                 long /*OUT*/*first_data_size)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_java_buffer_size)) {
        ret = pItf->vptrBasic->get_java_buffer_size(pHandle->mediaHandle,
                                                    java_buffer_size, 
                                                    first_data_size);
    }

    return ret;
}

javacall_result javacall_media_set_whole_content_size(javacall_handle handle,
                                 long whole_content_size)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_whole_content_size)) {
        ret = pItf->vptrBasic->set_whole_content_size(pHandle->mediaHandle, 
                                                      whole_content_size);
    }

    return ret;
}

javacall_result javacall_media_get_buffer_address(javacall_handle handle, 
                                 const void** /*OUT*/buffer, 
                                 long /*OUT*/*max_size)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_buffer_address)) {
        ret = pItf->vptrBasic->get_buffer_address(pHandle->mediaHandle,
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, do_buffering)) {
        ret = pItf->vptrBasic->do_buffering(
            pHandle->mediaHandle, buffer, length,
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, clear_buffer)) {
        ret = pItf->vptrBasic->clear_buffer(pHandle->mediaHandle);
    }

    return ret;
}

javacall_result javacall_media_realize(javacall_handle handle,
                                       javacall_const_utf16_string mime,
                                       long mimeLength){
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, realize)) {
        ret = pItf->vptrBasic->realize(pHandle->mediaHandle,
                                       mime, mimeLength);
    }

    return ret;
}

javacall_result javacall_media_prefetch(javacall_handle handle){
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, prefetch)) {
        ret = pItf->vptrBasic->prefetch(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Start playing (except JTS type)
 */
javacall_result javacall_media_start(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, start)) {
        ret = pItf->vptrBasic->start(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Stop playing
 */
javacall_result javacall_media_stop(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, stop)) {
        ret = pItf->vptrBasic->stop(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Pause playing
 */
javacall_result javacall_media_pause(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, pause)) {
        ret = pItf->vptrBasic->pause(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Resume playing
 */
javacall_result javacall_media_resume(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, resume)) {
        ret = pItf->vptrBasic->resume(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Get current position
 */
javacall_result javacall_media_get_time(javacall_handle handle, long* ms)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_time)) {
        return pItf->vptrBasic->get_time(pHandle->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

/**
 * Set current position
 */
javacall_result javacall_media_set_time(javacall_handle handle, long* ms)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_time)) {
        return pItf->vptrBasic->set_time(pHandle->mediaHandle, ms);
    }

    return JAVACALL_FAIL;
}

/**
 * Get media duration
 */
javacall_result javacall_media_get_duration(javacall_handle handle, long* ms)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_duration)) {
        return pItf->vptrBasic->get_duration(pHandle->mediaHandle, ms);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_foreground)) {
        ret = pItf->vptrBasic->switch_to_foreground(pHandle->mediaHandle, appId);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_background)) {
        ret = pItf->vptrBasic->switch_to_background(pHandle->mediaHandle, appId);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, get_volume)) {
        ret = pItf->vptrVolume->get_volume(pHandle->mediaHandle,level);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_set_volume(javacall_handle handle, long* level)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_volume)) {
        ret = pItf->vptrVolume->set_volume(pHandle->mediaHandle, level);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_is_mute(javacall_handle handle, javacall_bool* mute )
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, is_mute)) {
        ret = pItf->vptrVolume->is_mute(pHandle->mediaHandle, mute);
    }

    return ret;
}

/**
 *
 */
javacall_result javacall_media_set_mute(javacall_handle handle, javacall_bool mute)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_mute)) {
        ret = pItf->vptrVolume->set_mute(pHandle->mediaHandle, mute);
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
    static LimeFunction *f1 = NULL;
    static LimeFunction *f2 = NULL;

    /*if(on == TRUE){
        if (f1 == NULL) {
            f1 = NewLimeFunction("com.sun.mmedia",
                        "MediaBridge",
                        "resumeVideoPlayback");
        }
        f1->call(f1, &res);
    }else{
        if (f2 == NULL) {
            f2 = NewLimeFunction("com.sun.mmedia",
                        "MediaBridge",
                        "pauseVideoPlayback");
        }
        f2->call(f2, &res);

    }*/
    return JAVACALL_OK;
}

/**
 *
 */
javacall_result javacall_media_get_video_size(javacall_handle handle,
                                              long* width, long* height)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, get_video_size)) {
        ret = pItf->vptrVideo->get_video_size(pHandle->mediaHandle, width, height);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_visible)) {
        ret = pItf->vptrVideo->set_video_visible(pHandle->mediaHandle, visible);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_location)) {
        ret = pItf->vptrVideo->set_video_location(pHandle->mediaHandle, x, y, w, h);
    }

    return ret;
}

javacall_result javacall_media_set_video_fullscreenmode(javacall_handle handle, javacall_bool fullScreenMode)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_fullscreenmode)) {
        ret = pItf->vptrVideo->set_video_fullscreenmode(pHandle->mediaHandle, fullScreenMode);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, start_video_snapshot)) {
        ret = pItf->vptrSnapshot->start_video_snapshot(
            pHandle->mediaHandle, imageType, length);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, get_video_snapshot_data_size)) {
        ret = pItf->vptrSnapshot->get_video_snapshot_data_size(
            pHandle->mediaHandle, size);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, get_video_snapshot_data)) {
        ret = pItf->vptrSnapshot->get_video_snapshot_data(
            pHandle->mediaHandle, buffer, size);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_channel_volume)) {
        ret = pItf->vptrMidi->get_channel_volume(pHandle->mediaHandle, channel, volume);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_channel_volume)) {
        ret = pItf->vptrMidi->set_channel_volume(pHandle->mediaHandle, channel, volume);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_program)) {
        ret = pItf->vptrMidi->set_program(pHandle->mediaHandle, channel, bank, program);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    if (QUERY_MIDI_ITF(pItf, short_midi_event)) {
        ret = pItf->vptrMidi->short_midi_event(pHandle->mediaHandle, type, data1, data2);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_MIDI_ITF(pItf, long_midi_event)) {
        ret = pItf->vptrMidi->long_midi_event(pHandle->mediaHandle, data, offset, length);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, recording_handled_by_native)) {
        ret = pItf->vptrRecord->recording_handled_by_native(
            pHandle->mediaHandle, locator);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, set_recordsize_limit)) {
        ret = pItf->vptrRecord->set_recordsize_limit(pHandle->mediaHandle, size);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;


    if (QUERY_RECORD_ITF(pItf, start_recording)) {
        ret = pItf->vptrRecord->start_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, pause_recording)) {
        ret = pItf->vptrRecord->pause_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, stop_recording)) {
        ret = pItf->vptrRecord->stop_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, reset_recording)) {
        ret = pItf->vptrRecord->reset_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, commit_recording)) {
        ret = pItf->vptrRecord->commit_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, get_recorded_data_size)) {
        ret = pItf->vptrRecord->get_recorded_data_size(pHandle->mediaHandle, size);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, get_recorded_data)) {
        ret = pItf->vptrRecord->get_recorded_data(
            pHandle->mediaHandle, buffer, offset, size);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_RECORD_ITF(pItf, close_recording)) {
        ret = pItf->vptrRecord->close_recording(pHandle->mediaHandle);
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
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_max_rate)) {
        ret = pItf->vptrRate->get_max_rate(pHandle->mediaHandle, maxRate);
    }

    return ret;
}

javacall_result javacall_media_get_min_rate(javacall_handle handle, long* minRate)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_min_rate)) {
        ret = pItf->vptrRate->get_min_rate(pHandle->mediaHandle, minRate);
    }

    return ret;
}

javacall_result javacall_media_set_rate(javacall_handle handle, long rate)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, set_rate)) {
        ret = pItf->vptrRate->set_rate(pHandle->mediaHandle, rate);
    }

    return ret;
}

javacall_result javacall_media_get_rate(javacall_handle handle, long* rate)
{
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_RATE_ITF(pItf, get_rate)) {
        ret = pItf->vptrRate->get_rate(pHandle->mediaHandle, rate);
    }

    return ret;
}

/* TempoControl functions **************************************************/

javacall_result javacall_media_get_tempo(javacall_handle handle,
                                         /*OUT*/ long* tempo)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;


    if (QUERY_TEMPO_ITF(pItf, get_tempo)) {
        ret = pItf->vptrTempo->get_tempo(pHandle->mediaHandle, tempo);
    }

    return ret;
}

javacall_result javacall_media_set_tempo(javacall_handle handle, long tempo)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_TEMPO_ITF(pItf, set_tempo)) {
        ret = pItf->vptrTempo->set_tempo(pHandle->mediaHandle, tempo);
    }

    return ret;
}

/* PitchControl functions ******************************************************/

javacall_result javacall_media_get_max_pitch(javacall_handle handle,
                                             /*OUT*/ long* maxPitch)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_max_pitch)) {
        ret = pItf->vptrPitch->get_max_pitch(pHandle->mediaHandle, maxPitch);
    }

    return ret;
}

javacall_result javacall_media_get_min_pitch(javacall_handle handle,
                                             /*OUT*/ long* minPitch)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_min_pitch)) {
        ret = pItf->vptrPitch->get_min_pitch(pHandle->mediaHandle, minPitch);
    }

    return ret;
}

javacall_result javacall_media_set_pitch(javacall_handle handle, long pitch) {
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, set_pitch)) {
        ret = pItf->vptrPitch->set_pitch(pHandle->mediaHandle, pitch);
    }

    return ret;
}

javacall_result javacall_media_get_pitch(javacall_handle handle,
                                         /*OUT*/ long* pitch)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;

    if (QUERY_PITCH_ITF(pItf, get_pitch)) {
        ret = pItf->vptrPitch->get_pitch(pHandle->mediaHandle, pitch);
    }

    return ret;
}


/* MIDI Bank Query functions (mainly stubs) *******************************************/
javacall_result javacall_media_is_midibank_query_supported(javacall_handle handle,
                                                           /*OUT*/ long* supported) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_midibank_list(javacall_handle handle,
                                                 long custom, /*OUT*/short* banklist,
                                                 /*INOUT*/ long* numlist) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_midibank_key_name(javacall_handle handle,
                                                     long bank, long program,
                                                     long key,
                                                     /*OUT*/char* keyname,
                                                     /*INOUT*/ long* keynameLen) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_midibank_program_name(javacall_handle handle,
                                                         long bank, long program,
                                                         /*OUT*/char* progname,
                                                         /*INOUT*/ long* prognameLen) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_midibank_program_list(javacall_handle handle,
                                                         long bank,
                                                         /*OUT*/char* proglist,
                                                         /*INOUT*/ long* proglistLen) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_midibank_program(javacall_handle handle,
                                                    long channel, /*OUT*/long* prog) {
    return JAVACALL_FAIL;
}


javacall_result javacall_media_map_frame_to_time(javacall_handle handle,
                                                 long frameNum, /*OUT*/ long* ms) {
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, map_frame_to_time)) {
        ret = pItf->vptrFposition->map_frame_to_time(pHandle->mediaHandle, frameNum, ms);
    }

    return ret;
}

javacall_result javacall_media_map_time_to_frame(javacall_handle handle,
                                                 long ms, /*OUT*/ long* frameNum) {
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, map_time_to_frame)) {
        ret = pItf->vptrFposition->map_time_to_frame(pHandle->mediaHandle, ms, frameNum);
    }

    return ret;
}

javacall_result javacall_media_seek_to_frame(javacall_handle handle,
                                             long frameNum, /*OUT*/ long* actualFrameNum) {
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, seek_to_frame)) {
        ret = pItf->vptrFposition->seek_to_frame(pHandle->mediaHandle, frameNum, actualFrameNum);
    }

    return ret;
}

javacall_result javacall_media_skip_frames(javacall_handle handle, /*INOUT*/ long* nFrames) {
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->mediaItfPtr;
    javacall_result ret = JAVACALL_FAIL;

    if (QUERY_FPOSITION_ITF(pItf, skip_frames)) {
        ret = pItf->vptrFposition->skip_frames(pHandle->mediaHandle, nFrames);
    }

    return ret;
}
