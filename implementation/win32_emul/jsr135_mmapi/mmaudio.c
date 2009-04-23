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
 * This software is provided "AS IS," without a warranty of any kind. ALL
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING
 * ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN AND ITS LICENSORS SHALL NOT
 * BE LIABLE FOR ANY DAMAGES OR LIABILITIES SUFFERED BY LICENSEE AS A RESULT
 * OF OR RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR ITS
 * DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST
 * REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL,
 * INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY
 * OF LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE SOFTWARE, EVEN
 * IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */
 
#include "lime.h"
#include "multimedia.h"
#include "file.h"

#define TIMER_CALLBACK_DURATION 500

#define LIME_MMAPI_PACKAGE "com.sun.mmedia"
#define LIME_MMAPI_CLASS   "JavaCallBridge"

#define DEFAULT_BUFFER_SIZE  1024 * 1024 // default buffer size 1 MB
#define TIMER_TICK_PERIOD    100         // period between calls to audio_timer_callback

/* forward declaration */
javacall_result audio_start(javacall_handle handle);

/******************************************************************************/

void* mmaudio_mutex_create( )
{
    LPCRITICAL_SECTION pcs
        = (LPCRITICAL_SECTION)MALLOC( sizeof( CRITICAL_SECTION ) );

    InitializeCriticalSection( pcs );

    return pcs;
}

void mmaudio_mutex_destroy( void* m )
{
    DeleteCriticalSection( (LPCRITICAL_SECTION)m );
    FREE( m );
}

void mmaudio_mutex_lock( void* m )
{
    EnterCriticalSection( (LPCRITICAL_SECTION)m );
}

void mmaudio_mutex_unlock( void* m )
{
    LeaveCriticalSection( (LPCRITICAL_SECTION)m );
}

/******************************************************************************/

/**
 * 
 */
javacall_result audio_get_time(javacall_handle handle, long* ms){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    javacall_int64 res;

    /* From MMAPI spec (Player):
     * getMediaTime may return TIME_UNKNOWN (-1) to indicate 
     * that the media time cannot be determined
     */
    if (pHandle->fileName && 
        0 == strncmp(pHandle->fileName, "rtsp://", strlen("rtsp://"))) {
            *ms = -1;
            return JAVACALL_OK;
    }

    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "getTime");
        }
        f->call(f, &res, pHandle->hWnd);
        pHandle->curTime = (long)res;
    }

    *ms = (long)pHandle->curTime;

    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_set_time(javacall_handle handle, long* ms){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;
    javacall_int64 time = *ms;
    
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "setTime");
        }
        f->call(f, &res, pHandle->hWnd, time);
        pHandle->offset = *ms;
    } else {
        *ms = -1;
    }
    return JAVACALL_OK;
}
 
/**
 * 
 */
javacall_result audio_get_duration(javacall_handle handle, long* ms) {
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    javacall_int64 res;

    /* From MMAPI spec (Player):
     * If the duration cannot be determined (for example, the Player 
     * is presenting live media) getDuration returns TIME_UNKNOWN (-1).
     */
    if (pHandle->fileName && 
        0 == strncmp(pHandle->fileName, "rtsp://", strlen("rtsp://"))) {
        *ms = -1;
        return JAVACALL_OK;
    }

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "getDuration");
    }
    f->call(f, &res, pHandle->hWnd);
    pHandle->duration = (long)res;    
    
    *ms = (long)pHandle->duration;
    return JAVACALL_OK;
}



javacall_result audio_get_java_buffer_size(javacall_handle handle,
        long* java_buffer_size, long* first_data_size) 
{
    audio_handle* pHandle = (audio_handle *) handle;
    
    if (pHandle->wholeContentSize == -1) {
        *java_buffer_size = DEFAULT_BUFFER_SIZE;
        *first_data_size  = DEFAULT_BUFFER_SIZE;
        pHandle->wholeContentSize = DEFAULT_BUFFER_SIZE;
    } else {
        *java_buffer_size = pHandle->wholeContentSize;
        *first_data_size  = pHandle->wholeContentSize;
    }
        
    return JAVACALL_OK;
}

javacall_result audio_set_whole_content_size(javacall_handle handle,
        long whole_content_size)
{
    audio_handle* pHandle = (audio_handle *) handle;
    
    pHandle->wholeContentSize = whole_content_size;

    return JAVACALL_OK;
}

javacall_result audio_get_buffer_address(javacall_handle handle,
        const void** buffer, long* max_size)
{
    audio_handle* pHandle = (audio_handle *) handle;
    
    if (pHandle->buffer == NULL)
    {
        pHandle->buffer = MALLOC(pHandle->wholeContentSize);
    }
    if (pHandle->buffer == NULL) {
        return JAVACALL_FAIL;
    }
    
    *buffer = pHandle->buffer;
    *max_size = pHandle->wholeContentSize;
    
    return JAVACALL_OK;
}

