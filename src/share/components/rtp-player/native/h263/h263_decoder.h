/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file h263_decoder.h
 *
 * @brief Native H.263 decoder interface definitions.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native H.263 decoder.
 */


#include <stdlib.h>
#include <string.h>
#include "dllindex.h"
#include "vvenc.h"
#include "h261defs.h"

#ifdef CDC
#include "jni-util.h"
#endif

/**
 * @struct H263_DECODER_.
 * @brief Definition of the H263_DECODER_ structure.
 */

/**
 * @var typedef struct H263_DECODER.
 * @brief Type definition of the H263_DECODER variable.
 */

typedef struct H263_DECODER_ {    
    unsigned char *yuvBuffer;   /**< The YUV data produced by the codec. */
    int yuvBufferLength;        /**< The YUV buffer length. */
    unsigned int *rgbBuffer32;  /**< A buffer for 32-bit RGB output. */
    int rgbBuffer32Length;      /**< Buffer length of 32-bit RGB output. */
    unsigned char *rgbBuffer;   /**< Buffer for 24-bit RGB output. */
    int rgbBufferLength;        /**< Buffer length of 24-bit RGB output. */
    int width;                  /**< Frame width in pixels. */
    int height;                 /**< Frame height in pixels. */
    int headerLength;           /**< The length of the payload header in bytes. */

    /* internal state variables */
    PICTURE_DESCR *pictureDesc; /**< H.263 internal state variable */
    H261Decoder *decoder;       /**< H.263 internal state variable */
    int PBFrameCap;             /**< H.263 internal state variable */
    int bsStart;                /**< H.263 internal state variable */
    int nextGOB;                /**< H.263 internal state variable */
} H263_DECODER;

/** 
 * Initializes the H.263 decoder.
 *
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the H.263 decoder failed.
 */

int h263_initialize(RTP_DECODER *rtp_decoder);


/** 
 * Decodes an RTP packet containing H.263 data.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @param rtp_packet   Pointer to the RTP_PACKET structure.
 * @return             Returns RTP_SUCCESS if the packet was
 *                     decoded successfully, RTP_DECODER_FAILURE
 *                     otherwise.
 */

int h263_decode(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder, RTP_PACKET *rtp_packet);


/** @todo Needs documentation */
void initNativeDecoderClass();


/** @todo Needs documentation */
H263_DECODER * initNativeDecoder( int width, int height);

/** @todo Needs documentation */
void closeNativeDecoder(H263_DECODER *);

/** @todo Needs documentation */
int decodePacketNative(H263_DECODER *h263_decoder,
		       unsigned char *inputData,
		       int inputOffset,
		       int inputLength,
		       unsigned char *outputData,
		       unsigned char *packetHeader,
		       int packetOffset,
		       int sync,
		       int h263_1998);
