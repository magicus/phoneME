/*-----------------------------------------------------------------------------
 *  VLD.C
 *
 *  DESCRIPTION
 *      vld.c - Perform variable length decoding of H.261 bitstream
 *
 *  Author:     Staffan Ericsson    6/30/93
 *  Inspector:  Mary Deshon         7/6/93
 *  Revised:
 *  02/10/97 D Blair    Added decoding for H.263+
 *  12/21/95 wolfe      Fixed compiler warnings (MSVC 4.0)
 *  11/10/95 SE         Allow up to 7 zeros before H.263 startcodes
 *  07/10/95 S Ericsson Added decoding of H.263 bitstream
 *	08/23/94 G Su		Moved functions VLDECODE and INCBSPTR into vldecode.c
 *						vldecode.asm
 *  04/27/94 S Ericsson Replaced ERROR by H261_ERROR (bug #313)
 *  07/06/93 M Deshon	Changed to use S8, U8, ... ,S32, U32
 *  07/12/93 S Ericsson	Include "dllindex.h" instead of "h261type.h" 
 *  07/20/93 S Ericsson	Error messages displayed via H261ErrMsg
 *  07/26/93 S Ericsson	Removed "exit" calls
 *  10/29/93 M Deshon	Changed MAX_STRING to MAX_STRING_VLD (header file change)
 *  
 *  (c) 1993-1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "vldstate.h"
#include "vldtabs.h"
#include "vld.h"
#include "machine.h"
#include "vvutils.h"
#include "h263plus.h"

static int buildtable( int minBits, struct vlc_entry input[], DECTABENTRY output[DECTABSIZE]);
static int parse_bits(  char vlc[],     /* String with bit pattern */
                        int strmax,     /* Max characters in "vlc" excluding terminating null */
                        int maxbits,    /* Max # bits in codeword */
                        int * bits,     /* Returns # bits; 0 < bits < maxbits+1 */
                        int * codeword, /* Returns codeword */
                        int minBits     // Min # bits in codeword
);


//  Get8Bits - return next 8 bits from bitstream
extern U8 Get8Bits( BS_PTR bs)
{
    int k;

    k = (*bs.byteptr << 8) | *(bs.byteptr + 1);
    return (k >> (8 - bs.bitptr));
}

//  IncBsPtr - Increment (or decrement) bitstream pointer
extern void IncBsPtr( BS_PTR * bs, int incr)
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


// FLDecSymbol - Decode fixed length symbol of length "bits"
// Return values:   1   OK: successful decoding; symbol returned in "value"
//                  0   OUT_OF_BITS: consumed more than "numBits"; returning numBits < 0
extern int FLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int bits,           // Symbol length
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        int * value         // Returns decoded symbol
                        )
{
    *value = 0;
    while (bits > 8) {
        *value |= Get8Bits( *bs ) << (bits - 8);
        bits -= 8;
        *numBits -= 8;
        ++bs->byteptr;
    }
    *value |= Get8Bits( *bs ) >> (8 - bits);
    *numBits -= bits;       // Subtract parsed bits
    IncBsPtr( bs, bits);    // Advance bitpointer
    //printf("FLDecSymbol: value = %2x\n", *value );
    if (*numBits < 0) {
        return (OUT_OF_BITS);
    }
    return( OK );
}


