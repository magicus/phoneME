/*-----------------------------------------------------------------------------
 *  PIC2DIBD.C
 *
 *  DESCRIPTION
 *      pic2dibd.c - 
 *      Contains functions to convert an image represented as a 
 *      VfW compatible DIB to a YCrCb PICTURE structure.
 *
 *  NOTES
 *      1. Only supports RGB555, 16bpp, "normal" DIB orientation i.e. upside-down.   
 *
 *  Author:     Mary Deshon    11/05/93
 *  Inspector:  Staffan Ericsson 4/13/94
 *  Revised:
 *  03/27/97    db      Created 24 bit conversions for installable codec
 *  03/04/97    db      Hooked up some optimized upsampling code 
 *  02/22/97    db      Fixed some bilinear interpolation bugs 
 *  12/31/96    se      Use doublebuffered output in DecPICTUREToDIB
 *  10/22/96    se      Support PB-frames
 *  04/19/96    wolfe   Do not compile PicToYUV9 if using Win3.1 WATCOM 10.0a
 *                      since it is never called in this file!
 *  12/21/95    wolfe   Broke PICTODIB into PIC2DIBC and PIC2DIBD
 *  
 *  (c) 1993-1997 Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

//#define DEBUG

#include "mni.h"

#ifdef DEBUG
#include <windows.h>
#include <string.h>
#include <stdio.h>
#endif

#include "machine.h"
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "vvcoder.h"
#include "dorgb16.h"
#include <math.h>
#include <stdlib.h>
#ifdef VCM_CODEC
#include "dorgb24.h"
#include "dorgb32.h"
#endif
#define WIDTHBYTES(bits) ( (((bits) + 31)/32) * 4 )

#define SELECTIVE_UPDATE
//#define TESTING // Stand-alone testing of PicToRGB16 (DecoderRegistry not needed)

#ifndef TESTING
extern H261Decoder *DecoderRegistry[MAX_NUM_DECODERS];
#endif

//extern int InitYUVToRGB( void );  // Proto in h261func.h
static S32 makeEntry ( S32 y, S32 u, S32 v );
#ifdef VCM_CODEC
static S32 makeEntry24 ( S32 y, S32 u, S32 v );
#endif
static void InitDitherClip ( void );
/*extern void pascal DoRGB16( int cHor, int cVert,  // Proto in dorgb16.h
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 far *pDIB, int offsetDIB );*/
                    
static int InitColorIndexTab(void);
static void InitDitherClip(void);
void RescalePICTURE(PICTURE *In, PICTURE *Out, S32 srcHeight, S32 srcWidth, FLOAT scale);
#ifdef FOR_MAC
extern int  PicToRGB16( PICTURE *pic, U16 far dib[], int select, U8 MBmap[], S16 bytesPerRow );
#else
extern int  PicToRGB16( PICTURE *pic, U16 far dib[], int select, U8 MBmap[] );
#endif

#define Clip(x,lo,hi)   ((x) <= (lo) ? (lo) : ((x) <= (hi) ? (x) : (hi)))

#define NUM_CR  32
#define NUM_CB  32
static U8 colorCR[NUM_CR] =
{19,30,41,50,59,68,75,82,89,96,103,108,113,118,123,128,133,138,143,148,153,160,167,
174,181,188,195,204,213,224,235,246};
static U8 colorCB[NUM_CB] =
{19,30,41,50,59,68,75,82,89,96,103,108,113,118,123,128,133,138,143,148,153,160,167,
174,181,188,195,204,213,224,235,246};
static int stepCr[NUM_CR+1] = 
{0,25,36,46,55,64,72,79,86,93,100,106,111,116,121,126,131,136,141,146,151,157,164,
171,178,185,192,200,209,219,230,241,256};
static int stepCb[NUM_CB+1] =
{0,25,36,46,55,64,72,79,86,93,100,106,111,116,121,126,131,136,141,146,151,157,164,
171,178,185,192,200,209,219,230,241,256};

// The following arrays will be accessed by dorgb16.c or dorgb16.asm.
U16 tabCrCb[256][256];
U16 YUVToRGB[1024][64];
U8 DitherClip[256+6];
#ifdef VCM_CODEC
U32 YUVToRGB24[1024][64];
#endif
//#include <stdio.h>


/* The YUVToRGB table is a two dimensional table:   */ 
/*  The first dimension represents 8-bits of indexed color. */
/*  The second dimension represents 8-bits of luma: y7 y6 y5 y4 y3 y2 y1 y0 */
extern int
InitYUVToRGB( void )
{
    S32     y, u ,v;
    int     ii, cr, cb;
    
    InitColorIndexTab();    /* Initialize color index table */
    InitDitherClip();       /* Initialize dither clipping tables */

    /* now set the table entries appropriately */    
    for ( cr = 0; cr < NUM_CR; cr++ ) {
        v = colorCR[cr]; 
        for ( cb = 0; cb < NUM_CB; cb++ ) {
            u = colorCB[cb];
            ii = cb + cr * NUM_CB;
            for ( y = 0; y < 64; y ++ ) {
                YUVToRGB[ii][y] = (U16) makeEntry ( y*4, u, v ); 
#ifdef VCM_CODEC
                YUVToRGB24[ii][y] = (U32) makeEntry24 ( y*4, u, v ); 
#endif
            }
        }
    }
    return ( OK );
}

