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

/*---------------------------------------------------------------------------*/

#include "multimedia.h"
#include "pcm_out.h"

/*---------------------------------------------------------------------------*/

#ifndef PCM_VIA_DSOUND

#define HDRS_COUNT      10

typedef struct tag_pcm_channel
{
    get_ch_data         gd_callback;
    void*               cb_param;

    int                 bits;              /* 8 or 16                    */
    int                 nch;               /* mono=1, stereo=2           */
    long                rate;              /* 44100, 22050 etc           */

    DWORD               blk_size;          /* data block size, bytes     */

    HWAVEOUT            hwo;               /* audio device handle        */
    volatile int        hdrs_free;         /* number of free headers     */
    int                 cur_hdr;           /* current header to write to */
    WAVEHDR             hdrs[ HDRS_COUNT ];
    CRITICAL_SECTION    cs;
} pcm_channel_t;

/*---------------------------------------------------------------------------*/

static void CALLBACK wave_out_proc( HWAVEOUT hwo,
                                    UINT uMsg,
                                    DWORD_PTR dwInstance,
                                    DWORD_PTR dwParam1,
                                    DWORD_PTR dwParam2 );

/*---------------------------------------------------------------------------*/

HANDLE                  g_hMixThread;
volatile BOOL           g_bMixStopFlag = FALSE;

                        /* 1 extra zero-filled cell is used when 
                           channel is removed */
pcm_channel_t*          g_Ch[ PCM_MAX_CHANNELS + 1 ];
volatile int            g_ChNum = 0;
HANDLE                  g_hChMutex = NULL;


DWORD WINAPI mixing_thread( void* arg );

pcm_handle_t pcm_out_open_channel( int          bits,
                                   int          nch,
                                   long         rate,
                                   long         blk_size,
                                   get_ch_data  gd_callback,
                                   void*        cb_param )
{
    pcm_channel_t*  ch;
    MMRESULT        mmr;
    WAVEFORMATEX    wfmt;
    int             i;

    if( PCM_MAX_CHANNELS == g_ChNum ) return NULL;

    JC_MM_ASSERT( ( 16 == bits ) || ( 8 == bits ) );
    JC_MM_ASSERT( ( 1 == nch ) || ( 2 == nch ) );

    if( 0 == g_ChNum )
    {
        /* first channel ==> init globals */
        memset( g_Ch, 0, sizeof( g_Ch ) );
        
        JC_MM_ASSERT( NULL == g_hChMutex );
        g_hChMutex = CreateMutex( NULL, FALSE, NULL );
    }

    /* create and initialize new channel */

    ch = (pcm_channel_t*)MALLOC( sizeof( pcm_channel_t ) );

    JC_MM_ASSERT( NULL != ch );

    ch->gd_callback = gd_callback;
    ch->cb_param    = cb_param;
    ch->bits        = bits;
    ch->nch         = nch;
    ch->rate        = rate;

    ch->blk_size    = blk_size;

    ch->hdrs_free   = HDRS_COUNT;
    ch->cur_hdr     = 0;

    memset( ch->hdrs, 0, sizeof( ch->hdrs ) );

    for( i = 0; i < HDRS_COUNT; i++ )
    {
        ch->hdrs[ i ].dwBufferLength = blk_size;
        ch->hdrs[ i ].lpData = MALLOC( blk_size );
    }

    InitializeCriticalSection( &(ch->cs) );

    memset( &wfmt, 0, sizeof( WAVEFORMATEX ) );

    wfmt.nSamplesPerSec  = rate;
    wfmt.nChannels       = nch;
    wfmt.wBitsPerSample  = bits;
    wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wfmt.cbSize          = 0;
    wfmt.nBlockAlign     = (WORD)((wfmt.wBitsPerSample * wfmt.nChannels)/8);
    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;

    mmr = waveOutOpen( &(ch->hwo),      
                       WAVE_MAPPER,
                       &wfmt,      
                       (DWORD_PTR)wave_out_proc,
                       (DWORD_PTR)ch,
                       CALLBACK_FUNCTION );

    if( MMSYSERR_NOERROR != mmr )
    {
        free( ch );
        return NULL;
    }

    /* Add new channel to channel list.
       We don't need locks here because channel addition
       occurs atomically upon "g_ChNum++" execution */
    g_Ch[ g_ChNum ] = ch;
    g_ChNum++; 

    if( 1 == g_ChNum )
    {
        /* this was the first channel, start mixing thread*/

        g_bMixStopFlag = FALSE;
        g_hMixThread = CreateThread( NULL, 0, mixing_thread, NULL, 
            CREATE_SUSPENDED, NULL);
        SetThreadPriority( g_hMixThread, THREAD_PRIORITY_HIGHEST );
        ResumeThread( g_hMixThread );
    }

    return ch;
}

