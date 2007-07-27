/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/*---------------------------------------------------------------------------*
   File:    gsmdec.c

   Classification:  IBM confidential
                    

   Copyright:       (C) Copyright IBM Corp. 
                    Haifa Research Laboratory
                    Audio/Video Group.

   Contact:         Gilad Cohen (giladc@haifa.vnet.ibm.com)
                    
   Date:            November 1998

   Version:         3.0

   Description:     Decoder functions


*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/*
#include <time.h>
*/
#include "gsm_api.h"
#include "gsm_dec.h"
#include "gsm_mac.h"

#include "mni.h"

/**************************************************************/

GSMReturnCode APICALL gsm_decoder_close( void *dec_state) {
    if( dec_state != NULL) {
        MNI_FREE( dec_state);
    }

    return GSM_OK;
}

/**************************************************************/

GSMReturnCode APICALL gsm_decoder_open(void **dec_state)
{
    int i;

    SAMPLE  QLB[]                = {0.10f, 0.35f, 0.65f, 1.00f};

    INT     k_start[INTERPLAR+1] = {0, 13, 27, 40, BLOCKSIZE};

	ULONG   init_sid[nPARAMETERS] = {
       2,28,18,12, 7, 5, 3, 2, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0,
	};

    sme_data *data;

    /*
     *  allocate new data structure
     */

    data = (sme_data *)MNI_MALLOC(sizeof(sme_data));
    *dec_state = data;

    if (data == NULL) {
          *dec_state = NULL;
          return GSM_MEMORY_ERROR;
    }


    /*
     *  initialize decoder tables
     */
	for ( i = 0 ; i < sizeof(data->lastSID)/sizeof(ULONG) ; i++ )
        data->lastSID[i] = init_sid[i];

    for ( i = 0 ; i < sizeof(QLB)/sizeof(SAMPLE) ; i++ )
        data->QLB[i] = QLB[i];

    for ( i = 0 ; i < sizeof(k_start)/sizeof(INT) ; i++ )
        data->k_start[i] = k_start[i];

    /*
     *  initialize decoder variables
     */
    data->prevNc             = 40;
    data->bitpos             = 0;
	data->prev_out           = 0.f;
    data->q_frstbase         = data->q_frst + BLOCKSIZE - SBLOCKSIZE;
    memset(data->q_frst,  0, sizeof(data->q_frst));

	memset(data->prevLARpp,  0, sizeof(data->prevLARpp));
	memset(data->u,  0, sizeof(data->u));

    return GSM_OK;
}



/**************************************************************/
GSMReturnCode APICALL gsm_decode_frame(void   *dec_state,
                                       UCHAR  *input_bits,
                                       short  *output_samples,
 				       GSMFrameType gsm_frame_type)

{
  ULONG  Parameters[nPARAMETERS];
  /* Unpack Bitsteam */
  UnpackBits(Parameters,input_bits);
  return gsm_decode_frame_parameters(dec_state,Parameters,output_samples,gsm_frame_type);

}

/**************************************************************/
GSMReturnCode APICALL gsm_decode_frame_ms(void   *dec_state,
                                       UCHAR  *input_bits,
                                       short  *output_samples,
				       GSMFrameType gsm_frame_type)

{
  ULONG  Parameters[nPARAMETERS];
  /* Unpack Bitsteam */
  UnpackBits_ms_odd(Parameters,input_bits);
  gsm_decode_frame_parameters(dec_state,Parameters,output_samples,gsm_frame_type);
  UnpackBits_ms_even(Parameters,input_bits);
  return gsm_decode_frame_parameters(dec_state,Parameters,output_samples+160,gsm_frame_type);

}


GSMReturnCode APICALL gsm_decode_frame_parameters(void   *dec_state,
                                       ULONG  *input_paramters,
                                       short  *output_samples,
				       GSMFrameType gsm_frame_type)

