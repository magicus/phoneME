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

#ifdef ENABLE_AMR
#include "amr/AMRDecoder.h"
#endif // ENABLE_AMR

#include "javacall_memory.h"

#if( 1 == INTERNAL_SOUNDBANK )
#include "mQCore/soundbank.incl"
#endif /*INTERNAL_SOUNDBANK*/

/**********************************
 * GLOBALS
 **********************************/
globalMan g_QSoundGM[GLOBMAN_INDEX_MAX];   // IMPL_NOTE... NEED REVISIT

extern int wav_setStreamPlayerData(ah_wav *handle);   // IMPL_NOTE...

size_t mmaudio_get_isolate_mix( void *buffer, size_t length, void* param );

#if( 0 == INTERNAL_SOUNDBANK )
static void* g_pExternalBankData = NULL;    // IMPL_NOTE need to close
static int   g_iExternalBankSize = 0;
#endif /*INTERNAL_SOUNDBANK*/

static void doProcessHeader(ah* h, const void* buf, long buf_length);

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
    switch(((ah_hdr *)userData)->mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            ah_midi* hm = (ah_midi*)userData;
            // needs to be in milliseconds, so dive by 10
            long ms = mQ234_PlayControl_GetPosition(hm->synth) / 10;
            sendEOM(hm->hdr.isolateID, hm->hdr.playerID, ms);
        }
        break;

        default:
            JC_MM_DEBUG_ERROR_PRINT("Unexpected mediaType in trigger");
            JC_MM_ASSERT( FALSE );
    }
}

static void MQ234_CALLBACK fill_midi(void* userData,
                                     void* buffer, long position, int size)
{
    unsigned char *rb = NULL;

    if( userData != NULL )
    {
        rb = ((ah_midi *)userData)->midiBuffer;
        if ( rb != NULL )
        {
            int bytes_left = ((ah_midi *)userData)->midiBufferLen - position;
            int n = size > bytes_left ? bytes_left : size;

            if (n > 0)
            {
                memcpy(buffer, rb+position, n);
            }
        }
    }
}

/******************************************************************************/

// POST-condition: value (*pBytesGet) is correct or 0
static void* getNextSamples(void* userData, int bytesCnt, int* pBytesGet)
{
    void* pSB = NULL;
    int currentPos;
    ah_hdr* pHDR = (ah_hdr *)userData;

    if (pBytesGet)
        *pBytesGet = 0;

    switch(pHDR->mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            ah_wav* pWAV = (ah_wav *)userData;

            if (!pWAV->playing)
                return NULL;
            currentPos = pWAV->currentPos;

            if(currentPos >= pWAV->streamBufferLen) {
                if (pWAV->eom) {
                    return NULL;
                } else {
                    int bytesPerMilliSec = pWAV->bytesPerMilliSec;
                    int ms = (bytesPerMilliSec != 0)
                        ? (currentPos / bytesPerMilliSec)
                        : 0;
                    sendEOM(pWAV->hdr.isolateID,pWAV->hdr.playerID, ms);
                    pWAV->eom = 1;
                    pWAV->playing = 0;
                    return NULL;
                }
            }

            pSB = pWAV->streamBuffer + currentPos;
            if((currentPos + bytesCnt) > pWAV->streamBufferLen)
                bytesCnt = pWAV->streamBufferLen - currentPos;

            pWAV->currentPos += bytesCnt;
            if (pBytesGet)
                *pBytesGet = bytesCnt;
         }
         break;

        default:
            JC_MM_DEBUG_ERROR_PRINT("Unknown mediaType for stream read");
            JC_MM_ASSERT( FALSE );
            return NULL;
    }

    return pSB;
}

static void MQ234_CALLBACK fill_pcm_2c_16b(void* userData,
                                             void* buffer, int samples)
{
    const int numChannels = 2;
    const int sampleSize  = 2; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);

    if (pSB)
        memcpy(buffer, pSB, bytesGet);

    memset((char*)buffer + bytesGet, 0, bytesCnt - bytesGet);

/*
    {
        char msg[256];
        sprintf(msg, "filln2c16b: bytes=%d nread=%d\n", bytesGet, samples*4);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/

}

static void MQ234_CALLBACK fill_pcm_1c_16b(void* userData,
                                             void* buffer, int samples)
{
    const int numChannels = 1;
    const int sampleSize  = 2; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);

    if (pSB)
        memcpy(buffer, pSB, bytesGet);

    memset((char*)buffer + bytesGet, 0, bytesCnt - bytesGet);

/*
    {
        char msg[256];
        sprintf(msg, "filln1c16b: bytes=%d nread=%d\n", bytesGet, samples*2);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/
}



static void MQ234_CALLBACK fill_pcm_2c_8b(void* userData,
                                          void* buffer, int samples)
{
    const int numChannels = 2;
    const int sampleSize  = 1; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);

    /// Destination buffer has 16bit depth. So - upscale
    if (pSB) {
        char*  pSource = (char*)pSB;
        short* pDest   = (short*)buffer;
        int i;
        for (i = 0; i < bytesGet; i++) {
            pDest[i] = (pSource[i] << 8)^0x8000;
        }
    }
    memset((char*)buffer + bytesGet*2, 0, (bytesCnt - bytesGet)*2);

/*
    {
        char msg[256];
        sprintf(msg, "filln2c8b: bytesIn=%d bytesOut=%d nread=%d\n",
            bytesGet, bytesCnt*2, samples*1);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/

}


static void MQ234_CALLBACK fill_pcm_1c_8b(void* userData,
                                            void* buffer, int samples)
{
    const int numChannels = 1;
    const int sampleSize  = 1; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);

    /// Destination buffer has 16bit depth. So - upscale
    if (pSB) {
        char*  pSource = (char*)pSB;
        short* pDest   = (short*)buffer;
        int i;
        for (i = 0; i < bytesGet; i++) {
            pDest[i] = (pSource[i] << 8)^0x8000;
        }
    }
    memset((char*)buffer + bytesGet*2, 0, (bytesCnt - bytesGet)*2);

/*
    {
        char msg[256];
        sprintf(msg, "filln2c8b: bytesIn=%d bytesOut=%d nread=%d\n",
            bytesGet, bytesCnt*2, samples*1);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/

}


static void MQ234_CALLBACK fill_pcm_2c_24b(void* userData,
                                           void* buffer, int samples)
{
    const int numChannels = 2;
    const int sampleSize  = 3; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;
    int samplesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);
    samplesGet = bytesGet/3;

    /// Destination buffer has 16bit depth. So - downscale
    /// On Intel's 24bit pcm usually stored as little-endian
    if (pSB) {
        unsigned char*  pSource = (unsigned char*)pSB;
        unsigned short* pDest   = (unsigned short*)buffer;
        int i;
        for (i = 0; i < samplesGet; i++) {
            pDest[i] = (pSource[2] << 8) | pSource[1];
            pSource += 3;
        }
    }
    memset((char*)buffer + samplesGet*2, 0, (samples * numChannels - samplesGet)*2);
/*
    {
        char msg[256];
        sprintf(msg, "filln2c8b: bytesIn=%d bytesOut=%d nread=%d\n",
            bytesGet, bytesCnt*2, samples*1);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/

}


static void MQ234_CALLBACK fill_pcm_1c_24b(void* userData,
                                            void* buffer, int samples)
{
    const int numChannels = 1;
    const int sampleSize  = 3; /// In bytes
    int bytesCnt = samples * numChannels * sampleSize;

    void *pSB;
    int bytesGet;
    int samplesGet;

    pSB = getNextSamples(userData, bytesCnt, &bytesGet);
    samplesGet = bytesGet/3;

    /// Destination buffer has 16bit depth. So - downscale
    /// On Intel's 24bit pcm usually stored as little-endian
    if (pSB) {
        unsigned char*  pSource = (unsigned char*)pSB;
        unsigned short* pDest   = (unsigned short*)buffer;
        int i;
        for (i = 0; i < samplesGet; i++) {
            pDest[i] = (pSource[2] << 8) | pSource[1];
            pSource += 3;
        }
    }
    memset((char*)buffer + samplesGet*2, 0, (samples * numChannels - samplesGet)*2);

/*
    {
        char msg[256];
        sprintf(msg, "filln2c8b: bytesIn=%d bytesOut=%d nread=%d\n",
            bytesGet, bytesCnt*2, samples*1);
        JC_MM_DEBUG_ERROR_PRINT(msg);
    }
*/

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

