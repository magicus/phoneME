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

#define INTERNAL_SOUNDBANK      0
#include "mm_qsound_audio.h"

#include "javacall_memory.h"
#include "javautil_unicode.h"

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
    va_list        args;

    va_start(args, fmt);
    vsprintf( str8, fmt, args );
    va_end(args);

    OutputDebugStringA( str8 );
}

#if( 1 == INTERNAL_SOUNDBANK )
#include "mQCore/soundbank.incl"
#endif /*INTERNAL_SOUNDBANK*/

/**********************************
 * GLOBALS
 **********************************/
globalMan g_QSoundGM[GLOBMAN_INDEX_MAX];   // IMPL_NOTE... NEED REVISIT


size_t mmaudio_get_isolate_mix( void *buffer, size_t length, void* param );

#if( 0 == INTERNAL_SOUNDBANK )
static void* g_pExternalBankData = NULL;    // IMPL_NOTE need to close
static int   g_iExternalBankSize = 0;
#endif /*INTERNAL_SOUNDBANK*/

static void doProcessHeader(ah* h, const void* buf, long buf_length);
static javacall_result audio_qs_get_duration(javacall_handle handle, javacall_int32* ms);

/* 1 second of playback */
#define PCM_PACKET_SIZE(wav) ((wav).rate * (wav).channels * ((wav).bits >> 3))
/* This is a threshould to start buffering */
#define MIN_PCM_BUFFERED_SIZE(wav) (3 * PCM_PACKET_SIZE(wav))
/* This threshould is to stop buffering sequence */
#define MAX_PCM_BUFFERED_SIZE(wav) (10 * PCM_PACKET_SIZE(wav))

#define MAX_METADATA_KEYS 64

/******************************************************************************/

static void sendEOM(int appId, int playerId, long duration)
{
    JC_MM_DEBUG_PRINT3("sendEOM app=%d player=%d, dur=%ld\n",
                       appId, playerId, duration);

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_END_OF_MEDIA,
        appId, playerId, JAVACALL_OK, (void*)duration);
}

/******************************************************************************/

static void MQ234_CALLBACK eom_event_trigger(void *userData)
{
    ah* h = (ah*)userData;
    // needs to be in milliseconds, so dive by 10
    long ms = mQ234_PlayControl_GetPosition(h->synth) / 10;
    sendEOM(h->appId, h->playerId, ms);
    mQ234_PlayControl_Play(h->synth, FALSE);
    h->state = PL135_PREFETCHED;
}

static void MQ234_CALLBACK fill_midi(void* userData,
                                     void* buffer, long position, int size)
{
    unsigned char *rb = NULL;
    ah* h = (ah*)userData;

    if( userData != NULL )
    {
        rb = h->dataBuffer;
        if ( rb != NULL )
        {
            int bytes_left = h->dataBufferLen - position;
            int n = size > bytes_left ? bytes_left : size;

            if (n > 0)
            {
                memcpy(buffer, rb + position, n);
            }
        }
    }
}

/******************************************************************************/

static void MQ234_CALLBACK ReadMemStorage(void* user, void* buffer,
                                          long position, int size)
{
    const unsigned char * soundbank_ptr = (const unsigned char *)user;

    memcpy( buffer, soundbank_ptr + position, size );
}

/******************************************************************************/

static void* MQ234_CALLBACK mQCoreAlloc(void *heap, size_t bytes)
{
    return MALLOC(bytes);
}


static void MQ234_CALLBACK mQCoreFree(void *heap, void *ptr)
{
    FREE(ptr);
}

/******************************************************************************/

static void* MQ234_CALLBACK mq_mutex_create( void* params )
{
    LPCRITICAL_SECTION pcs
        = (LPCRITICAL_SECTION)MALLOC( sizeof( CRITICAL_SECTION ) );

    InitializeCriticalSection( pcs );

    return pcs;
}

static void MQ234_CALLBACK  mq_mutex_destroy( void* m )
{
    DeleteCriticalSection( (LPCRITICAL_SECTION)m );
    FREE( m );
}

static void MQ234_CALLBACK  mq_mutex_lock( void* m )
{
    EnterCriticalSection( (LPCRITICAL_SECTION)m );
}

static void MQ234_CALLBACK  mq_mutex_unlock( void* m )
{
    LeaveCriticalSection( (LPCRITICAL_SECTION)m );
}

/******************************************************************************/