/*
extern javacall_handle audio_create(javacall_int64 playerId,
jcafmt mediaType, const javacall_utf16* URI, long uriLength);
*/
/**********************************************************************************/

/**
 * Timer callback used to check about end of media (except JTS type)
 */
static void CALLBACK FAR audio_timer_callback(UINT uID, UINT uMsg, 
                                          DWORD dwUser, 
                                          DWORD dw1, 
                                          DWORD dw2) {
    audio_handle* pHandle = (audio_handle*)dwUser;

    mmaudio_mutex_lock(pHandle->mutex);
    if (pHandle->timerId != uID || pHandle->hWnd <= 0) {
        // timer already closed
        timeKillEvent(uID);
        mmaudio_mutex_unlock(pHandle->mutex);
        return;
    }
    if (pHandle->hWnd > 0) {
        if (-1 == pHandle->duration) {
            audio_get_duration((javacall_handle)dwUser, &(pHandle->duration) );
            JC_MM_DEBUG_PRINT1("[jc_media] audio_timer_callback %d\n", pHandle->duration);
            if (-1 == pHandle->duration) {
                mmaudio_mutex_unlock(pHandle->mutex);
                return;
            }
            else {
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DURATION_UPDATED,
                    pHandle->isolateId, pHandle->playerId, JAVACALL_OK, 
                    (void *) pHandle->duration);
            }
        }

        audio_get_time((javacall_handle)dwUser,&(pHandle->offset));
        pHandle->curTime = pHandle->offset;
        
        /* Is end of media reached? */
        if (pHandle->offset >= pHandle->duration) {
            /* Post EOM event to Java and kill player timer */
            timeKillEvent(pHandle->timerId);
            pHandle->timerId = 0;
            pHandle->offset = 0;

            if( JC_FMT_CAPTURE_VIDEO == pHandle->mediaType ) {
                // emulated camera loops ininitely,
                // but EOM must not be sent
                audio_set_time( (javacall_handle)dwUser, &(pHandle->offset) );
                audio_start( (javacall_handle)dwUser );
            } else {
                JC_MM_DEBUG_PRINT1("[jc_media] javanotify_on_media_notification %d\n", pHandle->playerId);
                audio_stop((javacall_handle) dwUser);
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                                                 pHandle->isolateId, pHandle->playerId, 
                                                 JAVACALL_OK, (void*)pHandle->duration);
            }
        }
    }
    mmaudio_mutex_unlock(pHandle->mutex);
}


/**********************************************************************************/

/**
 * 
 */