// VLDecSymbol - Decode variable length symbol using dectable[tabNum]
// Return values:   1   OK: successful decoding; symbol returned in sym
//                  0   OUT_OF_BITS: consumed more than "numBits"; returning numBits < 0
//                  -1  ILLEGAL_SYMBOL: bitstream error
//                  -2  ILLEGAL_STATE: internal program error
//                  -3  FINISHED_LAST_BLOCK: reached state ST263_FINISHED (program
//                      error for this routine)
extern int VLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int tabNum,         // Use dectable[tabNum] for decoding
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        SYMBOL * sym        // Returns decoded symbol
                        )
{
    DECTABENTRY * entry;

    /*printf( "Entered VLDecSymbol: bitstream = %2x %2x %2x %2x  bitptr = %d\n",
            *bs->byteptr, *(bs->byteptr + 1), *(bs->byteptr + 2),
            *(bs->byteptr + 3), bs->bitptr);*/
            
    entry = &dectable [tabNum] [Get8Bits( *bs )];
    while (entry->bits < 0) {   // Long codeword; sym.value indicates table */
        *numBits += entry->bits;        // Subtract parsed bits
        IncBsPtr( bs, -entry->bits);    // Advance bitpointer
        //printf("VLDecSymbol - long symbol: "); printsym( entry->sym ); printf("\n");
        entry = &dectable [ entry->sym.value ] [Get8Bits( *bs )];
    }
    *numBits -= entry->bits;        // Subtract parsed bits
    IncBsPtr( bs, entry->bits);     // Advance bitpointer
    /*{
        //int input;
        printf("VLDecSymbol: "); printsym( entry->sym ); printf("\n");
        printf("  Cont? (<0 to exit): ");
        scanf("%d", &input);
        if (input < 0) exit(0);
    }*/
    if (entry->sym.type  ==  SYM_EXIT) {    // Premature exit
        if (*numBits < 0) {
            return (OUT_OF_BITS);
        } else {
            return (entry->sym.value);
        }
    }
    *sym = entry->sym;
    if (*numBits < 0) {
        return (OUT_OF_BITS);
    }
    return( OK );
}


