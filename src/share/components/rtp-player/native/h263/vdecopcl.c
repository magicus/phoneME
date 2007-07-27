/*-----------------------------------------------------------------------------
 *  VDECOPCL.C
 *
 *  DESCRIPTION
 *      Video Decoder Open/Close
 *      These functions are part of the dynamic link library DECODE32.DLL.
 *
 *  ASSUMPTIONS
 *      1. This is 32-bit code.
 *      2. All pointers which are passed into the DLL from the
 *          application are 16:16 pointers.
 *  
 *  DEFECTS
 *      1. If Open fails, memory that has already been allocated
 *          needs to be freed.
 *
 *  Author:     Mary Deshon     7/01/93
 *              John Wolfe     12/21/95
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 *  01/03/98    S Ericsson  Set next_gob=next_mba=0 when opening decoder (RTP support)
 *  02/22/97    dblair      When closing decoder free up memory from upsampling workspace
 *  01/13/97    se          Perform malloc/free of decoder variable size structures in 
 *                          external functions AllocVarStructsDecoder/FreeVarStructsDecoder
 *  12/19/96    se          Support doublebuffering in the decoder
 *                          Support multiple decoders, and decoding in "chunks"
 *  10/22/96    se          Support PB-frames
 *  05/15/96  deshon/wolfe  Made GAliasDec into arrray
 *  12/21/95    wolfe       Broke encoder/decoder into seperate DLLs
 *
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

/*  Header File Includes        */
#include "mni.h"
#include <stdio.h>
#include <stddef.h>
#include "machine.h"
#ifdef FOR_MAC
#include <stdlib.h>
#endif
#include "mni.h"

#include <string.h>         // _fmemset //shall use "memset" instead ?
//#include "vvdefs.h"
#include "dllindex.h"
#include "h261func.h"
#include "h261defs.h"
#include "vvcoder.h"
#include "vscodes.h"

/*  Defines, Typedefs, & Enums  */

//#define DEBUG_VVOPCL(a)   a
#define DEBUG_VVOPCL(a)

/*  Globals                     */
#ifdef _NTWIN

#define BreakPoint DebugBreak 

#else // ! _NTWIN
#ifdef __WATCOMC__
extern void BreakPoint( void );                 // Used for debugging the DLL
#pragma aux BreakPoint = 0xcc;
#endif
#endif // ! _NTWIN

H261Decoder     *DecoderRegistry[MAX_NUM_DECODERS] = { 0, 0 };
U32             GAliasDec[MAX_NUM_DECODERS] = { 0, 0 };

/*  File-wide Statics           */
static int      DecCount = 0;       // Number of decoders open.

static short    makeDecoder( short formatCap );
static void     *vvMalloc( size_t size );
static void		vvFree(void *ptr);

/*
 * Function:    GrayFill()
 * Purpose:     Set unused image areas to constant value.
 */
static void
GrayFill( PICTURE *pic, long lumaSize, long chrSize )
{
    U8  *y = pic->y.ptr;
    U8  *cb = pic->y.ptr + lumaSize;

    _fmemset ( y, 128, lumaSize ); //shall use "memset" instead ?
    _fmemset ( cb, 128, chrSize ); //shall use "memset" instead ?
}