/* The ColorIndex table is a one dimensional table: */ 
/*  The look-up index represents 16 bits of color: ( [cr7-cr0] << 8 ) | [cb7-cb0] */
/*  The output is an 8-bit value which is used to index into the next table */
static int
InitColorIndexTab( void )
{
    int i, j, cblow, cbhigh, crlow, crhigh, cb, cr;

    for ( j = 0; j < NUM_CR; j++ ) {
        for ( i = 0; i < NUM_CB; i++ ) {
            crlow = stepCr[j];
            crhigh = stepCr[j+1] - 1;
            cblow = stepCb[i];
            cbhigh = stepCb[i+1] - 1;

#ifdef DO_ASM_DORGB16            
            for ( cr = crlow; cr <= crhigh; cr++ ) {
                for ( cb = cblow; cb <= cbhigh; cb++ ) {
                    tabCrCb[cr][cb] = ((i + j * NUM_CB) & 0x03ff) << 6;
                }
            }
#else
            for ( cr = crlow; cr <= crhigh; cr++ ) {
                for ( cb = cblow; cb <= cbhigh; cb++ ) {
                    tabCrCb[cr][cb] = i + j * NUM_CB;
                }
            }
#endif            
        }
    }
    return ( OK );
}


/* The DitherClip tables are one dimensional tables:
 *  The look-up index represents 8 bits of luma, 
 *  the output is an 8-bit value  */
static void
InitDitherClip( void )
{
    int y;

#ifdef DO_ASM_DORGB16
    for ( y = 0; y < 262; y++ ) {
        DitherClip[y] = ((Clip( y+0, 0, 255 )) & 0xfc) >> 1;
    }
#else
    for ( y = 0; y < 262; y++ ) {
        DitherClip[y] = ((Clip( y+0, 0, 255 )) & 0xfc) >> 2;
    }
#endif    
}


static S32
makeEntry ( S32 y, S32 u, S32 v )
{
    S32 r, g, b; 
    S32 result;

    /* perform conversion  (Y U V terms) */ 
    u -= 128;
    v -= 128;   
    r = g = b = (y<<8) + 128;   // Add "0.5" for rounding

    /* Color conversion */
    r += 359 * v; 
         
    g -=  88 * u;
    g -= 183 * v;
    
    b += 454 * u;

    /* clamp results */    
    if (r > 65535) r = 65535;
    if (g > 65535) g = 65535;
    if (b > 65535) b = 65535;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    /* assemble result. */
#ifdef FOR_UNIX_24BITRGBA
    result.r =  0xff & ( r >> 8);
    result.b = 0xff & ( b >> 8);
    result.g = 0xff & ( g >> 8);
    result.alpha = 0;
#elif defined (FOR_UNIX)
    result = 0;
    result =  0xf800 & r;
    result |= 0x07c0 & ( g >> 5);
    result |= 0x003e & ( b >> 10);
#else
    result =  0x7c00 & ( r >> 1);
    result |= 0x03e0 & ( g >> 6);
    result |= 0x001f & ( b >> 11);
#endif
    
    return result;
}

#ifdef VCM_CODEC
static S32
makeEntry24 ( S32 y, S32 u, S32 v )
{
    S32 r, g, b; 
    S32 result=0;

    /* perform conversion  (Y U V terms) */ 
    u -= 128;
    v -= 128;   
    r = g = b = (y<<8) + 128;   // Add "0.5" for rounding

    /* Color conversion */
    r += 359 * v; 
         
    g -=  88 * u;
    g -= 183 * v;
    
    b += 454 * u;

    /* clamp results */    
    if (r > 65535) r = 65535;
    if (g > 65535) g = 65535;
    if (b > 65535) b = 65535;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    /* assemble result. */
    result |=  0x00ff0000 & ( r << 8 );
    result |= 0x0000ff00 & ( g  );
    result |= 0x000000ff & ( b >> 8);
    
    return result;
}
#endif

//***************************************************************************
// PicToYVU9 - Convert Vivo PICTURE to 9-bit YVU Planar DIB
//**************************************************************************
#ifndef TESTING
static int  PicToYVU9(PICTURE *pic, U16 far dib[])
{
    PIXEL   *pY, *pCb, *pCr;
    PIXEL       *pYTemp,*pCbTemp, *pCrTemp;
    PIXEL       *pLastRow, *pLastCol;
    U8 FAR      *pDIB;
    int         nhor, nvert;
    
    //sprintf(msg,"Calling PicToYVU9\n");
    //OutputDebugString(msg);
    
        //Get dimensions of luma picture
    nhor = pic->y.nhor;
    nvert = pic->y.nvert;
   
        //Get pointer to the output DIB
    pDIB = (U8 FAR *)&dib[0];
    //Get pointer to first luma pel
    pY = pic->y.ptr;
        //Get pointer to the one row past last Row.
        pLastRow = pY + (pic->y.hoffset * (nvert));
        
        while (pY < pLastRow)
        {//For each row
                pYTemp = pY;
                //Pointer to one pel past the last pel
                pLastCol = pYTemp + nhor;       
                while (pYTemp < pLastCol) 
                {//Do all the columns, YYYYUV
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;                            
                }
                //Update the Luminance pointer
                pY += pic->y.hoffset;
        }
        
        //Next do the Cr component (V)
    pCr = pic->cr.ptr;  
        //Get dimensions of chroma picture
    nhor = pic->cr.nhor;
    nvert = pic->cr.nvert;
        pLastRow = pCr + (pic->cr.hoffset * (nvert));    
        
        while (pCr < pLastRow)
        {//For each row
                pCrTemp = pCr;
                //Pointer to one pel past the last pel
                pLastCol = pCrTemp + nhor;      
                while (pCrTemp < pLastCol) 
                {//Do all the columns, YYYYUV
                        *pDIB++ = *pCrTemp;  //4:1 chroma subsampling.  Chroma is already
                        pCrTemp += 2;           //subsampled 2:1 so subsample by another 2:                             
                }
                //Update the ponter, skip a row
                pCr += pic->cr.hoffset;
                pCr += pic->cr.hoffset; 
        }
        //Next do the Cb component (U)
    pCb = pic->cb.ptr;  
        //Get dimensions of chroma picture
    nhor = pic->cb.nhor;
    nvert = pic->cb.nvert;
        pLastRow = pCb + (pic->cb.hoffset * (nvert));    
        
        while (pCb < pLastRow)
        {//For each row
                pCbTemp = pCb;
                //Pointer to one pel past the last pel
                pLastCol = pCbTemp + nhor;      
                while (pCbTemp < pLastCol) 
                {//Do all the columns, YYYYUV
                        *pDIB++ = *pCbTemp;  //4:1 chroma subsampling.  Chroma is already
                        pCbTemp += 2;           //subsampled 2:1 so subsample by another 2:                             
                }
                //Update the ponter, skip a row
                pCb += pic->cr.hoffset;
                pCb += pic->cr.hoffset;
        }
    return( OK );
}
#endif


