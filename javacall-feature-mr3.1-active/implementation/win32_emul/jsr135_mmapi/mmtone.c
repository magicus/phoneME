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
 * 
 */
 
#include "multimedia.h"

#define JTS_SILENCE         -1  // the same as JAVACALL_SILENCE
#define JTS_VERSION         -2  
#define JTS_TEMPO           -3
#define JTS_RESOLUTION      -4
#define JTS_BLOCK_START     -5
#define JTS_BLOCK_END       -6
#define JTS_PLAY_BLOCK      -7
#define JTS_SET_VOLUME      -8  // the same as JAVACALL_SET_VOLUME
#define JTS_REPEAT          -9
#define JTS_DUALTONE        -50
#define JTS_C4              60

#define JTS_STACK_SIZE      32

#define DEF_TEMPO           120
#define DEF_RESOLUTION      64

javacall_result javacall_media_play_dualtone(int appId, long noteA, long noteB, long duration, long volume);

/**
 * Tone player handle
 */
typedef struct {
    int             isolateId;
    int             playerId;
    int             currentTime;   /* current playing time */
    javacall_int8*  pToneBuffer;   /* Pointer to tone data buffer */
    int             toneDataSize;  /* Current tone data size that stored to tone buffer in bytes */
    javacall_bool   isForeground;  /* Is in foreground? */
    javacall_bool   isPlaying;     /* Is playing? */
    javacall_bool   stopPlaying;   /* Stop JTS playing thread? */
    javacall_bool   isMute;        /* is Muted? */
    long            volumeChanged; /* is volume changed? use long-type for Interlocked*() function although it is bool value*/
    long            muteChanged;   /* is mute status changed? use long-type for Interlocked*() function although it is bool value*/
    int             blk[ 128 ];    /* block start offsets */
    long            volume;
    long            tempo;
    long            resolution;
    int             offset;                   /* stopped offset */
    int             stack[ JTS_STACK_SIZE ];  /* return stack for blocks */
    int             sp;                       /* return stack pointer */
} tone_player;

#define TONE_BASE_VOLUME (80) //default playback volume
#define SLEEP_TIME_SLICE 100

/**********************************************************************************/

static int async_sleep(int time, tone_player* pPlayer)
{
    while (time > 0) {
        if (time > SLEEP_TIME_SLICE) {
            Sleep(SLEEP_TIME_SLICE);
            time -= SLEEP_TIME_SLICE;
        } else {
            Sleep(time);
            time = 0;
        }
        if (JAVACALL_TRUE == pPlayer->stopPlaying) {
            return JAVACALL_TRUE;
        }
    }
    return JAVACALL_FALSE;
}
/**
 * Play tone by using MIDI short message
 */
static int tone_play_sync(int appId, int note, int duration, int volume, tone_player* pPlayer)
{
    javacall_media_play_tone(appId, note, duration, volume);
    return async_sleep(duration, pPlayer);
}
/**
 * Play dusltone by using MIDI short message
 */
static int dualtone_play_sync(int appId, int noteA, int noteB, int duration, int volume, tone_player* pPlayer)
{
    javacall_media_play_dualtone(appId, noteA, noteB, duration, volume);
    return async_sleep(duration, pPlayer);
}

/**
 * JTS playing thread
 */
