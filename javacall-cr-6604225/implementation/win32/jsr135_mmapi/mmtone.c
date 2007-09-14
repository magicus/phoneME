/*
 * @file mmtone.c 
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

#define JTS_SILENCE         -1
#define JTS_VERSION         -2
#define JTS_TEMPO           -3
#define JTS_RESOLUTION      -4
#define JTS_BLOCK_START     -5
#define JTS_BLOCK_END       -6
#define JTS_PLAY_BLOCK      -7
#define JTS_SET_VOLUME      -8
#define JTS_REPEAT          -9
#define JTS_C4              60

/**
 * Tone player handle
 */
typedef struct {
    javacall_int64  playerId;
    int     offset;             /* stopped offset */
    int     currentTime;        /* current playing time */
    char*    pToneBuffer;        /* Pointer to tone data buffer */
    /* Current tone data size that stored to tone buffer in bytes */
    int     toneDataSize;       
    int     totalDuration;
    javacall_bool isForeground; /* Is in foreground? */
    javacall_bool isPlaying;    /* Is playing? */
    javacall_bool stopPlaying;  /* Stop JTS playing thread? */
    HMIDIOUT      hmo;          /* Handle to opened midi output device */
} tone_handle;

/**********************************************************************************/

static BOOL tone_check_sequence( const char* seq, long len )
{
    int t;
    int pos = 0;

    char blk_defined[ 128 ];
    BOOL res_defined = FALSE;
    BOOL tmp_defined = FALSE;
    int  blk         = -1;

    memset( blk_defined, 0, 128 );

    if( len < 2 ) return FALSE;

    while( pos < len )
    {
        switch( t = seq[ pos++ ] )
        {
        case JTS_VERSION:
            if( 1 != seq[ pos ] ) return FALSE;
            pos++;
            break;
        case JTS_TEMPO:
            if( tmp_defined ) return FALSE;
            if( seq[ pos ] < 5 || seq[ pos ] > 127 ) return FALSE;
            tmp_defined = TRUE;
            pos++;
            break;
        case JTS_RESOLUTION:
            if( res_defined ) return FALSE;
            if( seq[ pos ] < 1 || seq[ pos ] > 127 ) return FALSE;
            res_defined = TRUE;
            pos++;
            break;
        case JTS_BLOCK_START:
            if( seq[ pos ] < 0 || seq[ pos ] > 127 ) return FALSE;
            blk = seq[ pos++ ];
            break;
        case JTS_BLOCK_END:
            if( -1 == blk ) return FALSE;
            if( seq[ pos++ ] != blk ) return FALSE;
            blk_defined[ blk ] = 1;
            blk = -1;
            break;
        case JTS_PLAY_BLOCK:
            if( seq[ pos ] < 0 || seq[ pos ] > 127 ) return FALSE;
            if( !blk_defined[ seq[ pos ] ] ) return FALSE;
            pos++;
            break;
        case JTS_SET_VOLUME:
            if( seq[ pos ] < 0 || seq[ pos ] > 100 ) return FALSE;
            pos++;
            break;
        case JTS_REPEAT:
            if( seq[ pos ] < 2 || seq[ pos ] > 127 ) return FALSE;
            pos++;
            break;
        case JTS_SILENCE:
            if( seq[ pos ] < 1 || seq[ pos ] > 127 ) return FALSE;
            pos++;
            break;
        default:
            if( t < 0 || t > 127 ) return FALSE;
            if( seq[ pos ] < 1 || seq[ pos ] > 127 ) return FALSE;
            pos++;
        }
    }

    return TRUE;
}

/**
 * Play tone by using MIDI short message
 */
static void tone_play_sync(tone_handle* pHandle, int note, int duration, int volume)
{
    DWORD msg = 0;

    msg = (((volume & 0xFF) << 16) | (((note & 0xFF) << 8) | 0x91));    
    /* Note on at channel 3 */
    midiOutShortMsg(pHandle->hmo, msg);

    Sleep(duration);
    msg &= 0xFFFFFF81;

    midiOutShortMsg(pHandle->hmo, msg);
}

/**
 * JTS playing thread
 */
