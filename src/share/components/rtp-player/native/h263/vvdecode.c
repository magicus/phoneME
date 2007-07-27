/*-----------------------------------------------------------------------------
 *  VVDECODE.C
 *
 *  DESCRIPTION
 *      Video Decode
 *      These functions are part of the dynamic link library VVCODER.DLL.
 *
 *  NOTES
 *      Color is set to true always - this should be set from outside
 *
 *  ASSUMPTIONS
 *      1. This is 32-bit code.
 *      2. All pointers which are passed into the DLL from the
 *          application are 16:16 pointers.
 *  
 *  Author:     Mary Deshon     7/21/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 *  01/07/98    se      RTP support: H263PacketDecode, H263FinishPicture. Restructured
 *                      VvDecode to share code with RTP functions.
 *  02/12/97    se      Report to higher layer whether we are returning a B-frame
 *  01/13/97    se      Increase size of decoder arrays if needed
 *  01/04/97    se      Turn on decoding in "chunks"
 *  12/19/96    se      Perform doublebuffering of output frames
 *                      Support multiple decoders and decoding in several "chunks"
 *  10/22/96    se      Support PB-frames
 *  04/19/96    wolfe   Fixed WATCOM 10.0a compiler warnings for Win16 build
 *  01/21/96    wolfe   Added wrapper around "__declspec" function specs
 *  12/18/95    md      Remove "#include vvenc.h"
 *  03/17/95    chet    changed arguments in VvDecGetMBMap to match vvenc.h
 *  03/14/95    md  Minor changes to support record and playback.
 *  01/19/95    md  Use manifest constant for size of status array.
 *  11/07/94    md  Clear Chroma components when CIF -> QCIF and monochrome.
 *  10/02/94    J Bruder    Clear Chroma components when color changes to monochrome.
 *  07/28/94    M Deshon    Back to using a single decoder map.
 *  06/09/94    M Deshon    Copy newDecMB to oldDecMB (this could be changed to a pointer swap).
 *  04/19/94    M Deshon    Added debugging macro.
 *  02/23/94    M Deshon    Added VvDecGetMBMap().
 *  11/16/93    J Bruder    Don't include h221bas.h
 *
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#define RTP_SUPPORT

/*  Header File Includes        */
#include "machine.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#ifdef FOR_MAC
#include <stdlib.h>
#endif
//#include "vvdefs.h"
#include "dllindex.h"
#include "h261func.h"
#include "h261defs.h"
#include "vscodes.h"
#include "vvcoder.h"

/*  Defines, Typedefs, & Enums  */
//#define DEBUG_VVDECODE(a) a
#define DEBUG_VVDECODE(a)

/*  Globals                     */
extern H261Decoder *DecoderRegistry[MAX_NUM_DECODERS];

/*  File-wide Statics           */
/******************************************************************************
* clearChromaFrame (PICTURE *newIn)
*
*
*/
static void clearChromaComponent (COMPONENT *c)
{
   PIXEL    *p;
   S32      nRows, nCols;
    
    /* clear the chroma image component */  
    p = c->ptr;
    nRows = c->nvert;  
    while (nRows--) {
        nCols = c->nhor;
        while (nCols--) {
         *p++ = 0x80;
        }
        p += (c->hoffset - c->nhor);
    }
}

