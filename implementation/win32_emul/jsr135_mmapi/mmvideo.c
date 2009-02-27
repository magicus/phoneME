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

#define LIME_MMAPI_PACKAGE "com.sun.mmedia"
#define LIME_MMAPI_CLASS "JavaCallBridge"

/**********************************************************************************/
extern javacall_result audio_close(javacall_handle handle);
extern javacall_result audio_destroy(javacall_handle handle);
extern javacall_result audio_acquire_device(javacall_handle handle);
extern javacall_result audio_release_device(javacall_handle handle);
extern javacall_result audio_stop(javacall_handle handle);
extern javacall_result audio_pause(javacall_handle handle);
extern javacall_result audio_resume(javacall_handle handle);
extern javacall_result audio_start(javacall_handle handle);
extern javacall_result audio_do_buffering(javacall_handle handle,
                               const void* buffer, long *length,
                               javacall_bool *need_more_data, long *min_data_size);
extern javacall_result audio_clear_buffer(javacall_handle handle);
extern javacall_result audio_get_time(javacall_handle handle, long* ms);
extern javacall_result audio_set_time(javacall_handle handle, long* ms);
extern javacall_result audio_get_duration(javacall_handle handle, long* ms);

extern javacall_result audio_get_java_buffer_size(javacall_handle handle,
        long* java_buffer_size, long* first_data_size);
extern javacall_result audio_set_whole_content_size(javacall_handle handle,
        long whole_content_size);
extern javacall_result audio_get_buffer_address(javacall_handle handle,
        const void** buffer, long* max_size);

//extern char * javautil_media_type_to_mime(javacall_media_format_type media_type);

//volume ctrl
extern javacall_result audio_get_volume(javacall_handle handle, long* level);
extern javacall_result audio_set_volume(javacall_handle handle, long* level);
extern javacall_result audio_is_mute(javacall_handle handle, javacall_bool* mute);
extern javacall_result audio_set_mute(javacall_handle handle, javacall_bool mute);

//rate ctrl
extern javacall_result audio_get_max_rate(javacall_handle handle, long* maxRate);
extern javacall_result audio_get_min_rate(javacall_handle handle, long* minRate);
extern javacall_result audio_set_rate(javacall_handle handle, long rate);
extern javacall_result audio_get_rate(javacall_handle handle, long* rate);

#ifdef ENABLE_EXTRA_CAMERA_CONTROLS
extern void extra_camera_controls_init( audio_handle * pHandle );
extern void extra_camera_controls_cleanup( audio_handle * pHandle );
#endif //ENABLE_EXTRA_CAMERA_CONTROLS

extern void * mmaudio_mutex_create();
/**
 * 
 */
static javacall_result video_create(int appId, int playerId, 
                                    jc_fmt mediaType, 
                                    const javacall_utf16_string URI,
                                    /*OUT*/ javacall_handle *pJCHandle)
{
    static LimeFunction *f = NULL;
    javacall_int64 res = 0;
    audio_handle* pHandle = NULL;
    size_t uriLength = ( NULL != URI ) ? wcslen(URI) : 0;

    javacall_utf16 *mediaTypeWide;
    int mediaTypeWideLen;
    
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

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                            LIME_MMAPI_CLASS,
                            "createVideoPlayer");
    }

    if (fmt_str2mime(fmt_enum2str(mediaType), buff, len) == JAVACALL_FAIL) {
        mediaTypeWide = NULL;
        mediaTypeWideLen = 0;
    }
    else {
        mediaTypeWide = char_to_unicode(buff);
        mediaTypeWideLen = wcslen(mediaTypeWide);
    }
    
    f->call(f, &res, mediaTypeWide, mediaTypeWideLen, URI, uriLength);

    if (res <= 0) {
        FREE( buff );
        FREE( pHandle );
        *pJCHandle = NULL;
	    return JAVACALL_FAIL;
    }

    pHandle->hWnd             = (long) res;
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

#ifdef ENABLE_EXTRA_CAMERA_CONTROLS
    pHandle->pExtraCC         = NULL;

    if( JC_FMT_CAPTURE_VIDEO == mediaType )
    {
        extra_camera_controls_init( pHandle );
    }
#endif //ENABLE_EXTRA_CAMERA_CONTROLS
    
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
static javacall_result video_close(javacall_handle handle)
{
#ifdef ENABLE_EXTRA_CAMERA_CONTROLS
    audio_handle* pHandle = (audio_handle*)handle;

    if( JC_FMT_CAPTURE_VIDEO == pHandle->mediaType )
    {
        extra_camera_controls_cleanup( pHandle );
    }
#endif //ENABLE_EXTRA_CAMERA_CONTROLS

    return audio_close(handle);
}

/**
 * 
 */
static javacall_result video_destroy(javacall_handle handle)
{
    return audio_destroy(handle);
}

/**
 * 
 */
static javacall_result video_acquire_device(javacall_handle handle)
{
    return audio_acquire_device(handle);
}

/**
 * 
 */
static javacall_result video_release_device(javacall_handle handle)
{
    return audio_release_device(handle);
}