static DWORD WINAPI tone_jts_player(void* pArg)
{
    static long volume = 100;   /* to reserve last volume */

    int i;
    long note;
    tone_handle* pHandle = (tone_handle*)pArg;
    long duration, totalDuration = pHandle->currentTime;
    char* pTone = pHandle->pToneBuffer;

    for(i = pHandle->offset; i < pHandle->toneDataSize; i += 2) {
        /* JTS playing stopped by external force */
        if (JAVACALL_TRUE == pHandle->stopPlaying) {
            /* Store stopped offset to start from stopped position later */
            pHandle->offset = i;
            break;
        }
        note = pTone[i];
        switch(note) {
        case JAVACALL_SET_VOLUME:
            volume = pTone[i + 1];
            break;
        case JAVACALL_SILENCE:
            duration = pTone[i + 1];
            totalDuration += duration;
            Sleep(duration);
            break;
        /* Note */
        default:
            duration = pTone[i + 1];
            duration = max(200, duration);
            totalDuration += duration;
            tone_play_sync(pHandle, note, duration, volume);
            break;
        }

        pHandle->currentTime = totalDuration;

    }

    JAVA_DEBUG_PRINT2("tone_jts_player END id=%d stopped=%d\n", 
        (int)pHandle->playerId, pHandle->stopPlaying);

    /* JTS loop ended not by stop => Post EOM event */
    if (JAVACALL_FALSE == pHandle->stopPlaying) {
        pHandle->totalDuration = totalDuration;
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DURATION_UPDATED, 
            pHandle->playerId, (void*)totalDuration);
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA, 
            pHandle->playerId, (void*)totalDuration);
        pHandle->offset = 0;
    }
    
    pHandle->stopPlaying = JAVACALL_FALSE;
    pHandle->isPlaying = JAVACALL_FALSE;  /* Now, stopped */

    return 0;
}

/**********************************************************************************/

/**
 * Create tone native player handle
 */
static javacall_handle tone_create(javacall_int64 playerId, 
                                   javacall_media_type mediaType,
                                   const javacall_utf16* URI, 
                                   long contentLength)
{
    tone_handle* pHandle = MALLOC(sizeof(tone_handle));
    if (NULL == pHandle) {
        return NULL;
    }

    pHandle->playerId = playerId;
    pHandle->currentTime = 0;
    pHandle->offset = 0;
    pHandle->pToneBuffer = NULL;
    pHandle->toneDataSize = 0;
    pHandle->isPlaying = JAVACALL_FALSE;
    pHandle->isForeground = JAVACALL_TRUE;
    pHandle->stopPlaying = JAVACALL_FALSE;
    pHandle->hmo = NULL;
    pHandle->totalDuration = -1;

    return pHandle;
}

/**
 * Close tone native player handle
 */