{
 sme_data *data =  (sme_data *) dec_state; 
 
 SAMPLE MIC[] = {0.0000f, -32.0f, -32.0f, -16.0f, -16.0f, -8.0f, -8.0f, -4.0f, -4.0f};
 SAMPLE B[] = {0.0f, 0.0f, 0.0f, 4.0f, -5.0f, 0.184f, -3.5f, -0.666f, -2.235f};
 SAMPLE INVA[] = {0.0f, 0.05f, 0.05f, 0.05f, 0.05f, (1.0f/13.637f), (1.0f/15.000f),
     (1.0f/8.334f), (1.0f/8.824f)};
 SAMPLE *outsig;        
 SAMPLE *prevLARpp;     
 SAMPLE prev_out;      
 LONG   prevNc;         
 SAMPLE *QLB;           
 SAMPLE *rp;           
 SAMPLE *u;             
 ULONG  *lastSID;      
 SAMPLE *q_frst;        
 INT    *k_start;       
 SAMPLE *q_frstbase;    

 ULONG  *Parameters=input_paramters;
 SAMPLE temp, gain, LARpp[ORDER+1], *q_ptr;
 ULONG  *pparm;
 INT    i, j, blocknumber, xmaxc, tempint, offset;
 SAMPLE xmaxp;

    outsig = data->outsig;
    prevLARpp = data->prevLARpp;
    prev_out = data->prev_out;
    prevNc = data->prevNc;
    QLB = data->QLB;
    rp = data->rp;
    u = data->u;
    lastSID = data->lastSID;
    q_frst= data->q_frst;
    k_start= data->k_start;
    q_frstbase = data->q_frstbase;

  /* move first residual to old first residual */
  for (i = 0; i < BLOCKSIZE-SBLOCKSIZE ; i++)  q_frst[i] = q_frst[i+BLOCKSIZE];

  CheckNullFrame(Parameters,lastSID,gsm_frame_type); /* if all zero frame reload SID */

   if (IsSIDframe(Parameters))         /* if SID frame update last SID */
     GenerateRandNoise(Parameters,lastSID);/* and generate random noise */


   for(blocknumber=0; blocknumber<SBLOCKS; blocknumber++)
    {

      q_ptr = q_frstbase + blocknumber*SBLOCKSIZE;
      pparm = Parameters + blocknumber*17+8;

      tempint = *pparm++;
      if ( (tempint>=40) && (tempint<=120) )
         prevNc = tempint;

      gain = QLB[*pparm++];
      offset =  *pparm++;

      xmaxc = *pparm++;

      if ( xmaxc < 16)
        {
          xmaxp = (SAMPLE)(31 + xmaxc*32);
        }
        else
        {
          i = (xmaxc-16) >> 3;
          xmaxp = (SAMPLE)((512<<i) + (64<<i) - 1 + (xmaxc-16-8*i)*(64<<i));
        }

      for (i = 0; i < SBLOCKSIZE; i++)
         q_ptr[i] =  gain * q_ptr[i-prevNc];

      for (i = 0; i < SUBSAMPSIZE ; i++)  q_ptr[offset+3*i] += 
           (  ((0.25f*(float)pparm[i]-0.875f) / 32768.0f)  * xmaxp);

    }

    // These tables implement the decoding function (index to LAR):
    for(i=1;i<9;i++) 
     LARpp[i]=(float)(Parameters[i-1]+MIC[i]-B[i])*INVA[i];

    // 
    // LAR interpolation
    //

	for(j=0; j< INTERPLAR; j++)
      {
    
#ifdef LINEAR_FUN
        if(j==0)        
           for(i=1; i<=ORDER; i++)
            {
              rp[i]=dec_lin_ltor_func(prevLARpp[i]*0.75f + LARpp[i]*0.25f);
            }
        else if (j==1)  
          for(i=1; i<=ORDER; i++)
            {
                rp[i]=dec_lin_ltor_func(prevLARpp[i]*0.5f + LARpp[i]*0.5f);
            }
        else if (j==2)  
          for(i=1; i<=ORDER; i++)
            {
                rp[i]=dec_lin_ltor_func(prevLARpp[i]*0.25f + LARpp[i]*0.75f);
            }
        else if (j==3) 
           for(i=1; i<=ORDER; i++)
              {
                rp[i]=dec_lin_ltor_func(prevLARpp[i] = LARpp[i]);
              }
#else
       if(j==0)        
           for(i=1; i<=ORDER; i++)
            {
              temp  = exp( ln10 * (prevLARpp[i]*0.75f + LARpp[i]*0.25f));
              rp[i] = (temp-1.0) / (temp+1.0);
            }
        else if (j==1)  
          for(i=1; i<=ORDER; i++)
            {
                temp  = exp( ln10 * (prevLARpp[i]*0.50 + LARpp[i]*0.50));
                rp[i] = (temp-1.0) / (temp+1.0);
            }
        else if (j==2)  
          for(i=1; i<=ORDER; i++)
            {
                temp  = exp( ln10 * (prevLARpp[i]*0.25 + LARpp[i]*0.75));
                rp[i] = (temp-1.0) / (temp+1.0);
            }
        else if (j==3) 
           for(i=1; i<=ORDER; i++)
              {
                temp  = exp( ln10 * (prevLARpp[i] = LARpp[i]));
                rp[i] = (temp-1.0) / (temp+1.0);
              }
#endif

        /* 
         * Lattice synthesis filtering
         */

        for(tempint=k_start[j]; tempint<k_start[j+1]; tempint++)
         {
          temp = q_frstbase[tempint];

          STEPD(0);
          STEPD(1);
          STEPD(2);
          STEPD(3);
          STEPD(4);
          STEPD(5);
          STEPD(6);
          STEPD(7);

          prev_out = temp + prev_out * (0x6E14/32768.0f);
          u[0] = temp;

          outsig[tempint] = prev_out + prev_out;
         } /* for tempint... */
       } /* for j... */

  data->prev_out     = prev_out;
  data->prevNc       = prevNc;
  data->q_frstbase   = q_frstbase;


  for (i=0;i<BLOCKSIZE ;i++)
     output_samples[i] = (short)(ClipTo1(outsig[i])*32766.0);

  return GSM_OK;
}



