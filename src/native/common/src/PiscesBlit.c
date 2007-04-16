/*
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


#include <PiscesBlit.h>

#include <PiscesUtil.h>
#include <PiscesRenderer.h>

#include <PiscesSysutils.h>

#define MIN_ALPHA 0
#define MAX_ALPHA 256
#define HALF_ALPHA (MAX_ALPHA >> 1)
#define ALPHA_SHIFT 8

static INLINE void blendSrcOver888(jint *intData, jint aval,
                            jint cred, jint cgreen, jint cblue);
static INLINE void blendSrcOver8888(jint *intData, jint aval,
                             jint sred, jint sgreen, jint sblue);
static INLINE void blendSrcOver8888_pre(jint *intData, jint aval, jint sred, 
                                 jint sgreen, jint sblue); 
                                                             
static INLINE void blendSrcOver565(jshort *shortData, jint pix);
static INLINE void blendSrcOver8(jbyte *byteData, jint aval, jint cgray);

static INLINE void blendSrc8888(jint *intData, jint aval, jint aaval,
                         jint sred, jint sgreen, jint sblue);
                         
static INLINE void blendSrc8888_pre(jint *intData, jint aval, jint raaval, jint sred, 
                             jint sgreen, jint sblue);
                             
                             
static INLINE void blendLine(void *data, jint imageType, jint offset, jint stride,
                      jint length, jint alpha, jint red, jint green, jint blue);

static jlong lmod(jlong x, jlong y);
static jint interp(jint x0, jint x1, jint frac);

void
fillRectSrcOver(Renderer* rdr,
                void *data, int imageType,
                jint imageOffset,
                jint imageScanlineStride,
                jint imagePixelStride,
                jint scrOrient,
                jint width, jint height,
                jint x0, jint y0, jint x1, jint y1,
                jint cred, jint cgreen, jint cblue) {
    jint ix0, iy0, ix1, iy1;
    jint xmask_l, xmask_r, ymask_t, ymask_b;
    jint offset;
    jint i, j;

    jint tw, th;

    jint *intData = NULL;
    unsigned short *shortData = NULL;
    jbyte *byteData = NULL;
    jint intVal = 0;
    unsigned short shortVal = (unsigned short)0;
    jbyte byteVal = (jbyte)0;

    jint alphaUL, alphaUR, alphaLL, alphaLR;

    jint RECT_LR_SIDE_ALPHA_SHIFT = ALPHA_SHIFT - rdr->_SUBPIXEL_LG_POSITIONS_X;
    jint RECT_TB_SIDE_ALPHA_SHIFT = ALPHA_SHIFT - rdr->_SUBPIXEL_LG_POSITIONS_Y;
    jint RECT_CORNER_ALPHA_SHIFT  = ALPHA_SHIFT -
                                    (rdr->_SUBPIXEL_LG_POSITIONS_X + 
                                     rdr->_SUBPIXEL_LG_POSITIONS_Y);

    // IMPL NOTE : to fix warning : unused parameter
    (void) scrOrient;

    xmask_l = rdr->_SUBPIXEL_POSITIONS_X - (x0 & rdr->_SUBPIXEL_MASK_X);
    if (xmask_l == rdr->_SUBPIXEL_POSITIONS_X) {
        xmask_l = 0;
    } else {
        x0 += rdr->_SUBPIXEL_POSITIONS_X;
    }
    xmask_r = x1 & rdr->_SUBPIXEL_MASK_X;

    ymask_t = rdr->_SUBPIXEL_POSITIONS_X - (y0 & rdr->_SUBPIXEL_MASK_X);
    if (ymask_t == rdr->_SUBPIXEL_POSITIONS_Y) {
        ymask_t = 0;
    } else {
        y0 += rdr->_SUBPIXEL_POSITIONS_Y;
    }

    ymask_b = y1 & rdr->_SUBPIXEL_MASK_X;

    ix0 = x0 >> rdr->_SUBPIXEL_LG_POSITIONS_X;
    ix1 = (x1 >> rdr->_SUBPIXEL_LG_POSITIONS_X) - 1;
    iy0 = y0 >> rdr->_SUBPIXEL_LG_POSITIONS_Y;
    iy1 = (y1 >> rdr->_SUBPIXEL_LG_POSITIONS_Y) -1;

    tw = ix1 - ix0;
    th = iy1 - iy0;

    if ((ix0 + tw) > width)
        width = width - ix0;
    else
        width = ix1 - ix0 + 1;


    if ((iy0 + th) > height)
        height = height - iy0;
    else
        height = iy1 - iy0 + 1;


    if (imageType == TYPE_INT_RGB || imageType == TYPE_INT_ARGB ||
        imageType == TYPE_INT_ARGB_PRE) {
        intData = (jint *)data;
        intVal = (jint)(0xff000000 | (cred << 16) | (cgreen << 8) | (cblue));
    } else if (imageType == TYPE_USHORT_565_RGB) {
        shortData = (unsigned short *)data;
        shortVal = (unsigned short)((cred << 11) | (cgreen << 5) | (cblue));
        intVal = (jint)shortVal & 0xffff;
    } else if (imageType == TYPE_BYTE_GRAY) {
        byteData = (jbyte *)data;
        cred = (int)(0.3f * cred + 0.59f * cgreen + 0.11f * cblue + 0.5f);
        byteVal = (jbyte)cred;
    }

    if (width > 0 && height > 0) {
        offset = imageOffset + iy0 * imageScanlineStride 
                             + ix0 * imagePixelStride;

        if (imageType == TYPE_INT_RGB || imageType == TYPE_INT_ARGB || 
            imageType == TYPE_INT_ARGB_PRE) {
            for (j = 0; j < height; j++) {
                int iidx = offset;
                for (i = 0; i < width; i++) {
                    intData[iidx] = intVal;
                    iidx += imagePixelStride;
                }
                offset += imageScanlineStride;
            }
        } else if (imageType == TYPE_USHORT_565_RGB) {
            unsigned short *sd = (unsigned short *)(shortData + offset);
            unsigned int cval = (shortVal << 16) | shortVal;

            for (j = height; j > 0; j--) {
                int cnt = width;
                int sdoff = 0;

                if (cnt & 0x01) {
                    *(unsigned short *)(sd + sdoff) = shortVal;
                    sdoff += imagePixelStride;
                    cnt--;
                }

                while (cnt) {
                    *(unsigned int *)(sd + sdoff) = cval;
                    sdoff += 2 * imagePixelStride;
                    cnt -= 2;
                }

                sd += imageScanlineStride;
            }

        } else if (imageType == TYPE_BYTE_GRAY) {
            for (j = 0; j < height; j++) {
                int iidx = offset;
                for (i = 0; i < width; i++) {
                    byteData[iidx] = byteVal;
                    iidx += imagePixelStride;
                }
                offset += imageScanlineStride;
            }
        }
    }

    if (xmask_l != 0) {
        int offset = imageOffset 
                     + iy0 * imageScanlineStride + (ix0 - 1) * imagePixelStride;
        jint alpha = xmask_l << RECT_LR_SIDE_ALPHA_SHIFT;
        blendLine(data, imageType, offset, imageScanlineStride, height,
                  alpha, cred, cgreen, cblue);
    }

    if (xmask_r != 0) {
        int offset = imageOffset 
                     + iy0 * imageScanlineStride + (ix1 + 1) * imagePixelStride;
        jint alpha = xmask_r << RECT_LR_SIDE_ALPHA_SHIFT;
        blendLine(data, imageType, offset, imageScanlineStride, height,
                  alpha, cred, cgreen, cblue);
    }

    if (ymask_t != 0) {
        int offset = imageOffset
                     + (iy0 - 1) * imageScanlineStride + ix0 * imagePixelStride;
        jint alpha = ymask_t << RECT_TB_SIDE_ALPHA_SHIFT;
        blendLine(data, imageType, offset, imagePixelStride, width,
                  alpha, cred, cgreen, cblue);
    }

    if (ymask_b != 0) {
        int offset = imageOffset 
                     + (iy1 + 1) * imageScanlineStride + ix0 * imagePixelStride;
        jint alpha = ymask_b << RECT_TB_SIDE_ALPHA_SHIFT;
        blendLine(data, imageType, offset, imagePixelStride, width,
                  alpha, cred, cgreen, cblue);
    }

    alphaUL = xmask_l * ymask_t << RECT_CORNER_ALPHA_SHIFT;
    alphaUR = xmask_r * ymask_t << RECT_CORNER_ALPHA_SHIFT;
    alphaLL = xmask_l * ymask_b << RECT_CORNER_ALPHA_SHIFT;
    alphaLR = xmask_r * ymask_b << RECT_CORNER_ALPHA_SHIFT;

    switch (imageType) {
    case TYPE_INT_RGB:
        if (alphaUL > 0) {
            blendSrcOver888(&intData[imageOffset 
                                     + (iy0 - 1) * imageScanlineStride 
                                     + (ix0 - 1) * imagePixelStride],
                            alphaUL, cred, cgreen, cblue);
        }
        if (alphaUR > 0) {
            blendSrcOver888(&intData[imageOffset 
                                     + (iy0 - 1) * imageScanlineStride 
                                     + (ix1 + 1) * imagePixelStride],
                            alphaUR, cred, cgreen, cblue);
        }
        if (alphaLL > 0) {
            blendSrcOver888(&intData[imageOffset 
                                     + (iy1 + 1) * imageScanlineStride 
                                     + (ix0 - 1) * imagePixelStride],
                            alphaLL, cred, cgreen, cblue);
        }
        if (alphaLR > 0) {
            blendSrcOver888(&intData[imageOffset 
                                     + (iy1 + 1) * imageScanlineStride 
                                     + (ix1 + 1) * imagePixelStride],
                            alphaLR, cred, cgreen, cblue);
        }
        break;

    case TYPE_INT_ARGB:
        if (alphaUL > 0) {
            blendSrcOver8888(&intData[imageOffset 
                                      + (iy0 - 1) * imageScanlineStride 
                                      + (ix0 - 1) * imagePixelStride],
                             alphaUL, cred, cgreen, cblue);
        }
        if (alphaUR > 0) {
            blendSrcOver8888(&intData[imageOffset 
                                      + (iy0 - 1) * imageScanlineStride 
                                      + (ix1 + 1) * imagePixelStride],
                             alphaUR, cred, cgreen, cblue);
        }
        if (alphaLL > 0) {
            blendSrcOver8888(&intData[imageOffset 
                                      + (iy1 + 1) * imageScanlineStride 
                                      + (ix0 - 1) * imagePixelStride],
                             alphaLL, cred, cgreen, cblue);
        }
        if (alphaLR > 0) {
            blendSrcOver8888(&intData[imageOffset 
                                      + (iy1 + 1) * imageScanlineStride 
                                      + (ix1 + 1) * imagePixelStride],
                             alphaLR, cred, cgreen, cblue);
        }
        break;
    
    case TYPE_INT_ARGB_PRE:
        if (alphaUL > 0) {
            blendSrcOver8888_pre(&intData[imageOffset 
                                      + (iy0 - 1) * imageScanlineStride 
                                      + (ix0 - 1) * imagePixelStride],
                             alphaUL, cred, cgreen, cblue);
        }
        if (alphaUR > 0) {
            blendSrcOver8888_pre(&intData[imageOffset 
                                      + (iy0 - 1) * imageScanlineStride 
                                      + (ix1 + 1) * imagePixelStride],
                             alphaUR, cred, cgreen, cblue);
        }
        if (alphaLL > 0) {
            blendSrcOver8888_pre(&intData[imageOffset 
                                      + (iy1 + 1) * imageScanlineStride 
                                      + (ix0 - 1) * imagePixelStride],
                             alphaLL, cred, cgreen, cblue);
        }
        if (alphaLR > 0) {
            blendSrcOver8888_pre(&intData[imageOffset 
                                      + (iy1 + 1) * imageScanlineStride 
                                      + (ix1 + 1) * imagePixelStride],
                             alphaLR, cred, cgreen, cblue);
        }
        break;
        
    case TYPE_USHORT_565_RGB:
        if (alphaUL > 0) {
            blendSrcOver565(
                    (jshort*)&shortData[imageOffset 
                                        + (iy0 - 1) * imageScanlineStride
                                        + (ix0 - 1) * imagePixelStride],
                    (alphaUL << 24) | intVal);
        }
        if (alphaUR > 0) {
            blendSrcOver565(
                    (jshort*)&shortData[imageOffset 
                                        + (iy0 - 1) * imageScanlineStride
                                        + (ix1 + 1) * imagePixelStride],
                    (alphaUR<<24) | intVal);
        }
        if (alphaLL > 0) {
            blendSrcOver565(
                    (jshort*)&shortData[imageOffset 
                                        + (iy1 + 1) * imageScanlineStride
                                        + (ix0 - 1) * imagePixelStride],
                    (alphaLL << 24) | intVal);
        }
        if (alphaLR > 0) {
            blendSrcOver565(
                    (jshort*)&shortData[imageOffset 
                                        + (iy1 + 1) * imageScanlineStride
                                        + (ix1 + 1) * imagePixelStride],
                    (alphaLR << 24) | intVal);
        }
        break;

    case TYPE_BYTE_GRAY:
        if (alphaUL > 0) {
            blendSrcOver8(&byteData[imageOffset 
                                    + (iy0 - 1) * imageScanlineStride 
                                    + (ix0 - 1) * imagePixelStride],
                          alphaUL, cred);
        }
        if (alphaUR > 0) {
            blendSrcOver8(&byteData[imageOffset 
                                    + (iy0 - 1) * imageScanlineStride 
                                    + (ix1 + 1) * imagePixelStride],
                          alphaUR, cred);
        }
        if (alphaLL > 0) {
            blendSrcOver8(&byteData[imageOffset 
                                    + (iy1 + 1) * imageScanlineStride 
                                    + (ix0 - 1) * imagePixelStride],
                          alphaLL, cred);
        }
        if (alphaLR > 0) {
            blendSrcOver8(&byteData[imageOffset 
                                    + (iy1 + 1) * imageScanlineStride 
                                    + (ix1 + 1) * imagePixelStride],
                          alphaLR, cred);
        }
        break;
    }
}

void 
blitSrc888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;

    cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;
    
    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = *a++ << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver888(&intData[iidx], aval, cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitSrc8888_pre(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;
    
    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;
    
    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a & 0xff];
            if (aval == MAX_ALPHA) {
                // alpha == 1 (255), thus we can use cval directly
                intData[iidx] = cval;
            } else {
                blendSrc8888_pre(&intData[iidx], aval,
                             256 - (*a << aaAlphaShift), 
                             cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
            ++a;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitSrc8888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;
    
    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;
    
    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a & 0xff];
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else {
                blendSrc8888(&intData[iidx], aval, 
                             256 - (*a << aaAlphaShift), 
                             cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
            ++a;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitSrc565(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint aidx, iidx, aval;
    jshort cval;

    jshort *shortData = (jshort *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jint cred5 = rdr->_cred;
    jint cgreen6 = rdr->_cgreen;
    jint cblue5 = rdr->_cblue;

    jbyte *a, *am;

    cval = (jshort)((cred5 << 11) | (cgreen6 << 5) | (cblue5));

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = *a++ << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                shortData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver565(&shortData[iidx], 
                                (aval << 24) | (cval & 0xffff));
            }
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitSrc8(Renderer *rdr, jint height) {
    jint i, j;
    jint minX, maxX, w;
    jint aidx, iidx, aval;
    jbyte cval;

    jbyte *byteData = (jbyte *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;

    jint cgray = (int)(0.3f * cred + 0.59f * cgreen + 0.11f * cblue + 0.5f);
    cval = (jbyte)cgray;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        for (i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
            aval = alpha[aidx] << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                byteData[iidx] = cval;
            } else if (aval == 0) {
                continue;
            } else {
                blendSrcOver8(&byteData[iidx], aval, cgray);
            }
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrc888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx] | 0xff000000;

            aval = *a++ << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver888(&intData[iidx], aval, (cval >> 16) & 0xff,
                                (cval >> 8) & 0xff, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrc8888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            aa = alphaMap[(cval >> 24) & 0xff];

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrc8888(&intData[iidx], aval,
                             256 - (*a << aaAlphaShift), 
                             (cval >> 16) & 0xff, 
                             (cval >> 8) & 0xff, 
                             cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrc8888_pre(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;
    jint palpha, pred, pgreen, pblue;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            
            aa = alphaMap[(cval >> 24) & 0xff];

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                palpha = (cval >> 24) & 0xff;
                pred   = (cval >> 16) & 0xff;
                pred = (pred*palpha + 127)/255;
                pgreen = (cval >>  8) & 0xff;
                pgreen = (pgreen*palpha + 127)/255;
                pblue  = cval & 0xff;
                pblue = (pblue*palpha + 127)/255;
                
                intData[iidx] = (palpha << 24) | (pred << 16) | (pgreen << 8) |
                                pblue;
            } else if (aval > 0) {
                blendSrc8888_pre(&intData[iidx], aval,
                             256 - (*a << aaAlphaShift), 
                             (cval >> 16) & 0xff, 
                             (cval >> 8) & 0xff, 
                             cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrc565(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jshort *shortData = (jshort *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            jint cred5, cgreen6, cblue5;

            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            cred5 = _Pisces_convert8To5[(cval >> 16) & 0xff];
            cgreen6 = _Pisces_convert8To6[(cval >> 8) & 0xff];
            cblue5 = _Pisces_convert8To5[cval & 0xff];

            cval = ((cred5 << 11) | (cgreen6 << 5) | cblue5);

            aval = *a++ << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                shortData[iidx] = (jshort)cval;
            } else if (aval > 0) {
                blendSrcOver565(&shortData[iidx], 
                                (aval << 24) | (cval & 0xffff));
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrc8(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jbyte *byteData = (jbyte *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint aaAlphaShift = rdr->_AA_ALPHA_SHIFT;

    jbyte *a, *am;

    jint* paint = (jint *)rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            // gray = .3 * red + .59 * green + .11 * blue
            cval = (19961 * ((cval >> 16) & 0xff) +
                    38666 * ((cval >> 8) & 0xff) +
                    7209 * (cval & 0xff)) >> 16;

            aval = *a++ << aaAlphaShift;
            if (aval == MAX_ALPHA) {
                byteData[iidx] = (jbyte)cval;
            } else if (aval > 0) {
                blendSrcOver8(&byteData[iidx], aval, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitSrcOver888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;
    
    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a++ & 0xff];
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver888(&intData[iidx], aval, cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitSrcOver8888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    cval = 0xff000000 | (cred << 16) | (cgreen << 8) | cblue;
    
    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a++ & 0xff];
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver8888(&intData[iidx], aval, cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitSrcOver8888_pre(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint aidx, iidx, aval;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;

    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a++ & 0xff];
            if (aval == MAX_ALPHA) {
                intData[iidx] = 0xff000000 | (cred << 16) | (cgreen << 8) | 
                                cblue;
            } else if (aval > 0) {
                blendSrcOver8888_pre(&intData[iidx], aval, cred, cgreen, cblue);
            }
            iidx += imagePixelStride;
        }
        
        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitSrcOver565(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint aidx, iidx, aval;
    jshort cval;

    jshort *shortData = (jshort *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint cred5 = rdr->_cred;
    jint cgreen6 = rdr->_cgreen;
    jint cblue5 = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    jbyte *a, *am;

    cval = (jshort)((cred5 << 11) | (cgreen6 << 5) | (cblue5));

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            aval = alphaMap[*a++ & 0xff];
            if (aval == MAX_ALPHA) {
                shortData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver565(&shortData[iidx], 
                                (aval << 24) | (cval & 0xffff));
            }
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitSrcOver8(Renderer *rdr, jint height) {
    jint i, j;
    jint minX, maxX, w;
    jint aidx, iidx, aval;
    jbyte cval;

    jbyte *byteData = (jbyte *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint cred = rdr->_cred;
    jint cgreen = rdr->_cgreen;
    jint cblue = rdr->_cblue;
    jint *alphaMap = rdr->_colorAlphaMap;

    jint cgray = (int)(0.3f * cred + 0.59f * cgreen + 0.11f * cblue + 0.5f);
    cval = (jbyte)cgray;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        for (i = 0; i < w; i++, aidx++, iidx += imagePixelStride) {
            aval = alphaMap[alpha[aidx] & 0xff];
            if (aval == MAX_ALPHA) {
                byteData[iidx] = cval;
            } else if (aval == 0) {
                continue;
            } else {
                blendSrcOver8(&byteData[iidx], aval, cgray);
            }
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

static jint
pad(jint ifrac, jint cycleMethod) {
    switch (cycleMethod) {
    case CYCLE_NONE:
        if (ifrac < 0) {
            ifrac = 0;
        } else if (ifrac > 0xffff) {
            ifrac = 0xffff;
        }
        break;
    case CYCLE_REPEAT:
        ifrac &= 0xffff;
        break;
    case CYCLE_REFLECT:
        if (ifrac < 0) {
            ifrac = -ifrac;
        }
        ifrac &= 0x1ffff;
        if (ifrac > 0xffff) {
            ifrac = 0x1ffff - ifrac;
        }
        break;
    }

    return ifrac;
}

void
genLinearGradientPaint(Renderer *rdr, jint height) {
    jint paintOffset = 0;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jint minX, maxX, w;
    jlong frac;
    jint pidx;

    jint x, y;
    jint i, j;

    jint cycleMethod = rdr->_gradient_cycleMethod;
    jlong mx = rdr->_lg_mx;
    jlong my = rdr->_lg_my;
    jlong b = rdr->_lg_b;

    jint* paint = rdr->_paint;
    jint* colors = rdr->_gradient_colors;

    y = rdr->_currY;
    for (j = 0; j < height; j++, y++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        x = rdr->_currX + minX;
        pidx = paintOffset + minX;

        frac = x * mx + y * my + b;
        for (i = 0; i < w; i++, pidx++) {
            jint ifrac = pad((jint)frac, cycleMethod);
            ifrac >>= 16 - LG_GRADIENT_MAP_SIZE;
            paint[pidx] = colors[ifrac];

            frac += mx;
        }

        paintOffset += width;
    }
}

void
genRadialGradientPaint(Renderer *rdr, jint height) {
    jint cycleMethod = rdr->_gradient_cycleMethod;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;
    jint minX, maxX;
    jint paintOffset = 0;
    jint pidx;
    jint i, j, w;
    jint x, y;

    jfloat a00, a01, a02, a10, a11, a12;
    jfloat cx, cy, fx, fy, r, rsq;
    jfloat fcx, fcy, d;

    double txx, tyy, fxx, fyy, cfx, cfy;
    double A, B, C, Csq, U, dU, V, dV, ddV, tmp;
    double g;
    jint ifrac;

    jint* paint = rdr->_paint;
    jint* colors = rdr->_gradient_colors;

    a00 = rdr->_gradient_inverse_transform.m00 / 65536.0f;
    a01 = rdr->_gradient_inverse_transform.m01 / 65536.0f;
    a02 = rdr->_gradient_inverse_transform.m02 / 65536.0f;
    a10 = rdr->_gradient_inverse_transform.m10 / 65536.0f;
    a11 = rdr->_gradient_inverse_transform.m11 / 65536.0f;
    a12 = rdr->_gradient_inverse_transform.m12 / 65536.0f;

    cx = rdr->_rg_cx / 65536.0f;
    cy = rdr->_rg_cy / 65536.0f;
    fx = rdr->_rg_fx / 65536.0f;
    fy = rdr->_rg_fy / 65536.0f;
    r = rdr->_rg_radius / 65536.0f;
    rsq = r * r;

    fcx = fx - cx;
    fcy = fy - cy;
    d = (float)PISCESsqrt(fcx * fcx + fcy * fcy);
    if (d > r * 0.97f) {
        jfloat f = (r * 0.97f) / d;
        fx = cx + f * fcx;
        fy = cy + f * fcy;
    }

    y = rdr->_currY;
    for (j = 0; j < height; j++, y++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        pidx = paintOffset + minX;
        x = rdr->_currX + minX;

        txx = x * a00 + y * a01 + a02;
        tyy = x * a10 + y * a11 + a12;

        fxx = fx - txx;
        fyy = fy - tyy;
        A = fxx * fxx + fyy * fyy;
        cfx = cx - fx;
        cfy = cy - fy;
        B = 2.0f * (cfx * fxx + cfy * fyy);
        C = cfx * cfx + cfy * cfy - rsq;
        Csq = C * C;
        U = -B / (2.0f * C);
        dU = (a00 * cfx + a10 * cfy) / C;
        V = (B * B - 4.0f * A * C) / (4.0f * Csq);
        dV = (2.0f * a00 * a10 * cfx * cfy +
              a00 * (a00 * (cfx * cfx - C) - B * cfx + 2.0f * C * fxx) +
              a10 * (a10 * (cfy * cfy - C) - B * cfy + 2.0f * C * fyy)) / Csq;
        tmp = a10 * cfx - a00 * cfy;
        ddV = 2.0f * ((a00 * a00 + a10 * a10) * rsq - tmp * tmp) / Csq;

        for (i = 0; i < w; i++, pidx++) {
            g = U + PISCESsqrt(V);
            U += dU;
            V += dV;
            dV += ddV;

            ifrac = (jint)(65536.0 * g);
            ifrac = pad(ifrac, cycleMethod);
            ifrac >>= 16 - LG_GRADIENT_MAP_SIZE;
            paint[pidx] = colors[ifrac];
        }

        paintOffset += width;
    }
}

void
genTexturePaint(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, pidx;

    jint *paint = rdr->_paint;
    jint paintOffset = 0;
    jint paintStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jint *a, *am;

    jint x, y;
    jlong ltx, lty;
    jint* txtData = rdr->_texture_intData;

    y = rdr->_currY;
    for (j = 0; j < height; j++, y++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        pidx = paintOffset + minX;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        x = rdr->_currX + minX;

        ltx = x * rdr->_texture_m00 + y * rdr->_texture_m01 + rdr->_texture_m02;
        lty = x * rdr->_texture_m10 + y * rdr->_texture_m11 + rdr->_texture_m12;

        a = paint + pidx;
        am = a + w;
        while (a < am) {
            jint tx = (jint)(ltx >> 16);
            jint ty = (jint)(lty >> 16);

            jint hfrac = (jint)(ltx & 0xffff);
            jint vfrac = (jint)(lty & 0xffff);

            jboolean inBounds = XNI_TRUE;

            if ((tx & rdr->_texture_wmask) != 0) {
                if (tx < -1 || tx >= rdr->_texture_imageWidth) {
                    if (rdr->_texture_repeat) {
                        ltx = lmod(ltx, rdr->_texture_imageWidth << 16);
                        tx = (jint)(ltx >> 16);
                    } else {
                        inBounds = XNI_FALSE;
                    }
                }
            }
            if ((ty & rdr->_texture_hmask) != 0) {
                if (ty < -1 || ty >= rdr->_texture_imageHeight) {
                    if (rdr->_texture_repeat) {
                        lty = lmod(lty, rdr->_texture_imageHeight << 16);
                        ty = (jint)(lty >> 16);
                    } else {
                        inBounds = XNI_FALSE;
                    }
                }
            }

            if (inBounds) {
                jint sidx = (ty + 1) * rdr->_texture_stride + tx + 1;
                jint p00 = txtData[sidx];
                jint aa;
                if (rdr->_texture_interpolate) {
                    jint sidx2 = sidx + rdr->_texture_stride;
                    jint p01 = txtData[sidx + 1];
                    jint p10 = txtData[sidx2];
                    jint p11 = txtData[sidx2 + 1];

                    jint a00 = (p00 >> 24) & 0xff;
                    jint r00 = (p00 >> 16) & 0xff;
                    jint g00 = (p00 >> 8)  & 0xff;
                    jint b00 =  p00        & 0xff;

                    jint a01 = (p01 >> 24) & 0xff;
                    jint r01 = (p01 >> 16) & 0xff;
                    jint g01 = (p01 >> 8)  & 0xff;
                    jint b01 =  p01        & 0xff;

                    jint a0 = interp(a00, a01, hfrac);
                    jint r0 = interp(r00, r01, hfrac);
                    jint g0 = interp(g00, g01, hfrac);
                    jint b0 = interp(b00, b01, hfrac);

                    jint a10 = (p10 >> 24) & 0xff;
                    jint r10 = (p10 >> 16) & 0xff;
                    jint g10 = (p10 >> 8)  & 0xff;
                    jint b10 =  p10        & 0xff;

                    jint a11 = (p11 >> 24) & 0xff;
                    jint r11 = (p11 >> 16) & 0xff;
                    jint g11 = (p11 >> 8)  & 0xff;
                    jint b11 =  p11        & 0xff;

                    jint a1 = interp(a10, a11, hfrac);
                    jint r1 = interp(r10, r11, hfrac);
                    jint g1 = interp(g10, g11, hfrac);
                    jint b1 = interp(b10, b11, hfrac);

                    jint rr = interp(r0, r1, vfrac);
                    jint gg = interp(g0, g1, vfrac);
                    jint bb = interp(b0, b1, vfrac);

                    aa = interp(a0, a1, vfrac);
                    cval = (aa << 24) | (rr << 16) | (gg << 8) | bb;

                    assert(pidx >= 0);
                    assert(pidx < rdr->_paint_length / 4);

                    paint[pidx] = cval;
                } else {
                    assert(pidx >= 0);
                    assert(pidx < rdr->_paint_length / 4);

                    paint[pidx] = p00;
                }
            } else {
                assert(pidx >= 0);
                assert(pidx < rdr->_paint_length / 4);

                paint[pidx] = 0x00000000;
            }

            ++a;
            ++pidx;

            ltx += rdr->_texture_m00;
            lty += rdr->_texture_m10;
        }

        paintOffset += paintStride;
    }
}

void
blitPTSrcOver888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            aa = alphaMap[(cval >> 24) & 0xff];

            // Scale combined alpha into [0, MAX_ALPHA]
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver888(&intData[iidx], aval, (cval >> 16) & 0xff,
                                (cval >> 8) & 0xff, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitPTSrcOver8888(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            aa = alphaMap[(cval >> 24) & 0xff];

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                intData[iidx] = cval;
            } else if (aval > 0) {
                blendSrcOver8888(&intData[iidx], aval, (cval >> 16) & 0xff,
                                 (cval >> 8) & 0xff, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void 
blitPTSrcOver8888_pre(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jint *intData = rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;

    jint palpha, pred, pgreen, pblue;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            
            aa = alphaMap[(cval >> 24) & 0xff];

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                // premultiplied source components
                palpha  = (cval >> 24) & 0xff;
                pred    = ((((cval >> 16) & 0xff)*palpha + 127)/255);
                pgreen  = ((((cval >>  8) & 0xff)*palpha + 127)/255);
                pblue   = ((cval          & 0xff)*palpha + 127)/255;
                intData[iidx] = (palpha << 24) | (pred << 16) | (pgreen << 8) | 
                                pblue;
            } else if (aval > 0) {
                blendSrcOver8888_pre(&intData[iidx], aval, (cval >> 16) & 0xff,
                                 (cval >> 8) & 0xff, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitPTSrcOver565(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jshort *shortData = (jshort *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            jint cred5, cgreen6, cblue5;

            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            aa = alphaMap[(cval >> 24) & 0xff];
            cred5 = _Pisces_convert8To5[(cval >> 16) & 0xff];
            cgreen6 = _Pisces_convert8To6[(cval >> 8) & 0xff];
            cblue5 = _Pisces_convert8To5[cval & 0xff];

            cval = ((cred5 << 11) | (cgreen6 << 5) | cblue5);

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                shortData[iidx] = (jshort)cval;
            } else if (aval > 0) {
                blendSrcOver565(&shortData[iidx], 
                                (aval << 24) | (cval & 0xffff));
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
blitPTSrcOver8(Renderer *rdr, jint height) {
    jint j;
    jint minX, maxX, w;
    jint cval, aidx, iidx, aval, aa;

    jbyte *byteData = (jbyte *)rdr->_data;
    jint imageOffset = rdr->_currImageOffset;
    jint imageScanlineStride = rdr->_imageScanlineStride;
    jint imagePixelStride = rdr->_imagePixelStride;
    jbyte *alpha = rdr->_rowAA;
    jint alphaOffset = 0;
    jint alphaStride = rdr->_alphaWidth;
    jint width = rdr->_alphaWidth;
    jint *minTouched = rdr->_minTouched;
    jint *maxTouched = rdr->_maxTouched;

    jbyte *a, *am;
    jint *alphaMap = rdr->_paintAlphaMap;

    jint denom = rdr->_MAX_AA_ALPHA * 255;
    jint denom2 = denom / 2;

    jint* paint = (jint *)rdr->_paint;

    for (j = 0; j < height; j++) {
        minX = minTouched[j];
        maxX = maxTouched[j];
        aidx = alphaOffset + minX;
        iidx = imageOffset + minX * imagePixelStride;

        w = (maxX >= minX) ? (maxX - minX + 1) : 0;
        if ((w > 0) && (w + minX > width)) {
            w = width - minX;
        }

        a = alpha + aidx;
        am = a + w;
        while (a < am) {
            assert(aidx >= 0);
            assert(aidx < rdr->_paint_length / 4);

            cval = paint[aidx];
            aa = alphaMap[(cval >> 24) & 0xff];
            // gray = .3 * red + .59 * green + .11 * blue
            cval = (19961*((cval >> 16) & 0xff) +
                    38666*((cval >> 8) & 0xff) +
                    7209*(cval & 0xff)) >> 16;

            /* Scale combined alpha into [0, MAX_ALPHA] */
            aval = (aa * (*a & 0xff) * MAX_ALPHA + denom2) / denom;
            if (aval == MAX_ALPHA) {
                byteData[iidx] = (jbyte)cval;
            } else if (aval > 0) {
                blendSrcOver8(&byteData[iidx], aval, cval & 0xff);
            }

            ++a;
            ++aidx;
            iidx += imagePixelStride;
        }

        imageOffset += imageScanlineStride;
        alphaOffset += alphaStride;
    }
}

