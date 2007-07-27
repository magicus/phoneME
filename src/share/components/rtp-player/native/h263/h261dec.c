/*-----------------------------------------------------------------------------
 *  H261DEC.C
 *
 *  DESCRIPTION
 *      h261dec.c - H.261 decoder
 *      H261Decode - Decode one or several GOBs in a picture
 *
 *      Search for startcodes, then decode GOBs until reaching the last startcode
 *      or until picture is completely decoded.  We only process complete GOBs,
 *      i.e., we do not process the information following the last found
 *      startcode.  If there is an error in the bitstream for a GOB, the GOB is
 *      not updated but repeated from the previous picture.
 *
 *      next_gob indicates which GOB to decode next.  It is updated and returned
 *      by this routine.  If next_gob=0, the routine skips all GOBs until it
 *      finds a PSC (Picture Start Code) and starts decoding from the PSC.
 *
 *      NOTE:  H.263 with Advanced Prediction Mode (AP mode) requires motion vectors from
 *      the previous row of macroblocks, which are saved in the static array mb[].
 *      If this routine is used for decoding multiple H.263 streams, the array mb[]
 *      should be made part of the H261Decoder structure s.  This will ensure that
 *      the previous row motion vectors are preserved between calls; multiple calls for
 *      decoding an H.263 picture can occur when GOB headers are transmitted.
 *
 *      Function returns YES if decoding of picture is complete, otherwise NO
 *      status.dwStartCodes returns # of startcodes detected
 *      status.dwGobs       returns # GOBs processed
 *      status.dwBadgobs    returns # erroneous GOBs
 *      status.dwIntraFrame returns non-zero if decoded frame is an intraframe
 *      status.psc          returns non-zero if a picture layer is successfully decoded  status.pscIndex
 *      status.pscIndex     returns byte and bit position of the picture start code if status.psc is non-zero
 *
 *      Author:     Staffan Ericsson    6/30/93
 *      Inspector:  Mary Deshon         7/6/93
 *      Revised:
 *      01/03/98    S Ericsson  Minor changes for RTP support: added parms when calling
 *                              DecMbLayer263; made initializePicture and build263gob external
 *      04/04/97    S Ericsson  Comment out update_decMB
 *      03/21/97    S Ericsson  Modify Reduced-Res Update mode to 2/97 draft spec
 *      02/07/97    D Blair     Advanced Intra Mode, Improved PB frames and Deblocking Filter Mode
                                for pseudo H.263+
 *      02/05/97    S Ericsson  Support Reduced-Resolution Update mode
 *                              Split off reconstruction functions into "ReconGob.c"
 *      01/13/97    S Ericsson  Check size of picture arrays in initializePicture
 *      01/04/97    S Ericsson  Decode in several "chunks"; removed TIMER macro
 *      12/19/96    S Ericsson  Support multiple decoders
 *      11/20/96    S Ericsson  Annotation support for PB-frames
 *      11/03/96    S Ericsson  Support PB-frames
 *      07/22/96    S Ericsson  Support ANYSIZE
 *		06/28/96	S Ericsson	Code for debugging: checkPicture etc. (commented out)
 *      06/19/96    S Ericsson  Force display of all MBs if ANNOTATE_PICTURE is defined
 *      06/03/96    S Ericsson  Support UMV mode.
 *                              Display motion etc. (ANNOTATE_PICTURE, DISPLAY_DIFFERENCE)
 *      05/15/96    md          Track dwGobs accurately.
 *      05/06/96    md/se       Fix bug: conceal_gob was not being called if an error occured
 *                              on Gob 0. Track which gobs to update. Track dwFirstBadgob.
 *                              Remove MV debug code.
 *      04/11/96    S Ericsson  Pass PBframeMode to OverlapMC
 *      03/25/96    wolfe       Added output of macroblock motion vectors
 *      02/16/96    wolfe       Merged in S Ericsson's changes of 01/28/96
 *      01/28/06    S Ericsson  Handle H.263 advancedPred with GOB headers
 *                              Define TESTING to turn on thorough bitstream checking
 *      12/21/95    wolfe       Fixed compiler warnings
 *      12/18/95    md			Remove "#include vvenc.h"
 *      09/13/95    S Ericsson  Modified find_sc() to generate detection of last startcode.
 *      09/13/95    M Deshon    Modify to support GFID.
 *      08/22/95    M Deshon    Fix bitstream length=0 bug.
 *      07/23/95    S Ericsson  Support Advanced Prediction mode.
 *                              Declare local functions.
 *      07/10/95    S Ericsson  Added H.263 decoding.
 *                              Renamed BS_ERR_EXIT to BS_ERR_MSG.
 *      03/28/95    wc/jk       Put in fix for CIF->QCIF bug.
 *      03/14/95    md          Add hooks for identifying PSC. Use stats struct instead of an array.
 *      01/16/95    md          Add hooks for identifying an intraframe.
 *      12/13/94    B Girod     Added clean to ReconInter and ReconIntra
 *      10/04/94    md          Turn off CHECKSYM.Causes decode errors on 486 machines.
 *      10/03/94    md          Turn on CHECKSYM.Turn off BS_ERR_EXIT.
 *      09/22/94    md          Turn on CHECKSYM and BS_ERR_EXIT. Use OutputDebugString instead
 *                              of MessageBox in H261ErrMsg.
 *      09/12/94    J Bruder    Stash pspare into PICTURE_STRUCTURE so app has access to it.
 *                              Also fixed bug #1056
 *      08/23/94    G Su        Added a new header file for the optimized VLDECODE routine -
 *                              vldecode.h
 *      07/28/94    M Deshon    Back to using a single decoder map.
 *      06/21/94    mjf         Added some forward decls for MS compiler.
 *      06/15/94    M Deshon    Turned off symbol checking.
 *      06/09/94    M Deshon    Using two maps for selective updating. One for oldOut, one for newOut.
 *      05/09/94    G Su        Changed the checksym function back to static.
 *      05/04/94    G Su        Moved decode_block into a C version file and an Assembly
 *                              version file (dec_blk.c & dec_blk.asm).  Also added a new
 *                              header file: dec_blk.h.
 *      04/25/94    S Ericsson  Replaced ERROR by H261_ERROR (bug #313)
 *      02/23/94    M Deshon    Moved DECODER_MAP define from this file into vvenc.h.
 *      1/19/94     S Ericsson  Update map of macroblock-types (s->decMB)
 *                              Call "initializePicture" only after decoding
 *                              of picture header (bug #121)
 *      12/13/93    M Deshon    Modifications for clean compile with BS_ERR_EXIT null. 
 *      11/16/93    J Bruder    Don't include h221bas.h.  Use CIF/QCIF not BAS codes
 *      11/10/93    J Bruder    Added initializePicture().  THis function
 *                              initializes the picture structures according to
 *                              The decoded format found in the PTYPE field of
 *                              the picture header.  QCIF and CIF are supported.
 *      10/31/93    S Ericsson  TIMER macro turns on timing measurements
 *                              Pass pointers to structures in function calls
 *                              Removed modulo ops. in build_gob_struct, recon_gob 
 *                              and conceal_gob.
 *                              Assume same line offset for Cr and Cb to save 
 *                              a multiply in ReconIntra and ReconInter.
 *      07/27/93    S Ericsson  Removed "exit" calls
 *      07/25/93    M Deshon    Changed build_gob_struct call; int -> S32
 *      07/22/93    S Ericsson  Moved LoopFilter and MotionComp to "predsel.c"
 *      07/20/93    S Ericsson  Fixed handling of MBA Stuffing in decode_gob
 *                              Error messages displayed via H261ErrMsg
 *      07/16/93    S Ericsson  Moved & corrected defs of MV_MIN/MAX/WRAP to "h261defs.h"
 *                              Made ReconIntra, ReconInter, MotionComp, 
 *                              LoopFilter, and BsDiff external functions
 *                              Fixed test of "zzpos" in decode_block/decode_intra_block
 *                              Fixed "newBs" in H261Decode
 *                              Using CHECKSYM and BS_ERR_EXIT macros for debug code
 *                              Use H261Decoder structure for calling H261Decode
 *      07/06/93    M Deshon    Changed to use S8, U8, ... ,S32, U32 
 *  
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#include <stdio.h>
#include <stdlib.h>
#include "dllindex.h"
#include "h261defs.h"
#include "vldstate.h"
#include "h261func.h"
#include "vscodes.h"        /* getImgParms can be called                    */
#include "dec_blk.h"        /* set the proper pragmas for decode_block      */
#include "vldecode.h"       /* set the proper pragmas for VLDecode          */
#include "h263plus.h"

//#define ANNOTATE_PICTURE    // Include diagnostic info in output picture
#ifdef TESTING
#define BS_ERR_MSG(a)   a   // Generate messages on bitstream error (i.e., illegal 
                            // bitstream syntax) to simplify debugging
#define CHECKSYM(a)     a   // Check symbol types to verify decoder state tables

#else
#define BS_ERR_MSG(a)       // Suppress error messages 
#define CHECKSYM(a)         // Don't check symbol types
#endif


