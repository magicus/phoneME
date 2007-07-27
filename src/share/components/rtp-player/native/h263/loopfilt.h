/*-----------------------------------------------------------------------------
 *  LOOPFILT.H
 *
 *  DESCRIPTION   
 *      Necessary pragmas and function prototypes for LOOPFILT.asm.
 *
 *  Author:     Gene Su			    5/18/94
 *  Inspector:  
 *  Revised:
 *  12/21/95    wolfe   Wrapped PRAGMA AUX with WATCOM ifdefs
 *
 *  (c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/
#ifndef	_INC_LOOPFILT
#define	_INC_LOOPFILT

//	for the loopfilt8 routine in predsel.c

extern void loopfilt8( PIXEL input[], int xdim, PIXEL output[] );
#if defined(__WATCOMC__)
#pragma aux loopfilt8 parm [eax ebx ecx edx];
#endif
extern int  LoopFilter( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic );

#endif
