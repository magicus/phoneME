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

#include <string.h>

#include <jar.h>
#include <pcsl_memory.h>
#include <midp_logging.h>

#include "imgdcd_intern_image_decode.h"

#define UNDF_CHUNK 0x00000000
#define IHDR_CHUNK 0x49484452
#define PLTE_CHUNK 0x504C5445
#define IDAT_CHUNK 0x49444154
#define IEND_CHUNK 0x49454E44

#define tRNS_CHUNK 0x74524E53

#define CT_PALETTE  0x01
#define CT_COLOR    0x02
#define CT_ALPHA    0x04

#define freeBytes(p) pcsl_mem_free((p))

typedef struct _pngData {
      signed int   width;
      signed int   height;

    /* current location, for writing pixels */
             int   y;
    unsigned int   scanlength;
    unsigned short bytesPerPixel;

    /* palette in stream is RGB (3 bytes) but we store it as XRGB (int) */
             long *palette;
    unsigned short paletteLength;

    unsigned char *trans;
    unsigned short transLength;

    unsigned char  pass;         /* pass #, for interlace loading        */

    unsigned char  depth;        /* bits per pixel: 1,2,4,8,16           */
    unsigned char  colorType;    /* 1=palette  2=hasColor  4=hasAlpha    */
    unsigned char  compress;     /* compression type, must be zero       */
    unsigned char  filter;       /* filter type, must be zero            */
    unsigned char  interlace;    /* interlacing: 0 (none) or 1 (adam7)   */
    unsigned int   lineBytes[7];
    unsigned int   passSize[7];
} pngData;

static unsigned long readHeader(imageSrcPtr, long, pngData *, unsigned long);
static unsigned long readPalette(imageSrcPtr, long, pngData *,
                                 long *, unsigned long);
static unsigned long readTransPal(imageSrcPtr, long, pngData *,
                                  unsigned char *, unsigned long);
static bool handleImageData(unsigned char *, int, imageDstPtr, pngData *);
static unsigned long getInt(imageSrcPtr);
static unsigned long skip(imageSrcPtr, int, unsigned long);
static bool getChunk(imageSrcPtr, unsigned long *, long *);
static bool signatureOK(imageSrcPtr);
static bool headerOK(pngData *);
static unsigned long init_CRC(unsigned long);
static bool check_CRC(imageSrcPtr, unsigned long);
static unsigned long update_crc(unsigned long, unsigned char *, int);
static int findPassWidth(int, pngData *);

typedef struct _iid {
    imageSrcPtr src;
    long clen;
} idatInflateData;

static int
PNGdecodeImage_getByte(void *p)
{
    idatInflateData *d = (idatInflateData *)p;
    imageSrcPtr src = d->src;
    unsigned long chunkType = UNDF_CHUNK;
    unsigned int  byte;

    /* while, because it's possible to have 0-length chunks! */
    while (d->clen == 0) {
        /* unsigned long CRC = */ (void)getInt(src);

        getChunk(src, &chunkType, &(d->clen));

        if (chunkType != IDAT_CHUNK) {
            return -1;
        }
    }

    --(d->clen);
    byte = src->getByte(src);
    return byte;
}

static long
PNGdecodeImage_getBytes(void *p, unsigned char *buffer, long count)
{
    idatInflateData *d = (idatInflateData *)p;
    imageSrcPtr src = d->src;
    unsigned long chunkType = UNDF_CHUNK;

    /* while, because it's possible to have 0-length chunks! */
    while (d->clen == 0) {
        /* unsigned long CRC = */ (void)getInt(src);

        getChunk(src, &chunkType, &(d->clen));

        if (chunkType != IDAT_CHUNK) {
            return 0;
        }
    }

    if (d->clen <= (int)count) {
        count = d->clen;
    }

    count = (long)(src->getBytes(src, buffer, (int)count));
    d->clen -= (int)count;
    return count;
}

static long PNGdecodeImage_size(void* p) {
    /* This function is not used by inflateData. */
    (void)p;
    return 0;
}

/* Emulates fseek. See fseek doc for offset and whence values. */
static int PNGdecodeImage_seek(void* p, long offset, int whence) {
    /* This function is not used by inflateData */
    (void)p;
    (void)offset;
    (void)whence;
    return -1;
}

/* returns a memory handle, call addrFromHandle to use */
static void* allocFunction(void* state, int n) {
    (void)state;
    return pcsl_mem_malloc(n);
}

/* handle, is a memory handle */
static void freeFunction(void* state, void* handle) {
    (void)state;
    pcsl_mem_free(handle);
}

/* This function is to support heaps that compact memory. */
static void* addrFromHandleFunction(void* state, void* handle) {
    (void)state;
    return handle;
}

bool get_decoded_png_imagesize(imageSrcPtr src, int* width, int* height) {
    unsigned long chunkType = UNDF_CHUNK;
    long chunkLength;
    unsigned long CRC;
    pngData data;

    if (!signatureOK(src)) {
        /* not a PNG image */
        return FALSE;
    }

    if (!getChunk(src, &chunkType, &chunkLength)) {
        return FALSE;
    }

    CRC = init_CRC(chunkType);
    if (chunkType != IHDR_CHUNK) {
        return FALSE;
    }        

    if (chunkLength < 13) {
        return FALSE;
    }

    memset(&data, 0, sizeof(data));

    CRC = readHeader(src, chunkLength, &data, CRC);

    if (!headerOK(&data)) {
        return FALSE;
    }

    *width = data.width;
    *height = data.height;
    return TRUE;
}

