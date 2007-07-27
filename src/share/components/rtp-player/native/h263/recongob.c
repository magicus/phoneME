/*-----------------------------------------------------------------------------
 *  RECONGOB.C
 *
 *  DESCRIPTION
 *      ReconGob.c - Image reconstruction for H.261 decoder
 *
 *      This file was split off from H261dec.c.
 *
 *      Author:     Staffan Ericsson    02/06/97
 *      Inspector:  <not inspected yet>
 *      Revised:
 *  01/06/98 S Ericsson     Set MB type for concealed blocks to SKIP (RTP support)
 *  04/04/97 S Ericsson     Check size in ReconAdvancedIntra
 *  03/21/97 S Ericsson     Modify Reduced-Res Update mode to 2/97 draft spec
 *  
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "h263plus.h"

extern S16 Recon [QUANT_MAX - QUANT_MIN + 1] [N_SYM_INDICES];

//#define DISPLAY_DIFFERENCE  // Display quantized pred. error (don't use prev. frame)

#ifdef TESTING
#define CHECKSYM(a)     a   // Check symbol types to verify decoder state tables
#else
#define CHECKSYM(a)         // Don't check symbol types
#endif

#define GRAY    (128)

#ifdef DO_H263_PLUS
extern void PredBframePlus( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );
#endif

// Declarations of local functions
static void reconBframe( MACROBLOCK_DESCR * mb, PICTURE * Bpic );
static void gray_mb( MACROBLOCK_DESCR * mb, PICTURE * pic );
static void fill_mb( MACROBLOCK_DESCR * mb, PICTURE * pic, PIXEL value );

#ifdef DO_H263_PLUS
static void idct32x32( MACROBLOCK_DESCR * mb,   // Macroblock to be reconstructed
                       PICTURE * tempPic,       // Store 16-bit IDCT values here
                       int intra                // INTER block if zero, otherwise INTRA
                       );
static void filtAddClip32x32( MACROBLOCK_DESCR * mb,// Macroblock to be reconstructed
                              PICTURE * pic,    // Input: motion-comp prediction w/ filtered
                                                // intra borders; output: reconstr. picture
                              PICTURE * tempPic // 16-bit IDCT values
                              );
static void filtAddClip( PIXEL x[], int xdim,       // Output pixels
                         S16 idct_out[], int idim,  // Input IDCT values
                         int hSize, int vSize       // Input block size for IDCT values
                        );
#endif

// ReconGob - Reconstruct one GOB of a picture
// state->i > 0 indicates that we are reentering after previous timeout
extern int ReconGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[],
                     PICTURE * prev_pic, PICTURE * pic, PICTURE * Bpic, 
                     int advancedPred, int PBframe, int PBframeCap, int reducedResUpdate,
                     int advancedIntraMode, H261DecState *state, int maxComp )
#define CLEAN NO
{
    int mbnum, i, col, dummy[4], reconBflag;
    int intra;
    PICTURE *tempPic;
    CHECKSYM( char msg[120]; )

    if (PBframe && PBframeCap) {
        reconBflag = 1; // Reconstruct B frame
    } else {
        reconBflag = 0; // Don't do B frame
    }
    tempPic = Bpic; // Use as work array in Reduced-res. Update mode
    if (state->i > 0) {
        // We timed out last time: restore state
        i     = state->i;
        mbnum = state->mbnum;
        col   = state->col;
    } else {
        // Reset state
        i = 0;
        mbnum = gob->first_col + gob->first_row * gob->mb_offset;
        col = gob->first_col;   // Used to check when we reach end of line
    }
#ifndef DISPLAY_DIFFERENCE  // Do proper reconstruction
    if (pic->y.nhor == prev_pic->y.nhor  &&  pic->y.nvert == prev_pic->y.nvert) {
        // Size of prev_pic is OK; use it if specified
        while (i < gob->num_mb) {
            //printf("recon_gob:  mbnum = %d   type = %d\n", mbnum, mb[mbnum].mtype);
            switch (mb[mbnum].mtype) {
            case MTYPE_SKIP:
                mb[mbnum].mv_x = 0;
                mb[mbnum].mv_y = 0;
                if (reducedResUpdate) {
                    MotionComp32x32( &mb[mbnum], prev_pic, pic);
                    if (advancedPred) {
                        Overlap32x32( &mb[mbnum], prev_pic, pic,
                                gob->mb_width, gob->mb_offset, dummy );
                    }
                } else {
                    MotionComp( &mb[mbnum], prev_pic, pic);
                    if (advancedPred) {
                        OverlapMC( &mb[mbnum], PBframe, prev_pic, pic,
                                gob->mb_width, gob->mb_offset, dummy );
                    }
                    if (reconBflag) {   // Repeat prev. picture in B frame
                        MotionComp( &mb[mbnum], prev_pic, Bpic );
                    }
                }
                break;
            
            case MTYPE263_INTER:
            case MTYPE263_INTER_Q:
            case MTYPE263_INTER4V:
                if (reducedResUpdate) {
                    MotionComp32x32( &mb[mbnum], prev_pic, pic);
                    if (advancedPred) {
                        Overlap32x32( &mb[mbnum], prev_pic, pic,
                                gob->mb_width, gob->mb_offset, dummy );
                        state->actComp += 4;   // Increment computation measure
                    }
                    intra = NO;
                    ReconReducedResMb( &mb[mbnum], pic, intra, tempPic );
                    state->actComp += 4;   // Increment computation measure
                } else {
                    MotionComp263( &mb[mbnum], prev_pic, pic);
                    if (advancedPred) {
                        OverlapMC( &mb[mbnum], PBframe, prev_pic, pic,
                                gob->mb_width, gob->mb_offset, dummy );
                        ++state->actComp;   // Increment computation measure
                    }
                    ReconInter( &mb[mbnum], pic, CLEAN);
                    ++state->actComp;   // Increment computation measure
                    if (reconBflag) {   // Reconstruct B frame
					    if(PBframe == H263PLUS_IMPROVED_PBFRAME_MODE) {
						    PredBframePlus( &mb[mbnum], prev_pic, pic, Bpic );
					    } else {
						    PredBframe( &mb[mbnum], prev_pic, pic, Bpic );
					    }
                        reconBframe( &mb[mbnum], Bpic );
                        ++state->actComp;   // Increment computation measure
                    }
                }
                break;
            case MTYPE263_INTRA:
            case MTYPE263_INTRA_Q:
                if (reducedResUpdate) {
                    Fill32x32( &mb[mbnum], pic, 0 );
                    intra = YES;
                    ReconReducedResMb( &mb[mbnum], pic, intra, tempPic );
                    state->actComp += 4;   // Increment computation measure
                } else {
                    if(advancedIntraMode)
                        ReconAdvancedIntra( &mb[mbnum], pic, CLEAN );
                    else
                        ReconIntra( &mb[mbnum], pic, CLEAN );
                    ++state->actComp;   // Increment computation measure
                    if (reconBflag) {   // Reconstruct B frame
                        if(PBframe==H263PLUS_IMPROVED_PBFRAME_MODE) {
						    PredBframePlus( &mb[mbnum], prev_pic, pic, Bpic );
					    } else {
                            PredBframe( &mb[mbnum], prev_pic, pic, Bpic );
                        }
                        reconBframe( &mb[mbnum], Bpic );
                        ++state->actComp;   // Increment computation measure
                    }
                }
                break;
            
            case MTYPE_INTER:
            case MTYPE_INTER_MQUANT:
                mb[mbnum].mv_x = 0;
                mb[mbnum].mv_y = 0;
                MotionComp( &mb[mbnum], prev_pic, pic);
                ReconInter( &mb[mbnum], pic, CLEAN);
                ++state->actComp;   // Increment computation measure
                break;
            case MTYPE_MCFILT_CBP:
            case MTYPE_MCFILT_MQUANT:
                LoopFilter( &mb[mbnum], prev_pic, pic);
                ReconInter( &mb[mbnum], pic, CLEAN);
                state->actComp += 2;    // Increment computation measure
                break;
            case MTYPE_MCFILT_NOCBP:
                LoopFilter( &mb[mbnum], prev_pic, pic);
                ++state->actComp;   // Increment computation measure
                break;
            case MTYPE_INTRA:
            case MTYPE_INTRA_MQUANT:
                //printf("Calling ReconIntra, mbnum = %d \n", mbnum);
                ReconIntra( &mb[mbnum], pic, CLEAN );
                ++state->actComp;   // Increment computation measure
                break;
            case MTYPE_MC_CBP:
            case MTYPE_MC_MQUANT:
                MotionComp( &mb[mbnum], prev_pic, pic);
                ReconInter( &mb[mbnum], pic, CLEAN);
                ++state->actComp;   // Increment computation measure
                break;
            case MTYPE_MC_NOCBP:
                MotionComp( &mb[mbnum], prev_pic, pic);
                break;
            default:
                CHECKSYM( sprintf( msg, "PROGRAM ERROR: MTYPE = %d in recon_gob", mb[mbnum].mtype); \
                    H261ErrMsg( msg );\
                    state->i = 0;  \
                    return( H261_ERROR ); )
                break;
            }
            i++, mbnum++;
            col++;
            if (col == gob->mb_width) { // Start on next row of macroblocks
                mbnum += gob->mb_offset - gob->mb_width;
                col = 0;
            }
            if (maxComp > 0  &&  state->actComp >= maxComp) {
                // We have timed out: save state and return
                state->i    = i;
                state->mbnum= mbnum;
                state->col  = col;
                return OK;
            }
        }
        state->i = 0;   // Indicate that we finished without timing out
        return (OK);
    } else
#endif
    {   // Don't use prev_pic, i.e., reconstruct difference image
        while (i < gob->num_mb) {
            switch (mb[mbnum].mtype) {
            case MTYPE_SKIP:
            case MTYPE_MCFILT_NOCBP:
            case MTYPE_MC_NOCBP:
                if (reducedResUpdate) {
                    Fill32x32( &mb[mbnum], pic, GRAY);
                } else {
                    gray_mb( &mb[mbnum], pic);
                    if (reconBflag) {   // Reconstruct B frame
                        gray_mb( &mb[mbnum], Bpic );
                    }
                }
                break;
            case MTYPE263_INTER:
            case MTYPE263_INTER_Q:
            case MTYPE263_INTER4V:
            case MTYPE_INTER:
            case MTYPE_INTER_MQUANT:
            case MTYPE_MCFILT_CBP:
            case MTYPE_MCFILT_MQUANT:
            case MTYPE_MC_CBP:
            case MTYPE_MC_MQUANT:
                if (reducedResUpdate) {
                    Fill32x32( &mb[mbnum], pic, GRAY);
                    intra = NO;
                    ReconReducedResMb( &mb[mbnum], pic, intra, tempPic );
                    state->actComp += 4;   // Increment computation measure
                } else {
                    gray_mb( &mb[mbnum], pic);
                    ReconInter( &mb[mbnum], pic, CLEAN);
                    ++state->actComp;   // Increment computation measure
                    if (reconBflag) {   // Reconstruct B frame
                        gray_mb( &mb[mbnum], Bpic );
                        reconBframe( &mb[mbnum], Bpic );
                    }
                }
                break;
            case MTYPE263_INTRA:
            case MTYPE263_INTRA_Q:
            case MTYPE_INTRA:
            case MTYPE_INTRA_MQUANT:
                if (reducedResUpdate) {
                    Fill32x32( &mb[mbnum], pic, 0 );
                    intra = YES;
                    ReconReducedResMb( &mb[mbnum], pic, intra, tempPic );
                    state->actComp += 4;   // Increment computation measure
                } else {
                    if(advancedIntraMode)
                        ReconAdvancedIntra( &mb[mbnum], pic, CLEAN );
                    else
                        ReconIntra( &mb[mbnum], pic, CLEAN );
                    ++state->actComp;   // Increment computation measure
                    if (reconBflag) {   // Reconstruct B frame
                        gray_mb( &mb[mbnum], Bpic );
                        reconBframe( &mb[mbnum], Bpic );
                    }
                }
                break;
            default:
                CHECKSYM( sprintf( msg, "PROGRAM ERROR: MTYPE = %d in recon_gob", mb[mbnum].mtype);\
                    H261ErrMsg( msg );\
                    state->i = 0;   \
                    return( H261_ERROR ); )
                break;
            }
            i++, mbnum++;
            col++;
            if (col == gob->mb_width) { // Start on next row of macroblocks
                mbnum += gob->mb_offset - gob->mb_width;
                col = 0;
            }
            if (maxComp > 0  &&  state->actComp >= maxComp) {
                // We have timed out: save state and return
                state->i    = i;
                state->mbnum= mbnum;
                state->col  = col;
                return H261_ERROR;
            }
        }
        state->i = 0;   // Indicate that we finished without timing out
        return (H261_ERROR);
    }
}


// Reconstruct GOB by repeating previous picture
extern int ConcealGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int reducedResUpdate,
                       PICTURE * prev_pic, PICTURE * pic )
{
    int mbnum, i, col;

    i = 0;
    mbnum = gob->first_col + gob->first_row * gob->mb_offset;
    col = gob->first_col;   // Used to check when we reach end of line
    if (pic->y.nhor == prev_pic->y.nhor  &&  pic->y.nvert == prev_pic->y.nvert) {
        // Size of prev_pic is OK; use it for new picture
        while (i < gob->num_mb) {
            mb[mbnum].mv_x = 0;
            mb[mbnum].mv_y = 0;
            mb[mbnum].mtype = MTYPE_SKIP;   /* RTP support */
            if (reducedResUpdate) {
                MotionComp32x32( &mb[mbnum], prev_pic, pic);
            } else {
                MotionComp( &mb[mbnum], prev_pic, pic);
            }
            i++, mbnum++;
            col++;
            if (col == gob->mb_width) { // Start on next row of macroblocks
                mbnum += gob->mb_offset - gob->mb_width;
                col = 0;
            }
        }
        return (OK);
    } else {    // Paint gray
        while (i < gob->num_mb) {
            if (reducedResUpdate) {
                Fill32x32( &mb[mbnum], pic, GRAY);
            } else {
                gray_mb( &mb[mbnum], pic);
            }
            i++, mbnum++;
            col++;
            if (col == gob->mb_width) { // Start on next row of macroblocks
                mbnum += gob->mb_offset - gob->mb_width;
                col = 0;
            }
        }
        return (H261_ERROR);
    }
}


