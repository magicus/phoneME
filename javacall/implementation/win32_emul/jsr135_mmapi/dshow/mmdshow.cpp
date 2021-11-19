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

#include "../multimedia.h"
#include "player_dshow.hpp"

#include "mm_qsound_audio.h"
#include "mmdshow.h"

//=============================================================================

extern "C" {
    // following functions are defined in lcd.c (MIDP-related javacall):
    int  lcd_open_overlay();
    void lcd_close_overlay( int ovl_n );
    void lcd_set_color_key( int ovl_n, javacall_bool use_keying, javacall_pixel key_color );
    void lcd_set_video_rect( int ovl_n, int x, int y, int w, int h );
    void lcd_output_video_frame( int ovl_n, javacall_pixel* video );

    extern globalMan g_QSoundGM[];
}

#define QSOUND_GET_GM( n ) ( g_QSoundGM[ (n) ] )

// ===========================================================================

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
    va_list        args;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);

    //OutputDebugString( str8 );
}

#define MAX_DSHOW_PLAYERS   1024
#define XFER_BUFFER_SIZE    0x1000   // bytes 
#define OUT_QUEUE_SIZE      0x10000  // bytes

class dshow_player : public player_callback, 
                     public IWaveStream
{
    // player_callback methods:
    virtual void        frame_ready( bits16 const* pFrame );
    virtual void        sample_ready(nat32 nbytes, void const* pdata);
    virtual void        size_changed( int16 w, int16 h );
    virtual void        audio_format_changed(nat32 samples_per_second, nat32 channels, nat32 bits_per_sample);
    virtual void        playback_finished();

    // IWaveStream methods:
	virtual long        getFormat(int* pChannels, long* pSampleRate);
	virtual long        read(short* buffer, int samples);

public:
    long                  get_media_time();
    void                  prepare_scaled_frame();

    int                   appId;
    int                   playerId;
    int                   gmIdx;
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

    int                   out_width;
    int                   out_height;
    javacall_pixel*       out_frame;

    int                   ovl;

    BYTE                  buf[ XFER_BUFFER_SIZE ];

    bool                  realizing;
    bool                  prefetching;
    bool                  prefetched;
    bool                  playing;

    long                  bytes_buffered;
    bool                  all_data_arrived;
    long                  whole_content_size;
    bool                  eom_sent;

    long                  volume;
    int                   pan;
    bool                  muted;
    bool                  visible;

    player*               ppl;
    IModule*              pModule;
    bool                  our_module;

    static dshow_player*  players[ MAX_DSHOW_PLAYERS ];
    static int            num_players;

    CRITICAL_SECTION      cs;
    BYTE                  out_queue[ OUT_QUEUE_SIZE ];
    size_t                out_queue_w; // write index
    size_t                out_queue_r; // read  index
    size_t                out_queue_n; // samples in queue
};

dshow_player* dshow_player::players[ MAX_DSHOW_PLAYERS ];
int           dshow_player::num_players = 0;

void dshow_player::prepare_scaled_frame()
{
    if( NULL == out_frame ) out_frame = new javacall_pixel[ out_width * out_height ];

    if( out_width == video_width && out_height == video_height )
    {
        memcpy( out_frame, video_frame, 
                sizeof( javacall_pixel ) * video_width * video_height );
    }
    else
    {
        double kx = double(video_width) / double(out_width);
        double ky = double(video_height) / double(out_height);

        for( int y = 0; y < out_height; y++ )
        {
            for( int x = 0; x < out_width; x++ )
            {
                int srcx = int( 0.5 + kx * x );
                int srcy = int( 0.5 + ky * y );
                *( out_frame + y * out_width + x )
                    = *( video_frame + srcy * video_width + srcx );
            }
        }
    }
}

