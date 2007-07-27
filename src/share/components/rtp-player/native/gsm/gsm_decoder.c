/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
/** 
 * @file gsm_decoder.c
 * @brief Native GSM decoder interface implementation.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native GSM decoder.
 */

#include "mni.h"
#include "rtp.h"
#include "mm2map.h"
#include "gsm_api.h"
#include "gsm_decoder.h"

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

static int gsm_write_audio_(RTP_MEMORY *rtp_memory, GSM_DECODER *gsm_decoder) {
    int written = gsm_decoder->outBufferLength;
    MAP_Error error;

#ifdef GSM_DEBUG
    printf("[GSM-DECODER] out-size: %d\n", written);
#endif
    
    error = MAP_AS_Write((MAP_AudioStream) gsm_decoder->audio_instance, 
			 gsm_decoder->outBuffer, 
			 gsm_decoder->outBufferOffset,
			 &written);	

    if (error != MAP_ERR_SUCCESS) {
        return RTP_WRITE_AUDIO_FAILURE;
    }  
       
    gsm_decoder->outBufferLength -= written;

    if (gsm_decoder->outBufferLength > 0) {
        rtp_memory->audio_ready = FALSE;
        gsm_decoder->outBufferOffset += written;
    } else {
        rtp_memory->audio_ready = TRUE;
        gsm_decoder->outBufferOffset = 0;
    }
    
    return RTP_SUCCESS;
}


/** 
 *  Closes the GSM decoder and releases all resources
 *  associated with it.
 * 
 * @param state   Pointer to the GSM_DECODER structure.
 */

void gsm_close(void *state) {
    GSM_DECODER *gsm_decoder = (GSM_DECODER *) state;

    MM_CloseDestroyAudioStream((MAP_AudioStream) gsm_decoder->audio_instance,
			       MM2MAP_WAVE);

    MNI_FREE(gsm_decoder->outBuffer);
    MNI_FREE(gsm_decoder->state);
    MNI_FREE(gsm_decoder);
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

int gsm_write_audio(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder) {
    GSM_DECODER *gsm_decoder = (GSM_DECODER *) rtp_decoder->state;

    return gsm_write_audio_(rtp_memory, gsm_decoder);
}


/** 
 *  Initializes the GSM decoder.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the GSM decoder fails.
 *                     write call. RTP_OPEN_AUDIOSTREAM_FAILURE
 *                     indicates that the audio stream could
 *                     not be created.
 */

int gsm_initialize(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder) {
    int bigEndian = TRUE;
    MAP_AudioStream as;

    GSM_DECODER *gsm_decoder = (GSM_DECODER *) MNI_MALLOC(sizeof(GSM_DECODER));

    if (gsm_decoder == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    if (gsm_decoder_open(&gsm_decoder->state) != GSM_OK) {
#ifdef GSM_DEBUG      
        printf("[GSM-DECODER] cannot initialize decoder memory.\n");
#endif
	return RTP_OUT_OF_MEMORY;
    }

    gsm_decoder->outBuffer = (unsigned char *) MNI_MALLOC(GSM_OUT_BUFFER_SIZE);

    if (gsm_decoder->outBuffer == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    gsm_decoder->outBufferLength = GSM_OUT_BUFFER_SIZE;
    gsm_decoder->outBufferOffset = 0;
    gsm_decoder->sampleRate = 8000;
    gsm_decoder->bitsPerSample = 16;
    gsm_decoder->channels = 1;
    gsm_decoder->isSigned = 1; /* signed */

    rtp_decoder->state = gsm_decoder;
    rtp_decoder->closeFn = gsm_close;
    rtp_decoder->writeAudioFn = gsm_write_audio;


    #ifdef JM_LITTLE_ENDIAN
        bigEndian= FALSE;
    #endif
    
    as = MM_CreateOpenAudioStream(gsm_decoder->sampleRate,
				  gsm_decoder->bitsPerSample,
				  gsm_decoder->channels,
				  gsm_decoder->isSigned,
				  bigEndian,
				  0, /* use default buffer size */
				  MM2MAP_WAVE);
    
    if (as == 0) { 
#ifdef GSM_DEBUG
        MAP_AS_Start( as);
        gsm_decoder->audio_instance = (void *) as;
        printf("[GSM-DECODER] failed to open the audio device\n");
#endif
	return RTP_OPEN_AUDIOSTREAM_FAILURE;
    } else {
        rtp_memory->audio_ready = TRUE;
    }

    return RTP_SUCCESS;
}


/** 
 *  Decodes an RTP packet containing GSM audio data and writes
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

int gsm_decode(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder, 
		RTP_BYTE *data, RTP_WORD size) {
    int i;
    int inOffset;
    int outOffset = 0;
    short outShort[ 160];
    
    GSM_DECODER *gsm_decoder = (GSM_DECODER *) rtp_decoder->state;
    
    for (inOffset = 0; inOffset < size; inOffset+= 33) {	
        gsm_decode_frame(gsm_decoder->state, data + inOffset, 
			 outShort, GSM_REGULAR_FRAME);
      
        for (i = 0; i < 160; i++) {
            gsm_decoder->outBuffer[ outOffset++]= (signed char)(outShort[ i] & 0x00ff);
            gsm_decoder->outBuffer[ outOffset++]= (signed char)(outShort[ i] >> 8);
	}	
    }

    gsm_decoder->outBufferLength = outOffset;

    return gsm_write_audio_(rtp_memory, gsm_decoder);
}