static int gmInit(int isolateID, int gmIdx)
{
    MQ234_CreateOptions opts;
    MQ234_SynthConfig synthConfig;
    MQ234_HostBlock dlsData;
    MQ234_ERROR err;

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

    g_QSoundGM[gmIdx].isolateId  = isolateID;
    g_QSoundGM[gmIdx].pcm_handle = pcm_out_open_channel( ENV_BITS,
                                                 ENV_CHANNELS,
                                                 ENV_RATE,
                                                 ENV_BLOCK_BYTES,
                                                 mmaudio_get_isolate_mix,
                                                 (void*)isolateID );

    JC_MM_DEBUG_PRINT1( "# pcm_out_open_channel returned 0x%08X\n",
            (int)(g_QSoundGM[gmIdx].pcm_handle) );
    JC_MM_ASSERT(NULL != g_QSoundGM[gmIdx].pcm_handle);

    g_QSoundGM[gmIdx].isolateRefs = 1;

    return gmIdx;
}

int isolateIDtoGM(int isolateID)
{
    int i = GLOBMAN_INDEX_MAX-1;

    while(i>=0)
    {
        if((g_QSoundGM[i].gm != NULL) && (g_QSoundGM[i].isolateId == isolateID))
        {
            g_QSoundGM[i].isolateRefs++;
            return i;
        }

        --i;
    }

    // No GM for isolate, must be new, create one
    i = GLOBMAN_INDEX_MAX-1;

    while(i>=0)
    {
        if(g_QSoundGM[i].gm == NULL)
            return gmInit(isolateID, i);

        --i;
    }

    return i;
}

static void gmDetach(int gmIdx)
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
    int isolateid = (int)param;
    int gmIdx     = isolateIDtoGM(isolateid);
    //long i;
    //long sampleBytes = ( length > ENV_BLOCK_BYTES ) ? ENV_BLOCK_BYTES : length;
    //int  sampleCount = (((int)sampleBytes * 8) / ENV_BITS) / ENV_CHANNELS;   // sc * bitsbytes / bits / nchannels
    JC_MM_ASSERT(length == ENV_BLOCK_BYTES);

    if( WAIT_OBJECT_0 == WaitForSingleObject( g_QSoundGM[gmIdx].hMutexREAD, 1000 ) )
    {
        err = mQ234_Read(g_QSoundGM[gmIdx].gm, buffer, ENV_BLOCK_SAMPLES);
        JC_MM_ASSERT(MQ234_ERROR_NO_ERROR==err);
        g_QSoundGM[gmIdx].bDelayedMIDI = FALSE;
        ReleaseMutex( g_QSoundGM[gmIdx].hMutexREAD );
    }

    /*
    for (i=0; i<sampleBytes; i++)
        if (((char *)buffer)[i] != 0)
            return sampleBytes;

    return 0;
    */
    return ENV_BLOCK_BYTES;
}

/*---------------------------------------------------------------------------*/

// NEED REVISIT: need handle to structure for the synth on this isolate.
int mmaudio_tone_note(long isolateid, long note, long dur, long vol)
{
    unsigned char msg[2];
    unsigned char status;
    int tchnl = 5; // NEED REVISIT: Need to see which channel should actually be used for tones.
    int gmidx = isolateIDtoGM(isolateid);

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

    return 0;
}

/**
 *
 */