#if !defined(__WATCOMC__) || defined(WIN95_VXD)
//***************************************************************************
// PicToYUV9 - Convert Vivo PICTURE to 9-bit YUV DIB
//**************************************************************************
#ifndef TESTING
/*
static int  PicToYUV9(PICTURE *pic, U16 far dib[])
{
    PIXEL   *pY, *pCb, *pCr;
    PIXEL       *pYTemp,*pCbTemp, *pCrTemp;
    PIXEL       *pLastRow, *pLastCol;
    U8 FAR      *pDIB;
    int         nhor, nvert;
    
    //sprintf(msg,"Calling PicToYUV9\n");
    //OutputDebugString(msg);
    
        //Get dimensions of luma picture
    nhor = pic->y.nhor;
    nvert = pic->y.nvert;
   
        //Get pointer to the output DIB
    pDIB = (U8 FAR *)&dib[0];
    //Get pointer to first luma pel
    pY = pic->y.ptr;
        //Get pointers to first chroma pels
    pCb = pic->cb.ptr;
    pCr = pic->cr.ptr;
    
        //Get pointer to the one row past last Row.
        pLastRow = pY + (nhor * (nvert));
        
        while (pY < pLastRow)
        {//For each row
                pCbTemp = pCb;
                pCrTemp = pCr;
                pYTemp = pY;
                //Pointer to one pel past the last pel
                pLastCol = pYTemp + nhor;       
                while (pYTemp < pLastCol) 
                {//Do all the columns, YYYYUV
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pCbTemp;
                        *pDIB++ = *pCrTemp;
                        pCbTemp += 2;
                        pCrTemp += 2;                           
                }
                //Update the Luminance pointer
                pY += pic->y.hoffset;
                
                // Do another row. Chroma only has 1/2 luma rows
                // So replicate the chroma row again.
                pCbTemp = pCb;
                pCrTemp = pCr;
                pYTemp = pY;
                //Pointer to one pel past the last pel
                pLastCol = pYTemp + nhor;       
                while (pYTemp < pLastCol) 
                {//Do all the columns, YYYYUV
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pYTemp++;
                        *pDIB++ = *pCbTemp;
                        *pDIB++ = *pCrTemp;
                        pCbTemp += 2;   //4:1 chroma subsampling.  Chroma is already
                        pCrTemp += 2;   //subsampled 2:1 so subsample by another 2:                             
                }
                //Update the Luminance pointer to point to the next row
                pY += pic->y.hoffset;
                
                //OK, did 2 luma rows, now update the chroma pointers
                pCr += pic->cr.hoffset;
                pCb += pic->cb.hoffset; 
        }
    return( OK );
}
*/
/* #ifndef TESTING ... */
#endif

/* #if !defined(__WATCOMC__) || defined(WIN95_VXD) ... */
#endif