void
clearRect8888(Renderer *rdr, jint x, jint y, jint w, jint h) {
    jint cval = (rdr->_calpha << 24) | (rdr->_cred << 16) | 
                (rdr->_cgreen << 8) | rdr->_cblue;
    jint pixelStride = rdr->_imagePixelStride;
    jint scanlineSkip = rdr->_imageScanlineStride - w * pixelStride;
    jint* intData = (jint*)rdr->_data + rdr->_imageOffset +
                    y * rdr->_imageScanlineStride + x * pixelStride;

    for (; h > 0; --h) {
        jint w2;
        for (w2 = w; w2 > 0; --w2) {
            *intData = cval;
            intData += pixelStride;
        }
        intData += scanlineSkip;
    }
}

void
clearRect565(Renderer *rdr, jint x, jint y, jint w, jint h) {
    jshort cval = (jshort)((_Pisces_convert8To5[rdr->_cred] << 11) |
                           (_Pisces_convert8To6[rdr->_cgreen] << 5) |
                           _Pisces_convert8To5[rdr->_cblue]);
    jint pixelStride = rdr->_imagePixelStride;
    jint scanlineSkip = rdr->_imageScanlineStride - w * pixelStride;
    jshort* shortData = (jshort*)rdr->_data + rdr->_imageOffset +
                        y * rdr->_imageScanlineStride + x * pixelStride;

    for (; h > 0; --h) {
        jint w2;
        for (w2 = w; w2 > 0; --w2) {
            *shortData = cval;
            shortData += pixelStride;
        }
        shortData += scanlineSkip;
    }
}