/**
 * 
 */
static javacall_result video_start(javacall_handle handle)
{
    return audio_start(handle);
}

/**
 * 
 */
static javacall_result video_stop(javacall_handle handle)
{
    return audio_stop(handle);
}

/**
 * 
 */
static javacall_result video_pause(javacall_handle handle)
{
    return audio_pause(handle);
}

/**
 * 
 */
static javacall_result video_resume(javacall_handle handle)
{
    return audio_resume(handle);
}

/**
 * Store media data to temp file (except JTS type)
 */
static javacall_result
video_do_buffering(javacall_handle handle,
                   const void* buffer, long *length,
                   javacall_bool *need_more_data, long *min_data_size) {
    return audio_do_buffering(handle, buffer, length,
                              need_more_data, min_data_size);
}

static javacall_result video_clear_buffer(javacall_handle handle) {
    return audio_clear_buffer(handle);
}

static javacall_result video_get_time(javacall_handle handle, long* ms)
{
    return audio_get_time(handle, ms);
}

static javacall_result video_set_time(javacall_handle handle, long* ms){
    return audio_set_time(handle, ms);
}
 
static javacall_result video_get_duration(javacall_handle handle, long* ms){
    return audio_get_duration(handle, ms);
}

static javacall_result video_get_volume(javacall_handle handle, long* level){
    return audio_get_volume(handle, level);
}

static javacall_result video_set_volume(javacall_handle handle, long* level){
    return audio_set_volume(handle, level);
}

static javacall_result video_is_mute(javacall_handle handle, javacall_bool* mute){
    return audio_is_mute(handle, mute);
}

static javacall_result video_set_mute(javacall_handle handle, javacall_bool mute){
    return audio_set_mute(handle, mute);
}

static javacall_result video_get_max_rate(javacall_handle handle, long* maxRate){
    return audio_get_max_rate(handle, maxRate);
}
static javacall_result video_get_min_rate(javacall_handle handle, long* minRate){
    return audio_get_min_rate(handle, minRate);
}
static javacall_result video_set_rate(javacall_handle handle, long rate){
    return audio_set_rate(handle, rate);
}
static javacall_result video_get_rate(javacall_handle handle, long* rate){
    return audio_get_rate(handle, rate);
}

/**********************************************************************************/

/**
 * 
 */
static javacall_result video_get_video_size(javacall_handle handle, long* width, long* height)
{
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f1 = NULL;
    static LimeFunction *f2 = NULL;

    if (f1 == NULL) {
        f1 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "getSourceWidth");
    }
    f1->call(f1, width, pHandle->hWnd);

    if (f2 == NULL) {
        f2 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "getSourceHeight");
    }
    f2->call(f2, height, pHandle->hWnd);
    
    return JAVACALL_OK;
}



/**
 * 
 */
static javacall_result video_set_video_visible(javacall_handle handle, javacall_bool visible)
{
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "setVisible");
    }
    f->call(f, &res, pHandle->hWnd, visible);
    
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f1 = NULL;
    static LimeFunction *f2 = NULL;
    static LimeFunction *f3 = NULL;
    static LimeFunction *f4 = NULL;
    int res;

    if (f1 == NULL) {
        f1 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "setDisplayX");
    }
    f1->call(f1, &res, pHandle->hWnd, x);

    if (f2 == NULL) {
        f2 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "setDisplayY");
    }
    f2->call(f2, &res, pHandle->hWnd, y);

    if (f3 == NULL) {
        f3 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "setDisplayWidth");
    }
    f3->call(f3, &res, pHandle->hWnd, w);

    if (f4 == NULL) {
        f4 = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "setDisplayHeight");
    }
    f4->call(f4, &res, pHandle->hWnd, h);
    
    return JAVACALL_OK;
}
 
/**********************************************************************************/

static long  dataLength;
static char* snapshot_buffer = NULL; // this buffer will hold the snapshot

/**
 * 
 */
static javacall_result video_start_video_snapshot(javacall_handle handle, 
                                                  const javacall_utf16* imageType, long length)
{
    audio_handle* pHandle = (audio_handle*)handle;
    char* data;
    static LimeFunction *f = NULL;
    
    if (snapshot_buffer != NULL) {
        return JAVACALL_FAIL;
    }
    
    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "snapshot");
    }
    
    f->call(f, &data, &dataLength, pHandle->hWnd, imageType, length);
    
    if(data == NULL){
        dataLength = -1;
    }
    
    snapshot_buffer = (char *) malloc(dataLength);
    memcpy(snapshot_buffer, data, dataLength);
    
    javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
        pHandle->isolateId, pHandle->playerId, NULL == data ? JAVACALL_FAIL :
        JAVACALL_OK, NULL);
    
    
    return JAVACALL_OK; 
}

/**
 * 
 */
