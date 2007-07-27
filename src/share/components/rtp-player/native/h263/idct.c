/*-----------------------------------------------------------------------------
 *  IDCT.C
 *
 *  DESCRIPTION
 *      idct.c - Inverse DCT on 8x8 block
 *      Goertzel Inverse DCT using ideas by Bernd and Staffan
 *      Implementation assumes a 32-bit ALU with 2's complement arithmetic
 *
 *  Author:     Staffan Ericsson    6/7/93
 *  Inspector:  Mary Deshon         7/6/93
 *  Revised:
 *  02/10/97    D Blair     Added advanced intra mode support
 *  01/24/97    S Ericsson  Added Idct2_s16 to support Reduced-res. update mode
 *  05/13/96    wolfe       Fixed a Win16 WATCOM 10.5 compiler warning
 *  04/19/96    wolfe       Fixed some Win16 WATCOM 10.5 warnings
 *  12/21/95    wolfe       Fixed some compiler warnings (MSVC 4.0)
 *  02/07/95    G. Su       Modify how block types are identified in Idct2 and 
 *                          Idct2Sum
 *  02/06/95    G. Su       Define the types of 8x8 blocks in Idct2 and Idct2Sum
 *  02/02/95    G. Su       Move the clipping operations in Idct2 and Idct2Sum
 *                          into separate files.
 *  01/27/95    G. Su       Minimize write operations in the clipping
 *                          mechanism of Idct2 and Idct2Sum.  Also consolidate
 *                          the final butterfly stage and the proper clipping
 *                          stage for certain special cases.
 *  01/19/95    G. Su       Optimized the initialization/TLU mechanisms in 
 *                          idct2_goertzel.
 *  01/13/95    G. Su       Modified the final butterfly stage in idct2_goertzel.
 *  12/13/94    B. Girod    Added clean to idct2_goertzel, idct2, and Idct2Sum.
 *  06/21/94 mjf            Added some forward declarations for MS compiler.
 *  4/12/94 S. Ericsson     Tweak IDCT tables to handle PictureTel S-4000 IDCT 
 *                          mismatch problem for rec. level +/-3 (index=1) (bug #250)
 *  6/26/93 S. Ericsson     Fixed Idct2 to handle INTRA DC coeff properly.
 *  6/27/93 S. Ericsson     Incorporated routines to generate inverse quant. tables
 *  6/30/93 S. Ericsson     Removed unnecessary function protos
 *  7/6/93  M. Deshon       Changed to use S8, U8, ... ,S32, U32 
 *  7/20/93 S. Ericsson     Error messages displayed via H261ErrMsg
 *  7/26/93 S Ericsson      Removed "exit" calls
 *  10/31/93    S Ericsson  Speed up reconstruction of first 3 coeffs
 *  
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "clip.h"
#include "h263plus.h"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (x) = (x)
#endif

extern S16 Recon [QUANT_MAX - QUANT_MIN + 1] [N_SYM_INDICES];

//  Function prototypes
static void init_inv_dct (void);
static void Fix_PTel_S4000_mismatch( void );
static void truncate_more( S32 * idct_tab_entry );
static void idct2_goertzel( SYMBOL sym[], int nsym, S32 x[8][4],
                            S16 recon[], int intra, int clean, int idct_class );
/* never defined
static int idct2_energy_test( SYMBOL sym[], int nsym,
                            S16 recon[] );                            
*/
static void recon_intra_dc( U8 index, S32 vec[8]);
static void recon_dc( S32 y, S32 vec[8]);
static void recon_hor_ac( S32 y, S32 vec[8]);
static void recon_vert_ac( S32 y, S32 vec[8]);
static void update( S32 x[8], S32 index, S32 table[][8*8*8]);
static double dctfunc (int freq, int index);
static S32 dct_tab_entry (double x, double y);
static S32 combine (S32 iy, S32 ix);

extern void idct2_advanced_intra( SYMBOL sym[], int nsym, S32 x[8][4], S16 recon[],
                            U8 rDCpred, S8 rACpred[8], U8 rDCstore[1], S8 rACstore[8], 
                            U8 cDCpred, S8 cACpred[8], U8 cDCstore[1], S8 cACstore[8],
                            int predtype, int fixedDC, int leftBoundary, int upperBoundary);

#define PI              3.141592654
#define FRACBITS        6   /* Fractional bits in IDCT computation */
#define SCALE_FACTOR    64. /* 2**FRACBITS */
#define MAX_DCT_INDEX   1024
#define IDCT_NTAB1_BITS 5
#define IDCT_NTAB1_SIZE 32  /* 2**NTAB1_BITS entries in table for small values */
#define IDCT_NTAB2_SIZE ((MAX_DCT_INDEX - 1) / IDCT_NTAB1_SIZE)
                            /* Entries to handle values > NTAB1_SIZE */
#define PIXEL_MIN       0
#define PIXEL_MAX       255
#define CLIPMARGIN      300
#define CLIPMIN         (PIXEL_MIN - CLIPMARGIN)
#define CLIPMAX         (PIXEL_MAX + CLIPMARGIN)
#define N_DCT_INDEX     1024


//  Reconstruction levels for QUANT = 1,2,..,31
S16 Recon [QUANT_MAX - QUANT_MIN + 1] [N_SYM_INDICES];

