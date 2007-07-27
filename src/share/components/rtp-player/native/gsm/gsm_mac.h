/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*---------------------------------------------------------------------------*
   File:    gsm_marcos.h

   Classification:  IBM confidential


   Copyright:       (C) Copyright IBM Corp.
                    Haifa Research Laboratory
                    Audio/Video Group.

   Contact:         Gilad Cohen (giladc@haifa.vnet.ibm.com)

   Date:            November 1998

   Version:         3.0

   Description:     definition of the API

*---------------------------------------------------------------------------*/
#ifndef __GSM_MAC_H
#define __GSM_MAC_H


#define LINEAR_FUN   // define this for piece wise linear conversion funcion

#define SIGN(X)     ( (X) >0.0f ? +1.0f : ( (X)==0.0f ? 0.0f : -1.0f ) )
#define ABS(s)      ( (s)>0 ? (s) : -(s) )
#define ClipTo1(s)  ( (s)>1.0 ? 1.0 : ( (s)<-1.0 ? -1.0 : (s) )  )

#define INVln10  0.43429448190325183f
#define ln10     2.30258509299404568f


/* macro definitions for GSM encoder tasks */

#define STEP(i)                                                           \
        temp  = u[i-1] + rp[i] * di;                                      \
        di    = di +  rp[i] * u[i-1];                                     \
        u[i-1]= temp2;                                                    \
        temp2 = temp;

// Added for decimated pitch search
#define DSTEP2(s)                                                         \
        result1 += decimated_subsegment[s]*decimated_q_frstbase[s-ii] ;   \
        result2 += decimated_subsegment[s]*decimated_q_frstbase[s-ii-1]

// Added for decimated pitch search
#define STEP3(s)                                                          \
        result1 += subsegment[s]*q_frstbase[s-ii  ] ;                     \
        result2 += subsegment[s]*q_frstbase[s-ii-1] ;                     \
        result3 += subsegment[s]*q_frstbase[s-ii-2]

#define STEP2(s)                                                          \
        result1 += subsegment[s]*q_frstbase[s-ii] ;                       \
        result2 += subsegment[s]*q_frstbase[s-ii-1]

#define STEPD(s)                                                          \
        temp -= rp[ORDER-s] * u[ORDER-s-1];                               \
        u[ORDER-s] = u[ORDER-s-1] + rp[ORDER-s] * temp

#define ZIP(a, b, delaystart, delaystop, out, conv_start, conv_stop)      \
        for (ii = delaystart; ii < delaystop; ii += 2)                    \
        {                                                                 \
            result1 = a[conv_start]*b[conv_start-ii]; result2 = 0.0f;     \
            for (jj = conv_start+1; jj < conv_stop; jj++)                 \
            {                                                             \
                result1 += a[jj] * b[jj-ii];                              \
                result2 += a[jj] * b[jj-ii-1];                            \
            }                                                             \
            out[ii]   = result1;                                          \
            out[ii+1] = result2;                                          \
        }

#endif