static javacall_result tone_close(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;

    if (pHandle->pToneBuffer) {
        FREE(pHandle->pToneBuffer);
        pHandle->pToneBuffer = NULL;
    }
    pHandle->toneDataSize = 0;
    pHandle->offset = 0;
    pHandle->currentTime = 0;
    pHandle->totalDuration = -1;
    javacall_close_midi_out(&pHandle->hmo);
    
    FREE(pHandle);

    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_destroy(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_acquire_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_release_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * Store media data to temp file (except JTS type)
 * NOTE: JTS data always transfered at one time.
 */
static long tone_do_buffering(javacall_handle handle, 
                              const void* buffer, long length, long offset)
{
    tone_handle* pHandle = (tone_handle*)handle;

    if (NULL == buffer) {
        return 0;
    }

    if( !tone_check_sequence( (const char*)buffer, length ) ) return -1;

    if (NULL != pHandle->pToneBuffer) {
        FREE(pHandle->pToneBuffer);
    }

    pHandle->pToneBuffer = MALLOC(length);
    if (NULL == pHandle->pToneBuffer) {
        return -1;
    }
    
    /* Store tone data to buffer */
    memcpy(pHandle->pToneBuffer, buffer, length);
    pHandle->toneDataSize = length;

    return length;
}

/**
 * Delete temp file
 */
static javacall_result tone_clear_buffer(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;

    if (pHandle->pToneBuffer) {
        FREE(pHandle->pToneBuffer);
        pHandle->pToneBuffer = NULL;
        pHandle->toneDataSize = 0;
        pHandle->offset = 0;
        pHandle->currentTime = 0;
        pHandle->totalDuration = -1;
    }

    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_start(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;

    /* Fake playing */
    if (JAVACALL_FALSE == pHandle->isForeground) {
        return JAVACALL_OK;
    }

    if (pHandle->hmo == NULL) 
        javacall_open_midi_out(&pHandle->hmo, JAVACALL_FALSE);

    if (pHandle->hmo == NULL) 
        return JAVACALL_FAIL;

    /* Create Win32 thread to play JTS data - non blocking */
    if (pHandle->toneDataSize) {
        pHandle->stopPlaying = JAVACALL_FALSE;
        if (NULL != CreateThread(NULL, 0, tone_jts_player, pHandle, 0, NULL)) {
            JAVA_DEBUG_PRINT1("tone_start started id=%d\n", (int)pHandle->playerId);
            pHandle->isPlaying = JAVACALL_TRUE;
            return JAVACALL_OK;
        }
        else
        {
            printf("tone_start: failed\n");
            return JAVACALL_FAIL;
        }
    }
    else
    {
        printf("tone_start: no data\n");
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA, 
            pHandle->playerId, (void*)0 );
    }
    
    return JAVACALL_OK;
}

/**
 * Stop JTS playing
 */
static javacall_result tone_stop(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;
    
    if (JAVACALL_FALSE == pHandle->isPlaying) {
        return JAVACALL_OK;
    }

    /* Stop playing */
    pHandle->stopPlaying = JAVACALL_TRUE;

    /* Wait until thread exit */
    while(1) {
        if (JAVACALL_FALSE == pHandle->isPlaying) {
            break;
        }
        Sleep(100);  /* Wait 100 ms */
    }
   
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_pause(javacall_handle handle)
{
    return tone_stop(handle);
}

/**
 * 
 */
static javacall_result tone_resume(javacall_handle handle)
{
    return tone_start(handle);
}

/**
 * 
 */
static long tone_get_time(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;
    return pHandle->currentTime;
}

/**
 * Set to ms position
 */
static long tone_set_time(javacall_handle handle, long ms)
{
    tone_handle* pHandle = (tone_handle*)handle;
    char* pTone = pHandle->pToneBuffer;
    int i;
    int note;
    int totalDuration = 0;
    javacall_bool needRestart = JAVACALL_FALSE;

    /* There is no tone data */
    if (0 == pHandle->toneDataSize) {
        return -1;
    }

    /* If playing, stop it */
    if (JAVACALL_TRUE == pHandle->isPlaying) {
        tone_stop(handle);
        needRestart = JAVACALL_TRUE;
    }
    
    pHandle->offset = 0;    /* init to zero */

    for(i = 0; i < pHandle->toneDataSize; i += 2) {
        note = pTone[i];
        switch(note) {
        case JAVACALL_SET_VOLUME:
            break;
        case JAVACALL_SILENCE:
            totalDuration += pTone[i + 1];
            break;
        default:
            totalDuration += pTone[i + 1];
            break;
        }

        if (totalDuration >= ms) {
            pHandle->currentTime = totalDuration;
            pHandle->offset = i;    /* Set start offset */
            break;
        }
    }

    /* Restart? */
    if (JAVACALL_TRUE == needRestart) {
        tone_start(handle);
    }

    return totalDuration;
}
 
/**
 * 
 */
static long tone_get_duration(javacall_handle handle)
{
    tone_handle* pHandle = (tone_handle*)handle;
    return pHandle->totalDuration;
}


/**
 * Now, switch to foreground
 */
static javacall_result tone_switch_to_foreground(javacall_handle handle, int options) {
    tone_handle* pHandle = (tone_handle*)handle;
    pHandle->isForeground = JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
static javacall_result tone_switch_to_background(javacall_handle handle, int options) {
    tone_handle* pHandle = (tone_handle*)handle;
    pHandle->isForeground = JAVACALL_FALSE;

    /* Stop the current playing */
    tone_stop(handle);

    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

static long _shadowLevel = 50;

/**
 *
 */
static long tone_get_volume(javacall_handle handle)
{
    return _shadowLevel;
}

/**
 *
 */
static long tone_set_volume(javacall_handle handle, long level)
{
    _shadowLevel = level;
    return level;
}

/**
 *
 */
static javacall_bool tone_is_mute(javacall_handle handle)
{
    return (tone_get_volume(handle) == 0 ? JAVACALL_TRUE : JAVACALL_FALSE);
}

/**
 *
 */
static javacall_result tone_set_mute(javacall_handle handle, javacall_bool mute)
{
    static LONG old_volume = 0;

    if (mute) {
        if (0 == old_volume) {
            old_volume = tone_get_volume(handle);
        }
        tone_set_volume(handle, 0);
    } else {
        if (0 != old_volume) {
            tone_set_volume(handle, old_volume);
        }
        old_volume = 0;
    }
    
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _tone_basic_itf = {
    tone_create,
    tone_close,
    tone_destroy,
    tone_acquire_device,
    tone_release_device,
    tone_start,
    tone_stop,
    tone_pause,
    tone_resume,
    tone_do_buffering,
    tone_clear_buffer,
    tone_get_time,
    tone_set_time,
    tone_get_duration,
    NULL,
    tone_switch_to_foreground,
    tone_switch_to_background
};

/**
 * Audio volume javacall function interface
 */
static media_volume_interface _tone_volume_itf = {
    tone_get_volume,
    tone_set_volume,
    tone_is_mute,
    tone_set_mute
};

/**********************************************************************************/
 
/* Global tone interface */
media_interface g_tone_itf = {
    &_tone_basic_itf,
    &_tone_volume_itf,
    NULL,
    NULL
}; 


