/*-----------------------------------------------------------------------------
 *	VLDSTATE.H
 *
 *	DESCRIPTION
 *		vldstate.h - Define states for variable length decoder
 *
 *	Author:		Staffan Ericsson	6/28/93
 *	Inspector:	Mary Deshon			7/6/93
 *	Revised:
 *  1/31/97     dblair          Added states for H.263+ decoding
 *  7/9/95      S Ericsson      Added states for H.263 decoding
 *	7/6/93		Mary Deshon		Minor style changes
 *
 *	(c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
 
#include "h263plus.h"

#ifndef _INC_VLDSTATE
#define _INC_VLDSTATE	1

/* Define states for bitstream decoder */
#define ST_DIFF_QUANT         (-4)  /* Decrease state by 4 after decoding QUANT */
#define ST_DIFF_ESC_LEVEL     (-28) /* Decrease state by 28 after decoding ESC_LEVEL */
#define ST_MC_NOCBP_MVDX        1
#define ST_MC_NOCBP_MVDY        2
#define ST_MBA_STARTCODE        3
#define ST_NEXT_BLK_6           4
#define ST_FIRST_BLK_6          5
#define ST_NEXT_BLK_5           6
#define ST_FIRST_BLK_5          7
#define ST_NEXT_BLK_4           8
#define ST_FIRST_BLK_4          9
#define ST_NEXT_BLK_3           10
#define ST_FIRST_BLK_3          11
#define ST_NEXT_BLK_2           12
#define ST_FIRST_BLK_2          13
#define ST_NEXT_BLK_1           14
#define ST_FIRST_BLK_1          15
#define ST_INTRA_DC_BLK_6       16
#define ST_INTRA_AC_BLK_5       17
#define ST_INTRA_DC_BLK_5       18
#define ST_INTRA_AC_BLK_4       19
#define ST_INTRA_DC_BLK_4       20
#define ST_INTRA_AC_BLK_3       21
#define ST_INTRA_DC_BLK_3       22
#define ST_INTRA_AC_BLK_2       23
#define ST_INTRA_DC_BLK_2       24
#define ST_INTRA_AC_BLK_1       25
#define ST_INTRA_DC_BLK_1       26
#define ST_MC_CBP_MVDX          27
#define ST_MC_CBP_MVDY          28
#define ST_CBP                  29
#define ST_INTRA_MQUANT         (ST_INTRA_DC_BLK_1 - ST_DIFF_QUANT)
#define ST_MC_CBP_MQUANT        (ST_MC_CBP_MVDX - ST_DIFF_QUANT)
#define ST_ESC_BLK_6            (ST_NEXT_BLK_6 - ST_DIFF_ESC_LEVEL)
#define ST_INTER_MQUANT         (ST_CBP - ST_DIFF_QUANT)
#define ST_ESC_BLK_5            (ST_NEXT_BLK_5 - ST_DIFF_ESC_LEVEL)
#define ST_MTYPE                35
#define ST_ESC_BLK_4            (ST_NEXT_BLK_4 - ST_DIFF_ESC_LEVEL)
#define ST_GEI_PEI              37
#define ST_ESC_BLK_3            (ST_NEXT_BLK_3 - ST_DIFF_ESC_LEVEL)
#define ST_PTYPE                39
#define ST_ESC_BLK_2            (ST_NEXT_BLK_2 - ST_DIFF_ESC_LEVEL)
#define ST_GQUANT               41
#define ST_ESC_BLK_1            (ST_NEXT_BLK_1 - ST_DIFF_ESC_LEVEL)
#define ST_TR                   43
#define ST_AFTER_STARTCODE      44
#define ST_ESC_INTRA_5          (ST_INTRA_AC_BLK_5 - ST_DIFF_ESC_LEVEL)
                                /* State 46 is not used */
#define ST_ESC_INTRA_4          (ST_INTRA_AC_BLK_4 - ST_DIFF_ESC_LEVEL)
                                /* State 48 is not used */
#define ST_ESC_INTRA_3          (ST_INTRA_AC_BLK_3 - ST_DIFF_ESC_LEVEL)
                                /* State 50 is not used */
#define ST_ESC_INTRA_2          (ST_INTRA_AC_BLK_2 - ST_DIFF_ESC_LEVEL)
                                /* State 52 is not used */
#define ST_ESC_INTRA_1          (ST_INTRA_AC_BLK_1 - ST_DIFF_ESC_LEVEL)

// Definitions for H.263
#define ST263_BASE              (54)
#define ST263_DIFF_ESC_LEVEL    (-1)    // Decrease state by 1 after decoding ESC_LEVEL
#define ST263_DIFF_INTRA_DC     (-14)   // Decrease state by 14 after decoding INTRA-DC
#define ST263_DIFF_LAST         (-2)    // Decrease state by 2 after decoding TCOEF with LAST=1

#define ST263_FINISHED          (ST263_BASE + 0)
#define ST263_ESC_FINISHED      (ST263_FINISHED - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_6             (ST263_FINISHED - ST263_DIFF_LAST)
#define ST263_ESC_BLK6          (ST263_BLK_6 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_5             (ST263_BLK_6 - ST263_DIFF_LAST)
#define ST263_ESC_BLK5          (ST263_BLK_5 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_4             (ST263_BLK_5 - ST263_DIFF_LAST)
#define ST263_ESC_BLK4          (ST263_BLK_4 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_3             (ST263_BLK_4 - ST263_DIFF_LAST)
#define ST263_ESC_BLK3          (ST263_BLK_3 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_2             (ST263_BLK_3 - ST263_DIFF_LAST)
#define ST263_ESC_BLK2          (ST263_BLK_2 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_1             (ST263_BLK_2 - ST263_DIFF_LAST)
#define ST263_ESC_BLK1          (ST263_BLK_1 - ST263_DIFF_ESC_LEVEL)
#define ST263_INTRA_DC_ONLY     (ST263_FINISHED - ST263_DIFF_INTRA_DC)
                                // State 69 is not used
#define ST263_INTRA_DC_AC       (ST263_BLK_6 - ST263_DIFF_INTRA_DC)

#ifdef DO_H263_PLUS

#define ST263PLUS_BASE              (ST263_INTRA_DC_AC + 1)

#define ST263PLUS_FINISHED          (ST263PLUS_BASE + 0)
#define ST263PLUS_ESC_FINISHED      (ST263PLUS_FINISHED - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_6             (ST263PLUS_FINISHED - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK6          (ST263PLUS_BLK_6 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_5             (ST263PLUS_BLK_6 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK5          (ST263PLUS_BLK_5 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_4             (ST263PLUS_BLK_5 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK4          (ST263PLUS_BLK_4 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_3             (ST263PLUS_BLK_4 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK3          (ST263PLUS_BLK_3 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_2             (ST263PLUS_BLK_3 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK2          (ST263PLUS_BLK_2 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_1             (ST263PLUS_BLK_2 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK1          (ST263PLUS_BLK_1 - ST263_DIFF_ESC_LEVEL)

#define NUMSTATES               (ST263PLUS_BASE + 15)

#else
#define NUMSTATES               (ST263_BASE + 17)
#endif
#endif
