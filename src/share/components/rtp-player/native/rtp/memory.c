/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file memory.c
 * RTP memory allocation & management.
 */

#include "mni.h"
#include <string.h>
#include "rtp.h"

/** 
 *  Checks the SSRC table whether the specified SSRC already exists.
 *
 * @param ssrc         The SSRC to be checked.
 * @return             Returns TRUE if the SSRC already exists, otherwise
 *                     FALSE.
 */

static RTP_BOOL ssrc_exists(RTP_MEMORY *rtp_memory, RTP_WORD ssrc);


/** @todo Needs documentation. */
static void shift_data_memory(RTP_BYTE *rtp_memory, RTP_WORD amount);

/** @todo Needs documentation. */
static void adjust_pointers(RTP_SSRC *ssrc, RTP_BYTE *rtp_memory, RTP_WORD amount);

/** 
 *  Initializes RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @return             Returns RTP_SUCCESS if the could be 
 *                     allocated, RTP_OUT_OF_MEMORY otherwise.
 */

int rtp_init_memory( RTP_MEMORY **rtp_memory) {
    (*rtp_memory) = (RTP_MEMORY *) MNI_MALLOC(sizeof(RTP_MEMORY));

    if (*rtp_memory == NULL) {
        return RTP_OUT_OF_MEMORY;
    }

    (*rtp_memory)->buffer_offset = 0;
    (*rtp_memory)->buffer_size = RTP_MIN_BUFFER_SIZE;
    (*rtp_memory)->buffer = (RTP_BYTE *) MNI_MALLOC(RTP_MIN_BUFFER_SIZE);

    if ((*rtp_memory)->buffer == NULL) {
        return RTP_OUT_OF_MEMORY;
    }   
 
    (*rtp_memory)->data_offset = 0;
    (*rtp_memory)->data_size = RTP_MIN_DATA_SIZE;
    (*rtp_memory)->data = (RTP_BYTE *) MNI_MALLOC(RTP_MIN_DATA_SIZE);

    if ((*rtp_memory)->data == NULL) {
        return RTP_OUT_OF_MEMORY;
    }   
 
    (*rtp_memory)->ssrc_table = NULL;
    (*rtp_memory)->audio_ready = TRUE;
    (*rtp_memory)->frame_ready = FALSE;

    (*rtp_memory)->connector = (RTP_CONNECTOR *) MNI_MALLOC(sizeof(RTP_CONNECTOR));
    
    if ((*rtp_memory)->connector == NULL) {
        return RTP_OUT_OF_MEMORY;
    }   
 
    return RTP_SUCCESS;
}


/** 
 * Resets RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 */

void rtp_reset_memory(RTP_MEMORY *rtp_memory) {
    rtp_memory->buffer_offset = 0;
}


/** 
 *  Removes an SSRC from the SSRC table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc_val     The value of the ssrc to be removed.
 */

void rtp_remove_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc_val) {
    RTP_SSRC *prev = NULL;
    RTP_SSRC *ssrc = rtp_memory->ssrc_table;

    while (ssrc != NULL) {
        if (ssrc->ssrc == ssrc_val) {
	  if (prev != NULL) {
	      prev->next = ssrc->next;
	  } else {
	      rtp_memory->ssrc_table = ssrc->next;
	  }

	  /* move down all memory blocks allocated at
	   * higher addresses than ssrc and ssrc->cname
	   */

	  shift_data_memory((RTP_BYTE *)ssrc, sizeof(RTP_SSRC));
	  adjust_pointers(rtp_memory->ssrc_table, (RTP_BYTE *)ssrc, sizeof(RTP_SSRC));
	  
	  shift_data_memory((RTP_BYTE *)ssrc->cname,
			     (RTP_WORD) (strlen((char *)(ssrc->cname)) + 1));
	  adjust_pointers(rtp_memory->ssrc_table, (RTP_BYTE *)ssrc->cname,
			   (RTP_WORD) (strlen((char *) (ssrc->cname)) + 1));

	  break;
	} else {
	    prev = ssrc;
	    ssrc = ssrc->next;
	}
    }
}


/** 
 * Frees RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 */

void rtp_free_memory(RTP_MEMORY *rtp_memory) {
    RTP_SSRC *ssrc;
    RTP_DECODER *decoder;

    if (rtp_memory != NULL) {
        ssrc = rtp_memory->ssrc_table;
	
	while (ssrc != NULL) {
	    decoder = ssrc->decoder;
	    
	    if (decoder != NULL) {
		if (decoder->closeFn != NULL) {
		    decoder->closeFn(decoder->state);
		}
	    }

	    ssrc = ssrc->next;
	}

	if (rtp_memory->buffer != NULL) {
	    MNI_FREE(rtp_memory->buffer);
	}

	if (rtp_memory->data != NULL) {
	    MNI_FREE(rtp_memory->data);
	}

	if (rtp_memory->connector != NULL) {
	    MNI_FREE(rtp_memory->connector);
	}

	MNI_FREE(rtp_memory);
    }
}