static
bool
PNGdecodeImage_real(imageSrcPtr src, imageDstPtr dst, 
		    long *paletteData, unsigned char *transData) {

    bool       OK = TRUE;
    bool saw_IDAT = FALSE,
             saw_PLTE = FALSE,
             saw_tRNS = FALSE;
    pngData      data;

    unsigned long chunkType = UNDF_CHUNK;
    long chunkLength;
    unsigned long CRC;

    if (!signatureOK(src)) {
        goto formaterror;               /* not a PNG image */
    }

    memset(&data, 0, sizeof(data));

    while (getChunk(src, &chunkType, &chunkLength)) {
        CRC = init_CRC(chunkType);

        if (chunkType == IHDR_CHUNK) {
            /* size of header is known.  headerOK => IHDR_CHUNK already seen */
            if ((chunkLength < 13) || headerOK(&data)) {
                goto formaterror;
            }

            CRC = readHeader(src, chunkLength, &data, CRC);

            if (!headerOK(&data)) {
                goto formaterror;
            }

            dst->setSize(dst, data.width, data.height);
            if ((data.colorType & CT_COLOR) == 0) {
                /* grayscale--set up a palette */
                int n = 0;
                long _p[4];

                switch (data.depth) {
                case 1:
                    _p[0] = 0;
                    _p[1] = 0xffffff;
                    n = 2;
                    dst->setColormap(dst, _p, n);
                    break;

                case 2:
                    _p[0] = 0;
                    _p[1] = 0x555555;
                    _p[2] = 0xaaaaaa;
                    _p[3] = 0xffffff;
                    n = 4;
                    dst->setColormap(dst, _p, n);
                    break;

                case 4:
                    {
                    long p[16];

                    for (n = 0; n < 16; ++n) {
                        p[n] = n*0x111111;
                    }

                    dst->setColormap(dst, p, n);
                    }
                    break;

                default:
                    {
                    long p[256];

                    for (n = 0; n < 256; ++n) {
                        p[n] = (n << 16) | (n << 8) | n;
                    }

                    dst->setColormap(dst, p, n);
                    }
                    break;
                }
            }
        } else if (chunkType == PLTE_CHUNK) {

            if ((chunkLength < 0) || (chunkLength > (256 * 3)) ||
                    ((chunkLength % 3) != 0)) {
                goto formaterror;
            }

            if ((data.paletteLength > 0) || saw_IDAT || saw_tRNS) {
                goto formaterror;
            }

            CRC = readPalette(src, chunkLength, &data, paletteData, CRC);
            if (data.palette == NULL) {
                goto formaterror;
            }

            if (data.colorType & CT_PALETTE) {
                dst->setColormap(dst, data.palette, data.paletteLength);
            }

            saw_PLTE = TRUE;
        } else if (chunkType == tRNS_CHUNK) {

            if (saw_IDAT || data.colorType == 4 
                         || data.colorType == 6) {
                goto formaterror;
            }

            CRC = readTransPal(src, chunkLength, &data, transData, CRC);
            if (data.trans == NULL) {
                goto formaterror;
            }

            dst->setTransMap(dst, data.trans, data.transLength,
                             saw_PLTE ? data.paletteLength : -1);

            saw_tRNS = TRUE;
        } else if (chunkType == IDAT_CHUNK) {
            idatInflateData  _data;
            FileObj fileObj;
            HeapManObj heapManObj;

            int compLen = 0;
            int decompLen = data.scanlength * data.height;
            unsigned char *decompBuf;
            long startPos = 0, lastGoodPos = 0;

            saw_IDAT = TRUE;

            if (data.interlace) {
                int i;

                /* decompLen is harder to calculate--there are extra rows! */
                decompLen = 0;
                for (i = 0; i < 7; ++i) {
                    int off    = 7 >> (i/2);            /* 7 7 3 3 1 1 0 */
                    int shift  = 3 - (((i - 1) / 2));   /* 3 3 3 2 2 1 1 */
                    int height = ((data.height + off) >> shift);
                    int width  = findPassWidth(i, &data);

                    data.lineBytes[i] = width;
                    data.passSize[i]  = width * height;

                    decompLen += data.passSize[i];
                }
            } else {
                data.lineBytes[6] = data.scanlength;
                data.passSize[6]  = decompLen;
            }

            if ((data.colorType & CT_PALETTE) && (data.palette == NULL)) {
                goto formaterror;
            }

            /*
             * we need to decompress all of the IDAT chunks en masse.
             * first step is to find out how much actual data there is.
             * While we're at it, we can look at the CRCs for all of
             * the blocks.
             */

            _data.src  = src;
            _data.clen = chunkLength;

            lastGoodPos = startPos = src->getpos(src);
            do {
                /* Make sure we only process IDAT chunks */
                if (chunkType != IDAT_CHUNK) break;

                CRC = init_CRC(chunkType);
                compLen += chunkLength;
                CRC = skip(src, chunkLength, CRC);
                lastGoodPos = src->getpos(src);

                if (!check_CRC(src, CRC)) {
                    goto formaterror;
                }
            } while (getChunk(src, &chunkType, &chunkLength));


            /*
             * IMPORTANT: CRC now contains the correct value for
             * the last IDAT_CHUNK.  This is checked at the bottom
             * of the loop so it must contain the right value.
             */

            src->seek(src, startPos);    /* reset to the first IDAT_CHUNK */

            decompBuf = (unsigned char*)pcsl_mem_malloc(decompLen);
            if (decompBuf == NULL) {
                OK = FALSE;
		goto done;
            }

            /*
             * inflate ignores the method and flags
             */
            compLen -= 2;
	    PNGdecodeImage_getByte(&_data);
	    PNGdecodeImage_getByte(&_data);

            fileObj.state = &_data;
            fileObj.size = PNGdecodeImage_size;
            fileObj.read = PNGdecodeImage_getBytes;
            fileObj.seek = PNGdecodeImage_seek;
            fileObj.readChar = PNGdecodeImage_getByte;

            heapManObj.state = NULL;
            heapManObj.alloc = allocFunction;
            heapManObj.free = freeFunction;
            heapManObj.addrFromHandle = addrFromHandleFunction;

            /* subtract 4 bytes from compLen -- it's the ZLIB trailer */
            if (inflateData(&fileObj, &heapManObj, compLen - 4,
                             decompBuf, decompLen, 0) != 0) {
                freeBytes(decompBuf);
		goto formaterror;
	    }

            OK = handleImageData(decompBuf, decompLen, dst, &data);

            freeBytes(decompBuf);
            src->seek(src, lastGoodPos);
        } else if (chunkType == IEND_CHUNK) {
            /* shouldn't happen because getChunk checks for this! */
        } else {
            /* unrecognized -- skip */
            CRC = skip(src, chunkLength, CRC);
        }

        if (!check_CRC(src, CRC)) {
	    REPORT_WARN1(LC_LOWUI,
                "PNG data corrupted (CRC mismatch for a chunk of 0x%X type)",
                chunkType); 

            /* interrupt image decoding only if the chunk is critical */
            if (chunkType == IHDR_CHUNK) {
                goto formaterror;
            }
        }
    }

    // No chunks or other content follow the IEND chunk
    if (chunkType != IEND_CHUNK) {
        goto formaterror;
    }

 done:

    return OK;

 formaterror:
    OK = FALSE;
    goto done;

}