/**************************************************************/

SAMPLE dec_lin_ltor_func(SAMPLE lar)
{
 SAMPLE r;

 if (fabs(lar)<0.675)
  r = lar;
 else if (fabs(lar)>=0.675 && fabs(lar)<1.225)
  r = (SAMPLE)( SIGN(lar) * (0.500*fabs(lar)+0.3375) );
 else
  r = (SAMPLE)( SIGN(lar) * (0.125*(float)fabs(lar)+0.796875) );

 return(r);
}

/***************************************************************************/
/* function IsSIDframe checks if a frame is a SID frame or a speeech frame */
/***************************************************************************/

BOOLEAN IsSIDframe( ULONG Parameters[nPARAMETERS])
{

 /*
 * the true test of a SID frame is a bit more delicate, but
 * the following works for error-free transmission
 *
 * by GSM, a frame is an SID frame if the bits of error class I
 * that correspond to the pulses are 0. I check all pulses...
 */
 INT i, j;

 for(i=0; i<SBLOCKS; i++)
   for(j=12; j<25; j++)
       if (Parameters[i*17 + j] !=0)    
          return FALSE;
       
 return TRUE;
}

/***************************************************************************/
/* function GenerateRandNoise keeps SID parameters & fill the noise param  */
/***************************************************************************/

void  GenerateRandNoise(ULONG Parameters[nPARAMETERS],
                        ULONG lastSID[nPARAMETERS])
{
  INT i, j;

 for (i=0;i<nPARAMETERS ;i++ ) /* keep last SID parameters */
    lastSID[i]=Parameters[i];

 /*
  * fill up pulses with a random number between 1 and 6
  * fill up grid positions with a random number between 0 and 3
  */

 for(i=0; i<SBLOCKS; i++)
   {
    for(j=12; j<25; j++)
       Parameters[i*17 + j] = rand()/5461 + 1;
    Parameters[i*17 + 10] = rand()/10922;
    Parameters[i*17 + 9] = 0;
   }

 /*
  *  pitch = 40,120,40,120
  */
 Parameters[8]  = Parameters[42] =  40;
 Parameters[25] = Parameters[59] = 120;
}

/***************************************************************************/
/* function CheckNullFrame return if not all the parameters are zero       */
/* else fills parameters with last received SID                            */
/***************************************************************************/

void CheckNullFrame(ULONG Parameters[nPARAMETERS],
                    ULONG lastSID[nPARAMETERS],
					GSMFrameType gsm_frame_type)
{
 INT i;

 if (GSM_FORCE_SILENCE_FRAME != gsm_frame_type)
  for (i=0;i<nPARAMETERS ;i++ )
     if (Parameters[i]) return;         /* check if all parametrs are zero */

 for (i=0;i<nPARAMETERS ;i++ )          /* if all zero reload last SID */
     Parameters[i]=lastSID[i];

}