static javacall_handle audio_qs_create(int appId, int playerId,
                                       jc_fmt mediaType,
                                       const javacall_utf16_string URI)
{
    ah *newHandle = NULL;
    int isolateId = appId;
    int gmIdx = isolateIDtoGM(isolateId);
    JC_MM_DEBUG_PRINT1("audio create %s\n",__FILE__);
    JC_MM_ASSERT(gmIdx>=0);
    switch(mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            MQ234_ERROR e;
            IPlayControl* synth;

            newHandle = MALLOC(sizeof(ah_midi));
            memset(newHandle, '\0', sizeof(ah_midi));

            e = mQ234_CreateSynthPlayer(g_QSoundGM[gmIdx].gm, &(synth), NULL);
            JC_MM_ASSERT(e==MQ234_ERROR_NO_ERROR);

            e = mQ234_AttachSynthPlayer(g_QSoundGM[gmIdx].gm, synth, 0);
            JC_MM_ASSERT(e==MQ234_ERROR_NO_ERROR);

            newHandle->hdr.mediaType        = mediaType;
            newHandle->hdr.isolateID        = isolateId;
            newHandle->hdr.playerID         = playerId;
            newHandle->hdr.gmIdx            = gmIdx;
            newHandle->hdr.wholeContentSize = -1;
            newHandle->hdr.needProcessHeader= JAVACALL_FALSE;
            newHandle->hdr.dataBuffer       = NULL;
            newHandle->midi.midiBuffer      = NULL;
            newHandle->midi.midiBufferLen   = 0;

            newHandle->hdr.controls[CON135_METADATA] =
                (IControl*)mQ234_PlayControl_getMetaDataControl(synth);
            newHandle->hdr.controls[CON135_MIDI] =
                (IControl*)mQ234_PlayControl_getMIDIControl(synth);
            newHandle->hdr.controls[CON135_PITCH] =
                (IControl*)mQ234_PlayControl_getPitchControl(synth);
            newHandle->hdr.controls[CON135_RATE] =
                (IControl*)mQ234_PlayControl_getRateControl(synth);
            newHandle->hdr.controls[CON135_TEMPO] =
                (IControl*)mQ234_PlayControl_getTempoControl(synth);

            newHandle->hdr.controls[CON135_VOLUME] =
                (IControl*)mQ234_PlayControl_getVolumeControl(synth);

            newHandle->midi.synth    = synth;
            
            // need some data to recognize sp-midi
            if (mediaType == JC_FMT_MIDI) {
                newHandle->hdr.needProcessHeader = JAVACALL_TRUE;
            }
        }
        break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            IEffectModule* ef;
            newHandle = MALLOC(sizeof(ah_wav));
            memset(newHandle, '\0', sizeof(ah_wav));

            newHandle->hdr.mediaType        = mediaType;
            newHandle->hdr.isolateID        = isolateId;
            newHandle->hdr.playerID         = playerId;
            newHandle->hdr.gmIdx            = gmIdx;
            newHandle->hdr.wholeContentSize = -1;
            newHandle->hdr.needProcessHeader= JAVACALL_FALSE;
            newHandle->hdr.dataBuffer       = NULL;
            newHandle->wav.originalData     = NULL;
            newHandle->wav.originalDataLen  = 0;
            newHandle->wav.em               = NULL;

            ef = g_QSoundGM[gmIdx].EM135;

            newHandle->hdr.controls[CON135_RATE] =
                (IControl*)mQ234_EffectModule_getRateControl(ef);
            newHandle->hdr.controls[CON135_VOLUME] =
                (IControl*)mQ234_EffectModule_getVolumeControl(ef);

            // default rate required by JSR 135 SPEC is 100000, since it's not supported on WAV,
            // set it manualy
            mQ135_Rate_SetRate((IRateControl*)newHandle->hdr.controls[CON135_RATE], 100000);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1("[jc-media] Unsupported media type %d\n", mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_INFO_PRINT3("audio_create: mt:%d nh:%d gmi:%d\n",
                            mediaType, (int)newHandle, gmIdx);

    return (javacall_handle)newHandle;
}

/**
 *
 */
static javacall_result audio_qs_get_format(javacall_handle handle, jc_fmt* fmt) {
    ah *h             = (ah*)handle;
    
    *fmt = h->hdr.mediaType;
    JC_MM_DEBUG_INFO_PRINT1("audio_format: %d \n",
                            h->hdr.mediaType);
    return JAVACALL_OK;
}

/**
 *
 */

static javacall_result audio_qs_destroy(javacall_handle handle)
{
    ah *h             = (ah*)handle;
    javacall_result r = JAVACALL_FAIL;
    int gmIdx         = h->hdr.gmIdx;
    JC_MM_DEBUG_PRINT1("audio_destroy %s\n",__FILE__);

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {

            if(h->midi.doneCallback != NULL)
            {
                mQ234_EventTrigger_Destroy(h->midi.doneCallback);
                h->midi.doneCallback = NULL;
            }
            if( h->midi.synth != NULL )
            {
                mQ234_PlayControl_Destroy(h->midi.synth);
                h->midi.synth = NULL;
            }
            if( h->midi.midiStream != NULL )
            {
                FREE( h->midi.midiStream );
                h->midi.midiStream = NULL;
            }
            if( h->midi.storage != NULL )
            {
                mQ234_HostStorage_Destroy( h->midi.storage );
                h->midi.storage = NULL;
            }

            // need to destroy controls...??

            FREE(h);
            gmDetach(gmIdx);
            r = JAVACALL_OK;
        }
        break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            mQ234_WaveStream_Destroy(h->wav.stream);
            if (h->wav.streamBuffer != NULL) {
                FREE(h->wav.streamBuffer);
                h->wav.streamBuffer = NULL;
           }

           // need to destroy controls...??
           FREE(h);
           gmDetach(gmIdx);
           r = JAVACALL_OK;
        }

        break;

        default:
            JC_MM_DEBUG_PRINT1("[jc-media] Trying to close unsupported media type %d\n", h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_close(javacall_handle handle){

    ah *h             = (ah*)handle;
    javacall_result r = JAVACALL_FAIL;
    int gmIdx         = h->hdr.gmIdx;

    JC_MM_DEBUG_PRINT2("audio_close: h:%d  mt:%d\n", (int)handle, h->hdr.mediaType);

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            MQ234_ERROR e = mQ234_DetachSynthPlayer(g_QSoundGM[gmIdx].gm, h->midi.synth);
            JC_MM_ASSERT(e == MQ234_ERROR_NO_ERROR);

            r = JAVACALL_OK;
        }
        break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            if( NULL != h->wav.em )
                mQ234_EffectModule_removePlayer(h->wav.em, h->wav.stream);
            r = JAVACALL_OK;
        }

        break;

        default:
            JC_MM_DEBUG_PRINT1("[jc-media] Trying to close unsupported media type %d\n", h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_player_controls(javacall_handle handle,
                                                    int* controls)
{
    ah* h = (ah*)handle;

    *controls = JAVACALL_MEDIA_CTRL_VOLUME;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_DEVICE_TONE:
            *controls |= JAVACALL_MEDIA_CTRL_TONE;
            *controls |= JAVACALL_MEDIA_CTRL_METADATA;
            *controls |= JAVACALL_MEDIA_CTRL_TEMPO;
            *controls |= JAVACALL_MEDIA_CTRL_RATE;
            *controls |= JAVACALL_MEDIA_CTRL_PITCH;
            break;
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_MIDI:
            *controls |= JAVACALL_MEDIA_CTRL_METADATA;
            *controls |= JAVACALL_MEDIA_CTRL_MIDI;
            *controls |= JAVACALL_MEDIA_CTRL_TEMPO;
            *controls |= JAVACALL_MEDIA_CTRL_RATE;
            *controls |= JAVACALL_MEDIA_CTRL_PITCH;
            break;
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
            *controls |= JAVACALL_MEDIA_CTRL_RATE;
            break;
        default:
            JC_MM_DEBUG_PRINT1("[jc-media] get_player_controls: unsupported media type %d\n", h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
            break;
    }

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_acquire_device(javacall_handle handle)
{
    ah*         h     = (ah *)handle;
    int         gmIdx = h->hdr.gmIdx;
    long        r     = -1;

    MQ234_ERROR e;
    int         sRate;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
            if(h->midi.midiBuffer != NULL)
            {
                MQ234_HostBlock *pHostBlock = NULL;
                ISynthPerformance *sp = NULL;
                MQ234_ERROR e;

                /* if HostStorage is still not created, create it! */
                if( h->midi.storage == NULL )
                {
                    h->midi.storage =
                        mQ234_CreateHostStorage(&(h->midi), fill_midi);
                    JC_MM_ASSERT(h->midi.storage != NULL);
                }

                /* create new HostBlock unconditionally */
                pHostBlock = MALLOC(sizeof(MQ234_HostBlock));
                JC_MM_ASSERT(pHostBlock != NULL);

                /* initialize new HostBlock */
                pHostBlock->length   = h->midi.midiBufferLen;
                pHostBlock->position = 0;
                pHostBlock->storage  = h->midi.storage;

                /* use the new HostBlock to change data to be played */
                /* NB: this function also checks if the Tone Sequence buffered
                   is valid */
                e = mQ234_SetSynthPlayerData(g_QSoundGM[gmIdx].gm,
                    h->midi.synth, pHostBlock);

                /* destroy the previously used HostBlock, if any */
                if( h->midi.midiStream != NULL )
                {
                    FREE(h->midi.midiStream);
                }

                /* remember the currently used HostBlock */
                h->midi.midiStream = pHostBlock;

                /* NB: the following condition is not met if
                   the buffered Tone Sequence was invalid */
                if(e == MQ234_ERROR_NO_ERROR)
                {
                    sp = mQ234_PlayControl_GetSynthPerformance(h->midi.synth);
                    JC_MM_ASSERT(sp != NULL);
                    h->midi.doneCallback =
                        mQ234_CreateEventTrigger(handle, eom_event_trigger);
                    JC_MM_ASSERT(h->midi.doneCallback != NULL);
                    mQ234_SynthPerformance_SetDoneCallback(sp,
                        h->midi.doneCallback);

                    r = h->midi.midiBufferLen;
                }

                /* NB: r==-1 here may mean that the buffered Tone Sequence
                   was invalid */
                if( -1 == r )
                {
                    JC_MM_DEBUG_PRINT("Synth data NOT set!\n");
                }
                else
                {
                    JC_MM_DEBUG_PRINT("Synth data set.\n");
                }
            }
        break;

        case JC_FMT_MS_PCM:
            if( NULL != h->wav.stream )
            {
                if( NULL != h->wav.em )
                {
                    mQ234_EffectModule_removePlayer( h->wav.em, h->wav.stream );
                    h->wav.em = NULL;
                }
                mQ234_WaveStream_Destroy( h->wav.stream );
                h->wav.stream = NULL;
            }

            if (1 != wav_setStreamPlayerData(&(h->wav))) {
                return JAVACALL_FAIL;
            }
            sRate = h->wav.rate;

            if(16 == h->wav.bits)
            {
                switch(h->wav.channels) {
                    case 1:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_1c_16b, 1, sRate);
                        break;
                    case 2:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_2c_16b, 2, sRate);
                        break;
                    default:
                        h->wav.stream = NULL;
                        break;
                }
            } else if(8 == h->wav.bits) {
                switch(h->wav.channels) {
                    case 1:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_1c_8b, 1, sRate);
                        break;
                    case 2:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_2c_8b, 2, sRate);
                        break;
                    default:
                        h->wav.stream = NULL;
                        break;
                }
            } else if(24 == h->wav.bits) {
                switch(h->wav.channels) {
                    case 1:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_1c_24b, 1, sRate);
                        break;
                    case 2:
                        h->wav.stream = mQ234_CreateWaveStreamPlayer(
                            &(h->wav), fill_pcm_2c_24b, 2, sRate);
                        break;
                    default:
                        h->wav.stream = NULL;
                        break;
                }
            } else {
                h->wav.stream = NULL;
            }

            h->wav.originalData    = NULL;
            h->wav.originalDataLen = 0;

            h->wav.bytesPerMilliSec = (h->wav.rate *
                h->wav.channels * (h->wav.bits >> 3)) / 1000;

            if(h->wav.stream != NULL) {
                mQ234_EffectModule_addPlayer(
                    g_QSoundGM[gmIdx].EM135, h->wav.stream);
                h->wav.em = g_QSoundGM[gmIdx].EM135;
            }

            JC_MM_DEBUG_PRINT4( 
                "wavBuffered: bytes=%d rate=%d channels=%d bits=%d\n",
                h->wav.streamBufferLen, h->wav.rate,
                h->wav.channels, h->wav.bits);
        break;