bool
decode_png_image(imageSrcPtr src, imageDstPtr dst) {
    long * paletteData;
    unsigned char * transData;

    paletteData = pcsl_mem_malloc(sizeof(long) * 256);
    if (paletteData == NULL) {
	/* out of memory */
	return FALSE;
    } else {
	transData = pcsl_mem_malloc(sizeof(unsigned char) * 256);
	if (transData == NULL) {
	    /* out of memory */
	    pcsl_mem_free(paletteData);
	    return FALSE;
	} else {
	    bool retval = PNGdecodeImage_real(src, dst, 
						  paletteData, transData);
	    pcsl_mem_free(paletteData);
	    pcsl_mem_free(transData);
	    return retval;
	}
    }
}


/*
* Interlacing parameters for ADAM7 interlace pattern:
*
* 1 6 4 6 2 6 4 6
* 7 7 7 7 7 7 7 7
* 5 6 5 6 5 6 5 6
* 7 7 7 7 7 7 7 7
* 3 6 4 6 3 6 4 6
* 7 7 7 7 7 7 7 7
* 5 6 5 6 5 6 5 6
* 7 7 7 7 7 7 7 7
*
*/
static const char * const pStartX = "\000\004\000\002\000\001\000\000";
static const char * const pIncX   = "\010\010\004\004\002\002\001\001";
/* pass number            1   2   3   4   5   6   7   - */
/* static const char * const pStartY = "\000\000\004\000\002\000\001\000"; */
/* static const char * const pIncY   = "\010\010\010\004\004\002\002\001"; */

static int
findPassWidth(int pass, pngData *data)
{
    int ix       = ((pass < 0) || (pass > 7)) ? 7 : pass;
    int pixels   = (data->width - pStartX[ix] + pIncX[ix] - 1) / pIncX[ix];
    int channels = (data->colorType & CT_COLOR) ? 3 : 1;
    if (data->colorType & CT_ALPHA) ++channels;
    if (data->colorType & CT_PALETTE) channels = 1; /* palette is always 1 channel */

    if (pixels <= 0) {
        return 0;
    }

    return 1 + ((pixels*data->depth*channels + 7) >> 3);
}

static unsigned long
readHeader(imageSrcPtr src, long length, pngData *data, unsigned long CRC)
{
    unsigned char buf[13];

    src->getBytes(src, buf, sizeof(buf));

    data->width     = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    data->height    = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    data->depth     = buf[8];
    data->colorType = buf[9];
    data->compress  = buf[10];
    data->filter    = buf[11];
    data->interlace = buf[12];

#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_LOWUI, 
		"reading PNG image, %dx%dx%d%s%s%s%s\n",
		data->width, data->height, data->depth,
		data->colorType & CT_PALETTE ? " palette" : "",
		data->colorType & CT_COLOR ? " color" : " grayscale",
		data->colorType & CT_ALPHA ? " w/alpha" : "",
		data->interlace ? ", INTERLACED" : "");
#endif

    if (data->interlace) {
        data->pass = 0;
    } else {
        data->pass = 7;
    }

    switch (data->colorType) {
    case 0:
    case 3:
        data->bytesPerPixel = (data->depth + 7) / 8;
        break;

    case 2:
        data->bytesPerPixel = 3*data->depth / 8;
        break;

    case 4:
        data->bytesPerPixel = data->depth / 4;
        break;

    case 6:
        data->bytesPerPixel = data->depth / 2;
        break;
    }

    data->scanlength = findPassWidth(6, data); /* pass 6 => no windowing */

    CRC = update_crc(CRC, buf, sizeof(buf));
    length -= sizeof(buf);

    while (length > 0) {
        int n = (length < (long)sizeof(buf)) ? length : (long)sizeof(buf);
        src->getBytes(src, buf, n);
        CRC = update_crc(CRC, buf, n);
    }

    return CRC;
}