void dshow_player::frame_ready( bits16 const* pFrame )
{
    EnterCriticalSection( &cs );

    if( NULL == video_frame ) video_frame = new javacall_pixel[ video_width * video_height ];

    for( int y = 0; y < video_height; y++ )
    {
        memcpy( video_frame + y * video_width, 
                pFrame + ( video_height - y - 1 ) * video_width,
                sizeof( javacall_pixel ) * video_width );
    }

    if( visible )
    {
        prepare_scaled_frame();
        lcd_output_video_frame( ovl, out_frame );
    }

    LeaveCriticalSection( &cs );
}

void dshow_player::size_changed( int16 w, int16 h )
{
    video_width  = w;
    video_height = h;

    if( -1 == out_width ) out_width = video_width;
    if( -1 == out_height ) out_height = video_height;
}

void dshow_player::audio_format_changed(nat32 samples_per_second, 
                                        nat32 _channels, nat32 bits_per_sample)
{
    channels = _channels;
    rate     = samples_per_second;
}

void dshow_player::playback_finished()
{
    if( !eom_sent )
    {
        eom_sent = true;

        long t = get_media_time();
        PRINTF( "*** playback finished, t=%ld\n", t );
        javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
                                          appId, playerId, JAVACALL_OK, (void*)t );

        PRINTF( "*** stopping...\n" );
        player::result r = ppl->stop();
        PRINTF( "*** player::stop = %i\n", r );
    }
}

void dshow_player::sample_ready(nat32 nbytes, void const* pdata)
{
    EnterCriticalSection( &cs );

    if( out_queue_n + nbytes > OUT_QUEUE_SIZE )
    {
        PRINTF( "### overflow ###\n" );
        nbytes = OUT_QUEUE_SIZE - out_queue_n;
    }

    if( 0 != nbytes )
    {
        size_t n1 = nbytes;
        size_t n2 = 0;

        if( out_queue_w + n1 > OUT_QUEUE_SIZE ) {
            n1 = OUT_QUEUE_SIZE - out_queue_w;
            n2 = nbytes - n1;
        }

        memcpy( out_queue + out_queue_w, pdata, n1 );

        if( 0 != n2 ) {
            memcpy( out_queue, (BYTE*)pdata + n1, n2 );
            out_queue_w = n2;
        } else {
            out_queue_w += n1;
        }

        out_queue_n += nbytes;
    }

    LeaveCriticalSection( &cs );
}

long dshow_player::getFormat(int* pChannels, long* pSampleRate)
{
    *pChannels   = channels;
    *pSampleRate = rate;
    return MQ234_ERROR_NO_ERROR;
}

