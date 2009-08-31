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

#include "../multimedia.h"
#include "player_dshow.hpp"

#include "mm_qsound_audio.h"
#include "mmdshow.h"

//=============================================================================

extern "C" {
    // following functions are defined in lcd.c (MIDP-related javacall):
    void lcd_set_color_key( javacall_bool use_keying, javacall_pixel key_color );
    void lcd_set_video_rect( int x, int y, int w, int h );
    void lcd_output_video_frame( javacall_pixel* video );

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

    OutputDebugStringA( str8 );
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
    virtual result      data(int64 offset, int32 len, nat8 *pdata, int32 *plen);
    virtual result      get_stream_length(int64 *plength);

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

    javacall_const_utf16_string mime;
    long                        mimeLength;

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

    javacall_uint8*       snapshot;
    javacall_uint32       snapshot_len;

    bool                  playing;

    long                  bytes_buffered;
    bool                  all_data_arrived;
    javacall_int64        whole_content_size;
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

    HANDLE                dwr_event;
    int64                 dwr_offset;
    int32                 dwr_len;
    BYTE*                 dwr_pdata;
    bool                  dwr_cancel;

    long                  target_mt;
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
        lcd_output_video_frame( out_frame );
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
                                        nat32 _channels, nat32 /*bits_per_sample*/)
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
                                          appId, playerId, JAVACALL_OK, (void*)(size_t)t );

        PRINTF( "*** stopping...\n" );
        player::result r = ppl->stop();
        PRINTF( "*** player::stop = %i\n", r );
    }
}

player_callback::result dshow_player::data(int64 offset, int32 len, nat8 *pdata, int32 *plen)
{
    PRINTF( "*** data(@%I64d l=%i) ***\n",offset,len );

    dwr_offset    = offset;
    dwr_len       = len;
    dwr_pdata     = (BYTE*)pdata;

    if( dwr_cancel )
    {
        PRINTF( "*** data -- refusing ***\n" );
        return player_callback::result_io;
    }
        
    javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_DATA_REQUEST,
                                      appId,
                                      playerId, 
                                      JAVACALL_OK,
                                      NULL );

    if( WAIT_OBJECT_0 != WaitForSingleObject( dwr_event, 30000 ) )
    {
        PRINTF( "*** ERROR: timeout waiting for data (@%I64d l=%i) ***\n", offset, len );
        return player_callback::result_io;
    }

    if( dwr_cancel )
    {
        PRINTF( "*** data -- aborted ***\n" );
        return player_callback::result_io;
    }
    else
    {
        *plen = dwr_len;
        return player_callback::result_success;
    }
}

player_callback::result dshow_player::get_stream_length(int64 * /*plength*/)
{
    return player_callback::result_io;
}