static javacall_result gmInit(int appId, int gmIdx)
{
    MQ234_CreateOptions opts;
    MQ234_SynthConfig   synthConfig;
    MQ234_HostBlock     dlsData;
    MQ234_ERROR         err;
    BOOL                pcm_ok;
    javacall_result     res = JAVACALL_FAIL;
    
    memset(&opts, '\0', sizeof(opts));

    opts.mutexCreateParams = NULL;
    opts.CreateMutex       = mq_mutex_create;
    opts.DestroyMutex      = mq_mutex_destroy;
    opts.LockMutex         = mq_mutex_lock;
    opts.UnlockMutex       = mq_mutex_unlock;

    opts.outputChannels = ENV_CHANNELS;
    opts.outputRate     = ENV_RATE;
    opts.outputBits     = ENV_BITS;
    opts.maxFrameSize   = ENV_BLOCK_SAMPLES;

    opts.HeapAlloc = mQCoreAlloc;
    opts.HeapFree = mQCoreFree;

    opts.scratchSize = 4096;
    opts.scratchMemory = MALLOC(opts.scratchSize);
    memset(opts.scratchMemory, '\0', opts.scratchSize);

    err = mQ234_Create(&(g_QSoundGM[gmIdx].gm), &opts, sizeof(opts));
    JC_MM_ASSERT(err==MQ234_ERROR_NO_ERROR);

    JC_MM_DEBUG_INFO_PRINT1("Created GM: %d\n",g_QSoundGM[gmIdx].gm);

    err = mQ234_SetupOutput( g_QSoundGM[gmIdx].gm,
                             MQ234_SPEAKERS,                  /*MQ234_HEADPHONES*/
                             MQ234_GEOMETRY_SPEAKER_DESKTOP); /*MQ234_GEOMETRY_SPEAKER_SIDE*/

    JC_MM_ASSERT(err==MQ234_ERROR_NO_ERROR);

    memset(&synthConfig, '\0', sizeof(synthConfig));
    synthConfig.structSize = sizeof(synthConfig);
    synthConfig.channels = ENV_CHANNELS;
    synthConfig.sampleRate = ENV_RATE;
    synthConfig.realTimeEvents = 4;
    synthConfig.voices = 32;

#if( 0 == INTERNAL_SOUNDBANK )
    if (g_pExternalBankData == NULL) {
        HKEY MyKey = NULL;
        DWORD VarType = REG_EXPAND_SZ;
        DWORD bs;
        char *b1;
        char *b2;

        FILE *dlsHandle;
        int tmp = 0;

        bs = MAX_PATH;
        b1 = MALLOC(MAX_PATH);
        memset(b1, '\0', MAX_PATH);

        RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic", 0,
            KEY_QUERY_VALUE, &MyKey);
        RegQueryValueEx(MyKey, "GMFilePath", NULL, &VarType, b1, &bs);

        if(MyKey != NULL) RegCloseKey(MyKey);

        if(!strlen(b1)) {
            FREE(b1);
            JC_MM_DEBUG_PRINT("Error: Can not find soundbank path in registry.\n");
            return 0;
        }

        b2 = MALLOC(MAX_PATH);
        ExpandEnvironmentStrings(b1, b2, MAX_PATH);
        FREE(b1);

        if( (dlsHandle = fopen(b2, "rb")) == NULL) {
            JC_MM_DEBUG_PRINT1("ERROR: Can't find soundbank file: %s\n", b2);
            // NEED REVISIT: shutdown create GM...
        }

        FREE(b2);
        fseek(dlsHandle, 0, SEEK_END);
        g_iExternalBankSize = ftell(dlsHandle);
        fseek(dlsHandle, 0, SEEK_SET );

        g_pExternalBankData = MALLOC(g_iExternalBankSize);
        if (g_pExternalBankData != NULL)
            tmp = fread(g_pExternalBankData, g_iExternalBankSize, 1, dlsHandle);

        fclose(dlsHandle);

        if ((g_pExternalBankData == NULL) || (tmp == 0)) {
            JC_MM_DEBUG_PRINT1("ERROR: Cannot load soundbank file: %s\n", b2);
            return 0;
        }
    }

    dlsData.storage = mQ234_CreateHostStorage(g_pExternalBankData, ReadMemStorage);
    dlsData.position = 0;
    dlsData.length  = g_iExternalBankSize;

#else
    dlsData.storage = mQ234_CreateHostStorage((void*)soundbank, ReadMemStorage);
    dlsData.position = 0;
    dlsData.length = sizeof( soundbank );
#endif /*INTERNAL_SOUNDBANK*/

    err = mQ234_SetupSynth(g_QSoundGM[gmIdx].gm, &dlsData, &synthConfig);
    JC_MM_ASSERT(err==MQ234_ERROR_NO_ERROR);

    err = mQ234_CreateSynthPlayer(g_QSoundGM[gmIdx].gm,
        &(g_QSoundGM[gmIdx].toneSynth), NULL);
    JC_MM_ASSERT(err==MQ234_ERROR_NO_ERROR);

    err = mQ234_AttachSynthPlayer(g_QSoundGM[gmIdx].gm,
        g_QSoundGM[gmIdx].toneSynth, 0);
    JC_MM_ASSERT(err==MQ234_ERROR_NO_ERROR);

    g_QSoundGM[gmIdx].sp = mQ234_PlayControl_GetSynthPerformance(
        g_QSoundGM[gmIdx].toneSynth);

    g_QSoundGM[gmIdx].EM135 = mQ234_GlobalManager_createEffectModule(
        g_QSoundGM[gmIdx].gm);

    g_QSoundGM[gmIdx].bDelayedMIDI = FALSE;
    g_QSoundGM[gmIdx].hMutexREAD   = CreateMutex( NULL, FALSE, NULL );
    /* IMPL_NOTE: this mutexes [and other resources associated with GMs]
     * are never released until we add code to reference-count GMs and
     * properely destroy them
     */

    g_QSoundGM[gmIdx].isolateId  = appId;
    pcm_ok = pcm_out_open_channel( &(g_QSoundGM[gmIdx].pcm_handle),
                                   ENV_BITS,
                                   ENV_CHANNELS,
                                   ENV_RATE,
                                   ENV_BLOCK_BYTES,
                                   mmaudio_get_isolate_mix,
                                   &(g_QSoundGM[gmIdx]) );

    JC_MM_DEBUG_PRINT1( "# pcm_out_open_channel returned 0x%08X\n",
            (int)(g_QSoundGM[gmIdx].pcm_handle) );

    if( !pcm_ok )
    {
        /* This happens if no audio device is found. Should proceed with the
           proper Java exceptions, not just debug assertion failure */

        res = JAVACALL_NO_AUDIO_DEVICE;
    }
    else 
    {
        res = JAVACALL_OK;
    }

    g_QSoundGM[gmIdx].isolateRefs = 1;

    return res;
}

javacall_result appIDtoGM(int appId, /*OUT*/ int *gmIdx )
{
    int i = GLOBMAN_INDEX_MAX-1;
    javacall_result res = JAVACALL_FAIL;

    while(i>=0)
    {
        if((g_QSoundGM[i].gm != NULL) && (g_QSoundGM[i].isolateId == appId))
        {
            g_QSoundGM[i].isolateRefs++;
            *gmIdx = i;
            res = ( NULL == g_QSoundGM[i].pcm_handle ) ?
                  JAVACALL_NO_AUDIO_DEVICE : JAVACALL_OK;
            return res;
        }

        --i;
    }

    // No GM for isolate, must be new, create one
    i = GLOBMAN_INDEX_MAX-1;

    while(i>=0)
    {
        if(g_QSoundGM[i].gm == NULL)
        {
            res = gmInit(appId, i);
            *gmIdx = i;
            return res;
        }

        --i;
    }

    *gmIdx = i;
    return res;
}

void gmDetach(int gmIdx)
{
    // NEED REVISIT: shutdown g_QSoundGM when isolateRefs gets to 0
    g_QSoundGM[gmIdx].isolateRefs--;
}

static int mmaudio_init()
{
    return 0;
}

static int mmaudio_destory(int gmIdx)
{
    return 0;

}