void
clearRect8(Renderer *rdr, jint x, jint y, jint w, jint h) {
    jbyte cval = (jbyte)((19961 * rdr->_cred + 38666 * rdr->_cgreen +
                          7209 * rdr->_cblue) >> 16);
    jint pixelStride = rdr->_imagePixelStride;
    jint scanlineSkip = rdr->_imageScanlineStride - w * pixelStride;
    jbyte* byteData = (jbyte*)rdr->_data + rdr->_imageOffset +
                      y * rdr->_imageScanlineStride + x * pixelStride;

    for (; h > 0; --h) {
        jint w2;
        for (w2 = w; w2 > 0; --w2) {
            *byteData = cval;
            byteData += pixelStride;
        }
        byteData += scanlineSkip;
    }
}

static void
blendSrcOver888(jint *intData,
                jint aval,
                jint cred, jint cgreen, jint cblue) {
    jint ival;
    jint red, green, blue;
    jint nred, ngreen, nblue;

    ival = *intData;
    red = (ival >> 16) & 0xff;
    green = (ival >> 8) & 0xff;
    blue = ival & 0xff;

    nred = (red << ALPHA_SHIFT) +
           (cred - red) * aval + HALF_ALPHA;
    nred >>= ALPHA_SHIFT;

    ngreen = (green << ALPHA_SHIFT) +
             (cgreen - green) * aval + HALF_ALPHA;
    ngreen >>= ALPHA_SHIFT;

    nblue = (blue << ALPHA_SHIFT) +
            (cblue - blue) * aval + HALF_ALPHA;
    nblue >>= ALPHA_SHIFT;

    ival = 0xff000000 | (nred << 16) | (ngreen << 8) | nblue;
    *intData = ival;
}

