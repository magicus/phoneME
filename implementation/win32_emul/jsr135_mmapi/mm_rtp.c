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
#include "pcm_out.h"

//=============================================================================

/*
static void rtp_dbg_out( LPCSTR str )
{
    FILE* f = fopen( "c:\\rtp.log", "ab" );
    if( NULL != f )
    {
        fputs( str, f );
        fclose( f );
    }
}

#define RTP_DBG( s ) rtp_dbg_out( (s) )
*/

#define RTP_DBG( s ) OutputDebugString( (s) )

//=============================================================================

#define XFER_BUFFER_SIZE  ( 8192 )
#define BUFFER_PACKETS    20  // number of packets necessary to stop buffering
#define MAX_MISSING       40  // number of missing packets necessary to consider EOM

typedef struct _xfer_buffer
{
    BYTE data[ XFER_BUFFER_SIZE ];
    struct _xfer_buffer* next;
} xfer_buffer;

typedef struct
{
    int                   appId;
    int                   playerId;
    jc_fmt                mediaType;
    javacall_utf16_string uri;

    int                   bits;
    int                   channels;
    int                   rate;

    pcm_handle_t          hpcm;
    CRITICAL_SECTION      cs;
    xfer_buffer*          queue_head;
    xfer_buffer*          queue_tail;
    int                   queue_size;
    xfer_buffer*          buf;

    BOOL                  realized;
    BOOL                  prefetched;
    BOOL                  acquired;
    BOOL                  playing;
    BOOL                  buffering;

    int                   missing;

    javacall_int64        samplesPlayed;

    long                  volume;
    javacall_bool         mute;
} rtp_player;

size_t rtp_pcm_callback( void* buf, size_t size, void* param )
{
    xfer_buffer* xbuf = NULL;
    rtp_player* p = (rtp_player*)param;
    long mt;

    EnterCriticalSection( &(p->cs) );
    {
        char str[ 80 ];
        sprintf( str, "   %i %i %i %i >> \n", size, p->queue_size, p->playing, p->buffering );
        RTP_DBG( str );

        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_NEED_MORE_MEDIA_DATA,
            p->appId, p->playerId, JAVACALL_OK, NULL);

        if( NULL != p->queue_head && p->playing && !p->buffering )
        {
            xbuf = p->queue_head;
            p->queue_head = xbuf->next;
            if( NULL == p->queue_head ) p->queue_tail = NULL;
            p->queue_size--;
        }

        if( p->playing )
        {
            if( 16 == p->bits )
            {
                p->samplesPlayed += size / p->channels / 2;
            }
            else // if( 8 == p=>bits )
            {
                p->samplesPlayed += size / p->channels;
            }

            mt = p->samplesPlayed * 1000 / p->rate;
        }
    }
    LeaveCriticalSection( &(p->cs) );

    if( NULL != xbuf )
    {
        if( JAVACALL_TRUE == p->mute || 0 == p->volume )
        {
            memset( buf, 0, size );
        }
        else
        {
            memcpy( buf, xbuf->data, size );
        }
        FREE( xbuf );
    }
    else
    {
        memset( buf, 0, size );

        if( p->playing )
        {
            if( !p->buffering )
            {
                p->buffering = TRUE;
                RTP_DBG( "  -------- buffering started --------\n" );
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_BUFFERING_STARTED,
                    p->appId, p->playerId, JAVACALL_OK, (void*)mt );
            }

            // TODO: this is a stub. need some other method to determine EOM
            if( ++p->missing >= MAX_MISSING ) 
            {
                RTP_DBG( "  -------- end of media      --------\n" );
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                    p->appId, p->playerId, JAVACALL_OK, (void*)mt );
                p->playing = FALSE;
            }
        }
    }

    return size;
}

static javacall_result rtp_create(int appId, 
                                  int playerId,
                                  jc_fmt mediaType,
                                  const javacall_utf16_string URI,
                                  javacall_handle *pHandle)
{
    rtp_player* p = (rtp_player*)MALLOC(sizeof(rtp_player));

    RTP_DBG( "*** create ***\n" );

    p->appId       = appId;
    p->playerId    = playerId;
    p->mediaType   = mediaType;
    p->uri         = NULL;
    p->queue_head  = NULL;
    p->queue_tail  = NULL;
    p->hpcm        = NULL;
    p->queue_size  = 0;
    p->realized    = FALSE;
    p->prefetched  = FALSE;
    p->acquired    = FALSE;
    p->playing     = FALSE;
    p->buffering   = TRUE;
    p->missing     = 0;
    p->samplesPlayed = 0;
    p->volume      = 100;
    p->mute        = JAVACALL_FALSE;

    InitializeCriticalSection( &(p->cs) );

    *pHandle = p;
    return JAVACALL_OK;
}