size_t mmaudio_get_isolate_mix( void *buffer, size_t length, void* param )
{
    MQ234_ERROR err;
    globalMan* gm = (globalMan*)param;
    
    JC_MM_ASSERT(length == ENV_BLOCK_BYTES);

    if( WAIT_OBJECT_0 == WaitForSingleObject( gm->hMutexREAD, 1000 ) )
    {
        err = mQ234_Read(gm->gm, buffer, ENV_BLOCK_SAMPLES);
        JC_MM_ASSERT(MQ234_ERROR_NO_ERROR==err);
        gm->bDelayedMIDI = FALSE;
        ReleaseMutex( gm->hMutexREAD );
    }

    return ENV_BLOCK_BYTES;
}

/*---------------------------------------------------------------------------*/

// NEED REVISIT: need handle to structure for the synth on this isolate.
javacall_result mmaudio_tone_note(long appId, long note, long dur, long vol)
{
    unsigned char msg[2];
    unsigned char status;
    int tchnl = 5; // NEED REVISIT: Need to see which channel should actually be used for tones.
    int gmidx = -1;
    javacall_result res = appIDtoGM( appId, &gmidx );
    
    if( JAVACALL_NO_AUDIO_DEVICE == res )
    {
        gmDetach(gmidx);
        return res;
    }

    if (vol < 0) {
        vol = 0;
    } else if (vol > 100) {
        vol = 127;
    } else {
        vol = (vol * 127)/100;
    }

    status = 0xC0 | (tchnl & 0x0F);   // MIDI set instrument cmd.
    msg[0] = 0x4B;  // General MIDI level 1. instrument. see:
    // http://www.midi.org/about-midi/gm/gm1sound.shtml
    // for a complete list of instruments available.
    // 4B = 75 = Flute (subtract 1 for 0 based offset)
    msg[1] = 0x00;

    mQ234_SynthPerformance_WriteEvent(g_QSoundGM[gmidx].sp, 0, status, msg, 2);

    status = 0x90 | (tchnl & 0x0F);
    msg[0] = (unsigned char)( note & 0xFF );
    msg[1] = (unsigned char)( vol & 0xFF );

    mQ234_SynthPerformance_WriteEvent(g_QSoundGM[gmidx].sp, 0, status, msg, 2);


    status = 0x80 | (tchnl & 0x0F);
    msg[0] = (unsigned char)( note & 0xFF );
    msg[1] = 0x00;

    mQ234_SynthPerformance_WriteEvent(g_QSoundGM[gmidx].sp, 2000, status, msg, 2);

    gmDetach(gmidx);

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_create(javacall_impl_player* outer_player)
{
    ah *h = NULL;
    int gmIdx = -1;
    MQ234_ERROR e;
    IPlayControl* synth;

    javacall_result res = appIDtoGM( outer_player->appId, &gmIdx );
    
    PRINTF( "\n ----- create(%S)", outer_player->uri );

    if( JAVACALL_OK != res )
    {
        gmDetach( gmIdx );
        return res;
    }
    
    JC_MM_DEBUG_PRINT1("audio create %s\n", __FILE__);
    JC_MM_ASSERT(gmIdx>=0);

    h = MALLOC(sizeof(ah));
    memset(h, '\0', sizeof(ah));

    e = mQ234_CreateSynthPlayer(g_QSoundGM[gmIdx].gm, &(synth), NULL);
    JC_MM_ASSERT(e == MQ234_ERROR_NO_ERROR);

    e = mQ234_AttachSynthPlayer(g_QSoundGM[gmIdx].gm, synth, 0);
    JC_MM_ASSERT(e == MQ234_ERROR_NO_ERROR);

    h->mediaType        = fmt_str2enum( outer_player->mediaType );
    h->appId            = outer_player->appId;
    h->playerId         = outer_player->playerId;
    h->streamLen        = outer_player->streamLen;
    h->gmIdx            = gmIdx;

    if( -1 != h->streamLen )
    {
        h->dataBufferLen = (int)( h->streamLen );
        h->dataBuffer    = malloc( h->dataBufferLen );

        if( NULL == h->dataBuffer ) return JAVACALL_OUT_OF_MEMORY;
    }

    // need some data to recognize sp-midi
    if( JC_FMT_MIDI == h->mediaType ) {
        h->needProcessHeader = JAVACALL_TRUE;
    } else {
        h->needProcessHeader = JAVACALL_FALSE;
    }

    h->controls[CON135_METADATA] =
        (IControl*)mQ234_PlayControl_getMetaDataControl(synth);
    h->controls[CON135_MIDI] =
        (IControl*)mQ234_PlayControl_getMIDIControl(synth);
    h->controls[CON135_PITCH] =
        (IControl*)mQ234_PlayControl_getPitchControl(synth);
    h->controls[CON135_RATE] =
        (IControl*)mQ234_PlayControl_getRateControl(synth);
    h->controls[CON135_TEMPO] =
        (IControl*)mQ234_PlayControl_getTempoControl(synth);

    h->controls[CON135_VOLUME] =
        (IControl*)mQ234_PlayControl_getVolumeControl(synth);

    h->synth    = synth;
    h->mtime    = -1;

    h->state = PL135_UNREALIZED;
    outer_player->mediaHandle = (javacall_handle)h;

    if( -1 != h->streamLen )
    {
        h->hRealizedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        javanotify_on_media_notification( JAVACALL_EVENT_MEDIA_DATA_REQUEST,
                                          h->appId,
                                          h->playerId, 
                                          JAVACALL_OK,
                                          NULL );

        res = ( WAIT_OBJECT_0 == WaitForSingleObject( h->hRealizedEvent, 10000 ) )
            ? JAVACALL_OK
            : JAVACALL_FAIL;

        CloseHandle( h->hRealizedEvent );
        h->hRealizedEvent = NULL;
    }

    return res;
}

static javacall_result audio_qs_destroy(javacall_handle handle)
{
    ah *h             = (ah*)handle;
    int appId         = h->appId;
    int playerId      = h->playerId;
    javacall_result r = JAVACALL_FAIL;
    int gmIdx         = h->gmIdx;

    JC_MM_DEBUG_PRINT1("audio_destroy %s\n",__FILE__);
    PRINTF( "- destroy" );

    if( h->synth != NULL )
    {
        mQ234_DetachSynthPlayer(g_QSoundGM[gmIdx].gm, h->synth);    
        mQ234_PlayControl_Destroy(h->synth);
        h->synth = NULL;
    }

    if (h->dataBuffer != NULL) {
        FREE(h->dataBuffer);
        h->dataBuffer    = NULL;
        h->dataBufferLen = 0;
        h->dataPos       = 0;
    }

    JC_MM_DEBUG_PRINT2("audio_close: h:%d  mt:%d\n", (int)handle, h->mediaType);
    h->state = PL135_CLOSED;

    if(h->doneCallback != NULL)
    {
        mQ234_EventTrigger_Destroy(h->doneCallback);
        h->doneCallback = NULL;
    }
    if( h->synth != NULL )
    {
        mQ234_PlayControl_Destroy(h->synth);
        h->synth = NULL;
    }
    if( h->midiStream != NULL )
    {
        FREE( h->midiStream );
        h->midiStream = NULL;
    }
    if( h->storage != NULL )
    {
        mQ234_HostStorage_Destroy( h->storage );
        h->storage = NULL;
    }

    if (h->dataBuffer) {
        FREE(h->dataBuffer);
        h->dataBuffer = NULL;
    }

    // need to destroy controls...??

    FREE(h);
    gmDetach(gmIdx);

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DESTROY_FINISHED,
                                     appId,
                                     playerId, 
                                     JAVACALL_OK, NULL );

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_format(javacall_handle handle, jc_fmt* fmt) {
    ah *h = (ah*)handle;
    *fmt = h->mediaType;
    JC_MM_DEBUG_INFO_PRINT1("audio_format: %d \n",
                            h->mediaType);
    return JAVACALL_OK;
}

static javacall_result audio_qs_get_player_controls(javacall_handle handle,
                                                    int* controls)
{
    ah* h = (ah*)handle;

    *controls = JAVACALL_MEDIA_CTRL_VOLUME;

    switch(h->mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_DEVICE_TONE:
            *controls |= JAVACALL_MEDIA_CTRL_TONE;
            *controls |= JAVACALL_MEDIA_CTRL_TEMPO;
            *controls |= JAVACALL_MEDIA_CTRL_RATE;
            *controls |= JAVACALL_MEDIA_CTRL_PITCH;
            break;
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
            *controls |= JAVACALL_MEDIA_CTRL_METADATA;
        case JC_FMT_DEVICE_MIDI:
            *controls |= JAVACALL_MEDIA_CTRL_MIDI;
            *controls |= JAVACALL_MEDIA_CTRL_TEMPO;
            *controls |= JAVACALL_MEDIA_CTRL_RATE;
            *controls |= JAVACALL_MEDIA_CTRL_PITCH;
            break;
        default:
            JC_MM_DEBUG_PRINT1("[jc-media] get_player_controls: unsupported media type %d\n", h->mediaType);
            JC_MM_ASSERT( FALSE );
            break;
    }

    return JAVACALL_OK;
}

//=============================================================================

static javacall_result audio_qs_do_deallocate(javacall_handle handle)
{
    ah *h = (ah*)handle;
    int gmIdx         = h->gmIdx;

    if(h->doneCallback != NULL)
    {
        mQ234_EventTrigger_Destroy(h->doneCallback);
        h->doneCallback = NULL;
    }

    if( h->midiStream != NULL && h->storage != NULL )
    {
        h->mtime = mQ234_PlayControl_GetPosition( h->synth );
    }

    if( h->midiStream != NULL )
    {
        FREE( h->midiStream );
        h->midiStream = NULL;
    }
    if( h->storage != NULL )
    {
        mQ234_HostStorage_Destroy( h->storage );
        h->storage = NULL;
    }

    h->state = PL135_REALIZED;
    return JAVACALL_OK;
}

static javacall_result audio_qs_do_prefetch(javacall_handle handle)
{
    ah*         h     = (ah *)handle;
    int         gmIdx = h->gmIdx;
    long        r     = -1;
    MQ234_ERROR e;

    if(h->dataBuffer != NULL)
    {
        MQ234_HostBlock *pHostBlock = NULL;
        ISynthPerformance *sp = NULL;

        // if HostStorage is still not created, create it!
        if( h->storage == NULL )
        {
            h->storage =
                mQ234_CreateHostStorage(h, fill_midi);
            JC_MM_ASSERT(h->storage != NULL);
        }

        // create new HostBlock unconditionally
        pHostBlock = MALLOC(sizeof(MQ234_HostBlock));
        JC_MM_ASSERT(pHostBlock != NULL);

        // initialize new HostBlock
        pHostBlock->length   = h->dataBufferLen;
        pHostBlock->position = 0;
        pHostBlock->storage  = h->storage;

        // use the new HostBlock to change data to be played
        // NB: this function also checks if the Tone Sequence buffered
        // is valid
        e = mQ234_SetSynthPlayerData(g_QSoundGM[gmIdx].gm,
            h->synth, pHostBlock);

        // destroy the previously used HostBlock, if any
        if( h->midiStream != NULL )
        {
            FREE(h->midiStream);
        }

        // remember the currently used HostBlock
        h->midiStream = pHostBlock;

        // NB: the following condition is not met if
        // the buffered Tone Sequence was invalid
        if(e == MQ234_ERROR_NO_ERROR)
        {
            sp = mQ234_PlayControl_GetSynthPerformance(h->synth);
            JC_MM_ASSERT(sp != NULL);
            h->doneCallback =
                mQ234_CreateEventTrigger(handle, eom_event_trigger);
            JC_MM_ASSERT(h->doneCallback != NULL);
            mQ234_SynthPerformance_SetDoneCallback(sp,
                h->doneCallback);

            r = h->dataBufferLen;

            if( -1 != h->mtime ) {
                mQ234_PlayControl_SetPosition( h->synth, h->mtime );
            }
        }

        // NB: r==-1 here may mean that the buffered Tone Sequence
        // was invalid
        if (-1 == r) {
            JC_MM_DEBUG_PRINT("Synth data NOT set!\n");
        } else {
            JC_MM_DEBUG_PRINT("Synth data set.\n");
        }
    }
    h->state = PL135_PREFETCHED;
    return JAVACALL_OK;
}

static javacall_result audio_qs_do_start(javacall_handle handle)
{
    ah* h = (ah*)handle;
    h->state = PL135_STARTED;
    mQ234_PlayControl_Play(h->synth, TRUE);
    return JAVACALL_OK;
}

static javacall_result audio_qs_do_stop(javacall_handle handle)
{
    ah* h = (ah*)handle;
    mQ234_PlayControl_Play(h->synth, FALSE);
    h->state = PL135_PREFETCHED;
    return JAVACALL_OK;
}

//=============================================================================

static javacall_result audio_qs_stop(javacall_handle handle){
    ah *h = (ah*)handle;

    if( PL135_STARTED == h->state )
    {
        PRINTF( "qs: stop -- stop...\n" );
        audio_qs_do_stop( handle );
        PRINTF( "qs: stop -- stop done.\n" );
    }

    if( PL135_PREFETCHED == h->state )
    {
        PRINTF( "qs: stop -- deallocate...\n" );
        audio_qs_do_deallocate( handle );
        PRINTF( "qs: stop -- deallocate done.\n" );
    }

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_STOP_FINISHED,
                                     h->appId,
                                     h->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result audio_qs_pause(javacall_handle handle){
    ah *h = (ah*)handle;

    if( PL135_STARTED == h->state )
    {
        PRINTF( "qs: pause -- stop...\n" );
        audio_qs_do_stop( handle );
        PRINTF( "qs: pause -- stop done.\n" );
    }
    else if( PL135_REALIZED == h->state )
    {
        PRINTF( "qs: pause -- prefetch...\n" );
        audio_qs_do_prefetch( handle );
        PRINTF( "qs: pause -- prefetch done.\n" );
    }

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_PAUSE_FINISHED,
                                     h->appId,
                                     h->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

static javacall_result audio_qs_run(javacall_handle handle){
    ah *h = (ah*)handle;

    if( PL135_REALIZED == h->state )
    {
        PRINTF( "qs: run -- prefetch...\n" );
        audio_qs_do_prefetch( handle );
        PRINTF( "qs: run -- prefetch done.\n" );
    }

    if( PL135_PREFETCHED == h->state )
    {
        PRINTF( "qs: run -- start...\n" );
        audio_qs_do_start( handle );
        PRINTF( "qs: run -- start done.\n" );
    }

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_RUN_FINISHED,
                                     h->appId,
                                     h->playerId, 
                                     JAVACALL_OK, 
                                     NULL );

    return JAVACALL_OK;
}

//=============================================================================

static javacall_result audio_qs_stream_length(javacall_handle handle,
                                              javacall_bool stream_len_known, 
                                              javacall_int64 length)
{
    ah* h = (ah*)handle;

    PRINTF( "- stream length %s (%I64d)", 
            (stream_len_known ? "known" : "unknown"), length );

    if( stream_len_known )
    {
        if(length > INT_MAX) return JAVACALL_OUT_OF_MEMORY;

        h->streamLen = length;

        if( h->dataBufferLen < (int)length )
        {
            h->dataBufferLen = (int)length;
            h->dataBuffer    = realloc( h->dataBuffer, h->dataBufferLen );

            if( NULL == h->dataBuffer ) return JAVACALL_OUT_OF_MEMORY;
        }
    }

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_data_request(javacall_handle handle, 
                                                 javacall_int64 *offset, 
                                                 javacall_int32 *length)
{
    ah* h = (ah*)handle;

    if( -1 == h->streamLen )
    {
        PRINTF( "  request: size unknown" );
        if( h->dataPos == h->dataBufferLen )
        {
            PRINTF( "  request: enlarging buffer" );
            h->dataBufferLen += 512;
            h->dataBuffer = realloc( h->dataBuffer, h->dataBufferLen );
        }
    }

    *offset = h->dataPos;
    *length = h->dataBufferLen - h->dataPos;

    PRINTF( "- request(%i @%i)", (int)*length, (int)*offset );

    return JAVACALL_OK;
}

static javacall_result audio_qs_data_ready(javacall_handle handle, 
                                           javacall_int32 length,
                                           void **data)
{
    ah* h = (ah*)handle;

    *data         = h->dataBuffer + h->dataPos;
    h->portionLen = length; // store until data is actually written

    return JAVACALL_OK;
}

static javacall_result audio_qs_data_written(javacall_handle handle, 
                                             javacall_bool *new_request)
{
    ah* h = (ah*)handle;

    PRINTF( "  - done(%i)", (int)h->portionLen );

    h->dataPos += h->portionLen;

    if( -1 != h->streamLen && h->dataPos == h->streamLen )
    {
        h->state = PL135_REALIZED;

        PRINTF( "  - realized." );

        if (h->needProcessHeader)
        {
            doProcessHeader(h, h->dataBuffer, h->dataBufferLen);
        }

        SetEvent( h->hRealizedEvent );

        *new_request = JAVACALL_FALSE;
    }
    else
    {
        *new_request = JAVACALL_TRUE;
    }

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_time(javacall_handle handle, javacall_int32* ms){

    ah* h = (ah*)handle;
    long pos;
    IRateControl *pRateControl = NULL;

    *ms = -1;

    if( h->midiStream != NULL && h->storage != NULL )
        pos = mQ234_PlayControl_GetPosition(h->synth);
    else
        pos = h->mtime;

    if(pos >= 0) *ms =  pos / 10;  // needs to be in millis

    pRateControl = mQ234_PlayControl_getRateControl( h->synth );
    if( NULL != pRateControl )
    {
        javacall_int64 rate = mQ135_Rate_GetRate( pRateControl );
        *ms = (long)( ( (javacall_int64)( *ms ) * rate ) / 100000L );
    }

    return JAVACALL_OK;
}

static javacall_result audio_qs_set_time(javacall_handle handle, javacall_int32 ms){

    ah* h = (ah*)handle;
    long currtime;

    PRINTF( "- set time (%ld)", ms );

    if( h->midiStream != NULL && h->storage != NULL )
    {
        currtime = mQ234_PlayControl_SetPosition(h->synth, ms*10) != 0 ?
        mQ234_PlayControl_GetPosition(h->synth)/10 : 0;
    }
    else
    {
        currtime = h->mtime = ms*10;
    }

    JC_MM_DEBUG_PRINT3("audio_set_time: h=0x%08X ms:%ld ct:%ld\n", (int)handle, ms, currtime);

    javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_SET_MEDIA_TIME_FINISHED,
                                     h->appId,
                                     h->playerId, 
                                     JAVACALL_OK, (void*)currtime );

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_duration(javacall_handle handle, javacall_int32* ms) {

    ah* h = (ah*)handle;

    long dur = mQ234_PlayControl_GetDuration(h->synth);

    if (dur > 0)
        *ms = dur / 10;// + 1600;
    else
        *ms = -1;

    // IMPL_NOTE: (1): add 1600 as get durations seems to be out by that
    // IMPL_NOTE: (2): removed because it leads to failure of some setMediaTime
    //                 tests.

    JC_MM_DEBUG_PRINT2("audio_duration: h=0x%08X dur=%ld\n", (int)handle, *ms);

    return JAVACALL_OK;
}

static javacall_result audio_qs_switch_to_foreground(javacall_handle handle,
                                                     int options)
{
    return JAVACALL_OK;
}

static javacall_result audio_qs_switch_to_background(javacall_handle handle,
                                                     int options)
{
    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

static javacall_result audio_qs_get_volume(javacall_handle handle, long* level) {

    ah* h = (ah*)handle;

    *level = (long) mQ135_Volume_GetLevel(
        (IVolumeControl*)h->controls[CON135_VOLUME]);

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_set_volume(javacall_handle handle, long* level) {

    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_FAIL;

    *level = (long) mQ135_Volume_SetLevel(
        (IVolumeControl*)h->controls[CON135_VOLUME], (int)(*level));

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_is_mute(javacall_handle handle, javacall_bool* mute ) {

    ah* h = (ah*)handle;

    int muted = (long) mQ135_Volume_IsMuted(
                      (IVolumeControl*)h->controls[CON135_VOLUME]);

    *mute = (muted == 0) ? JAVACALL_FALSE : JAVACALL_TRUE;

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_set_mute(javacall_handle handle,
                                      javacall_bool mute){

    ah* h = (ah*)handle;

    mQ135_Volume_SetMute(
        (IVolumeControl*)h->controls[CON135_VOLUME],
        (mute == JAVACALL_FALSE) ? 0 : 1);

    return JAVACALL_OK;
}


/* MIDIControl Functions ************************************************/
static javacall_result audio_qs_get_channel_volume(javacall_handle handle,
                                                   long channel, long *volume)
{
    ah* h = (ah*)handle;
    int gmIdx         = h->gmIdx;
    javacall_result r = JAVACALL_OK;
    int vol;

    while( g_QSoundGM[gmIdx].bDelayedMIDI ) Sleep( 0 );

    mQ135_MIDI_GetChannelVolume(
        (IMIDIControl*)h->controls[CON135_MIDI],
        (int)channel, &vol);
    *volume = (long)vol;

    return r;
}

static javacall_result audio_qs_set_channel_volume(javacall_handle handle,
                                                   long channel, long volume)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_FAIL;

    int tries;
    int v = -1;
    JSR135ErrorCode ec = JSR135Error_No_Error;
    for( tries = 5; tries > 0; tries-- )
    {

        ec = mQ135_MIDI_SetChannelVolume(
            (IMIDIControl*)h->controls[CON135_MIDI],
            (int)channel, (int)volume);


        /* This is necessary because only mQ234_Read will
         * cause this command to be executed. The mQ234_Read
         * will happen on the next poll in the RenderThread
         */

        if( ec != JSR135Error_No_Error )
        {
            return JAVACALL_FAIL;
        }

        Sleep(20);
        ec = mQ135_MIDI_GetChannelVolume(
            (IMIDIControl*)h->controls[CON135_MIDI],
            (int)channel, &v );

        if( ec != JSR135Error_No_Error )
        {
            return JAVACALL_FAIL;
        }

        if( v == volume ) // success
        {
            r = JAVACALL_OK;
            break;
        }
    }

    return r;
}

static javacall_result audio_qs_set_program(javacall_handle handle,
                                            long channel, long bank, long program)
{
    ah* h = (ah*)handle;

    int             b, p, retry;
    JSR135ErrorCode e;

    e = mQ135_MIDI_SetProgram(
            (IMIDIControl*)h->controls[CON135_MIDI],
            (int)channel, (int)bank, (int)program);

    if( JSR135Error_No_Error != e ) return JAVACALL_FAIL;

    for( retry = 0; retry < 5; retry++ )
    {
        e = mQ135_MIDI_GetProgram(
                (IMIDIControl*)h->controls[CON135_MIDI],
                channel, &b, &p);

        if( JSR135Error_No_Error != e ) return JAVACALL_FAIL;

        if( ( b == bank || -1 == bank ) && p == program )
        {
            return JAVACALL_OK;
        }

        Sleep( 20 );
    }

    return JAVACALL_FAIL;
}

static javacall_result audio_qs_short_midi_event(javacall_handle handle,
                                                 long type, long data1, long data2)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;
    int gmIdx         = h->gmIdx;

    if( WAIT_OBJECT_0 == WaitForSingleObject( g_QSoundGM[gmIdx].hMutexREAD, 500 ) )
    {
        mQ135_MIDI_ShortMidiEvent(
            (IMIDIControl*)h->controls[CON135_MIDI],
            (int)type, (int)data1, (int)data2);
        g_QSoundGM[gmIdx].bDelayedMIDI = TRUE;
        ReleaseMutex( g_QSoundGM[gmIdx].hMutexREAD );
    }
    else
    {
        r = JAVACALL_FAIL;
    }

    return r;
}

static javacall_result audio_qs_long_midi_event(javacall_handle handle,
                                                const char* data, long offset,
                                                long* length)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;
    int gmIdx         = h->gmIdx;
    int numused;

    if( WAIT_OBJECT_0 == WaitForSingleObject( g_QSoundGM[gmIdx].hMutexREAD, 500 ) )
    {
        mQ135_MIDI_LongMidiEvent(
            (IMIDIControl*)h->controls[CON135_MIDI],
            (char *)data, (int)offset, (int)*length, &numused);
        *length = (long)numused;
        g_QSoundGM[gmIdx].bDelayedMIDI = TRUE;
        ReleaseMutex( g_QSoundGM[gmIdx].hMutexREAD );
    }
    else
    {
        r = JAVACALL_FAIL;
    }

    return r;
}

/* MetaData Control Functions ************************************************/

static javacall_result audio_qs_get_metadata_key_counts(javacall_handle handle,
                                                        long *keyCounts)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;

    IMetaDataControl* mdc 
        = (IMetaDataControl*)( h->controls[CON135_METADATA] );

    char *keys[MAX_METADATA_KEYS];
    int i = 0;
    mQ135_MetaData_getKeys(mdc, keys, MAX_METADATA_KEYS);
    while(keys[i] != NULL) i++;
    *keyCounts = (long)i;

    return r;
}

static javacall_result audio_qs_get_metadata_key(javacall_handle handle,
                                                 long index, long bufLength,
                                                 javacall_utf16* buf)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;
    char *keys[MAX_METADATA_KEYS];
    int l, newl = 0;
    IMetaDataControl* mdc = NULL;

    if (index >= MAX_METADATA_KEYS || index < 0) {
        return JAVACALL_FAIL;
    }

    mdc = (IMetaDataControl*)( h->controls[CON135_METADATA] );

    mQ135_MetaData_getKeys(mdc, keys, MAX_METADATA_KEYS);
    l = strlen(keys[index]);
    javautil_unicode_utf8_to_utf16(keys[index], l, buf, bufLength, &newl);
    if (newl < bufLength)
        buf[newl] = 0;
    else
        buf[bufLength - 1] = 0;

    return r;
}

static javacall_result audio_qs_get_metadata(javacall_handle handle,
                                             javacall_const_utf16_string key, long bufLength,
                                             javacall_utf16* buf)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;
    int keyLen8 = 0, newl = 0;
    char *key8 = NULL, *val8 = NULL;

    IMetaDataControl* mdc 
        = (IMetaDataControl*)( h->controls[CON135_METADATA] );

    r = javautil_unicode_utf16_to_utf8(key, wcslen(key), 0, 0, &keyLen8);

    if (r == JAVACALL_OK) {
        key8 = MALLOC(keyLen8 + 1);
        javautil_unicode_utf16_to_utf8(key, wcslen(key), key8, keyLen8, &keyLen8);
        key8[keyLen8] = 0;

        val8 = MALLOC(bufLength);
        mQ135_MetaData_getKeyValue(mdc, key8, val8, bufLength);
        
        javautil_unicode_utf8_to_utf16(val8, strlen(val8), buf, bufLength, &newl);
        if (newl < bufLength)
            buf[newl] = 0;
        else
            buf[bufLength - 1] = 0;

        FREE(val8);
        FREE(key8);
    }

    return r;
}


/* RateControl Functions ************************************************/
static javacall_result audio_qs_get_max_rate(javacall_handle handle, long *maxRate)
{
    ah* h = (ah*)handle;
    
    *maxRate = mQ135_Rate_GetMaxRate((
        IRateControl*)h->controls[CON135_RATE]);

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_min_rate(javacall_handle handle, long *minRate)
{
    ah* h = (ah*)handle;

    *minRate = mQ135_Rate_GetMinRate(
        (IRateControl*)h->controls[CON135_RATE]);

    return JAVACALL_OK;
}

static javacall_result audio_qs_set_rate(javacall_handle handle, long rate)
{
    ah* h = (ah*)handle;
    long setRate;

    setRate = mQ135_Rate_SetRate(
        (IRateControl*)h->controls[CON135_RATE], rate);

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_rate(javacall_handle handle, long* rate)
{
    ah* h = (ah*)handle;

    *rate = mQ135_Rate_GetRate(
        (IRateControl*)h->controls[CON135_RATE]);

    return JAVACALL_OK;
}

/* TempoControl Functions ************************************************/
static javacall_result audio_qs_get_tempo(javacall_handle handle, long *tempo)
{
    ah* h = (ah*)handle;

    *tempo = mQ135_Tempo_GetTempo(
        (ITempoControl*)h->controls[CON135_TEMPO]);

    JC_MM_DEBUG_PRINT1("audio_get_tempo: rate:%ld\n", *tempo);

    return JAVACALL_OK;
}

static javacall_result audio_qs_set_tempo(javacall_handle handle, long tempo)
{
    ah* h = (ah*)handle;
    long setTempo;

    JC_MM_DEBUG_PRINT1("audio_set_tempo: rate:%ld\n", tempo);

    setTempo = mQ135_Tempo_SetTempo(
        (ITempoControl*)h->controls[CON135_TEMPO], tempo);

    return JAVACALL_OK;
}


/* PitchControl Functions ************************************************/

static javacall_result audio_qs_get_max_pitch(javacall_handle handle,
                                              long *maxPitch)
{
    ah* h = (ah*)handle;

    *maxPitch = mQ135_Pitch_GetMaxPitch(
        (IPitchControl*)h->controls[CON135_PITCH]);

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_min_pitch(javacall_handle handle,
                                              long *minPitch)
{
    ah* h = (ah*)handle;

    *minPitch = mQ135_Pitch_GetMinPitch(
        (IPitchControl*)h->controls[CON135_PITCH]);

    return JAVACALL_OK;
}

static javacall_result audio_qs_set_pitch(javacall_handle handle, long pitch)
{
    ah* h = (ah*)handle;
    long setPitch;

    setPitch = mQ135_Pitch_SetPitch(
        (IPitchControl*)h->controls[CON135_PITCH], pitch);

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_pitch(javacall_handle handle, long* pitch)
{
    ah* h = (ah*)handle;

    *pitch = mQ135_Pitch_GetPitch(
        (IPitchControl*)h->controls[CON135_PITCH]);

    return JAVACALL_OK;
}



/* Bank Query functions **************************************************/

static javacall_result audio_qs_is_bank_query_supported(javacall_handle handle,
                                                        long* supported)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;

    *supported = mQ135_MIDI_IsBankQuerySupported(
        (IMIDIControl*)h->controls[CON135_MIDI]);

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_bank_list(javacall_handle handle,
                                              long custom, short* banklist,
                                              long* numlist)
{
    ah* h = (ah*)handle;

    JSR135ErrorCode e = mQ135_MIDI_GetBankList(
        (IMIDIControl*)h->controls[CON135_MIDI],
        custom, banklist, numlist);

    return (e != JSR135Error_No_Error) ? JAVACALL_FAIL : JAVACALL_OK;
}

static javacall_result audio_qs_get_key_name(javacall_handle handle,
                                             long bank, long program,
                                             long key, char* keyname,
                                             long* keynameLen)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;

    JSR135ErrorCode e;

    memset(keyname, '\0', *keynameLen);
    e = mQ135_MIDI_GetKeyName(
        (IMIDIControl*)h->controls[CON135_MIDI],
        bank, program, key, keyname, *keynameLen);
    if(e == JSR135Error_Not_Found)
        *keynameLen = 0;
    else if(e != JSR135Error_No_Error)
        r = JAVACALL_FAIL;
    else
        *keynameLen = strlen(keyname);

    return r;
}

static javacall_result audio_qs_get_program_name(javacall_handle handle,
                                                 long bank, long program,
                                                 char* progname, long* prognameLen)
{
    ah* h = (ah*)handle;
    javacall_result r = JAVACALL_OK;

    JSR135ErrorCode e;

    memset(progname, '\0', *prognameLen);
    e = mQ135_MIDI_GetProgramName(
        (IMIDIControl*)h->controls[CON135_MIDI],
        bank, program, progname, *prognameLen);
    if(e != JSR135Error_No_Error)
        r = JAVACALL_FAIL;
    else
        *prognameLen = strlen(progname);

    return r;
}

static javacall_result audio_qs_get_program_list(javacall_handle handle,
                                                 long bank, char* proglist,
                                                 long* proglistLen)
{
    ah* h = (ah*)handle;

    JSR135ErrorCode e;

    memset(proglist, '\0', *proglistLen);
    e = mQ135_MIDI_GetProgramList(
        (IMIDIControl*)h->controls[CON135_MIDI],
        bank, proglist, proglistLen);

    return (e != JSR135Error_No_Error) ? JAVACALL_FAIL : JAVACALL_OK;
}

static javacall_result audio_qs_get_program(javacall_handle handle,
                                            long channel, long* prog)
{
    ah* h = (ah*)handle;
    int gmIdx         = h->gmIdx;
    JSR135ErrorCode e;

    while( g_QSoundGM[gmIdx].bDelayedMIDI ) Sleep( 0 );

    e = mQ135_MIDI_GetProgram(
        (IMIDIControl*)h->controls[CON135_MIDI],
        channel, &prog[0], &prog[1]);

    return (e != JSR135Error_No_Error) ? JAVACALL_FAIL : JAVACALL_OK;
}

/*****************************************************************************/

javacall_result audio_qs_tone_alloc_buffer(javacall_handle handle, int length, void** ptr)
{
    ah* h = (ah*)handle;

    PRINTF( "- alloc buffer (%i)", length );

    if( length > h->dataBufferLen )
    {
        h->dataBuffer = realloc( h->dataBuffer, length );
        if( NULL == h->dataBuffer ) return JAVACALL_OUT_OF_MEMORY;
    }

    h->dataBufferLen = length;
    h->dataPos       = length;
    h->streamLen     = length;

    *ptr = h->dataBuffer;

    return JAVACALL_OK;
}

javacall_result audio_qs_tone_sequence_written(javacall_handle handle)
{
    //ah* h = (ah*)handle;
    PRINTF( "- sequence written" );

    return JAVACALL_OK;
}

/*****************************************************************************/

/**
 * Retrieve needed parameters from the header. 
 * Now we distinquish MIDI and SP-MIDI.
 */
static void doProcessHeader(ah* h, const void* buf, long buf_length) {
    const unsigned char* buffer = (const unsigned char*)buf;
    
    if (h->needProcessHeader && buffer != NULL && buf_length > 0) {
        if (buf_length >= 6 && h->mediaType == JC_FMT_MIDI) {
            int i;
            int maxSearch = 512;
            
            if ((long)maxSearch > buf_length - 5) {
                maxSearch = (int)buf_length - 5;
            }
            for (i = 0; i < maxSearch; i++) {
                if ((buffer[i] == 0xF0) &&  
                    (buffer[i + 2] == 0x7F) &&
                    (buffer[i + 4] == 0x0B) &&
                    (buffer[i + 5] == 0x01)) {
                    h->mediaType = JC_FMT_SP_MIDI;
                    break;
                }
            }
        }
        h->needProcessHeader = JAVACALL_FALSE;
    }
}

/*****************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _audio_qs_basic_itf = {
    audio_qs_create,
    audio_qs_destroy,

    audio_qs_get_format,
    audio_qs_get_player_controls,

    audio_qs_stop,
    audio_qs_pause,
    audio_qs_run,

    audio_qs_stream_length,
    audio_qs_get_data_request,
    audio_qs_data_ready,
    audio_qs_data_written,

    audio_qs_get_time,
    audio_qs_set_time,
    audio_qs_get_duration,

    audio_qs_switch_to_foreground,
    audio_qs_switch_to_background
};

/**
 * Audio volume javacall function interface
 */
static media_volume_interface _audio_qs_volume_itf = {
    audio_qs_get_volume,
    audio_qs_set_volume,
    audio_qs_is_mute,
    audio_qs_set_mute
};

/**
 * MIDI javacall function interface
 */
static media_midi_interface _audio_qs_midi_itf = {
    audio_qs_get_channel_volume,
    audio_qs_set_channel_volume,
    audio_qs_set_program,
    audio_qs_short_midi_event,
    audio_qs_long_midi_event,
    audio_qs_is_bank_query_supported,
    audio_qs_get_bank_list,
    audio_qs_get_key_name,
    audio_qs_get_program_name,
    audio_qs_get_program_list,
    audio_qs_get_program
};

static media_tone_interface _audio_qs_tone_itf = {
    audio_qs_tone_alloc_buffer,
    audio_qs_tone_sequence_written
};

/**
 * MetaData javacall function interface
 */
static media_metadata_interface _audio_qs_metadata_itf = {
    audio_qs_get_metadata_key_counts,
    audio_qs_get_metadata_key,
    audio_qs_get_metadata
};

/**
 * Rate javacall function interface
 */
static media_rate_interface _audio_qs_rate_itf = {
    audio_qs_get_max_rate,
    audio_qs_get_min_rate,
    audio_qs_set_rate,
    audio_qs_get_rate
};


/**
 * Tempo javacall function interface
 */
static media_tempo_interface _audio_qs_tempo_itf = {
    audio_qs_get_tempo,
    audio_qs_set_tempo
};

/**
 * Pitch javacall function interface
 */
static media_pitch_interface _audio_qs_pitch_itf = {
    audio_qs_get_max_pitch,
    audio_qs_get_min_pitch,
    audio_qs_set_pitch,
    audio_qs_get_pitch
};

/*****************************************************************************/

/* Global audio interface */
media_interface g_qsound_itf = {
    &_audio_qs_basic_itf,
    &_audio_qs_volume_itf,
    NULL,
    NULL,
    &_audio_qs_midi_itf,
    &_audio_qs_tone_itf,
    &_audio_qs_metadata_itf,
    &_audio_qs_rate_itf,
    &_audio_qs_tempo_itf,
    &_audio_qs_pitch_itf,
    NULL,
    NULL
};
