/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file gsm_decoder.h
 * @brief Native GSM decoder interface definitions.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native GSM decoder.
 */

/** 
 * @def GSM_OUT_BUFFER_SIZE
 * @brief The size of the GSM output buffer.
 */

#define GSM_OUT_BUFFER_SIZE 2000


/**
 * the GSM debug flag
 *
 * #define GSM_DEBUG
 */


/**
 * @struct  GSM_DECODER_.
 * @brief   Definition of the GSM_DECODER_ structure.
 */

/**
 * @var     typedef struct GSM_DECODER.
 * @brief   Type definition of the GSM_DECODER variable.
 */

typedef struct GSM_DECODER_ {
    unsigned char *outBuffer; /**< Output byte array. */
    int outBufferLength; /**< Output byte array length. */
    int outBufferOffset; /**< Offset into output byte array */
    void *state; /**< Pointer to the decoder state. */    
    void *audio_instance; /**< The audio device instance associanted with this codec. */
    int sampleRate; /**< the sample rate. */
    int bitsPerSample; /**< bits per channel. */
    int channels; /**< number of channels. */
    int isSigned; /**< 1: signed, 0: unsigned. */
} GSM_DECODER;


/** 
 *  Initializes the GSM decoder.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the GSM decoder fails.
 *                     RTP_OPEN_AUDIOSTREAM_FAILURE indicates that 
 *                     the audio stream could not be created.
 */

int gsm_initialize(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder); 


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
		RTP_BYTE *data, RTP_WORD size);


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

int gsm_write_audio(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder);


/** 
 *  Closes the GSM decoder and releases all resources
 *  associated with it.
 * 
 * @param state   Pointer to the GSM_DECODER structure.
 */

void gsm_close(void *state);