static unsigned long
readPalette(imageSrcPtr src, long length, pngData *data,
            long *buffer, unsigned long CRC)
{
    int i;

    data->paletteLength = (unsigned short)(length / 3);
    data->palette = buffer;

    for (i = 0; i < data->paletteLength; ++i) {
        unsigned char buf[3];
        src->getBytes(src, buf, 3);
        CRC = update_crc(CRC, buf, 3);

        data->palette[i] = (buf[0] << 16) | (buf[1] << 8) | buf[2];
    }

    return CRC;
}

static unsigned long
readTransPal(imageSrcPtr src, long length, pngData *data,
            unsigned char *buffer, unsigned long CRC)
{
    int i;

    data->transLength = (unsigned short)length;
    data->trans = buffer;

    if (data->colorType & CT_PALETTE) { /* palette */
        for (i = 0; i < data->transLength; i++) {
            unsigned char buf[1];
            buf[0] = src->getByte(src);
            CRC = update_crc(CRC, buf, 1);
            data->trans[i] = buf[0];
        }
    } else if ((data->colorType & CT_COLOR) == 0) { /* grayscale */
        unsigned char buf[2];
        src->getBytes(src, buf, 2);
        CRC = update_crc(CRC, buf, 2);

        data->trans[0] = buf[0];
        data->trans[1] = buf[1];
        data->transLength = 2;
    } else if (data->colorType & CT_COLOR) { /* truecolor */

        unsigned char buf[6];
        src->getBytes(src, buf, 6);
        CRC = update_crc(CRC, buf, 6);

        data->trans[0] = buf[0];
        data->trans[1] = buf[1];
        data->trans[2] = buf[2];
        data->trans[3] = buf[3];
        data->trans[4] = buf[4];
        data->trans[5] = buf[5];
        data->transLength = 6;
    }

    return CRC;
}

static void
applyFilter(int filterType, unsigned char *buf, int n,
            unsigned char *prev, int bpp)
{
    int x;

    if (filterType == 0) {
        return;
    } else if (prev == NULL) {
        if (filterType == 4) {
            filterType = 1;
        } else if (filterType == 3) {
            for (x = bpp; x < n; ++x) {
                buf[x] += buf[x - bpp] >> 1;
            }
            return;
        } else if (filterType == 2) {
            return;
        }
    }

    switch (filterType) {
    case 1:
        /*
         * We start at x == bpp because for x < bpp, buf[x - bpp] is
         * to be treated as zero.
         */
        for (x = bpp; x < n; ++x) {
            buf[x] += buf[x - bpp];
        }
        break;

    case 2:
        for (x = 0; x < n; ++x) {
            buf[x] += prev[x];
        }
        break;

    case 3:
        /*
         * There is no valid "buf[x - bpp]" until (x == bpp), so the
         * first few values are treated as zero.
         */

        for (x = 0; x < bpp; ++x) {
            buf[x] += prev[x] >> 1;
        }

        /*
         * But for x >= bpp we can do the full computation.
         */
        for (x = bpp; x < n; ++x) {
            buf[x] += (prev[x] + (buf[x - bpp])) >> 1;
        }
        break;

    case 4:
        /*
         * There is no valid "buf[x - bpp]" or "prev[x - bpp]" until
         * (x == bpp).  In the meantime, those values are treated as
         * zeroes, which causes the Paeth predictor to degenerate to
         * the value prev[x].  (Set a and c to zero and try it.)
         */
        for (x = 0; x < bpp; ++x) {
            buf[x] += prev[x];
        }

        /*
         * Now we can do the full computation.
         */
        for (x = bpp; x < n; ++x) {
            int a, b, c, p, pa, pb, pc;
            a = buf[x - bpp];
            b = prev[x];
            c = prev[x - bpp];
            p = a + b - c;
            pa = p > a ? p - a : a - p;
            pb = p > b ? p - b : b - p;
            pc = p > c ? p - c : c - p;
            buf[x] += ((pa <= pb) && (pa <= pc)) ? a : ((pb <= pc) ? b : c);
        }
        break;

    default:
        REPORT_WARN1(LC_LOWUI, "Illegal filter value %d\n", filterType); 
        break;
    }
}

static void
filterAllRows(unsigned char *pixels, pngData *data) {

    if (data->interlace) {
        int pass = 0;
        while (pass <= 6) {
            int n = data->lineBytes[pass];
            unsigned char *end = pixels + data->passSize[pass];
            unsigned char *rowBuf = pixels;
            unsigned char *prevRow = NULL;

            while (rowBuf < end) {
                applyFilter(rowBuf[0], rowBuf + 1, n - 1,
                            prevRow, data->bytesPerPixel);

                prevRow = rowBuf + 1;
                rowBuf += n;
            }

            pixels = end;
            ++pass;
        }
    } else {
        unsigned char *rowBuf = pixels;
        unsigned char *prevRow = NULL;
        int n = data->lineBytes[6];
        int y;

        for (y = 0; y < data->height; ++y) {
            applyFilter(rowBuf[0], rowBuf + 1, n - 1,
                        prevRow, data->bytesPerPixel);

            prevRow = rowBuf + 1;
            rowBuf += n;
        }
    }
}