//***************************************************************************
// DoYUY2 - Convert input block to YUY2 format (Y Cb Y Cr Y Cb Y Cr
//                      From Planar Y U V 4:1:1 format.  2:1 vertical interpolation is 
//                      done using line replication.
//**************************************************************************
static int  DoYUY2(     int cHor, int cVert, 
                    PIXEL *pY, int offsetY, 
                    PIXEL *pCb, int offsetCb, 
                    PIXEL *pCr, int offsetCr, 
                    U16 FAR dib[], int offsetDIB)
{
        PIXEL           *pLastRow, *pLastCol;
        PIXEL           *pYTemp, *pCbTemp, *pCrTemp;
        PIXEL FAR       *pDIB;
        U32       FAR   *pDIBTemp;
        U32                     wBuffer;
        
        //Get pointer to the one row past last Row.
        pLastRow = pY + (offsetY * (cVert));
        
        //Get a pointer to the DIB
        pDIB = (PIXEL FAR *) &dib[0];
        
        while (pY < pLastRow)
        {//For each row
                pCbTemp = pCb;
                pCrTemp = pCr;
                pYTemp = pY;
                pDIBTemp = (U32 FAR *)pDIB;
                //Pointer to one pel past the last pel
                pLastCol = pYTemp + cHor;       
                while (pYTemp < pLastCol) 
                {//Do all the columns, Y Cb Y Cr Y ...
                        //*pDIBTemp++ = *pYTemp++;
                        //*pDIBTemp++ = *pCbTemp++;     
                        //*pDIBTemp++ = *pYTemp++;
                        //*pDIBTemp++ = *pCrTemp++;
                        wBuffer = *pYTemp++;
                        wBuffer |= (*pCbTemp++)<<8;     
                        wBuffer |= (*pYTemp++)<<16;
                        wBuffer |= (*pCrTemp++)<<24;
                        *pDIBTemp++ = wBuffer;                  
                }
                //Update the Luminance pointer
                pY += offsetY;
                //Update the DIB pointer to the next row
                pDIB += offsetDIB;                      
                // Do another row. Chroma only has 1/2 luma rows
                // So replicate the chroma row again.
                pCbTemp = pCb;
                pCrTemp = pCr;
                pYTemp = pY;
                pDIBTemp = (U32 FAR *)pDIB;
                //Pointer to one pel past the last pel
                pLastCol = pYTemp + cHor;       
                while (pYTemp < pLastCol) 
                {//Do all the columns, Y Cb Y Cr Y Cb ... 
                        //*pDIBTemp++ = *pYTemp++;
                        //*pDIBTemp++ = *pCbTemp++;     
                        //*pDIBTemp++ = *pYTemp++;
                        //*pDIBTemp++ = *pCrTemp++;
                        wBuffer = *pYTemp++;
                        wBuffer |= (*pCbTemp++)<<8;     
                        wBuffer |= (*pYTemp++)<<16;
                        wBuffer |= (*pCrTemp++)<<24;
                        *pDIBTemp++ = wBuffer;                  
                }
                //Update the Luminance pointer to point to the next row
                pY += offsetY;
                //Update the DIB pointer to the next row
                pDIB += offsetDIB;      
                //OK, did 2 luma rows, now update the chroma pointers
                pCr += offsetCr;
                pCb += offsetCb;        
        }
    return( OK );
}
//***************************************************************************
// PicToYUY2 - Convert Vivo PICTURE to YUY2 YUYV ...
//**************************************************************************
static int  PicToYUY2(PICTURE *pic, U16 far dib[], int bSelectiveUpdates, U8 MBmap[] )
{
    PIXEL   *pY, *pCb, *pCr;
    U16 FAR *pDIB;
    int nhor, nvert;
    int mapHorz, mapVert, jmap, imap, vert_index;
    
    //Get dimension of Chroma image 
    nhor = pic->y.nhor;
    nvert = pic->y.nvert;

        //For debugging
    //sprintf(msg,"Calling PicToYUY2\n");
    //OutputDebugString(msg);
    
    //Get pointer to output DIB
    pDIB = &dib[0]; 
    
    //point to input rows. All images are right-side-up
    pY = pic->y.ptr;
    
    //last rows of chroma
    pCb = pic->cb.ptr;  
    pCr = pic->cr.ptr;
    
    //either convert the whole picture or convert only the 8x8 blocks which were updated.
    if (NO == bSelectiveUpdates) {    
        // Convert whole picture
        DoYUY2( nhor, nvert, pY, (int) pic->y.hoffset,
                    pCb, (int) pic->cb.hoffset, 
                    pCr, (int) pic->cr.hoffset, 
                    pDIB, (int) 2*pic->y.nhor );
        
    } else {    
        //Convert Macroblocks that have non-zero map
        //16x16 luma pels per Macroblock 
        mapHorz = nhor >> 4;
        mapVert = nvert >> 4;
        vert_index = 0;
        for ( jmap = 0; jmap < mapVert; jmap++ ) {
            for ( imap = 0; imap < mapHorz; imap++ ) {
                if ( MBmap[vert_index + imap] != 0 ) {
                    MBmap[vert_index + imap] = 0;
                    DoYUY2( 16, 16, pY + 16*imap, (int) pic->y.hoffset,
                            pCb + 8*imap, (int) pic->cb.hoffset, 
                            pCr + 8*imap, (int) pic->cr.hoffset, 
                            pDIB + 16*imap, (int) 2*pic->y.nhor );
                }
            }
            //Update pointer into Macroblock active map
            vert_index += mapHorz;
            //pDIB is a 16 bit pointer!
            pDIB += 16 * pic->y.nhor;
            pY += 16 * pic->y.hoffset;
            pCb += 8 * pic->cb.hoffset;
            pCr += 8 * pic->cr.hoffset;
        }
    }
    return( OK );
}


// interpolateComponent - Stretch the picture in-place by bilinear interpolation
static void interpolateComponent( PIXEL In[],  // Upper left pixel of input picture array
                                  PIXEL Out[], // Upper left pixel of output picture array
                                  int   nhor,   // Output pixels per line
                                  int   nvert,  // Number of lines in output
                                  int   hInOffset,// Bytes between input rows
                                  int   hOutOffset,// Bytes between ouput rows
                                  int   inHeight,// Number of lines in input
                                  int   inWidth, // Number of cols in input
                                  float scale   // Stretch factor (do nothing if < 1.00001)
                                  )
{
#define MAX_DIM (352)
    int     row, col, intIndex;
    long    increment, inputIndex, weight;  // 16.16 format (16 fractional bits)
    PIXEL   *pIn,*pOut;
    static int temp[MAX_DIM];
    int     maxVert, maxHor;

    if (scale < 1.00001  ||  nhor > MAX_DIM  ||  nvert > MAX_DIM)
        return;
    increment = (int)(65536. / scale);  // Bump input this much for each output pixel

    maxHor = (int) ceil((inWidth - 1.0)*scale);
    maxVert = (int) ceil((inHeight - 1.0)*scale);


    // Do horizontal interpolation
    pIn = In;
    pOut = Out;
    for (row = 0; row < inHeight; ++row) {
        inputIndex = 0;
        for (col = 0; col < maxHor; ++col) {
            intIndex = inputIndex >> 16;    // Integer part of input pointer
            weight = inputIndex & 0xffff;   // Fractional part
            pOut[col] = pIn[intIndex] 
                + ((weight * (pIn[intIndex + 1] - pIn[intIndex]) + 0x8000) >> 16);
            inputIndex += increment;
        }
        for( ; col < nhor; ++col) {
            pOut[col] = pIn[inWidth-1];
        }
        pIn += hInOffset;
        pOut += hOutOffset;
    }

    // Do vertical interpolation
    for (col = 0; col < nhor; ++col) {
        pOut = Out + col;
        for (row = 0; row < inHeight; ++row) {
            temp[row] = *pOut;
            pOut += hOutOffset;
        }
        pOut = Out + col;
        inputIndex = 0;
        for (row = 0; row < maxVert; ++row) {
            intIndex = inputIndex >> 16;    // Integer part of input pointer
            weight = inputIndex & 0xffff;   // Fractional part
            *pOut = temp[intIndex] 
                + ((weight * (temp[intIndex + 1] - temp[intIndex]) + 0x8000) >> 16);
            inputIndex += increment;
            pOut += hOutOffset;
        }
        for( ; row < nvert; ++row) {
            *pOut = temp[inHeight - 1];
            pOut += hOutOffset;
        }
    }
    return;
}


