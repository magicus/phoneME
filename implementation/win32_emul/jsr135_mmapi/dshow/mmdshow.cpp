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

#include "multimedia.h"
#include <dshow.h>
#include "audioplayer.hpp"

//=============================================================================

//#define dshow_DBG( s )
#define dshow_DBG( s ) OutputDebugString( (s) )

#define XFER_BUFFER_SIZE  4096
#define EOM_TIMEOUT       5000

struct dshow_player : public ap_callback
{
    virtual void call( unsigned int bytes_in_queue );

    unsigned int          bytes_in_queue;
    bool                  notified;
    bool                  buffering;

    int                   appId;
    int                   playerId;
    jc_fmt                mediaType;
    javacall_utf16_string uri;

    int                   channels;
    int                   rate;

    BYTE                  buf[ XFER_BUFFER_SIZE ];

    bool                  realized;
    bool                  prefetched;
    bool                  acquired;
    volatile BOOL         playing;

    javacall_int64        samplesPlayed;

    long                  volume;
    javacall_bool         mute;

    audioplayer           ap;

    volatile HANDLE       hEomThread; // monitors buffering activity
    HANDLE                hBufEvent;  // if not signaled for EOM_TIMEOUT ms, EOM is generated
};

void dshow_player::call( unsigned int ap_bytes_in_queue )
{
    bytes_in_queue = ap_bytes_in_queue;

    if( ap_bytes_in_queue < XFER_BUFFER_SIZE )
    {
        if( !notified )
        {
            dshow_DBG( "        sending NMD...\n" );
            javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
                appId, playerId, JAVACALL_OK, NULL);

            notified  = true;
            buffering = true;
        }
    }
    else
    {
        notified = false;
    }
}

DWORD WINAPI dshow_eom_thread( LPVOID lpParameter )
{
    DWORD t0;
    dshow_player* p = (dshow_player*)lpParameter;

    dshow_DBG( "*** eom thread start ***\n" );

    while( NULL != p->hBufEvent )
    {
        if( p->playing )
        {
            t0 = GetTickCount();
            if( WAIT_TIMEOUT == WaitForSingleObject( p->hBufEvent, EOM_TIMEOUT ) )
            {
                if( p->playing )
                {
                    dshow_DBG( "*** buf timeout, sending EOM. ***\n" );
                    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                        p->appId, p->playerId, JAVACALL_OK, (void*)0 ); // IMPL_NOTE: mediatime!
                    p->ap.stop();
                    p->playing = FALSE;
                }
            }
            else
            {
                char str[ 80 ];
                sprintf( str, "*** buf ok, dt = %i ***\n", GetTickCount() - t0 );
                dshow_DBG( str );
            }
        }
        else
        {
            Sleep( 250 );
        }
    }

    dshow_DBG( "*** eom thread exit ***\n" );

    return 0;
}

//=============================================================================

extern "C" {

//=============================================================================

static javacall_handle dshow_create(int appId, 
                                  int playerId,
                                  jc_fmt mediaType,
                                  const javacall_utf16_string URI)
{
    dshow_player* p = new dshow_player;//(dshow_player*)MALLOC(sizeof(dshow_player));

    dshow_DBG( "*** create ***\n" );

    p->bytes_in_queue   = 0;
    p->notified         = false;
    p->appId            = appId;
    p->playerId         = playerId;
    p->mediaType        = mediaType;
    p->uri              = NULL;
    p->realized         = FALSE;
    p->prefetched       = FALSE;
    p->acquired         = FALSE;
    p->playing          = FALSE;
    p->buffering        = TRUE;
    p->samplesPlayed    = 0;
    p->volume           = 100;
    p->mute             = JAVACALL_FALSE;
    p->hBufEvent        = CreateEvent( NULL, FALSE, FALSE, NULL );
    p->hEomThread       = CreateThread( NULL, 0, dshow_eom_thread, p, 0, NULL );
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

    CloseHandle( p->hBufEvent );
    p->hBufEvent = NULL;
    // setting hBufEvent to NULL causes eom thread to exit
    WaitForSingleObject( p->hEomThread, INFINITE );

    p->ap.shutdown();

    delete p;

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

        p->ap.init( mimeLength, (wchar_t*)mime, p );

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

    char str[ 256 ];
    sprintf( str, "     do_buffering % 6i->% 6i...\n", p->bytes_in_queue, p->bytes_in_queue + *length );
    dshow_DBG( str );

    if( 0 != *length && NULL != buffer )
    {
        p->ap.data( *length, buffer );
        p->bytes_in_queue += *length;
        SetEvent( p->hBufEvent );
    }
    else
    {
        dshow_DBG( "           [zero]\n" );
    }

    if( p->buffering )
    {
        if( p->bytes_in_queue > XFER_BUFFER_SIZE * 10 )
        {
            p->buffering = FALSE;
            dshow_DBG( "        -- buffering stopped.\n" );

            if( p->playing )
            {
                dshow_DBG( "        ** resuming playback.\n" );
                p->ap.play();
            }
        }
        else
        {
            if( p->playing )
            {
                dshow_DBG( "        ** stopping playback.\n" );
                p->ap.stop();
            }
        }
    }

    if( p->buffering )
    {
        dshow_DBG( "        need more data immmediately...\n" );
        *need_more_data  = JAVACALL_TRUE;
    }
    else
    {
        dshow_DBG( "        enough data for now, posting NMD.\n" );
        *need_more_data  = JAVACALL_FALSE;

        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
            p->appId, p->playerId, JAVACALL_OK, NULL);

    }

    *next_chunk_size = XFER_BUFFER_SIZE;

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
    p->ap.stop();
    return JAVACALL_OK;
}

static javacall_result dshow_resume(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    dshow_DBG( "*** resume ***\n" );
    p->playing = TRUE;
    p->ap.play();
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
