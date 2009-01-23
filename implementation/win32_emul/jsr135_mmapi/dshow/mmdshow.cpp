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

#include <stdio.h>

#include "multimedia.h"
#include "audioplayer.hpp"

//=============================================================================

/*
static void dshow_DBG_out( LPCSTR str )
{
    FILE* f = fopen( "c:\\rtp_dshow.log", "ab" );
    if( NULL != f )
    {
        fputs( str, f );
        fclose( f );
    }
}

#define dshow_DBG( s ) dshow_DBG_out( (s) )
*/

#define dshow_DBG( s ) OutputDebugString( (s) )

//=============================================================================

extern "C" {

//=============================================================================

#define XFER_BUFFER_SIZE  1024
#define BUFFER_PACKETS    20  // number of packets necessary to stop buffering
#define MAX_MISSING       40  // number of missing packets necessary to consider EOM

typedef struct
{
    int                   appId;
    int                   playerId;
    jc_fmt                mediaType;
    javacall_utf16_string uri;

    int                   channels;
    int                   rate;

    BYTE                  buf[ XFER_BUFFER_SIZE ];

    BOOL                  realized;
    BOOL                  prefetched;
    BOOL                  acquired;
    BOOL                  playing;
    BOOL                  buffering;

    int                   enqueued;

    javacall_int64        samplesPlayed;

    long                  volume;
    javacall_bool         mute;

    audioplayer           ap;
} dshow_player;

static javacall_handle dshow_create(int appId, 
                                  int playerId,
                                  jc_fmt mediaType,
                                  const javacall_utf16_string URI)
{
    dshow_player* p = new dshow_player;//(dshow_player*)MALLOC(sizeof(dshow_player));

    dshow_DBG( "*** create ***\n" );

    p->appId       = appId;
    p->playerId    = playerId;
    p->mediaType   = mediaType;
    p->uri         = NULL;
    p->realized    = FALSE;
    p->prefetched  = FALSE;
    p->acquired    = FALSE;
    p->playing     = FALSE;
    p->buffering   = TRUE;
    p->enqueued    = 0;
    p->samplesPlayed = 0;
    p->volume      = 100;
    p->mute        = JAVACALL_FALSE;

    return p;
}

static javacall_result dshow_get_format(javacall_handle handle, jc_fmt* fmt)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** get format ***\n" );
    *fmt = p->mediaType;
    return JAVACALL_OK;
}

static javacall_result dshow_destroy(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;

    dshow_DBG( "*** destroy ***\n" );

    p->ap.shutdown();

    delete p;//FREE(p);

    return JAVACALL_OK;
}

static javacall_result dshow_close(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** close ***\n" );
    return JAVACALL_OK;
}

static javacall_result dshow_get_player_controls(javacall_handle handle,
                                               int* controls)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** get controls ***\n" );
    *controls = JAVACALL_MEDIA_CTRL_VOLUME;
    return JAVACALL_OK;
}

static javacall_result dshow_acquire_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** acquire device ***\n" );

    p->acquired = TRUE;

    return JAVACALL_OK;
}

static javacall_result dshow_release_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** release device ***\n" );

    p->acquired = FALSE;

    return JAVACALL_OK;
}

static javacall_result dshow_realize(javacall_handle handle, 
                                   javacall_const_utf16_string mime, 
                                   long mimeLength)
{
    dshow_player* p = (dshow_player*)handle;

    dshow_DBG( "*** realize ***\n" );

    if( !wcsncmp( (const wchar_t*)mime, L"audio/MPA", wcslen( L"audio/MPA" ) ) ||
        !wcsncmp( (const wchar_t*)mime, L"audio/X-MP3-draft-00", wcslen( L"audio/X-MP3-draft-00" ) ) )
    {
        p->rate     = 90000;
        p->channels = 1;

        p->mediaType = JC_FMT_RTP_MPA;

        get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        get_int_param( mime, (javacall_const_utf16_string)L"rate", &(p->rate) );

        p->ap.init( mimeLength, (wchar_t*)mime );

        p->realized = TRUE;
    }
    else
    {
        p->mediaType = JC_FMT_UNSUPPORTED;
    }

    return JAVACALL_OK;
}

static javacall_result dshow_prefetch(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** prefetch ***\n" );
    p->prefetched = TRUE;
    return JAVACALL_OK;
}

static javacall_result dshow_get_java_buffer_size(javacall_handle handle,
                                                long* java_buffer_size,
                                                long* first_chunk_size)
{
    dshow_player* p = (dshow_player*)handle;

    *java_buffer_size = XFER_BUFFER_SIZE;

    if( p->prefetched )
    {
        dshow_DBG( "*** get_java_buffer_size: XFER_BUFFER_SIZE ***\n" );
        *first_chunk_size = XFER_BUFFER_SIZE;

        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
            p->appId, p->playerId, JAVACALL_OK, NULL);

    }
    else
    {
        dshow_DBG( "*** get_java_buffer_size: 0 ***\n" );
        *first_chunk_size = 0;
    }

    return JAVACALL_OK;
}

static javacall_result dshow_set_whole_content_size(javacall_handle handle,
                                                  long whole_content_size)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