static javacall_result video_get_video_snapshot_data_size(javacall_handle handle, long* size)
{
    *size = dataLength;
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_get_video_snapshot_data(javacall_handle handle, char* buffer, long size)
{
    if (snapshot_buffer != NULL) {
        memcpy(buffer, snapshot_buffer, size);
        free(snapshot_buffer);
        snapshot_buffer = NULL;
        return JAVACALL_OK;
    }
    else {
        return JAVACALL_FAIL;
    }
}

static javacall_result video_set_video_fullscreenmode( javacall_handle handle, javacall_bool fullScreenMode )
{
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;
    int res = 0;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                        LIME_MMAPI_CLASS,
                        "setDisplayFullScreen");
    }
    
    f->call(f, &res, pHandle->hWnd, fullScreenMode);

    return JAVACALL_OK; 
}

static javacall_result map_frame_to_time(javacall_handle handle, 
                                                 long frameNum, long* ms) {
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "mapFrameToTime");
    }
    f->call(f, ms, pHandle->hWnd, frameNum);
    
    return JAVACALL_OK;
}

static javacall_result map_time_to_frame(javacall_handle handle, 
                                                 long ms, long* frameNum) {
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "mapTimeToFrame");
    }
    f->call(f, frameNum, pHandle->hWnd, ms);
    
    return JAVACALL_OK;
}

static javacall_result seek_to_frame(javacall_handle handle, 
                                             long frameNum, long* actualFrameNum) {
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "seek");
    }
    f->call(f, actualFrameNum, pHandle->hWnd, frameNum);
    
    return JAVACALL_OK;
}

static javacall_result skip_frames(javacall_handle handle, long* nFrames) {
    audio_handle* pHandle = (audio_handle*)handle;
    static LimeFunction *f = NULL;

    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE,
                    LIME_MMAPI_CLASS,
                    "skip");
    }
    f->call(f, nFrames, pHandle->hWnd, *nFrames);
    
    return JAVACALL_OK;

}

/**
 * Now, switch to foreground
 */
static javacall_result video_switch_to_foreground(javacall_handle handle, int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
static javacall_result video_switch_to_background(javacall_handle handle, int options) {
    audio_handle* pHandle = (audio_handle*)handle;
    pHandle->isForeground = JAVACALL_FALSE;

    // Stop the current playing
    audio_stop(handle);

    return JAVACALL_OK;
}

static javacall_result video_get_java_buffer_size(javacall_handle handle,
        long* java_buffer_size, long* first_data_size)
{
    return audio_get_java_buffer_size(
            handle, java_buffer_size, first_data_size);
}

static javacall_result video_set_whole_content_size(javacall_handle handle,
        long whole_content_size)
{
    return audio_set_whole_content_size(handle, whole_content_size);
}

static javacall_result video_get_buffer_address(javacall_handle handle,
        const void** buffer, long* max_size)
{
    return audio_get_buffer_address(handle, buffer, max_size);
}

static javacall_result video_set_video_color_key(
            javacall_handle handle, javacall_bool on, javacall_pixel color) {
    
    static LimeFunction *f = NULL;
    
    if (f == NULL) {
        f = NewLimeFunction(LIME_MMAPI_PACKAGE, LIME_MMAPI_CLASS,
                "setVideoAlpha");
    }
    
    f->call(f, NULL, !on, color);
    
    return JAVACALL_OK;
}
/**********************************************************************************/

/**
 * Video basic javacall function interface
 */
static media_basic_interface _video_basic_itf = {
    video_create,
    NULL,
    NULL, // get_player_controls
    video_close,
    video_destroy,
    video_acquire_device,
    video_release_device,
    NULL, // realize
    NULL, // prefetch
    video_start,
    video_stop,
    video_pause,
    video_resume,
    video_get_java_buffer_size,
    video_set_whole_content_size,
    video_get_buffer_address,
    video_do_buffering,
    video_clear_buffer,
    video_get_time,
    video_set_time,
    video_get_duration,
    video_switch_to_foreground,
    video_switch_to_background
};

/**
 * Video video javacall function interface
 */
static media_video_interface _video_video_itf = {
    video_get_video_size,
    video_set_video_visible,
    video_set_video_location,
    video_set_video_color_key,
    video_set_video_fullscreenmode,
};

/**
 * Video snapshot javacall function interface
 */
static media_snapshot_interface _video_snapshot_itf = {
    video_start_video_snapshot,
    video_get_video_snapshot_data_size,
    video_get_video_snapshot_data
};

static media_volume_interface _video_volume_itf = {
    video_get_volume,
    video_set_volume,
    video_is_mute,
    video_set_mute
};

static media_rate_interface _video_rate_itf = {
    video_get_max_rate, 
    video_get_min_rate, 
    video_set_rate, 
    video_get_rate
};

static media_fposition_interface _video_fposition_itf = {
    map_frame_to_time, 
    map_time_to_frame, 
    seek_to_frame, 
    skip_frames
};
/**********************************************************************************/
 
/* Global video interface */
media_interface g_video_itf = {
    &_video_basic_itf,
    &_video_volume_itf,
    &_video_video_itf,
    &_video_snapshot_itf,
    NULL,
    NULL,
    NULL, // IMPL_NOTE: _video_rate_itf excluded because of problems with Java StopTimeControl
    NULL,
    NULL,
    NULL,
    &_video_fposition_itf
}; 

 