static javacall_result audio_create(int appId, int playerId, 
                             jc_fmt mediaType, 
                             const javacall_utf16_string URI,
                             javacall_handle *pJCHandle ) {

    javacall_utf16 *mediaTypeWide;
    int mediaTypeWideLen;
    static LimeFunction *f = NULL;

    size_t uriLength = ( NULL != URI ) ? wcslen(URI) : 0;

    javacall_int64 res = 0;
    audio_handle* pHandle = NULL;
    
    int len = MAX_MIMETYPE_LENGTH;
    char *buff = MALLOC(len);

    if (buff == NULL) {
        *pJCHandle = NULL;
        return JAVACALL_OUT_OF_MEMORY;
    }

    pHandle = MALLOC(sizeof(audio_handle));
    if (NULL == pHandle) {
        FREE( buff );
        *pJCHandle = NULL;
        return JAVACALL_OUT_OF_MEMORY;
    }

    memset(pHandle,0,sizeof(audio_handle));

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                            LIME_MMAPI_CLASS,
                            "createSoundPlayer");
    }

    if (fmt_str2mime(fmt_enum2str(mediaType), buff, len) == JAVACALL_FAIL) {
        mediaTypeWide = NULL;
        mediaTypeWideLen = 0;
    }
    else {
        mediaTypeWide = char_to_unicode(buff);
        mediaTypeWideLen = wcslen(mediaTypeWide);//MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,(LPCSTR)mediaType,-1,mediaTypeWide,256);
    }
    
    f->call(f, &res, mediaTypeWide, mediaTypeWideLen, URI, uriLength);
    if (res <= 0) {
        FREE( buff );
        FREE( pHandle );
	    *pJCHandle = NULL;
        return JAVACALL_FAIL;
    }

    pHandle->hWnd             = (long)res;
    pHandle->offset           = 0;
    pHandle->duration         = -1;
    pHandle->curTime          = 0;
    pHandle->isolateId        = appId;
    pHandle->playerId         = playerId;
    pHandle->timerId          = 0;
    pHandle->isForeground     = JAVACALL_TRUE;
    pHandle->mediaType        = mediaType;
    pHandle->buffer           = NULL;
    pHandle->wholeContentSize = -1;
    pHandle->isBuffered       = JAVACALL_FALSE;
    pHandle->volume           = -1;
    pHandle->mute             = JAVACALL_FALSE;
    pHandle->mutex            = mmaudio_mutex_create();

    if(!pHandle->mutex) {
        FREE( buff );
        FREE( pHandle );
	    *pJCHandle = NULL;
        return JAVACALL_OUT_OF_MEMORY;
    }

    // set the file name to the URI
    if (NULL != URI && uriLength>0) {
        wcstombs(pHandle->fileName, URI, uriLength);
    }
    
    if (buff != NULL) {
        FREE(buff);
    }

    *pJCHandle = pHandle;
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_close(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;

    mmaudio_mutex_lock(pHandle->mutex);
    /* Kill player timer */
    if (pHandle->timerId) {
        timeKillEvent(pHandle->timerId);
        pHandle->timerId = 0;
    }

    /* make sure audio_timer_callback is finished */
    mmaudio_mutex_unlock(pHandle->mutex);
    Sleep(TIMER_TICK_PERIOD);
    mmaudio_mutex_lock(pHandle->mutex);

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                            LIME_MMAPI_CLASS,
                            "close");
    }
    
    f->call(f, &res, pHandle->hWnd);

    if (pHandle->hWnd > 0) {
        pHandle->hWnd = -1;
    }

    if (pHandle->buffer != NULL) {
        FREE(pHandle->buffer);
    }

    mmaudio_mutex_unlock(pHandle->mutex);
    mmaudio_mutex_destroy(pHandle->mutex);

    FREE(pHandle);

    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_destroy(javacall_handle handle){
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_acquire_device(javacall_handle handle){
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_release_device(javacall_handle handle){
    return JAVACALL_OK;
}

javacall_result audio_get_max_rate(javacall_handle handle, long* maxRate){
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "getMaxRate");
        }
        f->call(f, maxRate, pHandle->hWnd);
    }
    
    return JAVACALL_OK;
}

javacall_result audio_get_min_rate(javacall_handle handle, long* minRate){
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "getMinRate");
        }
        f->call(f, minRate, pHandle->hWnd);
    }
    
    return JAVACALL_OK;
}

javacall_result audio_set_rate(javacall_handle handle, long rate){
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "setRate");
        }
        f->call(f, &res, pHandle->hWnd, rate);
    }
    return JAVACALL_OK;
}

javacall_result audio_get_rate(javacall_handle handle, long* rate){
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "getRate");
        }
        f->call(f, rate, pHandle->hWnd);
    }
    
    return JAVACALL_OK;
}

/**
 * Store media data to temp file (except JTS type)
 */
javacall_result audio_do_buffering(javacall_handle handle,
                                   const void* buffer, long *length,
                                   javacall_bool *need_more_data, long *min_data_size){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;
    char *sendBuffer=(char *)buffer;
    
    if (NULL != buffer) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                                LIME_MMAPI_CLASS,
                                "doBuffering");
        }
    
        f->call(f, &res, pHandle->hWnd, sendBuffer, *length);
    
        *need_more_data = JAVACALL_FALSE;
        *min_data_size  = 0;
        
        pHandle->isBuffered = JAVACALL_TRUE;
    }
    
    return JAVACALL_OK;
}

/**
 * Delete temp file
 */
javacall_result audio_clear_buffer(javacall_handle hLIB){

    audio_handle* pHandle = (audio_handle*)hLIB;

    /* Reset offset */
    pHandle->offset = 0;
    if (pHandle->buffer != NULL) {
        FREE(pHandle->buffer);
        pHandle->buffer = NULL;
    }
    //DeleteFile(pHandle->fileName);
    
    return JAVACALL_OK;
}


/**
 * 
 */