static javacall_result tone_play(tone_player* pPlayer, javacall_bool seek_mode, int ms)
{
    static long volume = TONE_BASE_VOLUME;   /* to reserve last volume */
    int  i, k, note, parm, duration, repcnt;
    int noteA, noteB;
    BOOL defining_block;
    /* Tone data is integer array */
    javacall_int8* pTone = pPlayer->pToneBuffer;
    
    if (seek_mode) {
        pPlayer->offset = 0;
        pPlayer->currentTime = 0;
        pPlayer->tempo      = DEF_TEMPO;
        pPlayer->resolution = DEF_RESOLUTION;
        pPlayer->sp         = 0;
    }
    
    defining_block = FALSE;
    i = pPlayer->offset;

    /* Initialize */
    volume = (pPlayer->isMute? 0:pPlayer->volume);
    pPlayer->volumeChanged = JAVACALL_FALSE;
    pPlayer->muteChanged = JAVACALL_FALSE;
    
    while( i < pPlayer->toneDataSize ) {
        if (seek_mode && (ms >= 0) && (pPlayer->currentTime >= ms)) {
            break;
        }
        /* JTS playing stopped by external force */
        if (JAVACALL_TRUE == pPlayer->stopPlaying) {
            /* Store stopped offset to start from stopped position later */
            break;
        }
        note = pTone[ i++ ];
        parm = pTone[ i++ ];

        switch(note) {
        case JTS_VERSION:
            break;
        case JTS_TEMPO:
            pPlayer->tempo = parm*4;
            break;
        case JTS_RESOLUTION:
            pPlayer->resolution = parm;
            break;
        case JTS_BLOCK_START:
            //JC_MM_ASSERT( !defining_block );
            defining_block = TRUE;
            pPlayer->blk[ parm ] = i;
            break;
        case JTS_BLOCK_END:
            if( pPlayer->sp > 0 ) /* playing block */
            {
                i = pPlayer->stack[ --pPlayer->sp ];
            } else { /* defining block */
                //JC_MM_ASSERT( defining_block );
                defining_block = FALSE;
            }
            break;
        case JTS_PLAY_BLOCK:
            if( !defining_block && pPlayer->sp < JTS_STACK_SIZE ) {
                pPlayer->stack[ pPlayer->sp++ ] = i;
                i = pPlayer->blk[ parm ];
            }
            break;
        case JTS_SET_VOLUME:
            volume = (long)parm;
            break;
        case JTS_REPEAT:
            repcnt = parm;
            note = pTone[ i++ ];
            parm = pTone[ i++ ];
            duration = (parm * 240000 / (pPlayer->resolution * pPlayer->tempo));
            pPlayer->currentTime += duration;
            
            for( k = 0; !seek_mode && (k < repcnt); k++ ) {
                tone_play_sync( pPlayer->isolateId, note, duration, pPlayer->volume, pPlayer );
            }
            break;
        case JTS_SILENCE:
            duration = (parm * 240000 / (pPlayer->resolution * pPlayer->tempo));
            pPlayer->currentTime += duration;
            if (!seek_mode) {
                async_sleep(duration, pPlayer);
            }
            break;
        case JTS_DUALTONE:
            duration = ((parm * 240000) / (pPlayer->resolution * pPlayer->tempo));
            pPlayer->currentTime += duration;
            noteA = pTone[ i++ ];
            noteB = pTone[ i++ ];
            if (!seek_mode) {
                dualtone_play_sync( pPlayer->isolateId, noteA, noteB, duration, pPlayer->volume, pPlayer );
            }
            break;
        /* Note */
        default:
            if (!defining_block) {
                if (note < 0) note = 0;
                if (note > 127) note = 127;
                duration = (parm * 240000 / (pPlayer->resolution * pPlayer->tempo));
                pPlayer->currentTime += duration;
                if (!seek_mode) {
                    if (InterlockedCompareExchange(&(pPlayer->volumeChanged), JAVACALL_FALSE, JAVACALL_TRUE)) {
                        volume = pPlayer->volume;
                    }
                    if (InterlockedCompareExchange(&(pPlayer->muteChanged), JAVACALL_FALSE, JAVACALL_TRUE)) {
                        if (pPlayer->isMute) {
                            pPlayer->volume = volume; //backup current volume
                        } else {
                            volume = pPlayer->volume; //restore the old volume
                        }
                    }
                    if (pPlayer->isMute)  volume = 0;
                    tone_play_sync(pPlayer->isolateId, note, duration, volume, pPlayer);
                }
            }
            break;
        }
        pPlayer->offset = i;
        if (!pPlayer->isMute) pPlayer->volume = volume;
    }
    return JAVACALL_OK;
}

/**
 * JTS playing thread
 */
