/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */ 
/*
 * jpegencoder.c
 *
 * This file illustrates how to use the IJG code as a subroutine library
 * to read or write JPEG image files.  You should look at this code in
 * conjunction with the documentation file libjpeg.doc.
 *
 * This code will not do anything useful as-is, but it may be helpful as a
 * skeleton for constructing routines that call the JPEG library.
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <memory.h>

#include "mni.h"
#include "jpegencoder.h"
/*
no need in this header yet, since functions declared there 
are used only in one (current) file ... 
#include "jpeg_encoder_api.h"
*/
#include "jpeglib.h"

/****************************************************************
 * Structs for JPEG encoder
 ****************************************************************/

typedef struct {
    char * data;
    char * tmp_data;
    void * jerr;
    int length;
} jmf_dest_data;

typedef struct {
    struct jpeg_destination_mgr pub; /* public fields */

    JOCTET *buffer;		/* start of buffer */
} jmf_destination_mgr;

typedef jmf_destination_mgr * jmf_dest_ptr;

struct jmf_error_mgr {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct jmf_error_mgr * jmf_error_ptr;

#define JMF_OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */



/****************************************************************
 * Destination manager implementation for encoder
 ****************************************************************/

METHODDEF(void)
jmf_init_destination (j_compress_ptr cinfo)
{
    jmf_dest_ptr dest = (jmf_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *) ((jmf_dest_data *) (cinfo->client_data))->tmp_data;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = JMF_OUTPUT_BUF_SIZE;
}


METHODDEF(boolean)
jmf_empty_output_buffer (j_compress_ptr cinfo)
{
    jmf_dest_ptr dest = (jmf_dest_ptr) cinfo->dest;
    jmf_dest_data *destData = (jmf_dest_data*) cinfo->client_data;

    /* Copy the buffer contents. */
    memcpy(destData->data + destData->length, dest->buffer, JMF_OUTPUT_BUF_SIZE);
    destData->length += JMF_OUTPUT_BUF_SIZE;
/*     printf("wrote %d\n", destData->length); */
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = JMF_OUTPUT_BUF_SIZE;

    return TRUE;
}

METHODDEF(void)
jmf_term_destination (j_compress_ptr cinfo)
{
    jmf_dest_ptr dest = (jmf_dest_ptr) cinfo->dest;
    size_t datacount = JMF_OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    jmf_dest_data *destData = (jmf_dest_data*) cinfo->client_data;

    /* Write any data remaining in the buffer */
    if (datacount > 0) {
	memcpy(destData->data + destData->length, dest->buffer, datacount);
	destData->length += datacount;
/* 	printf("wrote %d\n", destData->length); */
    }
}


/****************************************************************
 * Error Manager implementation for encoder
 ****************************************************************/

METHODDEF(void)
jmf_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a jmf_error_mgr struct, so coerce pointer */
    jmf_error_ptr myerr = (jmf_error_ptr) cinfo->err;
    (myerr->pub.output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/****************************************************************
 * encoder creation, invocation and destruction methods
 ****************************************************************/

static struct jpeg_compress_struct *
RGB_To_JPEG_init(int width, int height, int quality, int decimation)
{
    struct jmf_error_mgr *jerr;
    struct jpeg_compress_struct *cinfo;
    jmf_destination_mgr *jmf_dest;
    jmf_dest_data *clientData;

    clientData = (jmf_dest_data*) MNI_MALLOC(sizeof(jmf_dest_data)); /* Alloc 1 */

    clientData->tmp_data = (char *) MNI_MALLOC(JMF_OUTPUT_BUF_SIZE); /* Alloc 2 */

    /* Step 1: allocate and initialize JPEG compression object */
    cinfo = (struct jpeg_compress_struct *)
	MNI_MALLOC(sizeof(struct jpeg_compress_struct));/* Alloc 3 */

    /* Initialize error parameters */
    jerr = (struct jmf_error_mgr *) MNI_MALLOC(sizeof(struct jmf_error_mgr)); /* Alloc 4 */
    clientData->jerr = (void *) jerr;

    cinfo->err = jm_jpeg_std_error(&(jerr->pub));
    (jerr->pub).error_exit = jmf_error_exit;

    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jm_jpeg_destroy_compress(cinfo);
        MNI_FREE(jerr);
        MNI_FREE(clientData->tmp_data);
        MNI_FREE(clientData);
        MNI_FREE(cinfo);
        printf("JPEG encoding error!\n");
        return 0;
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(cinfo);

    /* Set up my own destination manager. */
    jmf_dest = (jmf_destination_mgr *) MNI_MALLOC(sizeof(jmf_destination_mgr));/* Alloc 5 */

    jmf_dest->pub.init_destination = jmf_init_destination;
    jmf_dest->pub.empty_output_buffer = jmf_empty_output_buffer;
    jmf_dest->pub.term_destination = jmf_term_destination;

    cinfo->dest = (struct jpeg_destination_mgr *) jmf_dest;

    /* Set up client data */
    cinfo->client_data = clientData;

    /* Step 3: set parameters for compression */

    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */
    cinfo->image_width = width; 	/* image width and height, in pixels */
    cinfo->image_height = height;
    cinfo->input_components = 3;		/* # of color components per pixel */
    cinfo->in_color_space = JCS_RGB; 	/* colorspace of input image */

    /* Tell the library to set default parameters */
    jm_jpeg_set_defaults(cinfo);
    /* Default decimation is YUV 4:2:0. If we need 422 or 444, we
       modify the h_samp_factor and v_samp_factor for U and V components. */
    if (decimation >= 1) {
        int hs = 1, vs = 1;
        switch (decimation) {
        case 1: hs = 2; vs = 2; break;
        case 2: hs = 2; vs = 1; break;
        case 4: hs = 1; vs = 1;
            break;
    	}
        (cinfo->comp_info[0]).v_samp_factor = vs;
        (cinfo->comp_info[0]).h_samp_factor = hs;
        (cinfo->comp_info[1]).v_samp_factor = 1;
        (cinfo->comp_info[2]).v_samp_factor = 1;
        (cinfo->comp_info[1]).h_samp_factor = 1;
        (cinfo->comp_info[2]).h_samp_factor = 1;
    }

    /* Now you can set any non-default parameters you wish to.
     * Here we just illustrate the use of quality (quantization table) scaling:
     */
    jm_jpeg_set_quality(cinfo, quality, TRUE /* limit to baseline-JPEG values */
		     );

    return cinfo;
}

static void
RGB_To_JPEG_free(struct jpeg_compress_struct *cinfo)
{
    jm_jpeg_destroy_compress(cinfo);
    MNI_FREE(((jmf_dest_data*)cinfo->client_data)->tmp_data);
    MNI_FREE(((jmf_dest_data*)cinfo->client_data)->jerr);
    MNI_FREE(cinfo->client_data);
    MNI_FREE(cinfo->dest);
    MNI_FREE(cinfo);
}

static int
RGB_To_JPEG_encode(struct jpeg_compress_struct *cinfo,
		   char *inData, char *outData, int flipped,
		   int quality, int decimation, 
           JPEG_ENCODER_INPUT_COLOR_FORMAT colorFormat)
{
    struct jmf_error_mgr *jerr = (struct jmf_error_mgr *) cinfo->err;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int rowStride;		/* physical row width in image buffer */
    jmf_destination_mgr *jmf_dest = (jmf_destination_mgr *) cinfo->dest;
    jmf_dest_data *clientData = (jmf_dest_data *) cinfo->client_data;

    clientData->data = (char *) outData;
    clientData->length = 0;
    /* Establish the setjmp return context for jmf_error_exit to use. */

    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
        return 0;
    }

    { /* To work around compiler bug for setjmp, moved these 3 variables
         down from above */
        int direction = 1;
        int start = 0;
        char *tmpLine = NULL;
        int need_swap = 0;
        int pixelStride = 4;

        jmf_dest->buffer = (JOCTET *) clientData->tmp_data;
        if (quality >= 0) {
            jm_jpeg_set_quality(cinfo, quality, TRUE);
        }

        /* Default decimation is YUV 4:2:0. If we need 422 or 444, we
           modify the h_samp_factor and v_samp_factor for U and V components. */

        if (decimation >= 1) {
            int hs = 1, vs = 1;
            switch (decimation) {
            case 1: hs = 2; vs = 2; break;
            case 2: hs = 2; vs = 1; break;
            case 4: hs = 1; vs = 1;
            break;
            }
            (cinfo->comp_info[0]).v_samp_factor = vs;
            (cinfo->comp_info[0]).h_samp_factor = hs;
            (cinfo->comp_info[1]).v_samp_factor = 1;
            (cinfo->comp_info[2]).v_samp_factor = 1;
            (cinfo->comp_info[1]).h_samp_factor = 1;
            (cinfo->comp_info[2]).h_samp_factor = 1;
        }

        switch(colorFormat) {
        case JPEG_ENCODER_COLOR_GRAYSCALE: 
            pixelStride = 1;
            need_swap = 0;
            break;
        case JPEG_ENCODER_COLOR_RGB:
            pixelStride = 3;
            need_swap = 0;
            break;
        case JPEG_ENCODER_COLOR_BGR:
            pixelStride = 3;
            need_swap = 1;
            break;
        case JPEG_ENCODER_COLOR_XRGB:
            pixelStride = 4;
            need_swap = 1;
            break;
        case JPEG_ENCODER_COLOR_BGRX:
            pixelStride = 4;
            need_swap = 1;
            break;
        default:
            break;
        }

        jm_jpeg_start_compress(cinfo, TRUE);
        rowStride = cinfo->image_width * pixelStride; /* JSAMPLEs per row in image_buffer */
        if (flipped) {
            direction = -1;
            start = (cinfo->image_height - 1) * rowStride;
        }

        if (need_swap) {
            tmpLine = (char *) MNI_MALLOC(cinfo->image_width * pixelStride);
        }

        while (cinfo->next_scanline < cinfo->image_height) {
            if (need_swap) {
                int i;
                char *rgbLine = tmpLine;

                char *srcLine = (char *) &inData[start +
                             direction * cinfo->next_scanline * rowStride];

                switch(colorFormat) {     

                case JPEG_ENCODER_COLOR_BGR:
                    for (i = 0; i < (int) cinfo->image_width; i++) {
                        *rgbLine++ = srcLine[2]; /* R */
                        *rgbLine++ = srcLine[1]; /* G */ 
                        *rgbLine++ = srcLine[0]; /* B */
                        srcLine+=3;
                    }
                    break;
                case JPEG_ENCODER_COLOR_XRGB:
                    for (i = 0; i < (int) cinfo->image_width; i++) {
                        srcLine++; /* A */
                        *rgbLine++ = *srcLine++; /* R */
                        *rgbLine++ = *srcLine++; /* G */
                        *rgbLine++ = *srcLine++; /* B */
                    }
                    break;
                case JPEG_ENCODER_COLOR_BGRX:
                    for (i = 0; i < (int) cinfo->image_width; i++) {
                        *rgbLine++ = srcLine[2]; /* R */
                        *rgbLine++ = srcLine[1]; /* G */
                        *rgbLine++ = srcLine[0]; /* B */
                        srcLine+=4; /* A */
                    }
                    break;
                default:
                    break;
                }
                row_pointer[0] = tmpLine;
            } else {
                row_pointer[0] = &inData[start +
                        direction * cinfo->next_scanline * rowStride];
            }

            (void) jm_jpeg_write_scanlines(cinfo, row_pointer, 1);
        }

        if (need_swap) {
            MNI_FREE(tmpLine);
        }

        jm_jpeg_finish_compress(cinfo);
    }
    return clientData->length;
}

int
RGBToJPEG(char *inData, int width, int height, int quality, char *outData,
          JPEG_ENCODER_INPUT_COLOR_FORMAT colorFormat)
{
    struct jpeg_compress_struct* cinfo = 
                RGB_To_JPEG_init(width, height, quality, 1);

    int result = RGB_To_JPEG_encode(cinfo,
				    inData, outData, 0,
				    -1, -1, colorFormat);
    if (result)
	RGB_To_JPEG_free(cinfo);

    return result;
}