void dshow_player::sample_ready(nat32 nbytes, void const* pdata)
{
    EnterCriticalSection( &cs );

    if( out_queue_n + nbytes > OUT_QUEUE_SIZE )
    {
        PRINTF( "### overflow ###\n" );
        nbytes = (nat32)(OUT_QUEUE_SIZE - out_queue_n);
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
        //PRINTF( "### underflow ###\n" );
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

static void add_to_qsound( dshow_player* p )
{
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
}

static void remove_from_qsound( dshow_player* p )
{
    if( NULL != p->pModule )
    {
        p->pModule->removePlayer( p );
        if( p->our_module ) 
        {
            //Sleep( 500 );
            //p->pModule->destroy();
        }
        p->pModule = NULL;
    }

    gmDetach( p->gmIdx );
}

//=============================================================================

static bool mime_equal( javacall_const_utf16_string mime, long mimeLength, const wchar_t* v )
{
    return !wcsncmp( (const wchar_t*)mime, v, min( (size_t)mimeLength, wcslen( v ) ) );
}

static javacall_result dshow_create(javacall_impl_player* outer_player)
{
    dshow_player* p = new dshow_player;

    PRINTF( "\n\n*** create('%S','%S') ***\n", 
        ( NULL == outer_player->uri ) ? L"" : (const wchar_t*)outer_player->uri, 
        ( NULL == outer_player->mime ) ? L"" : (const wchar_t*)outer_player->mime );

    javacall_const_utf16_string mime     = outer_player->mime;
    int                         mime_len = ( NULL == mime ) ? 0 : (long)wcslen( (const wchar_t*)mime );

    if( dshow_player::num_players >= MAX_DSHOW_PLAYERS )
        return JAVACALL_OUT_OF_MEMORY;

    p->gmIdx = -1;
    javacall_result res = appIDtoGM( outer_player->appId, &(p->gmIdx) );
    
    if( JAVACALL_OK != res )
    {
        gmDetach( p->gmIdx );
        return JAVACALL_NO_AUDIO_DEVICE;
    }

    p->duration         = -1;

    p->appId            = outer_player->appId;
    p->playerId         = outer_player->playerId;
    p->mediaType        = fmt_str2enum( outer_player->mediaType );

    p->playing          = false;

    p->all_data_arrived = false;
    p->whole_content_size = outer_player->streamLen;
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

    p->snapshot         = NULL;
    p->snapshot_len     = NULL;

    p->ppl              = NULL;
    p->pModule          = NULL;
    p->our_module       = true;

    InitializeCriticalSection( &(p->cs) );
    p->out_queue_w      = 0;
    p->out_queue_r      = 0;
    p->out_queue_n      = 0;

    p->dwr_event        = CreateEvent( NULL, FALSE, FALSE, NULL );
    p->dwr_cancel       = false;

    outer_player->mediaHandle = (javacall_handle)p;

    if( NULL == mime || mime_equal( mime, mime_len, L"text/plain" ) )
    {
        switch( p->mediaType )
        {
        case JC_FMT_FLV:          mime = (javacall_const_utf16_string)L"video/x-flv"; break;
        case JC_FMT_MPEG1_LAYER3: mime = (javacall_const_utf16_string)L"audio/mpeg";  break;
        case JC_FMT_VIDEO_3GPP:   mime = (javacall_const_utf16_string)L"video/3gpp";  break;
        case JC_FMT_MPEG_4_SVP:   mime = (javacall_const_utf16_string)L"video/mp4";  break;
        case JC_FMT_MPEG_1:       mime = (javacall_const_utf16_string)L"video/mpeg";  break;
        case JC_FMT_AMR:          
        case JC_FMT_AMR_WB:          
        case JC_FMT_AMR_WB_PLUS:          
                                  mime = (javacall_const_utf16_string)L"audio/amr";  break;
        default:
            return JAVACALL_FAIL;
        }
    }

    mime_len = (long)wcslen( (const wchar_t*)mime );

    if( mime_equal( mime, mime_len, L"audio/mp3"  ) ||
        mime_equal( mime, mime_len, L"audio/mpeg" ) ||
        mime_equal( mime, mime_len, L"audio/MPA"  ) ||
        mime_equal( mime, mime_len, L"audio/X-MP3-draft-00" ) )
    {
        //get_int_param( mime, (javacall_const_utf16_string)L"channels", &(p->channels) );
        //get_int_param( mime, (javacall_const_utf16_string)L"rate",     &(p->rate)     );
        get_int_param( mime, (javacall_const_utf16_string)L"duration", &(p->duration) );

        p->mediaType = JC_FMT_MPEG1_LAYER3;
        mime = (javacall_const_utf16_string)L"audio/mpeg";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"video/x-vp6" ) ||
             mime_equal( mime, mime_len, L"video/x-flv" ) ||
             mime_equal( mime, mime_len, L"video/x-javafx" ) )
    {
        p->mediaType = JC_FMT_FLV;
        mime = (javacall_const_utf16_string)L"video/x-flv";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"video/3gpp" ) )
    {
        p->mediaType = JC_FMT_VIDEO_3GPP;
        mime = (javacall_const_utf16_string)L"video/3gpp";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"video/mp4" ) )
    {
        p->mediaType = JC_FMT_MPEG_4_SVP;
        mime = (javacall_const_utf16_string)L"video/mp4";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"video/mpeg" ) )
    {
        p->mediaType = JC_FMT_MPEG_1;
        mime = (javacall_const_utf16_string)L"video/mpeg";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"audio/amr" ) )
    {
        p->mediaType = JC_FMT_AMR;
        mime = (javacall_const_utf16_string)L"audio/amr";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else if( mime_equal( mime, mime_len, L"audio/wav" ) ||
             mime_equal( mime, mime_len, L"audio/x-wav" ))
    {
        p->mediaType = JC_FMT_MS_PCM;
        mime = (javacall_const_utf16_string)L"audio/wav";
        mime_len = (long)wcslen( (const wchar_t*)mime );
    }
    else
    {
        p->mediaType = JC_FMT_UNSUPPORTED;
    }

    p->mime       = mime;
    p->mimeLength = mime_len;

    p->is_video         = ( JC_FMT_FLV        == p->mediaType || 
                            JC_FMT_VIDEO_3GPP == p->mediaType || 
                            JC_FMT_MPEG_4_SVP == p->mediaType || 
                            JC_FMT_MPEG_1     == p->mediaType );

    lcd_set_color_key( JAVACALL_FALSE, 0 );

    PRINTF( "*** creating dshow player... ***\n" );

    bool ok = create_player_dshow( p->mimeLength, (const char16*)p->mime, p, &(p->ppl) );

    PRINTF( "*** dshow player creation finished (%s) ***\n",
            (ok ? "success" : "fail") );

    if( ok && -1 != p->whole_content_size )
    {
        p->ppl->set_stream_length(p->whole_content_size);
    }

    if( ok )
    {
        PRINTF( "*** realizing dshow player... ***\n" );
        ok = ( player::result_success == p->ppl->realize() );
    }

    PRINTF( "*** dshow player realize finished (%s), realize complete ***\n",
            (ok ? "success" : "fail") );

    if( ok )
    {
        dshow_player::players[ dshow_player::num_players++ ] = p;
        add_to_qsound( p );
    }
    else
    {
        outer_player->mediaHandle = NULL;
        delete p;
    }

    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}

