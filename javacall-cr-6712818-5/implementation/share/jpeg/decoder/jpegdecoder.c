/*
 *  
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
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
 * jpegdecoder.c
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

//#if 0 /* NOT NEEDED BY PORT */
#include <stdio.h>
//#endif
#include <string.h>

#include "mni.h"
#include "jpeglib.h"
#include <setjmp.h>

#include "jpegdecoder.h"
const unsigned char jm_huffmanTable[] =
{
/* JPEG DHT Segment for YCrCb omitted from Nielsen's JPEG stream */
0xFF,0xC4,                                            /* Marker Code */
0x01,0xA2,                                            /* Full Table Length 01A2 bytes */

/*
 * Table Class/Identifier for DC th=0
 * LUMINANCE_TCTH_DC_TABLE
 * Each of these tables is followed by an array of 16 values.  This is called the
 * Bitlength Table array, adding these together indicates how many
 * additional bytes to read into, this TC=0 is for DC table specification
 */
0x00,

0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*
 * Summing above entries indicates that there are 12 byteds of
 * additional huffman code values for the DC table
 */
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,

/*
 * Table Class/Identifier for DC values of Cr Component th=1
 * CHROMINACE_TCTH_DC_TABLE
 */
0x01,

0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,

/*
 * Summing above entries indicates that there are also 12 bytes of
 * additional huffman code values for this table.  Many are
 * associated with longer it lengths though.
 */
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,


/*
 * Table Class/Identifier for Y, AC Component (162 Code Values)
 * LUMINANCE_TCTH_AC_TABLE
 */
0x10,

0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,


0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,
0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,
0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34,0x35,0x36,0x37,
0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,
0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,
0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,

/*
 * Table Class/Identifier for Cr/Cb, These are AC huffman values for the Cr/Cb component
 * CHROMINANCE_TCTH_AC_TABLE
 */
0x11,

0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,

0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,
0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,0x62,0x72,0xD1,
0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,
0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,
0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,
0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,
0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,
0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
};

/****************************************************************
 * Structs for JPEG decoder
 ****************************************************************/

typedef struct {
    char * data;
    void * jerr;
    int length;
} jmf_src_data;

typedef struct {
    struct jpeg_source_mgr pub; /* public fields */
} jmf_source_mgr;

typedef jmf_source_mgr * jmf_src_ptr;

struct jmf_error_mgr2 {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct jmf_error_mgr2 * jmf_error_ptr2;

#define JMF_INPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */



/****************************************************************
 * Destination manager implementation for encoder
 ****************************************************************/

METHODDEF(void)
jmf_init_source (j_decompress_ptr cinfo)
{
    jmf_src_ptr src = (jmf_src_ptr) cinfo->src;

    src->pub.next_input_byte = (JOCTET *) ((jmf_src_data *) (cinfo->client_data))->data;
    src->pub.bytes_in_buffer = ((jmf_src_data *) (cinfo->client_data))->length;
}


METHODDEF(boolean)
jmf_fill_input_buffer (j_decompress_ptr cinfo)
{
    (void)cinfo;
    return TRUE;
}

METHODDEF(void)
jmf_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  jmf_src_ptr src = (jmf_src_ptr) cinfo->src;

  src->pub.next_input_byte += (size_t) num_bytes;
  src->pub.bytes_in_buffer -= (size_t) num_bytes;
}

METHODDEF(void)
jmf_term_source (j_decompress_ptr cinfo)
{
    (void)cinfo;
}


/****************************************************************
 * Error Manager implementation for encoder
 ****************************************************************/

