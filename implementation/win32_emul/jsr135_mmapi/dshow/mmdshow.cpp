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

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
	va_list        args;

	va_start(args, fmt);
    vsprintf( str8, fmt, args );
	va_end(args);

    OutputDebugString( str8 );
}

#define XFER_BUFFER_SIZE  4096
#define ENQUEUE_PACKETS   20
#define EOM_TIMEOUT       5000

struct xfer_buffer
{
    xfer_buffer() : next(NULL){};
    BYTE            data[XFER_BUFFER_SIZE];
    xfer_buffer*    next;
};

struct dshow_player
{
    long                  get_media_time();

    int                   appId;
    int                   playerId;
    jc_fmt                mediaType;
    javacall_utf16_string uri;

    int                   channels;
    int                   rate;
    int                   duration;

    xfer_buffer*          cur_buf;
    xfer_buffer*          queue_head;
    xfer_buffer*          queue_tail;
    int                   queue_size;

    bool                  realized;
    bool                  prefetched;
    bool                  acquired;
    volatile bool         playing;
    volatile bool         eom;

    long                  volume;
    javacall_bool         mute;

    audioplayer           ap;

    volatile HANDLE       hEomThread; // monitors buffering activity
    HANDLE                hBufEvent;  // if not signaled for EOM_TIMEOUT ms, EOM is generated
    CRITICAL_SECTION      cs;
};

long dshow_player::get_media_time()
{
    double t;
    if( ap.tell( &t ) )
    {
        long mt = long( t * 1000.0 );
        PRINTF( "media time = %ld\n", mt );
        return mt;
    }
    else
    {
        return -1;
    }
}

DWORD WINAPI dshow_eom_thread( LPVOID lpParameter )
{
    DWORD t0;
    dshow_player* p = (dshow_player*)lpParameter;

    HRESULT hr=S_OK;
    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if( FAILED( hr ) )
    {
        return 0;
    }

    PRINTF( "*** eom thread start ***\n" );

    while( NULL != p->hBufEvent )
    {
        if( p->playing && !p->eom )
        {
            t0 = GetTickCount();

            long mt          = p->get_media_time();
            long time_left   = p->duration - mt;
            bool eom_reached = ( -1 != p->duration && time_left <= 0 );
            bool no_packets  = false;

            DWORD timeout = EOM_TIMEOUT;
            if( -1 != p->duration && time_left < EOM_TIMEOUT ) {
                timeout = time_left;
            }

            if( !eom_reached ) {
                no_packets = ( WAIT_TIMEOUT == WaitForSingleObject( p->hBufEvent, timeout ) );
            }

            if( p->playing && !p->eom )
            {
                if( eom_reached ) PRINTF( "*** EOM EOM EOM ***\n" );
                if( no_packets  ) PRINTF( "*** NO  PACKETS ***\n" );

                if( eom_reached || no_packets )
                {
                    PRINTF( "***               dt = %i, time_left = %ld ***\n", GetTickCount() - t0, time_left );
                    if( p->playing )
                    {
                        PRINTF( "***              stopping player... ***\n" );

                        p->ap.stop();
                        p->playing = false;

                        PRINTF( "***              sending EOM... ***\n" );

                        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                            p->appId, p->playerId, JAVACALL_OK, (void*)(p->get_media_time()) );

                        p->eom = true;
                    }
                }
            }
        }
        else
        {
            Sleep( 250 );
        }
    }

    PRINTF( "*** eom thread exit ***\n" );

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
    dshow_player* p = new dshow_player;

    PRINTF( "*** create ***\n" );

    p->appId            = appId;
    p->playerId         = playerId;
    p->mediaType        = mediaType;
    p->uri              = NULL;

    p->realized         = false;
    p->prefetched       = false;
    p->acquired         = false;
    p->playing          = false;
    p->eom              = false;

    p->volume           = 100;
    p->mute             = JAVACALL_FALSE;

    p->hBufEvent        = CreateEvent( NULL, FALSE, FALSE, NULL );
    p->hEomThread       = CreateThread( NULL, 0, dshow_eom_thread, p, 0, NULL );

    p->cur_buf          = NULL;
    p->queue_head       = NULL;
    p->queue_tail       = NULL;
    p->queue_size       = 0;

    return p;
}

static javacall_result dshow_get_format(javacall_handle handle, jc_fmt* fmt)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** get format ***\n" );
    *fmt = p->mediaType;
    return JAVACALL_OK;
}

static javacall_result dshow_destroy(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    xfer_buffer* pbuf;
    xfer_buffer* t;

    PRINTF( "*** destroy ***\n" );

    CloseHandle( p->hBufEvent );
    p->hBufEvent = NULL;
    // setting hBufEvent to NULL causes eom thread to exit
    WaitForSingleObject( p->hEomThread, INFINITE );

    p->ap.shutdown();

    pbuf = p->queue_head;
    while(NULL != pbuf) 
    {
        t = pbuf;
        pbuf = pbuf->next;
        delete t;
    }
    p->queue_head = NULL;
    p->queue_tail = NULL;

    if(NULL != p->cur_buf) delete p->cur_buf;

    delete p;

    return JAVACALL_OK;
}

static javacall_result dshow_close(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** close ***\n" );
    return JAVACALL_OK;
}

static javacall_result dshow_get_player_controls(javacall_handle handle,
    int* controls)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** get controls ***\n" );
    *controls = JAVACALL_MEDIA_CTRL_VOLUME;
    return JAVACALL_OK;
}

