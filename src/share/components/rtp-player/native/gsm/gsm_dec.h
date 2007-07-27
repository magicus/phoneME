/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*---------------------------------------------------------------------------*
   File:    gsmdecd.h

   Classification:  IBM confidential


   Copyright:       (C) Copyright IBM Corp.
                    Haifa Research Laboratory
                    Audio/Video Group.

   Contact:         Gilad Cohen (giladc@haifa.vnet.ibm.com)

   Date:            November 1998

   Version:         3.0

   Description:     Decoder header file.


*---------------------------------------------------------------------------*/
#ifndef __GSMDECOD_H
#define __GSMDECOD_H


#define SAMPLE    float
#define UCHAR     unsigned char
#define LONG      long int
#define ULONG     unsigned long int
#define BOOLEAN   long int
#define SHORT     short int
#define INT       long int
#define OUT_TYPE  SHORT


/*
 * GSM parameters and constants
 */

#define NUMBLOCKS     8
#define ORDER         8
#define BLOCKSIZE     160
#define SBLOCKSIZE    40
#define SBLOCKS       4
#define FILTORDER     10
#define SUBSAMPBLOCKS 4
#define SUBSAMPSIZE   13
#define nPARAMETERS   76
#define UPDATE_RATE   8
#define BITS_PER_FRAME 264
#define INTERPLAR     4


#ifndef TRUE
  #define TRUE          1
#endif
#ifndef FALSE
  #define FALSE         0
#endif



/*
 * GSM decoder data structure
 */
typedef struct {
//    SAMPLE LARpp1[64];
//    SAMPLE LARpp5[16];
//    SAMPLE LARpp6[16];
//    SAMPLE LARpp7[8];
//    SAMPLE LARpp8[8];
    SAMPLE outsig[BLOCKSIZE];
    SAMPLE prevLARpp[ORDER+1];
    SAMPLE prev_out;
    SAMPLE rp[ORDER+1];
    SAMPLE u[ORDER+1];
    ULONG  lastSID[nPARAMETERS];
    SAMPLE q_frst[2*BLOCKSIZE-SBLOCKSIZE];
    SAMPLE *q_frstbase;
    LONG   prevNc;
    LONG   bitpos;
    SAMPLE QLB[4];
    INT    k_start[INTERPLAR+1];
} sme_data;


/*
 * function prototypes
 */
void   UnpackBits(ULONG  *Params, UCHAR *Bitstream);
void   UnpackBits_ms_odd(ULONG  *Params, UCHAR *Bitstream);
void   UnpackBits_ms_even(ULONG  *Params, UCHAR *Bitstream);

void   CheckParityBits(UCHAR *bts_byte);
BOOLEAN IsSIDframe( ULONG Parameters[nPARAMETERS]);
void    GenerateRandNoise(ULONG Parameters[nPARAMETERS],
                          ULONG lastSID[nPARAMETERS]);
void    CheckNullFrame(ULONG Parameters[nPARAMETERS],
                       ULONG lastSID[nPARAMETERS],
					   GSMFrameType gsm_frame_type);
SAMPLE dec_lin_ltor_func(SAMPLE lar);


#endif

