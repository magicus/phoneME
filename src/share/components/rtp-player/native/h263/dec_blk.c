/*-----------------------------------------------------------------------------
 *  DEC_BLK.C
 *
 *  DESCRIPTION
 *      dec_blk.c - originally a function named decode_block; a optimized assembly
 *                  has been written for this C routine.  Please refer to 
 *                  dec_blk.asm for more details.
 *
 *      Author:     Gene Su         4/26/94
 *      Inspector:  
 *      Revised:
 *      09/23/97    dblair      Changed wsprintf() back to sprintf()
 *      04/19/96    wolfe       Fixed 2 warnings for Win3.1 WATCOM 10.0a
 *      12/21/95    wolfe       Moved code around to remove compile warning
 *                              Changed SPRINTF() calls to WSPRINTF()
 *      12/18/95    md          Remove "#include vvenc.h"
 *      05/10/94    G Su        Replaced ERROR with H261_ERROR (bug #315)
 *      05/09/94    G Su        Introduced checksym1 and made it static.
 *
 *  (c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#include <stdio.h>
#include "machine.h"
#include <stdlib.h>	// For exit function

#include "dllindex.h"
#include "h261defs.h"
#include "vldstate.h"
#include "h261func.h"
#include "vscodes.h"
#include "dec_blk.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

extern S16 Recon [QUANT_MAX - QUANT_MIN + 1] [N_SYM_INDICES];

//#define BS_ERR_EXIT(a)  a     // Terminate on bitstream error (i.e., illegal 
                                // bitstream syntax) to simplify debugging
#define BS_ERR_EXIT(a)          // Handle bitstream errors without terminating 
                                // (to cope with a real channel)

#define CHECKSYM_ON             // To avoid unreferenced params

#ifdef CHECKSYM_ON
#define CHECKSYM(a)     a       // Check symbol types to verify decoder state tables
#else
#define CHECKSYM(a)             // Don't check symbol types
#endif

#ifdef CHECKSYM_ON
static int  checksym1( SYMBOL sym, int type, char routine[] )
{
    char msg[120], csym[80], ctype[80];
    
    UNREFERENCED_PARAMETER( routine );

    if (sym.type == type) {
        return (OK);
    } else {    // Not expected type
        sprintsym( sym , csym );
        sprinttype( type, ctype );
        sprintf( msg, "%s: Encountered %s  Expected %s", routine, csym, ctype);
        H261ErrMsg( msg );
        return (H261_ERROR);
    }
}
#endif


extern int decode_block( SYMBOL insym[], int * nextsym,
                            BLOCK_DESCR * block, SYMBOL outsym[])
{
    int isym, zzpos, run;
CHECKSYM(char msg[120]);

    isym = 0;
    zzpos = 0;
    //printf("decode_block first: "); printsym( insym[*nextsym] ); printf("\n");
    while (insym[*nextsym].type != SYM_EOB) {
        if (insym[*nextsym].type == SYM_ESC_RUN) {
            run = insym[(*nextsym)++].value;
            outsym[isym].type = run;
#if defined(__WATCOMC__) && !defined(WIN95_VXD)
            CHECKSYM( if (checksym1( insym[*nextsym], SYM_ESC_LEVEL, "decode_block") != OK) return(0); )
#else
            CHECKSYM( if (checksym1( insym[*nextsym], SYM_ESC_LEVEL, "decode_block") != OK) exit(0); )
#endif
            outsym[isym++].value = insym[(*nextsym)++].value;
        } else {
            run = insym[*nextsym].type;
            CHECKSYM( if (run < 0) {
                sprintf( msg, "PROGRAM ERROR: run = %d in decode_block", run);
                H261ErrMsg( msg );
                return( H261_ERROR );
                } )
            outsym[isym++] = insym[(*nextsym)++];
        }
        zzpos += run + 1;
        //printf("decode_block next: "); printsym( insym[*nextsym] );
        //printf("   zzpos = %d\n", zzpos);
        if (zzpos > 64) {   // If we decoded coeff. 63, we will now have zzpos=64
            BS_ERR_EXIT( sprintf( msg, "decode_block: Bitstream error, zzpos=%d", zzpos);
                    H261ErrMsg( msg ); )
            return (H261_ERROR);
        }
    }
    (*nextsym)++;   /* Advance pointer to symbol after EOB */
    block->sym = outsym;
    block->nsym = isym;
    return (isym);
}

