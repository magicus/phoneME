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

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
    va_list        args;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);

    OutputDebugStringA( str8 );
}

typedef struct _fake_camera
{
    int                   appId;
    int                   playerId;

    long                  video_width;
    long                  video_height;
    javacall_pixel*       video_frame;

    int                   out_width;
    int                   out_height;
    javacall_pixel*       out_frame;

    volatile BOOL         playing;
    BOOL                  visible;

    HANDLE                hThread;
    CRITICAL_SECTION      cs;
} fake_camera;

static void fake_camera_prepare_scaled_frame( fake_camera* c )
{
    int x, y, srcx, srcy;

    if( NULL == c->out_frame ) 
        c->out_frame = (javacall_pixel*)MALLOC( sizeof(javacall_pixel) * c->out_width * c->out_height );

    if( c->out_width == c->video_width && c->out_height == c->video_height )
    {
        memcpy( c->out_frame, c->video_frame, 
                sizeof( javacall_pixel ) * c->video_width * c->video_height );
    }
    else
    {
        double kx = (double)(c->video_width) / (double)(c->out_width);
        double ky = (double)(c->video_height) / (double)(c->out_height);

        for( y = 0; y < c->out_height; y++ )
        {
            for( x = 0; x < c->out_width; x++ )
            {
                srcx = (int)( 0.5 + kx * x );
                srcy = (int)( 0.5 + ky * y );
                *( c->out_frame + y * c->out_width + x )
                    = *( c->video_frame + srcy * c->video_width + srcx );
            }
        }
    }
}

static void fake_camera_frame_ready( fake_camera* c )
{
    EnterCriticalSection( &(c->cs) );

    if( c->visible )
    {
        fake_camera_prepare_scaled_frame( c );
        lcd_output_video_frame( c->out_frame );
    }

    LeaveCriticalSection( &(c->cs) );
}

static void fake_camera_generator_thread( void* param )
{
    int x, y;
    fake_camera* c = (fake_camera*)param;

    while( c->playing )
    {
        for( x = 0; x < c->video_width; x++ )
        {
            for( y = 0; y < c->video_height; y++ )
            {
                *( c->video_frame + y * c->video_width + x )
                    = RGB2PIXELTYPE( rand() & 0xFF, 
                                     rand() & 0xFF, 
                                     rand() & 0xFF );
            }
        }

        fake_camera_frame_ready( c );

        Sleep( 50 );
    }
}

//=============================================================================

static javacall_result fake_camera_create(int appId, 
    int playerId,
    jc_fmt mediaType,
    const javacall_utf16_string URI, 
    javacall_handle* pHandle)
{
    fake_camera* c = (fake_camera*)MALLOC( sizeof(fake_camera) );

    c->appId       = appId;
    c->playerId    = playerId;

    c->out_width   = c->video_width  = 160;
    c->out_height  = c->video_height = 120;

    c->video_frame = (javacall_pixel*)MALLOC( sizeof(javacall_pixel) 
                                              * c->video_width 
                                              * c->video_height );
    c->out_frame   = NULL;

    c->playing     = FALSE;
    c->visible     = FALSE;

    c->hThread     = NULL;

    InitializeCriticalSection( &(c->cs) );

    *pHandle = (javacall_handle)c;

    return JAVACALL_OK;
}

static javacall_result fake_camera_get_format(javacall_handle handle, jc_fmt* fmt)
{
    PRINTF( "*** get_format ***\n" );
    return JAVACALL_OK;
}

static javacall_result fake_camera_destroy(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** destroy ***\n" );

    if( NULL != c->video_frame ) FREE( c->video_frame );

    if( NULL != c->out_frame )
    {
        lcd_output_video_frame( NULL );
        FREE( c->out_frame );
    }

    DeleteCriticalSection( &(c->cs) );
    FREE( c );

    return JAVACALL_OK;
}

static javacall_result fake_camera_close(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** close ***\n" );
    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_CLOSE_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );
    return JAVACALL_OK;
}