void RescalePICTURE(PICTURE *In, PICTURE *Out, S32 srcHeight, S32 srcWidth, FLOAT scale)
{
    // We are assuming that the Out PICTURE has been allocated with enough 
    // room for the scaled picture
    interpolateComponent(In->y.ptr, Out->y.ptr, Out->y.nhor, Out->y.nvert, In->y.hoffset, 
        Out->y.hoffset, srcHeight, srcWidth, scale);
    
    interpolateComponent(In->cb.ptr, Out->cb.ptr, Out->cb.nhor, Out->cb.nvert, In->cb.hoffset, 
        Out->cb.hoffset, (srcHeight+1)>>1, (srcWidth+1)>>1, scale);
    
    interpolateComponent(In->cr.ptr, Out->cr.ptr, Out->cr.nhor, Out->cr.nvert, In->cr.hoffset, 
        Out->cr.hoffset, (srcHeight+1)>>1, (srcWidth+1)>>1, scale);
}

static void InitializePicConvert(PICTURE * pConvertPic)
{
    if(pConvertPic->y.ptr) return;
    pConvertPic->y.ptr = (PIXEL *)MNI_MALLOC( 2*352*288*sizeof(PIXEL) );
    pConvertPic->cb.ptr = pConvertPic->y.ptr + (352*288*sizeof(PIXEL));
    pConvertPic->cr.ptr = pConvertPic->cb.ptr + (176*144*sizeof(PIXEL));

    pConvertPic->y.hoffset = 352; 
    pConvertPic->cb.hoffset = 176;
    pConvertPic->cr.hoffset = 176;
}

/*-----------------------------------------------------------------------------
 *  Function:   DecAndScalePICTUREToDIB
 *
 *  DESCRIPTION
 *      Converts from YCrCb PICTURE structure to 16 bit DIB. This function has been changed
 *      to perform scaling on the output image appropriate and also hides the fact that the
 *      underlying encoded image may be a padded version of the original source image.  Also
 *      note that the size of outDIB that must be allocated is dstHeight*WIDTHBYTES(16*dstWidth)
 *
 *  CALLING SYNTAX
 *      PICTUREToDIB    S16     index
 *                      U32     outDIB
 *                      U32     srcWidth
 *                      U32     srcHeight
 *                      U32     dstWidth
 *                      U32     dstHeight
 *                      U32     prevPic
 *
 *      index:          Indicates which decoder is being called.
 *      outDIB:         16:16 pointer to output DIB.
 *
 *  RETURNS
 *      U32 status. 
 *
 *  Author:     David Blair     02/04/97
 *  Inspected:  
 *  Revised:
 *      03/04/97 db Use DoRGB16With3HalfsUpsample
 *      02/20/97 db Use DoRGB16With2xUpsample and H261Decoder::convertPic
 -----------------------------------------------------------------------------*/
#define CAN_USE_2x_UPSAMPLE() \
    ( ((srcWidth) & 1)==0 && ((srcHeight) & 1)==0 && (dstWidth)==2*(srcWidth) && (dstHeight)==2*(srcHeight) )
#define CAN_USE_3HALFS_UPSAMPLE() \
    ( ((srcWidth) & 3)==0 && ((srcHeight) & 3)==0 && 2*(dstWidth)==3*(srcWidth) && 2*(dstHeight)==3*(srcHeight) )

#ifndef TESTING
#ifdef _CONVERT_IN_EXE
U32 FAR PASCAL DecAndScalePICTUREToDIB ( S16 index, U32 outDIB, U32 dibRowBytes,
                                                      U32 srcWidth, U32 srcHeight,
                                                      U32 dstWidth, U32 dstHeight,
                                                      U32 prevPic )
#else
__declspec(dllexport) U32 FAR PASCAL DecAndScalePICTUREToDIB ( S16 index, U32 outDIB, U32 dibRowBytes,
                                                      U32 srcWidth, U32 srcHeight,
                                                      U32 dstWidth, U32 dstHeight,
                                                      U32 prevPic )

