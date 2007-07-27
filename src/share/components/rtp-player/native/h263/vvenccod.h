/*-----------------------------------------------------------------------------
 *  VVENCCOD.H
 *
 *  MODIFICATIONS:
 *  01/17/97    dblair  Added VvEncoderReset() to reinitialize encoder state
 *  12/31/96    se      Added argument to DecPICTUREToDIB to control doublebuffering
 *                      Added arg to VvDecode to control B-frame reconstruction
 *  10/30/96    se      Support PB-frames
 *  04/19/96    wolfe   Rearranged defines to work win WATCOM 10.0a for Win16
 *
 *      (c) 1993-1997, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#ifndef _INC_VVENCCOD_
#define _INC_VVENCCOD_      1

#if !defined(__WATCOMC__)
#define _MSVC_CODEC_
#endif

#include "machine.h"
#include "h261type.h" 

/* Function Protos      */
#if defined(__WATCOMC__)

U32 FAR PASCAL VvOpenEncoder ( S16 format );
U32 FAR PASCAL VvGetEncoderPtr ( S16 index );
U32 FAR PASCAL VvSetEncoderParams ( S16 index, U32 encParms);
U32 FAR PASCAL VvSetPreviewParams ( S16 index, U32 prevParms);
U32 FAR PASCAL VvPreProcess ( S16 index, U32 frmParams, S32 bright, S32 lBframe);
PICTURE * FAR PASCAL VvGetPreprocessedPicPtr( S16 index, BOOL bBframe );
U32 FAR PASCAL VvEncode ( S16 index, U32 frmParams);
U32 FAR PASCAL VvRecon ( S16 index );
U32 FAR PASCAL VvBchEncode ( S16 index, U32 inStart, U32 outStart, U16 numBlks );
U32 FAR PASCAL VvCloseEncoder ( S16 index );
void FAR PASCAL VvResetEncoder( S16 hEnc, S16 formatCap );

U32 FAR PASCAL VvOpenDecoder ( S16 format );
U32 FAR PASCAL VvGetDecoderPtr ( S16 index );
U32 FAR PASCAL VvDecode ( U32 pdec, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs );
              
U32 FAR PASCAL VvBchDecode ( S16 index, U32 sync, U32 syncPhase, S32 maxSyncErrs,
                 S32 correctFlag, U32 numBlks, U32 numErrs, 
                 U32 bsBegin, U32 bsEnd, U32 bsNext, U32 bsOut );
              
              
U32 FAR PASCAL VvCloseDecoder (S16 index );
U32 FAR PASCAL VvGetEncoderStats ( S16 index, U32 mbInfo, U32 frameInfo );
U32 FAR PASCAL EncPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat, U32 selectiveUpdates);
#ifdef FOR_MAC
U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat,
                                 U32 selectiveUpdates, U32 prevPic, U16 bytesPerRow );
#else
U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat,
                                 U32 selectiveUpdates, U32 prevPic );
#endif
U32 FAR PASCAL DecAndScalePICTUREToDIB ( S16 index, U32 outDIB, U32 dibRowBytes, 
                                         U32 srcWidth, U32 srcHeight,
                                         U32 dstWidth, U32 dstHeight, U32 prevPic );
U32 FAR PASCAL VvEncGetMBMap ( S16 index, U32 mapType, U32 outMap );
U32 FAR PASCAL VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap );

#elif defined(_MSVC_CODEC_)
 
__declspec(dllexport) U32 FAR PASCAL VvOpenEncoder ( S16 format );
__declspec(dllexport) U32 FAR PASCAL VvGetEncoderPtr ( S16 index );
__declspec(dllexport) U32 FAR PASCAL VvSetEncoderParams ( S16 index, U32 encParms);
__declspec(dllexport) U32 FAR PASCAL VvSetPreviewParams ( S16 index, U32 prevParms);
__declspec(dllexport) U32 FAR PASCAL VvPreProcess ( S16 index, U32 frmParams, S32 bright,
                                                    S32 lBframe);
__declspec(dllexport) PICTURE * FAR PASCAL VvGetPreprocessedPicPtr( S16 index, BOOL bBframe );
__declspec(dllexport) U32 FAR PASCAL VvEncode ( S16 index, U32 frmParams);
__declspec(dllexport) U32 FAR PASCAL VvRecon ( S16 index );
__declspec(dllexport) U32 FAR PASCAL VvBchEncode ( S16 index, U32 inStart, U32 outStart, U16 numBlks );
__declspec(dllexport) U32 FAR PASCAL VvCloseEncoder ( S16 index );
__declspec(dllexport) void FAR PASCAL VvResetEncoder( S16 hEnc, S16 formatCap );

