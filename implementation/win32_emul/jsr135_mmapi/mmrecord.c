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

#include <limits.h>
#include "mmrecord.h"
#include "wav_struct.h"

int create_wavhead(recorder* h, char *buffer, int buflen)
{
    struct std_head *wh;

    if (buffer == NULL)
        return sizeof(struct std_head);

    JC_MM_ASSERT(buflen >= sizeof(struct std_head));

    wh = (struct std_head *)buffer;

    memset(wh, 0, sizeof(struct std_head));

    wh->rc.chnk_id          = CHUNKID_RIFF;
    wh->rc.chnk_ds          = 4;
    wh->rc.type             = TYPE_WAVE;

    wh->fc.chnk_id          = CHUNKID_fmt ;
    wh->fc.chnk_ds          = 16;
    wh->fc.compression_code = 1;
    wh->fc.num_channels     = h->channels;
    wh->fc.sample_rate      = h->rate;
    wh->fc.bytes_per_second = h->rate * h->channels * h->bits / 8;
    wh->fc.block_align      = h->channels * h->bits / 8;
    wh->fc.bits             = h->bits;

    wh->dc.chnk_id          = CHUNKID_data;
    wh->dc.chnk_ds          = h->recordLen;

    // update full recodr data len
    wh->rc.chnk_ds          += wh->fc.chnk_ds + 8 + 8 + h->recordLen;

    return sizeof(struct std_head);
}

/*******************************************************************************/

void sendRSL(int appId, int playerId, long duration)
{

    OutputDebugString("Sending Record Size Limit\n");

    javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT, 
                                      appId, playerId, 
                                      JAVACALL_OK, (void*)duration );
}

/*******************************************************************************/

/**
 * Create native recorder
 */
static javacall_result recorder_create(int appId, int playerId,
                                       jc_fmt mediaType,
                                       const javacall_utf16_string URI,
                                       javacall_handle *pJCHandle )
{
    javacall_result result = JAVACALL_FAIL;
    
    recorder* newHandle = NULL;

    JC_MM_ASSERT( JC_FMT_CAPTURE_AUDIO == mediaType );

    newHandle = MALLOC(sizeof(recorder));
    JC_MM_ASSERT( NULL != newHandle );
    memset( newHandle, 0, sizeof(recorder) );

    newHandle->isolateId     = appId;
    newHandle->playerId      = playerId;

    // defaults
    newHandle->bits          = 16;
    newHandle->channels      = 1;
    newHandle->rate          = 22050;

    // overrides
    get_int_param(URI, L"bits",     &(newHandle->bits)    );
    get_int_param(URI, L"channels", &(newHandle->channels));
    get_int_param(URI, L"rate",     &(newHandle->rate)    );

    newHandle->lengthLimit   = INT_MAX;
    newHandle->recordLen     = 0;
    newHandle->rsl           = FALSE;
    newHandle->recInitDone   = FALSE;
    newHandle->recordData    = NULL;
    newHandle->fname         = NULL;

    result = JAVACALL_OK;

    *pJCHandle = (javacall_handle)newHandle;
    return result;
}

/**
 * Close native recorder
 */
static javacall_result recorder_close(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    FREE(h);
    r = JAVACALL_OK;

    return r;
}

/**
 * NOTING TO DO
 */