#endif
{
    H261Decoder *dec; 
    U16 far *fpOutDIB;
    PICTURE     *pInputPic;
    PIXEL *pY, *pCb, *pCr;
    S32 yOffset, cbOffset, crOffset;

    fpOutDIB = (U16 far * )makeFlat32( (void *)outDIB );
    dec = DecoderRegistry[index-1];
    if (prevPic) {
        pInputPic = dec->pPrevPic;
    } else {
        pInputPic = dec->pNewPic;
    }


    // Rescale frame if necessary - look for applicability of optimized
    // versions of 1:2 (requires even luma dims) and 2:3 (requires luma
    // dims multiples of 4).
    if((dstWidth != srcWidth || dstHeight != srcHeight) &&
        !CAN_USE_2x_UPSAMPLE() && !CAN_USE_3HALFS_UPSAMPLE() )
    {
        FLOAT scale = ((FLOAT)dstWidth/(FLOAT)srcWidth);
        dec->convertPic.y.nhor = dstWidth;
        dec->convertPic.y.nvert = dstHeight;
        dec->convertPic.cb.nhor = (dstWidth+1)>>1;
        dec->convertPic.cb.nvert = (dstHeight+1)>>1;
        dec->convertPic.cr.nhor = (dstWidth+1)>>1;
        dec->convertPic.cr.nvert = (dstHeight+1)>>1;

        InitializePicConvert( &dec->convertPic);
        RescalePICTURE(pInputPic, &dec->convertPic, srcHeight, srcWidth, scale);
        
        // Now turn YUV pointers upside down before passing to DoRGB16
        yOffset = dec->convertPic.y.hoffset;
        crOffset = dec->convertPic.cr.hoffset;
        cbOffset = dec->convertPic.cb.hoffset;
        pY = dec->convertPic.y.ptr + (dec->convertPic.y.nvert - 1)*dec->convertPic.y.hoffset;
        pCr = dec->convertPic.cr.ptr + (dec->convertPic.cr.nvert - 1)*dec->convertPic.cr.hoffset;
        pCb = dec->convertPic.cb.ptr + (dec->convertPic.cb.nvert - 1)*dec->convertPic.cb.hoffset;
    } else if ( CAN_USE_2x_UPSAMPLE() || CAN_USE_3HALFS_UPSAMPLE() ) {
        S32 lumaHeight = srcHeight;
        S32 chromaHeight = (srcHeight+1)>>1;
        // Now turn YUV pointers upside down before passing to DoRGB16
        pY = pInputPic->y.ptr + (lumaHeight - 1)*pInputPic->y.hoffset;
        pCr = pInputPic->cr.ptr + (chromaHeight - 1)*pInputPic->cr.hoffset;
        pCb = pInputPic->cb.ptr + (chromaHeight - 1)*pInputPic->cb.hoffset;
        yOffset = pInputPic->y.hoffset;
        crOffset = pInputPic->cr.hoffset;
        cbOffset = pInputPic->cb.hoffset;
   } else {
        S32 lumaHeight = dstHeight;
        S32 chromaHeight = (dstHeight+1)>>1;
        // Now turn YUV pointers upside down before passing to DoRGB16
        pY = pInputPic->y.ptr + (lumaHeight - 1)*pInputPic->y.hoffset;
        pCr = pInputPic->cr.ptr + (chromaHeight - 1)*pInputPic->cr.hoffset;
        pCb = pInputPic->cb.ptr + (chromaHeight - 1)*pInputPic->cb.hoffset;
        yOffset = pInputPic->y.hoffset;
        crOffset = pInputPic->cr.hoffset;
        cbOffset = pInputPic->cb.hoffset;
    }

    // Now move around pointers and call DoRGB16 -- only use DoRGB16AnyParity
    // if necessary since I haven't had time to figure how efficient it is in
    // comparison to DoRGB16.
    // NOTE: dibRowBytes needs to be halved since DIB is 16 bit
    if((dstWidth % 2) || (dstHeight % 2)) {
        DoRGB16AnyParity( dstWidth, dstHeight, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes>>1 );
    } else if ( CAN_USE_2x_UPSAMPLE() ) {
        DoRGB16With2xUpsample( srcWidth, srcHeight, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes>>1 );
    } else if ( CAN_USE_3HALFS_UPSAMPLE() ) {
        DoRGB16With3HalfsUpsample( srcWidth, srcHeight, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes>>1 );
    } else {
        DoRGB16( dstWidth>>1, dstHeight>>1, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes>>1 );
    }

    return ( OK );
}
#endif

#ifdef VCM_CODEC
/*-----------------------------------------------------------------------------
 *  Function:   DecAndScalePICTUREToDIB24
 *
 *  DESCRIPTION
 *      Converts from YCrCb PICTURE structure to 24 bit DIB. This function has been changed
 *      to perform scaling on the output image appropriate and also hides the fact that the
 *      underlying encoded image may be a padded version of the original source image.  Also
 *      note that the size of outDIB that must be allocated is dstHeight*WIDTHBYTES(16*dstWidth)
 *
 *  CALLING SYNTAX
 *      PICTUREToDIB    S16     index
 *                      U32     outDIB
 *                      U32     srcWidth
 *                      U32     srcHeight
 *                      U32     dstWidth
 *                      U32     dstHeight
 *                      U32     prevPic
 *                      U32     bitsPerPixel
 *
 *      index:          Indicates which decoder is being called.
 *      outDIB:         16:16 pointer to output DIB.
 *
 *  RETURNS
 *      U32 status. 
 *
 *  Author:     David Blair     03/27/97
 *  Inspected:  
 *  Revised:
 -----------------------------------------------------------------------------*/
#ifndef TESTING
#ifdef _CONVERT_IN_EXE
U32 FAR PASCAL DecAndScalePICTUREToDIB24 ( S16 index, U32 outDIB, U32 dibRowBytes,
                                                      U32 srcWidth, U32 srcHeight,
                                                      U32 dstWidth, U32 dstHeight,
                                                      U32 prevPic,
													  U32 bitsPerPixel)
#else
__declspec(dllexport) U32 FAR PASCAL DecAndScalePICTUREToDIB24 ( S16 index, U32 outDIB, U32 dibRowBytes,
                                                      U32 srcWidth, U32 srcHeight,
                                                      U32 dstWidth, U32 dstHeight,
                                                      U32 prevPic,
													  U32 bitsPerPixel)