//  Define zigzag scanning pattern
static int ZigZag[64] = {  0,  1,  5,  6,  14, 15, 27, 28,
                            2,  4,  7,  13, 16, 26, 29, 42,
                            3,  8,  12, 17, 25, 30, 41, 43,
                            9,  11, 18, 24, 31, 40, 44, 53,
                            10, 19, 23, 32, 39, 45, 52, 54,
                            20, 22, 33, 38, 46, 51, 55, 60,
                            21, 34, 37, 47, 50, 56, 59, 61,
                            35, 36, 48, 49, 57, 58, 62, 63
};
static  int InvZZ[64];

static  S32    idct_tab [(IDCT_NTAB1_SIZE + IDCT_NTAB2_SIZE)][8*8][8];
                    /* LUT for all coeffs; [amplitude][zigzag position][] */
static  S32    dc_tab [MAX_DCT_INDEX];         /* LUT for DC coeff */
static  S32    hor_ac_tab [MAX_DCT_INDEX][2];  /* LUT for 1st hor AC coeff */
static  S32    vert_ac_tab [MAX_DCT_INDEX][4]; /* LUT for 1st vert AC coeff */
static  S32    intra_dc_tab[N_SYM_INDICES];    /* LUT for INTRA DC coeff */
PIXEL   clip[(CLIPMAX-CLIPMIN+1)];      /* LUT to limit to 0-255 */
static  int     even_odd_index[8*8];    /* Classify zigzag pos. as even or odd */


extern void InitReconTables( void )
{
    int level, index, quant;

//  QUANT=1 => 3,5,7,...
//  QUANT=2 => 5,9,13,...
//  QUANT=3 => 9,15,21,...
//  QUANT=4 => 11,19,27,...

    for (quant = QUANT_MIN; quant <= QUANT_MAX; quant++) {
        index = (quant + 1) / 2;
        for (level = 1; level < N_SYM_INDICES/2; level++) {
            index += quant;
            index = min( index, N_DCT_INDEX);
            Recon [quant - QUANT_MIN] [level] = index;
            Recon [quant - QUANT_MIN] [N_SYM_INDICES-level] = -index;
        }
        Recon [quant - QUANT_MIN] [0] = 0;
    }
    /* Generate zigzag table */
    for (index = 0; index < 64; index++) {
        InvZZ[ ZigZag[index] ] = index;
    }
    init_inv_dct();
    return;
}


//  Idct2 - Reconstruct DCT coeffs, perform IDCT, and clip to allowed pixel range */
//  Requires nsym > 0
extern void Idct2( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[], int clean)
{
    union {
        S16   bshort[8][8];
        S32    blong[8][4];
    } block;    /* Output from IDCT */
    int     intra;
    int     idct_class;

    idct_class = GENERAL;   /* assume the general case */

    /* look for situations involving specific involving 1,2,3 symbols */
    if (sym[0].type==0) 
        {
            switch (nsym) 
            {
                case 1:
                    idct_class = DC_ONLY;
                break;
                case 2:
                    if (sym[1].type == 0)  
                        idct_class = DC_AC_H;
                    if (sym[1].type == 1)  
                        idct_class = DC_AC_V;
                break;
                case 3:
                    if ( (sym[1].type | sym[2].type) == 0) 
                        idct_class = DC_3;
                break;
            } 
        }

    intra = YES;

    switch (idct_class)
            {
                case DC_ONLY:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_ONLY);
                    idct2_clip(x, xdim, block.blong, DC_ONLY);
                    break;
                }               
                case DC_AC_H:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_AC_H);
                    idct2_clip(x, xdim, block.blong, DC_AC_H);
                    break;
                }               
                case DC_AC_V:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_AC_V);
                    idct2_clip(x, xdim, block.blong, GENERAL);
                    break;
                }               
                case DC_3:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_3);
                    idct2_clip(x, xdim, block.blong, GENERAL);
                    break;
                }               
                default:                
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, GENERAL);
                    idct2_clip(x, xdim, block.blong, GENERAL);
                }               
            }               

    return;
}

//  Idct2Sum - Reconstruct DCT coeffs, perform IDCT, add to predition,
//  and clip to allowed pixel range.  Requires nsym > 0
extern void Idct2Sum( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[],int clean)
{
    union {
        S16   bshort[8][8];
        S32    blong[8][4];
    } block;    /* Output from IDCT */
    int     intra;
    int     idct_class;
    
    idct_class = GENERAL;   /* assume the general case */

    /* look for situations involving specific involving 1,2,3 symbols */
    if (sym[0].type==0) 
        {
            switch (nsym) 
            {
                case 1:
                    idct_class = DC_ONLY;
                break;
                case 2:
                    if (sym[1].type == 0)  
                        idct_class = DC_AC_H;
                    if (sym[1].type == 1)  
                        idct_class = DC_AC_V;
                break;
                case 3:
                    if ( (sym[1].type | sym[2].type) == 0) 
                        idct_class = DC_3;
                break;
            } 
        }

    intra = NO;
//    if (clean == YES || (idct2_energy_test( sym, nsym, recon) > 20)){ 
// activate this for sleazy IDCT in decoder
//    
    switch (idct_class)
            {
                case DC_ONLY:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_ONLY);
                    idct2sum_clip(x, xdim, block.blong, DC_ONLY);
                    break;
                }               
                case DC_AC_H:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_AC_H);
                    idct2sum_clip(x, xdim, block.blong, DC_AC_H);
                    break;
                }               
                case DC_AC_V:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_AC_V);
                    idct2sum_clip(x, xdim, block.blong, GENERAL);
                    break;
                }               
                case DC_3:
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, DC_3);
                    idct2sum_clip(x, xdim, block.blong, GENERAL);
                    break;
                }               
                default:                
                {   
                    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, GENERAL);
                    idct2sum_clip(x, xdim, block.blong, GENERAL);
                }               
            }
 //   }         // activate this for sleazy IDCT in decoder
    return;
}