// ReconIntra
extern void ReconIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean)
{
    int     row, col, offset;
    PIXEL   * pixel0;
    S16   * recon_tab;

    recon_tab = Recon[mb->quant - QUANT_MIN];
    col = 16 * mb->x;
    row = 16 * mb->y;
    /*{
        int isym;
        printf("ReconIntra: x = %d  y = %d\n", col, row);
        printf("nsym = %d %d %d %d %d %d \n", mb->block[0].nsym,
                mb->block[1].nsym, mb->block[2].nsym, mb->block[3].nsym,
                mb->block[4].nsym, mb->block[5].nsym);
        printf("Symbol number to print: ");
        scanf("%d", &isym);
        while (isym > 0) {
            printf("Luma 1: ");
            printsym( *(mb->block[0].sym + isym - 1) ); printf("\n");
            printf("Luma 2: ");
            printsym( *(mb->block[1].sym + isym - 1) ); printf("\n");
            printf("Luma 3: ");
            printsym( *(mb->block[2].sym + isym - 1) ); printf("\n");
            printf("Luma 4: ");
            printsym( *(mb->block[3].sym + isym - 1) ); printf("\n");
            printf("Symbol number to print: ");
            scanf("%d", &isym);
        }
    }*/
    pixel0 = pic->y.ptr + col + row * pic->y.hoffset;
    //printf("Luma block 1 \n");
    Idct2( mb->block[0].sym, mb->block[0].nsym, pixel0,
            pic->y.hoffset, recon_tab, clean);
    //printf("Luma block 2 \n");
    Idct2( mb->block[1].sym, mb->block[1].nsym, pixel0 + 8,
            pic->y.hoffset, recon_tab, clean);
    //printf("Luma block 3 \n");
    Idct2( mb->block[2].sym, mb->block[2].nsym, pixel0 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab, clean);
    //printf("Luma block 4 \n");
    Idct2( mb->block[3].sym, mb->block[3].nsym, pixel0 + 8 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab, clean);
    if (pic->color) {
        // Assuming same offset for Cr and Cb
        col = 8 * mb->x;
        row = 8 * mb->y;
        offset = col + row * pic->cb.hoffset;
        pixel0 = pic->cb.ptr + offset;
        //printf("CB block \n");
        Idct2( mb->block[4].sym, mb->block[4].nsym, pixel0,
            pic->cb.hoffset, recon_tab, clean);
        pixel0 = pic->cr.ptr + offset;
        //printf("CR block \n");
        Idct2( mb->block[5].sym, mb->block[5].nsym, pixel0,
            pic->cr.hoffset, recon_tab, clean);
    }
    return;
}

