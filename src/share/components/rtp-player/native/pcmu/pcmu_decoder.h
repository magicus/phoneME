/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/** 
 * @file pcmu_decoder.h
 * @brief Native PCMU decoder interface definitions.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native PCMU decoder.
 */

/** 
 * @def PCMU_OUT_BUFFER_SIZE
 * @brief The size of the GSM output buffer.
 */

#define PCMU_OUT_BUFFER_SIZE 2000

/**
 * @struct  PCMU_DATA_.
 * @brief   Definition of the PCMU_DATA_ structure.
 */

/**
 * @var     typedef struct PCMU_DATA.
 * @brief   Type definition of the PCMU look-up table.
 */

typedef struct PCMU_DATA_ {
    char lutTableL[ 256];
    char lutTableH[ 256];
} PCMU_DATA;

/**
 * @struct  PCMU_DECODER_.
 * @brief   Definition of the PCMU_DECODER_ structure.
 */

/**
 * @var     typedef struct PCMU_DECODER.
 * @brief   Type definition of the PCMU_DECODER variable.
 */

typedef struct PCMU_DECODER_ {
    unsigned char *outBuffer; /**< Output byte array. */
    int outBufferLength; /**< Output byte array length. */
    int outBufferOffset; /**< Offset into output byte array */
    PCMU_DATA table; /**< The PCMU look-up table */
    void *audio_instance; /**< The audio device instance associanted with this codec. */
    int sampleRate; /**< the sample rate. */
    int bitsPerSample; /**< bits per channel. */
    int channels; /**< number of channels. */
    int isSigned; /**< 1: signed, 0: unsigned. */
} PCMU_DECODER;


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

int pcmu_initialize(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder); 


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

int pcmu_write_audio(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder);


/** 
 *  Closes the PCMU decoder and releases all resources
 *  associated with it.
 * 
 * @param state   Pointer to the PCMU_DECODER structure.
 */

void pcmu_close(void *state);