static javacall_result recorder_destroy(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO
 */
static javacall_result recorder_acquire_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO
 */
static javacall_result recorder_release_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO
 */
static javacall_result recorder_start(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO. Java'll call appropriate recording APIs.
 */
static javacall_result recorder_stop(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO.
 */
static javacall_result recorder_pause(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO.
 */
static javacall_result recorder_resume(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * NOTING TO DO.
 */
static javacall_result recorder_get_time(javacall_handle handle, long* ms)
{
    *ms = -1;
    return JAVACALL_OK;
}

/**
 * NOTING TO DO.
 */
static javacall_result recorder_set_time(javacall_handle handle, long* ms)
{
    *ms = -1;
    return JAVACALL_OK;
}
 
/**
 * NOTING TO DO.
 */
static javacall_result recorder_get_duration(javacall_handle handle, long* ms)
{
    *ms = -1;
    return JAVACALL_OK;
}

/******************************************************************************/

/**
 *
 */
static javacall_result recorder_set_recordsize_limit(javacall_handle handle, 
                                                     /*INOUT*/ long* size)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    if( INT_MAX != *size && *size > 20000000 )
        *size = 20000000;

    if ( *size < sizeof(struct std_head))
        *size = sizeof(struct std_head);

    h->lengthLimit = (*size);

    if ( INT_MAX != *size )
        h->lengthLimit -= sizeof(struct std_head);

    r = JAVACALL_OK;

    return r;
}


static 
javacall_result recorder_recording_handled_by_native(javacall_handle handle, 
                                                   const javacall_utf16* locator)
{
    return JAVACALL_FAIL;
}

/**
 * Start recording.
 * Allocate recording resources if there are no resources allocated now.
 */
static javacall_result recorder_start_recording(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    if(!h->recInitDone)
    {
        int fnameLen;
        int whl;
        char b[256];
        char path[MAX_PATH-14];
        char filename[MAX_PATH];

        assert(0 != GetTempPath(MAX_PATH-14, path));
        assert(0 != GetTempFileName(path, "capture", 0, filename));

        h->recordData = fopen(filename, "w+b");

        if( NULL != h->recordData )
        {
            fnameLen = strlen(filename);
            h->fname = MALLOC(fnameLen+1);
            memcpy(h->fname, filename, fnameLen);
            h->fname[ fnameLen ] = '\0';

            whl = create_wavhead( h, b, 256 );
            fwrite(b, whl, 1, h->recordData);

            h->recInitDone = initAudioCapture(h);

            if( h->recInitDone )
            {
                toggleAudioCapture(TRUE);
                r = JAVACALL_OK;
            }
            else
            {
                fclose( h->recordData );
                DeleteFile( h->fname );
                FREE( h->fname );

                h->fname      = NULL;
                h->recordData = NULL;
            }
        }
    }
    else
    {
        toggleAudioCapture(TRUE);
    }

    return r;
}

/**
 * Stop recording.
 * And, this recording could be resumed by 'recorder_start_recording' call.
 */
static javacall_result recorder_pause_recording(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    toggleAudioCapture(FALSE);
    r = JAVACALL_OK;

    return r;
}

/**
 * Purge recording buffer and delete temp file
 * 'recorder_pause_recording' MUST be called before by Java
 */
static javacall_result recorder_reset_recording(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    // NEED REVISIT: What is close doing?
    r = JAVACALL_OK;

    return r;
}

/**
 * Now, recording is finished. 
 */
static javacall_result recorder_commit_recording(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    int whl;
    char b[256];

    whl = create_wavhead( h, b, 256 );
    fseek(h->recordData, 0, SEEK_SET);
    fwrite(b, whl, 1, h->recordData);

    r = JAVACALL_OK;

    return r;
}

/**
 * Get current recorded data size. It can be called during recording.
 */
static javacall_result recorder_get_recorded_data_size(javacall_handle handle, 
                                              /*OUT*/ long* size)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;
    
    *size = h->recordLen + sizeof(struct std_head);
    r = JAVACALL_OK;

    return r;
}

/**
 * This function is called after 'recorder_commit_recording' calling.
 * Can be called multiple times.
 */
static javacall_result recorder_get_recorded_data(javacall_handle handle, 
                                                  /*OUT*/ char* buffer, 
                                                  long offset, long size)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    assert(0 == fseek(h->recordData, offset, SEEK_SET));
    fread(buffer, size, 1, h->recordData);
    r = JAVACALL_OK;

    return r;
}

/**
 * Delete all recording resources (now, recording is completed)
 * Next recording will be stared by 'recorder_start_recording' call
 */
static javacall_result recorder_close_recording(javacall_handle handle)
{
    javacall_result r = JAVACALL_FAIL;
    recorder* h = (recorder*)handle;

    if(h->recInitDone)
    {
        closeAudioCapture();
        fclose(h->recordData);
        DeleteFile(h->fname);
        FREE(h->fname); 

        h->fname       = NULL;
        h->recordData  = NULL;
        h->recInitDone = FALSE;
    }

    r = JAVACALL_OK;

    return r;
}

/******************************************************************************/

/**
 * Video basic javacall function interface
 */
static media_basic_interface _recorder_basic_itf = {
    recorder_create,
    NULL,
    NULL,
    recorder_close,
    recorder_destroy,
    recorder_acquire_device,
    recorder_release_device,
    NULL,
    NULL,
    recorder_start,
    recorder_stop,
    recorder_pause,
    recorder_resume,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    recorder_get_time,
    recorder_set_time,
    recorder_get_duration,
    NULL,
    NULL
};

/**
 * Record record javacall function interface
 */
static media_record_interface _recorder_record_itf = {
    recorder_set_recordsize_limit,
    recorder_recording_handled_by_native,
    recorder_start_recording,
    recorder_pause_recording,
    recorder_pause_recording,   /* Pause is same as stop */
    recorder_reset_recording,
    recorder_commit_recording,
    recorder_get_recorded_data_size,
    recorder_get_recorded_data,
    recorder_close_recording
};

/*******************************************************************************/
 
/* Global record interface */
media_interface g_record_itf = {
    &_recorder_basic_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &_recorder_record_itf,
    NULL
}; 
