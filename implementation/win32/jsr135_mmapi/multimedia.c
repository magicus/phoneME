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
 *  - How OEM could use native handle structure to implement MMAPI native porting layer
 *  - Buffering implementation by using temporary file
 *  - Non-blocking media playing 
 *  - How stop and start function related each other (re-start from stopped position)
 *  - How send media event to Java (End of media)
 *  - How to implement non-blocking JTS player (mmtone.c file)
 *  - How to implement camera capture interfaces - capture://video (mmcamera.c file)
 *  - How to implement audio recording - capture://audio (mmrecord.c file)
 */
 
/**
 * - IMPL_NOTE -
 * 1. Implement direct video playing by using DirectX library
 * 2. Implement optional controls by using Win32 multimedia library
 * 3. Fix JTS player => Stop and start, pause, resume, 
 *          get duration, set time, get time and etc..
 */

#include <math.h>
#include <string.h> 
#include <stdio.h>
#include <tchar.h>
#include "multimedia.h"
#include <windows.h>
#include <vfw.h>

/* Media Capabilities */
/*****************************************************************************/

/**
 * NOTE: Example of javacall_media_caps value
 *
 * OEM should list all of supported content types as a MIME string
 * (for example, audio/x-wav, video/mpeg, and etc...)
 * And, list supported protocols per content types 
 * (for example, http, file, rtsp, device, or capture)
 *
 * If OEM support interactive MIDI player, Add 'device' protocol
 * to the MIDI type
 *
 * If OEM support audio recording, Add 'capture' protocol
 * to the audio recording format MIME type
 *
 * If OEM support video recording, Add 'capture' protocol
 * to the video recording format MIME type
 *
 * At the end of item, content type string should be NULL
 * And, list all of supported protocols from this item
 *
 * <mime string>, <protocol count>, <protocol strings>
 */

// IMPL_NOTE: the only usage for this data now is
// to determine if we have support for JTS and AMR.
// actual supported protocols/types are defined in Java.

static const javacall_media_caps _media_caps[] = {
    /* End of caps => mimeType should be NULL and list all of 
       protocols from here ! */ 
    {NULL, 3, {"device", "capture", "http"}} 
};

/* Media native API interfaces */
/*****************************************************************************/

/**
 * Native Handle
 */
typedef struct {
    javacall_media_type mediaType;
    javacall_handle     mediaHandle;
    media_interface*    meidaItfPtr;
} native_handle;

extern media_interface g_audio_itf;
extern media_interface g_video_itf;
extern media_interface g_tone_itf;
extern media_interface g_camera_itf;
extern media_interface g_interactive_midi_itf;
extern media_interface g_record_itf;

/**
 * Media type to interface map table
 */
static media_interface* _itfTable[] = {
    NULL,   
    &g_video_itf,               // JAVACALL_VIDEO_MPEG4 = 1,   /** MPEG4 video     */
    &g_video_itf,               // JAVACALL_VIDEO_3GPP,        /** 3GPP video      */
    &g_audio_itf,               // JAVACALL_AUDIO_MIDI,        /** MIDI audio      */
    &g_audio_itf,               // JAVACALL_AUDIO_WAV,         /** WAV audio       */
    &g_audio_itf,               // JAVACALL_AUDIO_MP3,         /** MP3 audio       */
    NULL,                       // JAVACALL_AUDIO_AMR,         /** AMR audio       */
    NULL,                       // JAVACALL_AUDIO_MPEG4,       /** MPEG4 audio     */
    &g_tone_itf,                // JAVACALL_AUDIO_TONE,        /** JTS tone        */
    NULL,                       // JAVACALL_AUDIO_QCELP,       /** QCELP audio     */
    NULL,                       // JAVACALL_AUDIO_AAC,         /** AAC audio       */
    &g_record_itf,              // JAVACALL_CAPTURE_AUDIO,     /** Audio capture   */
    &g_camera_itf,              // JAVACALL_CAPTURE_VIDEO,     /** Video capture   */
    &g_interactive_midi_itf,    // JAVACALL_INTERACTIVE_MIDI,  /** Interactive MIDI */
};

