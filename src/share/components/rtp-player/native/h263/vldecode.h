/*-----------------------------------------------------------------------------
 *	VLDECODE.H
 *
 *	DESCRIPTION
 *		vldecode.h - Necessary pragmas, function prototypes, and declarations
 *				for both vldecode.asm (when DO_ASM_VLDECODE = 1) and vldecode.c. 
 *
 *	Author:		Gene Su				8/23/94
 *	Inspector:	
 *	Revised:
 *  12/21/95    wolfe       Wrapped PRAGMA AUX with WATCOM ifdefs
 *  07/07/95    S Ericsson  Removed unused defines
 *
 *	(c) 1994-1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#ifndef _INC_VLDECODE
#define _INC_VLDECODE

#ifndef DO_ASM_VLDECODE

typedef struct {
    SYMBOL		sym;
    S8			statechange;
    S8			bits;
	} DECTABENTRY;

#define DECTABBITS          8       /* Look up DECTABBITS bits at a time */
#define DECTABSIZE          256     /* 2 ** DECTABBITS */

#define MAX_STRING_VLD          (DECTABBITS + 4)

extern DECTABENTRY dectable[][DECTABSIZE];
extern U8 selectdectab[];

//extern void pascal INCBSPTR( BS_PTR * bs, int incr);
//#pragma aux INCBSPTR parm [eax ebx ecx edx];

#endif

#ifdef DO_ASM_VLDECODE

int overrun_err(void)
#if defined(__WATCOMC__)
#pragma aux overrun_err parm[];
#endif
{
	char msg[120];
	
	sprintf(msg, "VLDecode: Symbol array overrun");
	H261ErrMsg(msg);
	return (H261_ERROR);
}

#endif

extern int pascal VLDECODE	(   BS_PTR bs,          
                        int numbits,        
                        int * state,        
                        int * parsed_bits,  
                        int * nsym,         
                        SYMBOL sym[],      
                        int maxsym          
							);
#if defined(__WATCOMC__)
#pragma aux VLDECODE parm [eax ebx ecx edx];
#endif
		
#endif
