/*-----------------------------------------------------------------------------
 *  H261FUNC.H
 *
 *  DESCRIPTION   
 *      Function prototypes for H.261 codec.
 *
 *  Author:     Staffan Ericsson    6/30/93
 *  Inspector:  Mary Deshon         7/6/93
 *  Revised:
 *  01/04/98 S Ericsson RTP support: added H263RtpDecode, H263ConcealRest, InitializePicture,
 *                      and Build263Gob. Modified DecMbLayer263.
 *  02/06/97 S Ericsson Support Reduced-res. Update mode
 *  01/13/97 S Ericsson Added AllocVarStructsDecoder and FreeVarStructsDecoder
 *  11/24/96 S Ericsson Added PredBdist
 *  11/03/96 S Ericsson Support PB-frames: added PredBframe etc.
 *  09/18/96 S Ericsson Added roiRect to DIBToVideo prototype
 *	06/24/96 S Ericsson	Support Unrestricted Motion Vector mode (UMV)
 *  06/20/96 S Ericsson QuantSelect: pass "cntrl" using pointer
 *  06/02/96 S Ericsson Modified DecMbLayer263 for UMV mode; added TryInter4V
 *  04/22/96 S Ericsson Added ReconStep[]
 *  04/11/96 S Ericsson Modified OverlapMC and PredSelect
 *  11/13/95 md         Modifications to support GFID
 *  09/06/95 J Bruder   Added PreProcessYVU9 ()
 *  07/25/95 J Bruder   Merged Brazil changes for BtV.
 *  08/22/95 M Deshon   Add codingMethod to IntraCode() prototype.
 *  07/22/95 S Ericsson Added file MCOMP.C, RefineMotionVector, and err16.
 *                      Support Advanced Prediction mode.
 *  07/08/95 S Ericsson Modifications for H.263
 *  02/17/95 S Ericsson Added protos for Dct4x4 and Dct4x4Diff
 *  12/13/94 B Girod    Added clean parameter to Idct2, Idct2Sum, ReconIntra,
 *                      ReconInter prototypes
 *  11/10/94 M Deshon   Modify InitGammaCorrection() prototype.
 *  09/21/94 M Deshon   Add histogram parameters ReshufMovMan().
 *  09/11/94 M Deshon   Add rotate and gamma to TempFilter call.
 *  09/01/94 M Deshon   Added parameter to DIBToVideo.
 *  08/07/94 S Ericsson Added InitGammaCorrection and ReshufMovMan
 *  06/06/94 M Deshon   Modified calling sequence to PredSelect.
 *  05/30/94 S Ericsson Modified calling sequence to MotionEst
 *  05/22/94 M Deshon   Changed DIBToVideo prototype
 *  05/03/94 S Ericsson Changed protos for QuantSelect and PredSelect
 *                      Made VLCode "static", i.e., removed proto
 *  04/19/94 S Ericsson Use pointers when calling TempFilter
 *  04/07/94 M Deshon   Changed proto for H261Encode(); added S32 DCTStats[].
 *  03/20/94 M Deshon   Changed proto for H261Encode().
 *  02/20/94 M Deshon   Changed proto for IntraCode.
 *  01/24/94 M Deshon   Moved some prototypes from vvenc.h into this file.
 *  12/10/93 S Ericsson Changed call to MotionEst()
 *  11/16/93 J Bruder   Added getImgParms() prototype
 *  11/01/93 S Ericsson Pass pointers to structures in function calls
 *  09/30/93 M Deshon   Add prototypes for BCH
 *  08/07/93 M Deshon   Added protos for quant2.c
 *  07/26/93 S Ericsson Modified call to PredSelect
 *  07/22/93 S Ericsson Added "predsel.c"
 *  07/20/93 S Ericsson Added "h261err.c", EncStuffing, and sprintsym
 *  07/16/93 S Ericsson New functions for H261Encode
 *  07/06/93 M Deshon   Minor style changes
 *
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _INC_H261FUNC
#define _INC_H261FUNC   1 

//  dct.c
extern void Dct2( PIXEL x[], int xdim, S16 y[8][8] );
extern void Dct2Diff( PIXEL x[], int xdim, PIXEL pred[], S16 y[64] );
extern void InitDctTables ( void );
extern void Dct8x8ssDiff( PIXEL x[], int xdim, PIXEL pred[], S16 y[8*8],
                          int hSize,    // 16: normal DCT; otherwise: mirror input horizontally
                          int vSize     // 16: normal DCT; otherwise: mirror input vertically
                          );

//  dct4x4.c
extern void Dct4x4( PIXEL x[], int xdim, S16 y[4][4]);
extern void Dct4x4Diff( PIXEL x[], int xdim, PIXEL pred[], S16 y[4][4]);

//  h261dec.c
extern int  H261Decode( H261Decoder * s );
extern int Build263gob( S32 format, S32 gn, int next_gn, GOB_DESCR * gob,
                        PICTURE_DESCR * pic );
extern int  InitializePicture( H261Decoder * s );
extern int  BsDiff( BS_PTR bs1, BS_PTR bs2 );   // Compute # bits in segment bs1 - bs2
extern void InitFindSC( void );
extern int  checksym( SYMBOL sym, int type, char routine[] );

//  h261enc.c
extern int H261Encode( H261Encoder * s, H261PicDescriptor d, TIME32 time0[], S32 DCTStats[] );
extern int H261Recon( H261Encoder * s, H261PicDescriptor d );

//  h261err.c
extern void H261ErrMsg( char s[] );

//  h263dec.c
extern int  DecPicLayer263( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            GOB_DESCR * gob, int * decPtype );
extern void InitMvTabs( int trD, int trB,
                        int tabMvF[], int tabMvB[]  // [UMV_MIN:UMV_MAX]
                        );
extern int  DecGobLayer263( BS_PTR * bs, int nbits, GOB_DESCR * gob, int * gfid );
extern int  DecMbLayer263(  BS_PTR * bs,    // Bitstream pointer
                            int nbits,      // Bits to decode (incl. trailing startcode)
                            GOB_DESCR * gob,        // GOB descriptor
                            MACROBLOCK_DESCR mb[],  // Packed array of "gob->num_mb" MB descr.
                            int interFrame, // 0: ptype=INTRA, otherwise ptype=INTER
                            int PBframe,    // 0: not PB frame, otherwise PB frame
                            int unrestrictedMv, // 0: -16/+15.5 motion, otherwise +/- 31.5
                            int advancedIntraMode, // 0: off else on
                            SYMBOL sym[],   // symbol array
                            int maxsym,     // size of symbol array
                            int numHorPred, // Use hor pred for first numHorPred MBs
                            int rtpFlag,    // If set (nonzero), decode MBs until we reach
                                            // end of bitstream or have completed the GOB.
                            int *nextMba    // If rtpFlag is set, return MB address where
                                            // decoding should be resumed. Returns zero if
                                            // GOB is completed.  Undefined if rtpFlag=0
                            );
extern void GetMvBframe( MACROBLOCK_DESCR *mb, int unrestrictedMv,
                         int tabMvF[], int tabMvB[]  // [UMV_MIN:UMV_MAX]
                         );
extern int  H263RtpDecode( H261Decoder * s, // Start decoding at s->next_gob/next_mba
                           char * hdr,  // H.263 payload header (defined in RFC2190, Sept'97)
                                        // or H.263-1998 header (defined in RFC2429, Oct'98)
                           int h263_1998	// 0 = RFC2190, 1 = RFC2429
                           );
extern int  H263ConcealRest( H261Decoder * s    // Start concealment at s->next_gob/next_mba
                           );

//  idct.c
extern void InitReconTables( void );
extern void Idct2( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[], int clean );
extern void Idct2Sum( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[], int clean );
extern void Idct2_s16( int intra, SYMBOL sym[], int nsym, S16 x[], int xdim, S16 recon[] );

//  intercod.c
extern U8  ReconStep[];
extern int InterCode( PICTURE *pic, PICTURE *pred, MACROBLOCK_DESCR * mb, int masking,
                        SYMBOL sym[] );
extern int IntraCode( PICTURE *pic, MACROBLOCK_DESCR * mb, SYMBOL sym[], long mbDiff, int codingMethod );
extern void InitQuantTables( void );
extern int Code32x32( PICTURE *pic, PICTURE *pred, MACROBLOCK_DESCR * mb, int masking,
                        SYMBOL sym[], int intraFlag );


//  predsel.c
extern void InitPred( void );
extern int PredSelect( H261PicDescriptor *d, PICTURE *oldOut, PICTURE *newIn,
                       int mbhor, int mbvert, MACROBLOCK_DESCR mb[], GOB_DESCR gob[],
                       PICTURE *pred, int dct_thresh, H261Stats *stats,
                       S32 weight_1, S32 weight_2, S32 sendGobHeaders );
extern int var16( PIXEL pic[], int xdim, int phase, long var[4] );
extern int err16( PIXEL new[], PIXEL old[], int xdim, int phase, int err[4] );
extern int  LoopFilter( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic );

// mcomp.c
extern int  MotionComp( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic );
extern void MotionComp263( MACROBLOCK_DESCR * mb, // Describes block to be motion-compensated
                            PICTURE * prevPic,  // Describes previous picture used to form MC
                            PICTURE * pic       // Output picture where MC block is placed
                            );
extern void OverlapMC( MACROBLOCK_DESCR * mb,   // Describes block to be motion-compensated
                        int     PBframe,    // Non-zero if PB frame
                        PICTURE * prevPic,  // Describes previous picture used to form MC
                        PICTURE * pic,      // Output picture where MC block is placed
                        int     mbWidth,    // Macroblocks per row
                        int     mbOffset,   // Row offset; (mb-mbOffset) is neighbor on top
                        int     overlap[4]  // Returns YES or NO to indicate whether overlap
                                            // was done in each 8x8 subblock
                        );
extern int PointingOutside( int col1, int col2, // First and last column of block
                            int row1, int row2, // First and last row of block
                            int mvX, int mvY,   // Motion vector; one fractional bit
                            int nCols, int nRows    // Picture size
                            );
extern void PredBframe( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );
extern void PredBdist( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );

//  mest.c
extern int  MotionEst( PICTURE *new_in, PICTURE *old_in, int mbhor, int mbvert,
					   MACROBLOCK_DESCR Mb[], int unrestrictedMv, int advancedPred );
extern int  RefineMotionVector( MACROBLOCK_DESCR *mb,   // MB descriptor; contains initial MV
                                PIXEL new[],    // Upper left pixel of new 16x16 block
                                PIXEL old[],    // Old block (no motion comp)
                                int xdim,       // Line offset for "new" and "old"
                                int phase,      // Phase used for computing "mc_err"
                                int mc_err,     // Error for initial motion vector
                                int err[4],     // Error for each 8x8 block; submit values for
                                                // initial mv; returns values for chosen mv
                                int horPredOnly );   // 1-D prediction for mv?
extern int  TryInter4V( MACROBLOCK_DESCR mb[],  // MB descriptor array; mb[0] is current mb;
                                                // use surrounding mb's to get mv candidates
                                                // Note: "past" mv's have a fractional bit
                        PIXEL new[],    // Upper left pixel of new 16x16 block
                        PIXEL old[],    // Old block (no motion comp)
                        int xdim,       // Line offset for "new" and "old"
                        int phase,      // Phase used for computing "mc_err"
                        int mc_err,     // Error for initial motion vector
                        int errMc[4],   // Error for each 8x8 block with INTER mv (undefined
                                        //  if mv=0); returns values for chosen mv/mv's
                        int err[4],     // Error for each 8x8 block with mv=0
                                        //  We are assuming that sum(err) > sum(errMc)
                        int sqQuant     // QUANT^2; needed for INTER4V decision
                        );
extern int UmvErr( PIXEL new[], PIXEL old[], int xdim, int phase, int errvec[4],
                   int size, int col, int row, int mvX, int mvY );
extern int RefineMvReducedResMode( MACROBLOCK_DESCR *mb,// MB descriptor; contains initial MV
                                   int blk,     // Subblock (0,1,2,or 3)
                                   PIXEL new[], // Upper left pixel of new 16x16 or 8x8 block
                                   PIXEL old[], // Old block (no motion comp)
                                   int xdim,    // Line offset for "new" and "old"
                                   int phase,   // Phase used for computing "mc_err"
                                   int col, int row // (x,y) position for upper left corner
                                   );

//  vlc.c
extern int  EncStuffing( int bits, BS_PTR * bs, 
                    ENCTABENTRY * token[], int maxtok, int maxbits );
extern int  EncodePicLayer( H261PicDescriptor * d, BS_PTR * bs, 
                    ENCTABENTRY * token[], int maxtok, int maxbits,
                    GOB_DESCR * gob,   // Pointer to first GOB (so we can find PQUANT)
                    int * ptype, int * gfid );
extern int  EncodeEOS( BS_PTR * bs );
extern int  EncodeGob( GOB_DESCR * gob, BS_PTR * bs, 
                    ENCTABENTRY * token[], int maxtok, int maxbits, 
                    H261PicDescriptor * d, int mbhor, int gfid, S32 sendGobHeaders );
extern void MvPred( MACROBLOCK_DESCR mb[],
                    int blk,        // specify block: UL, UR, LL, LR, or WHOLE 
                    int mbhor,      // offset from previous row of MBs
                    int horPredOnly, 
                    int *mvX, int *mvY );
extern void InitEncodeTable( void );
extern int CountBits ( MACROBLOCK_DESCR *mb, int blkStart, int blkEnd );

//  vld.c
extern U8 Get8Bits( BS_PTR bs );
extern void IncBsPtr( BS_PTR * bs, int incr );
extern int FLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int bits,           // Symbol length
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        int * value         // Returns decoded symbol
                        );
extern int VLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int tabNum,         // Use dectable[tabNum] for decoding
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        SYMBOL * sym        // Returns decoded symbol
                        );
extern void InitDecodeTable( void );
//  quant2.c
extern int QuantSelect ( S32 codedBlkCnt, H261CodingControl * cntrl, int num_mb,
                         MACROBLOCK_DESCR mb[], MbInfo mbStats[], long *logSum, S32 color,
                         int reducedResUpdate );
extern void InitLogTab ( void );

//  bchenc.c &bchdec.c
extern U32 BchEncode( BS_PTR inStart, U8 P48 outStart, U16 numBlks);
extern U32 BchDecode( S32 P48 sync,
                      S32 P48 syncPhase,
                      S32 maxSyncErrs,
                      S32 correctFlag,
                      S32 P48 numBlks,
                      S32 P48 numErrs,
                      BS_PTR  bsBegin,
                      BS_PTR  bsEnd,
                      BS_PTR  * bsNext,
                      BS_PTR  * bsOut);
                      
// getImgParms().
extern S16 getImgParms( short format, short *numGOBS, short *numMBs, long *imgSize,
            long *lumaSize, long *chromaLineLength, long *chromaRows, long *maxsym);

//  dibtovid.c
extern int DIBToVideo ( VIDEOFrame *vFrame, PICTURE *newIn, long *mbDiff, U32 far *fpbDone,
                        U32 scale, ROI_RECT *roiRect );

//  vvtfilt.c
extern void InitTFiltTable ( int id );
extern short TempFilter ( VIDEOFrame *vFrame, PICTURE *newIn, PICTURE *oldIn, long *mbDiff, 
                            S32 rotate, S32 gamma, S32 enhancerMode);

// pictodib.c
extern int InitYUVToRGB( void );
extern int InitYUVToRGB2( void );

// reshufmm.c
extern void InitGammaCorrection( int fastFilterLuma, int fastFilterChroma );
extern void ReshufMovMan(   int color,          // TRUE: do color, FALSE: don't
                            S32 rotate,         // TRUE: rotate 180 degrees, FALSE: don't
                            S32 gamma,          // TRUE: perform gamma on input, FALSE: don't
                            S32 enhancerMode,   // Image enhancer mode (0=off, 1=1D, 2=2D)
                            int hSize,          // Pixels per row
                            int vSize,          // Lines per picture
                            int offsetVid,      // Line offset, input image
                            S32 *histCount,     // Counter of samples in lumaHist
                            S32 lumaHist[],     // Histogram of raw luma.
                            PIXEL far *pVid,    // Pointer to input image
                            int yOffset,        // Line offset, luma output
                            PIXEL *pLuma,       // Pointer to luma output
                            int cOffset,        // Line offset, chroma outputs
                            PIXEL *pCb, PIXEL *pCr,  // Pointers to chroma outputs
                            short vidType           // Capture frame type e.g. MovieMan, BtV, etc.
);


extern void PreProcessYVU9 ( VIDEOFrame *vFrame, PICTURE *newIn, PICTURE *oldIn, 
                 long *mbDiff);

extern void PreProcessYUY2 ( VIDEOFrame *vFrame, PICTURE *newIn, PICTURE *oldIn, 
                 long *mbDiff);

extern void PreProcessYUV422 ( VIDEOFrame *vFrame, PICTURE *newIn, PICTURE *oldIn, long *mbDiff,
							U8 pTfiltTabL[], U8 pTfiltTabC[]);

// vdecopcl.c
extern int AllocVarStructsDecoder( H261Decoder *dec, short formatCap_or_numMBs );
extern void FreeVarStructsDecoder( H261Decoder *dec );

// recongob.c
extern int ReconGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[],
                     PICTURE * prev_pic, PICTURE * pic, PICTURE * Bpic, 
                     int advancedPred, int PBframe, int PBframeCap, int reducedResUpdate,
                     int advancedIntraMode, H261DecState *state, int maxComp );
extern int ConcealGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int reducedResUpdate,
                       PICTURE * prev_pic, PICTURE * pic );
extern void ReconIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean);
extern void ReconAdvancedIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean);
extern void ReconInter( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean );
extern S8 ReducedResMvComponent( S8 x );
extern void MotionComp32x32( MACROBLOCK_DESCR * mb, // Describes block to be motion-compensated
                            PICTURE * prevPic,  // Describes previous picture used to form MC
                            PICTURE * pic       // Output picture where MC block is placed
                            );
extern void Overlap32x32( MACROBLOCK_DESCR * mb,   // Describes block to be motion-compensated
                        PICTURE * prevPic,  // Describes previous picture used to form MC
                        PICTURE * pic,      // Output picture where MC block is placed
                        int     mbWidth,    // Macroblocks per row
                        int     mbOffset,   // Row offset; (mb-mbOffset) is neighbor on top
                        int     overlap[4]  // Returns YES or NO to indicate whether overlap
                                            // was done in each 8x8 subblock
                        );
extern void Fill32x32( MACROBLOCK_DESCR * mb, PICTURE * pic, PIXEL value );
extern void ReconReducedResMb( MACROBLOCK_DESCR * mb,   // Macroblock to be reconstructed
                               PICTURE * pic,   // Input: motioncomp. prediction;
                                                // output: reconstr. picture
                               int intra,       // INTER block if zero, otherwise INTRA
                               PICTURE * tempPic// Use for temporary storage
                               );

//  Routines used for debugging
extern void printsym( SYMBOL sym );
extern void sprintsym( SYMBOL sym, char s[] );
extern void sprinttype( int symtype, char s[] );
extern void printstate( int state );

#endif