//  InitDecodeTable - Generate decoding tables
extern void InitDecodeTable( void )
{
    int minBits;
    
    minBits = 1;    // Codeword strings need to be non-empty
    buildtable( minBits, dct_next, dectable[TAB_DCT_NEXT]);
    buildtable( minBits, dct_first, dectable[TAB_DCT_FIRST]);
    buildtable( minBits, dct_00100, dectable[TAB_DCT_00100]);
    buildtable( minBits, dct_000000, dectable[TAB_DCT_000000]);
    buildtable( minBits, escape_run, dectable[TAB_ESCAPE_RUN]);
    buildtable( minBits, escape_level, dectable[TAB_ESCAPE_LEVEL]);

    buildtable( minBits, intra_dc, dectable[TAB_INTRA_DC]);
    buildtable( minBits, last_intra_dc, dectable[TAB_LAST_INTRA_DC]);

    buildtable( minBits, mba_startcode, dectable[TAB_MBA_STARTCODE]);
    buildtable( minBits, long_mba, dectable[TAB_LONG_MBA]);
    buildtable( minBits, long_startcode, dectable[TAB_LONG_STARTCODE]);

    buildtable( minBits, mtype, dectable[TAB_MTYPE]);
    buildtable( minBits, long_mtype, dectable[TAB_LONG_MTYPE]);

    buildtable( minBits, mvd, dectable[TAB_MVD]);
    buildtable( minBits, long_mvd, dectable[TAB_LONG_MVD]);

    buildtable( minBits, cbp, dectable[TAB_CBP]);
    buildtable( minBits, long_cbp, dectable[TAB_LONG_CBP]);

    buildtable( minBits, quant_tr, dectable[TAB_QUANT_TR]);  //Don't parse strings

    buildtable( minBits, gei_pei, dectable[TAB_GEI_PEI]);
    buildtable( minBits, long_spare, dectable[TAB_LONG_SPARE]);  //Don't parse strings

    buildtable( minBits, gn, dectable[TAB_GN]);
    buildtable( minBits, ptype, dectable[TAB_PTYPE]);    //Don't parse strings

    buildtable( minBits, illegal_state, dectable[TAB_ILLEGAL_STATE]);
    
    // Generate H.263 decoding tables
    buildtable( minBits, mcbpc263,               dectable[TAB263_MCBPC_INTER] );
    buildtable( minBits, long_mcbpc263,          dectable[TAB263_LONG_MCBPC_INTER] );
    buildtable( minBits, long_startcode263,      dectable[TAB263_LONG_STARTCODE] );
    buildtable( minBits, zeros_and_start263,     dectable[TAB263_ZEROS_AND_START] );
    buildtable( minBits, intra_mcbpc263,         dectable[TAB263_MCBPC_INTRA] );
    buildtable( minBits, long_intra_mcbpc263,    dectable[TAB263_LONG_MCBPC_INTRA] );
    buildtable( minBits, modb263,                dectable[TAB263_MODB] );
    buildtable( minBits, cbpy263,                dectable[TAB263_CBPY] );
    buildtable( minBits, intra_cbpy263,          dectable[TAB263_CBPY_INTRA] );
    buildtable( minBits, dquant263,              dectable[TAB263_DQUANT] );
    buildtable( minBits, mvd263,                 dectable[TAB263_MVD] );
    buildtable( minBits, long_mvd263,            dectable[TAB263_LONG_MVD] );
    buildtable( minBits, tcoef,                  dectable[TAB263_TCOEF] );
    buildtable( minBits, tcoef_0001,             dectable[TAB263_TCOEF_0001] );
    buildtable( minBits, tcoef_0000_1,           dectable[TAB263_TCOEF_0000_1] );
    buildtable( minBits, tcoef_0000_0,           dectable[TAB263_TCOEF_0000_0] );
    buildtable( minBits, esc263_run,             dectable[TAB263_ESC_RUN] );
    buildtable( minBits, esc263_level,           dectable[TAB263_ESCAPE_LEVEL] );
    buildtable( minBits, intra263_dc,            dectable[TAB263_INTRA_DC] );
#ifdef DO_H263_PLUS
    buildtable( minBits, modb263plus,            dectable[TAB263PLUS_MODB] );
    buildtable( minBits, intra_mode263plus,      dectable[TAB263PLUS_INTRA_MODE] );
    buildtable( minBits, tcoef_plus,             dectable[TAB263PLUS_TCOEF] );
    buildtable( minBits, tcoef_0001_plus,        dectable[TAB263PLUS_TCOEF_0001] );
    buildtable( minBits, tcoef_0000_1_plus,      dectable[TAB263PLUS_TCOEF_0000_1] );
    buildtable( minBits, tcoef_0000_0_plus,      dectable[TAB263PLUS_TCOEF_0000_0] );
#endif
    minBits = 0;    // Accept empty codeword string (length 0) to produce SYM_EXIT
    buildtable( minBits, finished_263blk,        dectable[TAB263_FINISHED] );
    
    /*{
        int i, ntab;

        printf("\nDiagnostic print of table #: ");
        scanf("%d", &ntab);
        while (ntab >= 0) {
            printf( "Entry    Type  Value  Length  Statechange\n");
            for (i = 0; i < 256; i++) {
                printf( " %3d     %3d    %3d    %3d        %3d\n", i,
                dectable[ntab][i].sym.type, dectable[ntab][i].sym.value,
                dectable[ntab][i].bits, dectable[ntab][i].statechange);
            }
            printf("\nDiagnostic print of table #: ");
            scanf("%d", &ntab);
        }
        
        printf("\nselectdectab:\n");
        for (i = 0; i < NUMSTATES; ++i)
        {
            if (i % 10  ==  0)  printf("\n%3d   ", i);
            if (i % 10  ==  5)  printf("  ");
            printf("%3d", selectdectab[i] );
        }
        printf("\n");
    }*/
    return;
}


