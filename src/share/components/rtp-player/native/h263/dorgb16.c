/*-----------------------------------------------------------------------------
 *	DoRGB16.C
 *
 *	DESCRIPTION
 *		dorgb16.c - originally a function in PicToDib.  DoRGB16 converts YUV to
 *					to 16-bit DIB.
 *
 *      Author:     Gene Su    		05/16/94
 *      Inspector:  
 *      Revised:
 *			03/04/97	D Blair	Added DoRGB16With3HalfsUpsample (PC and MAC versions)
 *			02/22/97	D Blair	Added DoRGB16With2xUpsample (PC and MAC versions)
 *			08/30/94	G Su	Added DitherClip0
 *			09/01/94	G Su	Use only one DitherClip table
 *
 *	(c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
//#define DEBUG

#ifdef DEBUG
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "vvcoder.h"
#include "dorgb16.h"

extern void pascal DoRGB16( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB )
{
    U16 far *r1i;		/* output row 1 pointer */
    U16 far *r2i;		/* output row 2 pointer */
    U8 *y1pi;			/* input row 1 pointer */
    U8 *y2pi;			/* input row 2 pointer */
    int j, k, ii;
    
    // Allow loop testing on 0
    pCb += cHor;
    pCr += cHor;
#ifdef FOR_MAC
	// Mac pixmaps are right-side-up, with respect to the encoded data, while DIBs
	// are upside-down. Easiest way to modify the existing logic is to copy bottom-up
	// for both input and output.
	pDIB += (offsetDIB * cVert * 2) - (offsetDIB * 2);	// Last pair of rows
#endif
    /* loop over chroma rows */
    for ( j = cVert; j > 0; j-- ) {
        /* initialize running pointer for each row */
        r1i = pDIB;
        y1pi = pY;
        r2i = pDIB + offsetDIB;
        y2pi = pY - offsetY;
        for ( k = -cHor; k < 0; ++k ) {
            /* loop over chroma pels */
            ii = tabCrCb[ *(pCr + k) ][ *(pCb + k) ];
            /* do luma pels, replicating chroma */
#ifdef FOR_MAC
            *r2i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r2i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi + 1)+4]];	/* Table look-up dither: +4 */
            r2i += 2;
            y1pi += 2;
            *(r1i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
            *(r1i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi + 1)+2]];	/* Table look-up dither: +2 */
            r1i += 2;
            y2pi += 2;
#else
            *r1i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r1i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi + 1)+4]];	/* Table look-up dither: +4 */
            r1i += 2;
            y1pi += 2;
            *(r2i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
            *(r2i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi + 1)+2]];	/* Table look-up dither: +2 */
            r2i += 2;
            y2pi += 2;
#endif
        }
        /* skip to previous row on input pointers */
        pY -= 2 * offsetY;  // Move up two rows
        pCb -= offsetCb;
        pCr -= offsetCr;
#ifdef FOR_MAC
        /* skip to previous row on output pointers */
		pDIB -= 2 * offsetDIB;
#else
        /* skip to next row on output pointers */
        pDIB += 2 * offsetDIB;
#endif
    }
    return;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Here is a version of DoRGB16 which makes no assumption about the parity of the image resolution.
// It is to be understood that if we have a odd dimension, the structure of the chroma 
// and luma is as follows on the right edge
//  
//         Last Column Before Right Edge
//                       |
//                       |
//                      \_/
//
//  L     L     L     L  |  L  
//     C           C     |      C     
//  L     L     L     L  |  L  
//                       |
//  L     L     L     L  |  L  
//     C           C     |      C     
//  L     L     L     L  |  L  
//
// with an analogous layout on the bottom.
//
// Note that unlike DoRGB16, the cHor and cVert parameters to DoRGB16AnyParity refer to
// the LUMA dimensions not the chroma!

