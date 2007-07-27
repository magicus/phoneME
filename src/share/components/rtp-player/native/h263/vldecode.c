/*-----------------------------------------------------------------------------
 *	VLDECODE.C
 *
 *	DESCRIPTION
 *		vldecode.c - originally a function named vldecode in vld.c; an optimized 
 *					assembly routine has been written for this C routine.  Please 
 *					refer to vldecode.asm for more details.  Please also note that
 *					IncBsPtr2 is also included in this file (originally IncBsPtr
 *					in vld.c).  Without having IncBsPtr2 in this file, the compiler 
 *					will not inline the function.
 *
 *      Author:     Gene Su    		8/22/94
 *      Inspector:  
 *      Revised:
 *
 *	(c) 1994, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "vldstate.h"
#include "vldecode.h"


// forward definition of function
void IncBsPtr2( BS_PTR * bs, int incr);


//  Return status
#define OUT_OF_BITS         0

//  VLDecode - Decode bitstream
extern int pascal VLDECODE(   BS_PTR bs,          // Bitstream pointer */
                        int numbits,        // Max # bits to decode */
                        int * state,        // Initial decoder state;
                                            // returns updated state
                        int * parsed_bits,  // Returns # decoded bits */
                        int * nsym,         // Returns # decoded symbols */
                        SYMBOL sym[],       // Returns nsym symbols */
                        int maxsym          // Dimension of sym array
)
{
    register DECTABENTRY * entry;
    char msg[120];

    //printf( "Entered VLDecode: bitstream = %2x %2x %2x %2x  bitptr = %d\n",
    //        *bs.byteptr, *(bs.byteptr + 1), *(bs.byteptr + 2),
    //        *(bs.byteptr + 3), bs.bitptr);
            
    *parsed_bits = 0, *nsym = 0;
    while (*parsed_bits < numbits  &&  *nsym < maxsym) {
        entry = &dectable [selectdectab[ *state ]] [Get8Bits( bs )];
        while (entry->bits < 0) {   /* Long codeword; sym.value indicates table */
            *parsed_bits -= entry->bits;
            IncBsPtr2( &bs, -entry->bits);
            *state += entry->statechange;
            //printf("VLDecode: "); printsym( entry->sym );
            //printf("  State: "); printstate( *state ); printf("\n");
            entry = &dectable [ entry->sym.value ] [Get8Bits( bs )];
        }
        *parsed_bits += entry->bits;
        IncBsPtr2( &bs, entry->bits);
        *state += entry->statechange;
/*        {
            int input;
            printf("VLDecode: "); printsym( entry->sym );
            printf("  State: "); printstate( *state );
            printf("  Cont? (<0 to exit): ");
            scanf("%d", &input);
            if (input < 0) exit(0);
        }*/
        if (entry->sym.type  ==  SYM_EXIT) { /* Premature exit */
            if (*parsed_bits > numbits) {
                return (OUT_OF_BITS);
            } else {
                return (entry->sym.value);
            }
        }
        sym [(*nsym)++] = entry->sym;
    }
    if (*nsym >= maxsym) {
        sprintf( msg, "VLDecode: Symbol array overrun");
        H261ErrMsg( msg );
        return( H261_ERROR );
    }
    if (*parsed_bits > numbits) {
        return (OUT_OF_BITS);
    }
    return( OK );
}

void IncBsPtr2( BS_PTR * bs, int incr)
{
    bs->bitptr += incr;
    while (bs->bitptr > 7) {
        ++(bs->byteptr);
        bs->bitptr -= 8;
    }
    while (bs->bitptr < 0) {
        --(bs->byteptr);
        bs->bitptr += 8;
    }
    return;
}
