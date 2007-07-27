/*-----------------------------------------------------------------------------
 *	VLD.H
 *
 *	DESCRIPTION
 *		vld.h - Tables for variable length decoding of H.261 bitstream
 *		Uses definitions in "h261type.h", "h261defs.h", and "vldstate.h"
 *
 *	Author:		Staffan Ericsson	6/28/93
 *	Inspector:	Mary Deshon			7/6/93
 *	Revised:
 *  02/10/97    D Blair         Added tables for H.263+ decoding.
 *  11/10/95    S Ericsson      Allow up to 7 zeros before startcodes.
 *  7/10/95     S Ericsson      Added tables for H.263 decoding
 *                              Broke out table numbering into VLDTAB.H
 *	8/23/94		G Su			Changed DECTABENTRY and selectdectab NOT static
 *	7/6/93		Mary Deshon		Changed typedefs to S8, U8, ... ,S32, U32
 *	7/6/93		Mary Deshon		Minor style changes
 *  10/29/93 	M Deshon		Change MAX_STRING to MAX_STRING_VLD
 *
 *	(c) 1993-1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _INC_VLD
#define _INC_VLD	1
 
/*  DECTABENTRY - Description of each entry in the decode tables
 *  An entry in the decode table contains the following information:
 *  sym  -      a)  If bits > 0, sym contains decoded symbol
 *              If sym.type = SYM_EXIT, we find the reason in sym.value
 *              b)  bits < 0 indicates that long codeword was encountered.
 *              We continue decoding using table indicated by sym.value.
 *              sym.type is not defined.
 *  statechange Add to state to get the new state
 *  bits  -     abs(bits) is # of parsed bits.  bits < 0 indicates code > 8 bits
 */

typedef struct {
    SYMBOL		sym;
    S8			statechange;
    S8			bits;
	} DECTABENTRY;

#define DECTABBITS          8       /* Look up DECTABBITS bits at a time */
#define DECTABSIZE          256     /* 2 ** DECTABBITS */

DECTABENTRY dectable[NUMTABS][DECTABSIZE];

U8 selectdectab[NUMSTATES] = {
    TAB_ILLEGAL_STATE,                      /* State 0 is not used */
    TAB_MVD,                TAB_MVD,        /* ST_MC_NOCBP_MVD */
    TAB_MBA_STARTCODE,
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 6 */
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 5 */
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 4 */
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 3 */
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 2 */
    TAB_DCT_NEXT,           TAB_DCT_FIRST,  /* Block 1 */
    TAB_LAST_INTRA_DC,                      /* INTRA Block 6 */
    TAB_DCT_NEXT,           TAB_INTRA_DC,   /* INTRA Block 5 */
    TAB_DCT_NEXT,           TAB_INTRA_DC,   /* INTRA Block 4 */
    TAB_DCT_NEXT,           TAB_INTRA_DC,   /* INTRA Block 3 */
    TAB_DCT_NEXT,           TAB_INTRA_DC,   /* INTRA Block 2 */
    TAB_DCT_NEXT,           TAB_INTRA_DC,   /* INTRA Block 1 */
    TAB_MVD,                TAB_MVD,        /* ST_MC_CBP_MVD */
    TAB_CBP,
    TAB_QUANT_TR,                           /* ST_INTRA_MQUANT */
    TAB_QUANT_TR,                           /* ST_MC_CBP_MQUANT */
    TAB_ESCAPE_LEVEL,
    TAB_QUANT_TR,                           /* ST_INTER_MQUANT */
    TAB_ESCAPE_LEVEL,
    TAB_MTYPE,
    TAB_ESCAPE_LEVEL,
    TAB_GEI_PEI,
    TAB_ESCAPE_LEVEL,
    TAB_PTYPE,
    TAB_ESCAPE_LEVEL,
    TAB_QUANT_TR,                           /* ST_GQUANT */
    TAB_ESCAPE_LEVEL,
    TAB_QUANT_TR,                           /* ST_TR */
    TAB_GN,                                 /* ST_AFTER_STARTCODE */
    TAB_ESCAPE_LEVEL,
    TAB_ILLEGAL_STATE,                      /* State 46 is not used */
    TAB_ESCAPE_LEVEL,
    TAB_ILLEGAL_STATE,                      /* State 48 is not used */
    TAB_ESCAPE_LEVEL,
    TAB_ILLEGAL_STATE,                      /* State 50 is not used */
    TAB_ESCAPE_LEVEL,
    TAB_ILLEGAL_STATE,                      /* State 52 is not used */
    TAB_ESCAPE_LEVEL,
    
    // H.263 states start at item 54
    TAB263_FINISHED,                        // Done with last block
    TAB263_ESCAPE_LEVEL,                    // ESCAPE-RUN-LEVEL as last TCOEF of last block
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 6
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 5
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 4
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 3
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 2
    TAB263_TCOEF,   TAB263_ESCAPE_LEVEL,    // Block 1
    TAB263_INTRA_DC,                        // INTRA-DC only
    TAB_ILLEGAL_STATE,                      // State 69 is not used
    TAB263_INTRA_DC                         // INTRA-DC followed by AC coefficients

#ifdef DO_H263_PLUS
    ,
    TAB263_FINISHED,                            // Done with last block
    TAB263_ESCAPE_LEVEL,                        // ESCAPE-RUN-LEVEL as last TCOEF of last block
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL,  // Block 6
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL,  // Block 5
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL,  // Block 4
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL,  // Block 3
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL,  // Block 2
    TAB263PLUS_TCOEF, TAB263_ESCAPE_LEVEL   // Block 1
#endif
};

#define MAX_STRING_VLD          (DECTABBITS + 4)

struct vlc_entry {
    char    vlc[MAX_STRING_VLD];
    int     type;
    int     value;
    int     statechange;
    int     last_value; /* Only used for fixed-length codes */
};


////////////////  Tables for H.261  //////////////////

static  struct vlc_entry dct_next[] = {
    {"0000 01",     SYM_ESCAPE, TAB_ESCAPE_RUN,    0},
    {"0010 0",      SYM_ESCAPE, TAB_DCT_00100,     0},
    {"0000 00",     SYM_ESCAPE, TAB_DCT_000000,    0},
    {"10",          SYM_EOB,    0,      (ST_FIRST_BLK_2 - ST_NEXT_BLK_1)},

    {"110",         0,      1,      0},
    {"111",         0,      -1,     0},

    {"0110",        1,      1,      0},
    {"0111",        1,      -1,     0},

    {"0100 0",      0,      2,      0},
    {"0100 1",      0,      -2,     0},
    {"0101 0",      2,      1,      0},
    {"0101 1",      2,      -1,     0},

    {"0010 10",     0,      3,      0},
    {"0010 11",     0,      -3,     0},
    {"0011 10",     3,      1,      0},
    {"0011 11",     3,      -1,     0},
    {"0011 00",     4,      1,      0},
    {"0011 01",     4,      -1,     0},

    {"0001 100",    1,      2,      0},
    {"0001 101",    1,      -2,     0},
    {"0001 110",    5,      1,      0},
    {"0001 111",    5,      -1,     0},
    {"0001 010",    6,      1,      0},
    {"0001 011",    6,      -1,     0},
    {"0001 000",    7,      1,      0},
    {"0001 001",    7,      -1,     0},

    {"0000 1100",   0,      4,      0},
    {"0000 1101",   0,      -4,     0},
    {"0000 1000",   2,      2,      0},
    {"0000 1001",   2,      -2,     0},
    {"0000 1110",   8,      1,      0},
    {"0000 1111",   8,      -1,     0},
    {"0000 1010",   9,      1,      0},
    {"0000 1011",   9,      -1,     0},
    {"End"}
};