METHODDEF(void)
jmf_error_exit2 (j_common_ptr cinfo)
{
    /* cinfo->err really points to a jmf_error_mgr struct, so coerce pointer */
    jmf_error_ptr2 myerr = (jmf_error_ptr2) cinfo->err;
    /*printf("At error %d\n", myerr->pub.msg_code);*/
    (myerr->pub.output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/****************************************************************
 * John Coffey's huffman table creation
 ****************************************************************/

/*
 * Code to preset the huffman table given the H_Factor
 */
void jm_calculate_huffman_table (j_decompress_ptr cinfo, const unsigned char *jm_huffmanTable)
{
   INT32 length = 0;
   UINT8 bits[17];
   UINT8 huffval[256];
   int i, index, count;
   JHUFF_TBL **htblptr;

   /* Read in the length bytes */
   jm_huffmanTable += 2;      // Skip over the marker first
// length = *(short *)jm_huffmanTable;
   length = 0x1a2;   // Hard coded from the table
   length -= 2;
   jm_huffmanTable += 2;

   /*
    * @JC Read in all components, note that all components start with
    * an identifyer that indicates the table class and the
    */
   while (length > 0) {
      index = *(jm_huffmanTable++);

      bits[0] = 0;
      count = 0;

      /* @JC read in the 16 bit length entries, the value at index 'i'
       * represents how many 'i' bitlength codewords follow.  Zero
       * entrues indicate that there are no codes for that bitlength
       *
       * Count keeps a track of how many code values should follow the
       * bits array.  This should be <= 256
       */
      for (i = 1; i <= 16; i++)
      {
         bits[i] = *(jm_huffmanTable++);
         count += bits[i];
      }

      /* Update length to acount for index & bitlengths */
      length -= 1 + 16;
      /*
      if (count > 256 || ((INT32) count) > length)
         ERREXIT(cinfo, JERR_DHT_COUNTS);
      */
      for (i = 0; i < count; i++)
         huffval[i] = *(jm_huffmanTable++);

      /* Update length as code values read in */
      length -= count;

      /* AC table definition */
      if (index & 0x10)
      {         
         index -= 0x10;
         htblptr = &cinfo->ac_huff_tbl_ptrs[index];
      }
      else
      { /* DC table definition */
         htblptr = &cinfo->dc_huff_tbl_ptrs[index];
      }
      /*
      if (index < 0 || index >= NUM_HUFF_TBLS)
         ERREXIT1(cinfo, JERR_DHT_INDEX, index);
      */
      /* @JC Must ensure that this if is never true in Modified Nielsen Codec
       * This is because the huffman table will be read in beforehand
       * in ISO format
       */
      if (*htblptr == NULL)
         *htblptr = jm_jpeg_alloc_huff_table((j_common_ptr) cinfo);

      memcpy((*htblptr)->bits, bits, sizeof((*htblptr)->bits));
      memcpy((*htblptr)->huffval, huffval, sizeof((*htblptr)->huffval));
   }
}

/****************************************************************
 * decoder creation, invocation and destruction methods
 ****************************************************************/

void *
JPEG_To_RGB_init()
{
    struct jmf_error_mgr2 *jerr;
    struct jpeg_decompress_struct *cinfo;
    jmf_source_mgr *jmf_src;
    jmf_src_data *clientData;
    
    clientData = (jmf_src_data*) MNI_MALLOC(sizeof(jmf_src_data)); /* Alloc 1 */
    
    clientData->data = NULL;

    /* Step 1: allocate and initialize JPEG decompression object */
    cinfo = (struct jpeg_decompress_struct *)
	MNI_MALLOC(sizeof(struct jpeg_decompress_struct));/* Alloc 3 */

    /* Initialize error parameters */
    jerr = (struct jmf_error_mgr2 *) MNI_MALLOC(sizeof(struct jmf_error_mgr2)); /* Alloc 4 */
    clientData->jerr = (void *) jerr;
    
    cinfo->err = jm_jpeg_std_error(&(jerr->pub));
    (jerr->pub).error_exit = jmf_error_exit2;

    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file, and return.
	 */
        jm_jpeg_destroy_decompress(cinfo);
        MNI_FREE(jerr);
        MNI_FREE(clientData);
        MNI_FREE(cinfo);
        return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(cinfo);
    /* Set up my own destination manager. */
    jmf_src = (jmf_source_mgr *) MNI_MALLOC(sizeof(jmf_source_mgr));/* Alloc 5 */
    
    jmf_src->pub.init_source = jmf_init_source;
    jmf_src->pub.skip_input_data = jmf_skip_input_data;
    jmf_src->pub.resync_to_restart = jm_jpeg_resync_to_restart;
    jmf_src->pub.fill_input_buffer = jmf_fill_input_buffer;
    jmf_src->pub.term_source = jmf_term_source;

    cinfo->src = (struct jpeg_source_mgr *) jmf_src;

    /* Set up client data */
    cinfo->client_data = clientData;

    { // John Coffey's code
    /*
     * @JC This is where the precalculated ISO huffmann tables
     * are calculated and put into instance data, in Nielsen's
     * case this data will not ne overwritten as no DHT markers
     * will be present in the stream
     */
    int i;
    for (i = 0; i < NUM_HUFF_TBLS - 1; i++)  /* Note the -1 @JC */ {
        /* @JC cinfo->dc_huff_tbl_ptrs[i] = NULL; */
        /* @JC cinfo->ac_huff_tbl_ptrs[i] = NULL; */
        /* Note probably should not allocate for 4th unused component */
        if (cinfo->dc_huff_tbl_ptrs[i]) {
        cinfo->dc_huff_tbl_ptrs[i] = jm_jpeg_alloc_huff_table ((j_common_ptr) cinfo);
        }

        if (cinfo->ac_huff_tbl_ptrs[i]) {
        cinfo->ac_huff_tbl_ptrs[i] = jm_jpeg_alloc_huff_table ((j_common_ptr) cinfo);
        }
    }

    /* @JC Ensure that the 4th table is as per normal, only allocate if needed */
    cinfo->dc_huff_tbl_ptrs[i] = NULL;
    cinfo->ac_huff_tbl_ptrs[i] = NULL;

    /* @JC This huffman table works for all 3 components, this 
     * is why it is defined outside the loop above
     */
    jm_calculate_huffman_table (cinfo, jm_huffmanTable);
    }
    
    return (void*) cinfo;
}

void
JPEG_To_RGB_free(void *info)
{
    struct jpeg_decompress_struct *cinfo =
	(struct jpeg_decompress_struct *) info;
    jm_jpeg_destroy_decompress(cinfo);
    MNI_FREE(((jmf_src_data*)cinfo->client_data)->jerr);
    MNI_FREE(cinfo->client_data);
    MNI_FREE(cinfo->src);
    MNI_FREE(cinfo);
}

int
JPEG_To_RGB_decodeHeader(void *info, 
    char *inData, int inDataLen, 
    int* width, int* height)
{
    struct jpeg_decompress_struct *cinfo =
	(struct jpeg_decompress_struct*) info;
    struct jmf_error_mgr2 *jerr = (struct jmf_error_mgr2 *) cinfo->err;
    // jmf_source_mgr *jmf_src = (jmf_source_mgr *) cinfo->src;
    jmf_src_data *clientData = (jmf_src_data *) cinfo->client_data;
    clientData->data = (char *) inData;
    clientData->length = inDataLen;
    
    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
        return 0;
    }
    jm_jpeg_read_header(cinfo, TRUE);
    
    *width = cinfo->output_width;
    *height = cinfo->output_height;
    
    return 1;
}

int
JPEG_To_RGB_decodeData(void *info, char *outData)
{
    struct jpeg_decompress_struct *cinfo =
	(struct jpeg_decompress_struct*) info;
    struct jmf_error_mgr2 *jerr = (struct jmf_error_mgr2 *) cinfo->err;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int rowStride;		/* physical row width in image buffer */
    int pixelSize;
    /*
     * Comment out unused variables.
     */
    /* jmf_source_mgr *jmf_src = (jmf_source_mgr *) cinfo->src; */
    /* jmf_src_data *clientData = (jmf_src_data *) cinfo->client_data; */
    
    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
        return 0;
    }
    
    /* currently we support only one output format = 24-bit RGB */
    pixelSize = 3;
    cinfo->out_color_space = JCS_RGB;
    
    jm_jpeg_start_decompress(cinfo);
    
    /* JSAMPLEs per row in image_buffer */
    rowStride = cinfo->output_width * pixelSize;
    
    while (cinfo->output_scanline < cinfo->output_height) {
    	row_pointer[0] = (unsigned char *) &outData[
            cinfo->output_scanline * rowStride];
        (void) jm_jpeg_read_scanlines(cinfo, row_pointer, 1);
    }
    jm_jpeg_finish_decompress(cinfo);
    
    return cinfo->output_width * cinfo->output_height * pixelSize;
}

