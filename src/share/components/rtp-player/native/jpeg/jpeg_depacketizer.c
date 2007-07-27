/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#include "mni.h"

#include <stdlib.h>
#include <string.h>
#include "rtp.h"
#include "jpeg_depacketizer.h"
#include "jpeg_decoder_api.h"

/* Function prototypes */
void jpeg_depacketizer_add( JPEG_DEPACKETIZER *depacketizer,
                            RTP_BYTE *data, RTP_WORD size, int extraskip);

void jpeg_increaseFrameBuffer( JPEG_DEPACKETIZER *depacketizer, int amount);

RTP_BOOL hasJFIFHeader( RTP_BYTE *data);

void jpeg_makeQTables(int q, int *lum_q, int *chr_q);

int generateJFIFHeader( JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *data);

int jpeg_makeHeaders( JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *p,
                      int offset, int type, int q, int w, int h);

int jpeg_makeQuantHeader(RTP_BYTE *p, int offset, int *qt, int tableNo);

int jpeg_makeHuffmanHeader( RTP_BYTE *p, int offset,
                            int *codelens, int ncodes,
                            int *symbols, int nsymbols,
                            int tableNo, int tableClass);

extern int jpeg_getFragOffset( RTP_BYTE *data, int doff);

#define DEC_420 1
#define DEC_444 2

static int lum_dc_codelens[] = {
                                   0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
                               };

static int lum_dc_symbols[] = {
                                  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                              };

static int lum_ac_codelens[] = {
                                   0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
                               };

static int lum_ac_symbols[] = {
                                  0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
                                  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
                                  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
                                  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
                                  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
                                  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
                                  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
                                  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                                  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
                                  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                                  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
                                  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                                  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
                                  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
                                  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
                                  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
                                  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
                                  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
                                  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
                                  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
                                  0xf9, 0xfa
                              };

static int chm_dc_codelens[] = {
                                   0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
                               };

static int chm_dc_symbols[] = {
                                  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                              };

static int chm_ac_codelens[] = {
                                   0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
                               };

static int chm_ac_symbols[] = {
                                  0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
                                  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
                                  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
                                  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
                                  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
                                  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
                                  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
                                  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                                  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                                  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                                  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
                                  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                                  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
                                  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
                                  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
                                  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
                                  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
                                  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
                                  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
                                  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
                                  0xf9, 0xfa
                              };


/*
 * Table K.1 from JPEG spec.
 */
static int jpeg_luma_quantizer[] = {
                                       16, 11, 10, 16, 24, 40, 51, 61,
                                       12, 12, 14, 19, 26, 58, 60, 55,
                                       14, 13, 16, 24, 40, 57, 69, 56,
                                       14, 17, 22, 29, 51, 87, 80, 62,
                                       18, 22, 37, 56, 68, 109, 103, 77,
                                       24, 35, 55, 64, 81, 104, 113, 92,
                                       49, 64, 78, 87, 103, 121, 120, 101,
                                       72, 92, 95, 98, 112, 100, 103, 99
                                   };

/*
 * Table K.2 from JPEG spec.
 */
static int jpeg_chroma_quantizer[] = {
                                         17, 18, 24, 47, 99, 99, 99, 99,
                                         18, 21, 26, 66, 99, 99, 99, 99,
                                         24, 26, 56, 99, 99, 99, 99, 99,
                                         47, 66, 99, 99, 99, 99, 99, 99,
                                         99, 99, 99, 99, 99, 99, 99, 99,
                                         99, 99, 99, 99, 99, 99, 99, 99,
                                         99, 99, 99, 99, 99, 99, 99, 99,
                                         99, 99, 99, 99, 99, 99, 99, 99
                                     };

static int ZigZag[] = {
                          0,  1,  8, 16,  9,  2,  3, 10,
                          17, 24, 32, 25, 18, 11,  4,  5,
                          12, 19, 26, 33, 40, 48, 41, 34,
                          27, 20, 13,  6,  7, 14, 21, 28,
                          35, 42, 49, 56, 57, 50, 43, 36,
                          29, 22, 15, 23, 30, 37, 44, 51,
                          58, 59, 52, 45, 38, 31, 39, 46,
                          53, 60, 61, 54, 47, 55, 62, 63
                      };