#ifdef DO_H263_PLUS

#define MAX_MACROBLKS_PER_ROW           (88)    /* Up to 16CIF (1408 pixels/line) */
#define UPPER_LUMA_BLOCK_CACHE_LENGTH   (2 * MAX_MACROBLKS_PER_ROW)
#define UPPER_CHROMA_BLOCK_CACHE_LENGTH (MAX_MACROBLKS_PER_ROW)

// Storage for the first rows of the lower 8x8 blocks of the intra macroblocks in the previous GOB.
// Refer to these for appropriate predictions.  For right now we make this static
// data, but should really allocate it on the heap only if advanced intra is being
// used?
static S8 upperLumaBlockCache[UPPER_LUMA_BLOCK_CACHE_LENGTH][8]; // allocated for CIF video!
static S8 upperCrBlockCache[UPPER_CHROMA_BLOCK_CACHE_LENGTH][8];   // allocated for CIF video!
static S8 upperCbBlockCache[UPPER_CHROMA_BLOCK_CACHE_LENGTH][8];   // allocated for CIF video!
// Storage for the first columns of the 8x8 blocks of the most recent intra macroblock.
// Refer to these for appropriate predictions.
static S8 leftLumaBlockCache[4][8];
static S8 leftCrBlockCache[1][8];
static S8 leftCbBlockCache[1][8];

static U8 upperDCLumaBlockCache[UPPER_LUMA_BLOCK_CACHE_LENGTH][1];
static U8 upperDCCrBlockCache[UPPER_CHROMA_BLOCK_CACHE_LENGTH][1];
static U8 upperDCCbBlockCache[UPPER_CHROMA_BLOCK_CACHE_LENGTH][1];
static U8 leftDCLumaBlockCache[4][1];
static U8 leftDCCrBlockCache[1];
static U8 leftDCCbBlockCache[1];

static void InitializeLeftCache(int qp)
{
    int i;
    for(i=0; i<4; i++) {
        //leftDCLumaBlockCache[i][0] = ( U8 )128; // MPEG4 style boundary conditions
        leftDCLumaBlockCache[i][0] = ( U8 )(512/qp); // H.263+ style boundary conditions
        memset(&leftLumaBlockCache[i][1], 0, 7);
    }

    leftDCCrBlockCache[0] = ( U8 )(512/qp); // H.263+ style boundary conditions
    memset(&leftCrBlockCache[0][1], 0, 7);
    
    leftDCCbBlockCache[0] = ( U8 )(512/qp); // H.263+ style boundary conditions
    memset(&leftCbBlockCache[0][1], 0, 7);
}

