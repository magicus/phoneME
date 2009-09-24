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

#include <math.h>
#include <process.h>

#include "multimedia.h"
#include "javacall_lcd.h"

//=============================================================================

// following functions are defined in lcd.c (MIDP-related javacall):
void lcd_set_color_key( javacall_bool use_keying, javacall_pixel key_color );
void lcd_set_video_rect( int x, int y, int w, int h );
void lcd_output_video_frame( javacall_pixel* video );

// ===========================================================================

#define DEBUG_ONLY(x)

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
    va_list        args;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);

    //OutputDebugStringA( str8 );
}

typedef struct _fake_camera
{
    int                   appId;
    int                   playerId;

    long                  video_width;
    long                  video_height;
    javacall_uint8*       video_frame;

    int                   out_width;
    int                   out_height;
    javacall_pixel*       out_frame;

    javacall_uint8*       snapshot;
    javacall_uint32       snapshot_len;

    volatile BOOL         playing;
    BOOL                  visible;

    HANDLE                hThread;
    CRITICAL_SECTION      cs;
} fake_camera;

static javacall_pixel rgb2pixel( javacall_uint8 r, 
                                 javacall_uint8 g,
                                 javacall_uint8 b )
{
    return    ( ((javacall_pixel)b >> 3) & 0x1f)
           | (( ((javacall_pixel)g >> 2) & 0x3f) << 5 )
           | (( ((javacall_pixel)r >> 3) & 0x1f) << 11);
}

static void fake_camera_prepare_scaled_frame( fake_camera* c )
{
    int x, y, srcx, srcy;

    double kx = (double)(c->video_width) / (double)(c->out_width);
    double ky = (double)(c->video_height) / (double)(c->out_height);
    javacall_uint8* p;

    if( NULL == c->out_frame ) 
        c->out_frame = (javacall_pixel*)MALLOC( sizeof(javacall_pixel) * c->out_width * c->out_height );

    for( y = 0; y < c->out_height; y++ )
    {
        for( x = 0; x < c->out_width; x++ )
        {
            srcx = (int)( 0.5 + kx * x );
            srcy = (int)( 0.5 + ky * y );
            p = c->video_frame + 3 * ( srcy * c->video_width + srcx );
            *( c->out_frame + y * c->out_width + x )
                = rgb2pixel( *(p++), *(p++), *(p++) );
        }
    }
}

static void fake_camera_frame_ready( fake_camera* c )
{
    if( c->visible )
    {
        fake_camera_prepare_scaled_frame( c );
        lcd_output_video_frame( c->out_frame );
    }
}

static void fake_camera_generator_thread( void* param )
{
    int x, y;
    javacall_uint8* p;
    fake_camera* c = (fake_camera*)param;

    while( c->playing )
    {
        EnterCriticalSection( &(c->cs) );
        for( x = 0; x < c->video_width; x++ )
        {
            for( y = 0; y < c->video_height; y++ )
            {
                p = c->video_frame + 3 * ( y * c->video_width + x );
                *(p++) = rand() & 0xFF;
                *(p++) = rand() & 0xFF;
                *(p++) = rand() & 0xFF;
            }
        }

        fake_camera_frame_ready( c );
        LeaveCriticalSection( &(c->cs) );

        Sleep( 50 );
    }
}

//=============================================================================

static javacall_result fake_camera_create(javacall_impl_player* outer_player)
{
    fake_camera* c = (fake_camera*)MALLOC( sizeof(fake_camera) );

    DEBUG_ONLY( PRINTF( "*** fake_camera_create: 0x%08X->0x%08X ***\n", outer_player, c ); )

    c->appId       = outer_player->appId;
    c->playerId    = outer_player->playerId;

    c->out_width   = c->video_width  = 160;
    c->out_height  = c->video_height = 120;

    c->video_frame = (javacall_uint8*)MALLOC( 3 * c->video_width 
                                                * c->video_height );
    c->out_frame   = NULL;

    c->snapshot     = NULL;
    c->snapshot_len = 0;

    c->playing     = FALSE;
    c->visible     = FALSE;

    c->hThread     = NULL;

    InitializeCriticalSection( &(c->cs) );

    outer_player->mediaHandle = (javacall_handle)c;

    return JAVACALL_OK;
}