// Declarations of local functions
static int  findPSC( BS_PTR start[], int gn[], int num_sc, int *sc, H261Decoder *s );
static void savePSCpointer( BS_PTR *bsPSC, H261Decoder *s );
static void outputBsPointer( BS_PTR * start, BS_PTR * output, int codingMethod );
static int decodeBitstream( H261Decoder *s,
                            GOB_DESCR   *gob,
                            BS_PTR      start[2],
                            int         gn[2],
                            int         *structStatus,
                            int         *decStatus );
static int  doGobStructures( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[],
                        BS_PTR start[2], int gn[2] );
static int  doGobDecoding( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[],
                        BS_PTR start[2], SYMBOL sym[], int maxsym );
static void gobConcealment( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[] );
static int build_gob_struct( S32 format, S32 gn, GOB_DESCR * gob);
/*static int build263gob( S32 format, S32 gn, int next_gn, GOB_DESCR * gob,
                        PICTURE_DESCR * pic );*/
/*static void update_decMB( GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int mb_per_row, 
                        int codingMethod, MBMap * decMB, S32 * iFrame, int forcedUpdate );*/
/*static int  initializePicture( H261Decoder * s );*/
static int  decodePicLayer( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            SYMBOL sym[], int maxsym, int codingMethod, GOB_DESCR * gob,
                            int * ptype );
static int  decode_pic261( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            SYMBOL sym[], int maxsym);
static int  decode261gob(  BS_PTR * bs, int nbits, GOB_DESCR * gob,
                        MACROBLOCK_DESCR mb[], SYMBOL sym[], int maxsym);
static int  decode_mb( SYMBOL insym[], int * nextsym, int * quant,
                        GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int mbnum,
                        SYMBOL outsym[]);
static int  decode_intra_block( SYMBOL insym[], int * nextsym,
                            BLOCK_DESCR * block, SYMBOL outsym[]);
static int  find_startcodes( BS_PTR * bs,   // Points to beginning of bitstream
                            int numbits,    // length of bitstream in bits
                            BS_PTR start[], // Returns positions for startcodes; size "max_start"
                            int gobnum[],   // Returns 4 bits after startcode (5 bits for H.263); size "max_start"
                            int max_start,  // Max number of start codes to be returned
                            int codingMethod );  // Indicate H.261 or H.263 syntax
static int  find_sc( BS_PTR * bs,       // Points to beginning of bitstream
                     int numbits,       // length of bitstream in bits
                     int startZeros,    // number of zeros in startcode
                     BS_PTR * start );   // Returns position for bit after startcode
#ifdef ANNOTATE_PICTURE
// saveOrRestoreReconPicture
static void saveOrRestoreReconPicture( PICTURE *pic,    // Picture to be saved or restored
                                       int  restore     // Save if 0, otherwise restore
                                     );
// annotatePicture - Draw motion vectors etc.
static void annotatePicture( PICTURE *pic, MACROBLOCK_DESCR mb[]);
// annotateMacroblock
static void annotateMacroblock( PIXEL data[], int xdim, MACROBLOCK_DESCR *mb,
                                int xMin, int xMax, int yMin, int yMax );
static void drawMv( PIXEL data[], int xdim, int mvX, int mvY,
                    int xMin, int xMax, int yMin, int yMax );
static void plusMb( PIXEL data[], int xdim );
static void crossMb( PIXEL data[], int xdim );
static void fillMb( PIXEL data[], int xdim );
static void markPixel( PIXEL *pixel );
static int  DrawVector( unsigned char data[], int xdim, int x1, int y1, int x2, int y2 );
static int  drawVec( unsigned char data[], int xdim, int x1, int y1 );
#endif


#define GN_END_OF_SEQUENCE  (31)    // GN=31 for EOS
#define H263_START_ZEROS    (16)
#define H263_GN_BITS        (5)
#define H261_START_ZEROS    (15)
#define H261_GN_BITS        (4)


//
//
//  ****************************************************
//  H261Decode - Decode one or several GOBs in a picture
//  ****************************************************
//
//
extern int  H261Decode( H261Decoder * s )
#define NUM_SC      0
#define NUMGOB      1
#define ERRGOB      2
#define IFRAME      3
{
    BS_PTR      *pStart;    // Position of startcodes ("start")
    int         *pGn;       // GOB numbers following each startcode ("gn")
    GOB_DESCR   *pGob;      // Can refer to several GOBs, if GOB headers are left out ("gob")
    int         *pNum_sc;   // Points to number of startcodes found ("num_sc")
    int         *pSc;       // Points to current startcode ("sc")
    BS_PTR      bsPSC;
    int         status, structStatus, decStatus;
    BS_ERR_MSG(char msg[120];)

    s->state.actComp = 0;  // Init computation measure
    pStart  =  s->state.bspStartCode;
    pGn     =  s->state.gobNumber;
    pGob    = &s->state.currentGob;
    pNum_sc = &s->state.nStartCodes;
    pSc     = &s->state.currentStartCode;

    if (s->pic_layer.decodingInProgress) {
        // Restore internal state; we interrupted reconstruction with status=OK
        structStatus = decStatus = OK;
        s->pic_layer.decodingInProgress = FALSE;    // Set only if we time out again

    } else {    // We did not time out last time, so do the usual thing

        s->state.i = 0;     // Tell ReconGob to reset its state

        /* Find Picture and GOB Start Codes */
        *pNum_sc = find_startcodes( &s->bsStart, BsDiff(s->bsEnd, s->bsStart),
                                    pStart, pGn, SC_MAX, (int)s->codingMethod );
        /*{   // Print startcode positions
            int i;
            for (i = 0; i < *pNum_sc; i++) {
                printf("Startcode %d: gn = %d starts in byte %d, bit %d\n", i,
                    pGn[i], pStart[i].byteptr - s->bsStart.byteptr, pStart[i].bitptr);
            }
            printf("Continue? ");
            scanf("%d", &i);
        }*/
        s->status.dwStartcodes = *pNum_sc;
        s->status.dwGobs = 0;       // # GOBs processed
        s->status.dwBadgobs = 0;    // # erroneous GOBs
        s->status.dwUpdateGobs = 0; // # GOBs for fast updating
        s->status.dwFirstBadgob = 0; // First bad Gob
        s->status.dwPsc = FALSE;    // indicates that a picture layer was successfully decoded
        s->newBs = s->bsStart;      // Just in case...  (will be updated later)
        if (*pNum_sc < 2) {
            if (*pNum_sc == 1) {
                outputBsPointer( &pStart[0], &s->newBs, (int)s->codingMethod ); // If we exit, begin here next time
            } else if ((s->bsEnd.byteptr - s->bsStart.byteptr) < 4) {
                // No startcode found and bitstream is less than 4 bytes long
                s->newBs = s->bsStart;
            } else {
                // No startcode found; skip to end of bitstream
                s->newBs = s->bsEnd;
                s->newBs.byteptr -= 4;  // Back up 32 bits (22-bit PSC and one byte margin)
            }
            //fprintf(stderr, "returned in 1\n");
            return( NO );
        }
        *pSc = 0;
        if (s->next_gob == 0) {   // Decode Picture Layer info
            do {
                if (findPSC( pStart, pGn, *pNum_sc, pSc, s ) != YES){   // Find Picture Start Code
                                //fprintf(stderr, "returned in 2\n");
return( NO );}
                // Try to decode Picture Layer
                outputBsPointer( &pStart[*pSc], &bsPSC, (int)s->codingMethod ); // Save possible picture start code pointer
                status = decodePicLayer( &pStart[*pSc], BsDiff(pStart[*pSc + 1], pStart[*pSc]),
                                    &s->pic_layer, s->sym, s->maxSym, (int)s->codingMethod, pGob,
                                    &s->ptype );
                // H.263: pStart[*pSc] gets bumped by decodePicLayer; now pointing to MB layer
                if (status != OK) {
                    ++s->status.dwBadgobs;
                    BS_ERR_MSG( H261ErrMsg( "Bitstream error when decoding Picture Layer" ); )
                } else {    // Decoded Picture Layer successfully
                    savePSCpointer( &bsPSC, s );
                    // Initialize image structures according to received image format: QCIF or CIF
                    status = InitializePicture ( s );   
                    if (status != OK)  H261ErrMsg("Decoder: illegal picture format");
                    s->status.dwIntraFrame = 1;  // Assume intraframe (set to zero if we encounter a non-INTRA MacroBlock)
#ifdef ANNOTATE_PICTURE
                    saveOrRestoreReconPicture( &s->oldOut, 1 ); // Restore prev. from local memory
#endif
                    if (s->codingMethod == H263_CODING  &&  status == OK) {
                        s->next_gob = 0;    // First GOB is gn=0 for H.263
                        break;  // Don't increment *pSc; decode GOB 0 first
                    } else if (status == OK) {  // H.261 decoding
                        s->next_gob = 1;    // Go on to decode first GOB (gn=1 for H.261)
                    }
                }
                ++(*pSc);
            } while (status != OK);
        }
    
        // Decode GOBs until we reach next to last startcode or until picture complete
        if (*pSc >= *pNum_sc - 1)  {            //fprintf(stderr, "returned in 3\n");
return NO;}

        // Decode bitstream for the first GOB
        status = decodeBitstream( s, pGob, &pStart[*pSc], &pGn[*pSc], &structStatus, &decStatus );
        if (status == H261_ERROR)  {            //fprintf(stderr, "returned in 4\n");
return NO;  } // Exit to resync on next PSC

    }   // end if !s->pic_layer.decodingInProgress

    while (1) {     // Process until next to last startcode
        // Reconstruct GOB
        if (structStatus == OK) {
            // decStatus!=OK means that we found bitstream errors
            if (decStatus == OK) {
                //printf("Calling ReconGob \n");
                ReconGob( pGob, s->mb, &s->oldOut, &s->newOut, &s->B_Out, 
                           s->pic_layer.advancedPred, s->pic_layer.PBframeMode, 
                           s->PBframeCap, s->pic_layer.reducedResUpdate, s->pic_layer.advancedIntraMode,
                           &s->state, s->maxComp );
                // Time to exit?
                if (s->state.i > 0) {   // ReconGob timed out
                    s->pic_layer.decodingInProgress = TRUE;
                                //fprintf(stderr, "returned in 5\n");
return NO;
                }
				/*// Update macroblock map to control RGB conversion
				// This is not very useful anymore; skipped blocks might change when
				// we use overlapped MC or deblocking filter /SE 4/4/97
                update_decMB( pGob, pGob->mb, (int)(s->newOut.y.nhor >> 4), 
                            (int)s->codingMethod, &s->decMB,
                            &s->status.dwIntraFrame, NO );*/
            } else {    //  Bitstream error
                gobConcealment( s, pGob, s->mb );
            }
            s->status.dwGobs += pGob->num_mb / max(1,pGob->mb_width);
            s->next_gob = pGob->next_gn;
        }
        (*pSc)++;
        outputBsPointer( &pStart[*pSc], &s->newBs, (int)s->codingMethod ); // If we exit, begin here next time
        if (s->next_gob == 0) {    // Completed picture
#ifdef DO_H263_PLUS
            // Perform deblocking filtering
            if(s->pic_layer.reducedResUpdate) {
                ReducedResDeblockingFilter( &s->newOut, s->mb );
            } else if(s->pic_layer.deblockingFilterMode) {
                ApplyDeblockingFilter(&s->newOut, s->mb, FALSE);
                if(s->pic_layer.PBframeMode && s->PBframeCap) 
                    ApplyDeblockingFilter(&s->B_Out, s->mb, TRUE);
            }
#endif
            //checkPicture( s->newOut );  // Very useful when chasing subtle encoder bugs
#ifdef ANNOTATE_PICTURE
            saveOrRestoreReconPicture( &s->newOut, 0 ); // Save new picture to local memory
            annotatePicture( &s->newOut, s->mb );  // Draw motion vectors etc.
            annotatePicture( &s->B_Out, s->mb );
#endif
            return (YES);
        }
        if (*pSc >= *pNum_sc - 1)  {            //fprintf(stderr, "returned in 6\n");
return NO;}

        // Decode bitstream for the next GOB
        status = decodeBitstream( s, pGob, &pStart[*pSc], &pGn[*pSc],
                                  &structStatus, &decStatus );
        if (status == H261_ERROR)  {            //fprintf(stderr, "returned in 7\n");
return NO;}   // Exit to resync on next PSC
    }
}


