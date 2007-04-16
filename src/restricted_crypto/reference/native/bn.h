/*
 *  
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
/**
 * This code is based on the big number code from OpenSSL
 * The copyright notice governing fair use of OpenSSL code is included
 * below.
 * 
 * Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
#ifndef HEADER_BN_H
#define HEADER_BN_H

#define SIXTEEN_BIT
#define BN_LLONG
#define INTEGER short
#define BN_MAX_INTEGER (0x7FFF)

#define BN_RECP
#define MONT_WORD

#ifdef SIXTEEN_BIT
#ifndef BN_DIV2W
#define BN_DIV2W
#endif
#define BN_ULLONG	unsigned long
#define BN_ULONG	unsigned short
#define BN_LONG		short
#define BN_BITS		32
#define BN_BYTES	2
#define BN_BITS2	16
#define BN_BITS4	8
#define BN_MASK2	(0xffff)
#define BN_MASK2l	(0xff)
#define BN_MASK2h1	(0xff80)
#define BN_MASK2h	(0xff00)
#define BN_TBIT		(0x8000)
#endif

typedef struct bignum_st
	{
	BN_ULONG *d;	/* Pointer to an array of 'BN_BITS2' bit chunks. */
	int top;	/* Index of last used d +1. */
	/* The next are internal book keeping for bn_expand. */
	int max;	/* Size of the d array. */
	int neg;	/* one if the number is negative */
        INTEGER byteSize;   /* Size of the number in bytes. */
	} BIGNUM;

/* Used for temp variables */
#define BN_CTX_NUM	12
typedef struct bignum_ctx
	{
	int tos;
	BIGNUM *bn[BN_CTX_NUM+1];
	} BN_CTX;


#define BN_num_bytes(a)	((BN_num_bits(a) + 7) / 8)
#define BN_is_zero(a)	(((a)->top <= 1) && ((a)->d[0] == (BN_ULONG)0))
#define BN_one(a)	(BN_set_word((a),1))
#define BN_zero(a)	(BN_set_word((a),0))

#define bn_fix_top(a) \
	{ \
	BN_ULONG *fix_top_l; \
	for (fix_top_l= &((a)->d[(a)->top-1]); (a)->top > 0; (a)->top--) \
		if (*(fix_top_l--)) break; \
	}

#define bn_expand(n,b) ((((b)/BN_BITS2) <= (n)->max)?(n):bn_expand2((n),(b)))
#define Lw(t)    (((BN_ULONG)(t))&BN_MASK2)
#define Hw(t)    (((BN_ULONG)((t)>>BN_BITS2))&BN_MASK2)

#ifdef BN_LLONG
#define mul_add(r,a,w,c) { \
        BN_ULLONG t; \
        t=(BN_ULLONG)w * (a) + (r) + (c); \
        (r)=Lw(t); \
        (c)= Hw(t); \
        }

#define mul(r,a,w,c) { \
        BN_ULLONG t; \
        t=(BN_ULLONG)w * (a) + (c); \
        (r)=Lw(t); \
        (c)= Hw(t); \
        }
#endif

/* Used for Montgomery Multiplication */
typedef struct bn_mont_ctx_st
        {
        int ri;         /* number of bits in R */
        BIGNUM *RR;     /* used to convert to montgomery form */
        BIGNUM *N;      /* The modulus */
        BIGNUM *Ni;     /* The inverse of N */
        BN_ULONG n0;    /* word form of inverse, normally only one of
                         * Ni or n0 is defined */
        } BN_MONT_CTX;


INTEGER BN_mod_exp_mont(BIGNUM *r, BIGNUM *a, BIGNUM *p, BIGNUM *m,BN_CTX *ctx);
BIGNUM  *BN_value_one();
INTEGER BN_mask_bits();
BIGNUM  *BN_mod_inverse();
INTEGER BN_mod_exp(BIGNUM *r, BIGNUM *a, BIGNUM *p, BIGNUM *m,BN_CTX *ctx);
BN_MONT_CTX *BN_MONT_CTX_new(INTEGER bytes);
INTEGER BN_from_montgomery(BIGNUM *r,BIGNUM *a,BN_MONT_CTX *mont,BN_CTX *ctx);
void BN_MONT_CTX_free(BN_MONT_CTX *mont);
INTEGER BN_MONT_CTX_set(BN_MONT_CTX *mont,BIGNUM *modulus,BN_CTX *ctx);
INTEGER BN_mod_mul_montgomery(BIGNUM *r,BIGNUM *a,BIGNUM *b,BN_MONT_CTX *mont,BN_CTX * ctx);

#define BN_to_montgomery(r,a,mont,ctx) BN_mod_mul_montgomery(r,a,(mont)->RR,(mont),ctx)
#define BN_is_odd(a)    ((a)->d[0] & 1)
#define BN_is_one(a)    (BN_is_word((a),1))
#define BN_is_word(a,w) (((a)->top == 1) && ((a)->d[0] == (BN_ULONG)(w)))


INTEGER	BN_add(BIGNUM *r, BIGNUM *a, BIGNUM *b);
void	bn_qadd(BIGNUM *r, BIGNUM *a, BIGNUM *b);
INTEGER	BN_div(BIGNUM *dv, BIGNUM *rem, BIGNUM *m, BIGNUM *d, BN_CTX *ctx);
INTEGER BN_num_bits(BIGNUM *a);
INTEGER	BN_num_bits_word(BN_ULONG);
void	BN_clear_free(BIGNUM *a);
BN_CTX *BN_CTX_new(INTEGER bytes);
void	BN_CTX_free(BN_CTX *c);
BIGNUM *BN_new(INTEGER bytes);
BIGNUM *BN_bin2bn(unsigned char *s,INTEGER len,BIGNUM *ret);
INTEGER BN_bn2bin(BIGNUM *a, unsigned char *to);
void	BN_free(BIGNUM *a);
BIGNUM *bn_expand2(BIGNUM *b, INTEGER bits);
BIGNUM *BN_copy(BIGNUM *a, BIGNUM *b);
INTEGER	BN_set_word(BIGNUM *a, unsigned long w);
INTEGER	BN_ucmp(BIGNUM *a, BIGNUM *b);
INTEGER	BN_is_bit_set(BIGNUM *a, INTEGER n);
INTEGER	BN_mod(BIGNUM *rem, BIGNUM *m, BIGNUM *d, BN_CTX *ctx);
INTEGER	BN_mul(BIGNUM *r, BIGNUM *a, BIGNUM *b);
BN_ULONG bn_mul_add_word(BN_ULONG *rp, BN_ULONG *ap, short num, BN_ULONG w);
BN_ULONG bn_mul_word(BN_ULONG *rp, BN_ULONG *ap, short num, BN_ULONG w);
void     bn_sqr_words(BN_ULONG *rp, BN_ULONG *ap, short num);
BN_ULONG bn_div64(BN_ULONG h, BN_ULONG l, BN_ULONG d);
INTEGER	BN_rshift(BIGNUM *r, BIGNUM *a, INTEGER n);
INTEGER	BN_lshift(BIGNUM *r, BIGNUM *a, INTEGER n);
INTEGER	BN_sub(BIGNUM *r, BIGNUM *a, BIGNUM *b);
void	bn_qsub(BIGNUM *r, BIGNUM *a, BIGNUM *b);
INTEGER	BN_sqr(BIGNUM *r, BIGNUM *a,BN_CTX *ctx);

#endif
