/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/**
 * @file h263_decoder.c
 * @brief Native H.263 decoder interface implementation.
 *  
 * This file represents the interface between the MM RTP stack
 * and the native H.263 decoder.
 */

#include "mni.h"

#include "rtp.h"
#include "dllindex.h"
#include "vvenc.h"
#include "h263_decoder.h"
/**
 * @var int widths[]
 *
 * Integer array containing well-known widths of H.263 frames.
 */

static int widths[] = {0, 128, 176, 352, 704, 1408,0,0};

/**
 * @var int heights[]
 *
 * Integer array containing well-known heights of H.263 frames.
 */

static int heights[] = {0, 96, 144, 288, 576, 1152,0,0};

/** 
 * @def DEFAULT_WIDTH
 *
 * Defines the default width of an H.263 frame.
 */

#define DEFAULT_WIDTH 176

/** 
 * @def DEFAULT_HEIGHT
 *
 * Defines the default height of an H.263 frame.
 */

#define DEFAULT_HEIGHT 144


/**
 * Returns the RTP payload header length.
 *
 * @param input        Pointer to the RTP payload header.
 * @return             The payload header length in bytes.
 */

static int getPayloadHeaderLength(RTP_BYTE *input);


/** 
 * Initializes the H.263 decoder.
 *
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the initialization
 *                     succeeded or RTP_OUT_OF_MEMORY if the
 *                     memory allocation for the H.263 decoder failed.
 */

int h263_initialize(RTP_DECODER *rtp_decoder) {
    printf("h263_initialize\n");
    initNativeDecoderClass();

    rtp_decoder->state = (void *)initNativeDecoder(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    if (rtp_decoder->state == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    return RTP_SUCCESS;
}


/**
 * Closes the H.263 decoder and releases all resources
 * associated with it.
 * 
 * @param state   Pointer to the H263_DECODER structure.
 */

void h263_close(void *state) {
    H263_DECODER *h263_decoder;

    printf("h263_close\n");

    h263_decoder = (H263_DECODER *) state;

    closeNativeDecoder(h263_decoder);
}


/**
 *  Allocates the H.263 decoder buffers.
 *
 *  This function checks the H.263 decoder buffers and allocates 
 *  enough space to decode a complete frame. In case the frame size
 *  changes mid-stream it will detect this and re-allocate the
 *  buffers to fit the new frame size.
 *
 * @param state   Pointer to the H263_DECODER structure.
 * @return        Returns RTP_SUCCESS if the decoder buffers
 *                were allocated successfully, RTP_OUT_OF_MEMORY
 *                otherwise.
 */

static int validateArrays(H263_DECODER *decoder) {
    int frameSize = decoder->width * decoder->height;
    int yuvSize = frameSize + (frameSize >> 1);

    if (decoder->yuvBuffer == NULL) {
        decoder->yuvBuffer = (unsigned char *) MNI_MALLOC(yuvSize);

	if (decoder->yuvBuffer == NULL) {
 	    return RTP_OUT_OF_MEMORY;
	}

        decoder->yuvBufferLength = yuvSize;

        decoder->rgbBuffer32 = (unsigned int *) MNI_MALLOC(frameSize * sizeof(unsigned int));

	if (decoder->rgbBuffer32 == NULL) {
 	    return RTP_OUT_OF_MEMORY;
	}

        decoder->rgbBuffer32Length = frameSize;

        decoder->rgbBuffer = (unsigned char *) MNI_MALLOC(frameSize * 3);

	if (decoder->rgbBuffer == NULL) {
 	    return RTP_OUT_OF_MEMORY;
	}

        decoder->rgbBufferLength = frameSize * 3;
    } else {
        if (frameSize > decoder->rgbBuffer32Length) {
            decoder->yuvBuffer = (unsigned char *) realloc(decoder->yuvBuffer, yuvSize);

	    if (decoder->yuvBuffer == NULL) {
	        return RTP_OUT_OF_MEMORY;
	    }

            decoder->yuvBufferLength = yuvSize;

            decoder->rgbBuffer32 = (unsigned int *) realloc(decoder->rgbBuffer32,
                                   frameSize * sizeof(unsigned int));

	    if (decoder->rgbBuffer32 == NULL) {
	        return RTP_OUT_OF_MEMORY;
	    }

            decoder->rgbBuffer32Length = frameSize;

            decoder->rgbBuffer = (unsigned char *) realloc(decoder->rgbBuffer,
                                 frameSize * 3);

	    if (decoder->rgbBuffer == NULL) {
	        return RTP_OUT_OF_MEMORY;
	    }

            decoder->rgbBufferLength = frameSize * 3;
        }
    }

    return RTP_SUCCESS;
}

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

int h263_decode(RTP_MEMORY *rtp_memory,
                RTP_DECODER *rtp_decoder, RTP_PACKET *packet) {
    H263_DECODER *h263_decoder = (H263_DECODER *) rtp_decoder->state;
#ifdef ZAURUS
    int x_off, y_off;
    int landscape;
#endif

    printf("h263_decode\n");

    if (h263_decoder->width == 0) {
        printf("a\n");
        h263_decoder->headerLength = getPayloadHeaderLength(packet->payload);

        /* check for I-Frame */
        if ((packet->payload[ 1] & 0x10) == 0x00) {
            printf("first I-Frame\n");

            if ((packet->payload[h263_decoder->headerLength] == 0)
                    && (packet->payload[h263_decoder->headerLength+1] == 0)
                    && ((packet->payload[h263_decoder->headerLength+2] & 0xfc) == 0x80)) {
                int s = (packet->payload[h263_decoder->headerLength+4] >> 2) & 0x7;

                h263_decoder->width = widths[s];
                h263_decoder->height = heights[s];

                printf("width: %d\n", h263_decoder->width);
                printf("height: %d\n", h263_decoder->height);
                

                validateArrays(h263_decoder);
            }
        }
    }


    if (h263_decoder->width != 0) {    
        if (decodePacketNative(h263_decoder,
			       packet->payload,
			       h263_decoder->headerLength,
			       packet->payloadlength,
			       h263_decoder->yuvBuffer,
			       packet->payload,
			       0,packet->marker,0)) {
	  
	    rtp_memory->frame_ready = TRUE;

#ifdef ZAURUS
        if (h263_decoder->width > 240) {
            landscape = 1;
	    /* in landscape mode */
	    x_off = (int) ((240 - h263_decoder->height) / 2);
	    y_off = (int) ((320 - h263_decoder->width) / 2);        
	} else {
	    landscape = 0;
	    x_off = (int) ((240 - h263_decoder->width) / 2);
	    y_off = (int) ((320 - h263_decoder->height) / 2);
	}

            render(h263_decoder->rgbBuffer, h263_decoder->width, 
		   h263_decoder->height, x_off + 3, y_off - 22, landscape);
#endif 
	} 
    }
    
    printf("h263_decode done\n");

    return RTP_SUCCESS;
}


/**
 * Returns the RTP payload header length.
 *
 * @param input        Pointer to the RTP payload header.
 * @return             The payload header length in bytes.
 */

static int getPayloadHeaderLength(RTP_BYTE *input) {
    int l = 0;
    RTP_BYTE b = input[0];

    if ((b & 0x80) != 0) { /* mode B or C */
        if ((b & 0x40) != 0) { /* mode C */
            l = 12;
	} else { /* mode B */
            l = 8;
	}
    } else { /* mode A */
        l = 4;
    }

    return l;
}
