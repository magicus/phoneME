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
#include "player_dshow.hpp"

//=============================================================================

extern "C" {
    // following functions are defined in lcd.c (MIDP-related javacall):
    void lcd_set_color_key( javacall_bool use_keying, javacall_pixel key_color );
    void lcd_set_video_rect( int x, int y, int w, int h );
    void lcd_output_video_frame( javacall_pixel* video );
}

// ===========================================================================

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
    va_list        args;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);

    //OutputDebugString( str8 );
}

#define XFER_BUFFER_SIZE  4096

class dshow_player : public player_callback
{
    virtual void frame_ready( bits16 const* pFrame );
    virtual void size_changed( int16 w, int16 h );
    virtual void playback_finished();

public:
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

    long                  video_width;
    long                  video_height;
    javacall_pixel*       video_frame;

    BYTE                  buf[ XFER_BUFFER_SIZE ];

    bool                  realizing;
    bool                  prefetching;
    bool                  prefetched;
    bool                  playing;

    long                  bytes_buffered;
    bool                  all_data_arrived;

    long                  volume;
    javacall_bool         mute;

    player*               ppl;
};

void dshow_player::frame_ready( bits16 const* pFrame )
{
    if( NULL == video_frame ) video_frame = new javacall_pixel[ video_width * video_height ];

    for( int y = 0; y < video_height; y++ )
    {
        memcpy( video_frame + y * video_width, 
                pFrame + ( video_height - y - 1 ) * video_width,
                sizeof( javacall_pixel ) * video_width );
    }

    lcd_output_video_frame( video_frame );
}

void dshow_player::size_changed( int16 w, int16 h )
{
    video_width  = w;
    video_height = h;
}

void dshow_player::playback_finished()
{
    long t = get_media_time();
    javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                                      appId, playerId, JAVACALL_OK, (void*)t );
}

long dshow_player::get_media_time()
{
    if( !playing )
    {
        return media_time;
    }
    else
    {
        player::result r;
        int64 time = ppl->get_media_time( &r );
        if( player::time_unknown != time ) time /= 1000;
        return ( media_time = long( time ) );
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

    p->realizing        = false;
    p->prefetching      = false;
    p->prefetched       = false;
    p->playing          = false;

    p->all_data_arrived = false;
    p->bytes_buffered   = 0;

    p->media_time       = 0;
    p->volume           = 100;
    p->mute             = JAVACALL_FALSE;

    p->video_width      = 0;
    p->video_height     = 0;
    p->video_frame      = NULL;

    p->ppl              = NULL;

    *pHandle =(javacall_handle)p;

    lcd_set_color_key( JAVACALL_FALSE, 0 );

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

    lcd_output_video_frame( NULL );

    if( NULL != p->video_frame ) delete p->video_frame;
    if( NULL != p->ppl ) delete p->ppl;
    delete p;
    return JAVACALL_OK;
}

static javacall_result dshow_close(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** close ***\n" );
    if( NULL != p->ppl ) p->ppl->close();

    lcd_output_video_frame( NULL );

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

    p->prefetched = true;

    return JAVACALL_OK;
}

static javacall_result dshow_release_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** release device ***\n" );

    return JAVACALL_OK;
}

static bool mime_equal( javacall_const_utf16_string mime, long mimeLength, const wchar_t* v )
{
    return !wcsncmp( (const wchar_t*)mime, v, min( (size_t)mimeLength, wcslen( v ) ) );
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

        get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        get_int_param( mime, (javacall_const_utf16_string)L"rate",     &(p->rate)     );
        get_int_param( mime, (javacall_const_utf16_string)L"duration", &(p->duration) );

        p->mediaType = JC_FMT_MPEG1_LAYER3;
        mime = (javacall_const_utf16_string)L"audio/mpeg";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"video/x-vp6" ) ||
             mime_equal( mime, mimeLength, L"video/x-flv" ) )
    {
        p->mediaType = JC_FMT_FLV;
        mime = (javacall_const_utf16_string)L"video/x-flv";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else
    {
        p->mediaType = JC_FMT_UNSUPPORTED;
    }

    if( JC_FMT_UNSUPPORTED != p->mediaType )
    {
        if( create_player_dshow( mimeLength, (const char16*)mime, p, &(p->ppl) ) )
        {
            if( player::result_success != p->ppl->realize() )
            {
                PRINTF( "*** player::realize() failed! ***\n" );
                return JAVACALL_FAIL;
            }

            p->realizing = true;

            return JAVACALL_OK;
        }
    }

    return JAVACALL_FAIL;
}