#ifdef ENABLE_AMR
        case JC_FMT_AMR:
            if( NULL != h->wav.stream )
            {
                if( NULL != h->wav.em )
                {
                    mQ234_EffectModule_removePlayer( h->wav.em, h->wav.stream );
                    h->wav.em = NULL;
                }
                mQ234_WaveStream_Destroy( h->wav.stream );
                h->wav.stream = NULL;
            }

            if (1 != AMRDecoder_setStreamPlayerData(&(h->wav))) {
                return JAVACALL_FAIL;
            }

            switch( h->wav.channels )
            {
            case 1:
                h->wav.stream = mQ234_CreateWaveStreamPlayer(&(h->wav), fill_pcm_1c_16b, 1, h->wav.rate );
                break;
            case 2:
                h->wav.stream = mQ234_CreateWaveStreamPlayer(&(h->wav), fill_pcm_2c_16b, 2, h->wav.rate );
                break;
            default:
                h->wav.stream = NULL;
                break;
            }

            if( NULL != h->wav.originalData )
            {
                h->wav.originalData = NULL;
            }
            h->wav.originalDataLen = 0;

            //h->amr.bytesPerMilliSec = 2;
            h->wav.bytesPerMilliSec = (h->wav.rate * h->wav.channels * (16 >> 3)) / 1000;

            if(h->wav.stream != NULL)
            {
                e = mQ234_EffectModule_addPlayer(g_QSoundGM[gmIdx].EM135, h->wav.stream);
                h->wav.em = g_QSoundGM[gmIdx].EM135;
                JC_MM_ASSERT( MQ234_ERROR_NO_ERROR == e );
            }
        break;
