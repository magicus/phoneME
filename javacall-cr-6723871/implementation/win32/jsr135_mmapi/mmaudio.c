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
 
#include "multimedia.h"
#include <windows.h>
#include <vfw.h>
#include <tchar.h>

#define TIMER_CALLBACK_DURATION 500

/* Native porting layer context */
typedef struct {
    HWND                hWnd;
    javacall_int64      playerId;
    UINT                timerId;
    long                duration;
    long                curTime;
    long                offset;
    javacall_media_type mediaType;
    javacall_bool       isForeground;
    TCHAR               fileName[MAX_PATH * 2];
} audio_handle;

/**********************************************************************************/

/**
 * Create MCI window to play audio data
 */
static void audio_prepare_MCIWnd(audio_handle* pHandle){

    /* JTS type don't use MCI */
    if (JAVACALL_AUDIO_TONE == pHandle->mediaType) return;

    if (NULL == pHandle->hWnd) {
        pHandle->hWnd = MCIWndCreate(GET_MCIWND_HWND(), GetModuleHandle(NULL),
            MCIWNDF_NOERRORDLG | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR, 
            pHandle->fileName);

        if (NULL == pHandle->hWnd) {
            JAVA_DEBUG_PRINT("[jc_media] MCIWndCreate fail\n");
        } else {
            /* Set time base to milli-second */
            MCIWndSetTimeFormat(pHandle->hWnd, "ms");
        }
    }
}

/**
 * Timer callback used to check about end of media (except JTS type)
 */
static void CALLBACK audio_timer_callback(UINT uID, UINT uMsg, 
                                          DWORD dwUser, 
                                          DWORD dw1, 
                                          DWORD dw2) {
    audio_handle* pHandle = (audio_handle*)dwUser;

    if (pHandle->hWnd) {
        if (-1 == pHandle->duration) {
            pHandle->duration = MCIWndGetLength(pHandle->hWnd);
            JAVA_DEBUG_PRINT1("[jc_media] audio_timer_callback %d\n", 
                pHandle->duration);
        }

        pHandle->offset = MCIWndGetPosition(pHandle->hWnd);
        pHandle->curTime = pHandle->offset;
        
        /* Is end of media reached? */
        if (pHandle->offset >= pHandle->duration) {
            /* Post EOM event to Java and kill player timer */
            pHandle->timerId = 0;
            pHandle->offset = 0;
            timeKillEvent(uID);
            JAVA_DEBUG_PRINT1("[jc_media] javanotify_on_media_notification %d\n", 
                pHandle->playerId);

            javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA, 
                pHandle->playerId, (void*)pHandle->duration);
        }
    }

}

/**
 * Get temporary file name
 */
static int audio_get_temp_file_name(javacall_media_type mediaType, 
                                    char* buffer){

    static int index = 1;
    char tempPath[MAX_PATH];
    char* extension = NULL;

    GetTempPath(MAX_PATH, tempPath);

    switch(mediaType) {
    case JAVACALL_AUDIO_MIDI:
        extension = "mid";
        break;
    case JAVACALL_AUDIO_WAV:
        extension = "wav";
        break;
    case JAVACALL_AUDIO_MP3:
        extension = "mp3";
        break;
    }

    JAVA_DEBUG_PRINT2("[jc_media] audio_get_temp_file_name %d %s\n", 
                      mediaType, extension);

    if (extension) {
        wsprintf(buffer, "%stmp%d.%s", tempPath, index++, extension);
        return 1;
    } else {
        return 0;
    }

}

/*******************************************************************************/

/**
 * 
 */
static javacall_handle audio_create(javacall_int64 playerId, 
                                    javacall_media_type mediaType, 
                                    const javacall_utf16* URI, 
                                    long contentLength){

    audio_handle* pHandle = MALLOC(sizeof(audio_handle));
    if (NULL == pHandle) {
        return NULL;
    }

    if (0 == audio_get_temp_file_name(mediaType, pHandle->fileName)) {
        JAVA_DEBUG_PRINT1("[jc_media] javacall_media_create fail %s\n", 
                          pHandle->fileName);
        FREE(pHandle);
        return NULL;
    }

    pHandle->hWnd = NULL;
    pHandle->offset = 0;
    pHandle->duration = -1;
    pHandle->curTime = 0;
    pHandle->playerId = playerId;
    pHandle->timerId = 0;
    pHandle->isForeground = JAVACALL_TRUE;
    pHandle->mediaType = mediaType;

    return pHandle;
}

/**
 * 
 */
