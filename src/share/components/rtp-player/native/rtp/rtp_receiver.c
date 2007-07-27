/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file rtp_receiver.c
 *
 * Processing of RTP packets.
 */


#include <string.h>
#include "rtp.h"
#include "parser_util.h"


/** 
 *  Parses an RTP packet.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param packet       Pointer to a byte array containing an RTP packet.
 * @param length       The length of the RTP packet.
 * @return             Returns RTP_SUCCESS if the RTP packet was
 *                     processed successfully. Other return values are
 *                     RTP_OUT_OF_MEMORY if not enough memory for the RTP
 *                     packet could be allocated, RTP_INVALID_VERSION if
 *                     the the version could not be handled and RTP_BAD_FORMAT
 *                     if a malformed RTP packet has been encountered.
 */

int rtpr_parse_packet(RTP_MEMORY *rtp_memory,
		      RTP_BYTE *packet, RTP_WORD length) {
    RTP_PACKET *rtp_packet;
    RTP_BYTE firstbyte;
    RTP_WORD i, off, padlen = 0;

    /* reset the RTP buffer */
    rtp_reset_memory(rtp_memory);

    /* allocate data structure for a new RTP packet */
    rtp_packet = (RTP_PACKET *) rtp_get_memory(rtp_memory, sizeof(RTP_PACKET));

    if (rtp_packet == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    rtp_packet->extension = NULL;
    rtp_packet->csrc = NULL;
    rtp_packet->payload = NULL;
    rtp_packet->extension_present = FALSE;

    /* reset the parser offset */
    parser_offset = 0;

    firstbyte = read_byte(packet);

    /* first two bits are RTP version */
    if ((firstbyte & 0xc0) != 0x80) {
#ifdef RTP_DEBUG
        printf("invalid rtp version.\n");
#endif
	return RTP_INVALID_VERSION;
    }

    if ((firstbyte & 0x10) != 0) {
        rtp_packet->extension_present = TRUE;
    }

    if ((firstbyte & 0x20) != 0) {
        padlen = packet[ length - 1] & 0xff;
    }

    /* remainder of first byte is CSRC count */
    firstbyte &= 0xf;

    rtp_packet->payload_type = read_byte(packet);

    rtp_packet->marker = rtp_packet->payload_type >> 7;

    rtp_packet->payload_type &= 0x7f;

    rtp_packet->seqnum = read_short(packet);
    
    rtp_packet->timestamp = read_word(packet);
    rtp_packet->ssrc = read_word(packet);

    /* end of fixed header */

    /* optional fixed header extension */

    off = 0;

    if (rtp_packet->extension_present) {
        rtp_packet->extension_type = read_short(packet);
	rtp_packet->extlen = read_short(packet);
	rtp_packet->extlen <<= 2;

	if (rtp_packet->extlen > 0) {
	    rtp_packet->extension = (RTP_BYTE *)rtp_get_memory(rtp_memory, rtp_packet->extlen);
	    read_bytes(packet, rtp_packet->extension, rtp_packet->extlen);

	    off += rtp_packet->extlen + 4;
	}
    }

    /* beginning of variable header */

    if (firstbyte > 0) {
        rtp_packet->csrc = (RTP_WORD *) rtp_get_memory(rtp_memory, firstbyte * sizeof(RTP_WORD));

        for (i = 0; i < firstbyte; i++) {
            rtp_packet->csrc[ i] = read_word(packet);
        }
    }

    off += (12 + (sizeof(rtp_packet->csrc) << 2));
    rtp_packet->payloadlength = length - (parser_offset + padlen);

    if (rtp_packet->payloadlength < 1) {
#ifdef RTP_DEBUG
	printf("bad format exception.\n");
#endif
	return RTP_BAD_FORMAT;
    }

    /* end of variable header */

    rtp_packet->payload = (RTP_BYTE *)rtp_get_memory(rtp_memory, rtp_packet->payloadlength);

    read_bytes(packet, rtp_packet->payload, rtp_packet->payloadlength);

    /* 
     * We're done parsing the rtp header and payload
     * send it to the Demultiplexer.
     */

    return rtp_demux_payload(rtp_memory, rtp_packet);
}


