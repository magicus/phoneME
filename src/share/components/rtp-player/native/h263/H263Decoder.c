/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
#include "mni.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dllindex.h"
#include "vvenc.h"
#include "h261defs.h"
/* #include "jni-util.h" */
#include "rtp.h"
#include "h263_decoder.h"

/* YUV to RGB conversion */
typedef  unsigned char 	ubyte;
typedef	 unsigned int	uint32;
typedef	 int		nint;

void createUVTable( void);

void c_yuv_to_rgb (ubyte *plm, ubyte *pcr, ubyte *pcb,
		   uint32 *prgb, nint xcnt, nint ycnt, nint shsz, nint dhsz, nint XBGR);

uint32 UVTAB[65536];
uint32 SATR[512];
uint32 SATG[512];
uint32 SATB[512];

//XXX sbd: no definition ?
#define ntohl(x) (0)

#define JAVA_STRUCT_STRING              "nativeData"
#define JAVA_PICTURE_DESC_STRING        "pictureDesc"


extern void FreeVarStructsDecoder( H261Decoder *);
extern U32 H263FinishPicture( H261Decoder *, PICTURE *);
extern U32 H263PacketDecode( H261Decoder *, int, char *, U32, PICTURE *, int, char *);
extern U32 H263_1998PacketDecode( H261Decoder *, int, char *, U32, PICTURE *, int, char *);
extern int AllocVarStructsDecoder( H261Decoder *, short);
extern void InitReconTables( void);
extern void InitDecodeTable( void);
extern void InitFindSC( void);

static void *vvMalloc( size_t size) {
    char    *ptr;
    U32     addr;

#ifdef FOR_MAC
    ptr = NewPtrClear(size+4);
#else
    ptr = (char *)MNI_CALLOC( size + 4, 1 ); /* calloc should zero memory */
#endif
    addr = (U32)ptr & 0x3L;

//   	assert(addr == 0);	// To trap the problem described above

    /* Assume returned address can have low-order bits 00, 01, 10, or 11. */
    if ( addr == 1 ) ptr += 3;
    else if (addr == 2 ) ptr += 2;
    else if ( addr == 3 ) ptr += 1;
    else {}

    return ( (void *) ptr );
//  return ( (void *)MNI_CALLOC( size, 1 ) );
}

static void vvFree(void *ptr) {
#ifdef FOR_MAC
	DisposePtr(ptr);
#else
	MNI_FREE(ptr);
#endif
}


static void convertVivoToYUV(char *VivoFrame, char *YUVFrame, int width, int
			     height, int color) {
    /* 
     * Vivo YUV frames have rather bizzarre layout: the chrominance planes are
     * interlaced. Our renderer needs to deinterlace them.
     * sbd: it is really Ycrcb and not YUV
     */
    int imgSize = width*height;
    int i;
    char *cr, *cb;

    /* copy Y plane */
    memcpy(YUVFrame, VivoFrame, imgSize);

    if (color) {
	/*deinterlace color */
	cr = YUVFrame + imgSize;
	cb = cr + (imgSize >> 2);
	VivoFrame += imgSize;
	width >>= 1;
	for (i = (height >> 1); i > 0; i--) {
	    memcpy(cr, VivoFrame, width);
	    VivoFrame+= width; cr += width;

	    memcpy(cb, VivoFrame, width);
	    VivoFrame+= width; cb += width;
	}
    } else {
	/* create a gray scale image*/
	memset((YUVFrame + imgSize), 0x80, (imgSize >> 1));
    }
}

void initComp(COMPONENT *c, int x, int y) {
    memset(c, 0, sizeof(COMPONENT));
    c->nhor = x;
    c->nvert = y;
    c->hoffset = x;
    c->ptr = MNI_MALLOC(x*y);
}

void initPict(PICTURE *p, int x, int y) {
    memset(p, 0, sizeof(PICTURE));
    p->color = 1;
    p->xSize = x;
    p->ySize = y;
    initComp(&(p->y), x, y);
    initComp(&(p->cr), x, y);
    initComp(&(p->cb), x, y);
}