static void clearChromaFrame ( PICTURE *newIn)
{
    /* clear both image components  */
    clearChromaComponent (&newIn->cr);
    clearChromaComponent (&newIn->cb);

    return;
}
/*-----------------------------------------------------------------------------
 *  Function:   VvDecode
 *
 *  DESCRIPTION
 *      Starts bitstream decoding.
 *
 *  CALLING SYNTAX
 *      VvDecode    S16 index
 *                  U32 PBframeCap
 *                  U32 picdesc
 *                  U32 bsStart
 *                  U32 bsEnd
 *                  U32 nextGOB
 *                  U32 newPic
 *                  U32 status
 *                  U32 newBS
 *
 *      index:      Indicates which decoder is being called.
 *      PBframeCap  If 0: no B-frame reconstruction; otherwise, reconstruct B-frame
 *      picdesc:    Pointer to a PICTURE DESCR struct used for the decoded picture layer info
 *      bsStart:    Offset into bistream indicating where to begin decoding:
 *                      high word indicates offset to a byte,lo word indicates bit number.
 *      bsEnd:      Offset into bistream indicating where to stop decoding (worst case):
 *                      high word indicates offset to a byte,lo word indicates bit number.
 *      nextGOB:    Pointer to S16 indicating which GOB to decode next. If zero, the routine
 *                  skips all GOBs until it finds a PSC (Picture Start Code ).
 *      newPic:     Pointer to a PICTURE struct used for the decoded image
 *      status:     Pointer to an array containing status info
 *      newBs:      Pointer to offset into bitstream indicating where the decoder actually stopped decoding 
 *                      high word indicates offset to a byte,lo word indicates bit number.
 * 
 *  RETURNS
 *      Returns type U32 containing status; YES = completed decoding an entire image
 *
 *  Author:     Mary Deshon     7/21/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/

#define NUM_STATUS_WORDS    4
#define BAD_PARMS   (-2)

// Static function declarations
// setupDecStructure - Initialize decoding structure before calling H261Decode/H263RtpDecode
static int setupDecStructure( H261Decoder *dec,
                              U32 PBframeCap,
                              PICTURE far *far32_newPic,
                              int *oldFormat
                              );
// juggleFrames - Rename frames after completing decoding of a picture
static int juggleFrames( H261Decoder *dec,
                         int oldFormat,
                         int decStatus,
                         PICTURE far *far32_newPic
                         );

//#define RTP_TEST_HARNESS

#ifndef RTP_TEST_HARNESS

#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvDecode ( U32 pdec, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs )
#else
__declspec(dllexport) U32 FAR PASCAL VvDecode ( U32 pdec, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs )
#endif
{
    H261Decoder     *dec;
    PICTURE_DESCR   far *far32_picdesc;
    PICTURE         far *far32_newPic;
    U32             far *far32_newBs;
    VvDecoderStats  far *far32_status;
    S16             far *far32_nextGOB;
    int             retval, oldFormat, numMBs;
    DEBUG_VVDECODE  (char   buf[128];)

    dec = (H261Decoder *)pdec;

    // Get pointers we can use
    far32_picdesc   =  makeFlat32( (void *)picdesc ); // 48-bit pointer equivalent to picdesc
    far32_status    =  makeFlat32( (void *)status );  // 48-bit pointer equivalent to status
    far32_nextGOB   =  makeFlat32( (void *)nextGOB ); // 48-bit pointer equivalent to nextGOB
    far32_newPic    =  makeFlat32( (void *)newPic );  // 48-bit pointer equivalent to newPic
    far32_newBs     =  makeFlat32( (void *)newBs );   // 48-bit pointer equivalent to newBs

    if ( !far32_picdesc->decodingInProgress ) {
        // Set picture size (if ANYSIZE)
        far32_picdesc->rows = ((far32_picdesc->rows + 15)>>4)<<4;
        far32_picdesc->cols = ((far32_picdesc->cols + 15)>>4)<<4;
        dec->pic_layer.rows = far32_picdesc->rows;
        dec->pic_layer.cols = far32_picdesc->cols;
	dec->maxComp  = 0;
        // Are arrays big enough?
        numMBs = ((dec->pic_layer.rows + 15) >> 4) *
                 ((dec->pic_layer.cols + 15) >> 4);
        if (numMBs > dec->maxMbnum) {
            // Reallocate arrays
            FreeVarStructsDecoder( dec );
            retval = AllocVarStructsDecoder( dec, (short)numMBs );
            if (retval)  return NO;
        }
        status = setupDecStructure( dec, PBframeCap, far32_newPic, &oldFormat );
        if (status != OK) return BAD_PARMS;
#ifdef JM_BIG_ENDIAN	// Mac
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart & 0xffff) ;
        dec->bsStart.bitptr =  (bsStart >> 16) & 0xff;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd & 0xffff) ;
        dec->bsEnd.bitptr =  (bsEnd >> 16) & 0xff;
#else
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart >> 16) ;
        dec->bsStart.bitptr =  bsStart & 0xff ;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd >> 16) ;
        dec->bsEnd.bitptr =  bsEnd & 0xff ;
#endif
        // Set nextGOB
        dec->next_gob = *far32_nextGOB;
    }
    // Call Decoder
    retval = H261Decode( dec );
    
    // Check if we are done
    far32_picdesc->decodingInProgress = dec->pic_layer.decodingInProgress;
    if ( dec->pic_layer.decodingInProgress )
        return 0;   // Not done yet

    // Copy to return areas
    _fmemcpy ( far32_picdesc, (void far *)&dec->pic_layer, sizeof ( PICTURE_DESCR ) );
    _fmemcpy ( far32_status, (void far *)&dec->status, sizeof (VvDecoderStats) );
    _fmemcpy ( far32_nextGOB, (void far *)&dec->next_gob, sizeof ( S32 ) );

    // Return updated bitstream
#ifdef JM_BIG_ENDIAN	// e.g. Mac
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) && 0xffff) |
                    ( (dec->newBs.bitptr << 16) & 0xff );
#else
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) << 16) |
                    ( dec->newBs.bitptr & 0xff );
#endif

    retval = juggleFrames( dec, oldFormat, retval, far32_newPic );
    // Report to higher layer whether we are returning a B-frame
    far32_status->dwBframe = dec->pendingFrame;
    // Return status; YES = completed decoding a picture, otherwise NO
    return ( (U32) retval );
}

#else   /* RTP_TEST_HARNESS */