long dshow_player::read(short* buffer, int samples)
{
    /*
    static double t  = 0;
    static DWORD  k  = 0;
    static double dt = 1.0 / 44100.0;
    static double f0  = 440.0;
    static double f1  = 440.0 * 1.5;

    for( int i = 0; i < samples; i ++ )
    {
        //buffer[ 2*i     ] = (k&0x00003000) ? short( 5000.0 * sin( t * 6.28 * f0 ) ) : 0;
        //buffer[ 2*i + 1 ] = (k&0x00006000) ? short( 5000.0 * sin( t * 6.28 * f1 ) ) : 0;
        buffer[ i ] = (k&0x00003000) ? short( 5000.0 * sin( t * 6.28 * f0 ) ) : 0;
        t += dt;
        k++;
    }
    */


    EnterCriticalSection( &cs );

    size_t zero_padding_size = 0;
    size_t nbytes = samples * 2 * channels;
    if( nbytes > out_queue_n ) {
        zero_padding_size = nbytes - out_queue_n;
        nbytes = out_queue_n;
        PRINTF( "### underflow ###\n" );
    }

    BYTE*  out = (BYTE*)buffer;
    size_t n1  = nbytes;
    size_t n2  = 0;

    if( out_queue_r + n1 > OUT_QUEUE_SIZE ) {
        n1 = OUT_QUEUE_SIZE - out_queue_r;
        n2 = nbytes - n1;
    }

    memcpy( out, out_queue + out_queue_r, n1 );

    if( 0 != n2 ) {
        memcpy( out + n1, out_queue, n2 );
        out_queue_r = n2;
    } else {
        out_queue_r += n1;
    }

    out_queue_n -= nbytes;

    if( 0 != zero_padding_size ) {
        memset( out + nbytes, 0, zero_padding_size );
    }

    LeaveCriticalSection( &cs );

    return MQ234_ERROR_NO_ERROR;
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

        if( long( time ) > media_time ) media_time = long( time );

        return media_time;
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

    PRINTF( "\n\n*** create('%S') ***\n", URI );

    if( dshow_player::num_players >= MAX_DSHOW_PLAYERS )
        return JAVACALL_OUT_OF_MEMORY;

    p->gmIdx = -1;
    javacall_result res = isolateIDtoGM( appId, &(p->gmIdx) );
    
    if( JAVACALL_OK != res )
    {
        gmDetach( p->gmIdx );
        *pHandle = ( javacall_handle )NULL;
        return JAVACALL_FAIL;
    }

    p->appId            = appId;
    p->playerId         = playerId;
    p->mediaType        = mediaType;
    p->uri              = NULL;
    p->is_video         = ( JC_FMT_FLV == mediaType || JC_FMT_VIDEO_3GPP == mediaType || JC_FMT_MPEG_4_SVP == mediaType || JC_FMT_MPEG_1 == mediaType );

    if( p->is_video )
    {
        p->ovl = lcd_open_overlay();
        if( -1 == p->ovl )
        {
            delete p;
            return JAVACALL_FAIL;
        }
    }
    else
    {
        p->ovl = -1;
    }

    p->realizing        = false;
    p->prefetching      = false;
    p->prefetched       = false;
    p->playing          = false;

    p->all_data_arrived = false;
    p->whole_content_size = -1;
    p->bytes_buffered   = 0;

    p->media_time       = 0;
    p->volume           = 100;
    p->muted            = false;
    p->visible          = false;

    p->video_width      = 0;
    p->video_height     = 0;
    p->video_frame      = NULL;

    p->out_width        = -1;
    p->out_height       = -1;
    p->out_frame        = NULL;

    p->ppl              = NULL;
    p->pModule          = NULL;
    p->our_module       = true;

    InitializeCriticalSection( &(p->cs) );
    p->out_queue_w      = 0;
    p->out_queue_r      = 0;
    p->out_queue_n      = 0;

    *pHandle =(javacall_handle)p;

    dshow_player::players[ dshow_player::num_players++ ] = p;

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

    if( p->is_video )
    {
        lcd_output_video_frame( p->ovl, NULL );
        lcd_close_overlay( p->ovl );
    }

    if( NULL != p->pModule )
    {
        p->pModule->removePlayer( p );
        //if( p->our_module ) p->pModule->destroy();
        p->pModule = NULL;
    }

    gmDetach( p->gmIdx );

    if( NULL != p->video_frame ) delete p->video_frame;
    if( NULL != p->out_frame ) delete p->out_frame;
    if( NULL != p->ppl ) delete p->ppl;

    DeleteCriticalSection( &(p->cs) );

    delete p;

    bool found = false;
    for( int i = 0; i < dshow_player::num_players-1; i++ )
    {
        if( dshow_player::players[i] == p ) found = true;
        if( found ) dshow_player::players[i] = dshow_player::players[i+1];
    }
    dshow_player::num_players--;

    return JAVACALL_OK;
}