javacall_result audio_start(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;
    
    if (JAVACALL_FALSE == pHandle->isForeground) {
        JC_MM_DEBUG_PRINT("[jc_media] Audio fake start in background!\n");
        return JAVACALL_OK;
    }
    

    JC_MM_DEBUG_PRINT1("[jc_media] + javacall_media_start %s\n", pHandle->fileName);

    /* Seek to play position */
    if (pHandle->offset) {
        pHandle->curTime = pHandle->offset;
        audio_set_time((javacall_handle)pHandle, &(pHandle->curTime));
    } else {
        pHandle->curTime = 0;
    }

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                            LIME_MMAPI_CLASS,
                            "start");
    }
    f->call(f, &res, pHandle->hWnd);
    
    pHandle->timerId = 
            (UINT)timeSetEvent(TIMER_CALLBACK_DURATION, TIMER_TICK_PERIOD, audio_timer_callback,(DWORD)pHandle, 
            TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_stop(javacall_handle handle){

    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;

    mmaudio_mutex_lock(pHandle->mutex);

    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "stop");
        }
        f->call(f, &res, pHandle->hWnd);
        /* Kill player timer */
        if (pHandle->timerId) {
            timeKillEvent(pHandle->timerId);
            pHandle->timerId = 0;
        }
        //MCIWndStop(pHandle->hWnd);
    }
    mmaudio_mutex_unlock(pHandle->mutex);
    
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_pause(javacall_handle handle) {

    audio_handle* pHandle = (audio_handle*) handle;
    static LimeFunction *f = NULL;
    int res;
    
    mmaudio_mutex_lock(pHandle->mutex);

    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "stop");
        }
        
        f->call(f, &res, pHandle->hWnd);

        /* Kill player timer */
        if (pHandle->timerId) {
            timeKillEvent(pHandle->timerId);
            pHandle->timerId = 0;
        }
    }

    mmaudio_mutex_unlock(pHandle->mutex);
    
    return JAVACALL_OK;
}

/**
 * 
 */
javacall_result audio_resume(javacall_handle handle) {

    audio_handle* pHandle = (audio_handle*) handle;
    static LimeFunction *f = NULL;
    int res;
    
    if (pHandle->hWnd > 0) {
        pHandle->timerId = (UINT) timeSetEvent(TIMER_CALLBACK_DURATION, TIMER_TICK_PERIOD, 
                audio_timer_callback, (DWORD)pHandle, 
                TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "resume");
        }
        f->call(f, &res, pHandle->hWnd);
    }
    
    return JAVACALL_OK;
}



/**
 * Now, switch to foreground
 */
javacall_result audio_switch_to_foreground(javacall_handle handle, int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
javacall_result audio_switch_to_background(javacall_handle handle, int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_FALSE;

    /* Stop the current playing */
    audio_stop(handle);

    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

static int audio_do_set_volume( audio_handle* pHandle, long level )
{
    static LimeFunction *f = NULL;
    int res;
    
    if (pHandle->hWnd > 0) {
        if (f == NULL) {
            f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "setVolume");
        }
        f->call(f, &res, pHandle->hWnd, level);
    }
    return res;
}

javacall_result audio_get_volume(javacall_handle handle, long* level) {
    audio_handle* pHandle = (audio_handle*)handle;
    *level = pHandle->volume;
    return JAVACALL_OK;
}

javacall_result audio_set_volume(javacall_handle handle, long* level) {
    audio_handle* pHandle = (audio_handle*)handle;
    if( !pHandle->mute ) audio_do_set_volume( pHandle, *level );
    pHandle->volume = *level;
    return JAVACALL_OK;
}

javacall_result audio_is_mute(javacall_handle handle, javacall_bool* mute) {
    audio_handle* pHandle = (audio_handle*)handle;
    *mute = pHandle->mute ? JAVACALL_TRUE : JAVACALL_FALSE;
    return JAVACALL_OK;
}

javacall_result audio_set_mute(javacall_handle handle, javacall_bool mute){
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->mute = ( JAVACALL_TRUE == mute ) ? TRUE : FALSE;
    audio_do_set_volume(pHandle, ( pHandle->mute ? 0 : pHandle->volume ) );
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _audio_basic_itf = {
    audio_create,
    NULL, // get_format
    NULL, // get_player_controls
    audio_close,
    audio_destroy,
    audio_acquire_device,
    audio_release_device,
    NULL, // realize
    NULL, // prefetch
    audio_start,
    audio_stop,
    audio_pause,
    audio_resume,
    audio_get_java_buffer_size,
    audio_set_whole_content_size,
    audio_get_buffer_address,
    audio_do_buffering,
    audio_clear_buffer,
    audio_get_time,
    audio_set_time,
    audio_get_duration,
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

static media_rate_interface _audio_rate_itf = {
    audio_get_max_rate, 
    audio_get_min_rate, 
    audio_set_rate, 
    audio_get_rate
};

/**********************************************************************************/
 
/* Global audio interface */
media_interface g_audio_itf = {
    &_audio_basic_itf,
    &_audio_volume_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, // IMPL_NOTE: _audio_rate_itf excluded because of problems with Java StopTimeControl
    NULL,
    NULL,
    NULL,
    NULL
}; 