// findPSC - Search for Picture Start Code
static int  findPSC( BS_PTR start[], int gn[], int num_sc, int *sc, H261Decoder *s )
{
    do {
        if (*sc > num_sc - 2) { //  Reached last startcode?
            return (NO);
        }
        (*sc)++;
        outputBsPointer( &start[*sc], &s->newBs, (int)s->codingMethod ); // If we exit, begin here next time
    } while (gn[*sc - 1] != 0); // GN=0 indicates PSC
    *sc = *sc - 1;    // Back up to point at PSC
    return( YES );
}


// savePSCpointer - Pass the pointer to the picture start code to caller
static void savePSCpointer( BS_PTR *bsPSC, H261Decoder *s )
{
    s->status.dwPsc = TRUE; 
    s->status.pscIndex.ww.byte = bsPSC->byteptr - s->bsAbs.byteptr; 
    s->status.pscIndex.ww.bit =  bsPSC->bitptr & 0xff;
    return;
}

// outputBsPointer - Copy "start" pointer and back up by length of startcode
static void outputBsPointer( BS_PTR * start, BS_PTR * output, int codingMethod )
{
    int lenStartCode;
    
    if (codingMethod == H263_CODING) {
        lenStartCode = H263_START_ZEROS + 1;
    } else {
        lenStartCode = H261_START_ZEROS + 1;
    }
    *output = *start;
    IncBsPtr( output, -lenStartCode );
    return;
}


// decodeBitstream - Decode bitstream for a GOB
//  Returns H261_ERROR if we need to exit to resync on next PSC
static int decodeBitstream( H261Decoder *s,
                            GOB_DESCR   *gob,
                            BS_PTR      start[2],
                            int         gn[2],
                            int         *structStatus,
                            int         *decStatus )
{
    BS_ERR_MSG(char msg[120];)
    int nextStart;

    *structStatus = doGobStructures( s, gob, s->mb, &start[0], &gn[0] );
    if (*structStatus == OK  &&  gn[0] == s->next_gob) {
        // GOB number is correct and structures in place
        *decStatus = doGobDecoding( s, gob, s->mb, &start[0], s->sym, s->maxSym );
    } else if (*structStatus == OK  &&  gn[0] == gob->next_gn) {     // GBSC was probably trashed
        // This case will not occur for H.263
        *structStatus = H261_ERROR;
        //printf("Calling ConcealGob \n");
        ConcealGob( gob, s->mb, s->pic_layer.reducedResUpdate, &s->oldOut, &s->newOut);
        ++s->status.dwBadgobs;
        s->status.dwGobs += gob->num_mb / max(1,gob->mb_width);
        s->next_gob = gob->next_gn;
        s->status.dwIntraFrame=0;
        BS_ERR_MSG(
            sprintf( msg, "Bitstream error: did not find GOB %d", s->next_gob);
            H261ErrMsg( msg ); )
    } else if (gn[0] == 0) {   // Unexpected PSC; exit so we can resync
        *structStatus = H261_ERROR;
        if (s->codingMethod == H263_CODING  &&  s->next_gob == 0) {
            nextStart = 1;  // Try next startcode; this PSC did not work
        } else {
            nextStart = 0;  // Start with PSC next time
        }
        outputBsPointer( &start[nextStart], &s->newBs, (int)s->codingMethod );
        BS_ERR_MSG( H261ErrMsg("Bitstream error: encountered PSC when expecting GBSC"); )
        ++s->status.dwBadgobs;
        s->status.dwIntraFrame=0;
        s->next_gob = 0;
        return H261_ERROR;  // This picture did not get done; start again
    } else {    // Assume that GBSC was false; keep looking for GN = next_gob
        *structStatus = H261_ERROR;
    }
    return OK;
}


// doGobStructures - Create gob and mb structures
static int  doGobStructures( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[],
                        BS_PTR start[2], int gn[2] )
{
    int     maxbits, status;
    
    //printf("Entering doGobStructures:  gn[] = %d %d\n", gn[0], gn[1]);
    gob->mb = mb;
    if (s->codingMethod == H263_CODING) {
        if (s->next_gob > 0) {  // No GOB Layer for GOB 0
            maxbits = BsDiff(start[1], start[0]);
            status = DecGobLayer263( &start[0], maxbits, gob, &s->gfid );
            // start[0] gets bumped by DecGobLayer263; now pointing to MB layer
            if (status != OK) {
                BS_ERR_MSG( H261ErrMsg( "Bit error: DecGobLayer263 returned error" );)
                return( H261_ERROR );
            }
        }
        status = Build263gob( s->pic_layer.format, s->next_gob, gn[1], gob, &s->pic_layer );
        if (status != OK) {
            BS_ERR_MSG( H261ErrMsg( "Bit error: Build263gob returned error" );)
            return( H261_ERROR );
        }
    } else {    // H.261
        status = build_gob_struct( s->pic_layer.format, s->next_gob, gob );
        if (status != OK) {
            H261ErrMsg( "Program error: build_gob_struct returned error" );
            return( H261_ERROR );
        }
    }
    return( status );
}


// doGobDecoding - Decode all information in a GOB
static int  doGobDecoding( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[],
                        BS_PTR start[2], SYMBOL sym[], int maxsym )
{
    int     maxbits, status, dummy, rtpFlag = 0;
    
    //printf("Entering doGobDecoding:  num_mb = %d\n", gob->num_mb);
    maxbits = BsDiff(start[1], start[0]);
    if (s->codingMethod == H263_CODING) {
        mb += gob->first_col + gob->first_row * gob->mb_offset;
        status = DecMbLayer263( &start[0], maxbits, gob, mb,
                                    (int)s->pic_layer.interFrame,
                                    (int)s->pic_layer.PBframeMode,
                                    (int)s->pic_layer.unrestrictedMv,
                                    (int)s->pic_layer.advancedIntraMode,
                                    sym, maxsym, gob->mb_width, rtpFlag, &dummy );
    } else {    // H.261
        status = decode261gob( &start[0], maxbits, gob, mb, sym, maxsym);
    }
    if (status != OK) {
        BS_ERR_MSG( H261ErrMsg( "Bitstream error in doGobDecoding" );)
    }
    return( status );
}