int JPEG_To_RGB_decodeData2(void *info, char *outData, int outPixelSize,
    int left, int top, int right, int bottom)
{
    
    struct jpeg_decompress_struct *cinfo =
	(struct jpeg_decompress_struct*) info;
    struct jmf_error_mgr2 *jerr = (struct jmf_error_mgr2 *) cinfo->err;
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int rowStride;		/* physical row width in image buffer */
    int pixelSize;

    /*
     * Comment out unused variables.
     */
    /* jmf_source_mgr *jmf_src = (jmf_source_mgr *) cinfo->src; */
    /* jmf_src_data *clientData = (jmf_src_data *) cinfo->client_data; */

    unsigned char *outDataPtr;
    unsigned int i;
    if ((outPixelSize != 2) && (outPixelSize != 4)) {
        return 0;
    }

    row_pointer[0] = NULL;

    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
    	return 0;
    }

    /* currently we support only one output format = 24-bit RGB */
    pixelSize = 3;
    cinfo->out_color_space = JCS_RGB;
    
    jm_jpeg_start_decompress(cinfo);

    /*ensure that top, left, bottom, right are valid and within image bounds*/
    if (top < 0 || left < 0 || right < 0 || bottom < 0 ||
        top >= bottom || left >= right) {
        return 0;
    }
    
    /* JSAMPLEs per row in image_buffer */
    /*rowStride = cinfo->output_width * pixelSize;*/
    rowStride = (right - left) * outPixelSize;
    row_pointer[0] = (unsigned char *)MNI_MALLOC(cinfo->output_width * pixelSize);

    /* Establish the setjmp return context for jmf_error_exit to use. */
    if (setjmp(jerr->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
        MNI_FREE(row_pointer[0]);
        return 0;
    }
    outDataPtr = (unsigned char *)outData;

    while (cinfo->output_scanline < cinfo->output_height) {
        /* all lines are scanned regardlessofthe needed rectangle */
        (void) jm_jpeg_read_scanlines(cinfo, row_pointer, 1);
        /* 
         * after call to jm_jpeg_read_scanlines() 
         * cinfo->output_scanline is increased by 1.
         */

        if ((cinfo->output_scanline > (unsigned)top) && 
            (cinfo->output_scanline <= (unsigned)bottom)) {
            /* convert pixels of the line to RGB565 format */
            for (i = 0; i < cinfo->output_width; i++) {
                if ((i >= (unsigned)left) && (i < (unsigned)right)) {
                    unsigned int r = row_pointer[0][i * 3 + 0] & 0xFF;
                    unsigned int g = row_pointer[0][i * 3 + 1] & 0xFF;
                    unsigned int b = row_pointer[0][i * 3 + 2] & 0xFF;
                    if (2 == outPixelSize) {
                        ((unsigned short*)outDataPtr)[i-(unsigned)left] =
                            ((b & 0xF8) >> 3) + ((g & 0xFC) << 3) + ((r & 0xF8) << 8);
                    } else /* if (4 == outPixelSize) */ {
                        ((unsigned long*)outDataPtr)[i-(unsigned)left] =
                            (b & 0xFF) + ((g & 0xFF) << 8) + ((r & 0xFF) << 16);
                    }
                }
            }
        }

        outDataPtr += rowStride;
    }

    jm_jpeg_finish_decompress(cinfo);

    if (row_pointer[0] != NULL) {
        MNI_FREE(row_pointer[0]);
    }

    return cinfo->output_width * cinfo->output_height * outPixelSize;
}

char*
JPEG_To_RGB_decode(void *info, char *inData, int inDataLen, 
    int *width, int* height)
{
    if (JPEG_To_RGB_decodeHeader(info, inData, inDataLen, width, height) == 0)
        return NULL;
    else {
        int outPixelSize = 3;
        char* outData = MNI_MALLOC((*width) * (*height) * outPixelSize);
        if (JPEG_To_RGB_decodeData(info, outData) != 0)
            return outData;
        else {
            MNI_FREE(outData);
            return NULL;
        }
    }
}

char*
JPEG_To_RGB_decode2(void *info, char *inData, int inDataLen, int outPixelSize,
    int left, int top, int right, int bottom, 
    int *width, int* height)
{
    if ((outPixelSize != 2) && (outPixelSize != 4)) {
        return NULL;
    }
    if (JPEG_To_RGB_decodeHeader(info, inData, inDataLen, width, height) == 0)
        return NULL;
    else {
        char* outData = MNI_MALLOC((*width) * (*height) * outPixelSize);
        if (JPEG_To_RGB_decodeData2(info, outData, outPixelSize,
            left, top, right, bottom) != 0)
            return outData;
        else {
            MNI_FREE(outData);
            return NULL;
        }
    }
}
