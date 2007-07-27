/*-----------------------------------------------------------------------------
 *  DEC_BLK.H
 *
 *  DESCRIPTION   
 *      Necessary pragmas and function prototypes for DoRGB16.asm and DoRGB16.c.
 *
 *  Author:     Gene Su			    05/15/94
 *  Inspector:  
 *  Revised:
 *  01/07/96    wolfe   Wrapped PRAGMA AUX with WATCOM ifdefs
 *			03/04/97	D Blair	Created DoRGB16With3HalfsUpsample
 *			02/20/97	D Blair	Created DoRGB16With2xUpsample
 *			02/04/97	D Blair	Created DoRGB16AnyParity which works for any output size
 *			09/01/94	G Su	Use only one DitherClip table
 *			08/30/94	G Su	Modify table declarations to reflect the new
 *								implementation for 10 bit chroma and 6 bit luma
 *								YUV to RGB conversion
 *
 *  (c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/
#ifndef	_INC_DORGB16
#define	_INC_DORGB16

extern U16 tabCrCb[256][256];
extern U16 YUVToRGB[1024][64];
extern U8 DitherClip[262];

extern void pascal DoRGB16( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB );

extern void pascal DoRGB16AnyParity( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB );
extern void pascal DoRGB16With2xUpsample( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB );
extern void pascal DoRGB16With3HalfsUpsample( int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB );
#if defined(__WATCOMC__)
#pragma aux DoRGB16 parm [eax ebx ecx edx];
#endif

#endif