#endif
{
    H261Decoder *dec; 
    U8 far *fpOutDIB;
    PICTURE     *pInputPic;
    PIXEL *pY, *pCb, *pCr;
    S32 yOffset, cbOffset, crOffset;

    fpOutDIB = (U8 far * )makeFlat32( (void *)outDIB );
    dec = DecoderRegistry[index-1];
    if (prevPic) {
        pInputPic = dec->pPrevPic;
    } else {
        pInputPic = dec->pNewPic;
    }


    // Rescale frame if necessary - look for applicability of optimized
    // versions of 1:2 (requires even luma dims) and 2:3 (requires luma
    // dims multiples of 4).
    if((dstWidth != srcWidth || dstHeight != srcHeight))
    {
        FLOAT scale = ((FLOAT)dstWidth/(FLOAT)srcWidth);
        dec->convertPic.y.nhor = dstWidth;
        dec->convertPic.y.nvert = dstHeight;
        dec->convertPic.cb.nhor = (dstWidth+1)>>1;
        dec->convertPic.cb.nvert = (dstHeight+1)>>1;
        dec->convertPic.cr.nhor = (dstWidth+1)>>1;
        dec->convertPic.cr.nvert = (dstHeight+1)>>1;

        InitializePicConvert( &dec->convertPic);
        RescalePICTURE(pInputPic, &dec->convertPic, srcHeight, srcWidth, scale);
        
        // Now turn YUV pointers upside down before passing to DoRGB16
        yOffset = dec->convertPic.y.hoffset;
        crOffset = dec->convertPic.cr.hoffset;
        cbOffset = dec->convertPic.cb.hoffset;
        pY = dec->convertPic.y.ptr + (dec->convertPic.y.nvert - 1)*dec->convertPic.y.hoffset;
        pCr = dec->convertPic.cr.ptr + (dec->convertPic.cr.nvert - 1)*dec->convertPic.cr.hoffset;
        pCb = dec->convertPic.cb.ptr + (dec->convertPic.cb.nvert - 1)*dec->convertPic.cb.hoffset;
   } else {
        S32 lumaHeight = dstHeight;
        S32 chromaHeight = (dstHeight+1)>>1;
        // Now turn YUV pointers upside down before passing to DoRGB16
        pY = pInputPic->y.ptr + (lumaHeight - 1)*pInputPic->y.hoffset;
        pCr = pInputPic->cr.ptr + (chromaHeight - 1)*pInputPic->cr.hoffset;
        pCb = pInputPic->cb.ptr + (chromaHeight - 1)*pInputPic->cb.hoffset;
        yOffset = pInputPic->y.hoffset;
        crOffset = pInputPic->cr.hoffset;
        cbOffset = pInputPic->cb.hoffset;
    }

    // Now move around pointers and call DoRGB24 -- only use DoRGB24AnyParity
    // if necessary since I haven't had time to figure how efficient it is in
    // comparison to DoRGB24.
    if((dstWidth % 2) || (dstHeight % 2)) {
        if(bitsPerPixel==24)
		{
			DoRGB24AnyParity( dstWidth, dstHeight, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes );
		}
		else
		{
			DoRGB32AnyParity( dstWidth, dstHeight, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes );
		}
    } else {
		if(bitsPerPixel==24)
		{
			DoRGB24( dstWidth>>1, dstHeight>>1, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes );
		}
		else
		{
			DoRGB32( dstWidth>>1, dstHeight>>1, pY, yOffset,
                    pCb, cbOffset, 
                    pCr, crOffset, 
                    fpOutDIB, (S32) dibRowBytes );
		}
    }

    return ( OK );
}
#endif
#endif


/*-----------------------------------------------------------------------------
 *  Function:   DecPICTUREToDIB
 *
 *  DESCRIPTION
 *      Converts from YCrCb PICTURE structure to DIB. DLL entry point. ( Direct
 *      translation from Ollie's display stream functions ). 
 *
 *  CALLING SYNTAX
 *      PICTUREToDIB    S16     index
 *                      U32     outDIB
 *
 *      index:          Indicates which decoder is being called.
 *      outDIB:         16:16 pointer to output DIB.
 *
 *  RETURNS
 *      U32 status. 
 *
 *  Author:     Mary Deshon     11/05/93
 *  Inspected:  Staffan Ericsson 4/13/94
 *  Revised:
 *      12/01/96 se Use doublebuffered output in DecPICTUREToDIB
 *      10/22/96 se Support PB-frames
 *      4/15/94 SE  Various modifications and optimizations
 *      1/17/93 md  Added support for conditional updates.
 -----------------------------------------------------------------------------*/

#ifndef TESTING
#ifdef _CONVERT_IN_EXE
#ifdef FOR_MAC
U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, 
                                 U32 dibFormat, U32 selectiveUpdates, U32 prevPic, U16 bytesPerRow )
#else
U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB,  U32 dibFormat,
                                 U32 selectiveUpdates, U32 prevPic )
#endif
#else
#ifdef FOR_MAC
__declspec(dllexport) U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, 
                                  U32 dibFormat, U32 selectiveUpdates, U32 prevPic, U16 bytesPerRow )
#else
__declspec(dllexport) U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, 
                                  U32 dibFormat, U32 selectiveUpdates, U32 prevPic )