static DWORD WINAPI tone_jts_player(void* pArg)
{
    tone_player* pPlayer = (tone_player*)pArg;
    tone_play(pPlayer, JAVACALL_FALSE, 0);

    JC_MM_DEBUG_PRINT2("tone_jts_player END id=%d stopped=%d\n", pPlayer->playerId, pPlayer->stopPlaying);

    /* JTS loop ended not by stop => Post EOM event */
    if (JAVACALL_FALSE == pPlayer->stopPlaying) {
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA, pPlayer->isolateId, pPlayer->playerId, 
                                         JAVACALL_OK, (void*)pPlayer->currentTime);
        pPlayer->offset = 0;
    }
    
    pPlayer->stopPlaying = JAVACALL_FALSE;
    pPlayer->isPlaying = JAVACALL_FALSE;  /* Now, stopped */

    return 0;
}

/**********************************************************************************/

/**
 * Create tone native player handle
 */
static javacall_handle tone_create(int appId, int playerId, jc_fmt mediaType, const javacall_utf16_string URI)
{
    tone_player* pPlayer = (tone_player*)MALLOC(sizeof(tone_player));
    if (NULL == pPlayer) {
        JC_MM_DEBUG_PRINTF("tone_create() playerId=0x%X, ERROR! allocating handle!\n", playerId);
        return NULL;
    }

    JC_MM_DEBUG_PRINTF("tone_create() playerId=0x%X, mediaType=%d, URI=0x%X, pPlayer=0x%X\n", 
        (int)playerId, mediaType, URI, pPlayer);

    pPlayer->isolateId     = appId;
    pPlayer->playerId      = playerId;
    pPlayer->currentTime   = 0;
    pPlayer->offset        = 0;
    pPlayer->pToneBuffer   = NULL;
    pPlayer->toneDataSize  = -1;
    pPlayer->isPlaying     = JAVACALL_FALSE;
    pPlayer->isForeground  = JAVACALL_TRUE;
    pPlayer->stopPlaying   = JAVACALL_FALSE;
    pPlayer->volumeChanged = JAVACALL_FALSE;
    pPlayer->muteChanged   = JAVACALL_FALSE;

    pPlayer->isMute        = JAVACALL_FALSE;
    pPlayer->volume        = TONE_BASE_VOLUME;

    //if (MMSYSERR_NOERROR == waveOutGetVolume(hWaveOut, &(pPlayer->volume))){
    //    pPlayer->isMute = (pPlayer->volume == 0) ? JAVACALL_TRUE : JAVACALL_FALSE;
    //    pPlayer->volume = ((pPlayer->volume & 0xFFFF) * 100)/0xFFFF; //convert to java range
    //}

    pPlayer->tempo      = DEF_TEMPO;
    pPlayer->resolution = DEF_RESOLUTION;
    pPlayer->offset     = 0;
    pPlayer->sp         = 0;

    return pPlayer;
}

/**
 * Close tone native player handle
 */
static javacall_result tone_close(javacall_handle handle)
{
    tone_player* pPlayer = (tone_player*)handle;

    JC_MM_DEBUG_PRINTF("tone_close() handle=0x%X\n", handle);

    if (pPlayer->pToneBuffer) {
        FREE(pPlayer->pToneBuffer);
        pPlayer->pToneBuffer = NULL;
    }
    pPlayer->toneDataSize = -1;
    pPlayer->offset       = 0;
    pPlayer->currentTime  = 0;
    pPlayer->tempo        = DEF_TEMPO;
    pPlayer->resolution   = DEF_RESOLUTION;
    pPlayer->sp           = 0;
    
    FREE(pPlayer);

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
    tone_player* pPlayer = (tone_player*)handle;

    return JAVACALL_OK;
}

static javacall_result tone_set_whole_content_size(javacall_handle handle,
                                                   long whole_content_size)
{
    tone_player* pPlayer = (tone_player*)handle;

    pPlayer->toneDataSize = whole_content_size;

    return JAVACALL_OK;
}

static javacall_result tone_get_java_buffer_size(javacall_handle handle,
                                                 long* java_buffer_size,
                                                 long* first_data_size)
{
    tone_player* pPlayer = (tone_player*)handle;
    *java_buffer_size = pPlayer->toneDataSize;
    if (pPlayer->pToneBuffer == NULL) {
        *first_data_size = pPlayer->toneDataSize;
    } else {
        *first_data_size = 0;
    }

    return JAVACALL_OK;
}