// gobConcealment - Conceal error and update error counters
static void gobConcealment( H261Decoder *s, GOB_DESCR *gob, MACROBLOCK_DESCR mb[] )
{
    BS_ERR_MSG( char msg[120]; )
    
    //printf("Calling ConcealGob \n");
    ConcealGob( gob, mb, s->pic_layer.reducedResUpdate, &s->oldOut, &s->newOut );
    if (s->pic_layer.PBframeMode && s->PBframeCap) {
        // Conceal B-frame
        ConcealGob( gob, mb, s->pic_layer.reducedResUpdate, &s->oldOut, &s->B_Out );
    }
    if (s->status.dwUpdateGobs == 0)   // If this is the first bad gob
    {
        s->status.dwFirstBadgob = s->next_gob;      // then save the number
        s->status.dwUpdateGobs = gob->num_mb/max(1,gob->mb_width);
    } else {
        s->status.dwUpdateGobs = s->next_gob + gob->num_mb / max(1,gob->mb_width) -
                                s->status.dwFirstBadgob;
    }
    ++s->status.dwBadgobs;
    s->status.dwIntraFrame=0;
    BS_ERR_MSG(
        sprintf( msg, "Bitstream error when decoding GOB %d", s->next_gob);
        H261ErrMsg( msg ); )
}


//  build_gob_struct - set up GOB descriptor and MB descriptors for GOB "gn"
static int build_gob_struct( S32 format, S32 gn, GOB_DESCR * gob)
#define MB_COLS_PER_GOB     11
#define MB_ROWS_PER_GOB     3
{
    int mb_col, mb_row, row, col, index;

    if (gn < 1  ||  gn > 12  ||     // CIF: GN=1,2,..,12
        (format == 0  &&  gn != 1  &&  gn != 3  &&  gn != 5)) { // QCIF: 1,3,5
        return (H261_ERROR);
    }
    // Fill in GOB descriptor
    gob->num_mb = MB_COLS_PER_GOB * MB_ROWS_PER_GOB;
    gob->first_col = 0;
    gob->first_row = 0;
    gob->mb_width = MB_COLS_PER_GOB;
    gob->mb_offset = MB_COLS_PER_GOB;
    // Determine next GOB number
    if (format == 0  &&  gn < 5) {  // QCIF
        gob->next_gn = gn + 2;
    } else if (format != 0  &&  gn < 12) {  // CIF
        gob->next_gn = gn + 1;
    } else {    // This is last GOB in picture
        gob->next_gn = 0;   // Look for PSC next time
    }
    // Fill in macroblock locations
    if ((gn & 0x1)  ==  1) {
        mb_col = 0;                 // Odd GOBs on left side
    } else {
        mb_col = MB_COLS_PER_GOB;   // Even GOBs on right side
    }
    mb_row = ((gn - 1) / 2) * MB_ROWS_PER_GOB;
    for (row = 0; row < MB_ROWS_PER_GOB; row++) {
        for (col = 0; col < MB_COLS_PER_GOB; col++) {
            index = row * MB_COLS_PER_GOB + col;
            (gob->mb + index)->x = col + mb_col;
            (gob->mb + index)->y = row + mb_row;
        }
    }
    return (OK);
}


// Build263gob - set up GOB and MB descriptors for GOBS "gn" to "next_gn-1" (H.263)
extern int Build263gob( S32 format, S32 gn, int next_gn, GOB_DESCR * gob,
                        PICTURE_DESCR * pic )
{
    int mbhor, mbvert, num_gn, row, col, index, mbRowsPerGob;

    switch (format) {
    case SQCIF:
        mbhor = 8;
        num_gn = 6;
        mbRowsPerGob = 1;
        break;
    case QCIF:
        mbhor = 11;
        num_gn = 9;
        mbRowsPerGob = 1;
        break;
    case CIF:
        mbhor = 22;
        num_gn = 18;
        mbRowsPerGob = 1;
        break;
    case CIF4:
        mbhor = 44;
        num_gn = 18;
        mbRowsPerGob = 2;
        break;
    case CIF16:
        mbhor = 88;
        num_gn = 18;
        mbRowsPerGob = 4;
        break;
    case ANYSIZE:
        mbhor = (pic->cols + 15) >> 4;
        num_gn = (pic->rows + 15) >> 4;
        mbRowsPerGob = 1;
        while (num_gn > GN_END_OF_SEQUENCE) {   // Max Group Number is 30
            mbRowsPerGob *= 2;
            num_gn = (num_gn + 1) >> 1;
        }
        break;
    default:
        return( H261_ERROR );
        break;
    }
#ifdef DO_H263_PLUS
    if (pic->reducedResUpdate) {
        // Use 32x32 macroblocks
        mbhor = ROUNDDIV2( mbhor );
        if (mbRowsPerGob == 1) {
            num_gn = ROUNDDIV2( num_gn );
        } else {
            mbRowsPerGob = ROUNDDIV2( mbRowsPerGob );
        }
    }
#endif
    if (gn < 0  ||  gn >= num_gn)  return( H261_ERROR );
    // Fill in GOB descriptor
    if (next_gn == 0  ||  next_gn == GN_END_OF_SEQUENCE) {
        gob->next_gn = 0;   // Look for PSC next time
        mbvert = num_gn - gn;
    } else {
        gob->next_gn = next_gn;
        mbvert = next_gn - gn;
    }
    mbvert *= mbRowsPerGob;
    if (mbvert <= 0)  return( H261_ERROR );
    gob->num_mb = mbhor * mbvert;
    gob->first_col = 0;
    gob->first_row = gn * mbRowsPerGob; // Need previous row for advancedPred mode
    gob->mb_width = mbhor;
    gob->mb_offset = mbhor;
    // Fill in macroblock locations
    for (row = gn * mbRowsPerGob; row < gn * mbRowsPerGob + mbvert; row++) {
        for (col = 0; col < mbhor; col++) {
            index = row * mbhor + col;
            gob->mb[index].x = col;
            gob->mb[index].y = row;
        }
    }
    return (OK);
}

/*********************
//  update_decMB - Update macroblock map by ORing MTYPE for each MB
static void update_decMB( GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int mb_per_row, 
                        int codingMethod, MBMap * decMB, S32 * iFrame, int forcedUpdate )
{
    int mbnum, i, row, col;
    U8  mtype;
    
    if (codingMethod == H263_CODING) {
        mbnum = gob->first_col + gob->first_row * gob->mb_offset;
        for (i = mbnum; i < mbnum + gob->num_mb; ++i) {
            mtype = mb[i].mtype;
            decMB->data[i] |= mtype;
            if ( mtype != MTYPE263_INTRA  &&  mtype != MTYPE263_INTRA_Q )  *iFrame = 0;
        } 
        if (forcedUpdate == YES) {  // Ensure that macroblock map is non-zero
            for (i = mbnum; i < mbnum + gob->num_mb; ++i) {
                decMB->data[i] = 1;
            } 
        }
    } else {    // H.261
        mbnum = mb[0].x + mb[0].y * mb_per_row;
        i = 0;
        for (row = 0; row < MB_ROWS_PER_GOB; row++) {
            for (col = 0; col < MB_COLS_PER_GOB; col++) {
                mtype = mb[i + col].mtype;
                decMB->data[mbnum + col] |= mtype;
                if ( mtype != MTYPE_INTRA  &&  mtype != MTYPE_INTRA_MQUANT )  *iFrame = 0;
            }
            mbnum += mb_per_row;
            i += MB_COLS_PER_GOB;
        } 
        if (forcedUpdate == YES) {  // Ensure that macroblock map is non-zero
            mbnum = mb[0].x + mb[0].y * mb_per_row;
            for (row = 0; row < MB_ROWS_PER_GOB; row++) {
                for (col = 0; col < MB_COLS_PER_GOB; col++) {
                    decMB->data[mbnum + col] = 1;
                }
                mbnum += mb_per_row;
            } 
        }
    }
    return;
}
*****************/