void pcm_out_close_channel( pcm_handle_t hch )
{
    int i;
    int nch = -1; /* index of ch in channels[] */

    for( i = 0; i < PCM_MAX_CHANNELS; i++ )
    {
        if( g_Ch[ i ] == hch )
        {
            nch = i; break;
        }
    }

    JC_MM_ASSERT( -1 != nch );

    {
        WaitForSingleObject( g_hChMutex, INFINITE ); 

        /* note: channels[] has 1 extra zero-filled cell at the end */
        for( i = nch; i < PCM_MAX_CHANNELS; i++ ) g_Ch[ i ] = g_Ch[ i + 1 ];

        g_ChNum--;

        ReleaseMutex( g_hChMutex );
    }

    /* wait for channel to process all pending headers */

    while( hch->hdrs_free < HDRS_COUNT ) Sleep( 5 );

    /* channel cleanup */

    waveOutReset( hch->hwo );
    waveOutClose( hch->hwo );

    DeleteCriticalSection( &(hch->cs) );

    for( i = 0; i < HDRS_COUNT; i++ )
    {
        FREE( hch->hdrs[ i ].lpData );
    }

    FREE( hch );

    /* stop mixing thread and release hardware
       if it was the last channel */

    if( 0 == g_ChNum )
    {
        g_bMixStopFlag = TRUE;
        if( WAIT_OBJECT_0 != WaitForSingleObject( g_hMixThread, 1000 ) )
        {
            JC_MM_DEBUG_WARNING_PRINT( "Failed to wait for mixing thread shutdown\n" );
        }

        CloseHandle( g_hChMutex );
        g_hChMutex = NULL;
    }
}

/*---------------------------------------------------------------------------*/

static void CALLBACK wave_out_proc( HWAVEOUT hwo,
                                    UINT uMsg,
                                    DWORD_PTR dwInstance,
                                    DWORD_PTR dwParam1,
                                    DWORD_PTR dwParam2 )
{
    pcm_channel_t* ch = (pcm_channel_t*)dwInstance;
    if( WOM_DONE == uMsg )
    {
        EnterCriticalSection( &(ch->cs) );
        ch->hdrs_free++;
        LeaveCriticalSection( &(ch->cs) );
    }
}

/*---------------------------------------------------------------------------*/

DWORD WINAPI mixing_thread( void* arg )
{
    int             c;
    pcm_channel_t*  ch;
    BOOL            should_sleep;
    int             nread;
    MMRESULT        mmr;

    while( !g_bMixStopFlag )
    {
        should_sleep = TRUE;

        if( WAIT_OBJECT_0 == WaitForSingleObject( g_hChMutex, 10 ) )
        {
            for( c = 0; c < g_ChNum; c++ )
            {
                ch = g_Ch[ c ];

                if( ch->hdrs_free > 0 )
                {
                    nread = ch->gd_callback( ch->hdrs[ ch->cur_hdr ].lpData,
                                             ch->blk_size, 
                                             ch->cb_param );
                    if( nread > 0 )
                    {
                        WAVEHDR* pwh = ch->hdrs + ch->cur_hdr;

                        if(pwh->dwFlags & WHDR_PREPARED)
                            waveOutUnprepareHeader(ch->hwo, pwh, sizeof(WAVEHDR));

                        pwh->dwBufferLength = nread;

                        mmr = waveOutPrepareHeader( ch->hwo, pwh, sizeof( WAVEHDR ) );
                        mmr = waveOutWrite( ch->hwo, pwh, sizeof( WAVEHDR ) );

                        EnterCriticalSection( &(ch->cs) );
                        ch->hdrs_free--;
                        LeaveCriticalSection( &(ch->cs) );

                        ch->cur_hdr++;
                        if( HDRS_COUNT == ch->cur_hdr ) ch->cur_hdr = 0;

                        if( ch->hdrs_free > HDRS_COUNT / 2 ) should_sleep = FALSE;
                    }
                }

                if( g_bMixStopFlag ) break;
            }
            ReleaseMutex( g_hChMutex );
        }

        if( g_bMixStopFlag ) break;

        Sleep( should_sleep ? 50 : 0 );
    }

    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCM_VIA_DSOUND */