extern U32 H263PacketDecode(
            H261Decoder *dec,   // (I/O) Decoder structure created by VvOpenDecoder
                                //  Keeps the decoder's internal state and returns
                                //  information useful to higher level routines.
                                //  The following returned info is of particular interest:
                                //  dec->status     Decoder stats (first bad GOB etc)
                                //  dec->pic_layer  Picture layer info
                                //  dec->pNewPic    Pointer to most recent output frame
                                //  dec->pPrevPic   Pointer to previous output frame
                                //  dec->pendingFrame   Set to 1 when returning B-frame,
                                //                      otherwise 0.
            int headerLength,   // (Input) Number of bytes in h263header
            char *h263header,   // (Input) H.263 payload header according to RFC2190
            U32 PBframeCap,     // (Input) If set, decoder will reconstruct B-frames
            PICTURE *newPic,    // (I/O) On input, "color" setting controls whether
                                //  chrominance is reconstructed.
                                //  On output, newPic contains a copy of the PICTURE
                                //  structure pointed to by dec->pNewPic
            int payloadLength,  // (Input) Number of bytes in payloadData
            char *payloadData   // (Input) H.263 bitstream
            );

#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvDecode ( S16 index, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs )
#else
__declspec(dllexport) U32 FAR PASCAL VvDecode ( S16 index, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs )
#endif
{
    H261Decoder     *dec;
    PICTURE_DESCR   far *far32_picdesc;
    PICTURE         far *far32_newPic;
    U32             far *far32_newBs;
    VvDecoderStats  far *far32_status;
    S16             far *far32_nextGOB;
    int             retval;
    char            hdr[4]; // mode A header

    dec = DecoderRegistry[index-1];

    // Get pointers we can use
    far32_picdesc   =  makeFlat32( (void *)picdesc ); // 48-bit pointer equivalent to picdesc
    far32_status    =  makeFlat32( (void *)status );  // 48-bit pointer equivalent to status
    far32_nextGOB   =  makeFlat32( (void *)nextGOB ); // 48-bit pointer equivalent to nextGOB
    far32_newPic    =  makeFlat32( (void *)newPic );  // 48-bit pointer equivalent to newPic
    far32_newBs     =  makeFlat32( (void *)newBs );   // 48-bit pointer equivalent to newBs

    if ( !far32_picdesc->decodingInProgress ) {
#ifdef JM_BIG_ENDIAN	// Mac
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart & 0xffff) ;
        dec->bsStart.bitptr =  (bsStart >> 16) & 0xff;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd & 0xffff) ;
        dec->bsEnd.bitptr =  (bsEnd >> 16) & 0xff;
#else
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart >> 16) ;
        dec->bsStart.bitptr =  bsStart & 0xff ;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd >> 16) ;
        dec->bsEnd.bitptr =  bsEnd & 0xff ;
