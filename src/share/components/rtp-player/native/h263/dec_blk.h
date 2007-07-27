/*-----------------------------------------------------------------------------
 *  DEC_BLK.H
 *
 *  DESCRIPTION   
 *      Necessary pragmas and function prototypes for error checking in 
 *		DEC_BLK.asm.
 *
 *  Author:     Gene Su			    4/26/94
 *  Inspector:  
 *  Revised:
 *	5/5/94		G Su				Added zzpos_err to help defining error messages
 *									more clearly.
 *
 *  (c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/
#ifndef	_INC_DEC_BLK
#define	_INC_DEC_BLK

//	for the decode_block routine in h261dec.c

#ifndef DO_ASM_DECODE_BLOCK
extern int decode_block( SYMBOL insym[], int * nextsym,
			    BLOCK_DESCR * block, SYMBOL outsym[]);
#endif

#ifdef DO_ASM_DECODE_BLOCK
extern int decode_block( SYMBOL insym[], int * nextsym,
			    BLOCK_DESCR * block, SYMBOL outsym[]);
#pragma aux decode_block parm [eax ebx ecx edx];

void zzpos_err(int zzpos)
#pragma aux zzpos_err parm [eax];
{                              
	char msg[120];
	
	sprintf(msg, "decode_block: Bitstream error, zzpos=%d",zzpos); 
	H261ErrMsg(msg);
}

void checksym_run_err(int run)
#pragma aux checksym_run_err parm [ecx];
{                              
	char msg[120];
	
	sprintf(msg, "PROGRAM ERROR: run = %d in decode_block",run); 
	H261ErrMsg(msg);
}

void checksym_type_err(int type)
#pragma aux checksym_type_err parm [ecx];
{                              
	char msg[120];

	sprintf(msg, "decode_block:  Encountered level = %d  Expected Esc Level (0b2h)", type); 
	H261ErrMsg(msg);
}
#endif
#endif