static RTP_BYTE APP0[] = { (RTP_BYTE) 0xFF, (RTP_BYTE) 0xE0, 0, 16,
                       0x4A, 0x46, 0x49, 0x46, 0, 1, 1,
                       0, 0, 1, 0, 1, 0, 0
                     };

JPEG_DEPACKETIZER * jpeq_depacketizer_init() {
    JPEG_DEPACKETIZER *depacketizer= (JPEG_DEPACKETIZER *) MNI_MALLOC( sizeof( JPEG_DEPACKETIZER));

    depacketizer->firstSeq= 0;
    depacketizer->frameBuffer= (char *) MNI_MALLOC( FRAME_BUFFER_INITIAL_SIZE);
    depacketizer->frameBufferLength= FRAME_BUFFER_INITIAL_SIZE;
    depacketizer->numPkts= 0;
    depacketizer->dataLength= 0;
    depacketizer->quality= 0;
    depacketizer->type= 0;
    depacketizer->jpgdecoder= NULL;
    depacketizer->rgbData= NULL;
    depacketizer->rgbDataLength= 0;

    depacketizer->lastQuality = -2;
    depacketizer->lastType = -1;
    depacketizer->lastWidth = -1;
    depacketizer->lastHeight = -1;
    
    depacketizer->lastJFIFHeader = NULL;
    depacketizer->lastJFIFHeaderLength = 0;
    
    return depacketizer;
}


RTP_BOOL hasJFIFHeader( RTP_BYTE *data) {
	int offset = 0;
	if (! ((data[offset+8] & 0xFF) == 0xFF &&
	        (data[offset+9] & 0xFF) == 0xD8))
		return FALSE;
	else
		return TRUE;
}

void jpeq_depacketizer_reset( JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *data, 
                              RTP_WORD size, RTP_SHORT seqnum) {
	int extraskip = 0;

	depacketizer->gotFirstPacket= TRUE;
	depacketizer->firstSeq= seqnum;
	depacketizer->numPkts= 0;
	depacketizer->dataLength= 0;
	depacketizer->quality= 0;
	depacketizer->type= 0;

	// If the first JPEG packet does not have a
	// JFIF header, generate it.
	// This is true for vic

	if (!hasJFIFHeader( data)) {
		extraskip = generateJFIFHeader( depacketizer, data);
	}
	jpeg_depacketizer_add( depacketizer, data, size, extraskip);
}

int generateJFIFHeader( JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *data) {
	int extraskip = 0;
	int offset = 0;
	int hdrOffset = 0;
	int lquantOffset;
	int cquantOffset;
	int type = data[offset + 4];
	// Q factor is the 6th byte
	int quality = data[offset +5];
	// width is the 7th byte in 8bit pixels
	int width = data[offset + 6];
	// height is the 8th byte in 8 bit pixels
	int height = data[offset + 7];

	if( quality == depacketizer->lastQuality &&
	        width == depacketizer->lastWidth &&
	        height == depacketizer->lastHeight &&
	        type == depacketizer->lastType) {

		memcpy( depacketizer->frameBuffer,
		        depacketizer->lastJFIFHeader, depacketizer->lastJFIFHeaderLength);
	} else {
		hdrOffset = jpeg_makeHeaders( depacketizer, depacketizer->frameBuffer, 0,
		                              type, quality, width, height);

		depacketizer->lastJFIFHeader = (RTP_BYTE *) MNI_MALLOC( hdrOffset);

		memcpy( depacketizer->lastJFIFHeader, depacketizer->frameBuffer, hdrOffset);

		depacketizer->lastJFIFHeaderLength= hdrOffset;
		depacketizer->lastQuality = quality;
		depacketizer->lastType = type;
		depacketizer->lastWidth = width;
		depacketizer->lastHeight = height;
	}

	if (quality >= 100) {
		extraskip = 132;

		lquantOffset = (
		                   2 + // For FF D8 (SOI)
		                   sizeof( APP0) +
		                   2 + // For FF DB (DQT)
		                   2 + // length
		                   1  // tableNo
		               );

		cquantOffset = (
		                   lquantOffset +
		                   64 + // size of luma quant table
		                   2 + // For FF DB (DQT)
		                   2 + // length
		                   1  // tableNo
		               );

		memcpy( depacketizer->frameBuffer + lquantOffset, data + offset + 8 + 4, 64);

		memcpy( depacketizer->frameBuffer + cquantOffset, data + offset + 8 + 4 + 64, 64);
	}

	depacketizer->dataLength += depacketizer->lastJFIFHeaderLength;

	return extraskip;
}