#endif
        // Create payload header assuming that bitstream contains whole frame
        hdr[0] = (dec->bsStart.bitptr & 0x7) << 3;
    }
    // Call RTP Decoder
    retval = H263PacketDecode( dec, 4, hdr, PBframeCap, far32_newPic,
                               1 + dec->bsEnd.byteptr - dec->bsStart.byteptr,
                               dec->bsStart.byteptr);

    // Check if we are done
    far32_picdesc->decodingInProgress = dec->pic_layer.decodingInProgress;
    if ( dec->pic_layer.decodingInProgress )
        return 0;   // Not done yet

    // Copy to return areas
    _fmemcpy ( far32_picdesc, (void far *)&dec->pic_layer, sizeof ( PICTURE_DESCR ) );
    _fmemcpy ( far32_status, (void far *)&dec->status, sizeof (VvDecoderStats) );
    _fmemcpy ( far32_nextGOB, (void far *)&dec->next_gob, sizeof ( S32 ) );

    // Return updated bitstream
#ifdef JM_BIG_ENDIAN	// e.g. Mac
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) && 0xffff) |
                    ( (dec->newBs.bitptr << 16) & 0xff );
#else
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) << 16) |
                    ( dec->newBs.bitptr & 0xff );
#endif

    // Report to higher layer whether we are returning a B-frame
    far32_status->dwBframe = dec->pendingFrame;
    // Return status; YES = completed decoding a picture, otherwise NO
    return ( (U32) retval );
}

#endif  /* RTP_TEST_HARNESS */

// setupDecStructure - Initialize decoding structure before calling H261Decode/H263RtpDecode
static int setupDecStructure( H261Decoder *dec,
                              U32 PBframeCap,
                              PICTURE far *far32_newPic,
                              int *oldFormat
                              )
{
    // Set coding method, and PB capability
    dec->pic_layer.decodingInProgress = FALSE;
    dec->codingMethod   = H263_CODING;
    dec->PBframeCap     = PBframeCap;    // Enable B-frame reconstruction
    dec->maxComp  = 0;    // No time-out
    //dec->maxComp  = 100;    // Set time-out limit
    // Set oldFormat
    *oldFormat = dec->decMB.format;
    // Initialize structures according to color parameter passed in.
    dec->newOut.color = far32_newPic->color;
    dec->oldOut.color = far32_newPic->color;
    dec->B_Out.color  = far32_newPic->color;
    return OK;
}


