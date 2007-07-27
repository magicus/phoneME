/*-----------------------------------------------------------------------------
 *  CLIP.H
 *
 *  DESCRIPTION   
 *      Necessary pragmas and function prototypes for the clipping
 *		mechanism in Idct2 and Idct2Sum.
 *
 *  Author:     Gene Su			    02/02/95
 *  Inspector:  
 *  Revised:
 *  12/21/95    wolfe   Wrapped PRAGMA AUX with WATCOM ifdefs
 *	02/06/95	G. Su	Added 'idct_class' to the clipping function
 *						prototypes.
 *  (c) 1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/
#ifndef	_INC_CLIP
#define	_INC_CLIP

#define DC_ONLY	1
#define	DC_AC_H 2
#define DC_AC_V 3
#define DC_3	4
#define GENERAL 5

extern void idct2_clip(PIXEL x[], 
						int xdim, S32 idct_out[8][4], int idct_class);
#if defined(__WATCOMC__)
#pragma aux idct2_clip parm [eax ebx ecx edx];
#endif

extern void idct2sum_clip(PIXEL x[], 
						int xdim, S32 idct_out[8][4], int idct_class);
#if defined(__WATCOMC__)
#pragma aux idct2sum_clip parm [eax ebx ecx edx];						
#endif

#endif
