/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/**
 * @file
 *
 * IMPL_NOTE:file definition
 */

/*
 * WARNING - THIS IS AN EXPERIMENTAL FEATURE OF KVM THAT MAY, OR MAY NOT
 * EXIST IN A FUTURE VERSION OF THIS PRODUCT. IT IS NOT A PART OF THE
 * CLDC SPECIFICATION AND IS PROVIDED FOR ILLUSTRATIVE PURPOSES ONLY
 */

#include <kni.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bn.h>

#include "jsrop_exceptions.h"
#include "jsrop_memory.h"

#define MAX(x,y) (((x)>(y))?(x):(y))

/*=========================================================================
 * FUNCTION:      modExp([B[B[B[B[B)V (STATIC)
 * CLASS:         com/sun/midp/crypto/RSA
 * TYPE:          static native function
 * OVERVIEW:      Perform modular exponentiation.
 * INTERFACE (operand stack manipulation):
 *   parameters:  data      contains the data on which exponentiation is to
 *                           be performed
 *                exponent  contains the exponent, e.g. 65537 (decimal) is 
 *                           written as a three-byte array containing 
 *                           0x01, 0x00, 0x01
 *                modulus   contains the modulus
 *                result    the result of the modular exponentiation is 
 *                           returned in this array
 *   returns: the length of the result
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_j2me_crypto_RSA_modExp) {
    jint dataLen, expLen, modLen, resLen;
    jint maxLen;
    INTEGER numbytes = 0;
    unsigned char *buf;
    BIGNUM *a, *b, *c, *d;
    BN_CTX *ctx;

    KNI_StartHandles(4);

    KNI_DeclareHandle(ires);
    KNI_DeclareHandle(imod);
    KNI_DeclareHandle(iexp);
    KNI_DeclareHandle(idata);

    KNI_GetParameterAsObject(4, ires);
    KNI_GetParameterAsObject(3, imod);
    KNI_GetParameterAsObject(2, iexp);
    KNI_GetParameterAsObject(1, idata);

    resLen    = KNI_GetArrayLength(ires);
    modLen    = KNI_GetArrayLength(imod);
    expLen    = KNI_GetArrayLength(iexp);
    dataLen   = KNI_GetArrayLength(idata);

    /* Find which parameter is largest and allocate that much space */
    maxLen = MAX(MAX(MAX(resLen, modLen), expLen), dataLen);
    if (maxLen > (BN_MAX_INTEGER / 8)) {
        /* The number of BITS must fit in a BN integer. */
        KNI_ThrowNew(jsropIllegalArgumentException, "arg too long");
    } else {
        buf = (unsigned char *) JAVAME_MALLOC(maxLen * sizeof(unsigned char));
        if (buf == NULL) {
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        } else {

            KNI_GetRawArrayRegion(idata, 0, dataLen, (jbyte*)buf);
            a = BN_bin2bn(buf, (INTEGER)dataLen, NULL);

            KNI_GetRawArrayRegion(iexp, 0, expLen, (jbyte*)buf);
            b = BN_bin2bn(buf, (INTEGER)expLen, NULL);

            KNI_GetRawArrayRegion(imod, 0, modLen, (jbyte*)buf);
            c = BN_bin2bn(buf, (INTEGER)modLen, NULL);

            d = BN_new((INTEGER)resLen);

            ctx = BN_CTX_new((INTEGER)maxLen);

            /* do the actual exponentiation */
            if (a != NULL && b != NULL && c != NULL && d != NULL &&
                    ctx != NULL && BN_mod_exp_mont(d,a,b,c,ctx)) {
                /* Covert result from BIGNUM d to char array */
                numbytes = BN_bn2bin(d, buf);
                KNI_SetRawArrayRegion(ires, 0, numbytes, (jbyte*)buf);
            } else {
                /* assume out of mem */
                KNI_ThrowNew(jsropOutOfMemoryError, "Mod Exp");
            }

            JAVAME_FREE(buf);

            BN_free(a);
            BN_free(b);
            BN_free(c);
            BN_free(d);
            BN_CTX_free(ctx);
        }
    }

    KNI_EndHandles();

    KNI_ReturnInt((jint)numbytes);
}

/*=========================================================================
 * FUNCTION:      nativetx([B[I[I[BII[BI)V (STATIC)
 * CLASS:         com/sun/midp/crypto/ARC4
 * TYPE:          static native function
 * OVERVIEW:      Transform the given buffer
 * INTERFACE (operand stack manipulation):
 *   parameters:  S      array of S box values
 *                X      first set intermediate results
 *                Y      second set of intermediate results
 *                inbuf  input buffer of data 
 *                inoff  offset in the provided input buffer
 *                inlen  length of data to be processed
 *                outbuf output buffer of data 
 *                outoff offset in the provided output buffer
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_crypto_ARC4_nativetx) {
    unsigned int x;
    unsigned int y;
    unsigned int tx;
    unsigned int ty;
    long outoff;
    long len;
    long inoff;
    long temp;
    long i;

    outoff = KNI_GetParameterAsInt(8);
    len    = KNI_GetParameterAsInt(6);
    inoff  = KNI_GetParameterAsInt(5);

    KNI_StartHandles(5);

    KNI_DeclareHandle(outbuf);
    KNI_DeclareHandle(inbuf);
    KNI_DeclareHandle(Y);
    KNI_DeclareHandle(X);
    KNI_DeclareHandle(S);

    KNI_GetParameterAsObject(7, outbuf);
    KNI_GetParameterAsObject(4, inbuf);
    KNI_GetParameterAsObject(3, Y);
    KNI_GetParameterAsObject(2, X);
    KNI_GetParameterAsObject(1, S);

    /*copy in the counters*/
    KNI_GetRawArrayRegion(X, 0, 4, (jbyte*)&temp);
    x = temp;
    KNI_GetRawArrayRegion(Y, 0, 4, (jbyte*)&temp);
    y = temp;

    for (i = 0 ; i < len; i++) {
        x = (x + 1) & 0xff;
        y = (y + KNI_GetByteArrayElement(S, x)) & 0xff;
        tx = KNI_GetByteArrayElement(S, x);
        KNI_SetByteArrayElement(S, x, KNI_GetByteArrayElement(S, y));
        KNI_SetByteArrayElement(S, y, tx);
        ty = (unsigned int)(KNI_GetByteArrayElement(S, x) + 
                            KNI_GetByteArrayElement(S, y)) & 0xff;
        KNI_SetByteArrayElement(outbuf, i+outoff, 
            KNI_GetByteArrayElement(S, ty) ^ 
            KNI_GetByteArrayElement(inbuf, i+inoff));
    }

    temp = x;
    KNI_SetRawArrayRegion(X, 0, 4, (jbyte*)&temp);
    temp = y;
    KNI_SetRawArrayRegion(Y, 0, 4, (jbyte*)&temp);

    KNI_EndHandles();
    KNI_ReturnVoid();
}
