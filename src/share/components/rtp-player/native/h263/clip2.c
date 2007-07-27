/*-----------------------------------------------------------------------------
 *	CLIP2.C
 *
 *	DESCRIPTION
 *		idct2sum_clip - This function performs the final clipping of the IDCT 
 *						outputs for Idct2Sum.							    
 *
 *      Author:     Gene Su    		02/02/95
 *      Inspector:  
 *      Revised:
 *      02/06/95	G. Su	Define the types of 8x8 blocks outside of the
 *							clipping function.
 *	(c) 1995, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "clip.h"

#define FRACBITS        6   /* Fractional bits in IDCT computation */
#define PIXEL_MIN       0
#define PIXEL_MAX       255
#define CLIPMARGIN      300
#define CLIPMIN         (PIXEL_MIN - CLIPMARGIN)
#define CLIPMAX         (PIXEL_MAX + CLIPMARGIN)
extern PIXEL   clip[(CLIPMAX-CLIPMIN+1)];

extern void idct2sum_clip(PIXEL x[], int xdim, S32 idct_out[8][4], int idct_class)
{
	int		i, temp;

#ifdef JM_LITTLE_ENDIAN
	int		hi_clip0, hi_clip1, hi_clip2, hi_clip3;
	int		lo_clip0, lo_clip1, lo_clip2, lo_clip3;

	switch (idct_class)
	// DC only case
	{
		case DC_ONLY:
		{
			temp = idct_out[0][0];
			lo_clip0 = -CLIPMIN + ((temp << 16) >> (FRACBITS + 16));
			hi_clip0 = -CLIPMIN + (temp >> (FRACBITS + 16));

    		for (i = 0; i < 8; i++) 
    				{
				        x[i*xdim] = clip[x[i*xdim]+lo_clip0];
        				x[i*xdim+1] = clip[x[i*xdim+1]+hi_clip0];
        				x[i*xdim+2] = clip[x[i*xdim+2]+lo_clip0];
        				x[i*xdim+3] = clip[x[i*xdim+3]+hi_clip0];
        				x[i*xdim+7] = clip[x[i*xdim+7]+lo_clip0];
        				x[i*xdim+6] = clip[x[i*xdim+6]+hi_clip0];
        				x[i*xdim+5] = clip[x[i*xdim+5]+lo_clip0];
        				x[i*xdim+4] = clip[x[i*xdim+4]+hi_clip0];
					}
		}
		break;	 
	// DC + 1 horizontal AC case 
		case DC_AC_H:
		{
			temp = idct_out[0][0];
			lo_clip0 = -CLIPMIN + ((temp << 16) >> (FRACBITS + 16));
			hi_clip0 = -CLIPMIN + (temp >> (FRACBITS + 16));

			temp = idct_out[0][1];
			lo_clip1 = -CLIPMIN + ((temp << 16) >> (FRACBITS + 16));
			hi_clip1 = -CLIPMIN + (temp >> (FRACBITS + 16));

			temp = idct_out[0][2];
			lo_clip2 = -CLIPMIN + ((temp << 16) >> (FRACBITS + 16));
			hi_clip2 = -CLIPMIN + (temp >> (FRACBITS + 16));

			temp = idct_out[0][3];
			lo_clip3 = -CLIPMIN + ((temp << 16) >> (FRACBITS + 16));
			hi_clip3 = -CLIPMIN + (temp >> (FRACBITS + 16));

    		for (i = 0; i < 8; i++) 
    				{
				        x[i*xdim] = clip[x[i*xdim]+lo_clip0];
        				x[i*xdim+1] = clip[x[i*xdim+1]+hi_clip0];
        				x[i*xdim+2] = clip[x[i*xdim+2]+lo_clip1];
        				x[i*xdim+3] = clip[x[i*xdim+3]+hi_clip1];
        				x[i*xdim+7] = clip[x[i*xdim+7]+lo_clip2];
        				x[i*xdim+6] = clip[x[i*xdim+6]+hi_clip2];
        				x[i*xdim+5] = clip[x[i*xdim+5]+lo_clip3];
        				x[i*xdim+4] = clip[x[i*xdim+4]+hi_clip3];
					}
		}
		break;	     
		
		case GENERAL:
		{
    		for (i = 0; i < 8; i++) 
    			{
        			temp = idct_out[i][0];
        			x[i*xdim] = clip[x[i*xdim] -CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        			x[i*xdim+1] = clip[x[i*xdim+1] -CLIPMIN + (temp >> (FRACBITS + 16))];
        			temp = idct_out[i][1];
        			x[i*xdim+2] = clip[x[i*xdim+2] -CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        			x[i*xdim+3] = clip[x[i*xdim+3] -CLIPMIN + (temp >> (FRACBITS + 16))];
        			temp = idct_out[i][2];
        			x[i*xdim+7] = clip[x[i*xdim+7] -CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        			x[i*xdim+6] = clip[x[i*xdim+6] -CLIPMIN + (temp >> (FRACBITS + 16))];
        			temp = idct_out[i][3];
       	 			x[i*xdim+5] = clip[x[i*xdim+5] -CLIPMIN + ((temp << 16) >> (FRACBITS + 16))];
        			x[i*xdim+4] = clip[x[i*xdim+4] -CLIPMIN + (temp >> (FRACBITS + 16))];
				}
		}
		break;
		
		default:
			break;
	}						
#else
		for (i = 0; i < 8; i++)		
        {
            int j;
            // Notice the funky casts of idct_out, which needs to be treated
            // as an S16 [8][8].
            for (j = 0; j < 4; j++) {
                temp = ((S16 (*)[8]) idct_out)[i][j] >> FRACBITS;
                x[i*xdim+j] = clip[x[i*xdim+j] -CLIPMIN + temp];
                temp = ((S16 (*)[8]) idct_out)[i][j+4] >> FRACBITS;
                x[i*xdim+7-j] = clip[x[i*xdim+7-j] -CLIPMIN + temp];
            }
        }
#endif
}