static  struct vlc_entry dct_first[] = {
    {"0000 01",     SYM_ESCAPE, TAB_ESCAPE_RUN, (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0010 0",      SYM_ESCAPE, TAB_DCT_00100,  (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 00",     SYM_ESCAPE, TAB_DCT_000000, (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"10",          0,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"11",          0,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"0110",        1,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0111",        1,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"0100 0",      0,      2,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0100 1",      0,      -2,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0101 0",      2,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0101 1",      2,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"0010 10",     0,      3,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0010 11",     0,      -3,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0011 10",     3,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0011 11",     3,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0011 00",     4,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0011 01",     4,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"0001 100",    1,      2,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 101",    1,      -2,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 110",    5,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 111",    5,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 010",    6,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 011",    6,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 000",    7,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0001 001",    7,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},

    {"0000 1100",   0,      4,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1101",   0,      -4,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1000",   2,      2,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1001",   2,      -2,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1110",   8,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1111",   8,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1010",   9,      1,      (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"0000 1011",   9,      -1,     (ST_NEXT_BLK_1 - ST_FIRST_BLK_1)},
    {"End"}
};

static  struct vlc_entry dct_00100[] = {
    {"110 0",       0,      5,      0},
    {"110 1",       0,      -5,     0},
    {"001 0",       0,      6,      0},
    {"001 1",       0,      -6,     0},
    {"101 0",       1,      3,      0},
    {"101 1",       1,      -3,     0},
    {"100 0",       3,      2,      0},
    {"100 1",       3,      -2,     0},
    {"111 0",       10,     1,      0},
    {"111 1",       10,     -1,     0},
    {"011 0",       11,     1,      0},
    {"011 1",       11,     -1,     0},
    {"010 0",       12,     1,      0},
    {"010 1",       12,     -1,     0},
    {"000 0",       13,     1,      0},
    {"000 1",       13,     -1,     0},
    {"End"}
};

static  struct vlc_entry dct_000000[] = {
    {"10 100",      0,      7,      0},
    {"10 101",      0,      -7,     0},
    {"11 000",      1,      4,      0},
    {"11 001",      1,      -4,     0},
    {"10 110",      2,      3,      0},
    {"10 111",      2,      -3,     0},
    {"11 110",      4,      2,      0},
    {"11 111",      4,      -2,     0},
    {"10 010",      5,      2,      0},
    {"10 011",      5,      -2,     0},
    {"11 100",      14,     1,      0},
    {"11 101",      14,     -1,     0},
    {"11 010",      15,     1,      0},
    {"11 011",      15,     -1,     0},
    {"10 000",      16,     1,      0},
    {"10 001",      16,     -1,     0},

    {"01 1101 0",   0,      8,      0},
    {"01 1101 1",   0,      -8,     0},
    {"01 1000 0",   0,      9,      0},
    {"01 1000 1",   0,      -9,     0},
    {"01 0011 0",   0,      10,     0},
    {"01 0011 1",   0,      -10,    0},
    {"01 0000 0",   0,      11,     0},
    {"01 0000 1",   0,      -11,    0},
    {"01 1011 0",   1,      5,      0},
    {"01 1011 1",   1,      -5,     0},
    {"01 0100 0",   2,      4,      0},
    {"01 0100 1",   2,      -4,     0},
    {"01 1100 0",   3,      3,      0},
    {"01 1100 1",   3,      -3,     0},
    {"01 0010 0",   4,      3,      0},
    {"01 0010 1",   4,      -3,     0},
    {"01 1110 0",   6,      2,      0},
    {"01 1110 1",   6,      -2,     0},
    {"01 0101 0",   7,      2,      0},
    {"01 0101 1",   7,      -2,     0},
    {"01 0001 0",   8,      2,      0},
    {"01 0001 1",   8,      -2,     0},
    {"01 1111 0",   17,     1,      0},
    {"01 1111 1",   17,     -1,     0},
    {"01 1010 0",   18,     1,      0},
    {"01 1010 1",   18,     -1,     0},
    {"01 1001 0",   19,     1,      0},
    {"01 1001 1",   19,     -1,     0},
    {"01 0111 0",   20,     1,      0},
    {"01 0111 1",   20,     -1,     0},
    {"01 0110 0",   21,     1,      0},
    {"01 0110 1",   21,     -1,     0},

    {"00 1101 00",  0,      12,     0},
    {"00 1101 01",  0,      -12,    0},
    {"00 1100 10",  0,      13,     0},
    {"00 1100 11",  0,      -13,    0},
    {"00 1100 00",  0,      14,     0},
    {"00 1100 01",  0,      -14,    0},
    {"00 1011 10",  0,      15,     0},
    {"00 1011 11",  0,      -15,    0},
    {"00 1011 00",  1,      6,      0},
    {"00 1011 01",  1,      -6,     0},
    {"00 1010 10",  1,      7,      0},
    {"00 1010 11",  1,      -7,     0},
    {"00 1010 00",  2,      5,      0},
    {"00 1010 01",  2,      -5,     0},
    {"00 1001 10",  3,      4,      0},
    {"00 1001 11",  3,      -4,     0},
    {"00 1001 00",  5,      3,      0},
    {"00 1001 01",  5,      -3,     0},
    {"00 1000 10",  9,      2,      0},
    {"00 1000 11",  9,      -2,     0},
    {"00 1000 00",  10,     2,      0},
    {"00 1000 01",  10,     -2,     0},
    {"00 1111 10",  22,     1,      0},
    {"00 1111 11",  22,     -1,     0},
    {"00 1111 00",  23,     1,      0},
    {"00 1111 01",  23,     -1,     0},
    {"00 1110 10",  24,     1,      0},
    {"00 1110 11",  24,     -1,     0},
    {"00 1110 00",  25,     1,      0},
    {"00 1110 01",  25,     -1,     0},
    {"00 1101 10",  26,     1,      0},
    {"00 1101 11",  26,     -1,     0},
    {"End"}
};

static  struct vlc_entry escape_run[] = {
    {"FLC"},
    {"00 0000",     SYM_ESC_RUN,    0,  -ST_DIFF_ESC_LEVEL,     63},
    {"End"}
};

/* ESC_LEVEL: Levels 0 and -128 are not allowed */
static  struct vlc_entry escape_level[] = {
    {"FLC"},
    {"0000 0001",   SYM_ESC_LEVEL,      1,  ST_DIFF_ESC_LEVEL,  127},
    {"1000 0001",   SYM_ESC_LEVEL,   -127,  ST_DIFF_ESC_LEVEL,  -1},
    {"End"}
};

/* INTRA DC coeff:  Levels 0 and 255 not allowed; level 128 repr. by 255 */
static  struct vlc_entry intra_dc[] = {
    {"FLC"},
    {"0000 0001", SYM_INTRA_DC,   1,    -1,     127},
    {"1111 1111", SYM_INTRA_DC, 128,    -1,     128},
    {"1000 0001", SYM_INTRA_DC, 129,    -1,     254},
    {"End"}
};

/* INTRA DC coeff:  Levels 0 and 255 not allowed; level 128 repr. by 255 */
static  struct vlc_entry last_intra_dc[] = {
    {"FLC"},
    {"0000 0001", SYM_INTRA_DC,   1, (ST_NEXT_BLK_6 - ST_INTRA_DC_BLK_6), 127},
    {"1111 1111", SYM_INTRA_DC, 128, (ST_NEXT_BLK_6 - ST_INTRA_DC_BLK_6), 128},
    {"1000 0001", SYM_INTRA_DC, 129, (ST_NEXT_BLK_6 - ST_INTRA_DC_BLK_6), 254},
    {"End"}
};

static  struct vlc_entry mba_startcode[] = {
    {"1",           SYM_MBA,        1,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011",         SYM_MBA,        2,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"010",         SYM_MBA,        3,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0011",        SYM_MBA,        4,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0010",        SYM_MBA,        5,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0001 1",      SYM_MBA,        6,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0001 0",      SYM_MBA,        7,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 111",    SYM_MBA,        8,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 110",    SYM_MBA,        9,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 1011",   SYM_MBA,       10,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 1010",   SYM_MBA,       11,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 1001",   SYM_MBA,       12,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"0000 1000",   SYM_MBA,       13,  (ST_MTYPE - ST_MBA_STARTCODE)},

    {"0000 0",      SYM_ESCAPE, TAB_LONG_MBA,   0},
    {"End"}
};

static  struct vlc_entry long_mba[] = {
    {"111",         SYM_MBA,       14,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"110",         SYM_MBA,       15,  (ST_MTYPE - ST_MBA_STARTCODE)},
    
    {"101 11",      SYM_MBA,       16,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"101 10",      SYM_MBA,       17,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"101 01",      SYM_MBA,       18,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"101 00",      SYM_MBA,       19,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"100 11",      SYM_MBA,       20,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"100 10",      SYM_MBA,       21,  (ST_MTYPE - ST_MBA_STARTCODE)},

    {"100 011",     SYM_MBA,       22,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"100 010",     SYM_MBA,       23,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"100 001",     SYM_MBA,       24,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"100 000",     SYM_MBA,       25,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 111",     SYM_MBA,       26,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 110",     SYM_MBA,       27,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 101",     SYM_MBA,       28,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 100",     SYM_MBA,       29,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 011",     SYM_MBA,       30,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 010",     SYM_MBA,       31,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 001",     SYM_MBA,       32,  (ST_MTYPE - ST_MBA_STARTCODE)},
    {"011 000",     SYM_MBA,       33,  (ST_MTYPE - ST_MBA_STARTCODE)},

    {"001 111",     SYM_MBA_STUFFING,   0,  0},
    {"000 0000 0",  SYM_ESCAPE, TAB_LONG_STARTCODE, 0},
    {"End"}
};

static  struct vlc_entry long_startcode[] = {
    {"001",     SYM_STARTCODE, 0, (ST_AFTER_STARTCODE - ST_MBA_STARTCODE)},
    {"End"}
};

static  struct vlc_entry mtype[] = {
    {"0001",    SYM_MTYPE,  MTYPE_INTRA,        (ST_INTRA_DC_BLK_1 - ST_MTYPE)},
    {"0000 001",SYM_MTYPE,  MTYPE_INTRA_MQUANT, (ST_INTRA_MQUANT - ST_MTYPE)},
    {"1",       SYM_MTYPE,  MTYPE_INTER,        (ST_CBP - ST_MTYPE)},
    {"0000 1",  SYM_MTYPE,  MTYPE_INTER_MQUANT, (ST_INTER_MQUANT - ST_MTYPE)},
    {"0000 0001",SYM_MTYPE, MTYPE_MC_CBP,       (ST_MC_CBP_MVDX - ST_MTYPE)},
    {"001",     SYM_MTYPE,  MTYPE_MCFILT_NOCBP, (ST_MC_NOCBP_MVDX - ST_MTYPE)},
    {"01",      SYM_MTYPE,  MTYPE_MCFILT_CBP,   (ST_MC_CBP_MVDX - ST_MTYPE)},
    {"0000 01", SYM_MTYPE,  MTYPE_MCFILT_MQUANT,(ST_MC_CBP_MQUANT - ST_MTYPE)},
    {"0000 0000", SYM_ESCAPE,   TAB_LONG_MTYPE, 0},
    {"End"}
};

static  struct vlc_entry long_mtype[] = {
    {"1",       SYM_MTYPE,  MTYPE_MC_NOCBP,     (ST_MC_NOCBP_MVDX - ST_MTYPE)},
    {"01",      SYM_MTYPE,  MTYPE_MC_MQUANT,    (ST_MC_CBP_MQUANT - ST_MTYPE)},
    {"End"}
};

static  struct vlc_entry mvd[] = {
    {"1",           SYM_MVD,    0,      1},
    {"010",         SYM_MVD,    1,      1},
    {"011",         SYM_MVD,    -1,     1},
    {"0010",        SYM_MVD,    2,      1},
    {"0011",        SYM_MVD,    -2,     1},
    {"0001 0",      SYM_MVD,    3,      1},
    {"0001 1",      SYM_MVD,    -3,     1},
    {"0000 110",    SYM_MVD,    4,      1},
    {"0000 111",    SYM_MVD,    -4,     1},
    {"0000 1010",   SYM_MVD,    5,      1},
    {"0000 1011",   SYM_MVD,    -5,     1},
    {"0000 1000",   SYM_MVD,    6,      1},
    {"0000 1001",   SYM_MVD,    -6,     1},
    {"0000 0",      SYM_ESCAPE, TAB_LONG_MVD,   1},
    {"End"}
};

static  struct vlc_entry long_mvd[] = {
    {"110",         SYM_MVD,    7,      0},
    {"111",         SYM_MVD,    -7,     0},
    {"101 10",      SYM_MVD,    8,      0},
    {"101 11",      SYM_MVD,    -8,     0},
    {"101 00",      SYM_MVD,    9,      0},
    {"101 01",      SYM_MVD,    -9,     0},
    {"100 10",      SYM_MVD,    10,     0},
    {"100 11",      SYM_MVD,    -10,    0},
    {"100 010",     SYM_MVD,    11,     0},
    {"100 011",     SYM_MVD,    -11,    0},
    {"100 000",     SYM_MVD,    12,     0},
    {"100 001",     SYM_MVD,    -12,    0},
    {"011 110",     SYM_MVD,    13,     0},
    {"011 111",     SYM_MVD,    -13,    0},
    {"011 100",     SYM_MVD,    14,     0},
    {"011 101",     SYM_MVD,    -14,    0},
    {"011 010",     SYM_MVD,    15,     0},
    {"011 011",     SYM_MVD,    -15,    0},
    {"011 001",     SYM_MVD,    -16,    0},
    {"End"}
};

static  struct vlc_entry cbp[] = {
    {"111",         SYM_CBP,    0x3c,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"1101",        SYM_CBP,    0x4,    (ST_FIRST_BLK_6 - ST_CBP)},
    {"1100",        SYM_CBP,    0x8,    (ST_FIRST_BLK_6 - ST_CBP)},
    {"1011",        SYM_CBP,    0x10,   (ST_FIRST_BLK_6 - ST_CBP)},
    {"1010",        SYM_CBP,    0x20,   (ST_FIRST_BLK_6 - ST_CBP)},

    {"1001 1",      SYM_CBP,    0xc,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"1001 0",      SYM_CBP,    0x30,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"1000 1",      SYM_CBP,    0x14,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"1000 0",      SYM_CBP,    0x28,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0111 1",      SYM_CBP,    0x1c,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0111 0",      SYM_CBP,    0x2c,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0110 1",      SYM_CBP,    0x34,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0110 0",      SYM_CBP,    0x38,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0101 1",      SYM_CBP,    0x1,    (ST_FIRST_BLK_6 - ST_CBP)},
    {"0101 0",      SYM_CBP,    0x3d,   (ST_FIRST_BLK_2 - ST_CBP)},
    {"0100 1",      SYM_CBP,    0x2,    (ST_FIRST_BLK_6 - ST_CBP)},
    {"0100 0",      SYM_CBP,    0x3e,   (ST_FIRST_BLK_2 - ST_CBP)},

    {"0011 11",     SYM_CBP,    0x18,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0011 10",     SYM_CBP,    0x24,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0011 01",     SYM_CBP,    0x3,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"0011 00",     SYM_CBP,    0x3f,   (ST_FIRST_BLK_1 - ST_CBP)},

    {"0010 111",    SYM_CBP,    0x5,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 110",    SYM_CBP,    0x9,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 101",    SYM_CBP,    0x11,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 100",    SYM_CBP,    0x21,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 011",    SYM_CBP,    0x6,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 010",    SYM_CBP,    0xa,    (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 001",    SYM_CBP,    0x12,   (ST_FIRST_BLK_5 - ST_CBP)},
    {"0010 000",    SYM_CBP,    0x22,   (ST_FIRST_BLK_5 - ST_CBP)},

    {"0001 1111",   SYM_CBP,    0x7,    (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1110",   SYM_CBP,    0xb,    (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1101",   SYM_CBP,    0x13,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1100",   SYM_CBP,    0x23,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1011",   SYM_CBP,    0xd,    (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1010",   SYM_CBP,    0x31,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1001",   SYM_CBP,    0x15,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 1000",   SYM_CBP,    0x29,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 0111",   SYM_CBP,    0xe,    (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 0110",   SYM_CBP,    0x32,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 0101",   SYM_CBP,    0x16,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 0100",   SYM_CBP,    0x2a,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0001 0011",   SYM_CBP,    0xf,    (ST_FIRST_BLK_3 - ST_CBP)},
    {"0001 0010",   SYM_CBP,    0x33,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0001 0001",   SYM_CBP,    0x17,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0001 0000",   SYM_CBP,    0x2b,   (ST_FIRST_BLK_3 - ST_CBP)},

    {"0000 1111",   SYM_CBP,    0x19,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0000 1110",   SYM_CBP,    0x25,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0000 1101",   SYM_CBP,    0x1a,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0000 1100",   SYM_CBP,    0x26,   (ST_FIRST_BLK_4 - ST_CBP)},
    {"0000 1011",   SYM_CBP,    0x1d,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 1010",   SYM_CBP,    0x2d,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 1001",   SYM_CBP,    0x35,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 1000",   SYM_CBP,    0x39,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 0111",   SYM_CBP,    0x1e,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 0110",   SYM_CBP,    0x2e,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 0101",   SYM_CBP,    0x36,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"0000 0100",   SYM_CBP,    0x3a,   (ST_FIRST_BLK_3 - ST_CBP)},

    {"0000 00",     SYM_ESCAPE, TAB_LONG_CBP,   0},
    {"End"}
};

static  struct vlc_entry long_cbp[] = {
    {"11 1",        SYM_CBP,    0x1f,   (ST_FIRST_BLK_2 - ST_CBP)},
    {"11 0",        SYM_CBP,    0x2f,   (ST_FIRST_BLK_2 - ST_CBP)},
    {"10 1",        SYM_CBP,    0x37,   (ST_FIRST_BLK_2 - ST_CBP)},
    {"10 0",        SYM_CBP,    0x3b,   (ST_FIRST_BLK_2 - ST_CBP)},
    {"01 1",        SYM_CBP,    0x1b,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"01 0",        SYM_CBP,    0x27,   (ST_FIRST_BLK_3 - ST_CBP)},
    {"End"}
};

static  struct vlc_entry quant_tr[] = {
    {"FLC"},
    {"00000",   SYM_QUANT_TR,   0,      ST_DIFF_QUANT,      31},
    {"End"}
};

static  struct vlc_entry gei_pei[] = {
    {"0",       SYM_GEI_PEI,    0,      (ST_MBA_STARTCODE - ST_GEI_PEI)},
    {"1",       SYM_ESCAPE, TAB_LONG_SPARE,     0},
    {"End"}
};

static  struct vlc_entry long_spare[] = {
    {"FLC"},
    {"0000 0000",   SYM_SPARE,  0,      0,      255},
    {"End"}
};

static  struct vlc_entry gn[] = {
    {"FLC"},
    {"0000",    SYM_GN,     0,  (ST_TR - ST_AFTER_STARTCODE),       0},
    {"0001",    SYM_GN,     1,  (ST_GQUANT - ST_AFTER_STARTCODE),   15},
    {"End"}
};

static  struct vlc_entry ptype[] = {
    {"FLC"},
    {"0000 00", SYM_PTYPE,  0,  (ST_GEI_PEI - ST_PTYPE),    63},
    {"End"}
};

static  struct vlc_entry illegal_state[] = {
    {"0",       SYM_EXIT,   ILLEGAL_STATE,      0},
    {"1",       SYM_EXIT,   ILLEGAL_STATE,      0},
    {"End"}
};


////////////////  Tables for H.263  //////////////////

// Starts with COD=0 for coded block; COD=1 for skipped block
static  struct vlc_entry mcbpc263[] = {
    {"0 0000 0",    SYM_ESCAPE, TAB263_LONG_MCBPC_INTER},
    
    {"1",           SYM_MCBPC,  MTYPE_SKIP},
    
    {"0 1",         SYM_MCBPC,  MTYPE263_INTER + 0},
    {"0 0011",      SYM_MCBPC,  MTYPE263_INTER + 1},
    {"0 0010",      SYM_MCBPC,  MTYPE263_INTER + 2},
    {"0 0001 01",   SYM_MCBPC,  MTYPE263_INTER + 3},
    {"0 011",       SYM_MCBPC,  MTYPE263_INTER_Q + 0},
    {"0 0000 111",  SYM_MCBPC,  MTYPE263_INTER_Q + 1},
    {"0 0000 110",  SYM_MCBPC,  MTYPE263_INTER_Q + 2},
    {"0 010",       SYM_MCBPC,  MTYPE263_INTER4V + 0},
    {"0 0000 101",  SYM_MCBPC,  MTYPE263_INTER4V + 1},
    {"0 0000 100",  SYM_MCBPC,  MTYPE263_INTER4V + 2},
    {"0 0001 1",    SYM_MCBPC,  MTYPE263_INTRA + 0},
    {"0 0001 00",   SYM_MCBPC,  MTYPE263_INTRA_Q + 0},
    {"End"}
};

static  struct vlc_entry long_mcbpc263[] = {        // Starts with "0 0000 0"
    {"00 0000 0",  SYM_ESCAPE, TAB263_LONG_STARTCODE},  // Note: no COD=0 preceeds startcode
    
    {"000 1",       SYM_MCBPC_STUFFING,  MTYPE263_STUFFING},

    {"010 1",       SYM_MCBPC,  MTYPE263_INTER_Q + 3},
    {"101",         SYM_MCBPC,  MTYPE263_INTER4V + 3},
    {"100",         SYM_MCBPC,  MTYPE263_INTRA + 1},
    {"011",         SYM_MCBPC,  MTYPE263_INTRA + 2},
    {"11",          SYM_MCBPC,  MTYPE263_INTRA + 3},
    {"010 0",       SYM_MCBPC,  MTYPE263_INTRA_Q + 1},
    {"001 1",       SYM_MCBPC,  MTYPE263_INTRA_Q + 2},
    {"001 0",       SYM_MCBPC,  MTYPE263_INTRA_Q + 3},
    {"End"}
};

static  struct vlc_entry long_startcode263[] = {    // Starts with "0000 0000 0000 0"
    {"000 0",       SYM_ESCAPE,     TAB263_ZEROS_AND_START},
    {"000 1",       SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"End"}
};

// Handle startcodes preceded by 1 to 7 zeros
// Starts with 17 zeros
// CJG 8.13.96: added line containing "0000 0001"
//	  Deals with Intel's padding more than 7 bits of zeros (8) after last macroblock in row.
//    This does not conform to H.263, and we may want to remove it after(if) Intel fixes
//    their video coder.  This change was interop tested in San Jose with all other vendors.
static  struct vlc_entry zeros_and_start263[] = {
    {"1",           SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"01",          SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"001",         SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"0001",        SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"0000 1",      SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"0000 01",     SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"0000 001",    SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"0000 0001",   SYM_STARTCODE,  MTYPE263_STARTCODE},
    {"End"}
};

static  struct vlc_entry intra_mcbpc263[] = {
    {"0000 0000",   SYM_ESCAPE, TAB263_LONG_MCBPC_INTRA},
    
    {"1",           SYM_MCBPC,  MTYPE263_INTRA + 0},
    {"001",         SYM_MCBPC,  MTYPE263_INTRA + 1},
    {"010",         SYM_MCBPC,  MTYPE263_INTRA + 2},
    {"011",         SYM_MCBPC,  MTYPE263_INTRA + 3},
    
    {"0001",        SYM_MCBPC,  MTYPE263_INTRA_Q + 0},
    {"0000 01",     SYM_MCBPC,  MTYPE263_INTRA_Q + 1},
    {"0000 10",     SYM_MCBPC,  MTYPE263_INTRA_Q + 2},
    {"0000 11",     SYM_MCBPC,  MTYPE263_INTRA_Q + 3},
    {"End"}
};

static  struct vlc_entry long_intra_mcbpc263[] = {  // Starts with "0000 0000"
    {" 0000 0",    SYM_ESCAPE,  TAB263_LONG_STARTCODE},
    
    {" 1",          SYM_MCBPC_STUFFING,  MTYPE263_STUFFING},
    {"End"}
};

static  struct vlc_entry modb263[] = {
    {"0",   SYM_MODB,   0},
    {"10",  SYM_MODB,   1},
    {"11",  SYM_MODB,   2},
    {"End"}
};

static  struct vlc_entry cbpy263[] = {
    {"0011",    SYM_CBPY,   0xf },
    {"0010 1",  SYM_CBPY,   0xe },
    {"0010 0",  SYM_CBPY,   0xd },
    {"1001",    SYM_CBPY,   0xc },
    
    {"0001 1",  SYM_CBPY,   0xb },
    {"0111",    SYM_CBPY,   0xa },
    {"0000 10", SYM_CBPY,   0x9 },
    {"1011",    SYM_CBPY,   0x8 },
    
    {"0001 0",  SYM_CBPY,   0x7 },
    {"0000 11", SYM_CBPY,   0x6 },
    {"0101",    SYM_CBPY,   0x5 },
    {"1010",    SYM_CBPY,   0x4 },
    
    {"0100",    SYM_CBPY,   0x3 },
    {"1000",    SYM_CBPY,   0x2 },
    {"0110",    SYM_CBPY,   0x1 },
    {"11",      SYM_CBPY,   0x0 },
    {"End"}
};

static  struct vlc_entry intra_cbpy263[] = {
    {"0011",    SYM_CBPY,   0x0 },
    {"0010 1",  SYM_CBPY,   0x1 },
    {"0010 0",  SYM_CBPY,   0x2 },
    {"1001",    SYM_CBPY,   0x3 },
    
    {"0001 1",  SYM_CBPY,   0x4 },
    {"0111",    SYM_CBPY,   0x5 },
    {"0000 10", SYM_CBPY,   0x6 },
    {"1011",    SYM_CBPY,   0x7 },
    
    {"0001 0",  SYM_CBPY,   0x8 },
    {"0000 11", SYM_CBPY,   0x9 },
    {"0101",    SYM_CBPY,   0xa },
    {"1010",    SYM_CBPY,   0xb },
    
    {"0100",    SYM_CBPY,   0xc },
    {"1000",    SYM_CBPY,   0xd },
    {"0110",    SYM_CBPY,   0xe },
    {"11",      SYM_CBPY,   0xf },
    {"End"}
};

static  struct vlc_entry dquant263[] = {
    {"00",      SYM_DQUANT, -1},
    {"01",      SYM_DQUANT, -2},
    {"10",      SYM_DQUANT, 1},
    {"11",      SYM_DQUANT, 2},
    {"End"}
};

static  struct vlc_entry mvd263[] = {
    {"0000 0",      SYM_ESCAPE, TAB263_LONG_MVD},
    
    {"0000 1001",   SYM_MVD,    -6},
    {"0000 1011",   SYM_MVD,    -5},
    
    {"0000 111",    SYM_MVD,    -4},
    {"0001 1",      SYM_MVD,    -3},
    {"0011",        SYM_MVD,    -2},
    {"011",         SYM_MVD,    -1},
    
    {"1",           SYM_MVD,    0},
    {"010",         SYM_MVD,    1},
    {"0010",        SYM_MVD,    2},
    {"0001 0",      SYM_MVD,    3},
    
    {"0000 110",    SYM_MVD,    4},
    {"0000 1010",   SYM_MVD,    5},
    {"0000 1000",   SYM_MVD,    6},
    {"End"}
};

static  struct vlc_entry long_mvd263[] = {  // Starts with "0000 0"
    {"000 0010 1",    SYM_MVD,    -32},
    {"000 0011 1",    SYM_MVD,    -31},
    {"000 0101",      SYM_MVD,    -30},
    {"000 0111",      SYM_MVD,    -29},
    
    {"000 1001",      SYM_MVD,    -28},
    {"000 1011",      SYM_MVD,    -27},
    {"000 1101",      SYM_MVD,    -26},
    {"000 1111",      SYM_MVD,    -25},
    
    {"001 001",       SYM_MVD,    -24},
    {"001 011",       SYM_MVD,    -23},
    {"001 101",       SYM_MVD,    -22},
    {"001 111",       SYM_MVD,    -21},
    
    {"010 001",       SYM_MVD,    -20},
    {"010 011",       SYM_MVD,    -19},
    {"010 101",       SYM_MVD,    -18},
    {"010 111",       SYM_MVD,    -17},
    
    {"011 001",       SYM_MVD,    -16},
    {"011 011",       SYM_MVD,    -15},
    {"011 101",       SYM_MVD,    -14},
    {"011 111",       SYM_MVD,    -13},
    
    {"100 001",       SYM_MVD,    -12},
    {"100 011",       SYM_MVD,    -11},
    {"100 11",        SYM_MVD,    -10},
    {"101 01",        SYM_MVD,    -9},
    
    {"101 11",        SYM_MVD,    -8},
    {"111",           SYM_MVD,    -7},

    {"110",           SYM_MVD,    7},
    
    {"101 10",        SYM_MVD,    8},
    {"101 00",        SYM_MVD,    9},
    {"100 10",        SYM_MVD,    10},
    {"100 010",       SYM_MVD,    11},
    
    {"100 000",       SYM_MVD,    12},
    {"011 110",       SYM_MVD,    13},
    {"011 100",       SYM_MVD,    14},
    {"011 010",       SYM_MVD,    15},
    
    {"011 000",       SYM_MVD,    16},
    {"010 110",       SYM_MVD,    17},
    {"010 100",       SYM_MVD,    18},
    {"010 010",       SYM_MVD,    19},
    
    {"010 000",       SYM_MVD,    20},
    {"001 110",       SYM_MVD,    21},
    {"001 100",       SYM_MVD,    22},
    {"001 010",       SYM_MVD,    23},
    
    {"001 000",       SYM_MVD,    24},
    {"000 1110",      SYM_MVD,    25},
    {"000 1100",      SYM_MVD,    26},
    {"000 1010",      SYM_MVD,    27},
    
    {"000 1000",      SYM_MVD,    28},
    {"000 0110",      SYM_MVD,    29},
    {"000 0100",      SYM_MVD,    30},
    {"000 0011 0",    SYM_MVD,    31},
    {"End"}
};

static  struct vlc_entry finished_263blk[] = {
    {" ",       SYM_EXIT,   FINISHED_LAST_BLOCK,    0},
    {"End"}
};

static  struct vlc_entry tcoef[] = {
    {"0001",        SYM_ESCAPE, TAB263_TCOEF_0001,      0},
    {"0000 1",      SYM_ESCAPE, TAB263_TCOEF_0000_1,    0},
    {"0000 0",      SYM_ESCAPE, TAB263_TCOEF_0000_0,    0},

    {"100",         0,      1,      0},
    {"101",         0,      -1,     0},
    {"1111 0",      0,      2,      0},
    {"1111 1",      0,      -2,     0},
    {"0101 010",    0,      3,      0},
    {"0101 011",    0,      -3,     0},
    {"0010 1110",   0,      4,      0},
    {"0010 1111",   0,      -4,     0},
    {"1100",        1,      1,      0},
    {"1101",        1,      -1,     0},
    {"0101 000",    1,      2,      0},
    {"0101 001",    1,      -2,     0},
    {"1110 0",      2,      1,      0},
    {"1110 1",      2,      -1,     0},
    {"0110 10",     3,      1,      0},
    {"0110 11",     3,      -1,     0},
    {"0110 00",     4,      1,      0},
    {"0110 01",     4,      -1,     0},
    {"0101 10",     5,      1,      0},
    {"0101 11",     5,      -1,     0},
    {"0100 110",    6,      1,      0},
    {"0100 111",    6,      -1,     0},
    {"0100 100",    7,      1,      0},
    {"0100 101",    7,      -1,     0},
    {"0100 010",    8,      1,      0},
    {"0100 011",    8,      -1,     0},
    {"0100 000",    9,      1,      0},
    {"0100 001",    9,      -1,     0},
    {"0010 1100",   10,     1,      0},
    {"0010 1101",   10,     -1,     0},
    {"0010 1010",   11,     1,      0},
    {"0010 1011",   11,     -1,     0},
    {"0010 1000",   12,     1,      0},
    {"0010 1001",   12,     -1,     0},

    {"0111 0",      0 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0111 1",      0 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 110",    1 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 111",    1 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 100",    2 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 101",    2 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 010",    3 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 011",    3 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 000",    4 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 001",    4 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0110",   5 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0111",   5 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0100",   6 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0101",   6 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0010",   7 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0011",   7 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0000",   8 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0001",   8 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0001[] = {
    {" 1111 0",     0,      5,      0},
    {" 1111 1",     0,      -5,     0},
    {" 0010 10",    0,      6,      0},
    {" 0010 11",    0,      -6,     0},
    {" 0010 00",    0,      7,      0},
    {" 0010 01",    0,      -7,     0},
    {" 1110 0",     1,      3,      0},
    {" 1110 1",     1,      -3,     0},
    {" 1101 0",     2,      2,      0},
    {" 1101 1",     2,      -2,     0},
    {" 0001 10",    3,      2,      0},
    {" 0001 11",    3,      -2,     0},
    {" 0001 00",    4,      2,      0},
    {" 0001 01",    4,      -2,     0},
    {" 1100 0",     13,     1,      0},
    {" 1100 1",     13,     -1,     0},
    {" 1011 0",     14,     1,      0},
    {" 1011 1",     14,     -1,     0},
    {" 0000 10",    15,     1,      0},
    {" 0000 11",    15,     -1,     0},
    {" 0000 00",    16,     1,      0},
    {" 0000 01",    16,     -1,     0},

    {" 1010 0",     9 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {" 1010 1",     9 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {" 1001 0",     10 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 1001 1",     10 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 1000 0",     11 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 1000 1",     11 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0111 0",     12 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0111 1",     12 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0110 0",     13 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0110 1",     13 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0101 0",     14 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0101 1",     14 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0100 0",     15 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0100 1",     15 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0011 0",     16 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0011 1",     16 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0000_1[] = {
    {"000 010",     0,      8,      0},
    {"000 011",     0,      -8,     0},
    {"000 000",     0,      9,      0},
    {"000 001",     0,      -9,     0},
    {"111 10",      17,     1,      0},
    {"111 11",      17,     -1,     0},
    {"111 00",      18,     1,      0},
    {"111 01",      18,     -1,     0},
    {"110 10",      19,     1,      0},
    {"110 11",      19,     -1,     0},
    {"110 00",      20,     1,      0},
    {"110 01",      20,     -1,     0},
    {"101 10",      21,     1,      0},
    {"101 11",      21,     -1,     0},
    {"101 00",      22,     1,      0},
    {"101 01",      22,     -1,     0},

    {"100 10",      0 + LAST263_RUNVAL,     2,      ST263_DIFF_LAST},
    {"100 11",      0 + LAST263_RUNVAL,     -2,     ST263_DIFF_LAST},
    {"100 00",      17 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 01",      17 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"011 10",      18 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"011 11",      18 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"011 00",      19 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"011 01",      19 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"010 10",      20 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"010 11",      20 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"010 00",      21 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"010 01",      21 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"001 10",      22 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 11",      22 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"001 00",      23 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 01",      23 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"000 10",      24 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"000 11",      24 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0000_0[] = {
    {"11",          SYM_ESCAPE, TAB263_ESC_RUN,     0},
    
    {"000 1110",    0,      10,     0},
    {"000 1111",    0,      -10,    0},
    {"000 1100",    0,      11,     0},
    {"000 1101",    0,      -11,    0},
    {"100 0000",    0,      12,     0},
    {"100 0001",    0,      -12,    0},
    {"011 110",     1,      4,      0},
    {"011 111",     1,      -4,     0},
    {"100 0010",    1,      5,      0},
    {"100 0011",    1,      -5,     0},
    {"101 0000 0",  1,      6,      0},
    {"101 0000 1",  1,      -6,     0},
    {"011 100",     2,      3,      0},
    {"011 101",     2,      -3,     0},
    {"101 0001 0",  2,      4,      0},
    {"101 0001 1",  2,      -4,     0},
    {"011 010",     3,      3,      0},
    {"011 011",     3,      -3,     0},
    {"101 0010 0",  4,      3,      0},
    {"101 0010 1",  4,      -3,     0},
    {"011 000",     5,      2,      0},
    {"011 001",     5,      -2,     0},
    {"101 0011 0",  5,      3,      0},
    {"101 0011 1",  5,      -3,     0},
    {"010 110",     6,      2,      0},
    {"010 111",     6,      -2,     0},
    {"101 0100 0",  6,      3,      0},
    {"101 0100 1",  6,      -3,     0},
    {"010 100",     7,      2,      0},
    {"010 101",     7,      -2,     0},
    {"010 010",     8,      2,      0},
    {"010 011",     8,      -2,     0},
    {"010 000",     9,      2,      0},
    {"010 001",     9,      -2,     0},
    {"101 0101 0",  10,     2,      0},
    {"101 0101 1",  10,     -2,     0},
    {"100 0100",    23,     1,      0},
    {"100 0101",    23,     -1,     0},
    {"100 0110",    24,     1,      0},
    {"100 0111",    24,     -1,     0},
    {"101 0110 0",  25,     1,      0},
    {"101 0110 1",  25,     -1,     0},
    {"101 0111 0",  26,     1,      0},
    {"101 0111 1",  26,     -1,     0},

    {"000 1010",    0 + LAST263_RUNVAL,     3,      ST263_DIFF_LAST},
    {"000 1011",    0 + LAST263_RUNVAL,     -3,     ST263_DIFF_LAST},
    {"000 1000",    1 + LAST263_RUNVAL,     2,      ST263_DIFF_LAST},
    {"000 1001",    1 + LAST263_RUNVAL,     -2,     ST263_DIFF_LAST},
    {"001 110",     25 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 111",     25 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"001 100",     26 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 101",     26 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"001 010",     27 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 011",     27 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"001 000",     28 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"001 001",     28 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"100 1000",    29 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 1001",    29 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"100 1010",    30 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 1011",    30 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"100 1100",    31 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 1101",    31 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"100 1110",    32 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 1111",    32 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1000 0",  33 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1000 1",  33 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1001 0",  34 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1001 1",  34 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1010 0",  35 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1010 1",  35 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1011 0",  36 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1011 1",  36 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1100 0",  37 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1100 1",  37 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1101 0",  38 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1101 1",  38 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1110 0",  39 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1110 1",  39 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1111 0",  40 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1111 1",  40 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry esc263_run[] = {
    {"FLC"},
    {"0 00 0000",   SYM_ESC_RUN,    0,  -ST263_DIFF_ESC_LEVEL,  63},
    {"1 00 0000",   SYM_ESC_RUN,
                    0 + LAST263_RUNVAL,
                    ST263_DIFF_LAST - ST263_DIFF_ESC_LEVEL,
                    63 + LAST263_RUNVAL},
    {"End"}
};

/* ESC_LEVEL: Levels 0 and -128 are not allowed */
static  struct vlc_entry esc263_level[] = {
    {"FLC"},
    {"0000 0001",   SYM_ESC_LEVEL,      1,  ST263_DIFF_ESC_LEVEL,   127},
    {"1000 0001",   SYM_ESC_LEVEL,   -127,  ST263_DIFF_ESC_LEVEL,   -1},
    {"End"}
};

// INTRA DC coeff:  Levels 0 and 255 not allowed; level 128 repr. by 255
// Indicate run=0 by setting type=0
static  struct vlc_entry intra263_dc[] = {
    {"FLC"},
    {"0000 0001",   0,      1,      ST263_DIFF_INTRA_DC,    127},
    {"1111 1111",   0,      128,    ST263_DIFF_INTRA_DC,    128},
    {"1000 0001",   0,      129,    ST263_DIFF_INTRA_DC,    254},
    {"End"}
};


////////////////  Tables for H.263+  //////////////////

static  struct vlc_entry modb263plus[] = {
    {"0",   SYM_MODB,   0},
    {"10",  SYM_MODB,   1},
    {"110", SYM_MODB,   2},
    {"111", SYM_MODB,   3},
    {"End"}
};

static  struct vlc_entry intra_mode263plus[] = {
    {"0",   SYM_INTRA_MODE,   0},
    {"10",  SYM_INTRA_MODE,   1},
    {"11",  SYM_INTRA_MODE,   2},
    {"End"}
};


static  struct vlc_entry tcoef_plus[] = {
    {"0001",        SYM_ESCAPE, TAB263PLUS_TCOEF_0001,      0},
    {"0000 1",      SYM_ESCAPE, TAB263PLUS_TCOEF_0000_1,    0},
    {"0000 0",      SYM_ESCAPE, TAB263PLUS_TCOEF_0000_0,    0},

    {"100",         0,      1,      0},
    {"101",         0,      -1,     0},
    {"1111 0",      1,      1,      0},
    {"1111 1",      1,      -1,     0},
    {"0101 010",    3,      1,      0},
    {"0101 011",    3,      -1,     0},
    {"0010 1110",   5,      1,      0},
    {"0010 1111",   5,      -1,     0},
    {"1100",        0,      2,      0},
    {"1101",        0,      -2,     0},
    {"0101 000",    1,      2,      0},
    {"0101 001",    1,      -2,     0},
    {"1110 0",      0,      3,      0},
    {"1110 1",      0,      -3,     0},
    {"0110 10",     0,      5,      0},
    {"0110 11",     0,      -5,     0},
    {"0110 00",     0,      4,      0},
    {"0110 01",     0,      -4,     0},
    {"0101 10",     2,      1,      0},
    {"0101 11",     2,      -1,     0},
    {"0100 110",    4,      1,      0},
    {"0100 111",    4,      -1,     0},
    {"0100 100",    0,      8,      0},
    {"0100 101",    0,      -8,     0},
    {"0100 010",    0,      7,      0},
    {"0100 011",    0,      -7,     0},
    {"0100 000",    0,      6,      0},
    {"0100 001",    0,      -6,     0},
    {"0010 1100",   0,      9,      0},
    {"0010 1101",   0,      -9,     0},
    {"0010 1010",   2,      2,      0},
    {"0010 1011",   2,      -2,     0},
    {"0010 1000",   1,      3,      0},
    {"0010 1001",   1,      -3,     0},

    {"0111 0",      0 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0111 1",      0 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 110",    1 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 111",    1 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 100",    2 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 101",    2 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 010",    3 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0011 011",    3 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0011 000",    0 + LAST263_RUNVAL,     2,      ST263_DIFF_LAST},
    {"0011 001",    0 + LAST263_RUNVAL,     -2,     ST263_DIFF_LAST},
    {"0010 0110",   5 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0111",   5 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0100",   6 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0101",   6 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0010",   4 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {"0010 0011",   4 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {"0010 0000",   0 + LAST263_RUNVAL,     3,      ST263_DIFF_LAST},
    {"0010 0001",   0 + LAST263_RUNVAL,     -3,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0001_plus[] = {
    {" 1111 0",     7,      1,      0},
    {" 1111 1",     7,      -1,     0},
    {" 0010 10",    8,      1,      0},
    {" 0010 11",    8,      -1,     0},
    {" 0010 00",    9,      1,      0},
    {" 0010 01",    9,      -1,     0},
    {" 1110 0",     1,      4,      0},
    {" 1110 1",     1,      -4,     0},
    {" 1101 0",     3,      2,      0},
    {" 1101 1",     3,      -2,     0},
    {" 0001 10",    4,      2,      0},
    {" 0001 11",    4,      -2,     0},
    {" 0001 00",    5,      2,      0},
    {" 0001 01",    5,      -2,     0},
    {" 1100 0",     6,      1,      0},
    {" 1100 1",     6,      -1,     0},
    {" 1011 0",     0,      10,     0},
    {" 1011 1",     0,      -10,    0},
    {" 0000 10",    0,      12,     0},
    {" 0000 11",    0,      -12,    0},
    {" 0000 00",    0,      11,     0},
    {" 0000 01",    0,      -11,    0},

    {" 1010 0",     9 + LAST263_RUNVAL,     1,      ST263_DIFF_LAST},
    {" 1010 1",     9 + LAST263_RUNVAL,     -1,     ST263_DIFF_LAST},
    {" 1001 0",     10 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 1001 1",     10 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 1000 0",     11 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 1000 1",     11 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0111 0",     12 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0111 1",     12 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0110 0",     13 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0110 1",     13 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0101 0",      8 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0101 1",      8 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0100 0",      7 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {" 0100 1",      7 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {" 0011 0",      0 + LAST263_RUNVAL,    4,      ST263_DIFF_LAST},
    {" 0011 1",      0 + LAST263_RUNVAL,    -4,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0000_1_plus[] = {
    {"000 010",     10,     1,      0},
    {"000 011",     10,     -1,     0},
    {"000 000",     11,     1,      0},
    {"000 001",     11,     -1,     0},
    {"111 10",       0,     18,     0},
    {"111 11",       0,     -18,    0},
    {"111 00",       0,     17,     0},
    {"111 01",       0,     -17,    0},
    {"110 10",       0,     16,     0},
    {"110 11",       0,     -16,    0},
    {"110 00",       0,     15,     0},
    {"110 01",       0,     -15,    0},
    {"101 10",       0,     14,     0},
    {"101 11",       0,     -14,    0},
    {"101 00",       0,     13,     0},
    {"101 01",       0,     -13,    0},

    {"100 10",      14 + LAST263_RUNVAL,     1,     ST263_DIFF_LAST},
    {"100 11",      14 + LAST263_RUNVAL,     -1,    ST263_DIFF_LAST},
    {"100 00",      17 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"100 01",      17 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"011 10",      18 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"011 11",      18 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"011 00",      16 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"011 01",      16 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"010 10",      15 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"010 11",      15 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"010 00",       2 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"010 01",       2 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"001 10",       1 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"001 11",       1 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"001 00",       0 + LAST263_RUNVAL,    6,      ST263_DIFF_LAST},
    {"001 01",       0 + LAST263_RUNVAL,    -6,     ST263_DIFF_LAST},
    {"000 10",       0 + LAST263_RUNVAL,    5,      ST263_DIFF_LAST},
    {"000 11",       0 + LAST263_RUNVAL,    -5,     ST263_DIFF_LAST},
    {"End"}
};

static  struct vlc_entry tcoef_0000_0_plus[] = {
    {"11",          SYM_ESCAPE, TAB263_ESC_RUN,     0},
    
    {"000 1110",    4,       3,    0},
    {"000 1111",    4,       -3,   0},
    {"000 1100",    9,      2,     0},
    {"000 1101",    9,      -2,    0},
    {"100 0000",   13,      1,     0},
    {"100 0001",   13,      -1,    0},
    {"011 110",     1,      5,      0},
    {"011 111",     1,      -5,     0},
    {"100 0010",    1,      6,      0},
    {"100 0011",    1,      -6,     0},
    {"101 0000 0",  1,      7,      0},
    {"101 0000 1",  1,      -7,     0},
    {"011 100",     2,      3,      0},
    {"011 101",     2,      -3,     0},
    {"101 0001 0",  3,      4,      0},
    {"101 0001 1",  3,      -4,     0},
    {"011 010",     3,      3,      0},
    {"011 011",     3,      -3,     0},
    {"101 0010 0",  5,      3,      0},
    {"101 0010 1",  5,      -3,     0},
    {"011 000",     6,      2,      0},
    {"011 001",     6,      -2,     0},
    {"101 0011 0",  0,      25,      0},
    {"101 0011 1",  0,      -25,     0},
    {"010 110",     7,      2,      0},
    {"010 111",     7,      -2,     0},
    {"101 0100 0",  0,      24,      0},
    {"101 0100 1",  0,      -24,     0},
    {"010 100",     8,      2,      0},
    {"010 101",     8,      -2,     0},
    {"010 010",     2,      4,      0},
    {"010 011",     2,      -4,     0},
    {"010 000",    12,      1,      0},
    {"010 001",    12,      -1,     0},
    {"101 0101 0",  0,     23,      0},
    {"101 0101 1",  0,     -23,     0},
    {"100 0100",    0,     20,      0},
    {"100 0101",    0,     -20,     0},
    {"100 0110",    0,     19,      0},
    {"100 0111",    0,     -19,     0},
    {"101 0110 0",  0,     22,      0},
    {"101 0110 1",  0,     -22,     0},
    {"101 0111 0",  0,     21,      0},
    {"101 0111 1",  0,     -21,     0},

    {"000 1010",    20 + LAST263_RUNVAL,     1,     ST263_DIFF_LAST},
    {"000 1011",    20 + LAST263_RUNVAL,     -1,    ST263_DIFF_LAST},
    {"000 1000",    19 + LAST263_RUNVAL,     1,     ST263_DIFF_LAST},
    {"000 1001",    19 + LAST263_RUNVAL,     -1,    ST263_DIFF_LAST},
    {"001 110",      4 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"001 111",      4 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"001 100",      3 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"001 101",      3 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"001 010",      1 + LAST263_RUNVAL,    3,      ST263_DIFF_LAST},
    {"001 011",      1 + LAST263_RUNVAL,    -3,     ST263_DIFF_LAST},
    {"001 000",      0 + LAST263_RUNVAL,    7,      ST263_DIFF_LAST},
    {"001 001",      0 + LAST263_RUNVAL,    -7,     ST263_DIFF_LAST},
    {"100 1000",     2 + LAST263_RUNVAL,    3,      ST263_DIFF_LAST},
    {"100 1001",     2 + LAST263_RUNVAL,    -3,     ST263_DIFF_LAST},
    {"100 1010",     1 + LAST263_RUNVAL,    4,      ST263_DIFF_LAST},
    {"100 1011",     1 + LAST263_RUNVAL,    -4,     ST263_DIFF_LAST},
    {"100 1100",     0 + LAST263_RUNVAL,    9,      ST263_DIFF_LAST},
    {"100 1101",     0 + LAST263_RUNVAL,    -9,     ST263_DIFF_LAST},
    {"100 1110",     0 + LAST263_RUNVAL,    8,      ST263_DIFF_LAST},
    {"100 1111",     0 + LAST263_RUNVAL,    -8,     ST263_DIFF_LAST},
    {"101 1000 0",  21 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1000 1",  21 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1001 0",  22 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1001 1",  22 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1010 0",  23 + LAST263_RUNVAL,    1,      ST263_DIFF_LAST},
    {"101 1010 1",  23 + LAST263_RUNVAL,    -1,     ST263_DIFF_LAST},
    {"101 1011 0",   7 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"101 1011 1",   7 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"101 1100 0",   6 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"101 1100 1",   6 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"101 1101 0",   5 + LAST263_RUNVAL,    2,      ST263_DIFF_LAST},
    {"101 1101 1",   5 + LAST263_RUNVAL,    -2,     ST263_DIFF_LAST},
    {"101 1110 0",   3 + LAST263_RUNVAL,    3,      ST263_DIFF_LAST},
    {"101 1110 1",   3 + LAST263_RUNVAL,    -3,     ST263_DIFF_LAST},
    {"101 1111 0",   0 + LAST263_RUNVAL,    10,     ST263_DIFF_LAST},
    {"101 1111 1",   0 + LAST263_RUNVAL,    -10,    ST263_DIFF_LAST},
    {"End"}
};


#endif