#endif // ENABLE_AMR

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to play unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_release_device(javacall_handle handle){
    return JAVACALL_OK;
}

#define DEFAULT_BUFFER_SIZE  100 * 1024
#define DEFAULT_PACKET_SIZE  4096
static javacall_result audio_qs_get_java_buffer_size(javacall_handle handle,
                                                     long* java_buffer_size,
                                                     long* first_data_size)
{
    ah* h = (ah*)handle;

    if (h->hdr.mediaType == JC_FMT_DEVICE_TONE || h->hdr.mediaType == JC_FMT_DEVICE_MIDI) {
        *java_buffer_size = h->hdr.wholeContentSize;
        *first_data_size  = 0;
    } else {
        if (h->hdr.wholeContentSize <= 0) {
            *java_buffer_size = DEFAULT_BUFFER_SIZE;
            *first_data_size  = DEFAULT_PACKET_SIZE;
        } else {
            *java_buffer_size = h->hdr.wholeContentSize;
            *first_data_size  = h->hdr.wholeContentSize;
        }
    }
    return JAVACALL_OK;
}

static javacall_result audio_qs_set_whole_content_size(javacall_handle handle,
                                                       long whole_content_size)
{
    ah* h = (ah*)handle;

    h->hdr.wholeContentSize = whole_content_size;

    return JAVACALL_OK;
}

static javacall_result audio_qs_get_buffer_address(javacall_handle handle,
                                                   const void** buffer,
                                                   long* max_size)
{
    ah*         h     = (ah*)handle;
    int         gmIdx = h->hdr.gmIdx;
    long        size;

    switch(h->hdr.mediaType)
    {
    case JC_FMT_TONE:
    case JC_FMT_MIDI:
    case JC_FMT_SP_MIDI:
    case JC_FMT_DEVICE_TONE:
    case JC_FMT_DEVICE_MIDI:
        h->midi.midiBuffer = NULL;
        h->midi.midiBufferLen = 0;
        break;

    case JC_FMT_MS_PCM:
    case JC_FMT_AMR:
        h->wav.originalData = NULL;
        h->wav.originalDataLen = 0;
        break;

    default:
        JC_MM_DEBUG_PRINT1("[jc-media] get_buffer_address: unsupported media type %d\n", h->hdr.mediaType);
        JC_MM_ASSERT( FALSE );
        break;
    }
    if (h->hdr.dataBuffer == NULL) {
        if (h->hdr.wholeContentSize <= 0) {
            size = DEFAULT_BUFFER_SIZE;
        } else {
            size = h->hdr.wholeContentSize;
        }
        h->hdr.dataBuffer = MALLOC(size);
        if (h->hdr.dataBuffer == NULL) {
            JC_MM_DEBUG_PRINT("[jc-media] get_buffer_address: Cannot allocate buffer\n");
            h->hdr.dataBufferLen = 0;
            h->hdr.dataBufferPos = 0;
            return JAVACALL_OUT_OF_MEMORY;
        }
        h->hdr.dataBufferLen = size;
        h->hdr.dataBufferPos = 0;
    } else {
        size = h->hdr.dataBufferLen - h->hdr.dataBufferPos;
        if (size < DEFAULT_PACKET_SIZE) {
            long new_size = h->hdr.dataBufferLen * 2;
            
            if (new_size < DEFAULT_PACKET_SIZE) {
                new_size = DEFAULT_PACKET_SIZE;
            }
            h->hdr.dataBuffer = REALLOC(h->hdr.dataBuffer, new_size);
            if (h->hdr.dataBuffer == NULL) {
                JC_MM_DEBUG_PRINT("[jc-media] get_buffer_address: Cannot re-allocate buffer\n");
                FREE(h->hdr.dataBuffer);
                h->hdr.dataBuffer = NULL;
                h->hdr.dataBufferLen = 0;
                h->hdr.dataBufferPos = 0;
                return JAVACALL_OUT_OF_MEMORY;
            }
            h->hdr.dataBufferPos = h->hdr.dataBufferLen;
            h->hdr.dataBufferLen = new_size;
            size = h->hdr.dataBufferLen - h->hdr.dataBufferPos;
        }
    }
    *max_size = size;
    *buffer = h->hdr.dataBuffer + h->hdr.dataBufferPos;
    return JAVACALL_OK;
}

/**
 * Buffer media data.
 */
static javacall_result audio_qs_do_buffering(
                            javacall_handle handle, const void* buffer,
                            long *length, javacall_bool *need_more_data, 
                            long *min_data_size){

    ah*         h     = (ah *)handle;

    JC_MM_ASSERT(h->hdr.dataBuffer != NULL);
    if( NULL != buffer ) {
        if (h->hdr.needProcessHeader) {
            doProcessHeader(h, h->hdr.dataBuffer, h->hdr.dataBufferPos + *length);
        }

        JC_MM_ASSERT(buffer == h->hdr.dataBuffer + h->hdr.dataBufferPos);
        h->hdr.dataBufferPos += *length;
        switch(h->hdr.mediaType)
        {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
            h->midi.midiBuffer = h->hdr.dataBuffer;
            h->midi.midiBufferLen = h->hdr.dataBufferPos;
            break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
            h->wav.originalData = h->hdr.dataBuffer;
            h->wav.originalDataLen = h->hdr.dataBufferPos;
            break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to play unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
        }
        *need_more_data = JAVACALL_TRUE;
        *min_data_size  = DEFAULT_PACKET_SIZE;
    } else {
        *need_more_data = JAVACALL_FALSE;
        *min_data_size  = 0;
    }

    
    return JAVACALL_OK;
}

/**
 * Delete temp file
 */
static javacall_result audio_qs_clear_buffer(javacall_handle handle){

    javacall_result r = JAVACALL_FAIL;
    ah *h = (ah*)handle;

    JC_MM_DEBUG_PRINT("audio_qs_clear_buffer\n");

    if(h->hdr.dataBuffer != NULL) {
        
        FREE(h->hdr.dataBuffer);
        h->hdr.dataBuffer = NULL;
        
        switch(h->hdr.mediaType)
        {
            case JC_FMT_TONE:
            case JC_FMT_MIDI:
            case JC_FMT_SP_MIDI:
            case JC_FMT_DEVICE_TONE:
            case JC_FMT_DEVICE_MIDI:
            {
                if( h->midi.midiBuffer != NULL )
                {
                    h->midi.midiBuffer = NULL;
                    h->midi.midiBufferLen = 0;
                }
    
                r = JAVACALL_OK;
            }
    
            break;
    
            case JC_FMT_MS_PCM:
            case JC_FMT_AMR:
                if(h->wav.originalData != NULL)
                {
                    h->wav.originalDataLen = 0;
                    h->wav.originalData = NULL;
                }
    
                if(h->wav.streamBuffer != NULL)
                {
                    FREE(h->wav.streamBuffer);
                    h->wav.streamBufferLen = 0;
                    h->wav.streamBuffer = NULL;
                }
    
                r = JAVACALL_OK;
            break;
    
            default:
                JC_MM_DEBUG_PRINT1(
                    "[jc-media] Trying to clear unsupported media type %d\n",
                    h->hdr.mediaType);
                JC_MM_ASSERT( FALSE );
        }
    }

    return r;
}