/*-----------------------------------------------------------------------------
 *  Function:   VvOpenDecoder
 *
 *  DESCRIPTION
 *      Opens a decoder instance. Returns the decoder registry number
 *      or an error code.
 *
 *  NOTES:
 *      1. Encoder tables are static storage.
 *
 *  Author:     Mary Deshon     6/28/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvOpenDecoder( S16 formatCap )    
#else
__declspec(dllexport) U32 FAR PASCAL VvOpenDecoder( S16 formatCap )    
#endif
{
    H261Decoder   *dec; 
    short         i, index;
  
#ifdef WATD
  	//BreakPoint();
#endif

    // If this is the first open call, set up read-only tables
    // and initialize decoder registry 
    if (DecCount == 0 ) {
        InitFindSC();
        InitDecodeTable(); 
    
        for (i = 0; i < MAX_NUM_DECODERS; i++)
            DecoderRegistry[i] = NULL;
    }                 

    // Init Recon tables
    InitReconTables ();
    InitYUVToRGB ();

  
    // Do not exceed maximum number of decoders allowed
    if ( DecCount == MAX_NUM_DECODERS ) return( MAX_DECODERS_OPEN );
  
    // Try to make a decoder.
    // If successful, then return_value is an DecoderRegistry entry 
    //                  which is <= MAX_NUM_DECODERS
    // otherwise, return_value is > MAX_NUM_DECODERS and indicates
    //  an error code (see dllindex.h for codes)
    
    index = makeDecoder( formatCap );
  
    dec = DecoderRegistry[index-1];   // Do this so that I can look at struct with the Wild & Wacky 
                                    // Watcom Debugger
    return ( (U32) index );
}


/*-----------------------------------------------------------------------------
 *  Function:   VvGetDecoderPtr
 *
 *  DESCRIPTION
 *      Returns a bitStream pointer.
 *
 *  NOTES:
 *      1. Encoder tables are static storage.
 *
 *  Author:     Mary Deshon     9/22/94
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvGetDecoderPtr( S16 index )    
#else
__declspec(dllexport) U32 FAR PASCAL VvGetDecoderPtr( S16 index )    
#endif
{
#ifndef _OS2
    if ( DecoderRegistry[index-1] ) {
        GAliasDec[index-1] =  (U32)makeHugeAlias16( DecoderRegistry[index-1]->bsAbs.byteptr, MAX_BITSTR_SIZE );
        return ( (U32) GAliasDec[index-1] );
    }
    else return (U32)NULL;
#else
    if ( DecoderRegistry[index-1] != NULL ) {       
        GAliasDec[index-1] = (U32) DecoderRegistry[index-1]->bsAbs.byteptr;
        return ( (U32) GAliasDec[index-1] );
    }
    else return (U32)NULL;
#endif
}


/*-----------------------------------------------------------------------------
 *  Function:   VvCloseDecoder
 *
 *  DESCRIPTION
 *      Closes a decoder instance.
 *
 *  Author:     Mary Deshon     7/04/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
#ifdef _DECODER_IN_EXE
U32 FAR PASCAL VvCloseDecoder( short hDecf )    
#else
__declspec(dllexport) U32 FAR PASCAL VvCloseDecoder( short hDecf )    
#endif
{
  H261Decoder   *dec;
  short         hDec = hDecf - 1;

  dec  = DecoderRegistry[hDec];

  // Free memory associated with decoder
  if (DecCount > 0 ) {
    FreeVarStructsDecoder( dec );
    vvFree ( DecoderRegistry[hDec]->bsAbs.byteptr );
    
    myFreeHugeAlias16 ( GAliasDec[hDec], MAX_BITSTR_SIZE );
    GAliasDec[hDec] = 0;

    vvFree ( DecoderRegistry[hDec] );
    DecoderRegistry[hDec] = NULL;
    DecCount--;
    return ( NO_ERROR );
  }
  else return ( NO_DECODERS_OPEN );
}

/*-----------------------------------------------------------------------------
 *  Function:   AllocVarStructsDecoder
 *
 *  DESCRIPTION
 *      Malloc variable size structures for a decoder instance
 *      Returns zero if no errors occurred; otherwise, error code is returned
 *
 *  Author:     Staffan Ericsson    1/13/97
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/

#define MBNUM_QCIF  (9*11)
#define MBNUM_CIF   (4 * MBNUM_QCIF)
#define BITS_PER_SYMBOL (3)
#define MAXSYM_QCIF (65536 / BITS_PER_SYMBOL)
#define MAXSYM_CIF  (4 * MAXSYM_QCIF)

extern int AllocVarStructsDecoder( H261Decoder *dec, short formatCap_or_numMBs )
{
  long          imgSize, lumaSize, chromaLineLength, chromaRows, maxsym, memSize;
  short         numGOBs, numMBs, retval, formatCap;
  
  // Get image parms needed for memory allocations: imgSize, lumaSize, and numMBs
  if (formatCap_or_numMBs > MBNUM_QCIF) {   // Indicates non-standard image size
    numMBs = formatCap_or_numMBs;
    lumaSize = 256 * numMBs;
    imgSize  = 384 * numMBs;
  } else {
    formatCap = formatCap_or_numMBs;
    retval = getImgParms( formatCap, &numGOBs, &numMBs, &imgSize,
                        &lumaSize, &chromaLineLength, & chromaRows, &maxsym);
    if ( retval ) {
	numMBs = formatCap;
	lumaSize = 256* numMBs;
	imgSize = 384* numMBs;
	maxsym = imgSize;
    }

  }
  // Determine maxsym and formatCap
  if (numMBs <= MBNUM_QCIF) {
    maxsym = MAXSYM_QCIF;
  } else {
    maxsym = MAXSYM_CIF;
  }
  if (numMBs < MBNUM_CIF) {
    dec->formatCap = QCIF;
  } else {
    dec->formatCap = CIF;
  }
    // Allocate memory for frame store
  if ( ( dec->newOut.y.ptr = vvMalloc( imgSize ) ) == NULL )
    return( FAILED_MALLOC );
  if ( ( dec->B_Out.y.ptr  = vvMalloc( imgSize ) ) == NULL )
    return( FAILED_MALLOC );
  if ( ( dec->oldOut.y.ptr = vvMalloc( imgSize ) ) == NULL )
    return( FAILED_MALLOC );
  if ( ( dec->prevOldOut.y.ptr = vvMalloc( imgSize ) ) == NULL )
    return( FAILED_MALLOC );
    
  //GrayFill ( &dec->newOut, lumaSize, imgSize-lumaSize );
  GrayFill ( &dec->oldOut, lumaSize, imgSize-lumaSize );           
    
  // Allocate work arrays
  dec->maxMbnum = numMBs;
  memSize = numMBs * sizeof(MACROBLOCK_DESCR);
  if ( ( dec->mb = vvMalloc( memSize ) ) == NULL )
    return( FAILED_MALLOC );
  //dec->maxSym = lumaSize;    // Probably bigger than necessary
  //memSize = lumaSize * sizeof(SYMBOL);
  dec->maxSym = maxsym;
  memSize = maxsym * sizeof(SYMBOL);
  if ( ( dec->sym = vvMalloc( memSize ) ) == NULL )
    return( FAILED_MALLOC );

  // NULL out the upsampling workspace array - we mightn't need it at all
  _fmemset(&dec->convertPic, 0, sizeof(PICTURE)); //shall use "memset" instead ?

  dec->pendingFrame = 0; // No pending frame to be displayed
  return 0;     // Errorcode = 0
}


/*-----------------------------------------------------------------------------
 *  Function:   FreeVarStructsDecoder
 *
 *  DESCRIPTION
 *      Free variable size structures for a decoder instance.
 *
 *  Author:     Staffan Ericsson    1/13/97
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
extern void FreeVarStructsDecoder( H261Decoder *dec )
{
    vvFree ( dec->oldOut.y.ptr );
    dec->oldOut.y.ptr = NULL;
    vvFree ( dec->newOut.y.ptr );
    dec->newOut.y.ptr = NULL;
    vvFree ( dec->B_Out.y.ptr );
    dec->B_Out.y.ptr = NULL;
    vvFree ( dec->prevOldOut.y.ptr );
    dec->prevOldOut.y.ptr = NULL;
    vvFree ( dec->mb );
    dec->mb = NULL;
    dec->maxMbnum = 0;
    vvFree ( dec->sym );
    dec->sym = NULL;
    dec->maxSym = 0;

    if( dec->convertPic.y.ptr ) {
        MNI_FREE( dec->convertPic.y.ptr );
        dec->convertPic.y.ptr = NULL;
        dec->convertPic.cb.ptr = NULL;
        dec->convertPic.cr.ptr = NULL;
    }
}

/*-----------------------------------------------------------------------------
 *  Function:   makeDecoder
 *
 *  DESCRIPTION
 *      Make a decoder instance
 *
 *  Author:     Mary Deshon     7/01/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 *  11/15/93    J Bruder    Commented out unnecessary stuff.
 -----------------------------------------------------------------------------*/