__declspec(dllexport) U32 FAR PASCAL VvOpenDecoder ( S16 format );
__declspec(dllexport) U32 FAR PASCAL VvGetDecoderPtr ( S16 index );
__declspec(dllexport) U32 FAR PASCAL VvDecode ( U32 pdec, U32 PBframeCap, U32 picdesc,
                          U32 bsStart, U32 bsEnd, U32 nextGOB, U32 newPic, 
                          U32 status, U32 newBs );
              
__declspec(dllexport) U32 FAR PASCAL VvBchDecode ( S16 index, U32 sync, U32 syncPhase, S32 maxSyncErrs,
                 S32 correctFlag, U32 numBlks, U32 numErrs, 
                 U32 bsBegin, U32 bsEnd, U32 bsNext, U32 bsOut );
              
              
__declspec(dllexport) U32 FAR PASCAL VvCloseDecoder (S16 index );
__declspec(dllexport) U32 FAR PASCAL VvGetEncoderStats ( S16 index, U32 mbInfo, U32 frameInfo );
__declspec(dllexport) U32 FAR PASCAL EncPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat, U32 selectiveUpdates);
#ifdef FOR_MAC
__declspec(dllexport) U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat,
                                 U32 selectiveUpdates, U32 prevPic, U16 bytesPerRow );
#else
__declspec(dllexport) U32 FAR PASCAL DecPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat,
                                 U32 selectiveUpdates, U32 prevPic );
#endif
__declspec(dllexport) U32 FAR PASCAL DecAndScalePICTUREToDIB ( S16 index, U32 outDIB,  U32 dibRowBytes,
                                         U32 srcWidth, U32 srcHeight,
                                         U32 dstWidth, U32 dstHeight, U32 prevPic );
__declspec(dllexport) U32 FAR PASCAL VvEncGetMBMap ( S16 index, U32 mapType, U32 outMap );
__declspec(dllexport) U32 FAR PASCAL VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap );

#else

__declspec(dllexport) U32 FAR PASCAL _VvOpenEncoder ( S16 format );
__declspec(dllexport) U32 FAR PASCAL _VvGetEncoderPtr ( S16 index );
__declspec(dllexport) U32 FAR PASCAL _VvSetEncoderParams ( S16 index, U32 encParms);
__declspec(dllexport) U32 FAR PASCAL _VvSetPreviewParams ( S16 index, U32 prevParms);
__declspec(dllexport) U32 FAR PASCAL _VvPreProcess ( S16 index, U32 frmParams, S32 bright,
                                                     S32 lBframe);
__declspec(dllexport) PICTURE * FAR PASCAL VvGetPreprocessedPicPtr( S16 index, BOOL bBframe );
__declspec(dllexport) U32 FAR PASCAL _VvEncode ( S16 index, U32 frmParams);
__declspec(dllexport) U32 FAR PASCAL _VvRecon ( S16 index );
__declspec(dllexport) U32 FAR PASCAL _VvBchEncode ( S16 index, U32 inStart, U32 outStart, U16 numBlks );
__declspec(dllexport) U32 FAR PASCAL _VvCloseEncoder ( S16 index );
__declspec(dllexport) void FAR PASCAL _VvResetEncoder( S16 hEnc, S16 formatCap );

__declspec(dllexport) U32 FAR PASCAL _VvOpenDecoder ( S16 format );
__declspec(dllexport) U32 FAR PASCAL _VvGetDecoderPtr ( S16 index );
__declspec(dllexport) U32 FAR PASCAL _VvDecode ( U32 pdec, U32 picdesc, U32 bsStart,
              U32 bsEnd, U32 nextGOB, U32 newPic, 
              U32 status, U32 newBs );
              
__declspec(dllexport) U32 FAR PASCAL _VvBchDecode ( S16 index, U32 sync, U32 syncPhase, S32 maxSyncErrs,
                 S32 correctFlag, U32 numBlks, U32 numErrs, 
                 U32 bsBegin, U32 bsEnd, U32 bsNext, U32 bsOut );
              
              
__declspec(dllexport) U32 FAR PASCAL _VvCloseDecoder (S16 index );
__declspec(dllexport) U32 FAR PASCAL _VvGetEncoderStats ( S16 index, U32 mbInfo, U32 frameInfo );
__declspec(dllexport) U32 FAR PASCAL _EncPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat, U32 selectiveUpdates);
__declspec(dllexport) U32 FAR PASCAL _DecPICTUREToDIB ( S16 index, U32 outDIB, U32 dibFormat, U32 selectiveUpdates);
__declspec(dllexport) U32 FAR PASCAL _DecAndScalePICTUREToDIB ( S16 index, U32 outDIB, U32 dibRowBytes, U32 srcWidth, U32 srcHeight,
                                         U32 dstWidth, U32 dstHeight, U32 prevPic );
__declspec(dllexport) U32 FAR PASCAL _VvEncGetMBMap ( S16 index, U32 mapType, U32 outMap );
__declspec(dllexport) U32 FAR PASCAL _VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap );

#endif

#endif