#define INIT_TRANS_PACK_RGB(b, x, a)        \
    (x) = ((b) > 8) ? 1 : 0;                \
    (a) = ((b) > 8) ? 6 : 3

#define TRANS_PACK_RGB(d, s, x, a, tmap)       \
    *(d)++ = (s)[0];                           \
    *(d)++ = (s)[1 << x];                      \
    *(d)++ = (s)[2 << x];                      \
    if(x) {                                    \
    *(d)++ = (memcmp(s,tmap,a)) ? 0xFF: 0x00;  \
    } else {                                   \
    *(d)++ = ((s[0] == tmap[1]) &&             \
              (s[1] == tmap[3]) &&             \
              (s[2] == tmap[5]))               \
             ? 0x00: 0xFF;                     \
    }                                          \
    (s) += (a)

#define INIT_TRANS_PACK_GS(b, x, a) \
    (x) = ((b) > 8) ? 1 : 0;        \
    (a) = ((b) > 8) ? 2 : 1

#define TRANS_PACK_GS(d, s, x, a, tmap) \
     *(d)++ = (s)[0];                   \
     if(x) {                            \
       *(d)++ = ((s[0] == tmap[0]) &&   \
                 (s[1] == tmap[1]) )    \
                ? 0x00: 0xFF;           \
     } else {                           \
        *(d)++ = ((s[1] == tmap[1]))    \
                 ? 0x00: 0xFF;          \
     }                                  \
    (s) += (a)

#define INIT_TRANS_UNPACK_GS(b, x, m) \
    (x) = (8 - (b));                  \
    (m) = 0xff ^ (0xff >> (x))

#define TRANS_UNPACK_GS(d, s, x, m, b, x0, m0, tmap) \
    *(d)++ = ((*s) & (m)) >> (x);                    \
    *(d) = (*(d-1) == tmap[1])                       \
             ? 0x00 : 0xFF;                          \
    (d)++;                                           \
    if (x) {                                         \
        (m) >>= (b);                        \
        (x) -= (b);                         \
    } else {                                \
        (x) = (x0);                         \
        (m) = (m0);                         \
        ++(s);                              \
    }


#define INIT_UNPACK(b, x, m)          \
    (x) = (8 - (b));                  \
    (m) = 0xff ^ (0xff >> (x))

#define UNPACK(d, s, x, m, b, x0, m0) \
    *(d)++ = ((*s) & (m)) >> (x);     \
    if (x) {                          \
        (m) >>= (b);                  \
        (x) -= (b);                   \
    } else {                          \
        (x) = (x0);                   \
        (m) = (m0);                   \
        ++(s);                        \
    }

#define INIT_PACK3(b, t, x, a)      \
    (x) = ((b) > 8) ? 1 : 0;        \
    (a) = ((t) ? 4 : 3) << (x)

#define PACK3(d, s, x, a, cta)      \
    *(d)++ = (s)[0];                \
    *(d)++ = (s)[1 << x];           \
    *(d)++ = (s)[2 << x];           \
    if (cta) *(d)++ = (s)[3 << x];  \
    (s) += (a)

#define INIT_PACK1(b, t, x, a)      \
    (x) = ((b) > 8) ? 1 : 0;        \
    (a) = ((t) ? 2 : 1) << (x)

#define PACK1(d, s, x, a, cta)      \
    *(d)++ = (s)[0];                \
    if (cta) *(d)++ = (s)[1 << x];  \
    (s) += (a)

static void
unpack1(unsigned char *dst, unsigned char *src, pngData *data)
{
  int i;
  int shift, inc, x0;
  unsigned char mask, m0;

    if (data->trans != NULL && data->colorType != 3) {

      /*transparency is possible for grayscale, palette and rgb triples.
        compare the pixel values with the 16-bit per channel transparency 
        map. if it matches then write alpha as 0x00 else write alpha as 0xff
      */

      if (data->colorType == 2) {
        /* each pixel is RGB with 8 or 16 bits per channel */

        INIT_TRANS_PACK_RGB(data->depth, shift, inc);

        for (i = 0; i < data->width; ++i) {
          TRANS_PACK_RGB(dst, src, shift, inc, data->trans);
        }

      }
      else if (data->colorType == 0) {
        /* each pixel is a grayscale value.
           if it is with a bit depth less than 8 we have to unpack the data.
        */

        if(data->depth < 8) {
          /* unpack function that handles transparency.
             takes 1, 2, 4 bit grayscale values with transparency info
             and converts them to an 8-bit grayscale with 8-bit alpha value.
          */

          INIT_TRANS_UNPACK_GS(data->depth, shift, mask);
          x0 = shift;
          m0 = mask;

          for (i = 0; i < data->width; ++i) {
            TRANS_UNPACK_GS(dst, src, shift, mask, 
                            data->depth, x0, m0, data->trans);
          }

        } else {
          INIT_TRANS_PACK_GS(data->depth, shift, inc);
          for (i = 0; i < data->width; ++i) {
            TRANS_PACK_GS(dst, src, shift, inc, data->trans);
          }
        }

      }

    } else if (data->depth < 8) {

      INIT_UNPACK(data->depth, shift, mask);
      x0 = shift;
      m0 = mask;

      for (i = 0; i < data->width; ++i) {
        UNPACK(dst, src, shift, mask, data->depth, x0, m0);
      }
    } else if ((data->colorType & 3) == 2) { /* color types: 2, 6 */
      INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift, inc);

      for (i = 0; i < data->width; ++i) {
        PACK3(dst, src, shift, inc, data->colorType & CT_ALPHA);
      }
    } else { /* color types: 3, 4 (depth >= 8) */
      INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift, inc);

      for (i = 0; i < data->width; ++i) {
        PACK1(dst, src, shift, inc, data->colorType & CT_ALPHA);
      }
    }
}

