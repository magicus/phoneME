/*-----------------------------------------------------------------------------
 *  H261TYPE.H
 *
 *  DESCRIPTION   
 *      Type definitions for H.261 codec.
 *
 *  Author:     Staffan Ericsson    6/28/93
 *  Inspector:  Mary Deshon         7/6/93
 *  Revised:
 *  11/02/96    S Ericsson      Support PB-frames
 *  04/22/96    S Ericsson      Added "quality" to MACROBLOCK_DESCR
 *  11/27/95    md              Added counter to MACROBLOCK_DESCR.
 *  07/08/95    S Ericsson      Modified MACROBLOCK_DESCR for H.263
 *  04/23/94    S Ericsson      Added intraVar & predErr to BLOCK_DESCR
 *                              Added lumaVar to MACROBLOCK_DESCR
 *  04/13/94    wolfe           Changed pointers to P32 (compiles in both
 *                              .DLLs and .EXE to same size structures.
 *  02/23/94    Mary Deshon     Frame border.
 *  7/06/93     Mary Deshon     Minor style changes
 *  7/06/93     Mary Deshon     Changed typedefs to S8, U8, ... ,S32, U32
 *  7/14/93     Mary Deshon     Added vXoffset & vYoffset to PICTURE
 *  7/20/93     S Ericsson      Replaced .pred by .active in MACROBLOCK_DESCR
 *  7/23/93     S Ericsson      Fixed type for macroblock_descr.mvd_x/y
 *  7/25/93     M Deshon        Moved COMPONENT, PICTURE, PIC_DESCR structures
 *                              into vvenc.h
 *  7/26/93     M Deshon        Removed U8,S8,.. Ollie had these in machine.h
 *
 *  (c) 1993-1997, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
 
#ifndef _INC_H261TYPE
#define _INC_H261TYPE   1
#include "machine.h"

//typedef signed char       S8;         // 8 bits signed
//typedef unsigned char     U8;         // 8 bits unsigned
//typedef short int         S16;        // 16 bits signed
//typedef unsigned short int U16;       // 16 bits unsigned
//typedef long int          S32;        // 32 bits signed
//typedef unsigned long int U32;        // 32 bits unsigned
//typedef unsigned char     PIXEL;      // 8 bits unsigned

/* SYMBOL - Symbol descriptor */
typedef struct {
    S8              value;
    S8              type;
} SYMBOL;

/* BS_PTR - Pointer to current position in bitstream */
typedef struct {
    U8          P32 byteptr;    // Points to current byte
    int             bitptr;     // Number of bits already consumed/processed in
                                // current byte, i.e., points at bit (7-bitptr)
} BS_PTR;

/* BLOCK_DESCR - Descriptor for symbols in a block */
typedef struct {
    SYMBOL      P32 sym;        // Points to array of run-level pairs
    int             nsym;       // Number of run-level pairs in array
    int             intraVar;   // Block variance
    int             predErr;    // Prediction error (undefined for INTRA blocks)
} BLOCK_DESCR;

/* MACROBLOCK_DESCR - 256-byte descriptor for MB, that consists of 6 blocks */
/*** NOTE! This structure MUST be 256 bytes in size for efficiency.  ***/
typedef struct {
    BLOCK_DESCR     block[6];
    BLOCK_DESCR     Bblock[6];  // Descriptors for B-blocks
    U32             codedCount; // Number of times macroblock has been coded
    int             lumaVar;    // Average of 4 luminance block variances (intraVar)
    int             dquant;     // H.263: differential value for quant
    int             quality;    // Quality index used to select "quant" and HF attenuation
    U8              mtype;
    U8              active;     // Indicates active part of picture for INTRA blocks;
                                // Undefined for other MTYPEs
    U8              quant;		// Quantizer step
    U8              cbp;		// Coded block pattern
    U8              x;          // mb_column
    U8              y;          // mb_row
    S8              mv_x;       // Hor. component of motion vector
    S8              mv_y;       // Vert. component of motion vector
    S8              mvd_x;      // Hor. motion vector difference
    S8              mvd_y;      // Vert. motion vector difference
    U8              modB;       // Mode for B-blocks; defined for PB frames only    
    U8              intra_mode; // Intra prediction mode; defined for advanced intra mode only
    S8              blkMvX[4];  // Hor. motion for 8x8 blocks; defined when mtype=INTER4V
    S8              blkMvY[4];  // Vert. motion for 8x8 blocks
    S8              blkDiffX[4]; // Hor. mv diff. for 8x8 blocks (mtype=INTER4V)
    S8              blkDiffY[4]; // Vert. mv difference for 8x8 blocks
    U8              Bquant;     // quant for B-frame
    U8              cbpB;       // Coded block pattern for B-blocks; defined if modB > 1
    S8              mvdB_x;     // Mv data for B-macroblock; defined if modB > 0
    S8              mvdB_y;
    S8              blkMvFx[4]; // Hor. motion for forward prediction of B-frame
    S8              blkMvFy[4]; // Vert. motion for forward prediction of B-frame
    S8              blkMvBx[4]; // Hor. motion for backward prediction of B-frame
    S8              blkMvBy[4]; // Vert. motion for backward prediction of B-frame
} MACROBLOCK_DESCR;

/* GOB_DESCR - Descriptor for Group Of Block */
typedef struct {
    int                     gn;         // GOB Number
    int                     next_gn;    // Next GOB Number
    int                     gquant;
    int                     gei;
    int                     num_mb;     // Number of macroblocks
    MACROBLOCK_DESCR    P32 mb;       // Origin of macroblock array
    int                     first_col;  // First MB in GOB
    int                     first_row;
    int                     mb_width;   // MBs per row
    int                     mb_offset;  // Row offset
    int                     num_gspare;
    S8                  P32 gspare;
} GOB_DESCR;

/*  ENCTABENTRY - Variable length code */
typedef struct {
    U32             code;   // Variable length code, starting in MSB; max 25 bits
    int             len;    // of bits
} ENCTABENTRY;

#define DCT_8X8_SCALE       2                       /* DCT_SCALE**2 */
#define QTABLE_MAX_MAGN     (2048*DCT_8X8_SCALE)    /* 2048 * DCT_SCALE**2 */
#define MAX_Q_INDEX         127
                     /* Largest qz output */
/* Dimensions for Qtab */
#define QUALITY_STEPS       (31)
#define MASKING_STEPS       (1)

/* Structures for table-based quantization */
typedef struct QTab {
    S8  qVal[(1+2*QTABLE_MAX_MAGN)];
} QTab;
typedef struct QTab P32 pQTab;
                 
typedef struct QMatrix {
    int     maxCoeff;           // last coeff to quantize [1,64]
    pQTab   qCoeffTabs[64];     // tables
} QMatrix;
typedef struct QMatrix P32 pQMatrix;


// typedef for region-of-interest rectangle
typedef struct {
    S32 left;
    S32 top;
    S32 right;
    S32 bottom;
} ROI_RECT;

#endif