static javacall_result fake_camera_destroy(javacall_handle handle)
{
    fake_camera* c        = (fake_camera*)handle;
    int          appId    = c->appId;
    int          playerId = c->playerId;

    DEBUG_ONLY( PRINTF( "*** fake_camera_destroy 0x%08X ***\n", handle ); )

    if( c->playing )
    {
        c->playing = FALSE;
        WaitForSingleObject( c->hThread, INFINITE );
        c->hThread = NULL;
    }

    if( NULL != c->video_frame ) FREE( c->video_frame );

    if( NULL != c->out_frame )
    {
        lcd_output_video_frame( NULL );
        FREE( c->out_frame );
    }

    if( NULL != c->snapshot ) javacall_media_release_data( c->snapshot, c->snapshot_len );

    DeleteCriticalSection( &(c->cs) );
    FREE( c );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DESTROY_FINISHED,
                                     appId,
                                     playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result fake_camera_get_player_controls(javacall_handle handle,
    int* controls)
{
    //fake_camera* c = (fake_camera*)handle;
    *controls = JAVACALL_MEDIA_CTRL_VIDEO;
    return JAVACALL_OK;
}

//=============================================================================

static void fake_camera_do_start(fake_camera* c)
{
    c->playing = TRUE;
    c->hThread = (HANDLE)_beginthread( fake_camera_generator_thread, 0, c );
}

static void fake_camera_do_stop(fake_camera* c)
{
    c->playing = FALSE;
    WaitForSingleObject( c->hThread, INFINITE );
    c->hThread = NULL;
}

//=============================================================================

static javacall_result fake_camera_stop(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    DEBUG_ONLY( PRINTF( "*** fake_camera_stop ***\n" ); )

    if( c->playing ) fake_camera_do_stop( c );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_STOP_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result fake_camera_pause(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    DEBUG_ONLY( PRINTF( "*** fake_camera_pause ***\n" ); )

    if( c->playing ) fake_camera_do_stop( c );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_PAUSE_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result fake_camera_run(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    DEBUG_ONLY( PRINTF( "*** fake_camera_run ***\n" ); )

    if( !c->playing ) fake_camera_do_start( c );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_RUN_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

//=============================================================================

static javacall_result fake_camera_get_time(javacall_handle handle, javacall_int32* ms)
{
    //fake_camera* c = (fake_camera*)handle;
    *ms = -1;
    return JAVACALL_OK;
}

static javacall_result fake_camera_set_time(javacall_handle handle, javacall_int32 ms)
{
    return JAVACALL_FAIL;
}

static javacall_result fake_camera_get_duration(javacall_handle handle, javacall_int32* ms)
{
    return JAVACALL_FAIL;
}

static javacall_result fake_camera_switch_to_foreground(javacall_handle handle,
    int options)
{
    return JAVACALL_OK;
}

static javacall_result fake_camera_switch_to_background(javacall_handle handle,
    int options)
{
    return JAVACALL_OK;
}

/*****************************************************************************\
                      V I D E O    C O N T R O L
\*****************************************************************************/

static javacall_result fake_camera_get_video_size(javacall_handle handle, long* width, long* height)
{
    fake_camera* c = (fake_camera*)handle;

    *width  = c->video_width;
    *height = c->video_height;

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_visible(javacall_handle handle, javacall_bool visible)
{
    fake_camera* c = (fake_camera*)handle;

    EnterCriticalSection( &(c->cs) );
    c->visible = ( JAVACALL_TRUE == visible );
    if( visible )
    {
        if( NULL != c->out_frame ) lcd_output_video_frame( c->out_frame );
    }
    else
    {
        lcd_output_video_frame( NULL );
    }
    LeaveCriticalSection( &(c->cs) );

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    fake_camera* c = (fake_camera*)handle;
    javacall_pixel* old_frame;

    EnterCriticalSection( &(c->cs) );

    if( c->out_width != w || c->out_height != h )
    {
        c->out_width  = w;
        c->out_height = h;

        old_frame = c->out_frame;
        c->out_frame = NULL;

        if( NULL != c->video_frame ) fake_camera_prepare_scaled_frame( c );

        lcd_set_video_rect( x, y, w, h );
        lcd_output_video_frame( c->out_frame );

        if( NULL != old_frame ) FREE( old_frame );
    }
    else
    {
        lcd_set_video_rect( x, y, w, h );
    }

    LeaveCriticalSection( &(c->cs) );

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_alpha(javacall_handle handle, javacall_bool on, javacall_pixel color)
{
    lcd_set_color_key( on, color );
    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_fullscreenmode(javacall_handle handle, javacall_bool fullScreenMode)
{
    return JAVACALL_OK;
}

/*****************************************************************************\
                      S N A P S H O T   S U P P O R T
\*****************************************************************************/

static javacall_result fake_camera_start_video_snapshot( javacall_handle handle, 
                                                         const javacall_utf16* imageType, 
                                                         long length )
{
    javacall_result res;
    fake_camera* c = (fake_camera*)handle;

    EnterCriticalSection( &(c->cs) );

    res = javacall_media_encode_start( c->video_frame,
                                       (javacall_uint16)c->video_width,
                                       (javacall_uint16)c->video_height,
                                       JAVACALL_JPEG_ENCODER,
                                       100, // javacall_uint8 quality,
                                       &( c->snapshot ),
                                       &( c->snapshot_len ),
                                       NULL );

    LeaveCriticalSection( &(c->cs) );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
                                     c->appId,
                                     c->playerId,
                                     res,
                                     NULL );

    return res;
}

static javacall_result fake_camera_get_video_snapshot_data_size( javacall_handle handle, 
                                                                 long* size )
{
    fake_camera* c = (fake_camera*)handle;

    if( NULL == c->snapshot ) return JAVACALL_FAIL;

    *size = c->snapshot_len;
    return JAVACALL_OK;
}

static javacall_result fake_camera_get_video_snapshot_data( javacall_handle handle, 
                                                            char* buffer,
                                                            long size )
{
    fake_camera* c = (fake_camera*)handle;

    if( NULL == c->snapshot ) return JAVACALL_FAIL;
    if( NULL == buffer || size < (long)c->snapshot_len ) return JAVACALL_INVALID_ARGUMENT;
    memcpy( buffer, c->snapshot, c->snapshot_len );
    return JAVACALL_OK;
}

/*****************************************************************************\
                      I N T E R F A C E   T A B L E S
\*****************************************************************************/

static media_basic_interface _fake_camera_basic_itf =
{
    fake_camera_create,
    fake_camera_destroy,

    NULL, // get_format
    fake_camera_get_player_controls,

    fake_camera_stop,
    fake_camera_pause,
    fake_camera_run,

    NULL,
    NULL,
    NULL,
    NULL,

    fake_camera_get_time,
    fake_camera_set_time,
    fake_camera_get_duration,

    fake_camera_switch_to_foreground,
    fake_camera_switch_to_background
};

static media_video_interface _fake_camera_video_itf = {
    fake_camera_get_video_size,
    fake_camera_set_video_visible,
    fake_camera_set_video_location,
    fake_camera_set_video_alpha,
    fake_camera_set_video_fullscreenmode
};
    
static media_snapshot_interface _fake_camera_snapshot_itf = {
    fake_camera_start_video_snapshot,
    fake_camera_get_video_snapshot_data_size,
    fake_camera_get_video_snapshot_data
};

media_interface g_fake_camera_itf =
{
    &_fake_camera_basic_itf,
    NULL,
    &_fake_camera_video_itf,
    &_fake_camera_snapshot_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*****************************************************************************\
                  E X T R A   C O N T R O L S   S U P P O R T
\*****************************************************************************/

javacall_bool is_fake_camera( javacall_handle handle )
{
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;

    return ( &g_fake_camera_itf == pPlayer->mediaItfPtr ) ? JAVACALL_TRUE : JAVACALL_FALSE;
}