static void
unpack2(unsigned char *dst,
        unsigned char *src0, unsigned char *src1,
        pngData *data)
{
    int i;
    int shift0, inc0, x0;
    int shift1, inc1;
    unsigned char mask0, m0;
    unsigned char mask1;

    if (data->trans != NULL && data->colorType != 3) {

      if (data->colorType == 2) {

        INIT_TRANS_PACK_RGB(data->depth, shift0, inc0);
        INIT_TRANS_PACK_RGB(data->depth, shift1, inc1);

        for (i = 0; i < data->width; ++i) {
          TRANS_PACK_RGB(dst, src0, shift0, inc0, data->trans);

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src1, shift1, inc1, data->trans);
          }
        }

      }
      else if (data->colorType == 0) {

        if(data->depth < 8) {

          INIT_TRANS_UNPACK_GS(data->depth, shift0, mask0);
          INIT_TRANS_UNPACK_GS(data->depth, shift1, mask1);

          x0 = shift0;
          m0 = mask0;

          for (i = 0; i < data->width; ++i) {
            TRANS_UNPACK_GS(dst, src0, shift0, mask0, 
                            data->depth, x0, m0, data->trans);

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }
          }

        } else {

          INIT_TRANS_PACK_GS(data->depth, shift0, inc0);
          INIT_TRANS_PACK_GS(data->depth, shift1, inc1);

          for (i = 0; i < data->width; ++i) {

            TRANS_PACK_GS(dst, src0, shift0, inc0, data->trans);

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }

          }
        }
      }

    } else if (data->depth < 8) {

        INIT_UNPACK(data->depth, shift0, mask0);
        INIT_UNPACK(data->depth, shift1, mask1);

        x0 = shift0;
        m0 = mask0;

        for (i = 0; i < data->width; ++i) {
            UNPACK(dst, src0, shift0, mask0, data->depth, x0, m0);

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }
        }
    } else if ((data->colorType & 3) == 2) {
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift1, inc1);

        for (i = 0; i < data->width; ++i) {
            PACK3(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    } else {
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift1, inc1);

        for (i = 0; i < data->width; ++i) {
            PACK1(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    }
}

static void
unpack3(unsigned char *dst,
        unsigned char *src0, unsigned char *src1, unsigned char *src2,
        pngData *data)
{
    int i;
    int shift0, inc0, x0;
    int shift1, inc1;
    int shift2, inc2;
    unsigned char mask0, m0;
    unsigned char mask1;
    unsigned char mask2;

    /* alternate 0 1 2 1 */

    if (data->trans != NULL && data->colorType != 3) {

      if (data->colorType == 2) {

        INIT_TRANS_PACK_RGB(data->depth, shift0, inc0);
        INIT_TRANS_PACK_RGB(data->depth, shift1, inc1);
        INIT_TRANS_PACK_RGB(data->depth, shift2, inc2);

        for (i = 0; i < data->width; ++i) {
          TRANS_PACK_RGB(dst, src0, shift0, inc0, data->trans);

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src1, shift1, inc1, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src2, shift2, inc2, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src1, shift1, inc1, data->trans);
          }

        }

      }
      else if (data->colorType == 0) {

        if(data->depth < 8) {

          INIT_TRANS_UNPACK_GS(data->depth, shift0, mask0);
          INIT_TRANS_UNPACK_GS(data->depth, shift1, mask1);
          INIT_TRANS_UNPACK_GS(data->depth, shift2, mask2);

          x0 = shift0;
          m0 = mask0;

          for (i = 0; i < data->width; ++i) {
            TRANS_UNPACK_GS(dst, src0, shift0, mask0, 
                            data->depth, x0, m0, data->trans);

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src2, shift2, mask2, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }

          }

        } else {
          INIT_TRANS_PACK_GS(data->depth, shift0, inc0);
          INIT_TRANS_PACK_GS(data->depth, shift1, inc1);
          INIT_TRANS_PACK_GS(data->depth, shift2, inc2);

          for (i = 0; i < data->width; ++i) {

            TRANS_PACK_GS(dst, src0, shift0, inc0, data->trans);

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src2, shift2, inc2, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }
          }
        }

      }

    } else if (data->depth < 8) {
        INIT_UNPACK(data->depth, shift0, mask0);
        INIT_UNPACK(data->depth, shift1, mask1);
        INIT_UNPACK(data->depth, shift2, mask2);

        x0 = shift0;
        m0 = mask0;

        for (i = 0; i < data->width; ++i) {
            UNPACK(dst, src0, shift0, mask0, data->depth, x0, m0);

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src2, shift2, mask2, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }
        }
    } else if ((data->colorType & 3) == 2) {
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift1, inc1);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift2, inc2);

        for (i = 0; i < data->width; ++i) {
            PACK3(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    } else {
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift1, inc1);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift2, inc2);

        for (i = 0; i < data->width; ++i) {
            PACK1(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    }
}