void dumpFrame(char *name, PICTURE *pic) {
    char n[100];
    FILE *f;
    static int nframeas = 0;
    sprintf(n, "%s%d.pgm", name, nframeas++);
    f = fopen(n, "w");
    fprintf(f, "P5\n%d %d 255\n", (int)pic->y.nhor, (int)pic->y.nvert);
    fwrite(pic->y.ptr, pic->y.nhor* pic->y.nvert, 1, f);
    fclose(f);
}

void dumpData(char *name, char *data, int len) {
    char n[100];
    FILE *f;
    static int nframeas = 0;
    sprintf(n, "%s%d", name, nframeas++);
    f = fopen(n, "w");
    fwrite(data, len, 1, f);
    fclose(f);
}

void dumpQuant(char *name, H261Decoder *dec) {
    char n[100];
    FILE *f;
    MACROBLOCK_DESCR *mb;
    int i;
    static int nframeas = 0;
    sprintf(n, "%s%d", name, nframeas++);
    f = fopen(n, "w");
    mb = dec->mb;
    for (i = 0; i < dec->maxMbnum; i++) {
	fprintf(f, "%02d ", mb[i].quant);
	if ((i % 22) == 21)
	    fprintf(f, "\n");
    }
    fclose(f);
}

void initNativeDecoderClass() {
  InitFindSC();
  InitDecodeTable();

  createUVTable();
}

H263_DECODER * initNativeDecoder( int width, int height) {
    int formatCap;
    int out = 0;
    int retval;
    PICTURE_DESCR *pictureDesc;
    H263_DECODER *h263_decoder;
    H261Decoder *dec;

    InitReconTables();

    /* align width, height, and transform them into frameCap */
    width  = (width  + 0xf) & 0xfffffff0;
    height = (height + 0xf) & 0xfffffff0;
    formatCap = (width * height) >> 8;
    /* transform the number of macroblocks into format*/
    if (formatCap <= 6336)
	out = CIF16;
    if (formatCap <= 1584)
	out = CIF4;
    if (formatCap <= 396)
	out = CIF;
    if (formatCap <= 99)
	out = QCIF;
    if (formatCap <= 48)
	out = SQCIF;

    /* allocate PICTURE_DESCR for use with the decoder */
    pictureDesc = (PICTURE_DESCR *)MNI_MALLOC(sizeof(PICTURE_DESCR));
    if (NULL==pictureDesc) return NULL;

    memset(pictureDesc, 0, sizeof(PICTURE_DESCR));
    pictureDesc->rows = height;
    pictureDesc->cols = width;

    /* allocate the decoder itself */
    dec = (H261Decoder *)vvMalloc( sizeof ( H261Decoder));
    if (NULL==dec) return NULL;

    retval = AllocVarStructsDecoder(dec, out);
    if (FAILED_MALLOC==retval) return NULL;

    dec->bsAbs.byteptr = (U8 *) vvMalloc( MAX_BITSTR_SIZE);
    if (NULL==dec->bsAbs.byteptr) return NULL;

    dec->bsAbs.bitptr = 0;

    h263_decoder = (H263_DECODER *) MNI_MALLOC( sizeof( H263_DECODER));
    if( h263_decoder == NULL) return NULL;

    h263_decoder->pictureDesc = pictureDesc;
    h263_decoder->decoder = dec;
    h263_decoder->yuvBuffer = NULL;
    h263_decoder->yuvBufferLength = 0;
    h263_decoder->width = 0;
    h263_decoder->height = 0;

    return h263_decoder;
}