static void InitializeUpperCache(int qp, int i)
{
        //upperDCLumaBlockCache[2*i][0] = ( U8 )128; // MPEG4 style boundary conditions
        upperDCLumaBlockCache[2*i][0] = ( U8 )(512/qp); // H.263+ style boundary conditions
        memset(&upperLumaBlockCache[2*i][1], 0, 7);
        //upperDCLumaBlockCache[2*i+1][0] = ( U8 )128; // MPEG4 style boundary conditions
        upperDCLumaBlockCache[2*i+1][0] = ( U8 )(512/qp); // H.263+ style boundary conditions
        memset(&upperLumaBlockCache[2*i+1][1], 0, 7);
        
        //upperDCCrBlockCache[i][0] = ( U8 )128; // MPEG4 style boundary conditions
        upperDCCrBlockCache[i][0] = ( U8 )(512/qp); // H.263+ style boundary conditions
        memset(&upperCrBlockCache[i][1], 0, 7);
        
        //upperDCCbBlockCache[i][0] = ( U8 )128; // MPEG4 style boundary conditions
        upperDCCbBlockCache[i][0] = ( U8 )(512/qp); // H.263+ style boundary conditions
        memset(&upperCbBlockCache[i][1], 0, 7);
}


#define INTRA(mb)   (((mb).mtype >= MTYPE263_INTRA_MIN && (mb).mtype <= MTYPE263_INTRA_MAX) ? 1:0)

extern void ReconAdvancedIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean)
{
    int     row, col, offset;
    PIXEL   * pixel0;
    S16   * recon_tab;
    int   predtype = mb->intra_mode;
    int   fixedDC = mb->quant<8;
    int   numhor=pic->y.nhor>>4; // number of mb per row

    // Ensure that we are not scribbling outside the allocated cache
    if (mb->x >= MAX_MACROBLKS_PER_ROW) return;
    // Reinitialize predictors if we're on a new row or if one of our abutting macroblocks
    // isn't intra
    if(mb->x == 0 || !INTRA(mb[-1])) {
        InitializeLeftCache(fixedDC ? 4 : mb->quant);
    }
    if(mb->y == 0 || !INTRA(mb[-numhor])) {
        InitializeUpperCache(fixedDC ? 4 : mb->quant, mb->x);
    }


    recon_tab = Recon[mb->quant - QUANT_MIN];
    col = 16 * mb->x;
    row = 16 * mb->y;
    /*{
        int isym;
        printf("ReconAdvancedIntra: x = %d  y = %d\n", col, row);
        printf("nsym = %d %d %d %d %d %d \n", mb->block[0].nsym,
                mb->block[1].nsym, mb->block[2].nsym, mb->block[3].nsym,
                mb->block[4].nsym, mb->block[5].nsym);
        printf("Symbol number to print: ");
        scanf("%d", &isym);
        while (isym > 0) {
            printf("Luma 1: ");
            printsym( *(mb->block[0].sym + isym - 1) ); printf("\n");
            printf("Luma 2: ");
            printsym( *(mb->block[1].sym + isym - 1) ); printf("\n");
            printf("Luma 3: ");
            printsym( *(mb->block[2].sym + isym - 1) ); printf("\n");
            printf("Luma 4: ");
            printsym( *(mb->block[3].sym + isym - 1) ); printf("\n");
            printf("Symbol number to print: ");
            scanf("%d", &isym);
        }
    }*/
    pixel0 = pic->y.ptr + col + row * pic->y.hoffset;
    //printf("Luma block 1 \n");
    Idct2AdvancedIntra( mb->block[0].sym, mb->block[0].nsym, pixel0,
            pic->y.hoffset, recon_tab,
            upperDCLumaBlockCache[2*mb->x][0], upperLumaBlockCache[2*mb->x], // DC and AC pred for row  
            upperDCLumaBlockCache[2*mb->x],upperLumaBlockCache[2*mb->x],  // DC and AC store for row
            leftDCLumaBlockCache[1][0], leftLumaBlockCache[1],  // DC and AC pred for column
            leftDCLumaBlockCache[0], leftLumaBlockCache[0],  // DC and AC store for column
            predtype, fixedDC,
            mb->x==0, mb->y==0); // can be on left or upper boundary
       
    //printf("Luma block 2 \n");
    Idct2AdvancedIntra( mb->block[1].sym, mb->block[1].nsym, pixel0 + 8,
            pic->y.hoffset, recon_tab, 
            upperDCLumaBlockCache[2*mb->x+1][0], upperLumaBlockCache[2*mb->x+1], // DC and AC pred for row  
            upperDCLumaBlockCache[2*mb->x+1],upperLumaBlockCache[2*mb->x+1],  // DC and AC store for row
            leftDCLumaBlockCache[0][0], leftLumaBlockCache[0],  // DC and AC pred for column
            leftDCLumaBlockCache[1], leftLumaBlockCache[1],  // DC and AC store for column
            predtype, fixedDC,
            FALSE, mb->y==0); // can only be on upper boudary

    //printf("Luma block 3 \n");
    Idct2AdvancedIntra( mb->block[2].sym, mb->block[2].nsym, pixel0 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab,
            upperDCLumaBlockCache[2*mb->x][0], upperLumaBlockCache[2*mb->x], // DC and AC pred for row  
            upperDCLumaBlockCache[2*mb->x],upperLumaBlockCache[2*mb->x],  // DC and AC store for row
            leftDCLumaBlockCache[3][0], leftLumaBlockCache[3],  // DC and AC pred for column
            leftDCLumaBlockCache[2], leftLumaBlockCache[2],  // DC and AC store for column
            predtype, fixedDC,
            mb->x==0, FALSE); // can only be on left boundary

    //printf("Luma block 4 \n");
    Idct2AdvancedIntra( mb->block[3].sym, mb->block[3].nsym, pixel0 + 8 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab,
            upperDCLumaBlockCache[2*mb->x+1][0], upperLumaBlockCache[2*mb->x+1], // DC and AC pred for row  
            upperDCLumaBlockCache[2*mb->x+1],upperLumaBlockCache[2*mb->x+1],  // DC and AC store for row
            leftDCLumaBlockCache[2][0], leftLumaBlockCache[2],  // DC and AC pred for column
            leftDCLumaBlockCache[3], leftLumaBlockCache[3],  // DC and AC store for column
            predtype, fixedDC,
            FALSE, FALSE); // can't be on left or upper boundary
    
    if (pic->color) {
        // Assuming same offset for Cr and Cb
        col = 8 * mb->x;
        row = 8 * mb->y;
        offset = col + row * pic->cb.hoffset;
        pixel0 = pic->cb.ptr + offset;
        //printf("CB block \n");
        Idct2AdvancedIntra( mb->block[4].sym, mb->block[4].nsym, pixel0,
            pic->cb.hoffset, recon_tab,
            upperDCCbBlockCache[mb->x][0], upperCbBlockCache[mb->x], // DC and AC pred for row  
            upperDCCbBlockCache[mb->x],upperCbBlockCache[mb->x],  // DC and AC store for row
            leftDCCbBlockCache[0], leftCbBlockCache[0],  // DC and AC pred for column
            leftDCCbBlockCache, leftCbBlockCache[0],  // DC and AC store for column
            predtype, fixedDC,
            mb->x==0, mb->y==0);  // what boundaries are we on?
 
        pixel0 = pic->cr.ptr + offset;
        //printf("CR block \n");
        Idct2AdvancedIntra( mb->block[5].sym, mb->block[5].nsym, pixel0,
            pic->cr.hoffset, recon_tab,
            upperDCCrBlockCache[mb->x][0], upperCrBlockCache[mb->x], // DC and AC pred for row  
            upperDCCrBlockCache[mb->x],upperCrBlockCache[mb->x],  // DC and AC store for row
            leftDCCrBlockCache[0], leftCrBlockCache[0],  // DC and AC pred for column
            leftDCCrBlockCache, leftCrBlockCache[0],  // DC and AC store for column
            predtype, fixedDC,
            mb->x==0, mb->y==0); // what boundaries are we on?
    }
    return;
}