/*
 * Given an RTP/JPEG type code, q factor, width, and height,
 * generate a frame and scan headers that can be prepended
 * to the RTP/JPEG data payload to produce a JPEG compressed
 * image in interchange format (except for possible trailing
 * garbage and absence of an EOI marker to terminate the scan).
 */
int jpeg_makeHeaders( JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *p,
                      int offset, int type, int q, int w, int h) {
	int lqt[ 64];
	int cqt[ 64];
	int app;

	/* convert from blocks to pixels */
	w *= 8;
	h *= 8;

	jpeg_makeQTables(q, lqt, cqt);

	p[offset++] = (RTP_BYTE) 0xff;
	p[offset++] = (RTP_BYTE) 0xd8;            /* SOI */

	// APP0 marker
	for( app = 0; app < sizeof( APP0); app++) {
		p[offset++] = APP0[app];
	}

	// TODO: pass q to makeQuantHeader. makeQuantHeader should
	// just skip computing the quant header as the quant data
	// is dynamic. Note that makeHeaders will be called only
	// if quality, width, height or type changes between frames.
	offset = jpeg_makeQuantHeader(p, offset, lqt, 0);

	offset = jpeg_makeQuantHeader(p, offset, cqt, 1);

	offset = jpeg_makeHuffmanHeader(p, offset,
	                                lum_dc_codelens, sizeof( lum_dc_codelens) / sizeof( int),
	                                lum_dc_symbols, sizeof( lum_dc_symbols) / sizeof( int),
	                                0, 0);

	offset = jpeg_makeHuffmanHeader(p, offset,
	                                lum_ac_codelens, sizeof( lum_ac_codelens) / sizeof( int),
	                                lum_ac_symbols, sizeof( lum_ac_symbols) / sizeof( int),
	                                0, 1);

	offset = jpeg_makeHuffmanHeader(p, offset,
	                                chm_dc_codelens, sizeof( chm_dc_codelens) / sizeof( int),
	                                chm_dc_symbols, sizeof( chm_dc_symbols) / sizeof( int),
	                                1, 0);

	offset = jpeg_makeHuffmanHeader(p, offset,
	                                chm_ac_codelens, sizeof( chm_ac_codelens) / sizeof( int),
	                                chm_ac_symbols, sizeof( chm_ac_symbols) / sizeof( int),
	                                1, 1);

	p[offset++] = (RTP_BYTE) 0xff;
	p[offset++] = (RTP_BYTE) 0xc0;			    /* SOF */
	p[offset++] = 0;				    /* length msb */
	p[offset++] = 17;				    /* length lsb */
	p[offset++] = 8;				    /* 8-bit precision */
	p[offset++] = (RTP_BYTE) ((h >> 8) & 0xFF);		    /* height msb */
	p[offset++] = (RTP_BYTE) ( h       & 0xFF);		    /* height lsb */
	p[offset++] = (RTP_BYTE) ((w >> 8) & 0xFF);		    /* width msb */
	p[offset++] = (RTP_BYTE) ( w       & 0xFF);		    /* width lsb */
	p[offset++] = 3;				    /* number of components */
	p[offset++] = 0;				    /* comp 0 */
	if (type == DEC_444)
		p[offset++] = 0x11;    /* hsamp = 2, vsamp = 1 */
	else if (type == DEC_420)
		p[offset++] = 0x22;    /* hsamp = 2, vsamp = 2 */
	else
		p[offset++] = 0x21;
	p[offset++] = 0;               /* quant table 0 */
	p[offset++] = 1;               /* comp 1 */

	p[offset++] = 0x11;
	p[offset++] = 1;               /* quant table 1 */
	p[offset++] = 2;               /* comp 2 */
	p[offset++] = 0x11;
	p[offset++] = 1;               /* quant table 1 */

	p[offset++] = (RTP_BYTE) 0xff;
	p[offset++] = (RTP_BYTE) 0xda;            /* SOS */
	p[offset++] = 0;               /* length msb */
	p[offset++] = 12;              /* length lsb */
	p[offset++] = 3;               /* 3 components */
	p[offset++] = 0;               /* comp 0 */
	p[offset++] = 0;               /* huffman table 0 */
	p[offset++] = 1;               /* comp 1 */
	p[offset++] = 0x11;            /* huffman table 1 */
	p[offset++] = 2;               /* comp 2 */
	p[offset++] = 0x11;            /* huffman table 1 */
	p[offset++] = 0;               /* first DCT coeff */
	p[offset++] = 63;              /* last DCT coeff */
	p[offset++] = 0;               /* sucessive approx. */

	return offset;
}