//  buildtable - Generate 8-bit (DECTABBITS) decode table;
//  Contains 256 (DECTABSIZE) entries
static int buildtable( int minBits, struct vlc_entry input[], DECTABENTRY output[DECTABSIZE])
{
    int i, j, bits, codeword, flc, value, status;
    char msg[120];

    for (i = 0; i < DECTABSIZE; i++) {
        output[i].sym.type = SYM_EXIT;
        output[i].sym.value = ILLEGAL_SYMBOL;
    }
    if (strcmpi( input[0].vlc, "FLC")) {    /* VLC is default */
        flc = 0;
        //printf("Variable length\n");
        i = 0;
    } else {
        flc = 1;
        //printf("Fixed length\n");
        i = 1;
    }
    //printf( "                   Hex   Type   Value   Statechange\n");
    while (strcmpi( input[i].vlc, "End")) {  /* Process until "End" */
        status = parse_bits( input[i].vlc, MAX_STRING_VLD-1, DECTABBITS,
                                &bits, &codeword, minBits);
        if (status != OK) {
            sprintf( msg, "Error in parse_bits from buildtable");
            H261ErrMsg( msg );
            return( H261_ERROR );
        }
        codeword <<= DECTABBITS - bits;
        value = input[i].value;
        if (flc == 0) { /* Do one codeword if VLC code */
            input[i].last_value = value;
        }
        while (value <= input[i].last_value) {
            //strpad( input[i].vlc, ' ', MAX_STRING_VLD-1);
            //printf( "%3d: %s    %2x  %4d   %4d      %4d    Bits = %d\n",
            //    i, input[i].vlc, codeword, input[i].type, value,
            //    input[i].statechange, bits);
            /* Build decode table */
            for (j = codeword; j < codeword + (1 << (DECTABBITS - bits)); j++) {
                if (output[j].sym.type != SYM_EXIT  ||
                        output[j].sym.value != ILLEGAL_SYMBOL) {
                    sprintf( msg, "String %d: Entry %d done previously", i, j);
                    H261ErrMsg( msg );
                    return( H261_ERROR );
                }
                output[j].sym.type = input[i].type;
                output[j].sym.value = value;
                output[j].statechange = input[i].statechange;
                /* Signal long codeword by inverting "bits" */
                if (input[i].type == SYM_ESCAPE) {
                    output[j].bits = -bits;
                } else {
                    output[j].bits = bits;
                }
            }
            ++value;
            codeword += (1 << (DECTABBITS - bits));
        }
        if (++i > DECTABSIZE) {
            goto no_end;
        }
    }
/*    if (flc == 0) {
        printf("Reached End after finding %d variable length codes\n", i);
    } else {
        printf("Reached End after finding %d fixed length entries\n", i-1);
    }
 */
    return (OK);
    
no_end:
    sprintf( msg, "No End mark");
    H261ErrMsg( msg );
    return( H261_ERROR );
}


//  Parse bitstring "vlc", return length in "bits" and value in "codeword"
static int parse_bits(  char vlc[],     /* String with bit pattern */
                        int strmax,     /* Max characters in "vlc" excluding terminating null */
                        int maxbits,    /* Max # bits in codeword */
                        int * bits,     /* Returns # bits; 0 < bits < maxbits+1 */
                        int * codeword, /* Returns codeword */
                        int minBits     // Min # bits in codeword
)
{
    int j, c;
    char msg[120];
    
    *bits = 0, j = 0, *codeword = 0;
    c = vlc[j];
    while (c != 0) {
        if (c == '0'  ||  c == '1') {
            *codeword <<= 1;
            ++(*bits);
            if (c == '1') {
                ++(*codeword);
            }
        } else if (isspace(c) == 0) {   /* Found illegal character */
            sprintf( msg, "parse_bits - Illegal string: %s", vlc);
            H261ErrMsg( msg );
            return( H261_ERROR );
        }
        c = vlc[++j];
    }
    if (j > strmax) {
        sprintf( msg, "Too long string: %s", vlc);
        H261ErrMsg( msg );
        return( H261_ERROR );
    }
    if (*bits > maxbits  ||  *bits < minBits) {
        sprintf( msg, "Illegal string: %s", vlc);
        H261ErrMsg( msg );
        return( H261_ERROR );
    }
    return (OK);
}


///////////  Routines for debugging  //////////////

//  strpad - Pad str to length len; return number of appended chars
/*static int strpad( char str[], int pad, int len)
{
    int i, numpad;

    numpad = 0;
    for (i = strlen(str); i < len; i++) {
        str[i] = pad;
        ++numpad;
    }
    if (numpad > 0) {
        str[len] = NULL;
    }
    return (numpad);
}*/

extern void printsym( SYMBOL sym )
{
    char ctype[80];
    
    sprinttype( sym.type, ctype);
    printf("%s =%4d", ctype, sym.value);
    return;
}

extern void sprintsym( SYMBOL sym, char s[] )
{
    char ctype[80];
    
    sprinttype( sym.type, ctype);
    sprintf( s, "%s =%4d", ctype, sym.value);
    return;
}