/**
 *
 */
static javacall_result audio_qs_start(javacall_handle handle){

    javacall_result r = JAVACALL_FAIL;
    ah *h = (ah *)handle;
    long *duration;
    
    if (JAVACALL_OK == audio_qs_get_duration(handle, &duration) && 
            duration != -1) {
        javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_DURATION_UPDATED,
            h->hdr.isolateID, h->hdr.playerID, JAVACALL_OK, (void*)duration);
    }
    //printf("audio_start...\n");
    switch(h->hdr.mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            if (h->wav.stream != NULL) {
                h->wav.playing = 1;
                h->wav.eom = 0;
                r = JAVACALL_OK;
            }
        }
        break;

        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            //printf("audio_start MIDI/TONE\n");
            mQ234_PlayControl_Play(h->midi.synth, TRUE);
            r = JAVACALL_OK;
            //printf("audio_start MIDI/TONE started\n");
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to play unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    //printf( "...audio_start: h=0x%08X\n", (int)handle);


    return r;
}

/**
 *
 */
static javacall_result audio_qs_stop(javacall_handle handle){

    javacall_result r = JAVACALL_FAIL;
    ah *h = (ah *)handle;
    JC_MM_DEBUG_PRINT1("audio_stop %s\n",__FILE__);

    switch(h->hdr.mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            h->wav.playing = 0;
            r = JAVACALL_OK;
        }
        break;

        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            mQ234_PlayControl_Play(h->midi.synth, FALSE);

            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to stop unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT1("audio_stop: h=0x%08X\n", (int)handle);

    return r;
}

/**
 *
 */
static javacall_result audio_qs_pause(javacall_handle handle){
    javacall_result r = JAVACALL_FAIL;
    ah *h = (ah *)handle;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            h->wav.playing = 0;
            r = JAVACALL_OK;
        }
        break;

        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            mQ234_PlayControl_Play(h->midi.synth, FALSE);

            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to stop unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT("audio_pause\n");

    return r;
}

/**
 *
 */
static javacall_result audio_qs_resume(javacall_handle handle){
    javacall_result r = JAVACALL_FAIL;
    ah *h = (ah *)handle;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            h->wav.playing = 1;
            r = JAVACALL_OK;
        }
        break;

        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            mQ234_PlayControl_Play(h->midi.synth, TRUE);

            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to play unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT("audio_resume\n");

    return r;
}


/**
 *
 */