static javacall_result dshow_destroy(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** destroy ***\n" );

    p->dwr_cancel = true;
    SetEvent( p->dwr_event );

    lcd_output_video_frame( NULL );

    if( NULL != p->ppl ) p->ppl->close();

    remove_from_qsound( p );

    if( NULL != p->video_frame ) delete p->video_frame;
    if( NULL != p->out_frame ) delete p->out_frame;
    if( NULL != p->ppl ) delete p->ppl;

    if( NULL != p->snapshot )
        javacall_media_release_data( p->snapshot, p->snapshot_len );

    DeleteCriticalSection( &(p->cs) );

    CloseHandle( p->dwr_event );

    int appId    = p->appId;
    int playerId = p->playerId;

    delete p;

    bool found = false;
    for( int i = 0; i < dshow_player::num_players-1; i++ )
    {
        if( dshow_player::players[i] == p ) found = true;
        if( found ) dshow_player::players[i] = dshow_player::players[i+1];
    }
    dshow_player::num_players--;

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DESTROY_FINISHED,
                                     appId,
                                     playerId, 
                                     JAVACALL_OK, NULL );

    return JAVACALL_OK;
}

//=============================================================================

static javacall_result dshow_get_format(javacall_handle handle, jc_fmt* fmt)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "*** get format ***\n" );
    *fmt = p->mediaType;
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

//=============================================================================

static void stop_thread( void* param )
{
    dshow_player*  p = (dshow_player*)param;
    bool           ok = true;
    player::result r;

    if( player::started == p->ppl->get_state() )
    {
        ok = ( player::result_success == ( r = p->ppl->stop() ) );
        if( ok ) p->playing = false;
    }

    if( ok && player::prefetched == p->ppl->get_state() )
    {
        ok = ( player::result_success == ( r = p->ppl->deallocate() ) );
    }

    assert( player::realized == p->ppl->get_state() );

    lcd_output_video_frame( NULL );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_STOP_FINISHED,
                                     p->appId,
                                     p->playerId,
                                     (ok ? JAVACALL_OK : JAVACALL_FAIL),
                                     NULL );
}

static javacall_result dshow_stop(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    player::result r;
    PRINTF( "*** stop, mt=%ld/%ld... ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );

    _beginthread( stop_thread, 0, p );

    return JAVACALL_OK;
}

static void pause_thread( void* param )
{
    dshow_player*  p = (dshow_player*)param;
    bool           ok = true;
    player::result r;

    if( player::started == p->ppl->get_state() )
    {
        p->get_media_time();
        ok = ( player::result_success == ( r = p->ppl->stop() ) );
        if( ok ) p->playing = false;
    } 
    else if( player::realized == p->ppl->get_state() )
    {
        ok = ( player::result_success == ( r = p->ppl->prefetch() ) );
    }

    assert( player::prefetched == p->ppl->get_state() );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_PAUSE_FINISHED,
                                     p->appId,
                                     p->playerId,
                                     (ok ? JAVACALL_OK : JAVACALL_FAIL),
                                     NULL );
}

