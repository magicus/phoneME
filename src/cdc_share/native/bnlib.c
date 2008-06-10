/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights 
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

#include <bn.h>

//#include <stdio.h>
#include <string.h>
//#include <stdlib.h>

#include "jsrop_logging.h"
#include "jsrop_memory.h"

/* r can == a or b */
INTEGER BN_add(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
{
    INTEGER i;
    BIGNUM *tmp;

    /*  a +  b  a+b
     *  a + -b  a-b
     * -a +  b  b-a
     * -a + -b  -(a+b)
     */
    if (a->neg ^ b->neg)
        {
        /* only one is negative */
        if (a->neg)
            { tmp=a; a=b; b=tmp; }

        /* we are now a - b */
        if (bn_expand(r,(INTEGER)(((a->top > b->top)?a->top:b->top)*BN_BITS2))
            == NULL) return(0);

        if (BN_ucmp(a,b) < 0)
            {
            bn_qsub(r,b,a);
            r->neg=1;
            }
        else
            {
            bn_qsub(r,a,b);
            r->neg=0;
            }
        return(1);
        }

    if (a->neg) /* both are neg */
        r->neg=1;
    else
        r->neg=0;

    i=(a->top > b->top);
    if (bn_expand(r,(INTEGER)((((i)?a->top:b->top)+1)*BN_BITS2)) == NULL) return(0);

    if (i)
        bn_qadd(r,a,b);
    else
        bn_qadd(r,b,a);
    return(1);
}


/* unsigned add of b to a, r must be large enough */
void bn_qadd(r,a,b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
{
    register INTEGER i;
    INTEGER max,min;
    BN_ULONG *ap,*bp,*rp,carry,t1,t2;

    max=a->top;
    min=b->top;
    r->top=max;

    ap=a->d;
    bp=b->d;
    rp=r->d;
    carry=0;
    for (i=0; i<min; i++)
        {
        t1= *(ap++);
        t2= *(bp++);
        if (carry)
            {
            carry=(t2 >= ((~t1)&BN_MASK2));
            t2=(t1+t2+1)&BN_MASK2;
            }
        else
            {
            t2=(t1+t2)&BN_MASK2;
            carry=(t2 < t1);
            }
        *(rp++)=t2;
        }
    if (carry)
        {
        while (i < max)
            {
            t1= *(ap++);
            t2=(t1+1)&BN_MASK2;
            *(rp++)=t2;
            carry=(t2 < t1);
            i++;
            if (!carry) break;
            }
        if ((i >= max) && carry)
            {
            *(rp++)=1;
            r->top++;
            }
        }
    for (; i<max; i++)
        *(rp++)= *(ap++);
    /* memcpy(rp,ap,sizeof(*ap)*(max-i));*/
}


INTEGER BN_div(dv, rm, num, divisor,ctx)
BIGNUM *dv;
BIGNUM *rm;
BIGNUM *num;
BIGNUM *divisor;
BN_CTX *ctx;
{
    INTEGER norm_shift,i,j,loop;
    BIGNUM *tmp,wnum,*snum,*sdiv,*res;
    BN_ULONG *resp,*wnump;
    BN_ULONG d0,d1;
    INTEGER num_n,div_n;

    if (BN_is_zero(divisor))
        {
	    REPORT_ERROR(LC_SECURITY,"BN_div: Division by zero"); 
	    return(0);
        }

    if (BN_is_zero(num))
        {
            if (rm != NULL) BN_zero(rm);
            if (dv != NULL) BN_zero(dv);
            return(1);
        }

    if (BN_ucmp(num,divisor) < 0)
        {
        if (rm != NULL)
            { if (BN_copy(rm,num) == NULL) return(0); }
        if (dv != NULL) BN_zero(dv);
        return(1);
        }

    tmp=ctx->bn[ctx->tos]; 
    tmp->neg=0;
    snum=ctx->bn[ctx->tos+1];
    sdiv=ctx->bn[ctx->tos+2];
    if (dv == NULL)
        res=ctx->bn[ctx->tos+3];
    else    res=dv;

    /* First we normalise the numbers */
    norm_shift=BN_BITS2-((BN_num_bits(divisor))%BN_BITS2);
    BN_lshift(sdiv,divisor,norm_shift);
    sdiv->neg=0;
    norm_shift+=BN_BITS2;
    BN_lshift(snum,num,norm_shift);
    snum->neg=0;
    div_n=sdiv->top;
    num_n=snum->top;
    loop=num_n-div_n;

    /* Lets setup a 'window' into snum
     * This is the part that corresponds to the current
     * 'area' being divided */
    wnum.d=  &(snum->d[loop]);
    wnum.top= div_n;
    wnum.max= snum->max; /* a bit of a lie */
    wnum.neg= 0;

    /* Get the top 2 words of sdiv */
    /* i=sdiv->top; */
    d0=sdiv->d[div_n-1];
    d1=(div_n == 1)?0:sdiv->d[div_n-2];

    /* pointer to the 'top' of snum */
    wnump= &(snum->d[num_n-1]);

    /* Setup to 'res' */
    res->neg= (num->neg^divisor->neg);
    res->top=loop;
    if (!bn_expand(res,(INTEGER)((loop+1)*BN_BITS2))) goto err;
    resp= &(res->d[loop-1]);

    /* space for temp */
    if (!bn_expand(tmp,(INTEGER)((div_n+1)*BN_BITS2))) goto err;

    if (BN_ucmp(&wnum,sdiv) >= 0)
        {
        bn_qsub(&wnum,&wnum,sdiv);
        *resp=1;
        res->d[res->top-1]=1;
        }
    else
        res->top--;
    resp--;

    for (i=0; i<loop-1; i++)
        {
        BN_ULONG q,n0,n1;
        BN_ULONG l0;

        wnum.d--; wnum.top++;
        n0=wnump[0];
        n1=wnump[-1];
        if (n0 == d0)
            q=BN_MASK2;
        else
            q=bn_div64(n0,n1,d0);
        {
#ifdef BN_LLONG
        BN_ULLONG t1,t2,rem;
        t1=((BN_ULLONG)n0<<BN_BITS2)|n1;
        for (;;)
            {
            t2=(BN_ULLONG)d1*q;
            rem=t1-(BN_ULLONG)q*d0;
            if ((rem>>BN_BITS2) ||
                (t2 <= ((BN_ULLONG)(rem<<BN_BITS2)+wnump[-2])))
                break;
            q--;
            }
#else
        BN_ULONG t1l,t1h,t2l,t2h,t3l,t3h,ql,qh,t3t;
        t1h=n0;
        t1l=n1;
        for (;;)
            {
            t2l=LBITS(d1); t2h=HBITS(d1);
            ql =LBITS(q);  qh =HBITS(q);
            mul64(t2l,t2h,ql,qh); /* t2=(BN_ULLONG)d1*q; */

            t3t=LBITS(d0); t3h=HBITS(d0);
            mul64(t3t,t3h,ql,qh); /* t3=t1-(BN_ULLONG)q*d0; */
            t3l=(t1l-t3t);
            if (t3l > t1l) t3h++;
            t3h=(t1h-t3h);

            /*if ((t3>>BN_BITS2) ||
                (t2 <= ((t3<<BN_BITS2)+wnump[-2])))
                break; */
            if (t3h) break;
            if (t2h < t3l) break;
            if ((t2h == t3l) && (t2l <= wnump[-2])) break;

            q--;
            }
#endif
        }
        l0=bn_mul_word(tmp->d,sdiv->d,div_n,q);
        tmp->d[div_n]=l0;
        for (j=div_n+1; j>0; j--)
            if (tmp->d[j-1]) break;
        tmp->top=j;

        j=wnum.top;
        BN_sub(&wnum,&wnum,tmp);

        snum->top=snum->top+wnum.top-j;

        if (wnum.neg)
            {
            q--;
            j=wnum.top;
            BN_add(&wnum,&wnum,sdiv);
            snum->top+=wnum.top-j;
            }
        *(resp--)=q;
        wnump--;
        }
    if (rm != NULL)
        {
        BN_rshift(rm,snum,norm_shift);
        rm->neg=num->neg;
        }
    return(1);
err:
    return(0);
}


INTEGER BN_num_bits_word(BN_ULONG l)
{
    static char bits[256]={
        0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        };

#ifdef SIXTY_FOUR_BIT_LONG
    if (l & 0xffffffff00000000L)
        {
        if (l & 0xffff000000000000L)
            {
            if (l & 0xff00000000000000L)
                {
                return(bits[l>>56]+56);
                }
            else    return(bits[l>>48]+48);
            }
        else
            {
            if (l & 0x0000ff0000000000L)
                {
                return(bits[l>>40]+40);
                }
            else    return(bits[l>>32]+32);
            }
        }
    else
#else
#ifdef SIXTY_FOUR_BIT
    if (l & 0xffffffff00000000LL)
        {
        if (l & 0xffff000000000000LL)
            {
            if (l & 0xff00000000000000LL)
                {
                return(bits[l>>56]+56);
                }
            else    return(bits[l>>48]+48);
            }
        else
            {
            if (l & 0x0000ff0000000000LL)
                {
                return(bits[l>>40]+40);
                }
            else    return(bits[l>>32]+32);
            }
        }
    else
#endif
#endif
        {
#if defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
        if (l & 0xffff0000L)
            {
            if (l & 0xff000000L)
                return(bits[l>>24L]+24);
            else    return(bits[l>>16L]+16);
            }
        else
#endif
            {
#if defined(SIXTEEN_BIT) || defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
            if (l & 0xff00L)
                return(bits[l>>8]+8);
            else    
#endif
                return(bits[l   ]  );
            }
        }
}


INTEGER BN_num_bits(a)
BIGNUM *a;
{
    BN_ULONG l;
    INTEGER i;

    if (a->top == 0) return(0);
    l=a->d[a->top-1];
    i=(a->top-1)*BN_BITS2;
    if (l == 0)
        {
	    REPORT_ERROR(LC_SECURITY, "BN_num_bits: Bad Top Value");
            return(0);
        }
    return(i+BN_num_bits_word(l));
}


void BN_clear_free(a)
BIGNUM *a;
{
    if (a == NULL) return;
    if (a->d != NULL)
        {
        memset(a->d,0,a->max*sizeof(a->d[0]));
        JAVAME_FREE(a->d);
        }
    memset(a,0,sizeof(BIGNUM));
    JAVAME_FREE(a);
}


void BN_free(a)
BIGNUM *a;
{
    if (a == NULL) return;
    if (a->d != NULL) JAVAME_FREE(a->d);
    JAVAME_FREE(a);
}


BIGNUM *BN_new(INTEGER bytes)
{
    BIGNUM *ret;
    BN_ULONG *p;

    if (bytes < 0) {
        REPORT_ERROR(LC_SECURITY, "BN_new::number of bytes is negative");
        return(NULL);
    }

    ret=(BIGNUM *)JAVAME_MALLOC(sizeof(BIGNUM));
    if (ret == NULL) goto err;
    ret->top=0;
    ret->neg=0;
        ret->byteSize = bytes;
    ret->max=((ret->byteSize * 8)/BN_BITS2);
    p=(BN_ULONG *)JAVAME_MALLOC(sizeof(BN_ULONG)*(ret->max+1));
    if (p == NULL) goto err;
    ret->d=p;

    memset(p,0,(ret->max+1)*sizeof(p[0]));
    return(ret);
err:
    if (ret != NULL) JAVAME_FREE(ret);

    REPORT_ERROR(LC_SECURITY, "BN_new::malloc failure");
    return(NULL);
}

BN_CTX *BN_CTX_new(INTEGER bytes)
{
    BN_CTX *ret;
    BIGNUM *n;
    INTEGER i,j;

    ret=(BN_CTX *)JAVAME_MALLOC(sizeof(BN_CTX));
    if (ret == NULL) goto err2;

    for (i=0; i<BN_CTX_NUM; i++)
        {
        n=BN_new(bytes);
        if (n == NULL) goto err;
        ret->bn[i]=n;
        }

    /* There is actually an extra one, this is for debugging my
     * stuff */
    ret->bn[BN_CTX_NUM]=NULL;

    ret->tos=0;
    return(ret);
err:
    for (j=0; j<i; j++)
        BN_free(ret->bn[j]);
    JAVAME_FREE(ret);
err2:
    REPORT_ERROR(LC_SECURITY, "BN_CTX_new::malloc failure");
    return(NULL);
}

void BN_CTX_free(c)
BN_CTX *c;
{
    INTEGER i;

    if (c == NULL) return;

    for (i=0; i<BN_CTX_NUM+1; i++)
        BN_clear_free(c->bn[i]);

    JAVAME_FREE(c);
}


BIGNUM *bn_expand2(BIGNUM *b, INTEGER bits)
{
    BN_ULONG *p;
    register INTEGER n;
    
    while (bits > b->max*BN_BITS2)
        {
        n=((bits+BN_BITS2-1)/BN_BITS2)*2;
        #if 1
        p = (BN_ULONG *)JAVAME_MALLOC(sizeof(BN_ULONG)*(n+1));
        #else
        p=b->d=(BN_ULONG *)midpRealloc(b->d,sizeof(BN_ULONG)*(n+1));
        #endif
        if (p == NULL)
            {
                REPORT_ERROR(LC_SECURITY, "bn_expand2::malloc failure");
                return(NULL);
            }
        #if 1
        memcpy(&(p[0]), &(b->d[0]), (b->max)*sizeof(BN_ULONG)); 
        memset(&(p[b->max]),0,((n+1)-b->max)*sizeof(BN_ULONG));
        JAVAME_FREE(b->d);
        b->d = p;
        b->max = n;
        #else
        memset(&(p[b->max]),0,((n+1)-b->max)*sizeof(BN_ULONG));
        b->max=n;
        #endif
        }
    return(b);
}

BIGNUM *BN_copy(a, b)
BIGNUM *a;
BIGNUM *b;
{
    if (bn_expand(a,(INTEGER)(b->top*BN_BITS2)) == NULL) return(NULL);
    memcpy(a->d,b->d,sizeof(b->d[0])*b->top);
/*  memset(&(a->d[b->top]),0,sizeof(a->d[0])*(a->max-b->top));*/
    a->top=b->top;
    a->neg=b->neg;
    return(a);
}

INTEGER BN_set_word(a,w)
BIGNUM *a;
unsigned long w;
{
    INTEGER i,n;
    if (bn_expand(a,(int)(sizeof(unsigned long)*8)) == NULL) {
        return(0);
    }

    n=sizeof(unsigned long)/BN_BYTES;
    a->neg=0;
    a->top=0;
    a->d[0]=(BN_ULONG)w&BN_MASK2;
    if (a->d[0] != 0) a->top=1;
    for (i=1; i<n; i++)
        {
        /* the following is done instead of
         * w>>=BN_BITS2 so compilers don't complain
         * on builds where sizeof(long) == BN_TYPES */
#ifndef SIXTY_FOUR_BIT /* the data item > unsigned long */
        w>>=BN_BITS4;
        w>>=BN_BITS4;
#endif
        a->d[i]=(BN_ULONG)w&BN_MASK2;
        if (a->d[i] != 0) a->top=i+1;
        }
    return(1);
}

/* ignore negative */
BIGNUM *BN_bin2bn(unsigned char *s, INTEGER len, BIGNUM *bn)
{
    BIGNUM *ret;
    unsigned INTEGER i,m;
    unsigned INTEGER n;
    BN_ULONG l;

    if (bn != NULL)
            {
            ret=bn;
            }
        else
            {
            ret=BN_new(len);
            if (ret == NULL) return(NULL);
            }

    l=0;
    n=len;
    if (n == 0)
        {
        ret->top=0;
        return(ret);
        }
    if (bn_expand(ret,(INTEGER)((n+2)*8)) == NULL)
       {
       if (bn == NULL) BN_free(ret);
       return(NULL);
       }

    i=((n-1)/BN_BYTES)+1;
    m=((n-1)%(BN_BYTES));
    ret->top=i;
    while (n-- > 0)
        {
        l=(l<<8L)| *(s++);
        if (m-- == 0)
            {
            ret->d[--i]=l;
            l=0;
            m=BN_BYTES-1;
            }
        }
    /* need to call this due to clear byte at top if avoiding
     * having the top bit set (-ve number) */
    bn_fix_top(ret);
    return(ret);
}

/* ignore negative */
INTEGER BN_bn2bin(a, to)
BIGNUM *a;
unsigned char *to;
{
    INTEGER n,i;
    BN_ULONG l;

    n=i=BN_num_bytes(a);
    while (i-- > 0)
        {
        l=a->d[i/BN_BYTES];
        *(to++)=(unsigned char)(l>>(8*(i%BN_BYTES)))&0xff;
        }
    return(n);
}

INTEGER BN_ucmp(a, b)
BIGNUM *a;
BIGNUM *b;
{
    INTEGER i;
    BN_ULONG t1,t2,*ap,*bp;

    i=a->top-b->top;
    if (i != 0) return(i);
    ap=a->d;
    bp=b->d;
    for (i=a->top-1; i>=0; i--)
        {
        t1= ap[i];
        t2= bp[i];
        if (t1 != t2)
            return(t1 > t2?1:-1);
        }
    return(0);
}


INTEGER BN_is_bit_set(BIGNUM *a, INTEGER n)
{
    INTEGER i,j;

    if (n < 0) return(0);
    i=n/BN_BITS2;
    j=n%BN_BITS2;
    if (a->top <= i) return(0);
    return((a->d[i]&(((BN_ULONG)1)<<j))?1:0);
}

/* rem != m */
INTEGER BN_mod(rem, m, d,ctx)
BIGNUM *rem;
BIGNUM *m;
BIGNUM *d;
BN_CTX *ctx;
{
    return(BN_div(NULL,rem,m,d,ctx));
}

/* r must be different to a and b */
INTEGER BN_mul(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
{
    INTEGER i;
    INTEGER max,al,bl;
    BN_ULONG *ap,*bp,*rp;

    al=a->top;
    bl=b->top;
    if ((al == 0) || (bl == 0))
        {
        r->top=0;
        return(1);
        }

    max=(al+bl);
    if (bn_expand(r,(INTEGER)((max)*BN_BITS2)) == NULL) return(0);
    r->top=max;
    r->neg=a->neg^b->neg;
    ap=a->d;
    bp=b->d;
    rp=r->d;

    rp[al]=bn_mul_word(rp,ap,al,*(bp++));
    rp++;
    for (i=1; i<bl; i++)
        {
        rp[al]=bn_mul_add_word(rp,ap,al,*(bp++));
        rp++;
        }
    if (r->d[max-1] == 0) r->top--;
    return(1);
}

BN_ULONG bn_div64(BN_ULONG h, BN_ULONG l, BN_ULONG d)
{
    return((BN_ULONG)(((((BN_ULLONG)h)<<BN_BITS2)|l)/(BN_ULLONG)d));
}


INTEGER BN_lshift(BIGNUM *r, BIGNUM *a, INTEGER n)
{
    INTEGER i,nw,lb,rb;
    BN_ULONG *t,*f;
    BN_ULONG l;

    r->neg=a->neg;
    if (bn_expand(r,(INTEGER)((a->top*BN_BITS2)+n)) == NULL) return(0);
    nw=n/BN_BITS2;
    lb=n%BN_BITS2;
    rb=BN_BITS2-lb;
    f=a->d;
    t=r->d;
    t[a->top+nw]=0;
    if (lb == 0)
        for (i=a->top-1; i>=0; i--)
            t[nw+i]=f[i];
    else
        for (i=a->top-1; i>=0; i--)
            {
            l=f[i];
            t[nw+i+1]|=(l>>rb)&BN_MASK2;
            t[nw+i]=(l<<lb)&BN_MASK2;
            }
    memset(t,0,nw*sizeof(t[0]));
/*  for (i=0; i<nw; i++)
        t[i]=0;*/
    r->top=a->top+nw+1;
    bn_fix_top(r);
    return(1);
}


INTEGER BN_rshift(BIGNUM *r, BIGNUM *a, INTEGER n)
{
    INTEGER i,j,nw,lb,rb;
    BN_ULONG *t,*f;
    BN_ULONG l,tmp;

    nw=n/BN_BITS2;
    rb=n%BN_BITS2;
    lb=BN_BITS2-rb;
    if (nw > a->top)
        {
        BN_zero(r);
        return(1);
        }
    if (r != a)
        {
        r->neg=a->neg;
        if (bn_expand(r,(INTEGER)((a->top-nw+1)*BN_BITS2)) == NULL) return(0);
        }

    f= &(a->d[nw]);
    t=r->d;
    j=a->top-nw;
    r->top=j;

    if (rb == 0)
        {
        for (i=j+1; i > 0; i--)
            *(t++)= *(f++);
        }
    else
        {
        l= *(f++);
        for (i=1; i<j; i++)
            {
            tmp =(l>>rb)&BN_MASK2;
            l= *(f++);
            *(t++) =(tmp|(l<<lb))&BN_MASK2;
            }
        *(t++) =(l>>rb)&BN_MASK2;
        }
    *t=0;
    bn_fix_top(r);
    return(1);
}

/* r must not be a */
/* I've just gone over this and it is now %20 faster on x86 - eay - 27 Jun 96 */
INTEGER BN_sqr(r, a, ctx)
BIGNUM *r;
BIGNUM *a;
BN_CTX *ctx;
{
    INTEGER i,j,max,al;
    BIGNUM *tmp;
    BN_ULONG *ap,*rp,c;

    tmp=ctx->bn[ctx->tos];

    al=a->top;
    if (al == 0)
        {
        r->top=0;
        return(1);
        }

    max=(al*2);
    if (bn_expand(r,(INTEGER)(max*BN_BITS2)) == NULL) return(0);
    if (bn_expand(tmp,(INTEGER)(max*BN_BITS2)) == NULL) return(0);

    r->neg=0;

    ap=a->d;
    rp=r->d;
    rp[0]=rp[max-1]=0;
    rp++;
    j=al;

    if (--j > 0)
        {
        ap++;
        rp[j]=bn_mul_word(rp,ap,j,ap[-1]);
        rp+=2;
        }

    for (i=2; i<al; i++)
        {
        j--;
        ap++;
        rp[j]=bn_mul_add_word(rp,ap,j,ap[-1]);
        rp+=2;
        }

    /* inlined shift, 2 words at once */
    j=max;
    rp=r->d;
    c=0;
    for (i=0; i<j; i++)
        {
        BN_ULONG t;

        t= *rp;
        *(rp++)=((t<<1)|c)&BN_MASK2;
        c=(t & BN_TBIT)?1:0;

#if 0
        t= *rp;
        *(rp++)=((t<<1)|c)&BN_MASK2;
        c=(t & BN_TBIT)?1:0;
#endif
        }
    /* there will not be a carry */

    bn_sqr_words(tmp->d,a->d,al);

    /* inlined add */
    ap=tmp->d;
    rp=r->d;
    c=0;
    j=max;
    for (i=0; i<j; i++)
        {
        BN_ULONG t1,t2;

        t1= *(ap++);
        t2= *rp;
        if (c)
            {
            c=(t2 >= ((~t1)&BN_MASK2));
            t2=(t1+t2+1)&BN_MASK2;
            }
        else
            {
            t2=(t1+t2)&BN_MASK2;
            c=(t2<t1);
            }
        *(rp++)=t2;
        }
    /* there will be no carry */

    r->top=max;
    if (r->d[max-1] == 0) r->top--;
    return(1);
}

/* unsigned subtraction of b from a, a must be larger than b. */
void bn_qsub(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
{
    INTEGER max,min;
    register BN_ULONG t1,t2,*ap,*bp,*rp;
    INTEGER i,carry;
#if defined(IRIX_CC_BUG) && !defined(LINT)
    INTEGER dummy;
#endif

    max=a->top;
    min=b->top;
    ap=a->d;
    bp=b->d;
    rp=r->d;

    carry=0;
    for (i=0; i<min; i++)
        {
        t1= *(ap++);
        t2= *(bp++);
        if (carry)
            {
            carry=(t1 <= t2);
            t1=(t1-t2-1);
            }
        else
            {
            carry=(t1 < t2);
            t1=(t1-t2);
            }
#if defined(IRIX_CC_BUG) && !defined(LINT)
        dummy=t1;
#endif
        *(rp++)=t1&BN_MASK2;
        }
    if (carry) /* subtracted */
        {
        while (i < max)
            {
            i++;
            t1= *(ap++);
            t2=(t1-1)&BN_MASK2;
            *(rp++)=t2;
            if (t1 > t2) break;
            }
        }
    memcpy(rp,ap,sizeof(*rp)*(max-i));
/*  for (; i<max; i++)
        *(rp++)= *(ap++);*/

    r->top=max;
    bn_fix_top(r);
}

INTEGER BN_sub(r, a, b)
BIGNUM *r;
BIGNUM *a;
BIGNUM *b;
{
    INTEGER max,i;
    INTEGER add=0,neg=0;
    BIGNUM *tmp;

    /*  a -  b  a-b
     *  a - -b  a+b
     * -a -  b  -(a+b)
     * -a - -b  b-a
     */
    if (a->neg)
        {
        if (b->neg)
            { tmp=a; a=b; b=tmp; }
        else
            { add=1; neg=1; }
        }
    else
        {
        if (b->neg) { add=1; neg=0; }
        }

    if (add)
        {
        i=(a->top > b->top);
            if (bn_expand(r,(INTEGER)((((i)?a->top:b->top)+1)*BN_BITS2)) == NULL)
            return(0);
        if (i)
            bn_qadd(r,a,b);
        else
            bn_qadd(r,b,a);
        r->neg=neg;
        return(1);
        }

    /* We are actually doing a - b :-) */

    max=(a->top > b->top)?a->top:b->top;
    if (bn_expand(r,(INTEGER)(max*BN_BITS2)) == NULL) return(0);
    if (BN_ucmp(a,b) < 0)
        {
        bn_qsub(r,b,a);
        r->neg=1;
        }
    else
        {
        bn_qsub(r,a,b);
        r->neg=0;
        }
    return(1);
}


BN_ULONG bn_mul_add_word(BN_ULONG *rp, BN_ULONG *ap, short num, BN_ULONG w) {
    BN_ULONG c1=0;

    for (;;)
        {
        mul_add(rp[0],ap[0],w,c1);
        if (--num == 0) break;
        mul_add(rp[1],ap[1],w,c1);
        if (--num == 0) break;
        mul_add(rp[2],ap[2],w,c1);
        if (--num == 0) break;
        mul_add(rp[3],ap[3],w,c1);
        if (--num == 0) break;
        ap+=4;
        rp+=4;
        }
    
    return(c1);
    } 

BN_ULONG bn_mul_word(BN_ULONG *rp, BN_ULONG *ap, short num, BN_ULONG w)
{
    BN_ULONG c1=0;

    for (;;)
        {
        mul(rp[0],ap[0],w,c1);
        if (--num == 0) break;
        mul(rp[1],ap[1],w,c1);
        if (--num == 0) break;
        mul(rp[2],ap[2],w,c1);
        if (--num == 0) break;
        mul(rp[3],ap[3],w,c1);
        if (--num == 0) break;
        ap+=4;
        rp+=4;
        }
    return(c1);
} 

void bn_sqr_words(BN_ULONG *r, BN_ULONG *a, short n)
{
    for (;;)
        {
        BN_ULLONG t;

        t=(BN_ULLONG)(a[0])*(a[0]);
        r[0]=Lw(t); r[1]=Hw(t);
        if (--n == 0) break;

        t=(BN_ULLONG)(a[1])*(a[1]);
        r[2]=Lw(t); r[3]=Hw(t);
        if (--n == 0) break;

        t=(BN_ULLONG)(a[2])*(a[2]);
        r[4]=Lw(t); r[5]=Hw(t);
        if (--n == 0) break;

        t=(BN_ULLONG)(a[3])*(a[3]);
        r[6]=Lw(t); r[7]=Hw(t);
        if (--n == 0) break;

        a+=4;
        r+=8;
        }
}


/* Returns true for success, false for error. */
INTEGER BN_mod_exp_mont(r,a,p,m,ctx)
BIGNUM *r;
BIGNUM *a;
BIGNUM *p;
BIGNUM *m;
BN_CTX *ctx;
{
        INTEGER i,j,bits,ret=0,wstart,wend,window,wvalue;
        INTEGER start=1;
        BIGNUM *d, *t, *aa;
        BIGNUM *val[16];
        BN_MONT_CTX *mont=NULL;

        if (!(m->d[0] & 1))
                {
                /*BNerr(BN_F_BN_MOD_EXP_MONT,BN_R_CALLED_WITH_EVEN_MODULUS);*/
                return(0);
                }
        
        d=ctx->bn[ctx->tos++];
        t=ctx->bn[ctx->tos++];
        bits=BN_num_bits(p);
        if (bits == 0)
                {
                BN_one(r);
                return(1);
                }

        /* If this is not done, things will break in the montgomery
         * part */

        if ((mont=BN_MONT_CTX_new(ctx->bn[0]->byteSize)) == NULL)
               {
               goto err;
               }

        if (!BN_MONT_CTX_set(mont,m,ctx)) goto err;

        if (BN_ucmp(a,m) >= 0)
                {
                BN_mod(t,a,m,ctx);
                aa=t;
                }
        else    aa=a;

        if (bits <= 17) {/* Probably 3 or 0x10001, so just do singles */
                if (!BN_to_montgomery(d,aa,mont,ctx)) goto err; 
                if (!BN_mod_mul_montgomery(r,d,d,mont,ctx)) goto err;
        wstart =  1<< (bits = bits-2);
                for (i=bits; i>0; i--)
                {
                  if (p->d[0] & wstart) {
                     if (!BN_mod_mul_montgomery(r,r,d,mont,ctx)) goto err;
                  }
                  if (!BN_mod_mul_montgomery(r,r,r,mont,ctx)) goto err;
          wstart >>= 1;
                }
        if (p->d[0] & wstart) {
            if (!BN_mod_mul_montgomery(r,r,aa,mont,ctx)) goto err;
        } else {
            if (!BN_from_montgomery(r,r,mont,ctx)) goto err;
        }
        if (mont != NULL) BN_MONT_CTX_free(mont);
        ctx->tos-=2;
                return(ret=1);
    }
        else if (bits >= 256)
                window=5;       /* max size of window */
        else if (bits >= 128)
                window=4;
        else
                window=3;

        val[0]=BN_new(ctx->bn[0]->byteSize);
        if (val[0] == NULL) goto err;
        if (!BN_to_montgomery(val[0],aa,mont,ctx)) goto err; /* 1 */
        if (!BN_mod_mul_montgomery(d,val[0],val[0],mont,ctx)) goto err; 
        j=1<<(window-1);
        for (i=1; i<j; i++)
                {
                val[i]=BN_new(ctx->bn[0]->byteSize);
                if (val[i] == NULL) goto err;
                if (!BN_mod_mul_montgomery(val[i],val[i-1],d,mont,ctx))
                        goto err;
                }
        for (; i<16; i++)
                val[i]=NULL;

        start=1;        /* This is used to avoid multiplication etc
                         * when there is only the value '1' in the
                         * buffer. */
        wvalue=0;       /* The 'value' of the window */
        wstart=bits-1;  /* The top bit of the window */
        wend=0;         /* The bottom bit of the window */

        if (!BN_to_montgomery(r,BN_value_one(),mont,ctx)) goto err;
        for (;;)
                {
                if (BN_is_bit_set(p,wstart) == 0)
                        {
                        if (!start)
                                if (!BN_mod_mul_montgomery(r,r,r,mont,ctx))
                                goto err;
                        if (wstart == 0) break;
                        wstart--;
                        continue;
                        }
                /* We now have wstart on a 'set' bit, we now need to work out
                 * how bit a window to do.  To do this we need to scan
                 * forward until the last set bit before the end of the
                 * window */
                j=wstart;
                wvalue=1;
                wend=0;
                for (i=1; i<window; i++)
                        {
                        if (wstart-i < 0) break;
                        if (BN_is_bit_set(p,(INTEGER)(wstart-i)))
                                {
                                wvalue<<=(i-wend);
                                wvalue|=1;
                                wend=i;
                                }
                        }

                /* wend is the size of the current window */
                j=wend+1;
                /* add the 'bytes above' */
                if (!start)
                        for (i=0; i<j; i++)
                                {
                                if (!BN_mod_mul_montgomery(r,r,r,mont,ctx))
                                        goto err;
                                }

                /* wvalue will be an odd number < 2^window */
                if (!BN_mod_mul_montgomery(r,r,val[wvalue>>1],mont,ctx))
                        goto err;

                /* move the 'window' down further */
                wstart-=wend+1;
                wvalue=0;
                start=0;
                if (wstart < 0) break;
                }
        BN_from_montgomery(r,r,mont,ctx);
        ret=1;
err:
        if (mont != NULL) BN_MONT_CTX_free(mont);
        ctx->tos-=2;
        for (i=0; i<16; i++)
                if (val[i] != NULL) BN_clear_free(val[i]);
        return(ret);
}

INTEGER BN_MONT_CTX_set(mont,mod,ctx)
BN_MONT_CTX *mont;
BIGNUM *mod;
BN_CTX *ctx;
{
        BIGNUM *Ri=NULL,*R=NULL;

        if (mont->RR == NULL) mont->RR=BN_new(ctx->bn[0]->byteSize);
        if (mont->RR == NULL) goto err;
        if (mont->N == NULL)  mont->N=BN_new(ctx->bn[0]->byteSize);
        if (mont->RR == NULL) goto err;

        R=mont->RR;                                     /* grab RR as a temp */
        BN_copy(mont->N,mod);                           /* Set N */

#ifdef MONT_WORD
{
        BIGNUM tmod;
        BN_ULONG buf[2];
        /* INTEGER z; */

        mont->ri=(BN_num_bits(mod)+(BN_BITS2-1))/BN_BITS2*BN_BITS2;
        BN_lshift(R,BN_value_one(),BN_BITS2);           /* R */
        /* I was bad, this modification of a passed variable was
         * breaking the multithreaded stuff :-(
         * z=mod->top;
         * mod->top=1; */

        buf[0]=mod->d[0];
        buf[1]=0;
        tmod.d=buf;
        tmod.top=1;
        tmod.max=mod->max;
        tmod.neg=mod->neg;

        if ((Ri=BN_mod_inverse(R,&tmod,ctx)) == NULL) goto err; /* Ri */
        BN_lshift(Ri,Ri,BN_BITS2);                      /* R*Ri */
        bn_qsub(Ri,Ri,BN_value_one());                  /* R*Ri - 1 */
        BN_div(Ri,NULL,Ri,&tmod,ctx);
        mont->n0=Ri->d[0];
        BN_free(Ri);
        /* mod->top=z; */
}
#else
        mont->ri=BN_num_bits(mod);
        BN_lshift(R,BN_value_one(),mont->ri);                   /* R */
        if ((Ri=BN_mod_inverse(R,mod,ctx)) == NULL) goto err;   /* Ri */
        BN_lshift(Ri,Ri,mont->ri);                              /* R*Ri */
        bn_qsub(Ri,Ri,BN_value_one());                          /* R*Ri - 1 */
        BN_div(Ri,NULL,Ri,mod,ctx);
        if (mont->Ni != NULL) BN_free(mont->Ni);
        mont->Ni=Ri;                                    /* Ni=(R*Ri-1)/N */
#endif

        /* setup RR for conversions */
        BN_lshift(mont->RR,BN_value_one(),(INTEGER)(mont->ri*2));
        BN_mod(mont->RR,mont->RR,mont->N,ctx);

        return(1);
err:
        return(0);
}


BIGNUM *BN_value_one()
{
        static BN_ULONG data_one=1L;
        static BIGNUM const_one={&data_one,1,1,0,sizeof (BN_ULONG)};

        return(&const_one);
}

INTEGER BN_mask_bits(a,n)
BIGNUM *a;
INTEGER n;
{
        INTEGER b,w;

        w=n/BN_BITS2;
        b=n%BN_BITS2;
        if (w >= a->top) return(0);
        if (b == 0)
                a->top=w;
        else
                {
                a->top=w+1;
                a->d[w]&= ~(BN_MASK2<<b);
                while ((w >= 0) && (a->d[w] == 0))
                        {
                        a->top--;
                        w--;
                        }
                }
        return(1);
}



BIGNUM *BN_mod_inverse(a, n, ctx)
BIGNUM *a;
BIGNUM *n;
BN_CTX *ctx;
{
        BIGNUM *A,*B,*X,*Y,*M,*D,*R;
        BIGNUM *ret=NULL,*T;
        INTEGER sign;

        A=ctx->bn[ctx->tos];
        B=ctx->bn[ctx->tos+1];
        X=ctx->bn[ctx->tos+2];
        D=ctx->bn[ctx->tos+3];
        M=ctx->bn[ctx->tos+4];
        Y=ctx->bn[ctx->tos+5];
        ctx->tos+=6;
        R=BN_new(ctx->bn[0]->byteSize);
        if (R == NULL) goto err;

        BN_zero(X);
        BN_one(Y);
        if (BN_copy(A,a) == NULL) goto err;
        if (BN_copy(B,n) == NULL) goto err;
        sign=1;

        while (!BN_is_zero(B))
                {
                if (!BN_div(D,M,A,B,ctx)) goto err;
                T=A;
                A=B;
                B=M;
                /* T has a struct, M does not */

                if (!BN_mul(T,D,X)) goto err;
                if (!BN_add(T,T,Y)) goto err;
                M=Y;
                Y=X;
                X=T;
                sign= -sign;
                }
        if (sign < 0)
                {
                if (!BN_sub(Y,n,Y)) goto err;
                }

        if (BN_is_one(A))
                { if (!BN_mod(R,Y,n,ctx)) goto err; }
        else
                {
                /*BNerr(BN_F_BN_MOD_INVERSE,BN_R_NO_INVERSE);*/
                goto err;
                }
        ret=R;
err:
        if ((ret == NULL) && (R != NULL)) BN_free(R);
        ctx->tos-=6;
        return(ret);
}


INTEGER BN_mod_mul_montgomery(r,a,b,mont,ctx)
BIGNUM *r,*a,*b;
BN_MONT_CTX *mont;
BN_CTX *ctx;
{
        BIGNUM *tmp;

        tmp=ctx->bn[ctx->tos++];

        if (a == b)
                {
                if (!BN_sqr(tmp,a,ctx)) goto err;
                }
        else
                {
                if (!BN_mul(tmp,a,b)) goto err;
                }
        /* reduce from aRR to aR */
        if (!BN_from_montgomery(r,tmp,mont,ctx)) goto err;
        ctx->tos--;
        return(1);
err:
        return(0);
}


INTEGER BN_from_montgomery(ret,a,mont,ctx)
BIGNUM *ret;
BIGNUM *a;
BN_MONT_CTX *mont;
BN_CTX *ctx;
{
        BIGNUM *n,*t1,*r;
        BN_ULONG *ap,*np,*rp,k,n0,v,v2;
        INTEGER al,nl,max,i,x;
        INTEGER retn=0;

        t1=ctx->bn[ctx->tos];
        r=ctx->bn[ctx->tos+1];

        if (!BN_copy(r,a)) goto err;
        n=mont->N;

        if (!BN_copy(t1,a)) goto err;
        BN_mask_bits(t1,mont->ri);

        a=t1;

        al=a->top;
        nl=n->top;
        if ((al == 0) || (nl == 0)) { r->top=0; return(1); }

        max=(nl+al+1); /* allow for overflow (no?) XXX */
        if (bn_expand(r,(INTEGER)((max)*BN_BITS2)) == NULL) goto err;

        r->neg=a->neg^n->neg;
        ap=a->d;
        np=n->d;
        rp=r->d;

        /* clear the top bytes of T */
        for (i=r->top; i<max; i++) /* memset? XXX */
                r->d[i]=0;
/*      memset(&(r->d[r->top]),0,(max-r->top)*sizeof(BN_ULONG)); */

        r->top=max;
        n0=mont->n0;

        for (i=0; i<nl; i++)
                {
                /* This is were part words probably goes wrong */
                k=(rp[0]*n0)&BN_MASK2;
                v=bn_mul_add_word(rp,np,nl,k);

                for (x=nl; v; x++)
                        {
                        v2=rp[x];
                        v2+=v;
                        rp[x]=v2;
                        v=((v2&BN_MASK2) < v)?1:0; /* ever true? XXX */
                        }
                rp++;
                }
        while (r->d[r->top-1] == 0)
                r->top--;

        BN_rshift(ret,r,(INTEGER)(mont->ri));

        if (BN_ucmp(ret,mont->N) >= 0)
                {
                bn_qsub(ret,ret,mont->N); /* XXX */
                }
        retn=1;
err:
        return(retn);
}


BN_MONT_CTX *BN_MONT_CTX_new(INTEGER bytes)
{
        BN_MONT_CTX *ret;

        if ((ret=(BN_MONT_CTX *)JAVAME_MALLOC(sizeof(BN_MONT_CTX))) == NULL)
                return(NULL);
        ret->ri=0;
        ret->RR=BN_new(bytes);
        ret->N=BN_new(bytes);
        ret->Ni=NULL;
        if ((ret->RR == NULL) || (ret->N == NULL))
                {
                BN_MONT_CTX_free(ret);
                return(NULL);
                }
        return(ret);
}

void BN_MONT_CTX_free(mont)
BN_MONT_CTX *mont;
{
        if (mont->RR != NULL) BN_free(mont->RR);
        if (mont->N != NULL) BN_free(mont->N);
        if (mont->Ni != NULL) BN_free(mont->Ni);
        JAVAME_FREE(mont);
}
