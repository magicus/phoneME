/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/** 
 * @file rtcp_receiver.c
 *
 * Processing of RTCP packets.
 */


#include "rtp.h"
#include "parser_util.h"

/** 
 *  Parses an RTCP packet.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param packet       Pointer to a byte array containing an RTCP packet.
 * @param length       The length of the RTCP packet.
 * @return             Returns RTP_SUCCESS if the RTCP packet was
 *                     processed successfully. Other return values are
 *                     RTP_OUT_OF_MEMORY if not enough memory for the RTCP
 *                     packet could be allocated, RTP_INVALID version if
 *                     the the version could not be handled and RTP_BAD_FORMAT
 *                     if a malformed RTCP packet has been encountered.
 */

int rtcpr_parse_packet(RTP_MEMORY *rtp_memory, 
		       RTP_BYTE *packet, RTP_WORD packet_length) {
    RTP_BYTE  firstbyte;
    RTP_BYTE  version;
    RTP_SHORT length;
    RTP_WORD  padlen;
    RTP_WORD  packet_offset;
    RTP_WORD  inlength;
    RTP_WORD  i;
    RTP_WORD  val;
    RTP_WORD  sdesoff;
    RTP_BYTE  sdestype;
    RTP_BOOL  gotcname;
    RTP_WORD  sdeslen;
    RTP_WORD  reasonlen;
    RTCP_PACKET rtcp_packet;
    RTCP_SDES_ITEM *item, *head;

#ifdef RTP_DEBUG
    RTP_BYTE buffer[ 80];
#endif

    packet_offset = 0;

    /* reset the packet buffer */
    rtp_reset_memory(rtp_memory);

    /* reset the parser offset */
    parser_offset = 0;

    while (packet_offset < packet_length) {
        firstbyte = read_byte(packet);

        version = firstbyte >> 6;

        /* first two bits are RTP version */
        if (version != 2) {
#ifdef RTP_DEBUG
            printf("invalid rtp version.\n");
#endif
	    return RTP_INVALID_VERSION;
        }

        rtcp_packet.type = read_byte(packet);

        length = read_short(packet);

        length = (length + 1) << 2;

        padlen = 0;

        if (packet_offset + length > packet_length) {
#ifdef RTP_DEBUG
            printf("bad format - packet length.");
#endif
	    return RTP_BAD_FORMAT;
        } else if (packet_offset + length == packet_length) {
            if ((firstbyte & 0x20) != 0) {
	        padlen = packet[ packet_offset + packet_length - 1] & 0xff;

	        if (padlen == 0) {
#ifdef RTP_DEBUG
		    printf("bad format - padlen == 0.\n");
#endif
		    return RTP_BAD_FORMAT;
	        }
            }
	} else if ((firstbyte & 0x20) != 0) {
#ifdef RTP_DEBUG
	    printf("bad format exception - invalid version.\n");
#endif
	}

        inlength = length - padlen;
        firstbyte &= 0x1f;
	       
	switch (rtcp_packet.type) {
	    case RTCP_SR:
#ifdef RTP_DEBUG
                printf("SR received.\n");
#endif

                if (inlength != 28 + (24 * firstbyte)) {
#ifdef RTP_DEBUG
		    printf("malformed SR.\n");
#endif
		    return RTP_BAD_FORMAT;
		}
		
		rtcp_packet.packet.sr.ssrc = read_word(packet);
		rtcp_packet.packet.sr.ntptimestampmsw = read_word(packet);
		rtcp_packet.packet.sr.ntptimestamplsw = read_word(packet);
		rtcp_packet.packet.sr.rtptimestamp = read_word(packet);
		rtcp_packet.packet.sr.packetcount = read_word(packet);
	        rtcp_packet.packet.sr.octetcount = read_word(packet);

		rtcp_packet.packet.sr.reports = (RTCP_REPORT_BLOCK *) 
                    rtp_get_memory(rtp_memory, firstbyte * sizeof(RTCP_REPORT_BLOCK));

		if (rtcp_packet.packet.sr.reports == NULL) {
		    return RTP_OUT_OF_MEMORY;
		}
 
	        for (i = 0; i < firstbyte; i++) {
		    rtcp_packet.packet.sr.reports[ i].ssrc = read_word(packet);

		    val= read_word(packet);

		    rtcp_packet.packet.sr.reports[ i].fractionlost = (RTP_BYTE) (val >> 24);

		    rtcp_packet.packet.sr.reports[ i].packetslost = (RTP_WORD) (val & 0x00ffffffL);
			    
		    rtcp_packet.packet.sr.reports[ i].lastseq = read_word(packet);
		    rtcp_packet.packet.sr.reports[ i].jitter = read_word(packet);
		    rtcp_packet.packet.sr.reports[ i].lsr = read_word(packet);
		    rtcp_packet.packet.sr.reports[ i].dlsr = read_word(packet);
		}
	    break;
	    case RTCP_RR:
#ifdef RTP_DEBUG
                printf("RR received.\n");
#endif

 	    	if (inlength != 8 + 24 * firstbyte) {
		    /* update number of malformed RR recd */
#ifdef RTP_DEBUG
		    printf("malformed RR.\n");
#endif
		    return RTP_BAD_FORMAT;
	    	}
			    
	    	rtcp_packet.packet.rr.ssrc = read_word(packet);
		rtcp_packet.packet.rr.reports = (RTCP_REPORT_BLOCK *) 
                    rtp_get_memory(rtp_memory, firstbyte * sizeof(RTCP_REPORT_BLOCK));

		if (rtcp_packet.packet.rr.reports == NULL) {
		    return RTP_OUT_OF_MEMORY;
		}
 
		i = 0;	    
	    	while (i < firstbyte) {
	    	    rtcp_packet.packet.rr.reports[ i].ssrc = read_word(packet);
			    
	    	    val= read_word(packet);

	    	    rtcp_packet.packet.rr.reports[ i].fractionlost = (RTP_BYTE) (val >> 24);

	    	    rtcp_packet.packet.rr.reports[ i].packetslost = (RTP_WORD) (val & 0x00ffffffL);
			    
	            rtcp_packet.packet.rr.reports[ i].lastseq = read_word(packet);
	       	    rtcp_packet.packet.rr.reports[ i].jitter = read_word(packet);
	    	    rtcp_packet.packet.rr.reports[ i].lsr = read_word(packet);
	    	    rtcp_packet.packet.rr.reports[ i].dlsr = read_word(packet);

		    i++;
	    	}
	    break;
	    case RTCP_SDES:
#ifdef RTP_DEBUG
                printf("SDES received.\n");
#endif

		rtcp_packet.packet.sdes.chunks = (RTCP_SDES_CHUNK *) 
                    rtp_get_memory(rtp_memory, firstbyte * sizeof(RTCP_SDES_CHUNK));

		if (rtcp_packet.packet.sdes.chunks == NULL) {
		    return RTP_OUT_OF_MEMORY;
		}
 	        
		sdesoff = 4;

		i = 0;
		while (i < firstbyte) {
		    rtcp_packet.packet.sdes.chunks[ i].ssrc = read_word(packet);
		    sdesoff += 5;

		    gotcname = FALSE;
		    
		    head = NULL;
		      
		    while ((sdestype = read_byte(packet)) != 0) {
			item = (RTCP_SDES_ITEM *) 
                            rtp_get_memory(rtp_memory, sizeof(RTCP_SDES_ITEM));

			if (item == NULL) {
			    return RTP_OUT_OF_MEMORY;
			}
 
			item->type = sdestype;

			sdeslen = read_byte(packet);

			item->data = (RTP_BYTE *) 
                            rtp_get_memory(rtp_memory, sdeslen * sizeof(RTP_BYTE));

			if (item->data == NULL) {
			    return RTP_OUT_OF_MEMORY;
			}
 
			read_bytes(packet, item->data, sdeslen);
			sdesoff += 2 + sdeslen;

		        if (sdestype == SDES_CNAME) {
			    gotcname = TRUE;
                            
			    rtp_map_ssrc(rtp_memory, 
					 rtcp_packet.packet.sdes.chunks[ i].ssrc, 
					 item->data, sdeslen);
			}

			if (head != NULL) {
			    item->next = head;
			}

			head = item;
		    }
			    
		    if (!gotcname) {
#ifdef RTP_DEBUG
			printf("Malformed RTCP packet: no CNAME.\n");
#endif
			return RTP_MALFORMED_RTCP_PACKET;
		    }

		    rtcp_packet.packet.sdes.chunks[ i].items = head;

		    if ((sdesoff & 3) != 0) {
		    	skip_bytes(4-(sdesoff & 3));
		    	sdesoff = (sdesoff + 3) & ~3;
		    }

		    i++;
		}

		if (inlength != sdesoff) {
#ifdef RTP_DEBUG
		    printf("malformed SDES: inlength\n");
#endif
		    return RTP_BAD_FORMAT;
		}
	    break;
	    case RTCP_BYE:
                rtcp_packet.packet.bye.ssrcs = (RTP_WORD *) 
                    rtp_get_memory(rtp_memory, firstbyte * sizeof(RTP_WORD));
		
		if (rtcp_packet.packet.bye.ssrcs == NULL) {
		    return RTP_OUT_OF_MEMORY;
		}
 
		i = 0;	
		while (i < firstbyte) {
	            rtcp_packet.packet.bye.ssrcs[ i] = read_word(packet);

		    rtp_remove_ssrc(rtp_memory, rtcp_packet.packet.bye.ssrcs[ i]);

		    i++;
		}
			
		if (inlength > 4 + 4 * firstbyte) {
		    reasonlen = read_byte(packet);
		    rtcp_packet.packet.bye.reason = (RTP_BYTE *) rtp_get_memory(rtp_memory, reasonlen);

		    if (rtcp_packet.packet.bye.reason == NULL) {
		      return RTP_OUT_OF_MEMORY;
		    }
 
		    reasonlen++;
		} else {
		    reasonlen = 0;
		    rtcp_packet.packet.bye.reason = NULL;
		}

		reasonlen = (reasonlen + 3) & ~3;

		if (inlength != 4 + 4 * firstbyte + reasonlen) {
#ifdef RTP_DEBUG
		    printf("malformed BYE packet\n.");
#endif

		    return RTP_BAD_FORMAT;
		}

		read_bytes(packet, rtcp_packet.packet.bye.reason, reasonlen);
	    break;
	    case RTCP_APP:
		if (inlength < 12) {
#ifdef RTP_DEBUG
		    printf("malformed APP packet.\n");
#endif

		    return RTP_BAD_FORMAT;
		}
			
		rtcp_packet.packet.app.ssrc = read_word(packet);
		rtcp_packet.packet.app.name = read_word(packet);
                rtcp_packet.packet.app.subtype = firstbyte;
		rtcp_packet.packet.app.data = (RTP_BYTE *) rtp_get_memory(rtp_memory, inlength - 12);

		if (rtcp_packet.packet.app.data == NULL) {
		    return RTP_OUT_OF_MEMORY;
		}
 
		read_bytes(packet, rtcp_packet.packet.app.data, inlength - 12);
	    break;
	    default:
#ifdef RTP_DEBUG
		printf("Unknown packet type.\n");
#endif

		return RTP_UNKNOWN_PACKET_TYPE;
	    break;
	}
    
        packet_offset += length;
    }

    return RTP_SUCCESS;
}