#endif


// ReconInter
extern void ReconInter( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean )
{
    int     row, col, offset ;
    PIXEL   * pixel0;
    S16   * recon_tab;

    recon_tab = Recon[mb->quant - QUANT_MIN];
    col = 16 * mb->x;
    row = 16 * mb->y;
/*    {
        int isym;
        printf("ReconInter: x = %d  y = %d\n", col, row);
        printf("nsym = %d %d %d %d %d %d \n", mb->block[0].nsym,
                mb->block[1].nsym, mb->block[2].nsym, mb->block[3].nsym,
                mb->block[4].nsym, mb->block[5].nsym);
        printf("Symbol number to print: ");
        scanf("%d", &isym);
        while (isym > 0) {
            printf("Luma 1: ");
            printsym( *(mb->block[0].sym + isym - 1) ); printf("\n");
            printf("Luma 2: ");
            printsym( *(mb->block[1].sym + isym - 1) ); printf("\n");
            printf("Luma 3: ");
            printsym( *(mb->block[2].sym + isym - 1) ); printf("\n");
            printf("Luma 4: ");
            printsym( *(mb->block[3].sym + isym - 1) ); printf("\n");
            printf("Symbol number to print: ");
            scanf("%d", &isym);
    }
    }
 */
    pixel0 = pic->y.ptr + col + row * pic->y.hoffset;

//    #define DB_DUMP_MACROBLOCK
#ifdef DB_DUMP_MACROBLOCK
    {
                static char foo[256];
                static char dump=0;
                int dbi,dbj;
                if(dump) {
                sprintf(foo, "Reference Block (from ReconInter) (%d, %d)\n", mb->x, mb->y);
                OutputDebugString(foo);
                for(dbi=0;dbi<16;dbi++) 
                {
                    foo[0] = '\0';
                    for(dbj=0;dbj<16;dbj++)
                    {
                        sprintf(foo+strlen(foo), "%d ", pixel0[dbi*pic->y.hoffset + dbj]);
                    }
                    sprintf(foo+strlen(foo), "\n");
                    OutputDebugString(foo);
                }
                }
    }
#endif

    if (mb->block[0].nsym > 0) {
        //printf("Luma block 1 \n");
        Idct2Sum( mb->block[0].sym, mb->block[0].nsym, pixel0,
            pic->y.hoffset, recon_tab, clean);
    }
    if (mb->block[1].nsym > 0) {
        //printf("Luma block 2 \n");
        Idct2Sum( mb->block[1].sym, mb->block[1].nsym, pixel0 + 8,
            pic->y.hoffset, recon_tab, clean);
    }
    if (mb->block[2].nsym > 0) {
        //printf("Luma block 3 \n");
        Idct2Sum( mb->block[2].sym, mb->block[2].nsym, pixel0 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab, clean);
    }
    if (mb->block[3].nsym > 0) {
        //printf("Luma block 4 \n");
        Idct2Sum( mb->block[3].sym, mb->block[3].nsym, pixel0 + 8 + 8 * pic->y.hoffset,
            pic->y.hoffset, recon_tab, clean);
    }
    if (pic->color  &&  (mb->block[4].nsym > 0  ||  mb->block[5].nsym > 0)) {
        // Assuming same offset for Cr and Cb
        col = 8 * mb->x;
        row = 8 * mb->y;
        offset = col + row * pic->cb.hoffset;
        if (mb->block[4].nsym > 0) {
            pixel0 = pic->cb.ptr + offset;
            //printf("CB block \n");
            Idct2Sum( mb->block[4].sym, mb->block[4].nsym, pixel0,
                        pic->cb.hoffset, recon_tab, clean);
        }
        if (mb->block[5].nsym > 0) {
            pixel0 = pic->cr.ptr + offset;
            //printf("CR block \n");
            Idct2Sum( mb->block[5].sym, mb->block[5].nsym, pixel0,
                        pic->cr.hoffset, recon_tab, clean);
        }
    }
//#define DB_DUMP_MACROBLOCK
#ifdef DB_DUMP_MACROBLOCK
    {
                static char foo[256];
                static char dump=0;
                int dbi,dbj;
                if(dump) {
                pixel0 = pic->y.ptr + col + row * pic->y.hoffset;
                sprintf(foo, "Reconstructed Macroblock (%d, %d)\n", mb->x, mb->y);
                OutputDebugString(foo);
                for(dbi=0;dbi<16;dbi++) 
                {
                    foo[0] = '\0';
                    for(dbj=0;dbj<16;dbj++)
                    {
                        sprintf(foo+strlen(foo), "%d ", pixel0[dbi*pic->y.hoffset + dbj]);
                    }
                    sprintf(foo+strlen(foo), "\n");
                    OutputDebugString(foo);
                }
                }
    }
#endif
    return;
}