static javacall_result dshow_close(javacall_handle handle)
{
    //dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** close ***\n" );

    /*
    if( NULL != p->pModule )
    {
        p->pModule->removePlayer( p );
        //if( p->our_module ) p->pModule->destroy();
        p->pModule = NULL;
    }

    if( NULL != p->ppl ) p->ppl->close();

    if( p->is_video ) lcd_output_video_frame( p->ovl, NULL );
    */

    return dshow_destroy(handle);
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

    if( NULL == p->pModule )
    {
        IGlobalManager* gm = QSOUND_GET_GM(p->gmIdx).gm;
        p->pModule = gm->createEffectModule();
        p->our_module = true;

        IPanControl* pan = (IPanControl*)( p->pModule->getControl( "PanControl" ) );
        if( NULL != pan ) pan->setPan( p->pan );

        IVolumeControl* vc = (IVolumeControl*)( p->pModule->getControl( "VolumeControl" ) );
        if( NULL != vc ) vc->SetLevel( p->volume );
    }

    p->pModule->addPlayer( p );

    p->prefetched = true;

    return JAVACALL_OK;
}

static javacall_result dshow_release_device(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** release device ***\n" );

    if( p->is_video ) lcd_output_video_frame( p->ovl, NULL );

    if( NULL != p->pModule )
    {
        p->pModule->removePlayer( p );
        //if( p->our_module ) p->pModule->destroy();
        p->pModule = NULL;
    }

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

    PRINTF( "*** realize('%S') ***\n", mime );

    p->duration = -1;

    if( NULL == mime || mime_equal( mime, mimeLength, L"text/plain" ) )
    {
        switch( p->mediaType )
        {
        case JC_FMT_FLV:          mime = (javacall_const_utf16_string)L"video/x-flv"; break;
        case JC_FMT_MPEG1_LAYER3: mime = (javacall_const_utf16_string)L"audio/mpeg";  break;
        case JC_FMT_VIDEO_3GPP:   mime = (javacall_const_utf16_string)L"video/3gpp";  break;
        case JC_FMT_MPEG_4_SVP:   mime = (javacall_const_utf16_string)L"video/mp4";  break;
        case JC_FMT_MPEG_1:   mime = (javacall_const_utf16_string)L"video/mpeg";  break;
        case JC_FMT_AMR:          
        case JC_FMT_AMR_WB:          
        case JC_FMT_AMR_WB_PLUS:          
                                  mime = (javacall_const_utf16_string)L"audio/amr";  break;
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
        //get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        //get_int_param( mime, (javacall_const_utf16_string)L"rate",     &(p->rate)     );
        get_int_param( mime, (javacall_const_utf16_string)L"duration", &(p->duration) );

        p->mediaType = JC_FMT_MPEG1_LAYER3;
        mime = (javacall_const_utf16_string)L"audio/mpeg";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"video/x-vp6" ) ||
             mime_equal( mime, mimeLength, L"video/x-flv" ) ||
             mime_equal( mime, mimeLength, L"video/x-javafx" ) )
    {
        p->mediaType = JC_FMT_FLV;
        mime = (javacall_const_utf16_string)L"video/x-flv";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"video/3gpp" ) )
    {
        p->mediaType = JC_FMT_VIDEO_3GPP;
        mime = (javacall_const_utf16_string)L"video/3gpp";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"video/mp4" ) )
    {
        p->mediaType = JC_FMT_MPEG_4_SVP;
        mime = (javacall_const_utf16_string)L"video/mp4";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"video/mpeg" ) )
    {
        p->mediaType = JC_FMT_MPEG_1;
        mime = (javacall_const_utf16_string)L"video/mpeg";
        mimeLength = wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mimeLength, L"audio/amr" ) )
    {
        p->mediaType = JC_FMT_AMR;
        mime = (javacall_const_utf16_string)L"audio/amr";
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
    p->whole_content_size = whole_content_size;
    p->ppl->data( nat32(whole_content_size), NULL );
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
            long preload_size = XFER_BUFFER_SIZE * 50;

            if( JC_FMT_FLV == p->mediaType ) preload_size = 400 * 1024;

            if( -1 != p->whole_content_size )
            {
                preload_size = p->whole_content_size;
            }

            *need_more_data  = ( p->bytes_buffered < preload_size ) 
                               ? JAVACALL_TRUE : JAVACALL_FALSE;

            if( JAVACALL_FALSE == *need_more_data )
            {
                PRINTF( "*** calling player::realize()***\n" );
                if( player::result_success != p->ppl->realize() )
                {
                    PRINTF( "*** player::realize() failed! ***\n" );
                    return JAVACALL_FAIL;
                }
                PRINTF( "*** player::realize() finished***\n" );

                PRINTF( "*** calling player::prefetch()***\n" );
                if( player::result_success != p->ppl->prefetch() )
                {
                    PRINTF( "*** player::prefetch() failed! ***\n" );
                    return JAVACALL_FAIL;
                }
                PRINTF( "*** player::prefetch() finished***\n" );
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
        //p->ppl->data( 0, NULL );

        *need_more_data  = JAVACALL_FALSE;
        *next_chunk_size = 0;
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
    player::result r;
    PRINTF( "*** start, mt = %ld/%ld***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );
    p->eom_sent = false;
    if( player::result_success == p->ppl->start() )
    {
        PRINTF( "*** started, mt = %ld/%ld ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );
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
    player::result r;
    PRINTF( "*** stop, mt=%ld/%ld... ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );
    p->get_media_time();
    if( player::result_success == p->ppl->stop() )
    {
        p->playing = false;
        PRINTF( "*** ...stopped, mt=%ld/%ld ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );
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
    PRINTF( "--- get_time: %ld",*ms );
    // if( p->duration != -1 && *ms > p->duration ) *ms = p->duration;
    return JAVACALL_OK;
}

static javacall_result dshow_set_time(javacall_handle handle, long* ms)
{
    dshow_player* p = (dshow_player*)handle;
    player::result r;

    int64 mt = int64( 1000 ) * int64( *ms );

    p->media_time = long( p->ppl->set_media_time( mt, &r ) / 1000 );

    PRINTF( "--- set_time(%ld): %ld", *ms, p->media_time );

    *ms = p->media_time;

    return (player::result_success == r) ? JAVACALL_OK : JAVACALL_FAIL;
}

static javacall_result dshow_get_duration(javacall_handle handle, long* ms)
{
    dshow_player* p = (dshow_player*)handle;

    player::result res;

    int64 d = p->ppl->get_duration( &res );

    PRINTF( "----------- get_duration: %i, %ld, (%i)\n", res, long( d / 1000 ), p->duration );

    if( res == player::result_success && player::time_unknown != d ) {
        *ms = long( d / 1000 );
    } else {
        *ms = p->duration;
    }

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
    if( NULL != p->pModule )
    {
        IVolumeControl* vc = (IVolumeControl*)( p->pModule->getControl("VolumeControl") );
        if( NULL != vc ) *level = vc->SetLevel( *level );
    }
    p->volume = *level;
    return JAVACALL_OK;
}

static javacall_result dshow_is_mute(javacall_handle handle, 
    javacall_bool* mute )
{
    dshow_player* p = (dshow_player*)handle;
    *mute = ( p->muted ? JAVACALL_TRUE : JAVACALL_FALSE );
    return JAVACALL_OK;
}

static javacall_result dshow_set_mute(javacall_handle handle,
    javacall_bool mute)
{
    dshow_player* p = (dshow_player*)handle;
    if( NULL != p->pModule )
    {
        IVolumeControl* vc = (IVolumeControl*)( p->pModule->getControl("VolumeControl") );
        if( NULL != vc ) vc->SetMute( mute );
    }
    p->muted = ( JAVACALL_TRUE == mute );
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
    dshow_player* p = (dshow_player*)handle;

    EnterCriticalSection( &(p->cs) );
    p->visible = ( JAVACALL_TRUE == visible );
    if( visible )
    {
        if( NULL != p->out_frame ) lcd_output_video_frame( p->ovl, p->out_frame );
    }
    else
    {
        lcd_output_video_frame( p->ovl, NULL );
    }
    LeaveCriticalSection( &(p->cs) );

    return JAVACALL_OK;
}

static javacall_result dshow_set_video_location(javacall_handle handle, long x, long y, long w, long h)
{
    dshow_player* p = (dshow_player*)handle;

    EnterCriticalSection( &(p->cs) );

    if( p->out_width != w || p->out_height != h )
    {
        p->out_width  = w;
        p->out_height = h;

        javacall_pixel* old_frame = p->out_frame;
        p->out_frame = NULL;

        if( NULL != p->video_frame ) p->prepare_scaled_frame();

        lcd_set_video_rect( p->ovl, x, y, w, h );
        lcd_output_video_frame( p->ovl, p->out_frame );

        if( NULL != old_frame ) delete old_frame;
    }
    else
    {
        lcd_set_video_rect( p->ovl, x, y, w, h );
    }

    LeaveCriticalSection( &(p->cs) );

    return JAVACALL_OK;
}

static javacall_result dshow_set_video_alpha(javacall_handle handle, javacall_bool on, javacall_pixel color)
{
    dshow_player* p = (dshow_player*)handle;
    lcd_set_color_key( p->ovl, on, color );
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

/*****************************************************************************\
                          A M M S   S U P P O R T
\*****************************************************************************/

javacall_bool is_dshow_player( javacall_handle handle )
{
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;

    return ( &g_dshow_itf == pPlayer->mediaItfPtr ) ? JAVACALL_TRUE : JAVACALL_FALSE;
}

javacall_result dshow_add_player_to_ss3d( javacall_handle handle, ISoundSource3D* ss3d )
{
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;
    dshow_player*          p = (dshow_player*)pPlayer->mediaHandle;

    if( NULL != p->pModule )
    {
        p->pModule->removePlayer( p );
        //if( p->our_module ) p->pModule->destroy();
        p->pModule = NULL;
    }

    MQ234_ERROR e = ss3d->addPlayer( p );

    if( MQ234_ERROR_NO_ERROR == e )
    {
        p->pModule = ss3d;
        p->our_module = false;
    }

    return ( MQ234_ERROR_NO_ERROR == e ) ? JAVACALL_OK : JAVACALL_FAIL;
}

javacall_result dshow_remove_player_from_ss3d( javacall_handle handle, ISoundSource3D* ss3d )
{
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;
    dshow_player*          p = (dshow_player*)pPlayer->mediaHandle;

    MQ234_ERROR e = ss3d->removePlayer( p );

    if( MQ234_ERROR_NO_ERROR == e )
    {
        p->pModule = NULL;
    }

    return ( MQ234_ERROR_NO_ERROR == e ) ? JAVACALL_OK : JAVACALL_FAIL;
}

void dshow_notify_ss3d_going_down( ISoundSource3D* ss3d )
{
    for( int i = 0; i < dshow_player::num_players; i++ )
    {
        if( dshow_player::players[i]->pModule == ss3d )
        {
            dshow_player::players[i]->pModule->removePlayer( dshow_player::players[i] );
            dshow_player::players[i]->pModule = NULL;
        }
    }
}

javacall_result dshow_set_pan( javacall_handle handle, int* pPan )
{
    javacall_impl_player*  pPlayer = (javacall_impl_player*)handle;
    dshow_player*          p = (dshow_player*)pPlayer->mediaHandle;

    p->pan = *pPan;

    if( NULL != p->pModule )
    {
        IPanControl* pan = (IPanControl*)( p->pModule->getControl( "PanControl" ) );

        if( NULL != pan )
        {
            *pPan = pan->setPan( *pPan );
            return JAVACALL_OK;
        }
        else
        {
            return JAVACALL_FAIL;
        }
    }
    else
    {
        return JAVACALL_OK;
    }
}

//=============================================================================

} // extern "C"

//=============================================================================