//  Idct2_s16 - Reconstruct DCT coeffs, perform IDCT, and write as signed 16-bit values
//  Set output block to zero if nsym=0
extern void Idct2_s16( int intra, SYMBOL sym[], int nsym, S16 x[], int xdim, S16 recon[] )
{
    int i, clean = YES;
    union {
        S16   bshort[8][8];
        S32    blong[8][4];
    } block;    /* Output from IDCT */

    if (nsym == 0) {
        for (i = 0; i < 8; i++) {
            S32 * px = (S32 *)x;
            px[0] = px[1] = px[2] = px[3] = 0;
            x += xdim;
        }
        return;
    }

    idct2_goertzel( sym, nsym, block.blong, recon, intra, clean, GENERAL);

    // Shift out fractional bits
    for (i = 0; i < 8; i++) {
        x[i*xdim + 0] = block.bshort[i][0] >> FRACBITS;
        x[i*xdim + 1] = block.bshort[i][1] >> FRACBITS;
        x[i*xdim + 2] = block.bshort[i][2] >> FRACBITS;
        x[i*xdim + 3] = block.bshort[i][3] >> FRACBITS;
        x[i*xdim + 4] = block.bshort[i][7] >> FRACBITS;
        x[i*xdim + 5] = block.bshort[i][6] >> FRACBITS;
        x[i*xdim + 6] = block.bshort[i][5] >> FRACBITS;
        x[i*xdim + 7] = block.bshort[i][4] >> FRACBITS;
    }
}


//  Initialize tables for inverse DCT
//  Note: This routine has not been optimized for speed
static void init_inv_dct (void)
{
    int             i,j,m,n, index, zzpos;
    double          magn;               /* amplitude of DCT coefficient */
    static  double  bfunc[8][8][4][4];  /* DCT basis functions [vert freq][hor freq][][] */

    for (n=0; n < 8; n++) {     /* Construct 2-D basis functions */
      for (m=0; m < 8; m++) {
        for (j=0; j < 4; j++) {
          for (i=0; i < 4; i++) {
              bfunc[n][m][j][i] = SCALE_FACTOR * dctfunc(n,j) * dctfunc(m,i);
          }
        }
      }
    }
    //  Initialize table for INTRA DC coeff reconstruction */
    for (index = 0; index < N_SYM_INDICES; index++) {
        magn = 8 * index;
        //printf( "Init index = %d   magn = %f \n", index, magn);
        intra_dc_tab [index] = dct_tab_entry (magn * bfunc[0][0][0][0],
                                              magn * bfunc[0][0][0][1]);
    }
    //  128 is represented by index=255
    index = 255;
    magn = 8 * 128;
    //printf( "Init index = %d   magn = %f \n", index, magn);
    intra_dc_tab [index] = dct_tab_entry (magn * bfunc[0][0][0][0],
                                              magn * bfunc[0][0][0][1]);
    
    //  Initialize tables for DC and first two AC coeffs
    for (index = 0; index < MAX_DCT_INDEX; index++) {
        magn = 2*index + 1;
        //printf( "Init index = %d   magn = %f \n", index, magn);
        dc_tab [index] = dct_tab_entry (magn * bfunc[0][0][0][0],
                                        magn * bfunc[0][0][0][1]);
        for (i = 0; i < 2; i++) {
            hor_ac_tab [index][i]
                        = dct_tab_entry (magn * bfunc[0][1][0][2*i],
                                         magn * bfunc[0][1][0][2*i+1]);
        }
        for (j = 0; j < 4; j++) {
            vert_ac_tab [index][j]
                        = dct_tab_entry (magn * bfunc[1][0][j][0],
                                         magn * bfunc[1][0][j][1]);
        }
    }
    //  Initialize table for all coeffs
    for (index = 0; index < IDCT_NTAB1_SIZE; index++) {
        magn = 2*index + 1;
        //printf( "Init index = %d   magn = %f \n", index, magn);
        for (zzpos = 0; zzpos < 8*8; zzpos++) {
            n = InvZZ[zzpos] / 8;
            m = InvZZ[zzpos] % 8;
            for (j=0; j < 4; j++) {
              for (i=0; i < 2; i++) {
                  idct_tab [index][zzpos][2*j+i]
                    = dct_tab_entry (magn * bfunc[n][m][j][2*i],
                                    magn * bfunc[n][m][j][2*i+1]);
              }
            }
        }
    }
    for (index = 0; index < IDCT_NTAB2_SIZE; index++) {
        magn = 2 * IDCT_NTAB1_SIZE * (index + 1);
        //printf( "Init index = %d   magn = %f \n", index+IDCT_NTAB1_SIZE, magn);
        for (zzpos = 0; zzpos < 8*8; zzpos++) {
            n = InvZZ[zzpos] / 8;
            m = InvZZ[zzpos] % 8;
            for (j=0; j < 4; j++) {
              for (i=0; i < 2; i++) {
                  idct_tab [index + IDCT_NTAB1_SIZE][zzpos][2*j+i]
                    = dct_tab_entry (magn * bfunc[n][m][j][2*i],
                                    magn * bfunc[n][m][j][2*i+1]);
              }
            }
        }
    }
    // Modify tables for index=1 to truncate more values to zero
    Fix_PTel_S4000_mismatch();
    //  LUT to clip to allowed range for PIXEL
    for (i = CLIPMIN; i <= CLIPMAX; i++) {
        clip[i-CLIPMIN] = max( PIXEL_MIN, min( PIXEL_MAX, i));
    }
    //  Classify zigzag position as even or odd, horizontally and vertically
    for (zzpos = 0; zzpos < 8*8; zzpos++) {
        n = InvZZ[zzpos] / 8;
        m = InvZZ[zzpos] % 8;
        even_odd_index[zzpos] = 2 * (n % 2) + m % 2;
    }

#ifdef DO_H263_PLUS
    InitAdvancedIntraTables();
#endif

    return;
}