int jpeg_makeQuantHeader(RTP_BYTE *p, int offset,
                         int *qt, int tableNo) {
	int i;

	p[offset++] = (RTP_BYTE) 0xff;
	p[offset++] = (RTP_BYTE) 0xdb;			    /* DQT */
	p[offset++] = 0;			    /* length msb */
	p[offset++] = 67;			    /* length lsb */
	p[offset++] = (RTP_BYTE) tableNo;

	for( i = 0; i < 64; i++) {
		p[offset++] = (RTP_BYTE) qt[i];
	}

	return offset;
}

int jpeg_makeHuffmanHeader( RTP_BYTE *p, int offset,
                            int *codelens, int ncodes,
                            int *symbols, int nsymbols,
                            int tableNo, int tableClass) {
	int i;

	p[offset++] = (RTP_BYTE) 0xff;
	p[offset++] = (RTP_BYTE) 0xc4;                          /* DHT */
	p[offset++] = 0;                                    /* length msb */
	p[offset++] = (RTP_BYTE) (3 + ncodes + nsymbols);       /* length lsb */
	p[offset++] = (RTP_BYTE) ((tableClass << 4) | tableNo);

	for (i = 0; i < ncodes; i++)
		p[offset++] = (RTP_BYTE) codelens[i];

	for (i = 0; i < nsymbols; i++)
		p[offset++] = (RTP_BYTE) symbols[i];

	return offset;
}

/*
 * Call MakeTables with the Q factor and two int[64] return arrays
 */
void jpeg_makeQTables(int q, int *lum_q, int *chr_q) {
	int i;
	int factor = q;

	if (q < 1) factor = 1;
	if (q > 99) factor = 99;
	if (q < 50)
		q = 5000 / factor;
	else
		q = 200 - factor*2;

	for (i = 0; i < 64; i++) {
		int lq = (jpeg_luma_quantizer[ZigZag[i]] * q + 50) / 100;
		int cq = (jpeg_chroma_quantizer[ZigZag[i]] * q + 50) / 100;

		/* Limit the quantizers to 1 <= q <= 255 */
		if (lq < 1)
			lq = 1;
		else if (lq > 255)
			lq = 255;

		lum_q[i] = lq;

		if (cq < 1)
			cq = 1;
		else if (cq > 255)
			cq = 255;
		chr_q[i] = cq;
	}
}

void jpeg_depacketizer_add( JPEG_DEPACKETIZER *depacketizer,
                            RTP_BYTE *data, RTP_WORD size, int extraskip) {
	int chunkSize= size - 8 - extraskip;

	int foff = jpeg_getFragOffset( data, 0);

	foff += depacketizer->lastJFIFHeaderLength;  // If a JFIF header is inserted,
	// we need to shift the data.
	// 2 bytes is for the EOI marker

	if (depacketizer->frameBufferLength >= foff + chunkSize + 2) {
		memcpy( depacketizer->frameBuffer + foff,
		        data + 8 + extraskip,  //RTP Hdr + JPEG Hdr
		        chunkSize);

		depacketizer->dataLength+= chunkSize;
		depacketizer->numPkts++;
	} else {
		// 2 bytes is for the EOI marker
		jpeg_increaseFrameBuffer( depacketizer, foff + chunkSize + 2);
		jpeg_depacketizer_add( depacketizer,
		                       data, size, extraskip);
	}
}