/**************************************************************************
* InitializePicture ( H261Decoder *s) -
*
* this function initializes the picture structures according to the
* format: QCIF or CIF
*
***************************************************************************/
#ifdef WATD
extern void BreakPoint(void);
#pragma aux BreakPoint = 0xcc;
#endif
extern int  InitializePicture( H261Decoder * s )
{
  S16   format;                     /* locals used for call to getImgParms  */
  S16   numGOBs;                    /* this is done for consistency with    */
  S16   numMBs;                     /* old init method                      */
  S32   imgSize ;
  S32   lumaSize;
  S32   chromaLineLength;
  S32   chromaRows;
  S32   maxsym;  
  int   status, mbHor, mbVert;

#ifdef WATD
   //BreakPoint();
#endif
  format = (S16)s->pic_layer.format;
  s->decMB.format = format;
  s->decMB.type = DECODER_MAP;
    
  if ( (s->formatCap == QCIF) && (format == CIF) )
    H261ErrMsg("decoder not CIF capable");

                    /* get the image parameters for the current format  */
  if (format == ANYSIZE) {
        mbHor = (s->pic_layer.cols + 15) >> 4;
        mbVert = (s->pic_layer.rows + 15) >> 4;
        //imgSize  = (long)384 * mbHor * mbVert;
        lumaSize = (long)256 * mbHor * mbVert;
        chromaLineLength = 8 * mbHor;
        chromaRows = 8 * mbVert;
        //numGOBs  = mbVert;
        numMBs   = mbHor * mbVert;
        //maxsym = 8 * lumaSize;
        status = 0;
        /////////if (numMBs > xxxxx)  status = UNKNOWN_PICTURE_FORMAT;
  } else {
    status = getImgParms (format, &numGOBs, &numMBs, &imgSize, &lumaSize, &chromaLineLength,
                    &chromaRows, &maxsym);
  }
  if (status != 0) {
    H261ErrMsg ("Decoder Error: InitializePicture - UNKNOWN_PICTURE_FORMAT");
    status = H261_ERROR;
  } else if (numMBs > s->maxMbnum) {
    H261ErrMsg ("Decoder Error: InitializePicture - Too large picture");
    status = H261_ERROR;
  } else {
    status = OK;
  }
  // setup the image structures
  s->newOut.picLayout = s->B_Out.picLayout = VVS_LAYOUT_TEE;
  //s->oldOut.picLayout   = VVS_LAYOUT_TEE;
  
  // Memory was already allocated when the decoder first opened.
  // The luma pointers hold the address of the start of the memory for image buffers
                                    
  // Set up chroma pointers
  s->newOut.cb.ptr = s->newOut.y.ptr + lumaSize;
  s->B_Out.cb.ptr  = s->B_Out.y.ptr  + lumaSize;
  //s->oldOut.cb.ptr = s->oldOut.y.ptr + lumaSize;
  s->newOut.cr.ptr = s->newOut.cb.ptr + chromaLineLength;

  s->B_Out.cr.ptr  = s->B_Out.cb.ptr  + chromaLineLength;
  //s->oldOut.cr.ptr = s->oldOut.cb.ptr + chromaLineLength;

  s->newOut.cb.ptrAlias = s->newOut.y.ptrAlias + lumaSize;
  s->B_Out.cb.ptrAlias  = s->B_Out.y.ptrAlias  + lumaSize;
  //s->oldOut.cb.ptrAlias = s->oldOut.y.ptrAlias + lumaSize;
  s->newOut.cr.ptrAlias = s->newOut.cb.ptrAlias + chromaLineLength;

  s->B_Out.cr.ptrAlias  = s->B_Out.cb.ptrAlias  + chromaLineLength;
  //s->oldOut.cr.ptrAlias = s->oldOut.cb.ptrAlias + chromaLineLength;

  // Set up remainder of COMPONENT structs
  s->newOut.y.nhor    = s->B_Out.y.nhor    = chromaLineLength * 2;
  s->newOut.y.nvert   = s->B_Out.y.nvert   = chromaRows * 2;
  s->newOut.y.hoffset = s->B_Out.y.hoffset = chromaLineLength * 2;
  //s->oldOut.y.nhor  = chromaLineLength * 2;
  //s->oldOut.y.nvert = chromaRows * 2;
  //s->oldOut.y.hoffset   = chromaLineLength * 2;

  s->newOut.cb.nhor    = s->B_Out.cb.nhor    = chromaLineLength;
  s->newOut.cb.nvert   = s->B_Out.cb.nvert   = chromaRows;
  s->newOut.cb.hoffset = s->B_Out.cb.hoffset = chromaLineLength * 2;
  //s->oldOut.cb.nhor = chromaLineLength;
  //s->oldOut.cb.nvert= chromaRows;
  //s->oldOut.cb.hoffset= chromaLineLength * 2;

  s->newOut.cr.nhor    = s->B_Out.cr.nhor    = chromaLineLength;
  s->newOut.cr.nvert   = s->B_Out.cr.nvert   = chromaRows;
  s->newOut.cr.hoffset = s->B_Out.cr.hoffset = chromaLineLength * 2;
  //s->oldOut.cr.nhor = chromaLineLength;
  //s->oldOut.cr.nvert= chromaRows;
  //s->oldOut.cr.hoffset= chromaLineLength * 2;

  return (status);
}


//  decodePicLayer - Decode Picture Layer information
static int  decodePicLayer( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            SYMBOL sym[], int maxsym, int codingMethod, GOB_DESCR * gob,
                            int * ptype )
{
    int     status;
    
    if (codingMethod == H263_CODING) {
        status = DecPicLayer263( bs, nbits, pic, gob, ptype );
    } else {
        status = decode_pic261( bs, nbits, pic, sym, maxsym );
    }
    return( status );
}


//  decode_pic261 - Decode H.261 Picture Layer information
//  This routine is very picky when determining whether bitstream is valid.
//  It returns OK only if a startcode ends the bitstream; otherwise, it
//  returns H261_ERROR.
static int  decode_pic261( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            SYMBOL sym[], int maxsym)
{
    int     state, status, parsed_bits, nsym, i;
    BS_ERR_MSG( char msg[120] );

    state = ST_AFTER_STARTCODE;
    //printf( "DecodePic: Entering VLDecode with  State = %d  nbits = %d\n",
    //            state, nbits);
    status = VLDECODE( *bs, nbits, &state, &parsed_bits, &nsym, sym, maxsym);
    if (parsed_bits != nbits) {
        BS_ERR_MSG( sprintf( msg, "decode_pic_layer: Tried to decode %d bits, exit after %d bits",
                nbits, parsed_bits);
                H261ErrMsg( msg ); )
        return (H261_ERROR);
    }
    if (state != ST_AFTER_STARTCODE) {
        BS_ERR_MSG( sprintf( msg, "decode_pic_layer: Bitstream did not end with startcode");
                H261ErrMsg( msg); )
        return (H261_ERROR);
    }
/*    {
        int i;
        printf( "DecodePic: Status = %d  State = %d  Decoded %d bits, %d symbols\n",
                status, state, parsed_bits, nsym);
        // Print decoded symbols
        printf("\n");
        for (i = 0; i < nsym; i++) {
            printf("DecodePic: "); printsym( sym[i] ); printf("\n");
        }
        printf("\n");
    }*/
    CHECKSYM( if (checksym( sym[0], SYM_GN, "decode_pic_layer") != OK) exit(0); )
    i = 1;  // Skip first symbol ("GN" = 0)
    CHECKSYM( if (checksym( sym[i], SYM_QUANT_TR, "decode_pic_layer") != OK) exit(0); )
    pic->tr = sym[i++].value;
    CHECKSYM( if (checksym( sym[i], SYM_PTYPE, "decode_pic_layer") != OK) exit(0); )
    pic->ptype = sym[i++].value;
    pic->splitscreen = pic->ptype & FLAG_SPLITSCREEN;
    pic->doccamera = pic->ptype & FLAG_DOCCAMERA;
    pic->fp_release = pic->ptype & FLAG_FP_RELEASE;
    if (pic->ptype & FLAG_CIF_FORMAT) {
        pic->format = CIF;
    } else {
        pic->format = QCIF;
    }
    pic->hi_res = (pic->ptype & FLAG_HI_RES) ^ FLAG_HI_RES; // 1 = off, 0 = on
    pic->advancedPred = 0;  // No overlapped MC for H.261
    pic->PBframeMode = 0;   // No PB-frames for H.261
#ifdef DO_H263_PLUS
    pic->reducedResUpdate = 0;
#endif
    pic->peiCount = 0;                  // Loop 'til PEI=0
    while (sym[i].type == SYM_SPARE) {
        //char msg[100];
        //sprintf(msg,"\n Decode DLL: PEI=%x", sym[i].value);
        //OutputDebugString(msg);
        if (pic->peiCount < MAX_PEI_COUNT) {
            //char msg[100];
            //sprintf(msg,"\n Decode DLL: pSpare: %2x", sym[i].value);
            //OutputDebugString(msg);
            pic->pSpare[pic->peiCount] = sym[i].value;
        }
        pic->peiCount++;                // Inc counter
        i++;                            // Next Symbol  
    }

    CHECKSYM( if (checksym( sym[i], SYM_GEI_PEI, "decode_pic_layer") != OK) exit(0));    
    i++;                                // Skip PEI Symbol
    
    if (sym[i].type != SYM_STARTCODE) {
        BS_ERR_MSG( sprintf( msg, "decode_pic_layer: Did not find startcode after pic_layer");
                H261ErrMsg( msg ); )
        return (H261_ERROR);
    }
    return (OK);
}