static javacall_result dshow_pause(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    player::result r;
    PRINTF( "*** pause, mt=%ld/%ld... ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );

    _beginthread( pause_thread, 0, p );

    return JAVACALL_OK;
}

static void run_thread( void* param )
{
    dshow_player*  p = (dshow_player*)param;
    bool           ok = true;
    player::result r;

    if( player::realized == p->ppl->get_state() )
    {
        ok = ( player::result_success == ( r = p->ppl->prefetch() ) );
    }

    if( ok && player::prefetched == p->ppl->get_state() )
    {
        p->eom_sent = false;

        ok = ( player::result_success == ( r = p->ppl->start() ) );
        if( ok ) p->playing = true;
    }

    assert( player::started == p->ppl->get_state() );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_RUN_FINISHED,
                                     p->appId,
                                     p->playerId,
                                     (ok ? JAVACALL_OK : JAVACALL_FAIL),
                                     NULL );
}

static javacall_result dshow_run(javacall_handle handle)
{
    dshow_player* p = (dshow_player*)handle;
    player::result r;
    PRINTF( "*** run, mt=%ld/%ld... ***\n", p->get_media_time(),long(p->ppl->get_media_time(&r)/1000) );

    _beginthread( run_thread, 0, p );

    return JAVACALL_OK;
}

//=============================================================================

javacall_result dshow_stream_length(javacall_handle handle, javacall_int64 length)
{
    dshow_player* p = (dshow_player*)handle;

    PRINTF( "*** stream_length(%I64d) ***\n", length );

    p->whole_content_size = length;

    if( NULL != p->ppl ) 
    {
        player::result r = p->ppl->set_stream_length(length);
        return (player::result_success==r) ? JAVACALL_OK : JAVACALL_FAIL;
    }
    else
    {
        return JAVACALL_OK;
    }
}

javacall_result dshow_get_data_request(javacall_handle handle,
                                       javacall_int64* offset,
                                       javacall_int32* length)
{
    dshow_player* p = (dshow_player*)handle;

    *offset = p->dwr_offset;
    *length = p->dwr_len;

    PRINTF( "   --- get_data_request: @%I64d %d\n", *offset, *length );

    return JAVACALL_OK;
}

javacall_result dshow_data_ready(javacall_handle handle,
                                 javacall_int32  length,
                                 void**          data)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "       --- data_ready: %d\n", length );

    p->dwr_len = length;
    *data      = p->dwr_pdata;

    return JAVACALL_OK;
}

javacall_result dshow_data_written(javacall_handle handle,
                                   javacall_bool*  new_request)
{
    dshow_player* p = (dshow_player*)handle;
    PRINTF( "       --- data_written.\n" );
    SetEvent( p->dwr_event );
    *new_request = JAVACALL_FALSE;
    return JAVACALL_OK;
}

static javacall_result dshow_get_time(javacall_handle handle, javacall_int32* ms)
{
    dshow_player* p = (dshow_player*)handle;
    *ms = p->get_media_time();
    PRINTF( "--- get_time: %ld",*ms );
    // if( p->duration != -1 && *ms > p->duration ) *ms = p->duration;
    return JAVACALL_OK;
}

static void time_set_thread( void* param )
{
    dshow_player* p = (dshow_player*)param;

    PRINTF( "*** setting media time ***\n" );

    player::result r;

    int64 mt = int64( 1000 ) * int64( p->target_mt );

    p->media_time = long( p->ppl->set_media_time( mt, &r ) / 1000 );

    PRINTF( "*** set_time(%ld) finished: %ld", p->target_mt, p->media_time );

    BOOL ok = (player::result_success == r);

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_SET_MEDIA_TIME_FINISHED,
                                     p->appId,
                                     p->playerId, 
                                     (ok ? JAVACALL_OK : JAVACALL_FAIL),
                                     (void*)(p->media_time) );
}

static javacall_result dshow_set_time(javacall_handle handle, javacall_int32 ms)
{
    dshow_player* p = (dshow_player*)handle;
    p->target_mt    = ms;

    _beginthread( time_set_thread, 0, p );

    return JAVACALL_OK;
}

static javacall_result dshow_get_duration(javacall_handle handle, javacall_int32* ms)
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