static javacall_result dshow_prefetch(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** prefetch ***\n" );
    p->prefetching = true;
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
    else if( p->prefetching )
    {
        PRINTF( "*** 0x%08X get_java_buffer_size: prefetching, 0 ***\n", handle );
        *first_chunk_size = 0;
    }
    else // if( p->realizing )
    {
        PRINTF( "*** 0x%08X get_java_buffer_size: realizing, XFER_BUFFER_SIZE ***\n", handle );
        *first_chunk_size = XFER_BUFFER_SIZE;
    }

    return JAVACALL_OK;
}

static javacall_result dshow_set_whole_content_size(javacall_handle handle,
    long whole_content_size)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** 0x%08X set_whole_content_size: %ld***\n", handle, whole_content_size );
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

        p->bytes_buffered += *length;
        p->ppl->data( *length, buffer );

        if( p->prefetched )
        {
            *need_more_data  = JAVACALL_TRUE;
        }
        else if( p->prefetching )
        {
            *need_more_data  = JAVACALL_FALSE;
        }
        else if( p->realizing )
        {
            *need_more_data  = ( p->bytes_buffered < XFER_BUFFER_SIZE * 50 ) 
                               ? JAVACALL_TRUE : JAVACALL_FALSE;

            if( JAVACALL_FALSE == *need_more_data )
            {
                if( player::result_success != p->ppl->prefetch() )
                {
                    PRINTF( "*** player::prefetch() failed! ***\n" );
                    return JAVACALL_FAIL;
                }
            }
        }
        else
        {
            assert( FALSE );
        }

        *next_chunk_size = XFER_BUFFER_SIZE;

        javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
                                          p->appId, p->playerId, JAVACALL_OK, NULL);
    }
    else
    {
        PRINTF( "           [all data arrived]\n" );
        p->all_data_arrived = true;
        p->ppl->data( 0, NULL );

        *need_more_data  = JAVACALL_FALSE;
        *next_chunk_size = 0;

        long t = p->get_media_time();
        if( -1 != p->duration && t > p->duration ) t = p->duration;
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
    if( player::result_success == p->ppl->start() )
    {
        PRINTF( "*** started ***\n" );
        p->playing = true;
        return JAVACALL_OK;
    }
    else
    {
        PRINTF( "*** start failed ***\n" );
        return JAVACALL_FAIL;
    }
}

static javacall_result dshow_stop(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** stop... ***\n" );
    p->get_media_time();
    if( player::result_success == p->ppl->stop() )
    {
        p->playing = false;
        PRINTF( "*** ...stopped ***\n" );
        return JAVACALL_OK;
    }
    else
    {
        PRINTF( "*** ...stop failed ***\n" );
        return JAVACALL_FAIL;
    }
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
    // if( p->duration != -1 && *ms > p->duration ) *ms = p->duration;
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
                       R A T E    C O N T R O L
\*****************************************************************************/

javacall_result dshow_get_max_rate(javacall_handle handle, long* maxRate)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_get_min_rate(javacall_handle handle, long* minRate)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_set_rate(javacall_handle handle, long rate)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_get_rate(javacall_handle handle, long* rate)
{
    dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

/*****************************************************************************\
                      V I D E O    C O N T R O L
\*****************************************************************************/

static javacall_result dshow_get_video_size(javacall_handle handle, long* width, long* height)
{
    dshow_player* p = (dshow_player*)handle;

    *width  = p->video_width;
    *height = p->video_height;

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
    return JAVACALL_FAIL;
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

static media_rate_interface _dshow_rate_itf = {
    dshow_get_max_rate,
    dshow_get_min_rate,
    dshow_set_rate,
    dshow_get_rate
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
    NULL, //&_dshow_rate_itf
    NULL,
    NULL,
    NULL,
    NULL
};

//=============================================================================

} // extern "C"

//=============================================================================