//  decode261gob - Decode all symbols for a Group Of Blocks
//  This routine is very picky when determining whether bitstream is valid.
//  It returns OK only if a startcode ends the bitstream; otherwise, it
//  returns H261_ERROR.
static int  decode261gob(  BS_PTR * bs, int nbits, GOB_DESCR * gob,
                        MACROBLOCK_DESCR mb[], SYMBOL sym[], int maxsym)
{
    int     state, status, parsed_bits, mbnum, nextsym, quant, i, mba,
            nsym,       // # symbols returned by VLDecode
            isym;       // # symbols returned by this routine, i.e.,
                        // after deletion of EOBs etc.
    BS_ERR_MSG ( char msg[120] );

    state = ST_AFTER_STARTCODE;
    //printf( "DecodeGOB: Entering VLDecode with  State: ");
    //printstate( state ); printf("  nbits = %d\n", nbits);
    status = VLDECODE( *bs, nbits, &state, &parsed_bits, &nsym, sym, maxsym);
    if (parsed_bits != nbits) {
        BS_ERR_MSG( sprintf( msg, "decode261gob: Tried to decode %d bits, exit after %d bits",
                nbits, parsed_bits);
                H261ErrMsg( msg ); )
        return (H261_ERROR);
    }
    if (state != ST_AFTER_STARTCODE) {
        BS_ERR_MSG( sprintf( msg, "decode261gob: Bitstream did not end with startcode");
                H261ErrMsg( msg ); )
        return (H261_ERROR);
    }
/*    {
        int num;
        printf( "DecodeGOB: Status = %d  State: ", status); printstate( state );
        printf("  Decoded %d bits, %d symbols\n", parsed_bits, nsym);
        // Print decoded symbols
        printf("DecodeGOB: # symbols to print: ");
        scanf("%d", &num);
        for (i = 0; i < num; i++) {
            printf("DecodeGOB: "); printsym( sym[i] ); printf("\n");
        }
        printf("\n");
    }
 */
    nextsym = 0;
    CHECKSYM( if (checksym( sym[nextsym], SYM_GN, "decode261gob") != OK) exit(0); )
    gob->gn = sym[nextsym++].value;      // GOB number
    CHECKSYM( if (checksym( sym[nextsym], SYM_QUANT_TR, "decode261gob") != OK) exit(0); )
    quant = sym[nextsym++].value;
    if (quant < QUANT_MIN  ||  quant > QUANT_MAX) {
        BS_ERR_MSG( sprintf( msg, "decode261gob: quant = %d", quant);
                H261ErrMsg( msg ); )
        return (H261_ERROR);
    }

    gob->gquant = quant;
    //CHECKSYM( if (checksym( sym[nextsym], SYM_GEI_PEI, "decode261gob") != OK) exit(0);  )
    //gob->gei = sym[nextsym].value;
    gob->num_gspare = 0;
    while (sym[nextsym].type == SYM_SPARE) {   // Loop until GEI=0
        CHECKSYM( if (checksym( sym[nextsym], SYM_SPARE, "decode261gob") != OK) exit(0); )
        ++nextsym;  // Drop GSPARE on the floor
        ++(gob->num_gspare);
        
    }
    CHECKSYM( if (checksym( sym[nextsym], SYM_GEI_PEI, "decode261gob") != OK) exit(0); )
    ++nextsym;      // Skip GEI
    
            
    mbnum = 0;
    isym = 0;
    //  We expect MBA, MBA Stuffing, or Startcode
    while (sym[nextsym].type == SYM_MBA_STUFFING) {
        ++nextsym;      // Remove MBA stuffing
    }
    while (sym[nextsym].type != SYM_STARTCODE) {    // Keep going until next startcode
        // We expect MBA
        CHECKSYM( if (checksym( sym[nextsym], SYM_MBA, "decode261gob") != OK) exit(0); )
        mba = sym[nextsym++].value;
        //printf("MB #%d: ", mbnum+mba); printsym( sym[nextsym-1] ); // MBA
        //printf("   "); printsym( sym[nextsym] ); printf("\n"); // MTYPE
        if (mbnum + mba  >  gob->num_mb) {
            BS_ERR_MSG( sprintf( msg, "decode261gob: Bitstream error, mbnum=%d", mbnum + mba);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
        for (i = mbnum + 1; i < mbnum + mba; i++) {
            mb[i-1].mtype = MTYPE_SKIP;
        }
        mbnum += mba;
        status = decode_mb( sym, &nextsym, &quant, gob, mb, mbnum-1, &sym[isym] );
        if (status == H261_ERROR) {
            BS_ERR_MSG( sprintf( msg, "decode261gob: Bitstream error, MB #%d", mbnum);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
        isym += status;
        //  We expect MBA, MBA Stuffing, or Startcode
        while (sym[nextsym].type == SYM_MBA_STUFFING) {
            ++nextsym;      // Remove MBA stuffing
        }
    }
    for (i = mbnum + 1; i <= gob->num_mb; i++) {
        mb[i-1].mtype = MTYPE_SKIP;
    }
    return (OK);
}


//  insym[*nextsym] contains MTYPE when we enter
static int  decode_mb( SYMBOL insym[], int * nextsym, int * quant,
                        GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int mbnum,
                        SYMBOL outsym[])
{
    int isym, blk, status, mask;
    CHECKSYM ( char msg[120] );

    //printf("decode_mb: "); printsym( insym[*nextsym] ); printf("\n");
    isym = 0;
    CHECKSYM( if (checksym( insym[*nextsym], SYM_MTYPE, "decode_mb") != OK) exit(0); )
    mb[mbnum].mtype = insym[(*nextsym)++].value;
    switch ( mb[mbnum].mtype ) {
    case MTYPE_INTER_MQUANT:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_QUANT_TR, "decode_mb") != OK) exit(0); )
        *quant = insym[(*nextsym)++].value;
        if (*quant < QUANT_MIN  ||  *quant > QUANT_MAX) {
            BS_ERR_MSG( sprintf( msg, "decode_mb: quant = %d", *quant);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
        // FALLTHROUGH
    case MTYPE_INTER:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_CBP, "decode_mb") != OK) exit(0); )
        mb[mbnum].cbp = insym[(*nextsym)++].value;
        mask = 0x20;    // Bitmask for first block
        for (blk = 0; blk < 6; blk++) {
            mb[mbnum].block[blk].nsym = 0;
            if ((mb[mbnum].cbp & mask) != 0) {
                status = decode_block( insym, nextsym,
                                &(mb[mbnum].block[blk]),
                                &outsym[isym] );
                if (status == H261_ERROR) {
                    BS_ERR_MSG( sprintf( msg, "decode_mb: Bitstream error, block #%d", blk);
                            H261ErrMsg( msg ); )
                    return (H261_ERROR);
                }
                isym += status;
            }
            mask >>= 1;
        }
        break;

    case MTYPE_MCFILT_MQUANT:
    case MTYPE_MC_MQUANT:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_QUANT_TR, "decode_mb") != OK) exit(0); )
        *quant = insym[(*nextsym)++].value;
        if (*quant < QUANT_MIN  ||  *quant > QUANT_MAX) {
            BS_ERR_MSG( sprintf( msg, "decode_mb: quant = %d", *quant);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
        // FALLTHROUGH
    case MTYPE_MCFILT_CBP:
    case MTYPE_MC_CBP:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_MVD, "decode_mb") != OK) exit(0); )
        mb[mbnum].mvd_x = insym[(*nextsym)++].value;
        CHECKSYM( if (checksym( insym[*nextsym], SYM_MVD, "decode_mb") != OK) exit(0); )
        mb[mbnum].mvd_y = insym[(*nextsym)++].value;
        if (mbnum % gob->mb_width > 0  &&
                    mb[mbnum-1].mtype >= MTYPE_MC_NOCBP) {
             mb[mbnum].mv_x = mb[mbnum-1].mv_x + mb[mbnum].mvd_x;
             mb[mbnum].mv_y = mb[mbnum-1].mv_y + mb[mbnum].mvd_y;
        } else {
             mb[mbnum].mv_x = mb[mbnum].mvd_x;
             mb[mbnum].mv_y = mb[mbnum].mvd_y;
        }
        CHECKSYM( if (checksym( insym[*nextsym], SYM_CBP, "decode_mb") != OK) exit(0); )
        mb[mbnum].cbp = insym[(*nextsym)++].value;
        mask = 0x20;    // Bitmask for first block
        for (blk = 0; blk < 6; blk++) {
            mb[mbnum].block[blk].nsym = 0;
            if ((mb[mbnum].cbp & mask) != 0) {
                status = decode_block( insym, nextsym,
                                &(mb[mbnum].block[blk]),
                                &outsym[isym] );
                if (status == H261_ERROR) {
                    BS_ERR_MSG( sprintf( msg, "decode_mb: Bitstream error, block #%d", blk);
                            H261ErrMsg( msg ); )
                    return (H261_ERROR);
                }
                isym += status;
            }
            mask >>= 1;
        }
        break;

    case MTYPE_MCFILT_NOCBP:
    case MTYPE_MC_NOCBP:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_MVD, "decode_mb") != OK) exit(0); )
        mb[mbnum].mvd_x = insym[(*nextsym)++].value;
        CHECKSYM( if (checksym( insym[*nextsym], SYM_MVD, "decode_mb") != OK) exit(0); )
        mb[mbnum].mvd_y = insym[(*nextsym)++].value;
        if (mbnum % gob->mb_width > 0  &&
                    mb[mbnum-1].mtype >= MTYPE_MC_NOCBP) {
             mb[mbnum].mv_x = mb[mbnum-1].mv_x + mb[mbnum].mvd_x;
             mb[mbnum].mv_y = mb[mbnum-1].mv_y + mb[mbnum].mvd_y;
        } else {
             mb[mbnum].mv_x = mb[mbnum].mvd_x;
             mb[mbnum].mv_y = mb[mbnum].mvd_y;
        }
        break;

    case MTYPE_INTRA_MQUANT:
        CHECKSYM( if (checksym( insym[*nextsym], SYM_QUANT_TR, "decode_mb") != OK) exit(0); )
        *quant = insym[(*nextsym)++].value;
        if (*quant < QUANT_MIN  ||  *quant > QUANT_MAX) {
            BS_ERR_MSG( sprintf( msg, "decode_mb: quant = %d", *quant);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
        // FALLTHROUGH
    case MTYPE_INTRA:
        for (blk = 0; blk < 6; blk++) {
            status = decode_intra_block( insym, nextsym,
                                &(mb[mbnum].block[blk]),
                                &outsym[isym] );
            if (status == H261_ERROR) {
                BS_ERR_MSG( sprintf( msg, "decode_mb: Bitstream error, block #%d", blk);
                        H261ErrMsg( msg  ); )
                return (H261_ERROR);
            }
            isym += status;
        }
        break;

    default:    // Illegal MTYPE is due to program error; not from bit errors
        CHECKSYM( sprintf( msg, "decode_mb: MTYPE = %d", mb[mbnum].mtype);
            H261ErrMsg( msg ); )
        return (H261_ERROR);
        break;
    }
    mb[mbnum].quant = *quant;
    return (isym);
}