int decodePacketNative( H263_DECODER *h263_decoder,
                        unsigned char *inputData,
                        int inputOffset,
                        int inputLength, 
                        unsigned char *outputData,
                        unsigned char *packetHeader,
                        int packetOffset,
                        int sync,
                        int h263_1998) {
    H261Decoder *dec;
    PICTURE newPic;
    int headerLength,
        PBFrameCap,
        retval;

    char * inDataArr, *outDataArr;
    char *header;
    char *headerCopy;
    int imageSize;
    int i, k, val;

    /* get inputs */
    dec = h263_decoder->decoder;
    header = (char *) packetHeader;
    headerCopy = header;
    header = &header[packetOffset];

    if (h263_1998) {
	int plen = ((header[0]&0x01) << 5) | ((header[1]&0xf8) >> 3);
	headerLength = 2 + plen;
	if (header[0] & 0x02) { /* Video Redundancy present */
	    headerLength++;
	}
    } else {
	if (header[0] & 0x80) { /* mode b or c*/
	    if (header[0] & 0x40) { /* mode c */
		headerLength = 12;
	    } else { /* mode b*/
		headerLength = 8;
	    }
	} else { /* mode a */
	    headerLength = 4;
	}
    }

    /* get input and output arrays */

    inDataArr = (char *) inputData;
    outDataArr = (char *) outputData;

    /* get the decoder state */
    PBFrameCap = h263_decoder->PBFrameCap;

    newPic.color = 1;

    if (h263_1998) {
	retval = H263_1998PacketDecode(dec,
				       headerLength,
				       header,
				       PBFrameCap,
				       &newPic,
				       inputLength,
				       &inDataArr[inputOffset]);     
    } else {
	retval = H263PacketDecode(dec,
				  headerLength,
				  header,
				  PBFrameCap,
				  &newPic,
				  inputLength,
				  &inDataArr[inputOffset]);
    }

    if ((retval != YES) && sync) {
	retval = H263FinishPicture(dec, &newPic);
    }

    if (retval == YES) {
        imageSize = newPic.y.nhor * newPic.y.nvert;

        convertVivoToYUV((char *) newPic.y.ptr,
			 outDataArr,
			 newPic.y.nhor,
			 newPic.y.nvert,
			 newPic.color);

	c_yuv_to_rgb( outDataArr, 
                      outDataArr + imageSize + (imageSize >> 2),
		      outDataArr + imageSize,
		      h263_decoder->rgbBuffer32, newPic.y.nhor >> 4, newPic.y.nvert >>1,
		      newPic.y.nhor, newPic.y.nhor,
                      0);

	// temp fix 32-bit to 24-bit conversion
	// this code is not very efficient and should
	// be part of convertVivoToYUV

	k = 0;
	for( i = 0; i < h263_decoder->rgbBuffer32Length; i++) {
	    val = h263_decoder->rgbBuffer32[ i];

	    h263_decoder->rgbBuffer[ k++] = (u_char) val;
	    h263_decoder->rgbBuffer[ k++] = (u_char) (val >> 8);
	    h263_decoder->rgbBuffer[ k++] = (u_char) (val >> 16);
	}	

	h263_decoder->bsStart = 0;
	h263_decoder->nextGOB = 0;
    }

    return (retval == YES);
}

void closeNativeDecoder( H263_DECODER *h263_decoder) {
    H261Decoder *dec;

    if( h263_decoder != NULL) {
        if( h263_decoder->yuvBuffer != NULL) MNI_FREE( h263_decoder->yuvBuffer);
	if( h263_decoder->rgbBuffer32 != NULL) MNI_FREE( h263_decoder->rgbBuffer32);
	if( h263_decoder->rgbBuffer != NULL) MNI_FREE( h263_decoder->rgbBuffer);

	if( h263_decoder->pictureDesc != NULL) {
	    MNI_FREE( h263_decoder->pictureDesc);
        }

	dec = (H261Decoder *) h263_decoder->decoder;    

	if( dec != NULL) {
	    FreeVarStructsDecoder(dec);
	    vvFree(dec->bsAbs.byteptr);
	    vvFree(dec);
	}
    }

    MNI_FREE( h263_decoder);
}

