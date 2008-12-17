/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/*
 * WARNING - THIS IS AN EXPERIMENTAL FEATURE OF KVM THAT MAY, OR MAY NOT
 * EXIST IN A FUTURE VERSION OF THIS PRODUCT. IT IS NOT A PART OF THE
 * CLDC SPECIFICATION AND IS PROVIDED FOR ILLUSTRATIVE PURPOSES ONLY
 */

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>

#include <midpError.h>
#include <SHA.h>
#include <MD5.h>
#include <MD2.h>

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_MD2_nativeUpdate() {
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    int i, j;
    MD2_CTX c;

    KNI_StartHandles(5);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(cksm);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(7, data);
    KNI_GetParameterAsObject(6, cksm);
    KNI_GetParameterAsObject(5, num);
    KNI_GetParameterAsObject(4, state);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(num,  0,  4, (jbyte*)&(c.num));
    KNI_GetRawArrayRegion(data, 0, 16, (jbyte*)&(c.data));
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_GetRawArrayRegion(cksm,  j,  4, (jbyte*)&(c.cksm[i]));
        j += 4;
    }
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_GetRawArrayRegion(state,  j,  4, (jbyte*)&(c.state[i]));
        j += 4;
    }
        
    /* Do SHA Update */
    SNI_BEGIN_RAW_POINTERS;

    MD2_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);

    SNI_END_RAW_POINTERS;

    /* Copy Context back */
    KNI_SetRawArrayRegion(num,  0,  4, (jbyte*)&(c.num));
    KNI_SetRawArrayRegion(data, 0, 16, (jbyte*)&(c.data));
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_SetRawArrayRegion(cksm,  j,  4, (jbyte*)&(c.cksm[i]));
        j += 4;
    }
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_SetRawArrayRegion(state,  j,  4, (jbyte*)&(c.state[i]));
        j += 4;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_MD2_nativeFinal() {
    unsigned long outoff = KNI_GetParameterAsInt(5);
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    unsigned char md[16];
    int i,j;
    MD2_CTX c;
        
    KNI_StartHandles(6);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(cksm);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(outbuf);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(9, data);
    KNI_GetParameterAsObject(8, cksm);
    KNI_GetParameterAsObject(7, num);
    KNI_GetParameterAsObject(6, state);
    KNI_GetParameterAsObject(4, outbuf);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(num,  0,  4, (jbyte*)&(c.num));
    KNI_GetRawArrayRegion(data, 0, 16, (jbyte*)&(c.data));
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_GetRawArrayRegion(cksm,  j,  4, (jbyte*)&(c.cksm[i]));
        j += 4;
    }
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_GetRawArrayRegion(state,  j,  4, (jbyte*)&(c.state[i]));
        j += 4;
    }
        
    /* Perform MD2Update if necessary */
    if (inlen != 0) {
	/* Do MD2 Update */
        SNI_BEGIN_RAW_POINTERS;

        MD2_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);

        SNI_END_RAW_POINTERS;
    }

    /* Create and initialize the mesage digest buffer */
    for (i = 0; i < 16; i++) {
        md[i] = 0;
    }

    /* Do MD2 final */
    MD2_Final(md, &c);

    /* Copy the message digest into output buffer at offset outoff */
    for (i = 0; i < 16; i++) {
        KNI_SetRawArrayRegion(outbuf, i+outoff, 1, (jbyte*)&(md[i]));
    }

    /* Reset the context */
    c.num = 0;
    for (i = 0 ; i < 16; i ++) {
        c.data[i] = 0;
        c.cksm[i] = 0;
        c.state[i] = 0;
    }

    /* Copy back the context for next use. */
    KNI_SetRawArrayRegion(num,  0,  4, (jbyte*)&(c.num));
    KNI_SetRawArrayRegion(data, 0, 16, (jbyte*)&(c.data));
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_SetRawArrayRegion(cksm,  j,  4, (jbyte*)&(c.cksm[i]));
        j += 4;
    }
    for (i = 0, j = 0 ; i < 16; i ++) {
	KNI_SetRawArrayRegion(state,  j,  4, (jbyte*)&(c.state[i]));
        j += 4;
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_MD5_nativeFinal() {
    unsigned long outoff = KNI_GetParameterAsInt(5);
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    unsigned char md[16];
    int i,j;
    MD5_CTX c;

    KNI_StartHandles(6);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(count);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(outbuf);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(9, data);
    KNI_GetParameterAsObject(8, count);
    KNI_GetParameterAsObject(7, num);
    KNI_GetParameterAsObject(6, state);
    KNI_GetParameterAsObject(4, outbuf);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(state,  0, 4, (jbyte*)&(c.A));
    KNI_GetRawArrayRegion(state,  4, 4, (jbyte*)&(c.B));
    KNI_GetRawArrayRegion(state,  8, 4, (jbyte*)&(c.C));
    KNI_GetRawArrayRegion(state, 12, 4, (jbyte*)&(c.D));
    KNI_GetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_GetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_GetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_GetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    /* Perform MD5Update if necessary */
    if (inlen != 0) {
        /* Do MD5 Update */
        SNI_BEGIN_RAW_POINTERS;

        MD5_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);
        
        SNI_END_RAW_POINTERS;

    }

    /* Create and initialize the mesage digest buffer */
    for (i = 0; i < 16; i++) {
        md[i] = 0;
    }

    /* Do MD5 final */
    MD5_Final(md, &c);

    /* Copy the message digest into output buffer at offset outoff */
    for (i = 0; i < 16; i++) {
	KNI_SetRawArrayRegion(outbuf,  i+outoff, 1, (jbyte*)&(md[i]));
    }

    /* Reset the context */
    c.A = (unsigned long)0x67452301L;
    c.B = (unsigned long)0xefcdab89L;
    c.C = (unsigned long)0x98badcfeL;
    c.D = (unsigned long)0x10325476L;
    c.Nl= 0;
    c.Nh = 0;
    c.num = 0;
    for (i = 0; i < 16; i ++) {
        c.data[i] = 0;
    }
        

    /* Copy back the context for next use. */
    KNI_SetRawArrayRegion(state,  0, 4, (jbyte*)&(c.A));
    KNI_SetRawArrayRegion(state,  4, 4, (jbyte*)&(c.B));
    KNI_SetRawArrayRegion(state,  8, 4, (jbyte*)&(c.C));
    KNI_SetRawArrayRegion(state, 12, 4, (jbyte*)&(c.D));
    KNI_SetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_SetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_SetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_SetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    KNI_EndHandles();
    KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_MD5_nativeUpdate() {
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    int i, j;
    MD5_CTX c;
        
    KNI_StartHandles(5);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(count);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(7, data);
    KNI_GetParameterAsObject(6, count);
    KNI_GetParameterAsObject(5, num);
    KNI_GetParameterAsObject(4, state);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(state,  0, 4, (jbyte*)&(c.A));
    KNI_GetRawArrayRegion(state,  4, 4, (jbyte*)&(c.B));
    KNI_GetRawArrayRegion(state,  8, 4, (jbyte*)&(c.C));
    KNI_GetRawArrayRegion(state, 12, 4, (jbyte*)&(c.D));
    KNI_GetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_GetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_GetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_GetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    /* Do MD5 Update */
    SNI_BEGIN_RAW_POINTERS;

    MD5_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);

    SNI_END_RAW_POINTERS;

    /* Copy back the context for next use. */
    KNI_SetRawArrayRegion(state,  0, 4, (jbyte*)&(c.A));
    KNI_SetRawArrayRegion(state,  4, 4, (jbyte*)&(c.B));
    KNI_SetRawArrayRegion(state,  8, 4, (jbyte*)&(c.C));
    KNI_SetRawArrayRegion(state, 12, 4, (jbyte*)&(c.D));
    KNI_SetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_SetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_SetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_SetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_SHA_nativeFinal() {
    unsigned long outoff = KNI_GetParameterAsInt(5);
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    unsigned char md[20];
    int i, j;
    SHA_CTX c;
        
    KNI_StartHandles(6);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(count);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(outbuf);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(9, data);
    KNI_GetParameterAsObject(8, count);
    KNI_GetParameterAsObject(7, num);
    KNI_GetParameterAsObject(6, state);
    KNI_GetParameterAsObject(4, outbuf);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(state,  0, 4, (jbyte*)&(c.h0));
    KNI_GetRawArrayRegion(state,  4, 4, (jbyte*)&(c.h1));
    KNI_GetRawArrayRegion(state,  8, 4, (jbyte*)&(c.h2));
    KNI_GetRawArrayRegion(state, 12, 4, (jbyte*)&(c.h3));
    KNI_GetRawArrayRegion(state, 16, 4, (jbyte*)&(c.h4));
    KNI_GetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_GetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_GetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_GetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    /* Perform SHA update if necessary */
    if (inlen != 0) {
        /* Do SHA Update */
        SNI_BEGIN_RAW_POINTERS;

        SHA1_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);

        SNI_END_RAW_POINTERS;
    }

    /* Create and initialize the mesage digest buffer */
    for (i=0; i < 20; i++) {
        md[i] = 0;
    }

    /* Do SHA */
    SHA1_Final(md, &c);

    /* Copy message digest into output buffer at offset outoff */
    for (i = 0; i < 20; i++) {
        KNI_SetRawArrayRegion(outbuf, i+outoff, 1, (jbyte*)&(md[i]));
    }

    /* Reset the context */
    c.h0 = (unsigned long)0x67452301L;
    c.h1 = (unsigned long)0xefcdab89L;
    c.h2 = (unsigned long)0x98badcfeL;
    c.h3 = (unsigned long)0x10325476L;
    c.h4 = (unsigned long)0xc3d2e1f0L;
    c.Nl = 0;
    c.Nh = 0;
    c.num = 0;
        
    /* Copy back the context for next use. */
    KNI_SetRawArrayRegion(state,  0, 4, (jbyte*)&(c.h0));
    KNI_SetRawArrayRegion(state,  4, 4, (jbyte*)&(c.h1));
    KNI_SetRawArrayRegion(state,  8, 4, (jbyte*)&(c.h2));
    KNI_SetRawArrayRegion(state, 12, 4, (jbyte*)&(c.h3));
    KNI_SetRawArrayRegion(state, 16, 4, (jbyte*)&(c.h4));
    KNI_SetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_SetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_SetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_SetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Accumulates a hash of the input data. This method is useful when
 * the input data to be hashed is not available in one byte array. 
 * @param inBuf input buffer of data to be hashed
 * @param inOff offset within inBuf where input data begins
 * @param inLen length (in bytes) of data to be hashed
 * @param state internal hash state
 * @param num internal hash state
 * @param count internal hash state
 * @param data internal hash state
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_crypto_SHA_nativeUpdate() {
    unsigned long  inlen = KNI_GetParameterAsInt(3);
    unsigned long  inoff = KNI_GetParameterAsInt(2);
    int i, j;
    SHA_CTX c;
        
    KNI_StartHandles(5);

    KNI_DeclareHandle(data);
    KNI_DeclareHandle(count);
    KNI_DeclareHandle(num);
    KNI_DeclareHandle(state);
    KNI_DeclareHandle(inbuf);

    KNI_GetParameterAsObject(7, data);
    KNI_GetParameterAsObject(6, count);
    KNI_GetParameterAsObject(5, num);
    KNI_GetParameterAsObject(4, state);
    KNI_GetParameterAsObject(1, inbuf);

    /* Copy the context in */
    KNI_GetRawArrayRegion(state,  0, 4, (jbyte*)&(c.h0));
    KNI_GetRawArrayRegion(state,  4, 4, (jbyte*)&(c.h1));
    KNI_GetRawArrayRegion(state,  8, 4, (jbyte*)&(c.h2));
    KNI_GetRawArrayRegion(state, 12, 4, (jbyte*)&(c.h3));
    KNI_GetRawArrayRegion(state, 16, 4, (jbyte*)&(c.h4));
    KNI_GetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_GetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_GetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_GetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }

    /* Do SHA Update */
    SNI_BEGIN_RAW_POINTERS;

    SHA1_Update(&c, (unsigned char*)&(JavaByteArray(inbuf)[inoff]), inlen);

    SNI_END_RAW_POINTERS;

    /* Copy Context back */
    KNI_SetRawArrayRegion(state,  0, 4, (jbyte*)&(c.h0));
    KNI_SetRawArrayRegion(state,  4, 4, (jbyte*)&(c.h1));
    KNI_SetRawArrayRegion(state,  8, 4, (jbyte*)&(c.h2));
    KNI_SetRawArrayRegion(state, 12, 4, (jbyte*)&(c.h3));
    KNI_SetRawArrayRegion(state, 16, 4, (jbyte*)&(c.h4));
    KNI_SetRawArrayRegion(count,  0, 4, (jbyte*)&(c.Nl));
    KNI_SetRawArrayRegion(count,  4, 4, (jbyte*)&(c.Nh));
    KNI_SetRawArrayRegion(num,    0, 4, (jbyte*)&(c.num));
    for (i = 0, j = 0 ; i < 16; i ++) {
        KNI_SetRawArrayRegion(data, j, 4, (jbyte*)&(c.data[i]));
        j += 4;
    }
        
    KNI_EndHandles();
    KNI_ReturnVoid();
}