static void
unpack4(unsigned char *dst,
        unsigned char *src0, unsigned char *src1,
        unsigned char *src2, unsigned char *src3,
        pngData *data)
{
    int i;
    int shift0, inc0, x0;
    int shift1, inc1;
    int shift2, inc2;
    int shift3, inc3;
    unsigned char mask0, m0;
    unsigned char mask1;
    unsigned char mask2;
    unsigned char mask3;

    /* alternate 0 1 2 1 3 1 2 1 */

    if (data->trans != NULL && data->colorType != 3) {

      if (data->colorType == 2) {

        INIT_TRANS_PACK_RGB(data->depth, shift0, inc0);
        INIT_TRANS_PACK_RGB(data->depth, shift1, inc1);
        INIT_TRANS_PACK_RGB(data->depth, shift2, inc2);
        INIT_TRANS_PACK_RGB(data->depth, shift3, inc3);

        for (i = 0; i < data->width; ++i) {
          TRANS_PACK_RGB(dst, src0, shift0, inc0, data->trans);

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src1, shift1, inc1, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src2, shift2, inc2, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src1, shift1, inc1, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src3, shift3, inc3, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src2, shift2, inc2, data->trans);
          }

          if ((++i) < data->width) {
          TRANS_PACK_RGB(dst, src2, shift2, inc2, data->trans);
          }

        }

      }
      else if (data->colorType == 0) {

        if(data->depth < 8) {

          INIT_TRANS_UNPACK_GS(data->depth, shift0, mask0);
          INIT_TRANS_UNPACK_GS(data->depth, shift1, mask1);
          INIT_TRANS_UNPACK_GS(data->depth, shift2, mask2);
          INIT_TRANS_UNPACK_GS(data->depth, shift3, mask3);

          x0 = shift0;
          m0 = mask0;

          for (i = 0; i < data->width; ++i) {
            TRANS_UNPACK_GS(dst, src0, shift0, mask0, 
                            data->depth, x0, m0, data->trans);

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src2, shift2, mask2, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src3, shift3, mask3, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src2, shift2, mask2, 
                              data->depth, x0, m0, data->trans);
            }

            if((++i) < data->width) {
              TRANS_UNPACK_GS(dst, src1, shift1, mask1, 
                              data->depth, x0, m0, data->trans);
            }

          }

        } else {
          INIT_TRANS_PACK_GS(data->depth, shift0, inc0);
          INIT_TRANS_PACK_GS(data->depth, shift1, inc1);
          INIT_TRANS_PACK_GS(data->depth, shift2, inc2);
          INIT_TRANS_PACK_GS(data->depth, shift3, inc3);

          for (i = 0; i < data->width; ++i) {

            TRANS_PACK_GS(dst, src0, shift0, inc0, data->trans);

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src2, shift2, inc2, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src3, shift3, inc3, data->trans);
            }

            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src2, shift2, inc2, data->trans);
            }
            if((++i) < data->width) {
              TRANS_PACK_GS(dst, src1, shift1, inc1, data->trans);
            }

          }
        }

      }

    } else if (data->depth < 8) {
        INIT_UNPACK(data->depth, shift0, mask0);
        INIT_UNPACK(data->depth, shift1, mask1);
        INIT_UNPACK(data->depth, shift2, mask2);
        INIT_UNPACK(data->depth, shift3, mask3);

        x0 = shift0;
        m0 = mask0;

        for (i = 0; i < data->width; ++i) {
            UNPACK(dst, src0, shift0, mask0, data->depth, x0, m0);

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src2, shift2, mask2, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src3, shift3, mask3, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src2, shift2, mask2, data->depth, x0, m0);
            }

            if ((++i) < data->width) {
                UNPACK(dst, src1, shift1, mask1, data->depth, x0, m0);
            }
        }
    } else if ((data->colorType & 3) == 2) {
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift1, inc1);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift2, inc2);
        INIT_PACK3(data->depth, data->colorType & CT_ALPHA, shift3, inc3);

        for (i = 0; i < data->width; ++i) {
            PACK3(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src3, shift3, inc3, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK3(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    } else {
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift0, inc0);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift1, inc1);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift2, inc2);
        INIT_PACK1(data->depth, data->colorType & CT_ALPHA, shift3, inc3);

        for (i = 0; i < data->width; ++i) {
            PACK1(dst, src0, shift0, inc0, data->colorType & CT_ALPHA);

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src3, shift3, inc3, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src2, shift2, inc2, data->colorType & CT_ALPHA);
            }

            if ((++i) < data->width) {
                PACK1(dst, src1, shift1, inc1, data->colorType & CT_ALPHA);
            }
        }
    }
}