// Fix_PTel_S4000_mismatch - Modify tables for index=1 to truncate more 
// values to zero.  This improves the quality when communicating with a 
// system that has an IDCT mismatch problem (PTel S-4000, bug #250)
static void Fix_PTel_S4000_mismatch( void )
{
    int index, i, j, zzpos;
    
    index = 1;
    for (i = 0; i < 2; i++) {
        truncate_more( &hor_ac_tab [index][i] );
    }
    for (j = 0; j < 4; j++) {
        truncate_more( &vert_ac_tab [index][j] );
    }
    for (zzpos = 0; zzpos < 8*8; zzpos++) {
        for (j=0; j < 4; j++) {
            for (i=0; i < 2; i++) {
                truncate_more( &idct_tab [index][zzpos][2*j+i] );
            }
        }
    }
    return;
}

// truncate_more
static void truncate_more( S32 * idct_tab_entry )
{
#define ONE_HALF            (1 << (FRACBITS - 1))
#define PTEL_FIX_INTERVAL   (3)     // Set to 0 if no fix needed
    S32 halfword[2];
    int i;
    
    halfword[0] = *idct_tab_entry >> 16;            // Upper part of word
    halfword[1] = (*idct_tab_entry << 16) >> 16;    // Lower part of word
    for (i = 0; i < 2; ++i) {
        if (halfword[i] >= ONE_HALF  &&  halfword[i] < ONE_HALF + PTEL_FIX_INTERVAL) {
            halfword[i] = ONE_HALF - 1;
        } else if (halfword[i] <= -ONE_HALF  &&  halfword[i] > -(ONE_HALF + PTEL_FIX_INTERVAL)) {
            halfword[i] = -(ONE_HALF - 1);
        }
    }
    *idct_tab_entry = (halfword[0] << 16) | (halfword[1] & 0xffff);
    return;
}

 
#ifdef WHY_IS_THIS_HERE
static int idct2_energy_test( SYMBOL sym[], int nsym,
                            S16 recon[] )
{
    int isym, zzpos;
    int temp;

    isym = 0;
    temp = 0; 
    zzpos = sym[0].type;
    while (isym < nsym && zzpos < 10) {
        temp += abs((int) recon[(U8)sym[isym].value]);
        isym++;
        zzpos += 1 + sym[isym].type; 
    }
    return(temp);
}
#endif