static javacall_result tone_get_buffer_address(javacall_handle handle,
                                               const void** buffer,
                                               long* max_size)
{
    tone_player* pPlayer = (tone_player*)handle;

    JC_MM_ASSERT( -1 != pPlayer->toneDataSize );

    pPlayer->pToneBuffer = (javacall_int8*)MALLOC( pPlayer->toneDataSize );
    if( NULL == pPlayer->pToneBuffer )
    {
        JC_MM_DEBUG_PRINTF( "tone_get_buffer_address: cannot allocate %d bytes for tone buffer!\n", 
                            pPlayer->toneDataSize );
        return JAVACALL_OUT_OF_MEMORY;
    }

    *buffer   = pPlayer->pToneBuffer;
    *max_size = pPlayer->toneDataSize;

    return JAVACALL_OK;
}

javacall_result tone_do_buffering(javacall_handle handle, const void* buffer, 
                                  long *length, javacall_bool *need_more_data,
                                  long *min_data_size)
{
    tone_player* pPlayer = (tone_player*)handle;

    JC_MM_DEBUG_PRINTF("tone_do_buffering() handle=0x%X, buffer=0x%X, length=%d\n", handle, buffer, length);

    if (NULL == buffer) {
        return 0;
    }

    JC_MM_ASSERT( buffer == pPlayer->pToneBuffer );

    *min_data_size  = 0;
    *need_more_data = JAVACALL_FALSE;

    return JAVACALL_OK;
}

javacall_result tone_clear_buffer(javacall_handle handle)
{
    tone_player* pPlayer = (tone_player*)handle;

    JC_MM_DEBUG_PRINTF("tone_clear_buffer() handle=0x%X\n", handle);
    if (pPlayer->pToneBuffer) {
        FREE(pPlayer->pToneBuffer);
        pPlayer->pToneBuffer  = NULL;
        pPlayer->toneDataSize = -1;
        pPlayer->offset       = 0;
        pPlayer->currentTime  = 0;
    }

    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result tone_start(javacall_handle handle)
{
    tone_player* pPlayer = (tone_player*)handle;

    JC_MM_DEBUG_PRINTF("tone_start() handle=0x%X\n", handle);

    /* Fake playing */
    if (JAVACALL_FALSE == pPlayer->isForeground) {
        return JAVACALL_OK;
    }

    /* Create Win32 thread to play JTS data - non blocking */
    if (pPlayer->toneDataSize) {
        pPlayer->stopPlaying = JAVACALL_FALSE;
        if (NULL != CreateThread(NULL, 0, tone_jts_player, pPlayer, 0, NULL)) {
            JC_MM_DEBUG_PRINTF("tone_start started id=%d\n", (int)pPlayer->playerId);
            pPlayer->isPlaying = JAVACALL_TRUE;
            return JAVACALL_OK;
        }
        else
        {
            JC_MM_DEBUG_PRINTF("tone_start: failed\n");
            return JAVACALL_FAIL;
        }
    }
    else
    {
        JC_MM_DEBUG_PRINTF("tone_start: no data\n");
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA, 
                        pPlayer->isolateId, pPlayer->playerId, 
                        JAVACALL_OK, (void*)0);
    }
    
    return JAVACALL_OK;
}

/**
 * Stop JTS playing
 */
