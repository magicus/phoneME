/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file demux.c
 *
 * Decoding of RTP payloads.
 */

#include <stdio.h>
#include <stdlib.h>
#include "rtp.h"   

#ifdef RTP_PCMU
#include "pcmu_decoder.h"
#endif

#ifdef RTP_GSM
#include "gsm_decoder.h"
#endif

#ifdef RTP_MPA
#include "mpa_decoder.h"
#endif

#ifdef RTP_JPEG
#include "jpeg_decoder_rtp.h"
#endif

#ifdef RTP_H263
#include "h263_decoder.h"
#endif

/**
 * the payload table. Payload types 0 - 95 are static payloads while 
 * dynamic payload types may be allocated in the range of 96 - 127.
 */

#ifdef RTP_NOT_USED /* disabled for now, mo */
    PAYLOAD payload[ 128]= { /* 0-10 */
                         "PCMU", AUDIO,
                         "1016", AUDIO,
			 "G721", AUDIO,
			 "GSM",  AUDIO,
			 "",     NA,
			 "DVI4", AUDIO,
			 "DVI4", AUDIO,
			 "LPC",  AUDIO,
			 "PCMA", AUDIO,
			 "G722", AUDIO,
			 "L16",  AUDIO,
			 
			 /* 11-20 */

			 "L16",  AUDIO,
			 "",     NA,
			 "",     NA,
			 "MPA",  AUDIO,
			 "G728", AUDIO,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 21-30 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "CeIB", VIDEO,
			 "JPEG", VIDEO,
			 "",     NA,
			 "nv",   VIDEO,
			 "",     NA,
			 "",     NA,

			 /* 31-40 */
			 "H261", VIDEO,
			 "MPV",  VIDEO,
			 "MP2T", AUDIO_VIDEO,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 41-50 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 51-60 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 61-70 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 71-80 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 81-90 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 91-100 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 101-110 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 111-120 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,

			 /* 121-127 */
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA,
			 "",     NA
		       };


static BYTE get_format(BYTE payload_type) {
    printf("get_format for %d\n", payload_type);

    return payload[ payload_type].type;
}

#endif	




/** 
 * Decodes the RTP payload according to its payload type.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_packet   Pointer to the RTP_PACKET structure.
 * @return             Returns RTP_SUCCESS if the payload is
 *                     decoded successfully. RTP_OUT_OF_MEMORY
 *                     indicates that the respective decoder
 *                     memory could not be allocated. An 
 *                     RTP_DECODER_FAILURE is returned if the
 *                     decoding of the payload fails.
 */

int rtp_demux_payload(RTP_MEMORY *rtp_memory, RTP_PACKET *rtp_packet) {
    int ret = RTP_SUCCESS;

    /* BYTE format = get_format(rtp_packet->payload_type); */

    RTP_SSRC *ssrc_ptr = rtp_get_ssrc(rtp_memory, rtp_packet->ssrc);

#ifdef RTP_DEBUG    
    printf("pt: %d\n", rtp_packet->payload_type);
#endif
		
    if (ssrc_ptr->decoder == NULL) {
#ifdef RTP_DEBUG      
        printf("new decoder for ssrc #%d\n", ssrc_ptr->ssrc);
#endif
        rtp_add_decoder(rtp_memory, ssrc_ptr, rtp_packet->payload_type);
	    
        switch (rtp_packet->payload_type) {
#ifdef RTP_PCMU
	    case RTP_PT_PCMU: /* PCMU */	      
	        ret = pcmu_initialize(rtp_memory, ssrc_ptr->decoder);
		break;
#endif

#ifdef RTP_GSM
    	    case RTP_PT_GSM: /* GSM */
                ret = gsm_initialize(rtp_memory, ssrc_ptr->decoder);
		break;
#endif

#ifdef RTP_MPA
	    case RTP_PT_MPA: /* MPEG-Audio */
	        ret = mpa_initialize(rtp_memory,
			             ssrc_ptr->decoder,
			             rtp_packet->payload,
				     rtp_packet->payloadlength);
		break;
#endif

#ifdef RTP_JPEG
	    case RTP_PT_JPEG: /* JPEG */
	        ret = jpeg_initialize(ssrc_ptr->decoder);
		break;
#endif

#ifdef RTP_H263
	    case RTP_PT_H263: /* H.263 */	      
	        ret = h263_initialize(ssrc_ptr->decoder);	      
		break;
#endif

	    default: 
	        /* payload type not handled */
		ret = RTP_UNKNOWN_PAYLOAD_TYPE;
		break;
	}    
    }

    if (ret == RTP_SUCCESS) {
        switch (rtp_packet->payload_type) {
#ifdef RTP_PCMU
            case RTP_PT_PCMU: /* PCMU */
	         ret = pcmu_decode(rtp_memory, ssrc_ptr->decoder,
				   rtp_packet->payload, rtp_packet->payloadlength);
		 break;
#endif

#ifdef RTP_GSM
            case RTP_PT_GSM: /* GSM */
	      
	        ret = gsm_decode(rtp_memory, ssrc_ptr->decoder,
				 rtp_packet->payload, rtp_packet->payloadlength);	      
		break;
#endif


#ifdef RTP_MPA
            case RTP_PT_MPA: /* MPEG-Audio */
                ret = mpa_decode(rtp_memory, ssrc_ptr->decoder, 
				 rtp_packet->payload, rtp_packet->payloadlength);
		break;
#endif


#ifdef RTP_JPEG
	    case RTP_PT_JPEG: /* JPEG */
	         ret = jpeg_decode(rtp_memory, 
				   ssrc_ptr->decoder, 
				   rtp_packet);
		 break;
#endif


#ifdef RTP_H263
	    case RTP_PT_H263: /* H.263 */	      
	         ret = h263_decode(rtp_memory, 
				   ssrc_ptr->decoder, 
				   rtp_packet);
	      
		 break;
#endif

            default: 
	        /* printf("Payload type not handled: %d\n", rtp_packet->payload_type); */
		ret = RTP_UNKNOWN_PAYLOAD_TYPE;
		break;
	}
    }

    return ret;
}

/** 
 * Writes audio data to the decoder's audio output.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE.  
 */

int rtp_write_audio(RTP_MEMORY *rtp_memory) {
    RTP_SSRC *ssrc = rtp_memory->ssrc_table;
    RTP_DECODER *decoder;
    int ret;

    while (ssrc != NULL) {
        decoder = ssrc->decoder;

        if (decoder->writeAudioFn != NULL) {
	    if ((ret = decoder->writeAudioFn(rtp_memory, decoder)) != RTP_SUCCESS) {
	        return ret;
	    }
	}

        ssrc = ssrc->next;
    }

    return RTP_SUCCESS;
}

/* disabled for now, mo
void add_dynamic_payload_type(BYTE *name, BYTE payload_type) {
    if (payload_type > 95) {
        if (payload[ payload_type].type == NA) {
	    payload[ payload_type].name= (char *)name;
	    payload[ payload_type].type= payload_type;
	} else {
	    #ifdef DEBUG
	    write_debug("dynamic payload type already assigned.\n");
	    #endif
	}
    } else {
        #ifdef DEBUG
        write_debug("Invalid dynamic payload type [expected range 96-127].\n");
	#endif
    }
}

void remove_dynamic_payload_type(BYTE payload_type) {
    if (payload_type > 95) {
        payload[ payload_type].name= "";
	payload[ payload_type].type= NA;
    }
}
*/