static void idct2_goertzel( SYMBOL sym[], int nsym, S32 x[8][4],
                            S16 recon[], int intra, int clean, int idct_class)
{
    int     i, zzpos, isym;
    S32     even_odd[4][8]; /* Accumulate even/even, even/odd, odd/even, odd/odd */
    S32     temp[4];        /* Used in final butterfly computations */
#ifdef JM_LITTLE_ENDIAN                    
    char    msg[120];
    int		LUT_index;
#endif

    UNREFERENCED_PARAMETER(clean);

//  The four special class implementations are specific to LITTLE_ENDIAN.
#ifdef JM_LITTLE_ENDIAN                    

    switch (idct_class)
    {
        case GENERAL:
            {
                memset(even_odd, 0, 128);

                isym = 0;
                zzpos = sym[0].type;
                //  Reconstruct DC coeff
                if (intra == YES) {
                recon_intra_dc( (U8) sym[0].value, even_odd[0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                } 
                else if (zzpos == 0) {
                recon_dc( (S32) recon[(U8)sym[0].value], even_odd[0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Init even/odd: reconstruct first hor. AC coeff
                if (isym < nsym  &&  zzpos == 1) {
                recon_hor_ac( (S32) recon[(U8)sym[isym].value], even_odd[1]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Init odd/even: reconstruct first vert. AC coeff
                if (isym < nsym  &&  zzpos == 2) {
                recon_vert_ac( (S32) recon[(U8)sym[isym].value], even_odd[2]); 
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Reconstruct remaining coeffs
//  if (clean == YES) {      // activate this for sleazy IDCT decoder
                while (isym < nsym) {
                //printf(" Calling update with  zzpos = %d\n", zzpos);
                update( &even_odd[ even_odd_index[zzpos]] [0],
                    (S32) recon[(U8)sym[isym].value],
                    (long (*)[512]) &idct_tab[0][zzpos][0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
//    }     // activate this for sleazy IDCT decoder
 //   else {
 //
 // Sleazy reconstruction
 //
 //      while (isym < nsym && zzpos < 10) {
 //       //printf(" Calling update with  zzpos = %d\n", zzpos);
 //       update( &even_odd[ even_odd_index[zzpos]] [0],
 //               (S32) recon[(U8)sym[isym].value],  
 //               (U32 (*)[8*8*8])&idct_tab[0][zzpos][0]);
 //       isym++;
 //       zzpos += 1 + sym[isym].type;
 //         }
 //   }             
            }
        break;

        case DC_ONLY:
        case DC_AC_H:
        case DC_AC_V:
        case DC_3:
            {
                isym = 0;
                zzpos = sym[0].type;
                //  Reconstruct DC coeff
                if (intra == YES) 
                    {
                        LUT_index = (U8)sym[0].value;
                        even_odd[0][0] = intra_dc_tab[LUT_index];
                        isym++;
                        zzpos += 1 + sym[isym].type;
                    } 
                else if (zzpos == 0) 
                        {
                            LUT_index = (S32)recon[(U8)sym[0].value];
                            if (LUT_index>0)
                                even_odd[0][0] = dc_tab[LUT_index-1];
                            else if (LUT_index<0)
                                    even_odd[0][0] = -dc_tab[-LUT_index-1];
                            else
                            {
                                sprintf(msg, "ERROR:recon_dc called with arg=0");
                                H261ErrMsg(msg);
                            }
                            isym++;
                            zzpos += 1 + sym[isym].type;
                        }
                //  Init even/odd: reconstruct first hor. AC coeff
                if (isym < nsym  &&  zzpos == 1) {
                recon_hor_ac( (S32) recon[(U8)sym[isym].value], even_odd[1]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Init odd/even: reconstruct first vert. AC coeff
                if (isym < nsym  &&  zzpos == 2) {
                recon_vert_ac( (S32) recon[(U8)sym[isym].value], even_odd[2]); 
                isym++;
                zzpos += 1 + sym[isym].type;
                }   
            }
        break;
                    
        default:
            break;
    }       

#else
                memset(even_odd, 0, 128);

                isym = 0;
                zzpos = sym[0].type;
                //  Reconstruct DC coeff
                if (intra == YES) {
                recon_intra_dc( (U8) sym[0].value, even_odd[0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                } 
                else if (zzpos == 0) {
                recon_dc( (S32) recon[(U8)sym[0].value], even_odd[0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Init even/odd: reconstruct first hor. AC coeff
                if (isym < nsym  &&  zzpos == 1) {
                recon_hor_ac( (S32) recon[(U8)sym[isym].value], even_odd[1]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Init odd/even: reconstruct first vert. AC coeff
                if (isym < nsym  &&  zzpos == 2) {
                recon_vert_ac( (S32) recon[(U8)sym[isym].value], even_odd[2]); 
                isym++;
                zzpos += 1 + sym[isym].type;
                }
                //  Reconstruct remaining coeffs
//  if (clean == YES) {      // activate this for sleazy IDCT decoder
                while (isym < nsym) {
                //printf(" Calling update with  zzpos = %d\n", zzpos);
                update( &even_odd[ even_odd_index[zzpos]] [0],
                    (S32) recon[(U8)sym[isym].value],  
                    (S32 (*)[8*8*8]) &idct_tab[0][zzpos][0]);
                isym++;
                zzpos += 1 + sym[isym].type;
                }
//    }     // activate this for sleazy IDCT decoder
 //   else {
 //
 // Sleazy reconstruction
 //
 //      while (isym < nsym && zzpos < 10) {
 //       //printf(" Calling update with  zzpos = %d\n", zzpos);
 //       update( &even_odd[ even_odd_index[zzpos]] [0],
 //               (S32) recon[(U8)sym[isym].value],  
 //               (U32 (*)[8*8*8]) &idct_tab[0][zzpos][0]);
 //       isym++;
 //       zzpos += 1 + sym[isym].type;
 //         }
 //   }             
#endif

#ifdef JM_LITTLE_ENDIAN

    //  Final butterflies
    switch (idct_class)
    {
    //  DC only case
        case DC_ONLY:
            {
                temp[0] = even_odd[0][0];                   
                temp[0] += (0x10001L << (FRACBITS-1));      

                x[0][0] = temp[0];
            }
        break;
        
    //  DC + 1 horizontal AC case   
        case DC_AC_H:
             {
                temp[0] = even_odd[0][0];
                
                temp[0] += (0x10001L << (FRACBITS-1));
                
                x[0][0] = temp[0] + even_odd[1][0];
                x[0][2] = temp[0] - even_odd[1][0];
                x[0][1] = temp[0] + even_odd[1][1];
                x[0][3] = temp[0] - even_odd[1][1];
             }
        break;   
    // DC + 1 vertical AC case
        case DC_AC_V:
             {
                for (i = 0; i < 4; i++)
                    {
                        temp[0] = even_odd[0][0] + even_odd[2][2*i];
                        temp[1] = even_odd[0][0] - even_odd[2][2*i];

                        temp[0] += (0x10001L << (FRACBITS-1));    
                        temp[1] += (0x10001L << (FRACBITS-1));    

                        x[i][0]     = temp[0];
                        x[i][1]     = temp[0];
                        x[i][2]     = temp[0];
                        x[i][3]     = temp[0];                                              
                        x[7-i][0]   = temp[1];
                        x[7-i][1]   = temp[1];
                        x[7-i][2]   = temp[1];
                        x[7-i][3]   = temp[1];                                                                      
                    }
             }      
        break;                              
    // DC + 1 vertical AC + 1 horizontal AC case                                    
        case DC_3:
             {
                for (i = 0; i < 4; i++)
                    {
                        temp[0] = even_odd[0][0] + even_odd[2][2*i];
                        temp[2] = even_odd[0][0] - even_odd[2][2*i];

                        temp[0] += (0x10001L << (FRACBITS-1));    
                        temp[2] += (0x10001L << (FRACBITS-1));    

                        x[i][0]     = temp[0] + even_odd[1][2*i];
                        x[i][2]     = temp[0] - even_odd[1][2*i];                                               
                        x[7-i][0]   = temp[2] + even_odd[1][2*i];
                        x[7-i][2]   = temp[2] - even_odd[1][2*i];
                        x[i][1]     = temp[0] + even_odd[1][2*i+1];
                        x[i][3]     = temp[0] - even_odd[1][2*i+1];                                             
                        x[7-i][1]   = temp[2] + even_odd[1][2*i+1];
                        x[7-i][3]   = temp[2] - even_odd[1][2*i+1];
                    }
             }
        break;   
        case GENERAL:
            {
                for (i = 0; i < 4; i++) 
                    {
                    // j=0
                        temp[0] = even_odd[0][2*i] + even_odd[2][2*i];
                        temp[2] = even_odd[0][2*i] - even_odd[2][2*i];
                        temp[1] = even_odd[1][2*i] + even_odd[3][2*i];
                        temp[3] = even_odd[1][2*i] - even_odd[3][2*i];
                        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[i][0] = temp[0] + temp[1];
                        x[i][2] = temp[0] - temp[1];
                        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[7-i][0] = temp[2] + temp[3];
                        x[7-i][2] = temp[2] - temp[3];
                    // j=1
                        temp[0] = even_odd[0][2*i+1] + even_odd[2][2*i+1];
                        temp[2] = even_odd[0][2*i+1] - even_odd[2][2*i+1];
                        temp[1] = even_odd[1][2*i+1] + even_odd[3][2*i+1];
                        temp[3] = even_odd[1][2*i+1] - even_odd[3][2*i+1];
                        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[i][1] = temp[0] + temp[1];
                        x[i][3] = temp[0] - temp[1];
                        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[7-i][1] = temp[2] + temp[3];
                        x[7-i][3] = temp[2] - temp[3];
                }
            }
        break;
    
        default:
            break;
    }
#else
            {
                for (i = 0; i < 4; i++) 
                    {
                    // j=0
                        temp[0] = even_odd[0][2*i] + even_odd[2][2*i];
                        temp[2] = even_odd[0][2*i] - even_odd[2][2*i];
                        temp[1] = even_odd[1][2*i] + even_odd[3][2*i];
                        temp[3] = even_odd[1][2*i] - even_odd[3][2*i];
                        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[i][0] = temp[0] + temp[1];
                        x[i][2] = temp[0] - temp[1];
                        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[7-i][0] = temp[2] + temp[3];
                        x[7-i][2] = temp[2] - temp[3];
                    // j=1
                        temp[0] = even_odd[0][2*i+1] + even_odd[2][2*i+1];
                        temp[2] = even_odd[0][2*i+1] - even_odd[2][2*i+1];
                        temp[1] = even_odd[1][2*i+1] + even_odd[3][2*i+1];
                        temp[3] = even_odd[1][2*i+1] - even_odd[3][2*i+1];
                        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[i][1] = temp[0] + temp[1];
                        x[i][3] = temp[0] - temp[1];
                        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
                        x[7-i][1] = temp[2] + temp[3];
                        x[7-i][3] = temp[2] - temp[3];
                }
            }
#endif                          
    return;
}


#ifdef DO_H263_PLUS
//  recon_advanced_intra_dc - Reconstruct DC coeff in INTRA block
static void recon_advanced_intra_dc( U8 index, S32 vec[8])
{
    int j, temp;
    double bfunc;
    bfunc = SCALE_FACTOR * dctfunc(0,0) * dctfunc(0,0);
    temp = 8 * index;
    temp = dct_tab_entry( temp * bfunc, temp * bfunc);
    for (j = 0; j < 8; j++) {
        vec[j] = temp;
    }
    return;
}

//
// Quick implementation of intra prediction
//
extern void idct2_advanced_intra( SYMBOL sym[], int nsym, S32 x[8][4], S16 recon[],
                            U8 rDCpred, S8 rACpred[8], U8 rDCstore[1], S8 rACstore[8], 
                            U8 cDCpred, S8 cACpred[8], U8 cDCstore[1], S8 cACstore[8],
                            int predtype, int fixedDC, int leftBoundary, int upperBoundary)
{
    int     i, pos, isym;
    S32     even_odd[4][8]; /* Accumulate even/even, even/odd, odd/even, odd/odd */
    S32     temp[4];        /* Used in final butterfly computations */
    int     skip;
    S8      value;
    int    *scan_to_zz = NULL;
    int    *inv_scan_order = NULL;

    memset(even_odd, 0, 128);
    
    // in advanced intra mode, the AC and DC are coded in the same way, so it
    // is possible for the DC residual to be zero and the first symbol to 
    // have a non-trivial zero run
    if(nsym) {
        if(sym[0].type) {
            value = 0;
            isym = 0;
            skip = sym[0].type - 1; // we'll take care of DC outside the loop
        } else {
            value = sym[0].value;
            isym = 1;
            skip = sym[isym].type;
        }
    } else {
        value = 0;
        isym = 0;
        skip = 0;
    }

    //  Reconstruct DC coeff
    switch(predtype) {
    case ADV_INTRA_PRED_DC_ONLY:
        scan_to_zz = &zigzag_to_zigzag[0];
        inv_scan_order = &InvZZ[0];
        rDCstore[0] = cDCstore[0] = (U8)(value + (( 
            (upperBoundary ? cDCpred : rDCpred) + // use row predictor unless on upper boundary
            (leftBoundary ? rDCpred : cDCpred) + // use column predictor unless on left boundary
            1)>>1) );
        break;
    case ADV_INTRA_PRED_COLUMN:
        scan_to_zz = &alt_ver_to_zigzag[0];
        inv_scan_order = &inv_alt_ver_scan_no_reorder[0];
        rDCstore[0] = cDCstore[0] = (U8)(value + cDCpred);
        break;
    case ADV_INTRA_PRED_ROW:
        scan_to_zz = &alt_hor_to_zigzag[0];
        inv_scan_order = &inv_alt_hor_scan_no_reorder[0];
        rDCstore[0] = cDCstore[0] = (U8)(value + rDCpred);
        break;
    case ADV_INTRA_PRED_NONE:
        scan_to_zz = &zigzag_to_zigzag[0];
        inv_scan_order = &InvZZ[0];
        rDCstore[0] = cDCstore[0] = (U8)(value);
        break;
    }

    // We deviate from the H.263+ spec and the standard H.263 8-bit INTRA_DC quantizer
    // for small quantizer values so that we avoid problems with the dynamic range
    // getting too small
    if(fixedDC) {
        recon_advanced_intra_dc( rDCstore[0], even_odd[0]);
    } else {
        if(rDCstore[0]) update( &even_odd[even_odd_index[0]][0],
                        (S32) recon[rDCstore[0]],  
                        (S32 (*)[8*8*8]) &idct_tab[0][0][0]);
    }


    for(pos=1; pos<64; pos++)
    {
       // compute the quantized AC coefficient
        if(skip || isym>=nsym) {
            value = 0;
            skip--;
        } else {
            value = sym[isym++].value;
            skip = sym[isym].type;
        }
         // row predict and store reconstructed
        if(inv_scan_order[pos] < 8) {
            if(predtype == ADV_INTRA_PRED_ROW)
                value += rACpred[inv_scan_order[pos]];
            rACstore[inv_scan_order[pos]] = value;
        }
         // col predict and store reconstructed
        if((inv_scan_order[pos] & 7) == 0) {
            if(predtype == ADV_INTRA_PRED_COLUMN)
                value += cACpred[inv_scan_order[pos]>>3];
            cACstore[inv_scan_order[pos]>>3] = value;
        }
    
        // note that both even_odd_index and idct_tab assume that we are indexing
        // by zigzag order; hence the conversion of the variable pos
        if(value) update( &even_odd[ even_odd_index[scan_to_zz[pos]]] [0],
                    (S32) recon[ (U8)value],  
                    (S32 (*)[8*8*8]) &idct_tab[0][scan_to_zz[pos]][0]);
    }


    // Do the final butterfly
    for (i = 0; i < 4; i++) 
    {
        // j=0
        temp[0] = even_odd[0][2*i] + even_odd[2][2*i];
        temp[2] = even_odd[0][2*i] - even_odd[2][2*i];
        temp[1] = even_odd[1][2*i] + even_odd[3][2*i];
        temp[3] = even_odd[1][2*i] - even_odd[3][2*i];
        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
        x[i][0] = temp[0] + temp[1];
        x[i][2] = temp[0] - temp[1];
        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
        x[7-i][0] = temp[2] + temp[3];
        x[7-i][2] = temp[2] - temp[3];
        // j=1
        temp[0] = even_odd[0][2*i+1] + even_odd[2][2*i+1];
        temp[2] = even_odd[0][2*i+1] - even_odd[2][2*i+1];
        temp[1] = even_odd[1][2*i+1] + even_odd[3][2*i+1];
        temp[3] = even_odd[1][2*i+1] - even_odd[3][2*i+1];
        temp[0] += (0x10001L << (FRACBITS-1));    /* Round */
        x[i][1] = temp[0] + temp[1];
        x[i][3] = temp[0] - temp[1];
        temp[2] += (0x10001L << (FRACBITS-1));    /* Round */
        x[7-i][1] = temp[2] + temp[3];
        x[7-i][3] = temp[2] - temp[3];
    }

    return;
}


//  Idct2 - Reconstruct DCT coeffs, perform IDCT, and clip to allowed pixel range */
//  Requires nsym > 0
extern void Idct2AdvancedIntra( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[],
                               U8 rDCpred, S8 rACpred[8], U8 rDCstore[1], S8 rACstore[8], 
                               U8 cDCpred, S8 cACpred[8], U8 cDCstore[1], S8 cACstore[8],
                               int predtype, int fixedDC, int leftBoundary, int upperBoundary)
{
    union {
        S16   bshort[8][8];
        S32    blong[8][4];
    } block;    /* Output from IDCT */

    idct2_advanced_intra( sym, nsym, block.blong, recon, 
        rDCpred, rACpred, rDCstore, rACstore,
        cDCpred, cACpred, cDCstore, cACstore,
        predtype, fixedDC,leftBoundary,upperBoundary);
    idct2_clip(x, xdim, block.blong, GENERAL);

    return;
}


#endif


//  recon_intra_dc - Reconstruct DC coeff in INTRA block
static void recon_intra_dc( U8 index, S32 vec[8])
{
    int j, temp;
    
    temp = intra_dc_tab [index];
    for (j = 0; j < 8; j++) {
        vec[j] = temp;
    }
    return;
}


//  recon_dc - Reconstruct DC coeff
static void recon_dc( S32 y, S32 vec[8])
{
    int j, temp = 0;
    char msg[120];
    
    if (y > 0) {
        temp = dc_tab[ y - 1 ];
    } else if (y < 0) {
        temp = -dc_tab[ -y - 1 ];
    } else {
        sprintf( msg, "ERROR: recon_dc called with arg=0");
        H261ErrMsg( msg );
    }
    for (j = 0; j < 8; j++) {
        vec[j] = temp;
    }
    return;
}


//  recon_hor_ac - Reconstruct first hor. AC coeff
static void recon_hor_ac( S32 y, S32 vec[8])
{
    int j, temp0 = 0, temp1 = 0;
    char msg[120];
    
    if (y > 0) {
        temp0 = hor_ac_tab[ y - 1 ] [0];
        temp1 = hor_ac_tab[ y - 1 ] [1];
    } else if (y < 0) {
        temp0 = -hor_ac_tab[ -y - 1 ] [0];
        temp1 = -hor_ac_tab[ -y - 1 ] [1];
    } else {
        sprintf( msg, "ERROR: recon_hor_ac called with arg=0");
        H261ErrMsg( msg );
    }
    for (j = 0; j < 8; j += 2) {
        vec[j] = temp0;
        vec[j+1] = temp1;
    }
    return;
}


//  recon_vert_ac - Reconstruct first vert. AC coeff
static void recon_vert_ac( S32 y, S32 vec[8])
{
    int j, temp, index;
    char msg[120];
    
    if (y > 0) {
        for (j = 0; j < 4; j++) {
            vec[2*j] = vert_ac_tab[ y - 1 ] [j];
            vec[2*j+1] = vert_ac_tab[ y - 1 ] [j];
        }
    } else if (y < 0) {
        index = -y - 1;
        for (j = 0; j < 4; j++) {
            temp = -vert_ac_tab[ index ] [j];
            vec[2*j] = temp;
            vec[2*j+1] = temp;
        }
    } else {
        sprintf( msg, "ERROR: recon_vert_ac called with arg=0");
        H261ErrMsg( msg );
    }
    return;
}


static void update( S32 x[8], S32 index, S32 table[][8*8*8])
{
    int tab1_index, tab2_index;
    char msg[120];
    
    //printf( "Entered update\n" );
    if (index > 0) {
        index -= 1;
        tab1_index = index & ((0x1L << IDCT_NTAB1_BITS) - 1);  /* index % SIZE */
        tab2_index = index >> IDCT_NTAB1_BITS;
        //printf( "update plus: tab2 = %d    tab1 = %d\n", tab2_index, tab1_index);
        //printf( "x0 = %d  table = %d\n", x[0], table[tab1_index][0]);
        x[0] += table [tab1_index][0];
        x[1] += table [tab1_index][1];
        x[2] += table [tab1_index][2];
        x[3] += table [tab1_index][3];
        x[4] += table [tab1_index][4];
        x[5] += table [tab1_index][5];
        x[6] += table [tab1_index][6];
        x[7] += table [tab1_index][7];
        if (tab2_index != 0) {
            //printf( "x0 = %d  table = %d\n", x[0],
            //            table[tab2_index - 1 + IDCT_NTAB1_SIZE][0]);
            x[0] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][0];
            x[1] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][1];
            x[2] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][2];
            x[3] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][3];
            x[4] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][4];
            x[5] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][5];
            x[6] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][6];
            x[7] += table [tab2_index - 1 + IDCT_NTAB1_SIZE][7];
        //printf( "x0 = %d\n", x[0]);
        }
    } else if (index < 0) {
        index = -index - 1;
        tab1_index = index & ((0x1L << IDCT_NTAB1_BITS) - 1);  /* index % SIZE */
        tab2_index = index >> IDCT_NTAB1_BITS;
        //printf( "update minus: tab2 = %d    tab1 = %d\n", tab2_index, tab1_index);
        x[0] -= table [tab1_index][0];
        x[1] -= table [tab1_index][1];
        x[2] -= table [tab1_index][2];
        x[3] -= table [tab1_index][3];
        x[4] -= table [tab1_index][4];
        x[5] -= table [tab1_index][5];
        x[6] -= table [tab1_index][6];
        x[7] -= table [tab1_index][7];
        if (tab2_index != 0) {
            x[0] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][0];
            x[1] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][1];
            x[2] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][2];
            x[3] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][3];
            x[4] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][4];
            x[5] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][5];
            x[6] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][6];
            x[7] -= table [tab2_index - 1 + IDCT_NTAB1_SIZE][7];
        }
    } else if (index == 0) {
        sprintf( msg, "ERROR: update called with arg=0");
        H261ErrMsg( msg );
    }
    return;
}


static double dctfunc (int freq, int index)
{
    if (freq == 0) {
        return (1./sqrt(8.));
    }
    return ( cos(PI*freq*(index+0.5)/8.) / 2.);
}


//  Round x and y; put x in lower halfword, y in upper halfword (if little-endian)
static S32 dct_tab_entry (double x, double y)
{
    S32 ix, iy;

    ix = x * 65536.;    /* Mult by 2**16 */
    iy = y * 65536.;
#ifdef JM_LITTLE_ENDIAN    /* x in lower halfword, y in upper */
    return (combine (iy, ix));
#elif defined JM_BIG_ENDIAN    /* x in upper halfword, y in lower */
    return (combine (ix, iy));
#else
#   error
#endif
}

//  iy in upper halfword, ix>>16 in lower
static S32 combine (S32 iy, S32 ix)
{
    S32 low_tab, high_tab;

	// Note: this should not be a big-endian problem because calls to this function
	// exchange the order of ix and iy as needed. (Though wouldn't it have been
	// simpler to make the change here?) tkent
    low_tab = (ix + 0x8000L) >> 16  &  0x0000ffffL;
    high_tab = iy + 0x8000L - low_tab;
    high_tab &= 0xffff0000L;
    return (low_tab | high_tab);
}