#define QUERY_BASIC_ITF(_pitf_, _method_)     \
    ( (_pitf_) && (_pitf_)->vptrBasic && (_pitf_)->vptrBasic->##_method_ )
    
#define QUERY_VOLUME_ITF(_pitf_, _method_)    \
    ( (_pitf_) && (_pitf_)->vptrVolume && (_pitf_)->vptrVolume->##_method_ )
    
#define QUERY_VIDEO_ITF(_pitf_, _method_)     \
    ( (_pitf_) && (_pitf_)->vptrVideo && (_pitf_)->vptrVideo->##_method_  )
    
#define QUERY_SNAPSHOT_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrSnapshot && (_pitf_)->vptrSnapshot->##_method_ )

#define QUERY_MIDI_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrMidi && (_pitf_)->vptrMidi->##_method_ )
    
#define QUERY_RECORD_ITF(_pitf_, _method_)  \
    ( (_pitf_) && (_pitf_)->vptrRecord && (_pitf_)->vptrRecord->##_method_ )

/*****************************************************************************/

/**
 * Checks, that second string contains first as prefix
 */
static int check_prefix(const char* left,
                        const char* right)
{
    int length = strlen(left);
    return strncmp(left, right, length);
}

/**
 * Convert mime string to media type constants value
 */
static javacall_media_type javautil_media_mime_to_type(const javacall_utf16* mime, long length)
{
    javacall_media_type ret = JAVACALL_END_OF_TYPE;
    char* cMime = MALLOC(length + 1);

    if (cMime) {
        int wres = WideCharToMultiByte(CP_ACP, 0, mime, length, 
            cMime, length + 1, NULL, NULL);

        if (0 != wres) {
            cMime[length] = 0;
            JAVA_DEBUG_PRINT1("javautil_media_mime_to_type %s\n", cMime);

            if (0 == strcmp(JAVACALL_AUDIO_MIDI_MIME, cMime)) {
                ret = JAVACALL_AUDIO_MIDI;
            } else if (0 == strcmp(JAVACALL_AUDIO_MIDI_MIME_2, cMime)) {
                ret = JAVACALL_AUDIO_MIDI;
            } else if (0 == strcmp(JAVACALL_AUDIO_SP_MIDI_MIME, cMime)) {
                ret = JAVACALL_AUDIO_MIDI;
            } else if (0 == strcmp(JAVACALL_AUDIO_WAV_MIME, cMime)) {
                ret = JAVACALL_AUDIO_WAV;
            } else if (0 == strcmp(JAVACALL_AUDIO_MP3_MIME, cMime)) {
                ret = JAVACALL_AUDIO_MP3;
            } else if (0 == strcmp(JAVACALL_AUDIO_TONE_MIME, cMime)) {
                ret = JAVACALL_AUDIO_TONE;
            } else if (0 == strcmp(JAVACALL_DEVICE_TONE_MIME, cMime)) {
                ret = JAVACALL_AUDIO_TONE;
            } else if (0 == strcmp(JAVACALL_DEVICE_MIDI_MIME, cMime)) {
                ret = JAVACALL_INTERACTIVE_MIDI;
            } else if (0 == strcmp(JAVACALL_VIDEO_MPEG4_MIME_2, cMime)) {
                ret = JAVACALL_VIDEO_MPEG4;
            } else if (0 == check_prefix(JAVACALL_CAPTURE_VIDEO_MIME, cMime)) {
                ret = JAVACALL_CAPTURE_VIDEO;
            } else if (0 == check_prefix(JAVACALL_CAPTURE_AUDIO_MIME, cMime)) {
                ret = JAVACALL_CAPTURE_AUDIO;
            }
        }
        FREE(cMime);
    }

    return ret;
}

/* Native Implementation Functions */
/*****************************************************************************/

/**
 * Send event to external event queue
 * This function is a sample implementation for Win32
 */
static void jmmpSendEvent(int type, int param1, int param2)
{
#if 0
    /* This memory SHOULD be deallocated from event handler */
    int* pParams = (int*)pcsl_mem_malloc(sizeof(int) * 2);

    if (pParams) {
        pParams[0] = param1;
        pParams[1] = param2;
        PostMessage(GET_MCIWND_HWND(), WM_MEDIA, type, (LPARAM)pParams);
    }
#endif
}

/**
 * Get multimedia capabilities of the device.
 * This function should return pointer to static array of javacall_media_caps value
 * The last item of javacall_media_caps array should hold NULL mimeType value
 * Java layer will use this NULL value as a end of item mark
 */
const javacall_media_caps* javacall_media_get_caps() 
{
    return _media_caps;
}

/**
 * Query whether audio mixing is supported or not
 */
javacall_bool javacall_media_supports_mixing()
{
    return JAVACALL_FALSE;
}

/**
 * Native player create. 
 * This function create internal information structure that will be used from other native API.
 */
javacall_handle javacall_media_create(javacall_int64 playerId, 
                                      const javacall_utf16* mime, long mimeLength,
                                      const javacall_utf16* uri, long uriLength,
                                      long contentLength)
{
    javacall_media_type mediaType;
    native_handle* pHandle = NULL;
    media_interface* pItf;

    pHandle = MALLOC(sizeof(native_handle));
    if (NULL == pHandle) return NULL;

    /* Mime type string to type constants */
    mediaType = javautil_media_mime_to_type(mime, mimeLength);
    if (JAVACALL_END_OF_TYPE == mediaType) {
        JAVA_DEBUG_PRINT1("javacall_media_create fail %d\n", mediaType);
        FREE(pHandle);
        return NULL;
    }

    /* Query interface table */
    pItf = _itfTable[mediaType];
    if (NULL == pItf) {
        FREE(pHandle);
        return NULL;
    }

    JAVA_DEBUG_PRINT2("javacall_media_create %d %x\n", mediaType, pItf);

    if (QUERY_BASIC_ITF(pItf, create)) {
        javacall_handle handle = pItf->vptrBasic->create(
            playerId, mediaType, uri, contentLength);
        if (NULL == handle) {
            FREE(pHandle);
            return NULL;
        }
        pHandle->mediaType = mediaType;
        pHandle->mediaHandle = handle;
        pHandle->meidaItfPtr = pItf;
    } else {
        JAVA_DEBUG_PRINT("QUERY_BASIC_ITF FAIL\n");
    }

    return (javacall_handle)pHandle;
}

/**
 * Testing purpose API
 */
javacall_handle javacall_media_create2(int playerId, javacall_media_type mediaType, 
                                       const javacall_utf16* fileName, 
                                       int fileNameLength) 
{
    return NULL;
}

/**
 * 
 */
javacall_result javacall_media_close(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, destroy)) {
        ret = pItf->vptrBasic->destroy(pHandle->mediaHandle);
    }

    if (pHandle) {
        pHandle->meidaItfPtr = NULL;
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, release_device)) {
        ret = pItf->vptrBasic->release_device(pHandle->mediaHandle);
    }
    return ret;
}

/**
 * Is this protocol handled by device? If yes return JAVACALL_OK.
 */
javacall_result javacall_media_protocol_handled_by_device(javacall_handle handle)
{
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, protocol_handled_by_device)) {
        ret = pItf->vptrBasic->protocol_handled_by_device(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Store media data to temp file (except JTS type)
 */
long javacall_media_do_buffering(javacall_handle handle, 
                                 const void* buffer, long length, long offset)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, do_buffering)) {
        ret = pItf->vptrBasic->do_buffering(
            pHandle->mediaHandle, buffer, length, offset);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, clear_buffer)) {
        ret = pItf->vptrBasic->clear_buffer(pHandle->mediaHandle);
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, resume)) {
        ret = pItf->vptrBasic->resume(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Get current position
 */
long javacall_media_get_time(javacall_handle handle)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_time)) {
        ret = pItf->vptrBasic->get_time(pHandle->mediaHandle);
    }

    return ret;
}