static javacall_result rtp_get_format(javacall_handle handle, jc_fmt* fmt)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** get format ***\n" );
    *fmt = p->mediaType;
    return JAVACALL_OK;
}

static javacall_result rtp_destroy(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    xfer_buffer* pb;
    xfer_buffer* t;

    RTP_DBG( "*** destroy ***\n" );

    EnterCriticalSection( &(p->cs ) );
    {
        pb = p->queue_head;
        while( NULL != pb ) 
        {
            t = pb;
            pb = pb->next;
            FREE( t );
        }
        p->queue_head = NULL;
        p->queue_tail = NULL;
    }
    LeaveCriticalSection( &(p->cs ) );

    FREE(p);

    return JAVACALL_OK;
}

static javacall_result rtp_close(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** close ***\n" );
    return JAVACALL_OK;
}

static javacall_result rtp_get_player_controls(javacall_handle handle,
                                               int* controls)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** get controls ***\n" );
    *controls = JAVACALL_MEDIA_CTRL_VOLUME;
    return JAVACALL_OK;
}

static javacall_result rtp_acquire_device(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** acquire device ***\n" );

    p->hpcm = pcm_out_open_channel( p->bits, p->channels, p->rate, 
                                    XFER_BUFFER_SIZE, rtp_pcm_callback, p );

    p->acquired = TRUE;

    return JAVACALL_OK;
}

static javacall_result rtp_release_device(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** release device ***\n" );

    if( NULL != p->hpcm )
    {
        pcm_out_close_channel( p->hpcm );
        p->hpcm = NULL;
    }

    p->acquired = FALSE;

    return JAVACALL_OK;
}

static javacall_result rtp_realize(javacall_handle handle, 
                                   javacall_const_utf16_string mime, 
                                   long mimeLength)
{
    rtp_player* p = (rtp_player*)handle;
    javacall_int32 cmp;

    RTP_DBG( "*** realize ***\n" );

    if( !wcsncmp( mime, L"audio/L16", wcslen( L"audio/L16" ) ) )
    {
        p->bits = 16;
        p->rate = 44100;
        p->channels = 1;

        p->mediaType = JC_FMT_RTP_L16;

        get_int_param( mime, L"channels", &(p->channels) );
        get_int_param( mime, L"rate", &(p->rate) );

        p->realized = TRUE;
    }
    else
    {
        p->mediaType = JC_FMT_UNSUPPORTED;
    }

    return JAVACALL_OK;
}

static javacall_result rtp_prefetch(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** prefetch ***\n" );
    p->prefetched = TRUE;
    return JAVACALL_OK;
}

static javacall_result rtp_get_java_buffer_size(javacall_handle handle,
                                                long* java_buffer_size,
                                                long* first_chunk_size)
{
    rtp_player* p = (rtp_player*)handle;

    *java_buffer_size = 16 * XFER_BUFFER_SIZE;

    if( p->prefetched )
    {
        RTP_DBG( "*** get_java_buffer_size: XFER_BUFFER_SIZE ***\n" );
        *first_chunk_size = XFER_BUFFER_SIZE;
    }
    else
    {
        RTP_DBG( "*** get_java_buffer_size: 0 ***\n" );
        *first_chunk_size = 0;
    }

    return JAVACALL_OK;
}

static javacall_result rtp_set_whole_content_size(javacall_handle handle,
                                                  long whole_content_size)
{
    rtp_player* p = (rtp_player*)handle;
    return JAVACALL_OK;
}

static javacall_result rtp_get_buffer_address(javacall_handle handle,
                                              const void** buffer,
                                              long* max_size)
{
    rtp_player* p = (rtp_player*)handle;

    p->buf = (xfer_buffer*)MALLOC( sizeof( xfer_buffer ) );
    p->buf->next = NULL;

    *buffer   = p->buf->data;
    *max_size = XFER_BUFFER_SIZE;

    return JAVACALL_OK;
}

static javacall_result rtp_do_buffering(javacall_handle handle, 
                                        const void* buffer,
                                        long *length, 
                                        javacall_bool *need_more_data, 
                                        long *next_chunk_size)
{
    int i;
    rtp_player* p = (rtp_player*)handle;
    BYTE t;
    BYTE* bb = (BYTE*)buffer;

    if( NULL != buffer && 0 != *length )
    {
        assert( p->buf->data == buffer );

        // data is in network byte order (i.e., big endian)
        for( i = 0; i < *length; i+= 2 )
        {
            t           = bb[ i ];
            bb[ i ]     = bb[ i + 1 ];
            bb[ i + 1 ] = t;
        }

        EnterCriticalSection( &(p->cs) );
        {
            char str[ 80 ];
            if( NULL == p->queue_tail ) // first packet
                p->queue_head = p->buf;
            else
                p->queue_tail->next = p->buf;

            p->queue_tail = p->buf;
            p->queue_size++;

            p->missing = 0;

            if( p->buffering && BUFFER_PACKETS == p->queue_size )
            {
                RTP_DBG( "  -------- buffering stopped --------\n" );
                p->buffering = FALSE;
                javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED,
                    p->appId, p->playerId, JAVACALL_OK, (void*)(p->samplesPlayed * 1000 / p->rate) );
            }
            sprintf( str, ">> %li %i %i %i\n", *length, p->queue_size, p->playing, p->buffering );
            RTP_DBG( str );
        }
        LeaveCriticalSection( &(p->cs) );

        p->buf = NULL;
    }

    if( p->acquired || p->buffering )
    {
        RTP_DBG( "        continue...\n" );
        *need_more_data  = JAVACALL_TRUE;
    }
    else
    {
        RTP_DBG( "        stop...\n" );
        *need_more_data  = JAVACALL_FALSE;
    }

    *next_chunk_size = XFER_BUFFER_SIZE;

    return JAVACALL_OK;
}