extern void pascal DoRGB16AnyParity( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB )
{
    U16 far *r1i;		/* output row 1 pointer */
    U16 far *r2i;		/* output row 2 pointer */
    U8 *y1pi;			/* input row 1 pointer */
    U8 *y2pi;			/* input row 2 pointer */
    int j, k, kk, ii;
    int cChromaHor;
    // Allow loop testing on 0
    // Note if we have an odd number of cols then this
    // is one less than the actual number of chroma
    cChromaHor = (cHor)>>1;
    pCb += cChromaHor;
    pCr += cChromaHor;

#ifdef FOR_MAC
	// Mac pixmaps are right-side-up, with respect to the encoded data, while DIBs
	// are upside-down. Easiest way to modify the existing logic is to copy bottom-up
	// for both input and output.
	pDIB += (offsetDIB * cVert) - (offsetDIB * 2);	// Last pair of rows
#endif

    // If we have an odd number of rows, then since we are doing our YUV bottom up,
    // the last row of luma is matched with the last row of chroma and the next to last
    // and second to last rows of luma are paired with the next to last row of chroma.
    // Accordingly we handle the last rows separately.
    if(cVert & 1) { 
        /* initialize running pointer for each row */
#ifdef FOR_MAC
        pDIB += offsetDIB; // Mac needs to point to LAST row
#endif
        r1i = pDIB; 
        y1pi = pY;
        for ( k = -cChromaHor, kk = -cHor; k < 0; kk+=2, k++ ) {
            /* loop over chroma pels */
            ii = tabCrCb[ *(pCr + k) ][ *(pCb + k) ];
            /* do luma pels, replicating chroma */

            *r1i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r1i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi + 1)+4]];	/* Table look-up dither: +4 */
            r1i += 2;
            y1pi += 2;
        }

        if(kk == -1) { // Got one more chroma and luma to process!
            /* loop over chroma pels */
            ii = tabCrCb[ *pCr ][ *pCb ];
            /* do luma pels, replicating chroma */
            *r1i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
        }
        pY -= offsetY;
        pCr -= offsetCr;
        pCb -= offsetCb;
        cVert -= 1;
#ifdef FOR_MAC
        /* skip to previous row on output pointers */
		pDIB -= 2 * offsetDIB;
#else
        pDIB += offsetDIB; 
#endif
    }
    
    
    
    /* loop over chroma rows */
    for ( j = cVert; j > 0; j-=2 ) {
        /* initialize running pointer for each row */
        r1i = pDIB;
        y1pi = pY;
        r2i = pDIB + offsetDIB;
        y2pi = pY - offsetY;
        for ( k = -cChromaHor, kk = -cHor; k < 0; kk+=2, k++ ) {
            /* loop over chroma pels */
            ii = tabCrCb[ *(pCr + k) ][ *(pCb + k) ];
            /* do luma pels, replicating chroma */
#ifdef FOR_MAC
            *r2i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r2i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi + 1)+4]];	/* Table look-up dither: +4 */
            r2i += 2;
            y1pi += 2;
            *(r1i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
            *(r1i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi + 1)+2]];	/* Table look-up dither: +2 */
            r1i += 2;
            y2pi += 2;
#else
            *r1i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r1i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi + 1)+4]];	/* Table look-up dither: +4 */
            r1i += 2;
            y1pi += 2;
            *(r2i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
            *(r2i + 1) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi + 1)+2]];	/* Table look-up dither: +2 */
            r2i += 2;
            y2pi += 2;
#endif
        }

        if(kk == -1) { // Got one more chroma and luma to process!
            /* loop over chroma pels */
            ii = tabCrCb[ *pCr ][ *pCb ];
            /* do luma pels, replicating chroma */
#ifdef FOR_MAC
            *r2i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r1i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
#else
            *r1i = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y1pi)]];
            *(r2i) = ((U16 (*)[64]) YUVToRGB)[ii][DitherClip[*(y2pi)+6]];			/* Table look-up dither: +6 */
#endif
        }

        /* skip to previous row on input pointers */
        pY -= 2 * offsetY;  // Move up two rows
        pCb -= offsetCb;
        pCr -= offsetCr;
#ifdef FOR_MAC
        /* skip to previous row on output pointers */
		pDIB -= 2 * offsetDIB;
#else
        /* skip to next row on output pointers */
        pDIB += 2 * offsetDIB;
#endif
    }

    return;
}

