/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/**
 * @file jpeg_decoder.c
 * @brief Native JPEG decoder interface implementation.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native JPEG decoder.
 */

#include "rtp.h"
#include "jpeg_decoder_rtp.h"

/**
 * Gets the fragment offset from the JPEG header.
 *
 * @param data   pointer to the JPEG header
 * @param rtp_decoder  data offset
 */

int jpeg_getFragOffset(RTP_BYTE *data, int doff) {
    // Fragment offset is the 2nd, 3rd & 4th byte of the JPEG Hdr.
    int foff = 0;

    foff |= data[doff + 1] << 16;
    foff |= data[doff + 2] << 8;
    foff |= data[doff + 3];
    
    return foff;
}

/**
 * Depacketizes an RTP packet containing JPEG payload data.
 *
 * @param rtp_memory   pointer to the RTP_MEMORY structure
 * @param rtp_decoder  pointer to the RTP_DECODER structure
 * @param packet  pointer to the RTP_PACKET structure
 *
 * @return BUFFER_PROCESSED_OK, if the packet was depacketized
 * successfully,OUTPUT_BUFFER_NOT_FILLED otherwise.
 */
static int jpeg_depacketize(RTP_MEMORY *rtp_memory, 
			    RTP_DECODER *rtp_decoder, RTP_PACKET *packet) {
    JPEG_DEPACKETIZER *depacketizer = (JPEG_DEPACKETIZER *)rtp_decoder->state;
    
    if (jpeg_getFragOffset(packet->payload, 0) == 0) {
	jpeq_depacketizer_reset(depacketizer, packet->payload,
	                         packet->payloadlength, packet->seqnum);
    } else if (depacketizer->gotFirstPacket) {
	// This is a new packet for the current frame.
	jpeg_depacketizer_add(depacketizer,
	                       packet->payload, packet->payloadlength, 0);
	// printf("additional data\n");
    } else {
	// If we don't have a current frame, then we are missing the
	// first packet for this frame.  We'll discard the current packet.
	return OUTPUT_BUFFER_NOT_FILLED;
    }

    // If this is the last packet from this frame, we need to
    // check if all the packets from this frame has been received.
    if (packet->marker != 0) {
	depacketizer->gotFirstPacket = FALSE;

	if (jpeg_gotAllPackets(depacketizer, packet->seqnum)) {            
	    jpeg_completeTransfer(rtp_memory, depacketizer,
				  packet->payload,
				  packet->payloadlength);

	    return BUFFER_PROCESSED_OK;
	} else {
	    return OUTPUT_BUFFER_NOT_FILLED;
	}
    }

    return OUTPUT_BUFFER_NOT_FILLED;
}

/**
 * Closes the JPEG decoder and releases all resources
 * associated with it.
 * 
 * @param state   Pointer to the JPEG_DEPACKETIZER structure.
 */
void jpeg_close(void *state) {
    JPEG_DEPACKETIZER *depacketizer = (JPEG_DEPACKETIZER *)state;

    jpeg_depacketizer_close(depacketizer);
}


/** 
 * Initializes the JPEG decoder.
 *
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the JPEG decoder failed.
 */
int jpeg_initialize(RTP_DECODER *rtp_decoder) {
    rtp_decoder->state = (void *)jpeq_depacketizer_init();
    rtp_decoder->closeFn = jpeg_close;

    return RTP_SUCCESS;
}

/**
 * Decodes an RTP packet containing JPEG data.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @param rtp_packet   Pointer to the RTP_PACKET structure.
 * @return             Returns RTP_SUCCESS if the packet was
 *                     decoded successfully, RTP_DECODER_FAILURE
 *                     otherwise.
 */
int jpeg_decode(RTP_MEMORY *rtp_memory,
		RTP_DECODER *decoder, RTP_PACKET *packet) {
                      
    if (jpeg_depacketize(rtp_memory, decoder, packet) == BUFFER_PROCESSED_OK) {
        rtp_memory->frame_ready = TRUE;
    }

    return RTP_SUCCESS;
}
