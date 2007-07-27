/*-----------------------------------------------------------------------------
 *	H263PLUS.C
 *
 *	DESCRIPTION
 *		h263plus.c - main header for H.263+ extensions
 *
 *      Author:     David Blair    		03/04/97
 *      Inspector:  
 *	(c) 1997-98, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
#ifndef _H263PLUS_H_
#define _H263PLUS_H_

#define DO_H263_PLUS

#ifdef DO_H263_PLUS
extern int ireorder[8];
extern int inv_alt_hor_scan[64];
extern int inv_alt_hor_scan_no_reorder[64];
extern int alt_hor_to_zigzag[64];
extern int inv_alt_ver_scan[64];
extern int inv_alt_ver_scan_no_reorder[64];
extern int alt_ver_to_zigzag[64];
extern int zigzag_to_zigzag[64];

// some predicates for B frames
#define BFRAME_IS_BIDIRECTIONAL(x)  ((x)->modB == 0 || (x)->modB == 3)
#define BFRAME_IS_FORWARD(x) ((x)->modB == 1 || (x)->modB == 2)
#define BFRAME_HAS_CBP(x) ((x)->modB >= 2)
#define BFRAME_HAS_BLOCKS(x) BFRAME_HAS_CBP(x)
#define BFRAME_HAS_MOTION_VECTOR(x) ((x)->modB == 1 || (x)->modB == 2)

#define H263PLUS_IMPROVED_PBFRAME_MODE (2)
#define H263_ORIGINAL_PBFRAME_MODE (1)

// Bitflags for the EPTYPE codeword
#define EPTYPE263PLUS_ADVANCED_INTRA_MODE    (0x400)
#define EPTYPE263PLUS_DEBLOCKING_FILTER_MODE (0x200)
#define EPTYPE263PLUS_IMPROVED_PBFRAME_MODE (0x10)
#define EPTYPE263PLUS_REDUCED_RES_UPDATE    (0x4)

#define H263PLUS_EXTENSIONS_USED(d) ((d)->PBframeMode==H263PLUS_IMPROVED_PBFRAME_MODE || \
   (d)->deblockingFilterMode || (d)->advancedIntraMode || (d)->reducedResUpdate)

#define ROUNDDIV2(n)    (((n) + 1) >> 1)    /* Divide by 2 and round up */


#define ADV_INTRA_PRED_DC_ONLY (0)
#define ADV_INTRA_PRED_COLUMN (1)
#define ADV_INTRA_PRED_ROW (2)
#define ADV_INTRA_PRED_NONE (3)

void ApplyDeblockingFilter( PICTURE * pic, MACROBLOCK_DESCR * mb, S32 Bframe);
extern void ReducedResDeblockingFilter( PICTURE * pic, MACROBLOCK_DESCR * mb );
void Idct2AdvancedIntra( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[],
                U8 rDCpred, S8 rACpred[8], U8 rDCstore[1], S8 rACstore[8], 
                U8 cDCpred, S8 cACpred[8], U8 cDCstore[1], S8 cACstore[8],
                int predtype, int fixedDC, int leftBoundary, int upperBoundary);
void ReconAdvancedIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean);
U8 pred_select_advanced_intra( S16 y[4][64], S8 rACpred0[8], S8 rACpred1[8],S8 cACpred0[8], 
                          S8 cACpred2[8],pQMatrix qtab);
U8 pred_select_advanced_intra_full_search( S16 y[6][64], 
                          U8 rDCpred0, S8 rACpred0[8], U8 rDCpred1, S8 rACpred1[8],
                          U8 rDCpredCb, S8 rACpredCb[8], U8 rDCpredCr, S8 rACpredCr[8],
                          U8 cDCpred0, S8 cACpred0[8], U8 cDCpred2, S8 cACpred2[8], 
                          U8 cDCpredCb, S8 cACpredCb[8], U8 cDCpredCr, S8 cACpredCr[8],
                          pQMatrix qtab, int ncoeffs, int fixedDC,
                          int leftBoundary, int upperBoundary);
int quant_advanced_intra( S16 y[64], 
                          U8 rDCpred, S8 rACpred[8], U8 rDCstore[1], S8 rACstore[8], 
                          U8 cDCpred, S8 cACpred[8], U8 cDCstore[1], S8 cACstore[8],
                          pQMatrix qtab, int ncoeffs, SYMBOL sym[], int predtype, int fixedDC,
                          int leftBoundary, int upperBoundary);
int AdvancedIntraCode( PICTURE *pic, MACROBLOCK_DESCR * mb, SYMBOL sym[], long mbDiff,
                      int codingMethod );
void InitAdvancedIntraTables();

#else
// These are for original H.263 B frames
#define BFRAME_IS_BIDIRECTIONAL(x)  (TRUE)
#define BFRAME_IS_FORWARD(x) (FALSE)
#define BFRAME_HAS_CBP(x) ((x)->modB >= 2)
#define BFRAME_HAS_BLOCKS(x) BFRAME_HAS_CBP(x)
#define BFRAME_HAS_MOTION_VECTOR(x) ((x)->modB != 0)
#endif

#endif