//////////////////////////////////////////////////////////////////////////
// Here's a little ditty which performs our YUV -> RGB conversion while
// simultaneously doing a 1:2 upsampling via bilinear interpolation.  
// The point here is to do both operations in one pass over the data.
// NOTE: (1) this function assumes that the luma dimensions are multiples of 2
//       (2) there is a slight difference between working bottom up (as we do
//           here) and top down.  In the bottom up case, the extrapolation
//           that is necessary to get exactly twice as many rows occurs at the
//           top of the image rather than at the bottom.
//
extern void pascal DoRGB16With2xUpsample( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB )
{
    U16 far *r1i;		/* output row 1 pointer */
    U16 far *r2i;		/* output row 2 pointer */
    U16 far *r3i;		/* output row 3 pointer */
    U16 far *r4i;		/* output row 4 pointer */
    U8 *cr1pi;			/* cr input row 1 pointer */
    U8 *cr2pi;			/* cr input row 2 pointer */
    U8 *cb1pi;			/* cb input row 1 pointer */
    U8 *cb2pi;			/* cb input row 2 pointer */
    U8 *y1pi;           /* y input row 1 pointer */
    U8 *y2pi;           /* y input row 2 pointer */
    U8 *y3pi;           /* y input row 3 pointer */
    U16 * myLUT;
    int j, k;
    int cChromaHor;
    // Allow loop testing on 0
    // Note if we have an odd number of cols then this
    // is one less than the actual number of chroma
    cChromaHor = (cHor)>>1;

#ifdef FOR_MAC
	// Mac pixmaps are right-side-up, with respect to the encoded data, while DIBs
	// are upside-down. Easiest way to modify the existing logic is to copy bottom-up
	// for both input and output.
	pDIB += (offsetDIB * (cVert<<1)) - (offsetDIB * 4);	// Last set of four rows
#endif

    for(j = cVert; j>0; j-=2) {
        /* initialize running pointer for each row */
#ifdef FOR_MAC
        r4i = pDIB;
        r3i = r4i + offsetDIB;
        r2i = r3i + offsetDIB;
        r1i = r2i + offsetDIB;
#else
        r1i = pDIB;
        r2i = r1i + offsetDIB;
        r3i = r2i + offsetDIB;
        r4i = r3i + offsetDIB;
#endif
        y1pi = pY;
        y2pi = y1pi - offsetY;
        cr1pi = pCr;
        cb1pi = pCb;
        if(j>2) {
            y3pi = y2pi - offsetY;
            cr2pi = cr1pi - offsetCr;
            cb2pi = cb1pi - offsetCb;
        } else {
            // When j==0 y2pi, cr1pi and cb1pi are all pointing to the first row of the image.
            // Instead of pointing of into bogus memory we essentially replicate the first row
            y3pi = y2pi; 
            cr2pi = cr1pi;
            cb2pi = cb1pi;
        }

        // Do all but the last column - we have to handle that pesky extrapolation issue separately
        for ( k = -cChromaHor+1; k < 0; k++ ) {
            // Interpolate the upper left chroma sample
            myLUT = YUVToRGB[ tabCrCb[cr1pi[0]][cb1pi[0]] ];
            // do interpolate and transform luma pels, replicating chroma
            *r1i = myLUT[DitherClip[y1pi[0]]];
            *(r1i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
            *(r2i) = myLUT[DitherClip[((y1pi[0]+y2pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
            *(r2i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+y2pi[0]+y2pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
            y1pi+=1;
            r1i+=2;
            r2i+=2;

            // Interpolate the lower left chroma sample
            myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr2pi[0]+1)>>1][(cb1pi[0]+cb2pi[0]+1)>>1] ];
            *r3i = myLUT[DitherClip[y2pi[0]]];
            *(r3i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
            *(r4i) = myLUT[DitherClip[((y2pi[0]+y3pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
            *(r4i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+y3pi[0]+y3pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
            y2pi+=1;
            y3pi+=1;
            r3i+=2;
            r4i+=2;

            // Interpolate the upper right chroma sample
            myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr1pi[1]+1)>>1][(cb1pi[0]+cb1pi[1]+1)>>1] ];
            *r1i = myLUT[DitherClip[y1pi[0]]];
            *(r1i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
            *(r2i) = myLUT[DitherClip[((y1pi[0]+y2pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
            *(r2i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+y2pi[0]+y2pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
            y1pi+=1;
            r1i+=2;
            r2i+=2;


            // Interpolate the lower right chroma sample
            myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr1pi[1]+cr2pi[0]+cr2pi[1]+2)>>2]
                                     [(cb1pi[0]+cb1pi[1]+cb2pi[0]+cb2pi[1]+2)>>2] ];
            *r3i = myLUT[DitherClip[y2pi[0]]];
            *(r3i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
            *(r4i) = myLUT[DitherClip[((y2pi[0]+y3pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
            *(r4i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+y3pi[0]+y3pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
            y2pi+=1;
            y3pi+=1;
            r3i+=2;
            r4i+=2;

            cr1pi += 1;
            cr2pi += 1;
            cb1pi += 1;
            cb2pi += 1;
        }        

        // Handle the last column.  The problem is with the upper and lower right samples -
        // these are pointing off of the edge of the image.  We change memory references in
        // such a way as to pad the image on the right by replicating the last column
        myLUT = YUVToRGB[ tabCrCb[cr1pi[0]][cb1pi[0]] ];
        // do interpolate and transform luma pels, replicating chroma
        *r1i = myLUT[DitherClip[y1pi[0]]];
        *(r1i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
        *(r2i) = myLUT[DitherClip[((y1pi[0]+y2pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
        *(r2i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[1]+y2pi[0]+y2pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
        y1pi+=1; r1i+=2; r2i+=2;
        // Interpolate the lower left chroma sample
        myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr2pi[0]+1)>>1][(cb1pi[0]+cb2pi[0]+1)>>1] ];
        *r3i = myLUT[DitherClip[y2pi[0]]];
        *(r3i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+1)>>1) + 4]];	/* Table look-up dither: +4 */
        *(r4i) = myLUT[DitherClip[((y2pi[0]+y3pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
        *(r4i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[1]+y3pi[0]+y3pi[1]+2)>>2) + 2]];	/* Table look-up dither: +2 */
        y2pi+=1; y3pi+=1; r3i+=2; r4i+=2;
        // Interpolate the upper right chroma sample - note here we only use zero indexed arrays.  That
        // is cause we're padding the image on the right
        myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr1pi[0]+1)>>1][(cb1pi[0]+cb1pi[0]+1)>>1] ];
        *r1i = myLUT[DitherClip[y1pi[0]]];
        *(r1i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[0]+1)>>1) + 4]];	/* Table look-up dither: +4 */
        *(r2i) = myLUT[DitherClip[((y1pi[0]+y2pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
        *(r2i + 1) = myLUT[DitherClip[((y1pi[0]+y1pi[0]+y2pi[0]+y2pi[0]+2)>>2) + 2]];	/* Table look-up dither: +2 */
        // Interpolate the lower right chroma sample - note here we only use zero indexed arrays.  That
        // is cause we're padding the image on the right
        myLUT = YUVToRGB[ tabCrCb[(cr1pi[0]+cr1pi[0]+cr2pi[0]+cr2pi[0]+2)>>2]
                                     [(cb1pi[0]+cb1pi[0]+cb2pi[0]+cb2pi[0]+2)>>2] ];
        *r3i = myLUT[DitherClip[y2pi[0]]];
        *(r3i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[0]+1)>>1) + 4]];	/* Table look-up dither: +4 */
        *(r4i) = myLUT[DitherClip[((y2pi[0]+y3pi[0]+1)>>1) + 6]];	    /* Table look-up dither: +6 */
        *(r4i + 1) = myLUT[DitherClip[((y2pi[0]+y2pi[0]+y3pi[0]+y3pi[0]+2)>>2) + 2]];	/* Table look-up dither: +2 */

            
        pY -= 2 * offsetY;  // Move up two rows of luma...
        pCb -= offsetCb; // ... and one row of chroma
        pCr -= offsetCr;

#ifdef FOR_MAC
        pDIB -= 4 * offsetDIB; // Move up four rows of output
#else
        pDIB += 4 * offsetDIB; // Move up four rows of output
#endif
    }

    return;
}

//////////////////////////////////////////////////////////////////////////
// Performs our YUV -> RGB conversion while simultaneously doing a 2:3 
// upsampling via bilinear interpolation.  
// NOTE: (1) this function assumes that the luma dimensions are multiples of 4
//       (2) there is a slight difference between working bottom up (as we do
//           here) and top down.  In the bottom up case, the extrapolation
//           that is necessary to get exactly twice as many rows occurs at the
//           top of the image rather than at the bottom.
//

#ifdef LOW_MEMORY
// If memory is a real issue, we can trade off some cycles with some memory
// by not using lookup tables to do these multiplies

// Some fractional constants in 16.16 format
static U32 weight_1_3 = (1<<16)/3;
static U32 weight_2_3 = (2<<16)/3;
static U32 weight_1_9 = (1<<16)/9;
static U32 weight_2_9 = (2<<16)/9;
static U32 weight_4_9 = (4<<16)/9;

// 1/3*far + 2/3*close
#define INTERP2(close,far) \
    ( (weight_1_3*(far) + weight_2_3*(close)) >> 16 )

// 1/9*a + 2/9*b + 2/9*c + 4/9*d
#define INTERP4(a, b, c, d) \
    (( weight_1_9*(a) + weight_2_9*((b) + (c)) + weight_4_9*(d) )>>16)

#else // LOW_MEMORY

static S32 weight_2_3[512];
static S32 weight_1_9[256];
static S32 weight_2_9[512];
static S32 weight_4_9[256];
static int weightsInitialized=0;

static void InitializeWeights()
{
    S32 i;
    for(i=0; i<256; i++) {
        weight_2_3[256 + i] = i*((2<<16)/3);
        weight_2_3[256 - i] = -i*((2<<16)/3);
        weight_1_9[i] = i*((1<<16)/9);
        weight_2_9[2*i] = 2*i*((2<<16)/9);
        weight_2_9[2*i+1] = (2*i+1)*((2<<16)/9);
        weight_4_9[i] = i*((4<<16)/9);
    }
    weightsInitialized = 1;
}

#define INTERP2(close,far) \
    ( (far) + (weight_2_3[256 + (close) - (far)]>>16) )

#define INTERP4(a, b, c, d) \
    (( weight_1_9[a] + weight_2_9[(b) + (c)] + weight_4_9[d] )>>16)

#endif

extern void pascal DoRGB16With3HalfsUpsample( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB )
{
    U16 far *r1i;		/* output row 1 pointer */
    U16 far *r2i;		/* output row 2 pointer */
    U16 far *r3i;		/* output row 3 pointer */
    U16 far *r4i;		/* output row 4 pointer */
    U16 far *r5i;		/* output row 5 pointer */
    U16 far *r6i;		/* output row 6 pointer */
    U8 *cr1pi;			/* cr input row 1 pointer */
    U8 *cr2pi;			/* cr input row 2 pointer */
    U8 *cr3pi;			/* cr input row 3 pointer */
    U8 *cb1pi;			/* cb input row 1 pointer */
    U8 *cb2pi;			/* cb input row 2 pointer */
    U8 *cb3pi;			/* cb input row 3 pointer */
    U8 *y1pi;           /* y input row 1 pointer */
    U8 *y2pi;           /* y input row 2 pointer */
    U8 *y3pi;           /* y input row 3 pointer */
    U8 *y4pi;           /* y input row 4 pointer */
    U8 *y5pi;           /* y input row 5 pointer */
    U16 * myLUT;
    int j, k;
    int cChromaHor;

#ifndef LOW_MEMORY
    if(!weightsInitialized) InitializeWeights();
#endif
    
    // Allow loop testing on 0
    // Note if we have an odd number of cols then this
    // is one less than the actual number of chroma
    cChromaHor = (cHor)>>1;

#ifdef FOR_MAC
	// Mac pixmaps are right-side-up, with respect to the encoded data, while DIBs
	// are upside-down. Easiest way to modify the existing logic is to copy bottom-up
	// for both input and output.
	pDIB += (offsetDIB * ((3 * cVert) / 2)) - (offsetDIB * 6);	// Last set of six rows
#endif

    for(j = cVert; j>0; j-=4) {
        /* initialize running pointer for each row */
#ifdef FOR_MAC
        r6i = pDIB;
        r5i = r6i + offsetDIB;
        r4i = r5i + offsetDIB;
        r3i = r4i + offsetDIB;
        r2i = r3i + offsetDIB;
        r1i = r2i + offsetDIB;
#else
        r1i = pDIB;
        r2i = r1i + offsetDIB;
        r3i = r2i + offsetDIB;
        r4i = r3i + offsetDIB;
        r5i = r4i + offsetDIB;
        r6i = r5i + offsetDIB;
#endif
        y1pi = pY;
        y2pi = y1pi - offsetY;
        y3pi = y2pi - offsetY;
        y4pi = y3pi - offsetY;
        cr1pi = pCr;
        cr2pi = cr1pi - offsetCr;
        cb1pi = pCb;
        cb2pi = cb1pi - offsetCb;
        if(j>4) {
            y5pi = y4pi - offsetY;
            cr3pi = cr2pi - offsetCr;
            cb3pi = cb2pi - offsetCb;
        } else {
            // When j==0 y5pi, cr3pi and cb3pi are all pointing to the first row of the image.
            // Instead of pointing of into bogus memory we essentially replicate the first row
            y5pi = y4pi; 
            cr3pi = cr2pi;
            cb3pi = cb2pi;
        }

        // Do all but the last column - we have to handle that pesky extrapolation issue separately
        for ( k = -cChromaHor+2; k < 0; k+=2 ) {
    
            // Interpolate chroma sample (0,0)
            myLUT = YUVToRGB[ tabCrCb[cr1pi[0]][cb1pi[0]] ];
            *r1i = myLUT[DitherClip[y1pi[0]]];
            *(r1i + 1) = myLUT[DitherClip[INTERP2(y1pi[1],y1pi[0]) + 4]];	
            *(r2i) = myLUT[DitherClip[INTERP2(y2pi[0], y1pi[0]) + 6]];	    
            *(r2i + 1) = myLUT[DitherClip[INTERP4(y1pi[0], y1pi[1], y2pi[0], y2pi[1]) + 2]];
            r1i+=2;
            r2i+=2;

            // Interpolate chroma sample (1,0)
            myLUT = YUVToRGB[ tabCrCb[INTERP2(cr2pi[0], cr1pi[0])] [INTERP2(cb2pi[0], cb1pi[0])] ];
            *r3i = myLUT[DitherClip[INTERP2(y2pi[0], y3pi[0])]];
            *(r3i + 1) = myLUT[DitherClip[INTERP4(y3pi[0], y3pi[1], y2pi[0], y2pi[1]) + 4]];
            *(r4i) = myLUT[DitherClip[y3pi[0] + 6]];
            *(r4i + 1) = myLUT[DitherClip[INTERP2(y3pi[1], y3pi[0]) + 2]];
            r3i+=2;
            r4i+=2;

            // Interpolate chroma sample (2,0)
            myLUT = YUVToRGB[ tabCrCb[INTERP2(cr2pi[0], cr3pi[0])] [INTERP2(cb2pi[0], cb3pi[0])] ];
            *r5i = myLUT[DitherClip[INTERP2(y4pi[0], y3pi[0])]];
            *(r5i + 1) = myLUT[DitherClip[INTERP4(y3pi[0], y3pi[1], y4pi[0], y4pi[1]) + 4]];
            *(r6i) = myLUT[DitherClip[INTERP2(y4pi[0], y5pi[0]) + 6]];
            *(r6i + 1) = myLUT[DitherClip[INTERP4(y5pi[0], y5pi[1], y4pi[0], y4pi[1]) + 2]];
            r5i+=2;
            r6i+=2;

            // Interpolate chroma sample (0,1)
            myLUT = YUVToRGB[ tabCrCb[INTERP2(cr1pi[1], cr1pi[0])] [INTERP2(cb1pi[1], cb1pi[0])] ];
            *r1i = myLUT[DitherClip[INTERP2(y1pi[1], y1pi[2])]];
            *(r1i + 1) = myLUT[DitherClip[y1pi[2] + 4]];
            *(r2i) = myLUT[DitherClip[INTERP4(y1pi[2], y1pi[1], y2pi[2], y2pi[1]) + 6]];	    
            *(r2i + 1) = myLUT[DitherClip[INTERP2(y2pi[2], y1pi[2]) + 2]];	
            r1i+=2;
            r2i+=2;

            // Interpolate right chroma sample (1,1)
            myLUT = YUVToRGB[ tabCrCb[INTERP4(cr1pi[0], cr1pi[1], cr2pi[0], cr2pi[1])]
                                     [INTERP4(cb1pi[0], cb1pi[1], cb2pi[0], cb2pi[1])] ];
            *r3i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[1], y2pi[2], y2pi[1])]];
            *(r3i + 1) = myLUT[DitherClip[INTERP2(y2pi[2], y3pi[2]) + 4]];
            *(r4i) = myLUT[DitherClip[INTERP2(y3pi[1],y3pi[2]) + 6]];	   
            *(r4i + 1) = myLUT[DitherClip[y3pi[2] + 2]];
            r3i+=2;
            r4i+=2;

            // Interpolate right chroma sample (2,1)
            myLUT = YUVToRGB[ tabCrCb[INTERP4(cr3pi[0], cr3pi[1], cr2pi[0], cr2pi[1])]
                                     [INTERP4(cb3pi[0], cb3pi[1], cb2pi[0], cb2pi[1])] ];
            *r5i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[1], y4pi[2], y4pi[1])]];
            *(r5i + 1) = myLUT[DitherClip[INTERP2(y4pi[2], y3pi[2]) + 4]];
            *(r6i) = myLUT[DitherClip[INTERP4(y5pi[2], y5pi[1], y4pi[2], y4pi[1]) + 6]];	   
            *(r6i + 1) = myLUT[DitherClip[INTERP2(y4pi[2], y5pi[2]) + 2]];
            r5i+=2;
            r6i+=2;

            // Interpolate chroma sample (0,2)
            myLUT = YUVToRGB[ tabCrCb[INTERP2(cr1pi[1], cr1pi[2])] [INTERP2(cb1pi[1], cb1pi[2])] ];
            *r1i = myLUT[DitherClip[INTERP2(y1pi[3], y1pi[2])]];
            *(r1i + 1) = myLUT[DitherClip[INTERP2(y1pi[3], y1pi[4]) + 4]];
            *(r2i) = myLUT[DitherClip[INTERP4(y1pi[2], y1pi[3], y2pi[2], y2pi[3]) + 6]];	    
            *(r2i + 1) = myLUT[DitherClip[INTERP4(y1pi[4], y1pi[3], y2pi[4], y2pi[3]) + 2]];	
            r1i+=2;
            r2i+=2;

            // Interpolate right chroma sample (1,2)
            myLUT = YUVToRGB[ tabCrCb[INTERP4(cr1pi[2], cr1pi[1], cr2pi[2], cr2pi[1])]
                                     [INTERP4(cb1pi[2], cb1pi[1], cb2pi[2], cb2pi[1])] ];
            *r3i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[3], y2pi[2], y2pi[3])]];
            *(r3i + 1) = myLUT[DitherClip[INTERP4(y3pi[4], y3pi[3], y2pi[4], y2pi[3])]];
            *(r4i) = myLUT[DitherClip[INTERP2(y3pi[3],y3pi[2]) + 6]];	   
            *(r4i + 1) = myLUT[DitherClip[INTERP2(y3pi[3],y3pi[4]) + 2]];
            r3i+=2;
            r4i+=2;

            // Interpolate right chroma sample (2,2)
            myLUT = YUVToRGB[ tabCrCb[INTERP4(cr3pi[2], cr3pi[1], cr2pi[2], cr2pi[1])]
                                     [INTERP4(cb3pi[2], cb3pi[1], cb2pi[2], cb2pi[1])] ];
            *r5i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[3], y4pi[2], y4pi[3])]];
            *(r5i + 1) = myLUT[DitherClip[INTERP4(y3pi[4], y3pi[3], y4pi[4], y4pi[3]) + 4]];
            *(r6i) = myLUT[DitherClip[INTERP4(y5pi[2], y5pi[3], y4pi[2], y4pi[3]) + 6]];	   
            *(r6i + 1) = myLUT[DitherClip[INTERP4(y5pi[4], y5pi[3], y4pi[4], y4pi[3]) + 2]];
            r5i+=2;
            r6i+=2;


            y1pi += 4;
            y2pi += 4;
            y3pi += 4;
            y4pi += 4;
            y5pi += 4;

            cr1pi += 2;
            cr2pi += 2;
            cr3pi += 2;
            cb1pi += 2;
            cb2pi += 2;
            cb3pi += 2;
        }  
        
        // Do the last column - here we replace references to y1pi[4], ..., y5pi[4] by
        // y1pi[3], ... , y5pi[3] in effect replicating the last column.  

        // Interpolate chroma sample (0,0)
        myLUT = YUVToRGB[ tabCrCb[cr1pi[0]][cb1pi[0]] ];
        *r1i = myLUT[DitherClip[y1pi[0]]];
        *(r1i + 1) = myLUT[DitherClip[INTERP2(y1pi[1],y1pi[0]) + 4]];	
        *(r2i) = myLUT[DitherClip[INTERP2(y2pi[0], y1pi[0]) + 6]];	    
        *(r2i + 1) = myLUT[DitherClip[INTERP4(y1pi[0], y1pi[1], y2pi[0], y2pi[1]) + 2]];
        r1i+=2;
        r2i+=2;

        // Interpolate chroma sample (1,0)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr2pi[0], cr1pi[0])] [INTERP2(cb2pi[0], cb1pi[0])] ];
        *r3i = myLUT[DitherClip[INTERP2(y2pi[0], y3pi[0])]];
        *(r3i + 1) = myLUT[DitherClip[INTERP4(y3pi[0], y3pi[1], y2pi[0], y2pi[1]) + 4]];
        *(r4i) = myLUT[DitherClip[y3pi[0] + 6]];
        *(r4i + 1) = myLUT[DitherClip[INTERP2(y3pi[1], y3pi[0]) + 2]];
        r3i+=2;
        r4i+=2;

        // Interpolate chroma sample (2,0)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr2pi[0], cr3pi[0])] [INTERP2(cb2pi[0], cb3pi[0])] ];
        *r5i = myLUT[DitherClip[INTERP2(y4pi[0], y3pi[0])]];
        *(r5i + 1) = myLUT[DitherClip[INTERP4(y3pi[0], y3pi[1], y4pi[0], y4pi[1]) + 4]];
        *(r6i) = myLUT[DitherClip[INTERP2(y4pi[0], y5pi[0]) + 6]];
        *(r6i + 1) = myLUT[DitherClip[INTERP4(y5pi[0], y5pi[1], y4pi[0], y4pi[1]) + 2]];
        r5i+=2;
        r6i+=2;

        // Interpolate chroma sample (0,1)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr1pi[1], cr1pi[0])] [INTERP2(cb1pi[1], cb1pi[0])] ];
        *r1i = myLUT[DitherClip[INTERP2(y1pi[1], y1pi[2])]];
        *(r1i + 1) = myLUT[DitherClip[y1pi[2] + 4]];
        *(r2i) = myLUT[DitherClip[INTERP4(y1pi[2], y1pi[1], y2pi[2], y2pi[1]) + 6]];	    
        *(r2i + 1) = myLUT[DitherClip[INTERP2(y2pi[2], y1pi[2]) + 2]];	
        r1i+=2;
        r2i+=2;

        // Interpolate right chroma sample (1,1)
        myLUT = YUVToRGB[ tabCrCb[INTERP4(cr1pi[0], cr1pi[1], cr2pi[0], cr2pi[1])]
                                     [INTERP4(cb1pi[0], cb1pi[1], cb2pi[0], cb2pi[1])] ];
        *r3i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[1], y2pi[2], y2pi[1])]];
        *(r3i + 1) = myLUT[DitherClip[INTERP2(y2pi[2], y3pi[2]) + 4]];
        *(r4i) = myLUT[DitherClip[INTERP2(y3pi[1],y3pi[2]) + 6]];	   
        *(r4i + 1) = myLUT[DitherClip[y3pi[2] + 2]];
        r3i+=2;
        r4i+=2;

        // Interpolate right chroma sample (2,1)
        myLUT = YUVToRGB[ tabCrCb[INTERP4(cr3pi[0], cr3pi[1], cr2pi[0], cr2pi[1])]
                                 [INTERP4(cb3pi[0], cb3pi[1], cb2pi[0], cb2pi[1])] ];
        *r5i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[1], y4pi[2], y4pi[1])]];
        *(r5i + 1) = myLUT[DitherClip[INTERP2(y4pi[2], y3pi[2]) + 4]];
        *(r6i) = myLUT[DitherClip[INTERP4(y5pi[2], y5pi[1], y4pi[2], y4pi[1]) + 6]];	   
        *(r6i + 1) = myLUT[DitherClip[INTERP2(y4pi[2], y5pi[2]) + 2]];
        r5i+=2;
        r6i+=2;

        // Interpolate chroma sample (0,2)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr1pi[1], cr1pi[1])] [INTERP2(cb1pi[1], cb1pi[1])] ];
        *r1i = myLUT[DitherClip[INTERP2(y1pi[3], y1pi[2])]];
        *(r1i + 1) = myLUT[DitherClip[INTERP2(y1pi[3], y1pi[3]) + 4]];
        *(r2i) = myLUT[DitherClip[INTERP4(y1pi[2], y1pi[3], y2pi[2], y2pi[3]) + 6]];	    
        *(r2i + 1) = myLUT[DitherClip[INTERP4(y1pi[3], y1pi[3], y2pi[3], y2pi[3]) + 2]];	
        r1i+=2;
        r2i+=2;

        // Interpolate right chroma sample (1,2)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr1pi[1], cr2pi[1])] [INTERP2(cb1pi[1], cb2pi[1])] ];
        *r3i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[3], y2pi[2], y2pi[3])]];
        *(r3i + 1) = myLUT[DitherClip[INTERP4(y3pi[3], y3pi[3], y2pi[3], y2pi[3])]];
        *(r4i) = myLUT[DitherClip[INTERP2(y3pi[3],y3pi[2]) + 6]];	   
        *(r4i + 1) = myLUT[DitherClip[INTERP2(y3pi[3],y3pi[3]) + 2]];
        r3i+=2;
        r4i+=2;

        // Interpolate right chroma sample (2,2)
        myLUT = YUVToRGB[ tabCrCb[INTERP2(cr3pi[1], cr2pi[1])] [INTERP2(cb3pi[1], cb2pi[1])] ];
        *r5i = myLUT[DitherClip[INTERP4(y3pi[2], y3pi[3], y4pi[2], y4pi[3])]];
        *(r5i + 1) = myLUT[DitherClip[INTERP4(y3pi[3], y3pi[3], y4pi[3], y4pi[3]) + 4]];
        *(r6i) = myLUT[DitherClip[INTERP4(y5pi[2], y5pi[3], y4pi[2], y4pi[3]) + 6]];	   
        *(r6i + 1) = myLUT[DitherClip[INTERP4(y5pi[3], y5pi[3], y4pi[3], y4pi[3]) + 2]];
        r5i+=2;
        r6i+=2;
            
        pY -= 4 * offsetY;  // Move up four rows of luma...
        pCb -= 2 * offsetCb; // ... and two rows of chroma
        pCr -= 2 * offsetCr;

#ifdef FOR_MAC
        pDIB -= 6 * offsetDIB; // Move up six rows of output
#else
        pDIB += 6 * offsetDIB; // Move up six rows of output
#endif
    }

    return;
}