static javacall_result fake_camera_get_player_controls(javacall_handle handle,
    int* controls)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** get controls ***\n" );
    *controls = JAVACALL_MEDIA_CTRL_VIDEO;
    return JAVACALL_OK;
}

static javacall_result fake_camera_deallocate(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** deallocate ***\n" );
    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DEALLOCATE_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );
    return JAVACALL_OK;
}

static javacall_result fake_camera_realize(javacall_handle handle, 
    javacall_const_utf16_string mime, 
    long mimeLength)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** realize('%S') ***\n", mime );
    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_REALIZE_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );
    return JAVACALL_OK;
}

static javacall_result fake_camera_prefetch(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** prefetch ***" );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_PREFETCH_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result fake_camera_start(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** start ***" );

    c->playing = TRUE;
    c->hThread = (HANDLE)_beginthread( fake_camera_generator_thread, 0, c );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_START_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result fake_camera_stop(javacall_handle handle)
{
    fake_camera* c = (fake_camera*)handle;
    PRINTF( "*** stop***\n" );

    c->playing = FALSE;
    WaitForSingleObject( c->hThread, INFINITE );
    c->hThread = NULL;

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_STOP_FINISHED,
                                     c->appId,
                                     c->playerId, 
                                     JAVACALL_OK, 
                                     NULL );
    return JAVACALL_OK;
}

static javacall_result fake_camera_get_time(javacall_handle handle, long* ms)
{
    fake_camera* ñ = (fake_camera*)handle;
    *ms = -1;
    PRINTF( "--- get_time: %ld",*ms );
    return JAVACALL_OK;
}

static javacall_result fake_camera_set_time(javacall_handle handle, long ms)
{
    return JAVACALL_FAIL;
}

static javacall_result fake_camera_get_duration(javacall_handle handle, long* ms)
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
    fake_camera* ñ = (fake_camera*)handle;

    *width  = ñ->video_width;
    *height = ñ->video_height;

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_visible(javacall_handle handle, javacall_bool visible)
{
    fake_camera* ñ = (fake_camera*)handle;

    EnterCriticalSection( &(ñ->cs) );
    ñ->visible = ( JAVACALL_TRUE == visible );
    if( visible )
    {
        if( NULL != ñ->out_frame ) lcd_output_video_frame( ñ->out_frame );
    }
    else
    {
        lcd_output_video_frame( NULL );
    }
    LeaveCriticalSection( &(ñ->cs) );

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    fake_camera* ñ = (fake_camera*)handle;
    javacall_pixel* old_frame;

    EnterCriticalSection( &(ñ->cs) );

    if( ñ->out_width != w || ñ->out_height != h )
    {
        ñ->out_width  = w;
        ñ->out_height = h;

        old_frame = ñ->out_frame;
        ñ->out_frame = NULL;

        if( NULL != ñ->video_frame ) fake_camera_prepare_scaled_frame( ñ );

        lcd_set_video_rect( x, y, w, h );
        lcd_output_video_frame( ñ->out_frame );

        if( NULL != old_frame ) FREE( old_frame );
    }
    else
    {
        lcd_set_video_rect( x, y, w, h );
    }

    LeaveCriticalSection( &(ñ->cs) );

    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_alpha(javacall_handle handle, javacall_bool on, javacall_pixel color)
{
    lcd_set_color_key( on, color );
    return JAVACALL_OK;
}

static javacall_result fake_camera_set_video_fullscreenmode(javacall_handle handle, javacall_bool fullScreenMode)
{
    return JAVACALL_FAIL;
}

/*****************************************************************************\
                      I N T E R F A C E   T A B L E S
\*****************************************************************************/

static media_basic_interface _fake_camera_basic_itf =
{
    fake_camera_create,
    fake_camera_get_format,
    fake_camera_get_player_controls,
    fake_camera_close,
    fake_camera_destroy,
    fake_camera_deallocate,
    fake_camera_realize,
    fake_camera_prefetch,
    fake_camera_start,
    fake_camera_stop,
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

media_interface g_fake_camera_itf =
{
    &_fake_camera_basic_itf,
    NULL,
    &_fake_camera_video_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //&_fake_camera_rate_itf
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
