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

//=============================================================================

#include "multimedia.h"
#include "pcm_out.h"

//=============================================================================

#ifdef PCM_VIA_DSOUND

#include <dsound.h>

typedef struct tag_pcm_channel
{
    get_ch_data         gd_callback;
    void*               cb_param;

    int                 bits;       /* 8 or 16                */
    int                 nch;        /* mono=1, stereo=2       */
    long                rate;       /* 44100, 22050 etc       */

    DWORD               blk_size;   /* data block size, bytes        */
    DWORD               blk_ready;  /* number of ready bytes in blk  */
    BYTE*               blk;        /* data block                    */

    LPDIRECTSOUNDBUFFER pDSB;
    DWORD               dsbuf_size; /* DirectSound buffer size       */
    DWORD               dsbuf_offs; /* current DS buf write position */
} pcm_channel_t;

//=============================================================================

LPDIRECTSOUND           g_pDS;
HANDLE                  g_hMixThread;
volatile BOOL           g_bMixStopFlag = FALSE;

                        /* 1 extra zero-filled cell is used when 
                           channel is removed */
pcm_channel_t*          g_Ch[ PCM_MAX_CHANNELS + 1 ];
volatile int            g_ChNum = 0;
HANDLE                  g_hChMutex = NULL;

//=============================================================================
#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================

DWORD WINAPI mixing_thread( void* arg );

pcm_handle_t pcm_out_open_channel( int          bits,
                                   int          nch,
                                   long         rate,
                                   long         blk_size,
                                   get_ch_data  gd_callback,
                                   void*        cb_param )
{
    HRESULT         r;
    HWND            hwnd;
    WAVEFORMATEX    wfmt;
    DSBUFFERDESC    bdsc; 

    pcm_channel_t*  ch;

    if( PCM_MAX_CHANNELS == g_ChNum ) return NULL;

    JC_MM_ASSERT( ( 16 == bits ) || ( 8 == bits ) );
    JC_MM_ASSERT( ( 1 == nch ) || ( 2 == nch ) );

    if( 0 == g_ChNum )
    {
        JC_MM_ASSERT( NULL == g_pDS );

        #ifdef ENABLE_JSR_135_DSHOW
        r = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if( DS_OK != r && S_FALSE != r )
        {
            return NULL;
        }
        #endif //ENABLE_JSR_135_DSHOW

        r = DirectSoundCreate(NULL, &g_pDS, NULL);
        if( DS_OK != r )
        {
            return NULL;
        }

        if( NULL == ( hwnd = GetForegroundWindow() ) ) hwnd = GetDesktopWindow();
        g_pDS->SetCooperativeLevel(hwnd, DSSCL_NORMAL);

        memset( g_Ch, 0, sizeof( g_Ch ) );
        
        JC_MM_ASSERT( NULL == g_hChMutex );
        g_hChMutex = CreateMutex( NULL, FALSE, NULL );
    }
    else /* not first channel ==> hardware device is already opened */
    {
        JC_MM_ASSERT( NULL != g_pDS );
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
    ch->blk         = (BYTE*)MALLOC( blk_size );
    ch->blk_ready   = 0;

    ch->dsbuf_size  = min( DSBSIZE_MAX, 8 * blk_size );
    ch->dsbuf_offs  = 0;

    JC_MM_ASSERT( NULL != ch->blk );

    memset( &wfmt, 0, sizeof( WAVEFORMATEX ) );
    memset( &bdsc, 0, sizeof( DSBUFFERDESC ) );

    wfmt.nSamplesPerSec  = rate;
    wfmt.nChannels       = nch;
    wfmt.wBitsPerSample  = bits;
    wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wfmt.cbSize          = 0;
    wfmt.nBlockAlign     = (WORD)((wfmt.wBitsPerSample * wfmt.nChannels)/8);
    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;

    bdsc.dwSize        = sizeof( DSBUFFERDESC );
    bdsc.dwBufferBytes = ch->dsbuf_size;
    bdsc.dwFlags       = DSBCAPS_STATIC |
                         DSBCAPS_GETCURRENTPOSITION2 |
                         DSBCAPS_GLOBALFOCUS;
    bdsc.lpwfxFormat   = &wfmt;

    r = g_pDS->CreateSoundBuffer( &bdsc, &(ch->pDSB), NULL );
    JC_MM_ASSERT(r == DS_OK);

    r = ch->pDSB->Play( 0, 0, DSBPLAY_LOOPING );
    JC_MM_ASSERT(r == DS_OK);


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

    /* channel cleanup */

    hch->pDSB->Stop();
    hch->pDSB->Release();
    FREE( hch->blk );
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

        g_pDS->Release();
        g_pDS = NULL;

        CloseHandle( g_hChMutex );
        g_hChMutex = NULL;
    }
}