/** 
 * Allocates a block of RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param size         Size in number of bytes to be allocated.
 * @return             Returns a pointer to the allocated data
 *                     segement or NULL if the memory allocation
 *                     failed.
 * @todo Needs work (see comments).
 */

void *rtp_get_memory(RTP_MEMORY *rtp_memory, RTP_WORD size) {
    void *ret_ptr = NULL; 

    if (rtp_memory->buffer_offset + size < rtp_memory->buffer_size) {
        ret_ptr = (void *)(rtp_memory->buffer + rtp_memory->buffer_offset);
        rtp_memory->buffer_offset += size;
    } else {
        rtp_memory->buffer_size = rtp_memory->buffer_offset + size;
	
	ret_ptr = (void *) realloc(rtp_memory->buffer, rtp_memory->buffer_size);

	if (ret_ptr == NULL) {
	    /* Achtung: the block pointed to by buffer is still intact
	     * and needs to be freed by a call to rtp_free_memory().
	     */
#ifdef DEBUG
	    write_debug("Out of RTP/RTCP memory - realloc failed!\n");
#endif
	} else {
	    /* ret_ptr now points to the new buffer, the old one has been
	     * freed by realloc().
	     */

	    /* swap the pointers */
	    rtp_memory->buffer = ret_ptr;
	  
	    /* finally, recurse to get the requested memory */
	    ret_ptr = rtp_get_memory(rtp_memory, size);
	  
	    /* TODO: after reallocating the block memory pointers
	     * held by rtp_packet may not point to the right addresses
	     * anymore and my need to be adjusted
	     */
	}
    }

    return ret_ptr;
}

/** 
 *  Allocates a block of RTP data memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param size         Size in number of bytes to be allocated.
 * @return             Returns a pointer to the allocated data
 *                     segement or NULL if the memory allocation
 *                     failed.
 *
 * @todo Needs work (see comments).
 */

void *rtp_get_data_memory(RTP_MEMORY *rtp_memory, RTP_WORD size) {
    void *ret_ptr= NULL;

    if (rtp_memory->data_offset + size < rtp_memory->data_size) {
        ret_ptr = (void *)(rtp_memory->data + rtp_memory->data_offset);
	rtp_memory->data_offset += size;
    } else {
        rtp_memory->data_size = rtp_memory->data_offset + size;

        ret_ptr= (void *) realloc(rtp_memory->data, rtp_memory->data_size);

        if (ret_ptr == NULL) {
	   /* Achtung: the block pointed to by data is still intact
	    * and needs to be freed by a call to rtp_free_memory().
	    */

#ifdef DEBUG
	    write_debug("Out of RTP/RTCP memory - realloc failed!\n");
#endif
	} else {
	    /* ret_ptr now points to the new data, the old one has been
	     * freed by realloc().
	     */

	  /* swap the pointers */
	  rtp_memory->data = ret_ptr;

	  /* finally, recurse to get the requested memory */
	  ret_ptr= rtp_get_memory(rtp_memory, size);

	  /* TODO: after reallocating the block memory pointers
	   * held by rtp_packet may not point to the right addresses
	   * anymore and my need to be adjusted
	   */
	}
    }

    return ret_ptr;
}

/** @todo Needs documentation. */
/* recycle the data buffer */
static void shift_data_memory(RTP_BYTE *rtp_memory, RTP_WORD amount) {
    memmove(rtp_memory, rtp_memory + amount, amount);
}

/** 
 * Adjusts pointers to data buffer entries after a
 * shift_data_memory operation.
 */
static void adjust_pointers(RTP_SSRC *ssrc, RTP_BYTE *rtp_memory, RTP_WORD amount) {
    if ((RTP_BYTE *)ssrc > rtp_memory) {
        /* adjust the pointer to the ssrc entry */
        ssrc-= amount;

	if (ssrc->cname > rtp_memory) {
	    /* adjust the pointer to the cname of the ssrc entry */
	    ssrc->cname-= amount;
	}
      
        ssrc= ssrc->next;
    }

    while (ssrc != NULL) {
        if ((RTP_BYTE *)ssrc > rtp_memory) {
	    /* adjust the pointer to the ssrc entry */
	    ssrc-= amount;

	    if (ssrc->cname > rtp_memory) {
	        /* adjust the pointer to the cname of the ssrc entry */
	        ssrc->cname-= amount;
	    }
	} else {
	    ssrc= ssrc->next;
	}
    }
}

