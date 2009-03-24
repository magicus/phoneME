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

#include "../multimedia.h"
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

//#define PRINTF printf

#define XFER_BUFFER_SIZE  4096
#define ENQUEUE_PACKETS   20
#define EOM_TIMEOUT       5000

struct dshow_player
{
    long                  get_media_time();

    int                   appId;
    int                   playerId;
    jc_fmt                mediaType;
    javacall_utf16_string uri;
    bool                  is_video;

    int                   channels;
    int                   rate;
    int                   duration;
    long                  media_time;

    BYTE                  buf[ XFER_BUFFER_SIZE ];

    bool                  realized;
    bool                  prefetched;
    bool                  acquired;
    bool                  playing;
    bool                  all_data_arrived;

    long                  volume;
    javacall_bool         mute;

    audioplayer           ap;
};

long dshow_player::get_media_time()
{
    if( !playing ) return media_time;

    double t;
    if( ap.tell( &t ) )
    {
        media_time = long( t * 1000.0 );
        return media_time;
    }
    else
    {
        return -1;
    }
}

//=============================================================================

extern "C" {

//=============================================================================

static javacall_result dshow_create(int appId, 
    int playerId,
    jc_fmt mediaType,
    const javacall_utf16_string URI, 
    javacall_handle* pHandle)
{
    dshow_player* p = new dshow_player;

    PRINTF( "\n\n*** create ***\n" );

    p->appId            = appId;
    p->playerId         = playerId;
    p->mediaType        = mediaType;
    p->uri              = NULL;
    p->is_video         = ( JC_FMT_FLV == mediaType );

    p->realized         = false;
    p->prefetched       = false;
    p->acquired         = false;
    p->playing          = false;
    p->all_data_arrived = false;

    p->media_time       = 0;
    p->volume           = 100;
    p->mute             = JAVACALL_FALSE;

    *pHandle =(javacall_handle)p;

    return JAVACALL_OK;
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
    PRINTF( "*** destroy ***\n" );

    p->ap.shutdown();
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
    if( p->is_video ) *controls |= JAVACALL_MEDIA_CTRL_VIDEO;
    return JAVACALL_OK;
}

static javacall_result dshow_acquire_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** acquire device ***\n" );

    p->ap.init2();
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

static bool mime_equal( javacall_const_utf16_string mime, long mimeLength, const wchar_t* v )
{
    return !wcsncmp( (const wchar_t*)mime, v, min( mimeLength, wcslen( v ) ) );
}

static javacall_result dshow_realize(javacall_handle handle, 
    javacall_const_utf16_string mime, 
    long mimeLength)
{
    dshow_player* p = (dshow_player*)handle;

    PRINTF( "*** realize ***\n" );

    p->duration = -1;

    if( NULL == mime || mime_equal( mime, mimeLength, L"text/plain" ) )
    {
        switch( p->mediaType )
        {
        case JC_FMT_FLV:          mime = (javacall_const_utf16_string)L"video/x-flv"; break;
        case JC_FMT_MPEG1_LAYER3: mime = (javacall_const_utf16_string)L"audio/mpeg";  break;
        default:
            return JAVACALL_FAIL;
        }
        mimeLength = wcslen( (const wchar_t*)mime );
    }

    if( mime_equal( mime, mimeLength, L"audio/mp3"  ) ||
        mime_equal( mime, mimeLength, L"audio/mpeg" ) ||
        mime_equal( mime, mimeLength, L"audio/MPA"  ) ||
        mime_equal( mime, mimeLength, L"audio/X-MP3-draft-00" ) )
    {
        p->rate     = 90000;
        p->channels = 1;

        p->mediaType = JC_FMT_MPEG1_LAYER3;

        get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        get_int_param( mime, (javacall_const_utf16_string)L"rate",     &(p->rate)     );
        get_int_param( mime, (javacall_const_utf16_string)L"duration", &(p->duration) );

        p->ap.init1( mimeLength, (wchar_t*)mime );

        p->realized = true;
    }
    else if( mime_equal( mime, mimeLength, L"video/x-vp6" ) ||
             mime_equal( mime, mimeLength, L"video/x-flv" ) )
    {
        p->mediaType = JC_FMT_FLV;

        p->ap.init1( mimeLength, (wchar_t*)mime );

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
        PRINTF( "*** 0x%08X get_java_buffer_size: XFER_BUFFER_SIZE ***\n", handle );
        *first_chunk_size = XFER_BUFFER_SIZE;
    }
    else
    {
        PRINTF( "*** 0x%08X get_java_buffer_size: 0 ***\n", handle );
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

    PRINTF( "     0x%08X do_buffering...\n", handle );

    if( 0 != *length && NULL != buffer )
    {
        assert( buffer == p->buf );
        p->ap.data( *length, buffer );

        *need_more_data  = JAVACALL_FALSE;
        *next_chunk_size = XFER_BUFFER_SIZE;

        javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
                                          p->appId, p->playerId, JAVACALL_OK, NULL);
    }
    else
    {
        PRINTF( "           [zero]\n" );
        p->all_data_arrived = true;
        *need_more_data  = JAVACALL_FALSE;
        *next_chunk_size = 0;

        long t = p->get_media_time();
        if( -1 != p->duration && t > p->duration ) t = p->duration;

        javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                                          p->appId, p->playerId, JAVACALL_OK, (void*)t );

    }

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
    p->ap.play();
    p->playing = true;
    return JAVACALL_OK;
}

static javacall_result dshow_stop(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** stop... ***\n" );
    p->get_media_time();
    p->playing = false;
    p->ap.stop();
    // setting p->playing to false causes thread to exit
    PRINTF( "*** ...stopped ***\n" );

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
    if( p->duration != -1 && *ms > p->duration ) *ms = p->duration;
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
V I D E O    I N T E R F A C E
\*****************************************************************************/

// following functions are defined in lcd.c (MIDP-related javacall):
void lcd_set_color_key( javacall_bool use_keying, javacall_pixel key_color );
void lcd_set_video_rect( int x, int y, int w, int h );
void lcd_output_video_frame( javacall_pixel* video );

// ===========================================================================

static javacall_result dshow_get_video_size(javacall_handle handle, long* width, long* height)
{
    return JAVACALL_OK;
}

static javacall_result dshow_set_video_visible(javacall_handle handle, javacall_bool visible)
{
    return JAVACALL_OK;
}

static javacall_result dshow_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    lcd_set_video_rect( x, y, w, h );
    return JAVACALL_OK;
}

static javacall_result dshow_set_video_alpha(javacall_handle handle, javacall_bool on, javacall_pixel color)
{
    lcd_set_color_key( on, color );
    return JAVACALL_OK;
}

static javacall_result dshow_set_video_fullscreenmode(javacall_handle handle, javacall_bool fullScreenMode)
{
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

static media_video_interface _dshow_video_itf = {
    dshow_get_video_size,
    dshow_set_video_visible,
    dshow_set_video_location,
    dshow_set_video_alpha,
    dshow_set_video_fullscreenmode
};

media_interface g_dshow_itf =
{
    &_dshow_basic_itf,
    &_dshow_volume_itf,
    &_dshow_video_itf,
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