char uvtablecreated = 0;

#define REDPOS   16
#define GREENPOS 8
#define BLUEPOS  0


#define GETRGB(rgb, red, green, blue, rsh, bsh) { \
    red   = (rgb >> rsh) & 0xFF; \
    green = (rgb >> 8) & 0xFF; \
    blue  = (rgb >> bsh) & 0xFF; \
}

#define ONEPIX(r, g, b, y, dst) { \
    dst = SATR[r + y] |          \
	  SATG[g + y] |          \
	  SATB[b + y];           \
}


void updateUVTable() {
#define LIMIT(x) (((x) < -128) ? -128 : (((x) > 127) ? 127 : (x)))
    
    /* Fill up the UV lookup table */
    uint32 r, g, b;
    int u, v;
    register double uf, vf;
    for (u = 0; u < 256; ++u) {
	
	uf = (double) (u - 128);
	for (v = 0; v < 256; ++v) {
	    vf = (double) (v - 128);
	    r = LIMIT(vf * 1.402) + 128;
	    b = LIMIT(uf * 1.772) + 128;
	    g = LIMIT(uf * -0.34414 - vf * 0.71414) + 128;
	    /* Store XBGR in UVTAB table */
	    UVTAB[(u << 8)|v] =
		((r & 0xFF) <<  0) |
		((g & 0xFF) <<  8) |
		((b & 0xFF) << 16);
	}
    }
}

void updateSaturationTable() {
    uint32 val, s;
    for (s = 0; s < 256; s++) {
	val = s;
	if (val > 255)
	    val = 255;
	SATR[s+128] = (val & 0xFF) << REDPOS;

	val = s;
	if (val > 255)
	    val = 255;
	SATG[s+128] = (val & 0xFF) << GREENPOS;

	val = s;
	if (val > 255)
	    val = 255;
	SATB[s+128] = (val & 0xFF) << BLUEPOS;
    }

    for (s = 0; s < 128; s++) {
	SATR[s] = SATR[128];
	SATG[s] = SATG[128];
	SATB[s] = SATB[128];

	SATR[s+382] = SATR[381];
	SATG[s+382] = SATG[381];
	SATB[s+382] = SATB[381];
    }    
}

void createUVTable() {
    updateUVTable();
    updateSaturationTable();
}

void c_yuv_to_rgb( ubyte *py, ubyte* pv, ubyte *pu, uint32 *prgb, nint xcnt, nint ycnt,
		   nint srcStride, nint dstStride, nint XBGR) {

    register uint32 *line1 = prgb;
    register uint32 *line2 = line1 + dstStride;
    register ubyte  *py2   = py + srcStride;
    uint32 width = xcnt * 16;
    uint32 height = ycnt * 2;
    register uint32 len;
    register uint32 r, g, b, sum;
    register uint32 w = width;
    register uint32 rsh = 0;
    register uint32 bsh = 16;
    
    if (!XBGR) {
	rsh = 16;
	bsh = 0;
    }
    
    for (len = width * height; len > 0; len -= 16) {
#define FOUR411(n) \
	sum = UVTAB[(pu[(n)/2] << 8) | pv[(n)/2]]; \
	GETRGB(sum, r, g, b, rsh, bsh) \
	ONEPIX(r, g, b, py[(n)], line1[(n)]) \
	ONEPIX(r, g, b, py[(n)+1], line1[(n)+1]) \
	ONEPIX(r, g, b, py2[(n)], line2[(n)]) \
	ONEPIX(r, g, b, py2[(n)+1], line2[(n)+1])

        FOUR411(0)
	FOUR411(2)
	FOUR411(4)
        FOUR411(6)

        line1 += 8;
	line2 += 8;

	py += 8;
	py2 += 8;
	pu += 4;
	pv += 4;
	w -= 8;
	if (w <= 0) {
	    w = width;
	    line1 += w;
	    line2 += w;
	    py += w;
	    py2 += w;
	}
    }
}