// 8-bit blend against an ARGB color
static void
blendSrcOver8888(jint *intData,
                 jint aval,
                 jint sred, jint sgreen, jint sblue) {
    jint denom;

    jint ival = *intData;
    jint dalpha = (ival >> 24) & 0xff;
    jint dred = (ival >> 16) & 0xff;
    jint dgreen = (ival >> 8) & 0xff;
    jint dblue = ival & 0xff;

    denom = 256 * dalpha + aval * (255 - dalpha);
    if (denom == 0) {
        // dalpha and aval must both be 0
        // The output is transparent black
        *intData = 0x00000000;
    } else {
        jlong recip = (1L << 24) / denom;
        jlong fa = (256 - aval) * dalpha * recip;
        jlong fb = 255 * aval * recip;
        jint oalpha = denom >> 8;
        jint ored = (jint)((fa * dred + fb * sred) >> 24);
        jint ogreen = (jint)((fa * dgreen + fb * sgreen) >> 24);
        jint oblue = (jint)((fa * dblue + fb * sblue) >> 24);

        ival = (oalpha << 24) | (ored << 16) | (ogreen << 8) | oblue;
        *intData = ival;
    }
}

// *intData are premultiplied, sred, sgreen, sblue are non-premultiplied
static void
blendSrcOver8888_pre(jint *intData,
                             jint aval,
                             jint sred, jint sgreen, jint sblue) {
    jint ival = *intData;
    //destination alpha
    jint dalpha = (ival >> 24) & 0xff;
    //destination components premultiplied by dalpha
    jint dred = (ival >> 16) & 0xff;
    jint dgreen = (ival >> 8) & 0xff;
    jint dblue = ival & 0xff;
    
    //premultiplied source components (we add 0.5 for presicion)
    jint psred   = (sred * aval   + 127);
    jint psgreen = (sgreen * aval + 127);
    jint psblue  = (sblue * aval  + 127);
    
    jint oneminusaval = (256 - aval);
    
    jint oalpha = (256 * aval + dalpha * oneminusaval)  >> 8;
    jint ored   = (psred   + oneminusaval * dred)     >> 8;
    jint ogreen = (psgreen  + oneminusaval * dgreen)   >> 8;
    jint oblue  = (psblue   + oneminusaval * dblue )   >> 8;
    
    *intData = (oalpha << 24) | (ored << 16) | (ogreen << 8) | oblue;
}