/**************************************************************/

 void UnpackBits_ms_odd(ULONG  *Parameters, UCHAR *Bitstream)
 {
     int paramIndex=0;
     int inputIndex=0;
     int n;
          //
          // Log Area Ratios
          //
          // input bytes 0-4
          Parameters[paramIndex++]=   Bitstream[ 0] & 0x0000003F ;

          Parameters[paramIndex++]= ((Bitstream[ 0] >> 6) & 0x00000003) |
                                    ((Bitstream[ 1] & 0x0000000F) << 2) ;
          Parameters[paramIndex++]= ((Bitstream[ 1] >> 4) & 0x0000000F) |
                                    ((Bitstream[ 2] & 0x00000001) << 4) ;
          Parameters[paramIndex++]=  (Bitstream[ 2] >> 1) & 0x0000001F;
          Parameters[paramIndex++]= ((Bitstream[ 2] >> 6) & 0x00000003) |
                                    ((Bitstream[ 3] & 0x00000003) << 2) ;
          Parameters[paramIndex++]=  (Bitstream[ 3] >> 2) & 0x0000000F;
          Parameters[paramIndex++]= ((Bitstream[ 3] >> 6) & 0x00000003) |
                                    ((Bitstream[ 4] & 0x00000001) << 2) ;
          Parameters[paramIndex++]=  (Bitstream[ 4] >> 1) & 0x00000007;

          //
          // Sub Frame No. 1-4
          //
          // input bytes 5-11,12-18,19-25,26-32
          for(n=0; n<4; n++) {
           Parameters[paramIndex++]= ((Bitstream[inputIndex+4] >> 4) & 0x0000000F) |
                                     ((Bitstream[inputIndex+5] & 0x00000007) << 4) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+5] >> 3) & 0x00000003;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+5] >> 5) & 0x00000003;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+5] >> 7) & 0x00000001) |
                                     ((Bitstream[inputIndex+6] & 0x0000001F) << 1) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+6] >> 5) & 0x00000007;
           Parameters[paramIndex++]=   Bitstream[inputIndex+7] & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+7] >> 3) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+7] >> 6) & 0x00000003) |
                                     ((Bitstream[inputIndex+8] & 0x00000001) << 2) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+8] >> 1) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+8] >> 4) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+8] >> 7) & 0x00000001) |
                                     ((Bitstream[inputIndex+9] & 0x00000003) << 1) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+9]>> 2) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+9] >> 5) & 0x00000007;
           Parameters[paramIndex++]=   Bitstream[inputIndex+10] & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+10] >> 3) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+10] >> 6) & 0x00000003) |
                                     ((Bitstream[inputIndex+11] & 0x00000001) << 2) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+11] >> 1) & 0x00000007;
           inputIndex += 7;
          }
}


 void UnpackBits_ms_even(ULONG  *Parameters, UCHAR *Bitstream)
 {
     int paramIndex=0;
     int inputIndex=32;
     int n;

          //
          // Log Area Ratios
          //
          // input bytes 0-4

          Parameters[paramIndex++]= ((Bitstream[inputIndex  ] >> 4) & 0x0000000F) |
                                    ((Bitstream[inputIndex+1] & 0x00000003) << 4) ;
          Parameters[paramIndex++]=  (Bitstream[inputIndex+1] >> 2) & 0x0000003F;
          Parameters[paramIndex++]=   Bitstream[inputIndex+2] & 0x0000001F;
          Parameters[paramIndex++]= ((Bitstream[inputIndex+2] >> 5) & 0x00000007) | 
                                    ((Bitstream[inputIndex+3] & 0x00000003) << 3) ;
          Parameters[paramIndex++]=  (Bitstream[inputIndex+3] >> 2) & 0x0000000F;
          Parameters[paramIndex++]= ((Bitstream[inputIndex+3] >> 6) & 0x00000003) | 
                                    ((Bitstream[inputIndex+4] & 0x00000003) << 2) ;
          Parameters[paramIndex++]=  (Bitstream[inputIndex+4] >> 2) & 0x00000007;
          Parameters[paramIndex++]=  (Bitstream[inputIndex+4] >> 5) & 0x00000007;

           //
          // Sub Frame No. 1-4
          //
          // input bytes 5-11,12-18,19-25,26-32

          for(n=0; n<4; n++) {
           Parameters[paramIndex++]=   Bitstream[inputIndex+5] & 0x0000007F;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+5] >> 7) & 0x00000001) |
                                     ((Bitstream[inputIndex+6] & 0x00000001) << 1) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+6] >> 1) & 0x00000003;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+6] >> 3) & 0x0000001F) |
                                     ((Bitstream[inputIndex+7] & 0x00000001) << 5) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+7] >> 1) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+7] >> 4) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+7] >> 7) & 0x00000001) |
                                     ((Bitstream[inputIndex+8] & 0x00000003) << 1) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+8] >> 2) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+8] >> 5) & 0x00000007;
           Parameters[paramIndex++]=   Bitstream[inputIndex+9] & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+9] >> 3) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+9] >> 6) & 0x00000003) |
                                     ((Bitstream[inputIndex+10] & 0x00000001) << 2) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+10] >> 1) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+10] >> 4) & 0x00000007;
           Parameters[paramIndex++]= ((Bitstream[inputIndex+10] >> 7) & 0x00000001) |
                                     ((Bitstream[inputIndex+11] & 0x00000003) << 1) ;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+11] >> 2) & 0x00000007;
           Parameters[paramIndex++]=  (Bitstream[inputIndex+11] >> 5) & 0x00000007;
           inputIndex+=7;
          }


 }