static javacall_result dshow_switch_to_foreground(javacall_handle /*handle*/,
    int /*options*/)
{
    // dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

static javacall_result dshow_switch_to_background(javacall_handle /*handle*/,
    int /*options*/)
{
    // dshow_player* p = (dshow_player*)handle;
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

javacall_result dshow_get_max_rate(javacall_handle /*handle*/, long* /*maxRate*/)
{
    // dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_get_min_rate(javacall_handle /*handle*/, long* /*minRate*/)
{
    // dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_set_rate(javacall_handle /*handle*/, long /*rate*/)
{
    // dshow_player* p = (dshow_player*)handle;
    return JAVACALL_OK;
}

javacall_result dshow_get_rate(javacall_handle /*handle*/, long* /*rate*/)
{
    // dshow_player* p = (dshow_player*)handle;
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
        if( NULL != p->out_frame ) lcd_output_video_frame( p->out_frame );
    }
    else
    {
        lcd_output_video_frame( NULL );
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

        lcd_set_video_rect( x, y, w, h );
        lcd_output_video_frame( p->out_frame );

        if( NULL != old_frame ) delete old_frame;
    }
    else
    {
        lcd_set_video_rect( x, y, w, h );
    }

    LeaveCriticalSection( &(p->cs) );

    return JAVACALL_OK;
}

static javacall_result dshow_set_video_alpha(javacall_handle /*handle*/, javacall_bool on, javacall_pixel color)
{
    lcd_set_color_key( on, color );
    return JAVACALL_OK;
}

static javacall_result dshow_set_video_fullscreenmode(javacall_handle /*handle*/, javacall_bool /*fullScreenMode*/)
{
    return JAVACALL_FAIL;
}

/*****************************************************************************\
                      S N A P S H O T   S U P P O R T
\*****************************************************************************/

static javacall_result dshow_start_video_snapshot( javacall_handle handle, 
                                                   const javacall_utf16* imageType, 
                                                   long length )
{
    javacall_result res;
    dshow_player* p = (dshow_player*)handle;

    javacall_uint8* rgb888 = (javacall_uint8*)MALLOC( 3 * p->video_width * p->video_height );

    EnterCriticalSection( &(p->cs) );

    if( NULL != p->video_frame )
    {
        for( int y = 0; y < p->video_height; y++ )
        {
            for( int x = 0; x < p->video_width; x++ )
            {
                javacall_pixel pixel = *( p->video_frame + y * p->video_width + x );
                javacall_uint8* ptr = rgb888 + 3 * ( y * p->video_width + x );
                *ptr++ = pixel >> 11;             // R
                *ptr++ = ( pixel >> 5 ) & 0x3F;   // G
                *ptr++ = pixel & 0x1F;            // B
            }
        }
    }
    else
    {
        memset( rgb888, 0, 3 * p->video_width * p->video_height );
    }

    res = javacall_media_encode_start( rgb888,
                                       (javacall_uint16)p->video_width,
                                       (javacall_uint16)p->video_height,
                                       JAVACALL_JPEG_ENCODER,
                                       100, // javacall_uint8 quality,
                                       &( p->snapshot ),
                                       &( p->snapshot_len ),
                                       NULL );

    FREE( rgb888 );

    LeaveCriticalSection( &(p->cs) );

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
                                     p->appId,
                                     p->playerId,
                                     res,
                                     NULL );

    return res;
}

static javacall_result dshow_get_video_snapshot_data_size( javacall_handle handle, 
                                                           long* size )
{
    dshow_player* p = (dshow_player*)handle;

    if( NULL == p->snapshot ) return JAVACALL_FAIL;
    *size = p->snapshot_len;

    return JAVACALL_OK;
}

static javacall_result dshow_get_video_snapshot_data( javacall_handle handle, 
                                                      char* buffer,
                                                      long size )
{
    dshow_player* p = (dshow_player*)handle;

    if( NULL == p->snapshot ) return JAVACALL_FAIL;
    if( NULL == buffer || size < (long)p->snapshot_len ) return JAVACALL_INVALID_ARGUMENT;
    memcpy( buffer, p->snapshot, p->snapshot_len );

    return JAVACALL_OK;
}

/*****************************************************************************\
                      I N T E R F A C E   T A B L E S
\*****************************************************************************/

static media_basic_interface _dshow_basic_itf =
{
    dshow_create,
    dshow_destroy,

    dshow_get_format,
    dshow_get_player_controls,

    dshow_stop,
    dshow_pause,
    dshow_run,

    dshow_stream_length,
    dshow_get_data_request,
    dshow_data_ready,
    dshow_data_written,

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

static media_snapshot_interface _dshow_snapshot_itf = {
    dshow_start_video_snapshot,
    dshow_get_video_snapshot_data_size,
    dshow_get_video_snapshot_data
};

media_interface g_dshow_itf =
{
    &_dshow_basic_itf,
    &_dshow_volume_itf,
    &_dshow_video_itf,
    &_dshow_snapshot_itf,
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