//  reconBframe - Reconstruct B-frame prediction error and add to prediction
static void reconBframe( MACROBLOCK_DESCR * mb, PICTURE * Bpic )
{
    int         i;
    U8          saveQuant, saveCbp;
    SYMBOL P32  saveSym[6];
    int         saveNsym[6];

    if (BFRAME_HAS_CBP(mb)) {
        // Set quant, cbp, and block[] to hold values for B-frame
        saveQuant = mb->quant;
        mb->quant = mb->Bquant;
        saveCbp = mb->cbp;
        mb->cbp = mb->cbpB;
        for (i = 0; i < 6; ++i) {
            saveSym[i] = mb->block[i].sym;
            mb->block[i].sym = mb->Bblock[i].sym;
            saveNsym[i] = mb->block[i].nsym;
            mb->block[i].nsym = mb->Bblock[i].nsym;
        }
        // Do reconstruction
        ReconInter( mb, Bpic, CLEAN );
        // Restore parameters (if needed for statistics)
        mb->quant = saveQuant;
        mb->cbp = saveCbp;
        for (i = 0; i < 6; ++i) {
            mb->block[i].sym = saveSym[i];
            mb->block[i].nsym = saveNsym[i];
        }
    }
}


//  gray_mb - fill macroblock with gray (value 128); assumes that pic is word-aligned
static void gray_mb( MACROBLOCK_DESCR * mb, PICTURE * pic )
{
    fill_mb( mb, pic, GRAY );
}


//  fill_mb - fill macroblock with constant color; assumes that pic is word-aligned
static void fill_mb( MACROBLOCK_DESCR * mb, PICTURE * pic, PIXEL value )
{
    int     row, col, i;
    union {     // Write words to speed up routine
        PIXEL   * pix;
        U32     * word;
    } pixel;
    U32     * dest;
    U32     dValue;

    dValue = value | (value << 8) | (value << 16) | (value << 24);
    col = 16 * mb->x;
    row = 16 * mb->y;
#ifdef DO_H263_PLUS
    if (col >= pic->y.nhor  ||  row >= pic->y.nvert)
        return; // Inactive part of Macroblock
#endif
    pixel.pix = pic->y.ptr + col + row * pic->y.hoffset;
    for (i = 0; i < 16; i++) {
        for (dest = pixel.word; dest < pixel.word + 16/4; dest++) {
            *dest = dValue;
        }
        pixel.pix += pic->y.hoffset;
    }
    if (pic->color) {
        col = 8 * mb->x;
        row = 8 * mb->y;
        pixel.pix = pic->cb.ptr + col + row * pic->cb.hoffset;
        for (i = 0; i < 8; i++) {
            for (dest = pixel.word; dest < pixel.word + 8/4; dest++) {
                *dest = dValue;
            }
            pixel.pix += pic->cb.hoffset;
        }
        pixel.pix = pic->cr.ptr + col + row * pic->cr.hoffset;
        for (i = 0; i < 8; i++) {
            for (dest = pixel.word; dest < pixel.word + 8/4; dest++) {
                *dest = dValue;
            }
            pixel.pix += pic->cr.hoffset;
        }
    }
    return;
}



#ifdef DO_H263_PLUS

/////////////////////////////////////////////////////////////////////////////////////////
//////////// Functions for reconstruction in Reduced-resolution Update mode /////////////
/////////////////////////////////////////////////////////////////////////////////////////


// ReducedResMvComponent - Translate Reduced-res. motion vector component to representation
//  with one fractional bit. The input value is "rounded" half a pixel towards zero, e.g.,
//  input values:   -2   -1   0    1    2  are
//  translated to: -1.5 -0.5  0   0.5  1.5,
extern S8 ReducedResMvComponent( S8 x )
{
    if (x > 0) {
        x = 2 * x - 1;
    } else if (x < 0) {
        x = 2 * x + 1;
    }
    return x;
}


// MotionComp32x32 - perform motion compensation for Reduced-res. Update mode
extern void MotionComp32x32( MACROBLOCK_DESCR * mb, // Describes block to be motion-compensated
                            PICTURE * prevPic,  // Describes previous picture used to form MC
                            PICTURE * pic       // Output picture where MC block is placed
                            )
{
    int blk;
    // Save MTYPE and x/y position
    int saveType = mb->mtype;
    int saveX = mb->x;
    int saveY = mb->y;
    int saveMvX = mb->mv_x;
    int saveMvY = mb->mv_y;

    mb->mtype = MTYPE263_INTER;
    if (saveType != MTYPE263_INTER4V) {
        // Use same motion vector for all four quadrants
        for (blk = 0; blk < 4; ++blk) {
            mb->blkMvX[blk] = mb->mv_x;
            mb->blkMvY[blk] = mb->mv_y;
        }
    }

    // Upper Left 16x16
    mb->x = 2 * saveX;
    mb->y = 2 * saveY;
    mb->mv_x = ReducedResMvComponent( mb->blkMvX[UPPER_LEFT_BLK] );
    mb->mv_y = ReducedResMvComponent( mb->blkMvY[UPPER_LEFT_BLK] );
    MotionComp263( mb, prevPic, pic);
    // Upper Right 16x16
    mb->x = 2 * saveX + 1;
    if (16 * mb->x < pic->y.nhor) {
        mb->mv_x = ReducedResMvComponent( mb->blkMvX[UPPER_RIGHT_BLK] );
        mb->mv_y = ReducedResMvComponent( mb->blkMvY[UPPER_RIGHT_BLK] );
        MotionComp263( mb, prevPic, pic);
    }

    mb->y = 2 * saveY + 1;
    if (16 * mb->y < pic->y.nvert) {
        // Lower Left 16x16
        mb->x = 2 * saveX;
        mb->mv_x = ReducedResMvComponent( mb->blkMvX[LOWER_LEFT_BLK] );
        mb->mv_y = ReducedResMvComponent( mb->blkMvY[LOWER_LEFT_BLK] );
        MotionComp263( mb, prevPic, pic);
        // Lower Right 16x16
        mb->x = 2 * saveX + 1;
        if (16 * mb->x < pic->y.nhor) {
            mb->mv_x = ReducedResMvComponent( mb->blkMvX[LOWER_RIGHT_BLK] );
            mb->mv_y = ReducedResMvComponent( mb->blkMvY[LOWER_RIGHT_BLK] );
            MotionComp263( mb, prevPic, pic);
        }
    }

    // Restore MTYPE, x/y position, and motion vector
    mb->mtype = saveType;
    mb->x = saveX;
    mb->y = saveY;
    mb->mv_x = saveMvX;
    mb->mv_y = saveMvY;
}