/**
 * Set current position
 */
long javacall_media_set_time(javacall_handle handle, long ms)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, set_time)) {
        ret = pItf->vptrBasic->set_time(pHandle->mediaHandle, ms);
    }

    return ret;
}

/**
 * Get media duration
 */
long javacall_media_get_duration(javacall_handle handle)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, get_duration)) {
        ret = pItf->vptrBasic->get_duration(pHandle->mediaHandle);
    }

    return ret;
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
javacall_result javacall_media_to_foreground(javacall_handle handle,
                                             javacall_media_mvm_option option) {
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_foreground)) {
        ret = pItf->vptrBasic->switch_to_foreground(pHandle->mediaHandle, option);
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
                                             javacall_media_mvm_option option) {
    javacall_result ret = JAVACALL_FAIL;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_BASIC_ITF(pItf, switch_to_background)) {
        ret = pItf->vptrBasic->switch_to_background(pHandle->mediaHandle, option);
    }

    return ret;
}

/* VolumeControl Functions ************************************************/

/**
 *
 */
long javacall_media_get_volume(javacall_handle handle)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, get_volume)) {
        ret = pItf->vptrVolume->get_volume(pHandle->mediaHandle);
    }

    return ret;
}

/**
 *
 */
long javacall_media_set_volume(javacall_handle handle, long level)
{
    long ret = -1;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_volume)) {
        ret = pItf->vptrVolume->set_volume(pHandle->mediaHandle, level);
    }

    return ret;
}