static javacall_result tone_stop(javacall_handle handle)
{
    tone_player* pPlayer = (tone_player*)handle;
    
    JC_MM_DEBUG_PRINTF("tone_stop() handle=0x%X\n", handle);

    if (JAVACALL_FALSE == pPlayer->isPlaying) {
        return JAVACALL_OK;
    }

    /* Stop playing */
    pPlayer->stopPlaying = JAVACALL_TRUE;

    /* Wait until thread exit */
    while(1) {
        if (JAVACALL_FALSE == pPlayer->isPlaying) {
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
    JC_MM_DEBUG_PRINTF("tone_pause() handle=0x%X\n", handle);
    return tone_stop(handle);
}

/**
 * 
 */
static javacall_result tone_resume(javacall_handle handle)
{
    JC_MM_DEBUG_PRINTF("tone_resume() handle=0x%X\n", handle);
    return tone_start(handle);
}

/**
 * 
 */
static javacall_result tone_get_time(javacall_handle handle, long* ms)
{
    tone_player* pPlayer = (tone_player*)handle;
    JC_MM_DEBUG_PRINTF("tone_get_time() handle=0x%X, currentTime=%d\n", handle, handle==NULL?-1:pPlayer->currentTime);
    *ms = pPlayer->currentTime;
    return JAVACALL_OK;
}

/**
 * Set to ms position
 */
static javacall_result tone_set_time(javacall_handle handle, long* ms)
{
    tone_player* pPlayer = (tone_player*)handle;
    javacall_bool needRestart = JAVACALL_FALSE;

    JC_MM_DEBUG_PRINTF("tone_set_time() handle=0x%X, ms=%d\n", handle, *ms);

    /* There is no tone data */
    if (0 == pPlayer->toneDataSize) {
        *ms = -1;
        return JAVACALL_OK;
    }

    /* If playing, stop it */
    if (JAVACALL_TRUE == pPlayer->isPlaying) {
        tone_stop(handle);
        needRestart = JAVACALL_TRUE;
    }

    tone_play(pPlayer, JAVACALL_TRUE, *ms);

    /* Restart? */
    if (JAVACALL_TRUE == needRestart) {
        tone_start(handle);
    }

    *ms = pPlayer->currentTime;
    return JAVACALL_OK;
}
 
/**
 * 
 */
static javacall_result tone_get_duration(javacall_handle handle, long* ms)
{
    tone_player temp;
    memcpy(&temp, handle, sizeof(tone_player));
    tone_play(&temp, JAVACALL_TRUE, -1);

    *ms = temp.currentTime;
    return JAVACALL_OK;
}


/**
 * Now, switch to foreground
 */
static javacall_result tone_switch_to_foreground(javacall_handle handle, int options) {
    tone_player* pPlayer = (tone_player*)handle;
    pPlayer->isForeground = JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
static javacall_result tone_switch_to_background(javacall_handle handle, int options) {
    tone_player* pPlayer = (tone_player*)handle;
    pPlayer->isForeground = JAVACALL_FALSE;

    /* Stop the current playing */
    tone_stop(handle);

    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

/**
 *
 */
static javacall_result tone_get_volume(javacall_handle handle, long* level)
{
    tone_player* pPlayer = (tone_player*) handle;
    *level = pPlayer->volume;
    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result tone_set_volume(javacall_handle handle, long* level)
{
    tone_player* pPlayer = (tone_player*) handle;

    pPlayer->volume = *level;
     //atomically set value as it may be changed in another thread tone_jts_player()
    InterlockedExchange(&(pPlayer->volumeChanged), JAVACALL_TRUE);

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result tone_is_mute(javacall_handle handle, javacall_bool* mute)
{
    tone_player* pPlayer = (tone_player*) handle;
    *mute = pPlayer->isMute;
    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result tone_set_mute(javacall_handle handle, javacall_bool mute)
{
    tone_player* pPlayer = (tone_player*) handle;

    if (pPlayer->isMute != mute) {
        pPlayer->isMute = mute;
        //atomically set value as it may be changed in another thread tone_jts_player()
        InterlockedExchange(&(pPlayer->muteChanged), JAVACALL_TRUE);
    }
    
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _tone_basic_itf = {
    tone_create,
    NULL,
    NULL,
    tone_close,
    NULL,
    tone_acquire_device,
    tone_release_device,
    NULL,
    NULL,
    tone_start,
    tone_stop,
    tone_pause,
    tone_resume,
    tone_get_java_buffer_size,
    tone_set_whole_content_size,
    tone_get_buffer_address,
    tone_do_buffering,
    tone_clear_buffer,
    tone_get_time,
    tone_set_time,
    tone_get_duration,
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
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