// Overlap32x32 - Do overlapped motion comp. for luma (Reduced-res. Update mode)
extern void Overlap32x32( MACROBLOCK_DESCR * mb,   // Describes block to be motion-compensated
                        PICTURE * prevPic,  // Describes previous picture used to form MC
                        PICTURE * pic,      // Output picture where MC block is placed
                        int     mbWidth,    // Macroblocks per row
                        int     mbOffset,   // Row offset; (mb-mbOffset) is neighbor on top
                        int     overlap[4]  // Returns YES or NO to indicate whether overlap
                                            // was done in each 8x8 subblock
                        )
{
    // Placeholder -- not yet implemented
    overlap[UPPER_LEFT_BLK] = NO;
    overlap[UPPER_RIGHT_BLK] = NO;
    overlap[LOWER_LEFT_BLK] = NO;
    overlap[LOWER_RIGHT_BLK] = NO;
}


// Fill32x32 - Fill Reduced-res. MB (32x32 block) with constant color
extern void Fill32x32( MACROBLOCK_DESCR * mb, PICTURE * pic, PIXEL value )
{
    // Get MB coordinates
    int saveX = mb->x;
    int saveY = mb->y;

    // Fill Upper Left 16x16
    mb->x = 2 * saveX;
    mb->y = 2 * saveY;
    fill_mb( mb, pic, value);
    // Fill Upper Right 16x16
    mb->x = 2 * saveX + 1;
    if (16 * mb->x < pic->y.nhor) {
        fill_mb( mb, pic, value);
    }

    mb->y = 2 * saveY + 1;
    if (16 * mb->y < pic->y.nvert) {
        // Fill Lower Left 16x16
        mb->x = 2 * saveX;
        fill_mb( mb, pic, value);
        // Fill Lower Right 16x16
        mb->x = 2 * saveX + 1;
        if (16 * mb->x < pic->y.nhor) {
            fill_mb( mb, pic, value);
        }
    }
    // Restore MB coordinates
    mb->x = saveX;
    mb->y = saveY;
}


// ReconReducedResMb - Reconstruct macroblock in Reduced-resolution Update mode
extern void ReconReducedResMb( MACROBLOCK_DESCR * mb,   // Macroblock to be reconstructed
                               PICTURE * pic,   // Input: motioncomp. prediction;
                                                // output: reconstr. picture
                               int intra,       // INTER block if zero, otherwise INTRA
                               PICTURE * tempPic// Use for temporary storage
                               )
{
    // Perform IDCT of prediction error; use "first half" of tempPic for temporary storage
    idct32x32( mb, tempPic, intra );
    // Interpolate prediction error, add to motioncomp. prediction and clip to [0,255]
    filtAddClip32x32( mb, pic, tempPic );
}


// idct32x32 - Perform IDCT for 32x32 macroblock (Reduced-res. update mode)
static void idct32x32( MACROBLOCK_DESCR * mb,   // Macroblock to be reconstructed
                       PICTURE * tempPic,       // Store 16-bit IDCT values here
                       int intra                // INTER block if zero, otherwise INTRA
                       )
{
    int     row, col, yHoffset, cHoffset, chromaPixels;
    S16     * pIdct;
    S16     * recon_tab;

    recon_tab = Recon[mb->quant - QUANT_MIN];

    // Reconstruct luminance
    // The PIXEL y[V][H] array is used as S16[V/2][H/2], i.e., only upper half is used
    col = 16 * mb->x;
    row = 16 * mb->y;
    yHoffset = tempPic->y.hoffset >> 1;
    pIdct = (S16 *)tempPic->y.ptr;
    pIdct += col + row * yHoffset;
    Idct2_s16( intra, mb->block[0].sym, mb->block[0].nsym, 
               pIdct + 0 + 0 * yHoffset,
               yHoffset, recon_tab );
    if (2 * col + 16 < tempPic->y.nhor) {
        Idct2_s16( intra, mb->block[1].sym, mb->block[1].nsym, 
                   pIdct + 8 + 0 * yHoffset,
                   yHoffset, recon_tab );
    }
    if (2 * row + 16  <  tempPic->y.nvert) {
        Idct2_s16( intra, mb->block[2].sym, mb->block[2].nsym, 
                   pIdct + 0 + 8 * yHoffset,
                   yHoffset, recon_tab );
        if (2 * col + 16 < tempPic->y.nhor) {
            Idct2_s16( intra, mb->block[3].sym, mb->block[3].nsym, 
                       pIdct + 8 + 8 * yHoffset,
                       yHoffset, recon_tab );
        }
    }

    // Reconstruct chrominance
    //  Ensure that we have memory for picture sizes that are
    //  not multiples of 32, i.e., odd number of macroblocks.  In that case, we will
    //  throw away 4 chroma pixels on the right and/or bottom after the IDCT.
    //  This routine assumes that the two VxH chroma arrays can be treated as one block of
    //  memory starting at tempPic->cb.ptr and of size (V+8)/16 * (H+8)/16 * 256 bytes.
    //  The current memory allocation done in initializePicture fulfills this as long
    //  as each chroma array is at least 16x16 (2x2 macroblocks).
    if (tempPic->color) {
        col = 8 * mb->x;
        row = 8 * mb->y;
        chromaPixels = 8 * ((tempPic->cb.nhor + 8) >> 4);
        cHoffset =  2 * chromaPixels;
        pIdct = (S16 *)tempPic->cb.ptr;
        pIdct += col + row * cHoffset;
        //printf("CB block \n");
        Idct2_s16( intra, mb->block[4].sym, mb->block[4].nsym, pIdct,
                        cHoffset, recon_tab );
        // CR array is placed "to the right" of CB array
        //printf("CR block \n");
        Idct2_s16( intra, mb->block[5].sym, mb->block[5].nsym, pIdct + chromaPixels,
                        cHoffset, recon_tab );
    }
}