// juggleFrames - Rename frames after completing decoding of a picture
static int juggleFrames( H261Decoder *dec,
                         int oldFormat,
                         int decStatus,
                         PICTURE far *far32_newPic
                         )
{
    PICTURE         temp_pic;
    static int      StaticOldColor=1;
    int             resetChroma = 0;
    int             retval;
    retval = decStatus;
    /* check to see if chroma needs to be cleared  */
    if ( dec->newOut.color == FALSE && StaticOldColor == TRUE )
        resetChroma = 1;
    StaticOldColor = dec->newOut.color;
//    if ( ( oldFormat == CIF && dec->formatCap == QCIF ) && ( dec->newOut.color == FALSE ) )
    if ( (oldFormat == CIF && dec->decMB.format == QCIF) && (dec->newOut.color == FALSE) )
        resetChroma = 1;
    /* reset the chroma image components if necessary   */
    if ( resetChroma ) {
        clearChromaFrame (&dec->newOut);
        clearChromaFrame (&dec->B_Out);
        clearChromaFrame (&dec->oldOut);
        clearChromaFrame (&dec->prevOldOut);
    }

    // If retval is YES, then the decoder has completed decoding of an entire frame.
    // The new frame becomes the old.  Aliases are set up for return.
    // For PB-frames, return the B-frame first, and the P-frame on next call.
    // Doublebuffered output: prevOldOut holds previous output frame
    if ( retval == YES ) {
        temp_pic = dec->prevOldOut;         // Reuse oldest frame
        if ( dec->PBframeCap  &&  dec->pic_layer.PBframeMode ) {
            // We have reconstructed B-frame and P-frame
            dec->prevOldOut = dec->B_Out;   // Will become PrevOut after next call
            dec->B_Out      = temp_pic;     // Reuse oldest frame
            temp_pic        = dec->oldOut;
            dec->oldOut     = dec->newOut;  // New P-frame becomes old
            dec->newOut     = temp_pic;     // Reuse prev. output (after it's been displayed)
            dec->pPrevPic = &dec->newOut;       // Old P-frame (reuse after display)
            dec->pNewPic  = &dec->prevOldOut;   // Return B-frame
            dec->pendingFrame = 1;              // Hold on to P-frame
        } else {
            dec->prevOldOut = dec->oldOut;  // Previous output frame
            dec->oldOut     = dec->newOut;  // New P-frame becomes old
            dec->newOut     = temp_pic;     // Reuse oldest frame
            dec->pPrevPic   = &dec->prevOldOut; // Previous output frame
            dec->pNewPic    = &dec->oldOut;     // Output frame
            dec->pendingFrame = 0;              // No frame left awaiting output
        }
        *far32_newPic  = *dec->pNewPic;
        //*far32_prevPic = *dec->pPrevPic;
    } else if ( dec->pendingFrame ) {
        // P-frame is intact even if next frame is being decoded
        dec->pPrevPic  = dec->pNewPic;      // B-frame was previous output
        dec->pNewPic   = &dec->oldOut;      // Return P-frame
        *far32_newPic  = *dec->pNewPic;
        //*far32_prevPic = *dec->pPrevPic;
        dec->pendingFrame = 0;          // No frame left awaiting output
        retval = YES;                   // We have a frame to return
    }
    // Report to higher layer whether we are returning a B-frame

    /*if ( retval == YES ) {

          // Exchange pointers to newOut and oldOut
        temp_pic    = dec->oldOut;
        dec->oldOut  = dec->newOut; // This is the output P (or I) frame
        dec->newOut  = temp_pic;
        if ( dec->PBframeCap  &&  dec->pic_layer.PBframeMode ) {
            // We have reconstructed a B-frame
            *far32_newPic = dec->B_Out; // Return B-frame
            dec->pendingFrame = 1;      // Hold on to P-frame
        } else {
            *far32_newPic = dec->oldOut;    // Return P (or I) frame
            dec->pendingFrame = 0;          // No frame left awaiting output
        }
    } else if ( dec->pendingFrame ) {
        // P-frame is intact even if next frame is being decoded
        *far32_newPic = dec->oldOut;    // Return P (or I) frame
        dec->pendingFrame = 0;          // No frame left awaiting output
        retval = YES;                   // We have a frame to return
    }*/

    // Return status; YES = completed decoding a picture, otherwise N0.
    return ( (U32) retval );
}

/*-----------------------------------------------------------------------------
 *  Function:   VvDecGetMBMap
 *
 *  DESCRIPTION
 *      Get decoder macroblock map.
 *
 *  CALLING SYNTAX
 *      VvDecGetMBMap   S16 index
 *                      U32 mapType
 *                      U32 outMap
 *
 *      index:      Input. Indicates which decoder is being called.
 *      mapType:    Input. Indicates which map to ouput.
 *      outMap:     Output. 16:16 pointer to output map.
 * 
 *  Author:         Mary Deshon             02/23/94
 *  Inspected:      <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap )
#else
__declspec(dllexport) U32 FAR PASCAL VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap )
#endif
{
    H261Decoder *dec;
    U16 far *fpOutMap;
/*    int   mapHorz = 11;
    int mapVert = 9;*/
  
    dec = DecoderRegistry[index-1];
   
    // Get a pointer we can work with
    fpOutMap = (U16 far * )makeFlat32( (void *)outMap );
  
/*  if (dec->decMB.format == CIF) { 
        mapHorz = 22;
        mapVert = 18;
    }*/
    
    switch ( mapType ) {
    case DECODER_MAP:
        /* Copy map to pointed at area */
        _fmemcpy ( (void far *)fpOutMap, (void far *)&dec->decMB, sizeof (MBMap));
        break;
    default:
        break;
    }
    return ( OK );    

}


#ifdef RTP_SUPPORT