static short makeDecoder( short formatCap)
{
  H261Decoder   *dec;
  int           i, retval;
  
    // Find an open slot in the DecoderRegistry, we know that there 
    // is at least one available              
  for (i = 0; DecoderRegistry[i]; i++) {};

    // Allocate memory for decoder structure
  if ( ( DecoderRegistry[i] = vvMalloc( sizeof ( H261Decoder ) ) ) == NULL )
    return( FAILED_MALLOC );
 
  dec = DecoderRegistry[i];                 // For looking at with the W2D
  dec->next_gob = dec->next_mba = 0;        // RTP support
  
    // Allocate memory for frame store and work arrays
  retval = AllocVarStructsDecoder( dec, formatCap );
  if (retval) return (short)retval;

    // Allocate memory for bitstream
  if ( ( DecoderRegistry[i]->bsAbs.byteptr = vvMalloc( MAX_BITSTR_SIZE ) ) == NULL )
    return( FAILED_MALLOC );
  DecoderRegistry[i]->bsAbs.bitptr = 0;
  
  DecCount++;
  return ( ++i );   // Decoders are numbered 1,2,...
}


/*-----------------------------------------------------------------------------
 *  Function:   VvMalloc
 *
 *  DESCRIPTION
 *      Wrapper for malloc.
 *
 *  ASSUMPTIONS
 *      1. Malloc returns quad-aligned pointers for all compilers.
 *
 *	CAUTION:
 *		There's a serious bug here! If a given implementation does not return
 *		longword-aligned data, then this function will return a long-aligned
 *		pointer but free won't free it because it won't be a recognized
 *		pointer. This will lead to a memory leak or a crash. I haven't fixed
 *		it because I'm not aware of any systems that don't return long-aligned
 *		data, so the problem won't occur. tkent, 8/20/96
 *
 *  Author:     Mary Deshon     7/01/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 *  02/15/94    M Deshon    Force quad-alignment.
 -----------------------------------------------------------------------------*/
static void *vvMalloc( size_t size )
{
    char    *ptr;
    U32     addr;

#ifdef FOR_MAC
    ptr = NewPtrClear(size+4);
#else
    ptr = (char *)MNI_CALLOC( size + 4, 1 ); /* calloc should zero memory */
    
#endif
    addr = (U32)ptr & 0x3L;
    
//   	assert(addr == 0);	// To trap the problem described above
   	
    /* Assume returned address can have low-order bits 00, 01, 10, or 11. */
    if ( addr == 1 ) ptr += 3;
    else if (addr == 2 ) ptr += 2;
    else if ( addr == 3 ) ptr += 1;
    else {}

    return ( (void *) ptr );
//  return ( (void *)MNI_CALLOC( size, 1 ) );
}

void vvFree(void *ptr)
{
#ifdef FOR_MAC
	DisposePtr(ptr);
#else
	MNI_FREE(ptr);
#endif
}