static javacall_result dshow_get_buffer_address(javacall_handle handle,
                                              const void** buffer,
                                              long* max_size)
{
    dshow_player* p = (dshow_player*)handle;

    *buffer   = p->buf;
    *max_size = XFER_BUFFER_SIZE;

    return JAVACALL_OK;
}

static javacall_result dshow_do_buffering(javacall_handle handle, 
                                        const void* buffer,
                                        long *length, 
                                        javacall_bool *need_more_data, 
                                        long *next_chunk_size)
{
    dshow_player* p = (dshow_player*)handle;

    dshow_DBG( "        do_buffering...\n" );

    /*
    FILE* f = fopen( "c:\\buffering.log", "a" );
    if( NULL != f )
    {
        for( int i = 0; i < *length; i++ )
        {
            fprintf(f, "%02X ", ((BYTE*)buffer)[i] );
        }
        fprintf(f,"\n");
        fclose(f);
    }
    */

    /*
    FILE* f1 = fopen( "c:\\buffering.mp3", "ab" );
    if( NULL != f1 )
    {
        fwrite( buffer, *length, 1, f1 );
        fclose(f1);
    }*/

    if( 0 != *length && NULL != buffer )
    {
        p->ap.data( *length, buffer );
    }
    else
    {
        dshow_DBG( "           [zero]\n" );
    }

    if( p->acquired || p->buffering )
    {
        dshow_DBG( "        continue...\n" );
        *need_more_data  = JAVACALL_TRUE;
        *next_chunk_size = XFER_BUFFER_SIZE;

        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
            p->appId, p->playerId, JAVACALL_OK, NULL);

        if(++p->enqueued > 10 )
        {
            p->buffering = FALSE;
            dshow_DBG( "        -- enough!\n" );
        }
        else
        {
            dshow_DBG( "        -- buffering...\n" );
        }
    }
    else
    {
        dshow_DBG( "        stop...\n" );
        *need_more_data  = JAVACALL_FALSE;
        *next_chunk_size = 0;
    }

    return JAVACALL_OK;
}

static javacall_result dshow_clear_buffer(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** clear buffer ***\n" );
    return JAVACALL_OK;
}

static javacall_result dshow_start(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** start ***\n" );
    p->playing = TRUE;
    p->ap.play();
    return JAVACALL_OK;
}

static javacall_result dshow_stop(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** stop ***\n" );
    p->playing = FALSE;
    p->ap.stop();
    return JAVACALL_OK;
}

static javacall_result dshow_pause(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** pause ***\n" );
    p->playing = FALSE;
    return JAVACALL_OK;
}

static javacall_result dshow_resume(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** resume ***\n" );
    p->playing = TRUE;
    return JAVACALL_OK;
}

static javacall_result dshow_get_time(javacall_handle handle, 
                                    long* ms)
{
    dshow_player* p = (dshow_player*)handle;

    return JAVACALL_OK;
}

static javacall_result dshow_set_time(javacall_handle handle, 
                                    long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_FAIL;
}

static javacall_result dshow_get_duration(javacall_handle handle, 
                                        long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_NO_DATA_AVAILABLE;
}

static javacall_result dshow_switch_to_foreground(javacall_handle handle,
                                                int options)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

static javacall_result dshow_switch_to_background(javacall_handle handle,
                                                int options)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

/*****************************************************************************\
                         V O L U M E   C O N T R O L
\*****************************************************************************/

static javacall_result dshow_get_volume(javacall_handle handle, 
                                      long* level)
{
    dshow_player* p = (dshow_player*)handle;
    *level = p->volume;
    return JAVACALL_OK;
}

static javacall_result dshow_set_volume(javacall_handle handle, 
                                      long* level)
{
    dshow_player* p = (dshow_player*)handle;
    p->volume = *level;
    return JAVACALL_OK;
}

static javacall_result dshow_is_mute(javacall_handle handle, 
                                   javacall_bool* mute )
{
    dshow_player* p = (dshow_player*)handle;
    *mute = p->mute;
    return JAVACALL_OK;
}

static javacall_result dshow_set_mute(javacall_handle handle,
                                    javacall_bool mute)
{
    dshow_player* p = (dshow_player*)handle;
    p->mute = mute;
    return JAVACALL_OK;
}

/*****************************************************************************\
                        I N T E R F A C E   T A B L E S
\*****************************************************************************/

static media_basic_interface _dshow_basic_itf =
{
    dshow_create,
    dshow_get_format,
    dshow_get_player_controls,
    dshow_close,
    dshow_destroy,
    dshow_acquire_device,
    dshow_release_device,
    dshow_realize,
    dshow_prefetch,
    dshow_start,
    dshow_stop,
    dshow_pause,
    dshow_resume,
    dshow_get_java_buffer_size,
    dshow_set_whole_content_size,
    dshow_get_buffer_address,
    dshow_do_buffering,
    dshow_clear_buffer,
    dshow_get_time,
    dshow_set_time,
    dshow_get_duration,
    dshow_switch_to_foreground,
    dshow_switch_to_background
};

static media_volume_interface _dshow_volume_itf = {
    dshow_get_volume,
    dshow_set_volume,
    dshow_is_mute,
    dshow_set_mute
};

media_interface g_dshow_itf =
{
    &_dshow_basic_itf,
    &_dshow_volume_itf,
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

//=============================================================================

} // extern "C"

//=============================================================================