extern void sprinttype( int symtype, char s[] )
{
    switch (symtype) {
    case SYM_EXIT:
        sprintf( s, "EXIT");
        break;
    case SYM_ESCAPE:
        sprintf( s, "ESCAPE  Table");
        break;
    case SYM_EOB:
        sprintf( s, "End Of Block");
        break;
    case SYM_INTRA_DC:
        sprintf( s, "Intra DC");
        break;
    case SYM_MBA:
        sprintf( s, "MBA");
        break;
    case SYM_STARTCODE:
        sprintf( s, "StartCode");
        break;
    case SYM_MBA_STUFFING:
        sprintf( s, "MBA Stuffing");
        break;
    case SYM_MTYPE:
        sprintf( s, "MB Type");
        break;
    case SYM_MVD:
        sprintf( s, "MVDiff");
        break;
    case SYM_CBP:
        sprintf( s, "CBP");
        break;
    case SYM_QUANT_TR:
        sprintf( s, "Quant/TR");
        break;
    case SYM_GEI_PEI:
        sprintf( s, "ExtraInsert");
        break;
    case SYM_SPARE:
        sprintf( s, "Spare");
        break;
    case SYM_GN:
        sprintf( s, "GOB Number");
        break;
    case SYM_PTYPE:
        sprintf( s, "Picture Type");
        break;
    case SYM_ESC_RUN:
        sprintf( s, "ESC Run");
        break;
    case SYM_ESC_LEVEL:
        sprintf( s, "ESC Level");
        break;
    case SYM_MCBPC:
        sprintf( s, "MCBPC");
        break;
    case SYM_MCBPC_STUFFING:
        sprintf( s, "MCBPC Stuffing");
        break;
    case SYM_MODB:
        sprintf( s, "MODB");
        break;
    case SYM_CBPY:
        sprintf( s, "CBPY");
        break;
    case SYM_DQUANT:
        sprintf( s, "DQUANT");
        break;
    default:
        sprintf( s, "Run = %d   Level", symtype);
        break;
    }
    return;
}