static bool
handleImageData(unsigned char *pixels, int pixelsLength,
                imageDstPtr dst, pngData *data)
{
    int pixelSize = ((data->colorType & (CT_PALETTE | CT_COLOR)) ? 3 : 1) +
                    (((data->colorType & CT_ALPHA) || (data->trans != NULL)) 
		     ? 1 : 0 );
    int rgba = data->colorType;

    int sendDirect = FALSE;

    unsigned char *scanline = NULL;
    unsigned char *passes[7];
    int y;

    (void)pixelsLength;

    filterAllRows(pixels, data);

    if (data->interlace) {
        scanline = (unsigned char *) pcsl_mem_malloc(data->width * pixelSize);
        if (scanline == NULL) {
            return FALSE;
        }

        passes[0] = pixels;
        for (y = 1; y < 7; ++y) {
            passes[y] = passes[y - 1] + data->passSize[y - 1];
        }
    } else {
        passes[6] = pixels;
    }

    if ( (data->depth == 8) &&
         ( (data->colorType & CT_PALETTE) || (data->trans == NULL) ) ) {
      /* if data is in the desired format
         i.e. 8 bit Palette or 8 bit RGB/gs without transparency*/
        sendDirect = TRUE;
    } else if (scanline == NULL) {
        scanline = (unsigned char *) pcsl_mem_malloc(data->width * pixelSize);
        if (scanline == NULL) {
            return FALSE;
        }
    }

    for (y = 0; y < data->height; ++y) {
        if ((y & 1) || !data->interlace) {
            if (sendDirect) {
                dst->sendPixels(dst, y, passes[6] + 1, rgba);
            } else {
                unpack1(scanline, passes[6] + 1, data);
                dst->sendPixels(dst, y, scanline, rgba);
            }

            passes[6] += data->lineBytes[6];
        } else {
            switch (y & 6) {
            case 2:
            case 6:
                unpack2(scanline, passes[4] + 1, passes[5] + 1, data);
                passes[4] += data->lineBytes[4];
                passes[5] += data->lineBytes[5];
                break;

            case 4:
                unpack3(scanline,
                        passes[2] + 1, passes[5] + 1, passes[3] + 1,
                        data);
                passes[2] += data->lineBytes[2];
                passes[5] += data->lineBytes[5];
                passes[3] += data->lineBytes[3];
                break;

            case 0:
                unpack4(scanline,
                        passes[0] + 1, passes[5] + 1,
                        passes[3] + 1, passes[1] + 1,
                        data);
                passes[0] += data->lineBytes[0];
                passes[5] += data->lineBytes[5];
                passes[3] += data->lineBytes[3];
                passes[1] += data->lineBytes[1];
                break;
            }

            dst->sendPixels(dst, y, scanline, rgba);
        }
    }

    if (scanline != NULL) {
        pcsl_mem_free(scanline);
    }

    return TRUE;
}


static unsigned long
getInt(imageSrcPtr src)
{
    unsigned long n;

    n  = ((unsigned long)src->getByte(src) << 24);
    n |= ((unsigned long)src->getByte(src) << 16);
    n |= ((unsigned long)src->getByte(src) <<  8);
    n |= ((unsigned long)src->getByte(src) <<  0);

    return n;
}

static bool
endOfData(imageSrcPtr src)
{
    return src->endOfData(src);
}

static bool
getChunk(imageSrcPtr src, unsigned long *chunkType, long *chunkLength)
{
    if (endOfData(src)) {
        return FALSE;
    }

    *chunkLength = getInt(src);
    *chunkType   = getInt(src);

    return (*chunkType != IEND_CHUNK) ? TRUE : FALSE;
}

static unsigned long
skip(imageSrcPtr src, int nBytes, unsigned long CRC)
{
    static unsigned char buf[64];
    while (nBytes > 0) {
        int n = (nBytes > (long)sizeof(buf)) ? (long)sizeof(buf) : nBytes;
        src->getBytes(src, buf, n);
        CRC = update_crc(CRC, buf, n);
        nBytes -= n;
    }

    return CRC;
}


static bool
signatureOK(imageSrcPtr src)
{
    unsigned char buf[8];
    static unsigned char png_header[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

    src->getBytes(src, buf, sizeof(buf));

    return (memcmp(png_header, buf, sizeof(png_header)) == 0) ? TRUE : FALSE;
}

static bool
headerOK(pngData *data)
{
    /*
     * simple sanity checks.  Most are obvious.
     * (data->depth & (data->depth-1)) is checking to see that
     * depth is a simple power of two, i.e. only one bit is set
     */
    if (
            ((data->width <= 0) || (data->height <= 0))
         || (data->compress != 0)
         || (data->filter != 0)
         || ((data->interlace != 0) && (data->interlace != 1))
         || ((data->depth > 16) || (data->depth & (data->depth-1)))
        ) {

        return FALSE;
    }

    /* based on colorType, only certain depths are allowed. */
    /* depth > 16 was already guarded against above.        */
    switch (data->colorType) {
    case 0:
        return TRUE;

    case 2:
    case 4:
    case 6:
        return (data->depth >= 8) ? TRUE : FALSE;

    case 3: 
        return (data->depth <= 8) ? TRUE : FALSE;

    default: 
        return FALSE;

    }
}

static unsigned long
init_CRC(unsigned long chunkType)
{
    unsigned char buf[4];

    buf[0] = (unsigned char)((chunkType >> 24) & 0xff);
    buf[1] = (unsigned char)((chunkType >> 16) & 0xff);
    buf[2] = (unsigned char)((chunkType >>  8) & 0xff);
    buf[3] = (unsigned char)((chunkType >>  0) & 0xff);

    return update_crc(0xffffffffL, buf, 4);
}

static bool
check_CRC(imageSrcPtr src, unsigned long CRC)
{
    unsigned long transmitted_CRC = getInt(src);

    CRC = (~CRC) & 0xffffffffl;

#if REPORT_LEVEL <= LOG_INFORMATION
    if (transmitted_CRC != CRC) {
        reportToLog(LOG_INFORMATION, LC_LOWUI, "CRC mismatch: xmitted 0x%08lx "
		    "vs calculated 0x%08lx ", transmitted_CRC, CRC);
    }
#endif

    return (transmitted_CRC == CRC) ? TRUE : FALSE;
}


/***** CRC support, copied from PNG spec *****/


/* Table of CRCs of all 8-bit messages.  */
static unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void
make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xedb88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/*
* Update a running CRC with the bytes buf[0..len-1]
*/
static unsigned long
update_crc(unsigned long crc, unsigned char *buf, int len)
{
    unsigned long c = crc;
    int n;

    if (!crc_table_computed) {
        make_crc_table();
    }

    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }

    return c;
}