static javacall_result rtp_clear_buffer(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** clear buffer ***\n" );
    return JAVACALL_OK;
}

static javacall_result rtp_start(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** start ***\n" );
    p->playing = TRUE;
    return JAVACALL_OK;
}

static javacall_result rtp_stop(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** stop ***\n" );
    p->playing = FALSE;
    return JAVACALL_OK;
}

static javacall_result rtp_pause(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** pause ***\n" );
    p->playing = FALSE;
    return JAVACALL_OK;
}

static javacall_result rtp_resume(javacall_handle handle)
{
    rtp_player* p = (rtp_player*)handle;
    RTP_DBG( "*** resume ***\n" );
    p->playing = TRUE;
    return JAVACALL_OK;
}

static javacall_result rtp_get_time(javacall_handle handle, 
                                    long* ms)
{
    rtp_player* p = (rtp_player*)handle;

    EnterCriticalSection( &(p->cs) );
    *ms = p->samplesPlayed * 1000 / p->rate;
    LeaveCriticalSection( &(p->cs) );

    return JAVACALL_OK;
}

static javacall_result rtp_set_time(javacall_handle handle, 
                                    long* ms)
{
    rtp_player* p = (rtp_player*)handle;
    return JAVACALL_FAIL;
}

static javacall_result rtp_get_duration(javacall_handle handle, 
                                        long* ms)
{
    rtp_player* p = (rtp_player*)handle;
    return JAVACALL_NO_DATA_AVAILABLE;
}

static javacall_result rtp_switch_to_foreground(javacall_handle handle,
                                                int options)
{
    rtp_player* p = (rtp_player*)handle;
    return JAVACALL_OK;
}

static javacall_result rtp_switch_to_background(javacall_handle handle,
                                                int options)
{
    rtp_player* p = (rtp_player*)handle;
    return JAVACALL_OK;
}

/*****************************************************************************\
                         V O L U M E   C O N T R O L
\*****************************************************************************/

static javacall_result rtp_get_volume(javacall_handle handle, 
                                      long* level)
{
    rtp_player* p = (rtp_player*)handle;
    *level = p->volume;
    return JAVACALL_OK;
}

static javacall_result rtp_set_volume(javacall_handle handle, 
                                      long* level)
{
    rtp_player* p = (rtp_player*)handle;
    p->volume = *level;
    return JAVACALL_OK;
}

static javacall_result rtp_is_mute(javacall_handle handle, 
                                   javacall_bool* mute )
{
    rtp_player* p = (rtp_player*)handle;
    *mute = p->mute;
    return JAVACALL_OK;
}

static javacall_result rtp_set_mute(javacall_handle handle,
                                    javacall_bool mute)
{
    rtp_player* p = (rtp_player*)handle;
    p->mute = mute;
    return JAVACALL_OK;
}

/*****************************************************************************\
                        I N T E R F A C E   T A B L E S
\*****************************************************************************/

static media_basic_interface _rtp_basic_itf =
{
    rtp_create,
    rtp_get_format,
    rtp_get_player_controls,
    rtp_close,
    rtp_destroy,
    rtp_acquire_device,
    rtp_release_device,
    rtp_realize,
    rtp_prefetch,
    rtp_start,
    rtp_stop,
    rtp_pause,
    rtp_resume,
    rtp_get_java_buffer_size,
    rtp_set_whole_content_size,
    rtp_get_buffer_address,
    rtp_do_buffering,
    rtp_clear_buffer,
    rtp_get_time,
    rtp_set_time,
    rtp_get_duration,
    rtp_switch_to_foreground,
    rtp_switch_to_background
};

static media_volume_interface _rtp_volume_itf = {
    rtp_get_volume,
    rtp_set_volume,
    rtp_is_mute,
    rtp_set_mute
};

media_interface g_rtp_itf =
{
    &_rtp_basic_itf,
    &_rtp_volume_itf,
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