#endif
#endif
{
    H261Decoder *dec; 
    U16 far *fpOutDIB;
    PICTURE     *inputPic;
    
    fpOutDIB = (U16 far * )makeFlat32( (void *)outDIB );
    dec = DecoderRegistry[index-1];
    if (prevPic) {
        inputPic = dec->pPrevPic;
    } else {
        inputPic = dec->pNewPic;
    }
    /*if (dec->pendingFrame)
        inputPic = &dec->B_Out;     // Display B-frame
    else
        inputPic = &dec->oldOut;    // Display P (or I) frame*/


    if (VIVO_RGB555 == dibFormat)
#ifdef  SELECTIVE_UPDATE
#ifdef FOR_MAC
        PicToRGB16( inputPic, fpOutDIB, (int)selectiveUpdates, dec->decMB.data, bytesPerRow );
#else
        PicToRGB16( inputPic, fpOutDIB, (int)selectiveUpdates, dec->decMB.data );
#endif
#else
        PicToRGB16( inputPic, fpOutDIB, NO, dec->decMB.data );
#endif
        else if (VIVO_YVU9 == dibFormat)
                PicToYVU9( inputPic, fpOutDIB);
        else if (VIVO_YUY2 == dibFormat)
                PicToYUY2( inputPic, fpOutDIB, (int)selectiveUpdates, dec->decMB.data);
        else
                H261ErrMsg ("DecPICTUREToDIB: Bad Dib Format");         
    return ( OK );
}
#endif


// PicToRGB16 - Convert Vivo PICTURE to 16-bit DIB
#ifdef FOR_MAC
extern int  PicToRGB16( PICTURE *pic, U16 far dib[], int select, U8 MBmap[], S16 bytesPerRow )
#else
extern int  PicToRGB16( PICTURE *pic, U16 far dib[], int select, U8 MBmap[] )
#endif
{
    PIXEL   *pY, *pCb, *pCr;
    U16 far *pDIB;
    int nhor, nvert;
    int mapHorz, mapVert, jmap, imap, vert_index;
     
    nhor = pic->cb.nhor;
    nvert = pic->cb.nvert;

    //sprintf(msg,"Calling DORGB16\n");
    //OutputDebugString(msg);
        
    /* point to output DIB */
    pDIB = &dib[0];
    /* point to input rows (start with the last one) */
    pY = pic->y.ptr;
    /* bottom row */
    pY += (pic->y.hoffset * ( pic->y.nvert - 1) ) ;
    /* last rows of chroma */
    pCb = pic->cb.ptr;
    pCb += ( pic->cb.hoffset * (pic->cb.nvert - 1) ) ;
    pCr = pic->cr.ptr;
    pCr += ( pic->cr.hoffset * (pic->cr.nvert - 1) ) ;
    if (select == NO) {    // Convert whole picture
#ifdef FOR_MAC
        DoRGB16( nhor, nvert, pY, (int) pic->y.hoffset,
                    pCb, (int) pic->cb.hoffset, 
                    pCr, (int) pic->cr.hoffset, 
                    pDIB, (bytesPerRow ? bytesPerRow / 2 : (int) pic->y.nhor) );
#else
        DoRGB16( nhor, nvert, pY, (int) pic->y.hoffset,
                    pCb, (int) pic->cb.hoffset, 
                    pCr, (int) pic->cr.hoffset, 
                    pDIB, (int) pic->y.nhor );
#endif        
    } else {    // Convert blocks that have non-zero map
        mapHorz = nhor >> 3;
        mapVert = nvert >> 3;
        vert_index = mapVert * mapHorz;
        for ( jmap = 0; jmap < mapVert; jmap++ ) {
            vert_index -= mapHorz;
            for ( imap = 0; imap < mapHorz; imap++ ) {
                if ( MBmap[vert_index + imap] != 0 ) {
                    MBmap[vert_index + imap] = 0;
#ifdef FOR_MAC
                    DoRGB16( 8, 8, pY + 16*imap, (int) pic->y.hoffset,
                            pCb + 8*imap, (int) pic->cb.hoffset, 
                            pCr + 8*imap, (int) pic->cr.hoffset, 
                            pDIB + 16*imap,
                            (bytesPerRow ? bytesPerRow / 2 : (int) pic->y.nhor) );
#else
                    DoRGB16( 8, 8, pY + 16*imap, (int) pic->y.hoffset,
                            pCb + 8*imap, (int) pic->cb.hoffset, 
                            pCr + 8*imap, (int) pic->cr.hoffset, 
                            pDIB + 16*imap, (int) pic->y.nhor );
#endif
                }
            }
            pDIB += 16 * pic->y.nhor;
            pY -= 16 * pic->y.hoffset;
            pCb -= 8 * pic->cb.hoffset;
            pCr -= 8 * pic->cr.hoffset;
        }
    }
    return( OK );
}

#ifdef DEBUG
static int  ShowDIB( short *myDIB )
{
    FILE    *ofp;
    char    out[80];
    U8      tout[176];
    int i,j;
    static int nframe = 0;

    wsprintf (out, "f%6.6ld.BIN", nframe++);
    if ((ofp = fopen(out, "wb")) == NULL) {
        exit(0);
    }
    for ( j = 0; j < 144; j++) {
        for ( i = 0; i < 176; i++ )
            tout[i] = (U8)((*(myDIB + i + 176*(143-j)) & 0x001f) << 3);
        fwrite( tout, 1, 176, ofp );
    }
    fclose(ofp);
    
    return (OK);
}

static int  ShowMap( U8 *map )
{
    FILE    *ofp;
    char    out[80];
    int i,j;
    static int nframe = 1;

    wsprintf (out, "f%6.6ld.MAP", 1);
    if ((ofp = fopen(out, "a")) == NULL) {
        exit(0);
    }
    fprintf ( ofp, "\nFrame: %d\n", nframe++ );
    for ( j = 0; j < 9; j++) {
        for ( i = 0; i < 11; i++ ) {
            fprintf ( ofp, "%d ",map[i + 11*(8-j)]);
        }
    fprintf ( ofp, "\n" );
    }
    fclose(ofp);
    return (OK);
}
#endif