static void
blendSrcOver565(jshort *shortData, jint pix) {
    /* assume cred, cgreen, cblue are at the correct bit depths */

    jshort sval = *shortData;
    int dwAlphaRBtemp = (sval & 0xf81f);
    int dwAlphaGtemp = (sval & 0x07e0);
    int coRB = pix & 0xf81f;
    int coG = pix & 0x07e0;
    int dw6bitOpacity = (pix >> 26) & 0x3f;

    *shortData = ((dwAlphaRBtemp + (((coRB - dwAlphaRBtemp) * dw6bitOpacity) 
                                    >> 6)) & 0xf81f) |
                 ((dwAlphaGtemp + (((coG - dwAlphaGtemp) * dw6bitOpacity) 
                                    >> 6)) & 0x07e0);
}

static void
blendSrcOver8(jbyte *byteData,
              jint aval,
              jint cgray) {
    jint gray;
    jint ngray;

    gray = *byteData & 0xff;

    ngray = (gray << ALPHA_SHIFT) +
            (cgray - gray) * aval + HALF_ALPHA;
    ngray >>= ALPHA_SHIFT;

    *byteData = (jbyte)ngray;
}

static void
blendSrc8888(jint *intData,
             jint aval, jint raaval,
             jint sred, jint sgreen, jint sblue) {
    jint denom;

    jint ival = *intData;
    jint dalpha = (ival >> 24) & 0xff;
    jint dred = (ival >> 16) & 0xff;
    jint dgreen = (ival >> 8) & 0xff;
    jint dblue = ival & 0xff;

    denom = 255 * aval + dalpha * raaval;
    if (denom == 0) {
        // The output is transparent black
        *intData = 0x00000000;
    } else {
        jlong recip = (1L << 24) / denom;
        jlong fa = raaval * dalpha * recip;
        jlong fb = 255 * aval * recip;
        jint oalpha = denom >> 8;
        jint ored = (jint)((fa * dred + fb * sred) >> 24);
        jint ogreen = (jint)((fa * dgreen + fb * sgreen) >> 24);
        jint oblue = (jint)((fa * dblue + fb * sblue) >> 24);

        ival = (oalpha << 24) | (ored << 16) | (ogreen << 8) | oblue;
        *intData = ival;
    }
}