void jpeg_increaseFrameBuffer( JPEG_DEPACKETIZER *depacketizer, int amount) {
	char *newFrameBuffer= (char *) MNI_MALLOC( amount);

	// Copy from the old buffer to the new buffer.
	memcpy( newFrameBuffer,
	        depacketizer->frameBuffer,
	        depacketizer->frameBufferLength);

	// Free up the old buffer space
	if( depacketizer->frameBuffer != NULL) {
		MNI_FREE( depacketizer->frameBuffer);
	}

	depacketizer->frameBuffer = newFrameBuffer;
}

RTP_BOOL jpeg_gotAllPackets( JPEG_DEPACKETIZER *depacketizer, long lastSeq) {
    return (lastSeq - depacketizer->firstSeq + 1 == depacketizer->numPkts);
}

void jpeg_completeTransfer( RTP_MEMORY *mem_ptr,
                            JPEG_DEPACKETIZER *depacketizer,
                            RTP_BYTE *data,
                            RTP_WORD size) {
    int x_off, y_off;
    int offset = 0;
    int landscape;

    // height is the 8th byte in 8 bit pixels
    int height = data[offset + 7] * 8;
    // width is the 7th byte in 8bit pixels
    int width = data[offset + 6] * 8;
    // Q factor is the 6th byte
    depacketizer->quality = data[offset +5];
    // type is the 5th byte
    depacketizer->type = data[offset + 4];

    depacketizer->width = width;
    depacketizer->height = height;

    /* we may need this */
    if (!((RTP_BYTE) (depacketizer->frameBuffer[depacketizer->dataLength-2]) == (RTP_BYTE) 0xff &&
	  (RTP_BYTE) (depacketizer->frameBuffer[depacketizer->dataLength-1]) == (RTP_BYTE) 0xd9)) {
        depacketizer->frameBuffer[depacketizer->dataLength++] = (RTP_BYTE) 0xff;
        depacketizer->frameBuffer[depacketizer->dataLength++] = (RTP_BYTE) 0xd9; // EOI
    }

    if (depacketizer->jpgdecoder == NULL) {
        /*
         * TBD: here we already have image size in {width,height}
         * before reading of JPEG header
         * (done in JPEG_To_RGB_decodeHeader(...) 
         * called from JPEG_To_RGB_decode(...) ). 
         * In future need to check if this explicitly defined size
         * match to image size read from JPEG header....
         */
        depacketizer->jpgdecoder = (void *)JPEG_To_RGB_init(/*width, height*/);
        depacketizer->rgbData= (char *)MNI_MALLOC( 3 * width * height + 3);
        depacketizer->rgbDataLength= (3 * width * height + 3);
    }

    if( width > 240) {
        landscape = 1;
        // in landscape mode:
        x_off= (int) ((240 - height) / 2);
        y_off= (int) ((320 - width) / 2);        
    } else {
        landscape = 0;
        x_off= (int) ((240 - width) / 2);
        y_off= (int) ((320 - height) / 2);
    }
        
    JPEG_To_RGB_decode( depacketizer->jpgdecoder, depacketizer->frameBuffer,
			depacketizer->frameBufferLength, depacketizer->rgbData, 0,
			JPEG_DECODER_COLOR_RGB);
	
#ifdef ZAURUS
    render(depacketizer->rgbData, width, height, x_off + 3, y_off - 22, landscape);
#endif        
}

void jpeg_depacketizer_close( JPEG_DEPACKETIZER *depacketizer) {
    if( depacketizer != NULL) {
        if( depacketizer->frameBuffer != NULL) {
	    MNI_FREE( depacketizer->frameBuffer);
	}

	if( depacketizer->jpgdecoder != NULL) {
	    JPEG_To_RGB_free( depacketizer->jpgdecoder);
	}

	if( depacketizer->rgbData != NULL) {
	    MNI_FREE( depacketizer->rgbData);
	}

	if( depacketizer->lastJFIFHeader != NULL) {
	    MNI_FREE( depacketizer->lastJFIFHeader);
	}

	MNI_FREE( depacketizer);
    }
}