static javacall_result audio_close(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;

    if (NULL != pHandle->hWnd) {
        MCIWndClose(pHandle->hWnd);
        MCIWndDestroy(pHandle->hWnd);
        pHandle->hWnd = NULL;
    }

    if (handle) {
        FREE(handle);
    }

    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result audio_destroy(javacall_handle handle){
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result audio_acquire_device(javacall_handle handle){
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result audio_release_device(javacall_handle handle){
    return JAVACALL_OK;
}

/**
 * Store media data to temp file (except JTS type)
 */
static long audio_do_buffering(javacall_handle handle, const void* buffer, 
                        long length, long offset){

    audio_handle* pHandle = (audio_handle*)handle;
    HANDLE hFile;

    if (NULL == buffer) {
        return 0;
    }

    hFile = CreateFile(pHandle->fileName, GENERIC_READ | GENERIC_WRITE, 
               FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD dwWritten;
        SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
        if (0 == WriteFile(hFile, buffer, length, &dwWritten, NULL)) {
            CloseHandle(hFile);
            return -1;
        } else {
            CloseHandle(hFile);
            return (int)dwWritten;
        }
    }

    return -1;
}

/**
 * Delete temp file
 */
static javacall_result audio_clear_buffer(javacall_handle hLIB){

    audio_handle* pHandle = (audio_handle*)hLIB;

    /* Reset offset */
    pHandle->offset = 0;
    DeleteFile(pHandle->fileName);
    
    return JAVACALL_OK;
}


/**
 * 
 */
static javacall_result audio_start(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;

    if (JAVACALL_FALSE == pHandle->isForeground) {
        JAVA_DEBUG_PRINT("[jc_media] Audio fake start in background!\n");
        return JAVACALL_OK;
    }
    
    /* Create MCI window by using temp file */
    audio_prepare_MCIWnd(pHandle);

    JAVA_DEBUG_PRINT1("[jc_media] + javacall_media_start %s\n", pHandle->fileName);

    /* Seek to play position */
    if (pHandle->offset) {
        MCIWndSeek(pHandle->hWnd, pHandle->offset);
        pHandle->curTime = pHandle->offset;
    } else {
        pHandle->curTime = 0;
    }

    /* Start play */
    if (pHandle->hWnd && 0 == MCIWndPlay(pHandle->hWnd)) {
        JAVA_DEBUG_PRINT("[jc_media] - javacall_media_start OK\n");
        pHandle->duration = MCIWndGetLength(pHandle->hWnd);
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DURATION_UPDATED, 
            pHandle->playerId, (void*)pHandle->duration);
        pHandle->timerId = 
            (UINT)timeSetEvent(TIMER_CALLBACK_DURATION, 100, 
            audio_timer_callback,(DWORD)pHandle, TIME_PERIODIC);
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/**
 * 
 */
static javacall_result audio_stop(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    audio_prepare_MCIWnd(pHandle);

    if (pHandle->hWnd) {
        /* Get stopped position for later use */
        pHandle->offset = MCIWndGetPosition(pHandle->hWnd);
        if (pHandle->offset >= MCIWndGetLength(pHandle->hWnd)) {
            pHandle->offset = 0;
        }
        /* Kill player timer */
        if (pHandle->timerId) {
            timeKillEvent(pHandle->timerId);
            pHandle->timerId = 0;
        }
        MCIWndStop(pHandle->hWnd);
    }
    
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result audio_pause(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    audio_prepare_MCIWnd(pHandle);

    if (pHandle->hWnd) {
        /* Kill player timer */
        if (pHandle->timerId) {
            timeKillEvent(pHandle->timerId);
            pHandle->timerId = 0;
        }
        MCIWndPause(pHandle->hWnd);
    }

    return JAVACALL_FAIL;
}

/**
 * 
 */
static javacall_result audio_resume(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    audio_prepare_MCIWnd(pHandle);

    if (pHandle->hWnd) {
        pHandle->timerId = (UINT)timeSetEvent(500, 100, 
            audio_timer_callback,(DWORD)pHandle, TIME_PERIODIC);
        MCIWndResume(pHandle->hWnd);
    }

    return JAVACALL_FAIL;
}

/**
 * 
 */
static long audio_get_time(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;

    if (pHandle->timerId && pHandle->hWnd) {
        pHandle->curTime = MCIWndGetPosition(pHandle->hWnd);
    }

    return pHandle->curTime;
}

/**
 * 
 */
static long audio_set_time(javacall_handle handle, long ms){

    audio_handle* pHandle = (audio_handle*)handle;
    audio_prepare_MCIWnd(pHandle);

    if (pHandle->hWnd) {
        long real_ms = MCIWndSeek(pHandle->hWnd, ms);
        pHandle->offset = ms;
        return ms;
    }

    return -1;
}
 
/**
 * 
 */
static long audio_get_duration(javacall_handle handle) {
    audio_handle* pHandle = (audio_handle*)handle;
    return pHandle->duration;
}

/**
 * Now, switch to foreground
 */
static javacall_result audio_switch_to_foreground(javacall_handle handle, 
                                                  int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
static javacall_result audio_switch_to_background(javacall_handle handle, 
                                                  int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_FALSE;

    /* Stop the current playing */
    audio_stop(handle);

    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

/* IMPL_NOTE - FAKE stubs : Do nothing */

static long _volume = 50;
static long _mute = 0;

/**
 *
 */
static long audio_get_volume(javacall_handle handle) {
    return _volume;
}

/**
 *
 */
static long audio_set_volume(javacall_handle handle, long level) {
    _volume = level;
    return _volume;
}

/**
 *
 */
static javacall_bool audio_is_mute(javacall_handle handle) {
    return _mute == 0 ? JAVACALL_FALSE : JAVACALL_TRUE;
}

/**
 *
 */
static javacall_result audio_set_mute(javacall_handle handle, 
                                      javacall_bool mute){
    _mute = mute;
    return JAVACALL_TRUE;
}

/*******************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _audio_basic_itf = {
    audio_create,
    audio_close,
    audio_destroy,
    audio_acquire_device,
    audio_release_device,
    audio_start,
    audio_stop,
    audio_pause,
    audio_resume,
    audio_do_buffering,
    audio_clear_buffer,
    audio_get_time,
    audio_set_time,
    audio_get_duration,
    NULL,
    audio_switch_to_foreground,
    audio_switch_to_background
};

/**
 * Audio volume javacall function interface
 */
static media_volume_interface _audio_volume_itf = {
    audio_get_volume,
    audio_set_volume,
    audio_is_mute,
    audio_set_mute
};

/*******************************************************************************/
 
/* Global audio interface */
media_interface g_audio_itf = {
    &_audio_basic_itf,
    &_audio_volume_itf,
    NULL,
    NULL
}; 