static void
blendSrc8888_pre(jint *intData,
                 jint aval, jint raaval,
                 jint sred, jint sgreen, jint sblue) {
    jint denom;

    jint ival = *intData;
    jint dalpha = (ival >> 24) & 0xff;
    //premultiplied color components
    jint dred =   (ival >> 16) & 0xff;
    jint dgreen = (ival >>  8) & 0xff;
    jint dblue =  (ival & 0xff);

    denom = 255 * aval + dalpha * raaval;
    if (denom == 0) {
        // The output is transparent black
        *intData = 0x00000000;
    } else {
        jlong fa = raaval ;
        jlong fb = aval;
        jint oalpha = denom >> 8;
        jint ored = (jint)((fa * dred + fb * sred) >> 8);
        jint ogreen = (jint)((fa * dgreen + fb * sgreen) >> 8);
        jint oblue = (jint)((fa * dblue + fb * sblue) >> 8);
        
        ival = (oalpha << 24) | (ored << 16) | (ogreen << 8) | oblue;
        *intData = ival;
    }
}

static void
blendLine(void *data, jint imageType, jint offset, jint stride,
          jint length, jint alpha, jint red, jint green, jint blue) {
    jint *intData;
    jshort *shortData;
    jbyte *byteData;
    jint i;

    if (imageType == TYPE_INT_RGB) {
        intData = (jint *)data;
        for (i = 0; i < length; i++) {
            blendSrcOver888(&intData[offset], alpha, red, green, blue);
            offset += stride;
        }
    } else if (imageType == TYPE_INT_ARGB) {
        intData = (jint *)data;
        for (i = 0; i < length; i++) {
            blendSrcOver8888(&intData[offset], alpha, red, green, blue);
            offset += stride;
        }
    } else if (imageType == TYPE_INT_ARGB_PRE) {
        intData = (jint *)data;
        for (i = 0; i < length; i++) {
            blendSrcOver8888_pre(&intData[offset], alpha, red, green, blue);
            offset += stride;
        }    
    } else if (imageType == TYPE_USHORT_565_RGB) {
        int cval = (alpha << 24) | (red << 11) | (green << 6) | blue;
        shortData = (jshort *)data;
        for (i = 0; i < length; i++) {
            blendSrcOver565(&shortData[offset], cval);
            offset += stride;
        }
    } else if (imageType == TYPE_BYTE_GRAY) {
        byteData = (jbyte *)data;
        for (i = 0; i < length; i++) {
            blendSrcOver8(&byteData[offset], alpha, red);
            offset += stride;
        }
    }
}

static jlong
lmod(jlong x, jlong y) {
    x = x % y;
    if (x < 0) {
        x += y;
    }
    return x;
}

static jint
interp(jint x0, jint x1, jint frac) {
    return ((x0 << 16) + (x1 - x0) * frac + 0x8000) >> 16;
}