/**
 *
 */
javacall_bool javacall_media_is_mute(javacall_handle handle)
{
    javacall_bool ret = JAVACALL_FALSE;
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, is_mute)) {
        ret = pItf->vptrVolume->is_mute(pHandle->mediaHandle);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_VOLUME_ITF(pItf, set_mute)) {
        ret = pItf->vptrVolume->set_mute(pHandle->mediaHandle, mute);
    }

    return ret;
}

/* VideoControl Functions ************************************************/

/**
 * 
 */
javacall_result javacall_media_set_video_alpha(javacall_bool on, 
                                               javacall_pixel color) {
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_VIDEO_ITF(pItf, set_video_location)) {
        ret = pItf->vptrVideo->set_video_location(pHandle->mediaHandle, x, y, w, h);
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_SNAPSHOT_ITF(pItf, get_video_snapshot_data)) {
        ret = pItf->vptrSnapshot->get_video_snapshot_data(
            pHandle->mediaHandle, buffer, size);
    }

    return ret;
}

/* Simple Tone Play Functions */
/*****************************************************************************/

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
javacall_result javacall_media_play_tone(long note, long duration, long volume)
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_MIDI_ITF(pItf, get_channel_volume)) {
        ret = pItf->vptrMidi->get_channel_volume(pHandle, channel, volume);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_channel_volume)) {
        ret = pItf->vptrMidi->set_channel_volume(pHandle, channel, volume);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_MIDI_ITF(pItf, set_program)) {
        ret = pItf->vptrMidi->set_program(pHandle, channel, bank, program);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_MIDI_ITF(pItf, short_midi_event)) {
        ret = pItf->vptrMidi->short_midi_event(handle, type, data1, data2);
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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_MIDI_ITF(pItf, long_midi_event)) {
        ret = pItf->vptrMidi->long_midi_event(handle, data, offset, length);
    }

    return ret;
}

/* Record Control functions */
/*****************************************************************************/

/**
 * Query if recording is supported based on the player's content-type
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_supports_recording(javacall_handle handle) {
    native_handle* pHandle = (native_handle*)handle;
    media_interface* pItf = pHandle->meidaItfPtr;

    if (pItf->vptrRecord != NULL) {
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

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
    media_interface* pItf = pHandle->meidaItfPtr;

    if (QUERY_RECORD_ITF(pItf, recording_handled_by_native)) {
        ret = pItf->vptrRecord->recording_handled_by_native(
            pHandle->mediaHandle, locator);
    }

    return ret;
}

/**
 * Is javacall_media_set_recordsize_limit function is working for this player?
 * 
 * @retval JAVACALL_TRUE    Yes. Supported.
 * @retval JAVACALL_FALSE   No. Not supported.
 */
javacall_bool javacall_media_set_recordsize_limit_supported(javacall_handle handle) {
    return JAVACALL_FALSE;
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
    media_interface* pItf = pHandle->meidaItfPtr;

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
int javacall_media_get_record_content_type_length(javacall_handle handle) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};

    return sizeof(contentType) / sizeof(*contentType); 
}

/**
 * Get the current recording data content type mime string length
 * For example : 'audio/x-wav' for audio recording
 *
 * @param handle                Handle of native player
 * @param contentTypeBuf        Buffer to return content type Unicode string
 * @param contentTypeBufLength  Length of contentTypeBuf buffer (in Unicode metrics)
 *
 * @return  Length of content type string stored in contentTypeBuf
 */
int javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           int contentTypeBufLength) {
    javacall_utf16 contentType[] = {'a','u','d','i','o','/','x','-','w','a','v'};
    memcpy(contentTypeBuf, contentType, sizeof(contentType));

    return sizeof(contentType) / sizeof(*contentType);
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
    media_interface* pItf = pHandle->meidaItfPtr;

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
                                                long index, long bufLength,
                                                char* buf)
{
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_metadata(javacall_handle handle, 
                                            const char* key, long bufLength, 
                                            char* buf)
{
    return JAVACALL_FAIL;
}

/* RateControl functions ***********************************************************/

javacall_result javacall_media_get_max_rate(javacall_handle handle, long* maxRate)
{
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_min_rate(javacall_handle handle, long* minRate)
{
    return JAVACALL_FAIL;
}

javacall_result javacall_media_set_rate(javacall_handle handle, long rate)
{
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_rate(javacall_handle handle, long* rate)
{
    return JAVACALL_FAIL;
}

/* TempoControl functions ***********************************************************/

javacall_result javacall_media_get_tempo(javacall_handle handle, /*OUT*/ long* tempo) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_set_tempo(javacall_handle handle, long tempo) {
    return JAVACALL_FAIL;
}

/* PitchControl functions ***********************************************************/

javacall_result javacall_media_get_max_pitch(javacall_handle handle, /*OUT*/ long* maxPitch) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_min_pitch(javacall_handle handle, /*OUT*/ long* minPitch) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_set_pitch(javacall_handle handle, long pitch) {
    return JAVACALL_FAIL;
}

javacall_result javacall_media_get_pitch(javacall_handle handle, /*OUT*/ long* pitch) {
    return JAVACALL_FAIL;
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


#define JFIF_HEADER_MAXIMUM_LENGTH 1024
/**
 * Encodes given raw RGB888 image to specified format.
 * 
 * @param rgb888        [IN] soure raw image to be encoded
 * @param width         [IN] source image width
 * @param height        [IN] source image height
 * @param encode        [IN]destination format
 * @param quality       [IN]quality of encoded image (for format
 *                      with losses)
 * @param result_buffer [OUT]a pointer where result buffer will
 *                      be stored
 * @param result_buffer_len [OUT] a pointer for result buffer
 *                          size
 * @param context       [OUT] a context saved during
 *                      asynchronous operation
 * 
 * @return  JAVACALL_OK  in case of success,
 *          JAVACALL_OUT_OF_MEMORY if there is no memory for
 *          destination buffer
 *          JAVACALL_FAIL if encoder failed
 *          JAVACALL_WOULD_BLOCK if operation requires time to
 *          complete, an application should call
 *          <tt>javacall_media_encode_finish</tt> to get result
 */
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

/**
 * Finish encode procedure for given raw RGB888 image.
 * 
 * @param result_buffer [OUT]a pointer where result buffer will
 *                      be stored
 * @param result_buffer_len [OUT] a pointer for result buffer
 *                          size
 * @param context       [OUT] a context saved during
 *                      asynchronous operation
 * 
 * @return  JAVACALL_OK  in case of success,
 *          JAVACALL_OUT_OF_MEMORY if there is no memory for
 *          destination buffer
 *          JAVACALL_FAIL if encoder failed
 *          JAVACALL_WOULD_BLOCK if operation requires time to
 *          complete, an application should call
 *          <tt>javacall_media_encode_finish</tt> to get result
 */
javacall_result javacall_media_encode_finish(javacall_handle context,
                                             javacall_uint8** result_buffer, javacall_uint32* result_buffer_len) {
    // should never be called
    return JAVACALL_FAIL;
}

/**
 * Release a data was acuired by <tt>javacall_media_encode</tt>
 * 
 * @param result_buffer     a pointer to a buffer need to be
 *                          released
 * @param result_buffer_len the buffer length
 */
void javacall_media_release_data(javacall_uint8* result_buffer, javacall_uint32 result_buffer_len) {
    javacall_free(result_buffer);
}

/**
 * Get current system audio volume level.
 * Audio volume range have to be in 0 to 100 inclusive. 0 means that audio is
 * muted.
 *
 * @note Player's volume level will be multiplied by the system volume 
 *       (divided by 100) before the javacall_media_set_volume() method will be 
 *       called by the Java layer. To block this calculation 
 *       the javacall_media_get_system_volume() method must return 
 *       JAVACALL_NO_DATA_AVAILABLE.
 *
 * @note If the device have a system mute/unmute capability 
 *       the Javacall layer is responsible for saving/restoring the current 
 *       system volume level when the audio is muted/unmuted.
 * 
 * @note This method must return JAVACALL_NO_DATA_AVAILABLE when the 
 *       System volume feature is not implemented.
 *
 * @param volume        Volume value
 *
 * @retval JAVACALL_OK                Success
 * @retval JAVACALL_NO_DATA_AVAILABLE System volume is not available
 */
javacall_result javacall_media_get_system_volume(/*OUT*/ javacall_int32 *volume) {
    return JAVACALL_NO_DATA_AVAILABLE;
}