/*---------------------------------------------------------------------------*/

DWORD get_available_space( pcm_channel_t* pch )
{
    DWORD   ret;
    HRESULT hr;
    DWORD   dwPlayCsr; //, dwWriteCsr;

    hr = pch->pDSB->GetCurrentPosition( &dwPlayCsr, NULL );

    if( DS_OK == hr )
    {
        if( pch->dsbuf_offs <= dwPlayCsr )
            ret = dwPlayCsr - pch->dsbuf_offs;
        else
            ret = pch->dsbuf_size - ( pch->dsbuf_offs - dwPlayCsr );
    }
    else
    {
        ret = 0;
    }

    return ret;
}

void output_channel( pcm_channel_t* pch )
{
    HRESULT hr;
    void*   ptr1;
    DWORD   cb1;
    void*   ptr2;
    DWORD   cb2;

    hr = pch->pDSB->Lock( pch->dsbuf_offs, pch->blk_ready,
                          &ptr1, &cb1,
                          &ptr2, &cb2,
                          0 );

    if( DSERR_BUFFERLOST == hr )
    {
        if( DS_OK == pch->pDSB->Restore() )
        {
            // second attempt
            hr = pch->pDSB->Lock( pch->dsbuf_offs, pch->blk_ready,
                                  &ptr1, &cb1,
                                  &ptr2, &cb2,
                                  0 );
        }
        else
        {
            // TODO: unable to restore buffer, signal an error.
        }
    }

    if( DS_OK == hr )
    {
        JC_MM_ASSERT( cb1 + cb2 == pch->blk_ready );

        if( 0 != cb1 ) memcpy( ptr1, pch->blk,       cb1 );
        if( 0 != cb2 ) memcpy( ptr2, pch->blk + cb1, cb2 );

        hr = pch->pDSB->Unlock( ptr1, cb1, ptr2, cb2 );
        JC_MM_ASSERT( DS_OK == hr );

        pch->blk_ready  -= ( cb1 + cb2 ); // must be 0 now
        pch->dsbuf_offs += ( cb1 + cb2 );

        if( pch->dsbuf_offs >= pch->dsbuf_size )
            pch->dsbuf_offs -= pch->dsbuf_size;
    }
}

DWORD WINAPI mixing_thread( void* arg )
{
    int             c;
    pcm_channel_t*  pch;
    BOOL            should_sleep;
    //long            read, bytes_mixed;

    //int smpl_size   = playback_nch * playback_bits / 8;
    //int blk_time_ms = 1000 * playback_blk / smpl_size / playback_rate;

    while( !g_bMixStopFlag )
    {
        should_sleep = TRUE;

        if( WAIT_OBJECT_0 == WaitForSingleObject( g_hChMutex, 10 ) )
        {
            for( c = 0; c < g_ChNum; c++ )
            {
                pch = g_Ch[ c ];

                if( 0 == pch->blk_ready )
                {
                    pch->blk_ready = pch->gd_callback( pch->blk,
                                                       pch->blk_size, 
                                                       pch->cb_param );
                }

                if( pch->blk_ready > 0 )
                {
                    DWORD avail = get_available_space( pch );
                    if( avail > pch->blk_ready )
                    {
                        output_channel( pch );
                        if( avail > pch->dsbuf_size / 2 ) should_sleep = FALSE;
                    }
                    else
                    {
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

#endif // PCM_VIA_DSOUND