static javacall_result audio_qs_get_time(javacall_handle handle, long* ms){

    ah *h = (ah *)handle;
    *ms = -1;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        {
            if(h->wav.bytesPerMilliSec != 0)
                *ms = h->wav.currentPos / h->wav.bytesPerMilliSec;
        }
        break;

        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            long pos = mQ234_PlayControl_GetPosition(h->midi.synth);

            if(pos >= 0) *ms =  pos / 10;  // needs to be in millis
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get time for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_set_time(javacall_handle handle, long* ms){

    ah *h = (ah *)handle;
    long currtime;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
           currtime = mQ234_PlayControl_SetPosition(h->midi.synth, (*ms)*10) != 0 ?
           mQ234_PlayControl_GetPosition(h->midi.synth)/10 : 0;

        }
        break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR: // will need revisit when real streaming will be used
        {
            int newPos = h->wav.bytesPerMilliSec * (*ms);

            if(newPos > h->wav.streamBufferLen) newPos = h->wav.streamBufferLen;

            h->wav.currentPos = newPos;

            currtime = h->wav.bytesPerMilliSec != 0
                ? newPos / h->wav.bytesPerMilliSec
                : 0;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set time for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT3("audio_set_time: h=0x%08X ms:%ld ct:%ld\n", (int)handle, *ms, currtime);

    *ms = currtime;

    return JAVACALL_OK;
}

/**
 *
 */
static javacall_result audio_qs_get_duration(javacall_handle handle, long* ms) {

    ah *h = (ah *)handle;
    *ms = -1;
    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            long dur = mQ234_PlayControl_GetDuration(h->midi.synth);
            if(dur > 0) *ms = dur / 10;// + 1600;
            // IMPL_NOTE: (1): add 1600 as get durations seems to be out by that
            // IMPL_NOTE: (2): removed because it leads to failure of some setMediaTime
            //                 tests.
        }
        break;

        case JC_FMT_MS_PCM:
        case JC_FMT_AMR: // will need revisit when real streaming will be used
        {
            if(h->wav.bytesPerMilliSec != 0)
                *ms = h->wav.streamBufferLen / h->wav.bytesPerMilliSec;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get duration for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT2("audio_duration: h=0x%08X dur=%ld\n", (int)handle, *ms);

    return JAVACALL_OK;
}

/**
 * Now, switch to foreground
 */
static javacall_result audio_qs_switch_to_foreground(javacall_handle handle,
                                                     int options)
{

    return JAVACALL_OK;
}

/**
 * Now, switch to background
 */
static javacall_result audio_qs_switch_to_background(javacall_handle handle,
                                                     int options)
{

    return JAVACALL_OK;
}

/* VolumeControl Functions ************************************************/

static javacall_result audio_qs_get_volume(javacall_handle handle, long* level) {

    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_FAIL;
    *level = 0;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *level = (long) mQ135_Volume_GetLevel(
                (IVolumeControl*)h->hdr.controls[CON135_VOLUME]);
            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get volume for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

/**
 *
 */
static javacall_result audio_qs_set_volume(javacall_handle handle, long* level) {

    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_FAIL;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *level = (long) mQ135_Volume_SetLevel(
                (IVolumeControl*)h->hdr.controls[CON135_VOLUME], (int)(*level));
            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set volume for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

/**
 *
 */
static javacall_result audio_qs_is_mute(javacall_handle handle, javacall_bool* mute ) {

    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_FAIL;
    int muted = 0;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            muted = (long) mQ135_Volume_IsMuted(
                (IVolumeControl*)h->hdr.controls[CON135_VOLUME]);
            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get mute for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    *mute = (muted == 0) ? JAVACALL_FALSE : JAVACALL_TRUE;

    return r;
}

/**
 *
 */
static javacall_result audio_qs_set_mute(javacall_handle handle,
                                      javacall_bool mute){

    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_FAIL;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            mQ135_Volume_SetMute(
                (IVolumeControl*)h->hdr.controls[CON135_VOLUME],
                (mute == JAVACALL_FALSE) ? 0 : 1);
            r = JAVACALL_OK;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set mute for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}


/* MIDIControl Functions ************************************************/
static javacall_result audio_qs_get_channel_volume(javacall_handle handle,
                                                   long channel, long *volume)
{
    ah *h = (ah *)handle;
    int gmIdx         = h->hdr.gmIdx;
    javacall_result r = JAVACALL_OK;
    int vol;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            while( g_QSoundGM[gmIdx].bDelayedMIDI ) Sleep( 0 );

            mQ135_MIDI_GetChannelVolume(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                (int)channel, &vol);
            *volume = (long)vol;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get channel volume " \
                "for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_set_channel_volume(javacall_handle handle,
                                                   long channel, long volume)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_FAIL;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            int tries;
            int v = -1;
            JSR135ErrorCode ec = JSR135Error_No_Error;
            for( tries = 5; tries > 0; tries-- )
            {

                ec = mQ135_MIDI_SetChannelVolume(
                    (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                    (int)channel, (int)volume);


                /* This is needed because an mQ234_Read is needed to
                 * cause this command to executed. The mQ234_Read
                 * will happned on the next poll in the runRenderThread
                 */

                if( ec != JSR135Error_No_Error )
                {
                    return JAVACALL_FAIL;
                }

                Sleep(20);
                ec = mQ135_MIDI_GetChannelVolume(
                    (IMIDIControl*)h->hdr.controls[CON135_MIDI],
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
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set channel volume for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_set_program(javacall_handle handle,
                                            long channel, long bank, long program)
{
    ah *h = (ah *)handle;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            int             b, p, retry;
            JSR135ErrorCode e;

            e = mQ135_MIDI_SetProgram(
                    (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                    (int)channel, (int)bank, (int)program);

            if( JSR135Error_No_Error != e ) return JAVACALL_FAIL;

            for( retry = 0; retry < 5; retry++ )
            {
                e = mQ135_MIDI_GetProgram(
                        (IMIDIControl*)h->hdr.controls[CON135_MIDI],
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
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set program for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
            return JAVACALL_FAIL;
    }
}

static javacall_result audio_qs_short_midi_event(javacall_handle handle,
                                                 long type, long data1, long data2)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    int gmIdx         = h->hdr.gmIdx;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            if( WAIT_OBJECT_0 == WaitForSingleObject( g_QSoundGM[gmIdx].hMutexREAD, 500 ) )
            {
                mQ135_MIDI_ShortMidiEvent(
                    (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                    (int)type, (int)data1, (int)data2);
                g_QSoundGM[gmIdx].bDelayedMIDI = TRUE;
                ReleaseMutex( g_QSoundGM[gmIdx].hMutexREAD );
            }
            else
            {
                r = JAVACALL_FAIL;
            }
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to short midi event for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_long_midi_event(javacall_handle handle,
                                                const char* data, long offset,
                                                long* length)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    int gmIdx         = h->hdr.gmIdx;
    int numused;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            if( WAIT_OBJECT_0 == WaitForSingleObject( g_QSoundGM[gmIdx].hMutexREAD, 500 ) )
            {
                mQ135_MIDI_LongMidiEvent(
                    (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                    (char *)data, (int)offset, (int)*length, &numused);
                *length = (long)numused;
                g_QSoundGM[gmIdx].bDelayedMIDI = TRUE;
                ReleaseMutex( g_QSoundGM[gmIdx].hMutexREAD );
            }
            else
            {
                r = JAVACALL_FAIL;
            }
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to long midi event for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

/* MetaData Control Functions ************************************************/

static javacall_result audio_qs_get_metadata_key_counts(javacall_handle handle,
                                                        long *keyCounts)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            IMetaDataControl* mdc 
                = (IMetaDataControl*)( h->hdr.controls[CON135_METADATA] );

            char *keys[50];
            int i = 0;
            mQ135_MetaData_getKeys(mdc, keys, 50);
            while(keys[i] != NULL) i++;
            *keyCounts = (long)i;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get key counts for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_metadata_key(javacall_handle handle,
                                                 long index, long bufLength,
                                                 char *buf)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    char *keys[50];
    int l;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            IMetaDataControl* mdc 
                = (IMetaDataControl*)( h->hdr.controls[CON135_METADATA] );

            mQ135_MetaData_getKeys( mdc, keys, 50);
            l = strlen(keys[index]);
            memset(buf, '\0', bufLength);
            memcpy(buf, keys[index], bufLength < l ? bufLength : l);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get key index for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_metadata(javacall_handle handle,
                                             const char* key, long bufLength,
                                             char *buf)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            IMetaDataControl* mdc 
                = (IMetaDataControl*)( h->hdr.controls[CON135_METADATA] );

            memset(buf, '\0', bufLength);
            mQ135_MetaData_getKeyValue(mdc, key, buf, bufLength);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get key index for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}


/* RateControl Functions ************************************************/
static javacall_result audio_qs_get_max_rate(javacall_handle handle, long *maxRate)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    
    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *maxRate = mQ135_Rate_GetMaxRate((
                IRateControl*)h->hdr.controls[CON135_RATE]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get max rate for "\
                " unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_min_rate(javacall_handle handle, long *minRate)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *minRate = mQ135_Rate_GetMinRate(
                (IRateControl*)h->hdr.controls[CON135_RATE]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get min rate for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }
    return r;
}

static javacall_result audio_qs_set_rate(javacall_handle handle, long rate)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    long setRate;
    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            setRate = mQ135_Rate_SetRate(
                (IRateControl*)h->hdr.controls[CON135_RATE], rate);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set rate for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }
    return r;
}

static javacall_result audio_qs_get_rate(javacall_handle handle, long* rate)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_MS_PCM:
        case JC_FMT_AMR:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *rate = mQ135_Rate_GetRate(
                (IRateControl*)h->hdr.controls[CON135_RATE]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get rate for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }
    return r;
}

/* TempoControl Functions ************************************************/
static javacall_result audio_qs_get_tempo(javacall_handle handle, long *tempo)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *tempo = mQ135_Tempo_GetTempo(
                (ITempoControl*)h->hdr.controls[CON135_TEMPO]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get tempo for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    JC_MM_DEBUG_PRINT1("audio_get_tempo: rate:%ld\n", *tempo);

    return r;
}

static javacall_result audio_qs_set_tempo(javacall_handle handle, long tempo)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    long setTempo;

    JC_MM_DEBUG_PRINT1("audio_set_tempo: rate:%ld\n", tempo);

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            setTempo = mQ135_Tempo_SetTempo(
                (ITempoControl*)h->hdr.controls[CON135_TEMPO], tempo);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set tempo for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}


/* PitchControl Functions ************************************************/

static javacall_result audio_qs_get_max_pitch(javacall_handle handle,
                                              long *maxPitch)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *maxPitch = mQ135_Pitch_GetMaxPitch(
                (IPitchControl*)h->hdr.controls[CON135_PITCH]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get max pitch for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_min_pitch(javacall_handle handle,
                                              long *minPitch)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *minPitch = mQ135_Pitch_GetMinPitch(
                (IPitchControl*)h->hdr.controls[CON135_PITCH]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get min pitch for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_set_pitch(javacall_handle handle, long pitch)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;
    long setPitch;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            setPitch = mQ135_Pitch_SetPitch(
                (IPitchControl*)h->hdr.controls[CON135_PITCH], pitch);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to set pitch for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_pitch(javacall_handle handle, long* pitch)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *pitch = mQ135_Pitch_GetPitch(
                (IPitchControl*)h->hdr.controls[CON135_PITCH]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get pitch for unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}



/* Bank Query functions **************************************************/

static javacall_result audio_qs_is_bank_query_supported(javacall_handle handle,
                                                        long* supported)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            *supported = mQ135_MIDI_IsBankQuerySupported(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI]);
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to query bank support for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_bank_list(javacall_handle handle,
                                              long custom, short* banklist,
                                              long* numlist)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            JSR135ErrorCode e = mQ135_MIDI_GetBankList(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                custom, banklist, numlist);
            if(e != JSR135Error_No_Error) r = JAVACALL_FAIL;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get bank list for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_key_name(javacall_handle handle,
                                             long bank, long program,
                                             long key, char* keyname,
                                             long* keynameLen)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            JSR135ErrorCode e;

            memset(keyname, '\0', *keynameLen);
            e = mQ135_MIDI_GetKeyName(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                bank, program, key, keyname, *keynameLen);
            if(e == JSR135Error_Not_Found)
                *keynameLen = 0;
            else if(e != JSR135Error_No_Error)
                r = JAVACALL_FAIL;
            else
                *keynameLen = strlen(keyname);

        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get key name for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_program_name(javacall_handle handle,
                                                 long bank, long program,
                                                 char* progname, long* prognameLen)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            JSR135ErrorCode e;

            memset(progname, '\0', *prognameLen);
            e = mQ135_MIDI_GetProgramName(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                bank, program, progname, *prognameLen);
            if(e != JSR135Error_No_Error)
                r = JAVACALL_FAIL;
            else
                *prognameLen = strlen(progname);

        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get program name for " \
                " unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_program_list(javacall_handle handle,
                                                 long bank, char* proglist,
                                                 long* proglistLen)
{
    ah *h = (ah *)handle;
    javacall_result r = JAVACALL_OK;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            JSR135ErrorCode e;

            memset(proglist, '\0', *proglistLen);
            e = mQ135_MIDI_GetProgramList(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                bank, proglist, proglistLen);
            if(e != JSR135Error_No_Error)
                r = JAVACALL_FAIL;
        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to get program name for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

static javacall_result audio_qs_get_program(javacall_handle handle,
                                            long channel, long* prog)
{
    ah *h = (ah *)handle;
    int gmIdx         = h->hdr.gmIdx;
    javacall_result r = JAVACALL_OK;
    JSR135ErrorCode e;

    switch(h->hdr.mediaType)
    {
        case JC_FMT_TONE:
        case JC_FMT_MIDI:
        case JC_FMT_SP_MIDI:
        case JC_FMT_DEVICE_TONE:
        case JC_FMT_DEVICE_MIDI:
        {
            while( g_QSoundGM[gmIdx].bDelayedMIDI ) Sleep( 0 );

            e = mQ135_MIDI_GetProgram(
                (IMIDIControl*)h->hdr.controls[CON135_MIDI],
                channel, &prog[0], &prog[1]);

            if(e != JSR135Error_No_Error) r = JAVACALL_FAIL;

        }
        break;

        default:
            JC_MM_DEBUG_PRINT1(
                "[jc-media] Trying to query bank support for " \
                "unsupported media type %d\n",
                h->hdr.mediaType);
            JC_MM_ASSERT( FALSE );
    }

    return r;
}

/*****************************************************************************/

/**
 * Retrieve needed parameters from the header. 
 * Now we distinquish MIDI and SP-MIDI.
 */
static void doProcessHeader(ah* h, const void* buf, long buf_length) {
    const unsigned char* buffer = (const unsigned char*)buf;
    
    if (h->hdr.needProcessHeader && buffer != NULL && buf_length > 0) {
        if (buf_length >= 6 && h->hdr.mediaType == JC_FMT_MIDI) {
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
                    h->hdr.mediaType = JC_FMT_SP_MIDI;
                    break;
                }
            }
        }
        h->hdr.needProcessHeader = JAVACALL_FALSE;
    }
}

/*****************************************************************************/

/**
 * Audio basic javacall function interface
 */
static media_basic_interface _audio_qs_basic_itf = {
    audio_qs_create,
    audio_qs_get_format,
    audio_qs_get_player_controls,
    audio_qs_close,
    audio_qs_destroy,
    audio_qs_acquire_device,
    audio_qs_release_device,
    NULL,
    NULL,
    audio_qs_start,
    audio_qs_stop,
    audio_qs_pause,
    audio_qs_resume,
    audio_qs_get_java_buffer_size,
    audio_qs_set_whole_content_size,
    audio_qs_get_buffer_address,
    audio_qs_do_buffering,
    audio_qs_clear_buffer,
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
    &_audio_qs_metadata_itf,
    &_audio_qs_rate_itf,
    &_audio_qs_tempo_itf,
    &_audio_qs_pitch_itf,
    NULL,
    NULL
};

/* Limited audio interface for AMR format */
media_interface g_amr_audio_itf = {
    &_audio_qs_basic_itf,
    &_audio_qs_volume_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    &_audio_qs_rate_itf,
    NULL,
    NULL,
    NULL,
    NULL
};