static javacall_result dshow_acquire_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** acquire device ***\n" );

    p->acquired = true;

    return JAVACALL_OK;
}

static javacall_result dshow_release_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** release device ***\n" );

    p->acquired = false;

    return JAVACALL_OK;
}

static javacall_result dshow_realize(javacall_handle handle, 
    javacall_const_utf16_string mime, 
    long mimeLength)
{
    dshow_player* p = (dshow_player*)handle;

    PRINTF( "*** realize ***\n" );

    p->duration = -1;

    if( !wcsncmp( (const wchar_t*)mime, L"audio/mp3",  wcslen( L"audio/mp3"  ) ) ||
        !wcsncmp( (const wchar_t*)mime, L"audio/mpeg", wcslen( L"audio/mpeg" ) ) ||
        !wcsncmp( (const wchar_t*)mime, L"audio/MPA",  wcslen( L"audio/MPA"  ) ) ||
        !wcsncmp( (const wchar_t*)mime, L"audio/X-MP3-draft-00", wcslen( L"audio/X-MP3-draft-00" ) ) )
    {
        p->rate     = 90000;
        p->channels = 1;

        p->mediaType = JC_FMT_MPEG1_LAYER3;

        get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        get_int_param( mime, (javacall_const_utf16_string)L"rate",     &(p->rate)     );
        get_int_param( mime, (javacall_const_utf16_string)L"duration", &(p->duration) );

        p->ap.init( mimeLength, (wchar_t*)mime );

        p->realized = true;
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
    PRINTF( "*** prefetch ***\n" );
    p->prefetched = true;
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
        PRINTF( "*** get_java_buffer_size: XFER_BUFFER_SIZE ***\n" );
        *first_chunk_size = XFER_BUFFER_SIZE;
    }
    else
    {
        PRINTF( "*** get_java_buffer_size: 0 ***\n" );
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

    if(NULL == p->cur_buf) p->cur_buf = new xfer_buffer;

    *buffer   = p->cur_buf->data;
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

    PRINTF( "     do_buffering %i->%i...\n", p->queue_size, p->queue_size + 1 );

    if( 0 != *length && NULL != buffer )
    {
        assert( buffer == p->cur_buf->data );

        if(p->playing)
        {
            // IMPL_NOTE: current player version supports only one-byte sends
            // p->ap.data( *length, buffer );
            for(int i=0; i < *length; i++ ) {
                p->ap.data( 1, (const BYTE*)buffer + i );
            }

            SetEvent( p->hBufEvent );
            *need_more_data  = JAVACALL_FALSE;
            PRINTF( "        posting NMD.\n" );
            javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
                p->appId, p->playerId, JAVACALL_OK, NULL);

        }
        else
        {
            if( NULL == p->queue_tail ) // first packet
                p->queue_head = p->cur_buf;
            else
                p->queue_tail->next = p->cur_buf;

            p->queue_tail = p->cur_buf;
            p->cur_buf = NULL;
            p->queue_size++;

            if(p->queue_size < ENQUEUE_PACKETS)
            {
                PRINTF( "        need more data.\n" );
                *need_more_data = JAVACALL_TRUE;
            }
            else
            {
                PRINTF( "        enough data,posting NMD.\n" );
                *need_more_data = JAVACALL_FALSE;
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
                    p->appId, p->playerId, JAVACALL_OK, NULL);
            }
        }
    }
    else
    {
        PRINTF( "           [zero -- sending EOM...]\n" );
        //PRINTF( "***              stopping player... ***\n" );
        //p->ap.stop();
        p->playing = false;
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
            p->appId, p->playerId, JAVACALL_OK, (void*)(p->get_media_time()) );
        p->eom = true;
    }

    *next_chunk_size = XFER_BUFFER_SIZE;
    return JAVACALL_OK;
}

static javacall_result dshow_clear_buffer(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** clear buffer ***\n" );
    return JAVACALL_OK;
}

static javacall_result dshow_start(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** start ***\n" );
    p->playing = true;
    p->ap.play();

    DWORD t = GetTickCount();

    while(NULL != p->queue_head)
    {
        // IMPL_NOTE: current player version supports only one-byte sends
        // should be p->ap.data(XFER_BUFFER_SIZE,p->queue_head->data);
        for(int i=0; i < XFER_BUFFER_SIZE; i++) {
            p->ap.data(1,p->queue_head->data+i);
        }

        xfer_buffer* t = p->queue_head;
        p->queue_head = p->queue_head->next;
        p->queue_size--;
        delete t;
    }

    PRINTF( "*** started in %i ms.", GetTickCount() - t );

    p->queue_tail = NULL;

    return JAVACALL_OK;
}

static javacall_result dshow_stop(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** stop ***\n" );
    p->playing = false;
    p->ap.stop();
    return JAVACALL_OK;
}

static javacall_result dshow_pause(javacall_handle handle)
{
    return dshow_stop(handle);
}

static javacall_result dshow_resume(javacall_handle handle)
{
    return dshow_start(handle);
}

static javacall_result dshow_get_time(javacall_handle handle, long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    *ms = p->get_media_time();
    return JAVACALL_OK;
}

static javacall_result dshow_set_time(javacall_handle handle, long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_FAIL;
}

static javacall_result dshow_get_duration(javacall_handle handle, long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    *ms = p->duration;
    return JAVACALL_OK;
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