extern void printstate( int state )
{
    switch (state) {
    case ST_MC_NOCBP_MVDX:
        printf("MC No coeffs MVDx");
        break;
    case ST_MC_NOCBP_MVDY:
        printf("MC No coeffs MVDy");
        break;
    case ST_MBA_STARTCODE:
        printf("MBA or Startcode");
        break;
    case ST_NEXT_BLK_6:
        printf("One block to go, AC coeff");
        break;
    case ST_FIRST_BLK_6:
        printf("One block to go, first coeff");
        break;
    case ST_NEXT_BLK_5:
        printf("Two blocks to go, AC coeff");
        break;
    case ST_FIRST_BLK_5:
        printf("Two blocks to go, first coeff");
        break;
    case ST_NEXT_BLK_4:
        printf("Three blocks to go, AC coeff");
        break;
    case ST_FIRST_BLK_4:
        printf("Three blocks to go, first coeff");
        break;
    case ST_NEXT_BLK_3:
        printf("Four blocks to go, AC coeff");
        break;
    case ST_FIRST_BLK_3:
        printf("Four blocks to go, first coeff");
        break;
    case ST_NEXT_BLK_2:
        printf("Five blocks to go, AC coeff");
        break;
    case ST_FIRST_BLK_2:
        printf("Five blocks to go, first coeff");
        break;
    case ST_NEXT_BLK_1:
        printf("Six blocks to go, AC coeff");
        break;
    case ST_FIRST_BLK_1:
        printf("Six blocks to go, first coeff");
        break;
    case ST_INTRA_DC_BLK_6:
        printf("Last INTRA block, DC coeff");
        break;
    case ST_INTRA_AC_BLK_5:
        printf("Two INTRA blocks to go, AC coeff");
        break;
    case ST_INTRA_DC_BLK_5:
        printf("Two INTRA blocks to go, DC coeff");
        break;
    case ST_INTRA_AC_BLK_4:
        printf("Three INTRA blocks to go, AC coeff");
        break;
    case ST_INTRA_DC_BLK_4:
        printf("Three INTRA blocks to go, DC coeff");
        break;
    case ST_INTRA_AC_BLK_3:
        printf("Four INTRA blocks to go, AC coeff");
        break;
    case ST_INTRA_DC_BLK_3:
        printf("Four INTRA blocks to go, DC coeff");
        break;
    case ST_INTRA_AC_BLK_2:
        printf("Five INTRA blocks to go, AC coeff");
        break;
    case ST_INTRA_DC_BLK_2:
        printf("Five INTRA blocks to go, DC coeff");
        break;
    case ST_INTRA_AC_BLK_1:
        printf("Six INTRA blocks to go, AC coeff");
        break;
    case ST_INTRA_DC_BLK_1:
        printf("Six INTRA blocks to go, DC coeff");
        break;
    case ST_MC_CBP_MVDX:
        printf("MVDx");
        break;
    case ST_MC_CBP_MVDY:
        printf("MVDy");
        break;
    case ST_CBP:
        printf("CBP");
        break;
    case ST_INTRA_MQUANT:
        printf("INTRA MQUANT");
        break;
    case ST_MC_CBP_MQUANT:
        printf("MC MQUANT");
        break;
    case ST_ESC_BLK_6:
        printf("ESCAPE, one block to go");
        break;
    case ST_INTER_MQUANT:
        printf("INTER MQUANT");
        break;
    case ST_ESC_BLK_5:
        printf("ESCAPE, two blocks to go");
        break;
    case ST_MTYPE:
        printf("MTYPE");
        break;
    case ST_ESC_BLK_4:
        printf("ESCAPE, three blocks to go");
        break;
    case ST_GEI_PEI:
        printf("PEI/GEI");
        break;
    case ST_ESC_BLK_3:
        printf("ESCAPE, four blocks to go");
        break;
    case ST_PTYPE:
        printf("PictureType");
        break;
    case ST_ESC_BLK_2:
        printf("ESCAPE, five blocks to go");
        break;
    case ST_GQUANT:
        printf("GQUANT");
        break;
    case ST_ESC_BLK_1:
        printf("ESCAPE, six blocks to go");
        break;
    case ST_TR:
        printf("TempRef");
        break;
    case ST_AFTER_STARTCODE:
        printf("After Startcode");
        break;
    case ST_ESC_INTRA_5:
        printf("ESCAPE, two INTRA blocks to go");
        break;
    case ST_ESC_INTRA_4:
        printf("ESCAPE, three INTRA blocks to go");
        break;
    case ST_ESC_INTRA_3:
        printf("ESCAPE, four INTRA blocks to go");
        break;
    case ST_ESC_INTRA_2:
        printf("ESCAPE, five INTRA blocks to go");
        break;
    case ST_ESC_INTRA_1:
        printf("ESCAPE, six INTRA blocks to go");
        break;

    // H.263 states
    case ST263_FINISHED:
        printf("Finished H.263 block decoding");
        break;
    case ST263_ESC_FINISHED:
        printf("Decode ESC-LEVEL, then done");
        break;
    case ST263_BLK_6:
        printf("Decoding last block (block 6)");
        break;
    case ST263_ESC_BLK6:
        printf("Decode ESC-LEVEL, then continue with block 6");
        break;
    case ST263_BLK_5:
        printf("Decoding block 5");
        break;
    case ST263_ESC_BLK5:
        printf("Decode ESC-LEVEL, then continue with block 5");
        break;
    case ST263_BLK_4:
        printf("Decoding block 4");
        break;
    case ST263_ESC_BLK4:
        printf("Decode ESC-LEVEL, then continue with block 4");
        break;
    case ST263_BLK_3:
        printf("Decoding block 3");
        break;
    case ST263_ESC_BLK3:
        printf("Decode ESC-LEVEL, then continue with block 3");
        break;
    case ST263_BLK_2:
        printf("Decoding block 2");
        break;
    case ST263_ESC_BLK2:
        printf("Decode ESC-LEVEL, then continue with block 2");
        break;
    case ST263_BLK_1:
        printf("Decoding block 1");
        break;
    case ST263_ESC_BLK1:
        printf("Decode ESC-LEVEL, then continue with block 1");
        break;
    case ST263_INTRA_DC_ONLY:
        printf("Decode INTRA-DC only, then exit");
        break;
    case ST263_INTRA_DC_AC:
        printf("Decode INTRA-DC and AC coeffs, then exit");
        break;

    default:
        printf("Unknown state = %d", state);
        break;
    }
    return;
}
