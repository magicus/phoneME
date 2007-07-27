/*-----------------------------------------------------------------------------
 *	VLDTABS.H
 *
 *	DESCRIPTION
 *		vldtabs.h - Define table numbering for variable length decoding tables;
 *                  previously part of VLD.H
 *
 *	Author:		Staffan Ericsson	7/8/95
 *	Inspector:	<not inspected yet>
 *	Revised:
 *  02/10/97    DB  Added tables for H.263+ (advanced intra mode and improved PB frames)
 *  11/10/95    SE  Allow up to 7 zeros before H.263 startcodes
 *
 *	(c) 1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _INC_VLDTABS
#define _INC_VLDTABS    1

#include "h263plus.h"

/* Decode tables */
#define TAB_DCT_NEXT        0
#define TAB_DCT_FIRST       1
#define TAB_INTRA_DC        2
#define TAB_LAST_INTRA_DC   3
#define TAB_MBA_STARTCODE   4
#define TAB_MTYPE           5
#define TAB_MVD             6
#define TAB_CBP             7
#define TAB_QUANT_TR        8
#define TAB_GEI_PEI         9
#define TAB_GN              10
#define TAB_PTYPE           11
#define TAB_ESCAPE_LEVEL    12
#define TAB_ILLEGAL_STATE   13

/* Decode tables for codewords longer than 8 bits */
#define TAB_DCT_00100       14
#define TAB_DCT_000000      15
#define TAB_ESCAPE_RUN      16
#define TAB_LONG_MBA        17
#define TAB_LONG_STARTCODE  18
#define TAB_LONG_MTYPE      19
#define TAB_LONG_MVD        20
#define TAB_LONG_CBP        21
#define TAB_LONG_SPARE      22

// Decode tables for H.263
#define TAB263_BASE             (23)
#define TAB263_TCOEF            (TAB263_BASE + 0)
#define TAB263_ESCAPE_LEVEL     (TAB263_BASE + 1)
#define TAB263_INTRA_DC         (TAB263_BASE + 2)
#define TAB263_MCBPC_INTRA      (TAB263_BASE + 3)
#define TAB263_MCBPC_INTER      (TAB263_BASE + 4)
#define TAB263_MODB             (TAB263_BASE + 5)

#define TAB263_CBPY             (TAB263_BASE + 6)
#define TAB263_CBPY_INTRA       (TAB263_BASE + 7)
#define TAB263_DQUANT           (TAB263_BASE + 8)
#define TAB263_MVD              (TAB263_BASE + 9)
#define TAB263_FINISHED         (TAB263_BASE + 10)

// Decode tables for codewords longer than 8 bits
#define TAB263_TCOEF_0001       (TAB263_BASE + 11)
#define TAB263_TCOEF_0000_1     (TAB263_BASE + 12)
#define TAB263_TCOEF_0000_0     (TAB263_BASE + 13)
#define TAB263_ESC_RUN          (TAB263_BASE + 14)
#define TAB263_LONG_MCBPC_INTRA (TAB263_BASE + 15)
#define TAB263_LONG_MCBPC_INTER (TAB263_BASE + 16)
#define TAB263_LONG_STARTCODE   (TAB263_BASE + 17)
#define TAB263_LONG_MVD         (TAB263_BASE + 18)
#define TAB263_ZEROS_AND_START  (TAB263_BASE + 19)

#ifdef DO_H263_PLUS
// Decode table to support improved PB frame mode
#define TAB263PLUS_MODB		(TAB263_BASE + 20)

// New VLC tables for H.263+
#define TAB263PLUS_TCOEF        (TAB263_BASE + 21)
#define TAB263PLUS_TCOEF_0001   (TAB263_BASE + 22)
#define TAB263PLUS_TCOEF_0000_1 (TAB263_BASE + 23)
#define TAB263PLUS_TCOEF_0000_0 (TAB263_BASE + 24)

// Table to support advanced intra mode
#define TAB263PLUS_INTRA_MODE   (TAB263_BASE + 25)

#define NUMTABS					(TAB263_BASE + 26)

#else 

#define NUMTABS                 (TAB263_BASE + 20)

#endif // DO_H263_PLUS

// H.263: indicate last RL-pair by adding 64 to RUN
#define LAST263_RUNVAL      (64)

/* Reasons for exit */
#define OUT_OF_BITS         (0)
#define ILLEGAL_SYMBOL      (-1)
#define ILLEGAL_STATE       (-2)
#define FINISHED_LAST_BLOCK (-3)
#define QUANT_OUT_OF_BOUNDS (-4)
#define UNKNOWN_MTYPE       (-5)
#define BITSTREAM_ERROR     (-6)

#endif
