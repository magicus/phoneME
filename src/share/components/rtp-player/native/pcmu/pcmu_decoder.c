/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file pcmu_decoder.c
 * @brief Native PCMU decoder interface implementation.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native PCMU decoder.
 */

#include "mni.h"
#include "rtp.h"
#include "pcmu_decoder.h"
#include "mm2map.h"


/** 
 * Closes the PCMU decoder and releases all resources
 * associated with it.
 * 
 * @param state   Pointer to the PCMU_DECODER structure.
 */


/* #define PCMU_DEBUG */


void pcmu_close(void *state) {
    PCMU_DECODER *pcmu_decoder = (PCMU_DECODER *) state;

    MM_CloseDestroyAudioStream((MAP_AudioStream) pcmu_decoder->audio_instance,
			       MM2MAP_WAVE);

    MNI_FREE(pcmu_decoder->outBuffer);
    MNI_FREE(pcmu_decoder);
}

/** 
 *  Writes data to the audio device. This function tries
 *  to write audio data to the audio device and will
 *  return RTP_SUCCESS if called successfully. However,
 *  RTP_SUCCESS does not mean that any or all of the data
 *  has been written. It merely states that the call to the
 *  audio device returns without error. RTP_WRITE_AUDIO_FAILURE
 *  is returned when a serious error in occurred from which
 *  there is no recovery. The RTP player should then terminate
 *  this RTP session.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE.                      
 */

static int pcmu_write_audio_(RTP_MEMORY *rtp_memory, PCMU_DECODER *pcmu_decoder) {
    int written = pcmu_decoder->outBufferLength;
    MAP_Error error;

#ifdef PCMU_DEBUG
    printf("[PCMU-DECODER] out-size: %d\n", written);
#endif
    
    error = MAP_AS_Write((MAP_AudioStream) pcmu_decoder->audio_instance, 
			 pcmu_decoder->outBuffer, 
			 pcmu_decoder->outBufferOffset,
			 &written);	

    if (error != MAP_ERR_SUCCESS) {
        return RTP_WRITE_AUDIO_FAILURE;
    }  

#ifdef PCMU_DEBUG
    printf("[PCMU-DECODER] written: %d\n", written);
#endif

    pcmu_decoder->outBufferLength -= written;

    if (pcmu_decoder->outBufferLength > 0) {
        rtp_memory->audio_ready = FALSE;
	pcmu_decoder->outBufferOffset += written;
    } else {
      rtp_memory->audio_ready = TRUE;
      pcmu_decoder->outBufferOffset = 0;
    }

    return RTP_SUCCESS;
}

/** 
 *  Writes data to the audio device. This function tries
 *  to write audio data to the audio device and will
 *  return RTP_SUCCESS if called successfully. However,
 *  RTP_SUCCESS does not mean that any or all of the data
 *  has been written. It merely states that the call to the
 *  audio device returns without error. RTP_WRITE_AUDIO_FAILURE
 *  is returned when a serious error in occurred from which
 *  there is no recovery. The RTP player should then terminate
 *  this RTP session.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE.                      
 */

int pcmu_write_audio(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder) {
    PCMU_DECODER *pcmu_decoder = (PCMU_DECODER *) rtp_decoder->state;

    return pcmu_write_audio_(rtp_memory, pcmu_decoder);
}


/** 
 *  Initializes the PCMU decoder.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the PCMU decoder fails.
 *                     write call. RTP_OPEN_AUDIOSTREAM_FAILURE
 *                     indicates that the audio stream could
 *                     not be created.
 */

int pcmu_initialize(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder) {
    int input;
    int mantissa;
    int segment;
    int value;
    int i;
    int bigEndian = TRUE;
    PCMU_DECODER *pcmu_decoder;
    PCMU_DATA *table;
    MAP_AudioStream as;

    pcmu_decoder = (PCMU_DECODER *) MNI_MALLOC(sizeof(PCMU_DECODER));

    if (pcmu_decoder == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    table = &pcmu_decoder->table;


    for (i = 0; i < 256; i++) {
        input     = ~i;
        mantissa  = ((input & 0xf ) << 3) + 0x84;
        segment   = (input & 0x70) >> 4;
        value     = mantissa << segment;

        value -= 0x84;

        if ((input & 0x80) !=0) {
            value = -value;
	}

        table->lutTableL[i] = (char) value;
        table->lutTableH[i] = (char) (value >> 8);
    }

    pcmu_decoder->outBuffer = (unsigned char *) MNI_MALLOC(PCMU_OUT_BUFFER_SIZE);

    if (pcmu_decoder->outBuffer == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    pcmu_decoder->outBufferLength = PCMU_OUT_BUFFER_SIZE;
    pcmu_decoder->outBufferOffset = 0;
    pcmu_decoder->sampleRate = 8000;
    pcmu_decoder->bitsPerSample = 16;
    pcmu_decoder->channels = 1;
    pcmu_decoder->isSigned = 1; /* signed */

    rtp_decoder->state = pcmu_decoder;
    rtp_decoder->closeFn = pcmu_close;
    rtp_decoder->writeAudioFn = pcmu_write_audio;
    
    #ifdef JM_LITTLE_ENDIAN
        bigEndian= FALSE;
    #endif

    as = MM_CreateOpenAudioStream(pcmu_decoder->sampleRate,
				  pcmu_decoder->bitsPerSample,
				  pcmu_decoder->channels,
				  pcmu_decoder->isSigned,
				  bigEndian,
				  0, /* use default buffer size */
				  MM2MAP_WAVE); 

    if (as == 0) { 
#ifdef PCMU_DEBUG
        printf("[PCMU-DECODER] failed to open the audio device\n"); 
#endif
	return RTP_OPEN_AUDIOSTREAM_FAILURE;
    } else {
        MAP_AS_Start( as);
        pcmu_decoder->audio_instance = (void *) as;
        rtp_memory->audio_ready = TRUE;
    }

    return RTP_SUCCESS;
}


/** 
 *  Decodes an RTP packet containing PCMU audio data and writes
 *  the data to the audio device.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @param data         Pointer the the byte array to be decoded.
 * @param size         The size of the array in number of bytes.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE.
 */

int pcmu_decode(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder, 
		RTP_BYTE *inData, RTP_WORD count) {
  
    int outOffset = 0;
    int temp;
    int i;

    PCMU_DECODER *pcmu_decoder = (PCMU_DECODER *) rtp_decoder->state;

    PCMU_DATA *table = &pcmu_decoder->table;

      
    for (i = 0; i < count; i++) {
        temp = (unsigned char) inData[ i];

        pcmu_decoder->outBuffer[ outOffset++] = table->lutTableL[ temp];
        pcmu_decoder->outBuffer[ outOffset++] = table->lutTableH[ temp];		
    }

    pcmu_decoder->outBufferLength = outOffset;

    return pcmu_write_audio_(rtp_memory, pcmu_decoder); 
}