// H263PacketDecode - Decode an RTP packet containing H.263 data
extern U32 H263PacketDecode(
            H261Decoder *dec,   // (I/O) Decoder structure created by VvOpenDecoder
                                //  Keeps the decoder's internal state and returns
                                //  information useful to higher level routines.
                                //  The following returned info is of particular interest:
                                //  dec->status     Decoder stats (first bad GOB etc)
                                //  dec->pic_layer  Picture layer info
                                //  dec->pNewPic    Pointer to most recent output frame
                                //  dec->pPrevPic   Pointer to previous output frame
                                //  dec->pendingFrame   Set to 1 when returning B-frame,
                                //                      otherwise 0.
            int headerLength,   // (Input) Number of bytes in h263header
            char *h263header,   // (Input) H.263 payload header according to RFC2190
            U32 PBframeCap,     // (Input) If set, decoder will reconstruct B-frames
            PICTURE *newPic,    // (I/O) On input, "color" setting controls whether
                                //  chrominance is reconstructed.
                                //  On output, newPic contains a copy of the PICTURE
                                //  structure pointed to by dec->pNewPic
            int payloadLength,  // (Input) Number of bytes in payloadData
            char *payloadData   // (Input) H.263 bitstream
            )
{
    int status, skipLsb, retval, oldFormat;

    status = setupDecStructure( dec, PBframeCap, newPic, &oldFormat );
    if (status != OK) return BAD_PARMS;
    // Set up bitstream pointers
    dec->bsStart.byteptr = (unsigned char *)payloadData;
    dec->bsStart.bitptr = (h263header[0] >> 3) & 0x7;
    dec->bsEnd.byteptr = (unsigned char *)payloadData + payloadLength;
    dec->bsEnd.bitptr = 0;
    skipLsb = h263header[0] & 0x7;
    if (skipLsb > 0) {
        --dec->bsEnd.byteptr;
        dec->bsEnd.bitptr = 8 - skipLsb;
    }
    
    // Call Decoder
    retval = H263RtpDecode( dec, h263header, 0 );
    
    retval = juggleFrames( dec, oldFormat, retval, newPic );

    // Return status; YES = completed decoding a picture, otherwise NO
    return ( (U32) retval );
}