void   UnpackBits(ULONG  *Params, UCHAR *Bitstream)
{
 LONG BitstreamIndex = 5;
 LONG paramsIndex    = 8;
 LONG subFrame;

 //
 // Log Area Ratios (first 5 bytes)
 //
#ifdef GSM_HOTMEDIA_BIT_PACKING
  Params[0]= ((Bitstream[0] & 0xF0) >> 2) |
	         ((Bitstream[1] >> 6) & 0x3);
#else // RTP packing:
  Params[0]= ((Bitstream[0] & 0xF) << 2) |
	         ((Bitstream[1] >> 6) & 0x3) ;
#endif
  Params[1]=   Bitstream[1] & 0x3F ;
  Params[2]=  (Bitstream[2]>>3) & 0x1F;
  Params[3]= ((Bitstream[2] & 0x7) << 2) |
             ((Bitstream[3] >> 6) & 0x3) ;
  Params[4]=  (Bitstream[3]>>2) & 0xF;
  Params[5]= ((Bitstream[3] & 0x3) << 2) |
             ((Bitstream[4] >> 6) & 0x3) ;
  Params[6]=  (Bitstream[4]>>3) & 0x7;
  Params[7]=   Bitstream[4] & 0x7 ;

 //
 // Sub Frame No. 1-4
 //
 // input bytes 5-11,12-18,19-25,26-32
 for(subFrame=0; subFrame<4; subFrame++) {

   Params[paramsIndex   ]=  (Bitstream[BitstreamIndex  ]>>1) & 0x7F;
   Params[paramsIndex+ 1]= ((Bitstream[BitstreamIndex  ] & 0x1) << 1) |
	                        ((Bitstream[BitstreamIndex+1] >> 7) & 0x1) ;
   Params[paramsIndex+ 2]=  (Bitstream[BitstreamIndex+1]>>5) & 0x3;
   Params[paramsIndex+ 3]= ((Bitstream[BitstreamIndex+1] & 0x1f) << 1) |
	                        ((Bitstream[BitstreamIndex+2] >> 7) & 0x1) ;
   Params[paramsIndex+ 4]=  (Bitstream[BitstreamIndex+2]>>4) & 0x7;
   Params[paramsIndex+ 5]=  (Bitstream[BitstreamIndex+2]>>1) & 0x7;
   Params[paramsIndex+ 6]= ((Bitstream[BitstreamIndex+2] & 0x1) << 2) |
	                        ((Bitstream[BitstreamIndex+3] >> 6) & 0x3) ;
   Params[paramsIndex+ 7]=  (Bitstream[BitstreamIndex+3]>>3) & 0x7;
   Params[paramsIndex+ 8]=   Bitstream[BitstreamIndex+3] & 0x7 ;
   Params[paramsIndex+ 9]=  (Bitstream[BitstreamIndex+4]>>5) & 0x7;
   Params[paramsIndex+10]=  (Bitstream[BitstreamIndex+4]>>2) & 0x7;
   Params[paramsIndex+11]= ((Bitstream[BitstreamIndex+4] & 0x3) << 1) |
	                        ((Bitstream[BitstreamIndex+5] >> 7) & 0x1) ;
   Params[paramsIndex+12]=  (Bitstream[BitstreamIndex+5]>>4) & 0x7;
   Params[paramsIndex+13]=  (Bitstream[BitstreamIndex+5]>>1) & 0x7;
   Params[paramsIndex+14]= ((Bitstream[BitstreamIndex+5] & 0x1) << 2) |
	                        ((Bitstream[BitstreamIndex+6] >> 6) & 0x3) ;
   Params[paramsIndex+15]=  (Bitstream[BitstreamIndex+6]>>3) & 0x7;
   Params[paramsIndex+16]=  (Bitstream[BitstreamIndex+6]) & 0x7;

   BitstreamIndex += 7;
   paramsIndex += 17;
  }

 // For debugging only
 /*
 {
  int i;
  printf("bits **********\n");
  for(i=0;i<33;i++)
   printf("[%d] %d\n",i,Bitstream[i]);
  printf("params **********\n");
  for(i=0;i<76;i++)
   printf("[%d] %d\n",i,Params[i]);
 }
 */
}