// filtAddClip32x32 - Interpolate, add & clip 32x32 macroblock (Reduced-res. update mode)
static void filtAddClip32x32( MACROBLOCK_DESCR * mb,// Macroblock to be reconstructed
                              PICTURE * pic,    // Input: motion-comp prediction w/ filtered
                                                // intra borders; output: reconstr. picture
                              PICTURE * tempPic // 16-bit IDCT values
                              )
{
    int     row, col, offset, yHoffset, cHoffset, chromaPixels;
    int     hSize, vSize;
    PIXEL   * pixel0;
    S16     * pIdct;

    yHoffset = pic->y.hoffset >> 1;
    col = 16 * mb->x;
    row = 16 * mb->y;
    hSize = vSize = 16;
    if (2 * col + 16 >= pic->y.nhor)
        hSize = 8;
    if (2 * row + 16 >= pic->y.nvert)
        vSize = 8;

    // Interpolate, add, and clip
    pixel0 = pic->y.ptr + 2 * col + 2 * row * pic->y.hoffset;
    pIdct = (S16 *)tempPic->y.ptr;
    pIdct += col + row * yHoffset;
    filtAddClip( pixel0, pic->y.hoffset, pIdct, yHoffset, 8, 8 );
    if (hSize == 16)
        filtAddClip( pixel0 + 16, pic->y.hoffset, pIdct + 8, yHoffset, 8, 8 );
    if (vSize == 16) {
        pixel0 += 16 * pic->y.hoffset;
        pIdct += 8 * yHoffset;
        filtAddClip( pixel0, pic->y.hoffset, pIdct, yHoffset, 8, 8 );
        if (hSize == 16)
            filtAddClip( pixel0 + 16, pic->y.hoffset, pIdct + 8, yHoffset, 8, 8 );
    }
    if (pic->color) {
        chromaPixels = 8 * ((pic->cb.nhor + 8) >> 4);
        cHoffset =  2 * chromaPixels;
        offset = col + row * pic->cb.hoffset;
        pixel0 = pic->cb.ptr + offset;
        pIdct = (S16 *)tempPic->cb.ptr;
        pIdct += (col >> 1) + (row >> 1) * cHoffset;
        filtAddClip( pixel0, pic->cb.hoffset, pIdct,
                    cHoffset, hSize>>1, vSize>>1 );
        pixel0 = pic->cr.ptr + offset;
        filtAddClip( pixel0, pic->cr.hoffset, pIdct + chromaPixels,
                    cHoffset, hSize>>1, vSize>>1 );
    }
}


#define PIXEL_MIN       0
#define PIXEL_MAX       255
#define CLIPMARGIN      300
#define CLIPMIN         (PIXEL_MIN - CLIPMARGIN)
#define CLIPMAX         (PIXEL_MAX + CLIPMARGIN)
extern PIXEL   clip[(CLIPMAX-CLIPMIN+1)];

// filtAddClip - interpolate IDCT output (typically 8x8), add to prediction (typ. 16x16),
//  and clip.  Interpolation is only done within the block.
static void filtAddClip( PIXEL x[], int xdim,       // Output pixels
                         S16 idct_out[], int idim,  // Input IDCT values
                         int hSize, int vSize       // Input block size for IDCT values
                        )
{
    int     i, j, e;

    // Handle top border
    e = idct_out[0];
    x[0] = clip[ -CLIPMIN + e + x[0] ];
    for (j = 1; j < hSize; ++j) {
        e = (3 * idct_out[j-1] + 1 * idct_out[j] + 2) >> 2;
        x[2*j-1] = clip[ -CLIPMIN + e + x[2*j-1] ];
        e = (1 * idct_out[j-1] + 3 * idct_out[j] + 2) >> 2;
        x[2*j]   = clip[ -CLIPMIN + e + x[2*j] ];
    }
    e = idct_out[hSize-1];
    x[2*hSize-1] = clip[ -CLIPMIN + e + x[2*hSize-1] ];
    x += 2 * xdim;
    idct_out += idim;

    // Process 2*vSize-2 rows
    for (i = 1; i < vSize; ++i) {
        e = (3 * idct_out[-idim] + 1 * idct_out[0] + 2) >> 2;
        x[-xdim] = clip[ -CLIPMIN + e + x[-xdim] ];
        e = (1 * idct_out[-idim] + 3 * idct_out[0] + 2) >> 2;
        x[0]     = clip[ -CLIPMIN + e + x[0] ];
        for (j = 1; j < hSize; ++j) {
            e = (9 * idct_out[j-1-idim] + 3 * idct_out[j-idim]
               + 3 * idct_out[j-1]      + 1 * idct_out[j] + 8) >> 4;
            x[2*j-1 - xdim] = clip[ -CLIPMIN + e + x[2*j-1 - xdim] ];
            e = (3 * idct_out[j-1-idim] + 9 * idct_out[j-idim]
               + 1 * idct_out[j-1]      + 3 * idct_out[j] + 8) >> 4;
            x[2*j - xdim]   = clip[ -CLIPMIN + e + x[2*j - xdim] ];
            e = (3 * idct_out[j-1-idim] + 1 * idct_out[j-idim]
               + 9 * idct_out[j-1]      + 3 * idct_out[j] + 8) >> 4;
            x[2*j-1]        = clip[ -CLIPMIN + e + x[2*j-1] ];
            e = (1 * idct_out[j-1-idim] + 3 * idct_out[j-idim]
               + 3 * idct_out[j-1]      + 9 * idct_out[j] + 8) >> 4;
            x[2*j]          = clip[ -CLIPMIN + e + x[2*j] ];
        }
        e = (3 * idct_out[hSize-1-idim] + 1 * idct_out[hSize-1] + 2) >> 2;
        x[2*hSize-1-xdim] = clip[ -CLIPMIN + e + x[2*hSize-1-xdim] ];
        e = (1 * idct_out[hSize-1-idim] + 3 * idct_out[hSize-1] + 2) >> 2;
        x[2*hSize-1]      = clip[ -CLIPMIN + e + x[2*hSize-1] ];
        x += 2 * xdim;
        idct_out += idim;
    }
    // Handle bottom border
    x -= xdim;
    idct_out -= idim;
    e = idct_out[0];
    x[0] = clip[ -CLIPMIN + e + x[0] ];
    for (j = 1; j < hSize; ++j) {
        e = (3 * idct_out[j-1] + 1 * idct_out[j] + 2) >> 2;
        x[2*j-1] = clip[ -CLIPMIN + e + x[2*j-1] ];
        e = (1 * idct_out[j-1] + 3 * idct_out[j] + 2) >> 2;
        x[2*j]   = clip[ -CLIPMIN + e + x[2*j] ];
    }
    e = idct_out[hSize-1];
    x[2*hSize-1] = clip[ -CLIPMIN + e + x[2*hSize-1] ];
}

#endif
