/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file jpeg_decoder_rtp.h
 * @brief Native JPEG decoder interface implementation.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native JPEG decoder.
 */

#include "jpeg_depacketizer.h"

/** 
 * Initializes the JPEG decoder.
 *
 * @param rtp_decoder  pointer to RTP_DECODER
 * @return             returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the JPEG decoder fails
 */

int jpeg_initialize(RTP_DECODER *rtp_decoder);


/** 
 * Decodes an RTP packet containing JPEG data.
 *
 * @param rtp_memory   pointer to RTP_MEMORY
 * @param rtp_decoder  pointer to RTP_DECODER.
 * @param rtp_packet   pointer to RTP_PACKET.
 * @return             RTP_SUCCESS if the decoding of the RTP
 *                     packet succeeded, RTP_DECODER_FAILURE otherwise
 *
 * @todo Needs to check for RTP_DECODER_FAILURE.
 */

int jpeg_decode(RTP_MEMORY *rtp_memory, RTP_DECODER *rtp_decoder, RTP_PACKET *rtp_packet);