static int  decode_intra_block( SYMBOL insym[], int * nextsym,
                            BLOCK_DESCR * block, SYMBOL outsym[])
{
    int isym, zzpos, run;
    CHECKSYM ( char msg[120] );

    /* DC coefficient */
    //printf("decode_intra_block DC: "); printsym( insym[*nextsym] ); printf("\n");
    CHECKSYM( if (checksym( insym[*nextsym], SYM_INTRA_DC, "decode_intra") != OK) exit(0); )
    outsym[0].type = 0;
    outsym[0].value = insym[(*nextsym)++].value;
    /* AC coefficients */
    isym = 1;
    zzpos = 1;
    //printf("decode_intra_block AC: "); printsym( insym[*nextsym] ); printf("\n");
    while (insym[*nextsym].type != SYM_EOB) {
        if (insym[*nextsym].type == SYM_ESC_RUN) {
            run = insym[(*nextsym)++].value;
            outsym[isym].type = run;
            CHECKSYM( if (checksym( insym[*nextsym], SYM_ESC_LEVEL, "decode_intra") != OK) exit(0); )
            outsym[isym++].value = insym[(*nextsym)++].value;
        } else {
            run = insym[*nextsym].type;
            CHECKSYM( if (run < 0) {
                sprintf( msg, "PROGRAM ERROR: run = %d in decode_intra", run);
                H261ErrMsg( msg );
                return( H261_ERROR );
                } )
            outsym[isym++] = insym[(*nextsym)++];
        }
        zzpos += run + 1;
//        printf("decode_intra_block AC: "); printsym( insym[*nextsym] );
//        printf("   zzpos = %d\n", zzpos);
        if (zzpos > 64) {   // If we decoded coeff. 63, we will now have zzpos=64
            BS_ERR_MSG( sprintf( msg, "decode_intra_block: Bitstream error, zzpos=%d", zzpos);
            H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
    }
    (*nextsym)++;   /* Advance pointer to symbol after EOB */
    block->sym = outsym;
    block->nsym = isym;
    return (isym);
}



/* Compute # bits in segment bs1 - bs2 */
extern int BsDiff( BS_PTR bs1, BS_PTR bs2 )
{
    int bits;

    bits = 8 * (bs1.byteptr - bs2.byteptr) + bs1.bitptr - bs2.bitptr;
    return (bits);
}


static int  find_startcodes( BS_PTR * bs,   // Points to beginning of bitstream
                            int numbits,    // length of bitstream in bits
                            BS_PTR start[], // Returns positions for startcodes; size "max_start"
                            int gobnum[],   // Returns 4 bits after startcode (5 bits for H.263); size "max_start"
                            int max_start,  // Max number of start codes to be returned
                            int codingMethod )  // Indicate H.261 or H.263 syntax
{
    int i, bits_left, startZeros, gnBits;
    BS_PTR  bs_next;

    if (codingMethod == H263_CODING) {
        startZeros = H263_START_ZEROS;
        gnBits = H263_GN_BITS;
    } else {    // H.261
        startZeros = H261_START_ZEROS;
        gnBits = H261_GN_BITS;
    }
    i = 0;
    bs_next = *bs;
    while (i < max_start) {
        bits_left = numbits - BsDiff( bs_next, *bs);
        if (find_sc( &bs_next, bits_left, startZeros, &start[i] ) != YES) {
            return (i);
        }
        gobnum[i] = Get8Bits( start[i] ) >> (8 - gnBits);
        bs_next = start[i];
        i++;
    }
    return (max_start);
}


static  U8 leading[256];
static  U8 trailing[256];

extern void InitFindSC( void )
{
    int i, j, bitmask;

    for (i = 0; i < 256; i++) {
        leading[i] = 0;
        bitmask = 0x80;
        for (j = 0; j < 8; j++) {
            if ((i & bitmask) != 0) {
                break;
            }
            bitmask >>= 1;
            ++leading[i];
        }
        trailing[i] = 0;
        bitmask = 0x1;
        for (j = 0; j < 8; j++) {
            if ((i & bitmask) != 0) {
                break;
            }
            bitmask <<= 1;
            ++trailing[i];
        }
        //printf("%3x: Leading = %d  Trailing = %d\n", i, leading[i], trailing[i]);
    }
    return;
}


//  find_sc - Find startcode consisting of startZeros zeros followed by 1
//  Returns YES if startcode found, otherwise NO
//  Operates on byte boundaries; might not look at last 8 bits in bitstream
static int  find_sc( BS_PTR * bs,       // Points to beginning of bitstream
                     int numbits,       // length of bitstream in bits
                     int startZeros,    // number of zeros in startcode
                     BS_PTR * start )   // Returns position for bit after startcode
{
    int     zeros, nbytes, i, validBitsInLastByte;
    U8   b;

    /* Read first byte from bitstream; set "irrelevant bits" to 1 */
    b = *bs->byteptr | (0xff & (0xff << (8 - bs->bitptr)));
    zeros = trailing[b];
//    nbytes = (numbits + bs->bitptr - 8 - max(H263_GN_BITS, H261_GN_BITS) ) >> 3;
    nbytes = (numbits + bs->bitptr - 1 ) >> 3;
    validBitsInLastByte = (numbits + bs->bitptr) &07;
    if (validBitsInLastByte == 0)
        validBitsInLastByte = 8;
    start->byteptr = bs->byteptr + 1;
    for (i = 0; i < nbytes; i++) {
        b = *start->byteptr;
        zeros += leading[b];
        if (zeros < startZeros  &&  b != 0) {
            zeros = trailing[b];
        } else if (b != 0) {    /* Found startcode */
            start->bitptr = leading[b] + 1;
            if ((i == nbytes - 1 && 
		 start->bitptr + H263_GN_BITS > validBitsInLastByte)
                ||
                (i == nbytes - 2  &&
                start->bitptr + H263_GN_BITS > 8 + validBitsInLastByte))
                return(NO);
            if (start->bitptr > 7) {
                ++(start->byteptr);
                start->bitptr -= 8;
            }
            return (YES);
        }   /* else: b=0; continue until 1 is found */
        ++(start->byteptr);
    }
    return (NO);
}


///////////  Routines for debugging  //////////////
extern int  checksym( SYMBOL sym, int type, char routine[] )
{
  // char msg[120], csym[80], ctype[80];
    
    if (sym.type == type) {
        return (OK);
    } else {    // Not expected type
	// H263-1998 RTP can result in false errors on short blocks
        BS_ERR_MSG ( sprintsym( sym , csym );
        sprinttype( type, ctype );
        sprintf( msg, "%s: Encountered %s  Expected %s", routine, csym, ctype);
        H261ErrMsg( msg ); )
        return (H261_ERROR);
    }
}


#ifdef ANNOTATE_PICTURE
// saveOrRestoreReconPicture
static void saveOrRestoreReconPicture( PICTURE *pic,    // Picture to be saved or restored
                                       int  restore     // Save if 0, otherwise restore
                                     )
{
#define MAX_BYTES   ((176*144*3)/2)
    int             nBytes;
    static PIXEL    savePic[MAX_BYTES];
    
    nBytes = (pic->y.nhor * pic->y.nvert * 3) >> 1;
    // Packed format is assumed
    if (nBytes > MAX_BYTES) {
        H261ErrMsg("Error in saveOrRestoreReconPicture\n");
        exit(0);
    } else if (restore) {   // Restore the saved picture
        memcpy( pic->y.ptr, savePic, nBytes );
    } else {                // Save picture for next decoding
        memcpy( savePic, pic->y.ptr, nBytes );
    }
}


// annotatePicture - Draw motion vectors etc.
static void annotatePicture( PICTURE *pic, MACROBLOCK_DESCR mb[])
{
    int     xSize, ySize, numMb, i, x, y;
    PIXEL   *upperLeft;
    
    xSize = pic->y.nhor;
    ySize = pic->y.nvert;
    numMb = (xSize * ySize) >> 8;
    for (i = 0; i < numMb; ++i) {
        x = 16 * mb[i].x;
        y = 16 * mb[i].y;
        upperLeft = pic->y.ptr + x + y * pic->y.hoffset;
        annotateMacroblock( upperLeft, pic->y.hoffset, &mb[i],
                            -x, xSize - 1 - x,
                            -y, ySize - 1 - y );
    }
}

#define WHITE   (255)
#define BLACK   (0)
#define BLACK_THRESHOLD (192)   // Paint with black if pixel is brighter than threshold
// Limit x to interval [low,high]
#define LIMIT( low, x, high )    max( low, min( x, high ))

// annotateMacroblock
static void annotateMacroblock( PIXEL data[], int xdim, MACROBLOCK_DESCR *mb,
                                int xMin, int xMax, int yMin, int yMax )
{

    switch (mb->mtype)
    {
    case MTYPE_SKIP:
        // Do nothing
        break;
    case MTYPE263_INTER:
    case MTYPE263_INTER_Q:
        // Draw motion vector
        drawMv( &data[7 + 7 * xdim], xdim, mb->mv_x, mb->mv_y,
                xMin-7, xMax-7, yMin-7, yMax-7 );
        break;
    case MTYPE263_INTER4V:
        // Draw a "plus"
        plusMb( data, xdim );
        // Draw 4 motion vectors
        drawMv( &data[3 + 3 * xdim], xdim, mb->blkMvX[0], mb->blkMvY[0],
                xMin-3, xMax-3, yMin-3, yMax-3 );
        drawMv( &data[11 + 3 * xdim], xdim, mb->blkMvX[1], mb->blkMvY[1],
                xMin-11, xMax-11, yMin-3, yMax-3 );
        drawMv( &data[3 + 11 * xdim], xdim, mb->blkMvX[2], mb->blkMvY[2],
                xMin-3, xMax-3, yMin-11, yMax-11 );
        drawMv( &data[11 + 11 * xdim], xdim, mb->blkMvX[3], mb->blkMvY[3],
                xMin-11, xMax-11, yMin-11, yMax-11 );
        break;
    case MTYPE263_INTRA:
    case MTYPE263_INTRA_Q:
        //  Draw motion vector and mark block with a cross
        drawMv( &data[7 + 7 * xdim], xdim, mb->mv_x, mb->mv_y,
                xMin-7, xMax-7, yMin-7, yMax-7 );
        crossMb( data, xdim );
        break;
    default:
        // Fill block with white
        fillMb( data, xdim );
        break;
    }
}


// drawMv - Draw motion vector starting in (0,0)
static void drawMv( PIXEL data[], int xdim, int mvX, int mvY,
                    int xMin, int xMax, int yMin, int yMax )
{
    if (mvX == 0  &&  mvY == 0) {
        // Draw a circle
        markPixel( &data[-xdim] );
        markPixel( &data[-xdim + 1] );
        markPixel( &data[-1] );
        markPixel( &data[2] );
        markPixel( &data[xdim - 1] );
        markPixel( &data[xdim + 2] );
        markPixel( &data[2*xdim] );
        markPixel( &data[2*xdim+1] );
    } else {    // Draw motion vector
        // Divide by 2 (one frac. bit); round "away" from 0
        /*if (mvX > 0) ++mvX;
        mvX >>= 1;
        if (mvY > 0) ++mvY;
        mvY >>= 1;*/    // Leave mv's scaled up by factor of 2
        // Clip to picture rectangle
        mvX = LIMIT( xMin, mvX, xMax );
        mvY = LIMIT( yMin, mvY, yMax );
        DrawVector( data, xdim, 0, 0, mvX, mvY );
    }
}


// plusMb - Mark block with a "plus"
static void plusMb( PIXEL data[], int xdim )
{
    int col;
    
    // Horizontal line
    data += 7 * xdim;
    for (col = 5; col <= 9; ++col) {
        markPixel( &data[col] );
    }
    // Vertical line
    markPixel( &data[7 - 2 * xdim] );
    markPixel( &data[7 - 1 * xdim] );
    markPixel( &data[7 + 1 * xdim] );
    markPixel( &data[7 + 2 * xdim] );
}


// crossMb - Mark block with a cross
static void crossMb( PIXEL data[], int xdim )
{
    int row;
    
    for (row = 0; row < 16; ++row) {
        markPixel( &data[row] );
        markPixel( &data[15 - row] );
        data += xdim;
    }
}


// fillMb - Fill block with white
static void fillMb( PIXEL data[], int xdim )
{
    int row, col;
    
    for (row = 0; row < 16; ++row) {
        for (col = 0; col < 16; ++col) {
            markPixel( &data[col] );
        }
        data += xdim;
    }
}


// markPixel - Set pixel to white if < 192, otherwise set it to black
static void markPixel( PIXEL *pixel )
{
    if (*pixel < BLACK_THRESHOLD) {
        *pixel = WHITE;
    } else {
        *pixel = BLACK;
    }
}

///////////// Graphics routines /////////////

#include <stdlib.h>
#include <math.h>

// Static function declarations
static int DrawVector( unsigned char data[], int xdim, int x1, int y1, int x2, int y2 );
static int drawVec( unsigned char data[], int xdim, int x1, int y1 );

// DrawVector - Draw vector from (x1,y1) to (x2,y2)
static int DrawVector( unsigned char data[], int xdim, int x1, int y1, int x2, int y2 )
{
    int x, y;
    
    if (y1 == y2) {         // Draw horizontal line
        data += y1 * xdim;
        for (x = min(x1,x2); x <= max(x1,x2); ++x)  markPixel( &data[x] );
    } else if (x1 == x2) {  // Draw vertical line
        data += min(y1,y2) * xdim;
        for (y = min(y1,y2); y <= max(y1,y2); ++y) {
            markPixel( &data[x1] );
            data += xdim;
        }
    } else if (x2 > x1) {   // Use (x1,y1) as origin
        data += x1 + y1 * xdim;
        drawVec( data, xdim, x2-x1, y2-y1 );
    } else {                // Use (x2,y2) as origin
        data += x2 + y2 * xdim;
        drawVec( data, xdim, x1-x2, y1-y2 );
    }
    return(1);
}


// drawVec - Draw vector from origin to (x1,y1)
//  Assumes that x1 > 0, and y1 != 0
//  Draw pixels that are within 0.5 units distance of vector
static int drawVec( unsigned char data[], int xdim, int x1, int y1 )
{
    int x, y, xLast, xNew;
    float   fX1, fY1, fNorm, fA, fB, fXstep, fX, fDist;
    
    if (y1 < 0) {
        y1 = -y1;
        xdim = -xdim;
    }
    // Now, both x1 and y1 are >0
    // Compute equation for line:  ax + by = 0, where (a,b) is normal vector of length 1
    // The distance between (x,y) and the line is the inner product: abs(ax + by)
    fX1 = (float)x1;
    fY1 = (float)y1;
    fNorm = (float) sqrt( fX1*fX1 + fY1*fY1);
    fA = -fY1 / fNorm;  // fA < 0
    fB = fX1 / fNorm;   // fB > 0
    fXstep = fX1 / fY1; // x increment per row
    // First row
    fX = (float)(0.5 * fXstep);  // Intersection with y=0.5
    xLast = (int)fX;    // Truncate (Note: fX > 0; int always truncates towards zero)
    for (x = 0; x <= xLast; ++x)  markPixel( &data[x] );
    // Intermediate rows
    for (y = 1; y < y1; ++y) {
        data += xdim;
        // Check distance to (xLast, y)
        fDist = fA * (float)xLast + fB * (float)y;  // fDist > 0
        if (fDist < 0.4999) {
            markPixel( &data[xLast] );
        } else if (fDist + fA - fB > -0.4999) {     // Check distance to (xLast+1, y-1)
            //fDist = fA * (float)(xLast+1) + fB * (float)(y-1);  // fDist < 0
            markPixel( &data[xLast+1 - xdim] );
        }
        fX += fXstep;   // Intersection with y+0.5
        xNew = (int)fX;
        for (x = xLast + 1; x <= xNew; ++x)  markPixel( &data[x] );
        xLast = xNew;
    }
    // Last row (y = y1)
    data += xdim;
    // Check distance to (xLast, y1)
    fDist = fA * (float)xLast + fB * (float)y1; // fDist > 0
    if (fDist < 0.4999) {
        markPixel( &data[xLast] );
    } else if (fDist + fA - fB > -0.4999) {     // Check distance to (xLast+1, y1-1)
        markPixel( &data[xLast+1 - xdim] );
    }
    for (x = xLast + 1; x <= x1; ++x)  markPixel( &data[x] );
    
    return(1);
}

#endif  /* ANNOTATE_PICTURE */

/*
// Compute checksums for picture (used for debugging only)
static int checkPicture( PICTURE pic );
static int checkComponent( COMPONENT comp );

static PIXEL	encPic[176*144], decPic[176*144], diffPic[176*144];

extern int storePicture( PICTURE pic, int picNum )
{
	int		i, j;
	PIXEL	*in, *out;

	in = pic.y.ptr;
	if (picNum == 0)
		out = encPic;
	else
		out = decPic;
    for (i = 0; i < pic.y.nvert; i++) {
        for (j = 0; j < pic.y.nhor; j++) {
			out[j] = in[j];
        }
		in += pic.y.hoffset;
		out += pic.y.nhor;
    }
	return( 1 );
}

static int checkPicture( PICTURE pic )
{
	int	        y, cb, cr, i;
    static int  sum = 0;

	storePicture( pic, 1 );
	y = checkComponent( pic.y );
	cb = checkComponent( pic.cb );
	cr = checkComponent( pic.cr );
	sum += y + cb + cr;
	for (i = 0; i < 176*144; ++i)
		diffPic[i] = encPic[i] - decPic[i];
	return( sum );
}

static int checkComponent( COMPONENT comp )
{
    int		i, j, sum;
    PIXEL   *p;

	sum = 0, p = comp.ptr;
    for (i = 0; i < comp.nvert; i++) {
        for (j = 0; j < comp.nhor; j++) {
			sum += p[j];
        }
		p += comp.hoffset;
    }
	return( sum );
}
*/