/** 
 * Allocates and adds a new RTP decoder for the specified synchronization 
 * source and payload type.
 *
 * @param rtp_memory     Pointer to the RTP_MEMORY structure.
 * @param ssrc           The synchronization source.
 * @param payload_type   The payload type.
 */

void rtp_add_decoder(RTP_MEMORY *rtp_memory, RTP_SSRC *ssrc, int payload_type) {
    ssrc->decoder = 
      (RTP_DECODER *) rtp_get_data_memory(rtp_memory, sizeof(RTP_DECODER));

    ssrc->decoder->state = NULL;
    ssrc->decoder->writeAudioFn = NULL;
    ssrc->decoder->closeFn = NULL;
    ssrc->payload_type = payload_type;
}

/** @todo Needs documentation. */

static RTP_SSRC *rtp_add_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc_val) {
    RTP_SSRC *ssrc_entry= NULL;
    RTP_SSRC *ssrc;

    if (ssrc_exists(rtp_memory, ssrc_val)) {
#ifdef DEBUG
        /* write_debug("SSRC already exists.\n"); */
#endif
    } else {
        ssrc_entry = (RTP_SSRC *) rtp_get_data_memory(rtp_memory, sizeof(RTP_SSRC));
	ssrc_entry->ssrc = ssrc_val;
	ssrc_entry->cname = NULL;
	ssrc_entry->decoder = NULL;
	ssrc_entry->next = NULL;
	
	if (rtp_memory->ssrc_table == NULL) {
	    rtp_memory->ssrc_table= ssrc_entry;
	} else {
	    ssrc= rtp_memory->ssrc_table;

	    while (ssrc->next != NULL) {
	        ssrc= ssrc->next;
	    }

	    ssrc->next= ssrc_entry;
	}
    }
    
    return ssrc_entry;
}


/** 
 * Retrieves the RTP_SSRC structure for the specified
 * synchronization source. A new RTP_SSRC will be created
 * if the ssrc does not exist in the table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc         The synchronization source.
 *
 * @return A pointer to the RTP_SSRC element or NULL if
 * it cannot be found or allocated.
 */

RTP_SSRC *rtp_get_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc) {
    RTP_SSRC *ret_ptr = NULL;
    RTP_SSRC *ssrc_entry = rtp_memory->ssrc_table;

    while (ssrc_entry != NULL) {
        if (ssrc_entry->ssrc == ssrc) {
	    ret_ptr = ssrc_entry;
	    break;
	}

	ssrc_entry = ssrc_entry->next;
    }

    if (ret_ptr == NULL) {
        ret_ptr = rtp_add_ssrc(rtp_memory, ssrc);
    }

    return ret_ptr;
}

/** 
 *  Checks the SSRC table whether the specified SSRC already exists.
 *
 * @param ssrc         The SSRC to be checked.
 * @return             Returns TRUE if the SSRC already exists, otherwise
 *                     FALSE.
 */

static RTP_BOOL ssrc_exists(RTP_MEMORY *rtp_memory, RTP_WORD ssrc) {
    RTP_BOOL  exists= FALSE;
    RTP_SSRC *ssrc_entry= rtp_memory->ssrc_table;

    while (ssrc_entry != NULL) {
        if (ssrc_entry->ssrc == ssrc) {
	    exists= TRUE;
	    break;
	} else {
	    ssrc_entry= ssrc_entry->next;
	}
    }
    
    return exists;
}


/** 
 *  Links a CNAME to the respective entry in the the SSRC table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc_val     The value of the SSRC in the SSRC table.
 * @param cname_data   A byte array representing a CNAME.
 * @param cname_length The length of the CNAME byte array.
 *
 * @todo rtp_map_ssrc should return RTP_SUCCESS or RTP_OUT_OF_MEMORY. 
 */

void rtp_map_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc_val, 
		  RTP_BYTE *cname_data, RTP_WORD cname_length) {
    RTP_SSRC *ssrc_table = rtp_memory->ssrc_table;

    while (ssrc_table != NULL) {
        if (ssrc_table->ssrc == ssrc_val) {
	    ssrc_table->cname = (RTP_BYTE *)
	      rtp_get_data_memory(rtp_memory, (RTP_WORD) (cname_length + 1));

	    memcpy(ssrc_table->cname, cname_data, cname_length);

	    ssrc_table->cname[ cname_length]= '\0';
	    break;
	} else {
	    ssrc_table = ssrc_table->next;
	}
    }
}