// H263_1998PacketDecode - Decode an RTP packet containing H.263 data
extern U32 H263_1998PacketDecode(
            H261Decoder *dec,   // (I/O) Decoder structure created by VvOpenDecoder
                                //  Keeps the decoder's internal state and returns
                                //  information useful to higher level routines.
                                //  The following returned info is of particular interest:
                                //  dec->status     Decoder stats (first bad GOB etc)
                                //  dec->pic_layer  Picture layer info
                                //  dec->pNewPic    Pointer to most recent output frame
                                //  dec->pPrevPic   Pointer to previous output frame
                                //  dec->pendingFrame   Set to 1 when returning B-frame,
                                //                      otherwise 0.
            int headerLength,   // (Input) Number of bytes in h263header
            char *h263header,   // (Input) H.263 payload header according to RFC2429
            U32 PBframeCap,     // (Input) If set, decoder will reconstruct B-frames
            PICTURE *newPic,    // (I/O) On input, "color" setting controls whether
                                //  chrominance is reconstructed.
                                //  On output, newPic contains a copy of the PICTURE
                                //  structure pointed to by dec->pNewPic
            int payloadLength,  // (Input) Number of bytes in payloadData
            char *payloadData   // (Input) H.263 bitstream
            )
{
    int status = OK, retval, oldFormat;

    if (!dec->pic_layer.decodingInProgress)
	status = setupDecStructure( dec, PBframeCap, newPic, &oldFormat );
    if (status != OK) return BAD_PARMS;
    // Set up bitstream pointers
    dec->bsStart.byteptr = (unsigned char *)payloadData;
    dec->bsStart.bitptr = 0;
    dec->bsEnd.byteptr = (unsigned char *)payloadData + payloadLength;
    dec->bsEnd.bitptr = 0;
    if (h263header[0] & 0x04) {
	// Copy the header and re-insert the 0000 from the start code
	char pHdr[128];

	memcpy(pHdr, h263header, headerLength);
	dec->bsStart.byteptr -= 2;
	dec->bsStart.byteptr[0] = 0x0;
	dec->bsStart.byteptr[1] = 0x0;
	if ((dec->bsStart.byteptr[2]&0xfc)==0x80) {
	    // new picture -- insure gob, mba are reset
/*
	    if (dec->next_gob != 0 || dec->next_mba != 0) {
		fprintf(stderr, "H263 new picture, ngob=%d, nmba=%d \n",
				dec->next_gob, dec->next_mba);
	    }
*/
	    dec->next_gob = 0;
	    dec->next_mba = 0;
	}
	dec->bsAbsEnd.byteptr = 0;	// forget any saved data
	dec->bsAbsEnd.bitptr = 0;	// forget any saved data
/*
	fprintf(stderr, "H263 P=1, pLen=%d, pic=%d, ngob=%d, nmba=%d\n",
		(((pHdr[0]&0x01)<<5)|((pHdr[1]&0xf8)>>3)),
		((dec->bsStart.byteptr[2]&0xfc)==0x80),
		dec->next_gob, dec->next_mba);
*/
	// Call Decoder
	retval = H263RtpDecode( dec, pHdr, 1 );
	if (dec->pic_layer.decodingInProgress)
	    return NO;

	retval = juggleFrames( dec, oldFormat, retval, newPic );

	// Return status; YES = completed decoding a picture, otherwise NO
	return ( (U32) retval );
    }

    /*
     * This is a follow-on packet
     * Need to decode as continuation of the previous GOB or slice
     */
 
/*
    fprintf(stderr, "H263 P=0, pLen=%d, pic=%d, ngob=%d, nmba=%d\n",
		(((h263header[0]&0x01)<<5)|((h263header[1]&0xf8)>>3)),
		((dec->bsStart.byteptr[0]==0x00) &&
			(dec->bsStart.byteptr[1]==0x00) &&
			((dec->bsStart.byteptr[2]&0xfc)==0x80)),
		dec->next_gob, dec->next_mba);
*/
    if (dec->bsAbsEnd.byteptr != 0) {
	// Part of Mb left over from previous payload
	int nbytes = dec->bsAbsEnd.byteptr - dec->bsAbs.byteptr;
/*
	fprintf(stderr, "\tconcatenating %d bytes with %d\n",
				payloadLength, nbytes); 
*/
	memcpy(dec->bsAbsEnd.byteptr, payloadData, payloadLength);
	dec->bsEnd.byteptr = dec->bsAbsEnd.byteptr + payloadLength;
	dec->bsEnd.bitptr = 0;
	payloadLength += nbytes;
	dec->bsStart = dec->bsAbs;
	dec->bsAbsEnd.byteptr = 0;	// forget any saved data
	dec->bsAbsEnd.bitptr = 0;	// forget any saved data
    }
/*
    else {
        // int nbytes = dec->bsEnd.byteptr - dec->bsStart.byteptr;
    }
*/
    // Call Decoder
    retval = H263RtpDecode( dec, h263header, 1 );
    if (dec->pic_layer.decodingInProgress)
	return NO;

    retval = juggleFrames( dec, oldFormat, retval, newPic );

    // Return status; YES = completed decoding a picture, otherwise NO
    return ( (U32) retval );
}


// H263FinishPicture - Conceal remainder of H.263 picture
extern U32 H263FinishPicture(
            H261Decoder *dec,   // (I/O) Decoder structure created by VvOpenDecoder
                                //  Keeps the decoder's internal state and
                                //  returns information useful to higher level
                                //  routines, in particular:
                                //  dec->pNewPic    Pointer to most recent output frame
                                //  dec->pPrevPic   Pointer to previous output frame
                                //  dec->pendingFrame   Set to 1 when returning B-frame,
                                //                      otherwise 0.
            PICTURE *newPic     // (I/O) On input, "color" setting controls whether
                                //  chrominance is reconstructed.
                                //  On output, newPic contains a copy of the PICTURE
                                //  structure pointed to by dec->pNewPic
            )
{
    int retval, oldFormat;

    // Set oldFormat
    oldFormat = dec->decMB.format;
    // Conceal remainder of picture
    retval = H263ConcealRest( dec );

    retval = juggleFrames( dec, oldFormat, retval, newPic );

    // Return status; YES = completed a picture.  NO if we hadn't started a picture.
    return ( (U32) retval );
}

#endif  /* RTP_SUPPORT */
