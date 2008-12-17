/*
 * @(#)ccm_runtime.c	1.74 06/10/10
 *
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
 *
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"

#include "javavm/include/basictypes.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/directmem.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/globals.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/limits.h"
#include "javavm/include/stacks.h"
#include "javavm/include/sync.h"
#include "javavm/include/utils.h"

#include "javavm/include/ccm_runtime.h"
#include "javavm/include/ccee.h"

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/float.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/system.h"
#include "javavm/include/porting/jit/jit.h"


/* NOTE: See comments in ccm_runtime.h for a description of the categorization
         of CCM helper function types. */

/* Purpose: Handles exceptions caused by compiled code by returning
 * directly to the interpreter so it can handle the exception.
 *
 * NOTE: compiled code will never execute while an exception is pending.
 *       We always detect the exception before returning to compiled
 *       code and return to the interpreter instead.
 */
void CVMCCMhandleException(CVMCCExecEnv* ccee)
{
    /*
     * CVMJITgoNative() will blow away our current ccee and the
     * CVMJITgoNative() C frame and return to the CVMJITgoNative() caller
     * which will deal with the pending exception.
     */
    CVMCCMruntimeLazyFixups(CVMcceeGetEE(ccee));
    CVMJITexitNative(ccee);
}

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_IDIV
/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException. */
CVMJavaInt CVMCCMruntimeIDiv(CVMJavaInt value1, CVMJavaInt value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeIDiv);
    return CVMintDiv(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_IREM
/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException. */
CVMJavaInt CVMCCMruntimeIRem(CVMJavaInt value1, CVMJavaInt value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeIRem);
    return CVMintRem(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LMUL
/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLMul(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLMul);
    return CVMlongMul(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LDIV
/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException. */
CVMJavaLong CVMCCMruntimeLDiv(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLDiv);
    return CVMlongDiv(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LREM
/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_SINGLE */
/* Throws: ArithmeticException. */
CVMJavaLong CVMCCMruntimeLRem(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLRem);
    return CVMlongRem(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LNEG
/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLNeg(CVMJavaLong value)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLNeg);
    return CVMlongNeg(value);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LSHL
/* Purpose: Computes value1 << value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLShl(CVMJavaLong value1, CVMJavaInt value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLShl);
    return CVMlongShl(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LSHR
/* Purpose: Computes value1 >> value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLShr(CVMJavaLong value1, CVMJavaInt value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLShr);
    return CVMlongShr(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LUSHR
/* Purpose: Computes value1 >>> value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLUshr(CVMJavaLong value1, CVMJavaInt value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLUshr);
    return CVMlongUshr(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LAND
/* Purpose: Computes value1 & value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLAnd(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLAnd);
    return CVMlongAnd(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LOR
/* Purpose: Computes value1 | value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLOr(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLOr);
    return CVMlongOr(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LXOR
/* Purpose: Computes value1 ^ value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeLXor(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLXor);
    return CVMlongXor(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FADD
/* Purpose: Computes value1 + value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFAdd(CVMUint32 value1, CVMUint32 value2)
{
    CVMJavaVal32 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFAdd);
    v1.raw = value1;
    v2.raw = value2;
    res.f = CVMfloatAdd(v1.f, v2.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FSUB
/* Purpose: Computes value1 - value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFSub(CVMUint32 value1, CVMUint32 value2)
{
    CVMJavaVal32 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFSub);
    v1.raw = value1;
    v2.raw = value2;
    res.f = CVMfloatSub(v1.f, v2.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FMUL
/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFMul(CVMUint32 value1, CVMUint32 value2)
{
    CVMJavaVal32 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFMul);
    v1.raw = value1;
    v2.raw = value2;
    res.f = CVMfloatMul(v1.f, v2.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FDIV
/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFDiv(CVMUint32 value1, CVMUint32 value2)
{
    CVMJavaVal32 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFDiv);
    v1.raw = value1;
    v2.raw = value2;
    res.f = CVMfloatDiv(v1.f, v2.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FREM
/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFRem(CVMUint32 value1, CVMUint32 value2)
{
    CVMJavaVal32 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFRem);
    v1.raw = value1;
    v2.raw = value2;
    res.f = CVMfloatRem(v1.f, v2.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FNEG
/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeFNeg(CVMUint32 value)
{
    CVMJavaVal32 v32, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFNeg);
    v32.raw = value;
    res.f = CVMfloatNeg(v32.f);
    return res.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DADD
/* Purpose: Computes value1 + value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDAdd(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDAdd);
    v1.l = value1;
    v2.l = value2;
    res.d = CVMdoubleAdd(v1.d, v2.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DSUB
/* Purpose: Computes value1 - value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDSub(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDSub);
    v1.l = value1;
    v2.l = value2;
    res.d = CVMdoubleSub(v1.d, v2.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DMUL
/* Purpose: Computes value1 * value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDMul(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDMul);
    v1.l = value1;
    v2.l = value2;
    res.d = CVMdoubleMul(v1.d, v2.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DDIV
/* Purpose: Computes value1 / value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDDiv(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDDiv);
    v1.l = value1;
    v2.l = value2;
    res.d = CVMdoubleDiv(v1.d, v2.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DREM
/* Purpose: Computes value1 % value2. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDRem(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDRem);
    v1.l = value1;
    v2.l = value2;
    res.d = CVMdoubleRem(v1.d, v2.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DNEG
/* Purpose: Computes -value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeDNeg(CVMJavaLong value)
{
    CVMJavaVal64 v, res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDNeg);
    v.l = value;
    res.d = CVMdoubleNeg(v.d);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_I2L
/* Purpose: Converts an int value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeI2L(CVMJavaInt value)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeI2L);
    return CVMint2Long(value);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_I2F
/* Purpose: Converts an int value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeI2F(CVMJavaInt value)
{
    CVMJavaVal32 v32;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeI2F);
    v32.f = CVMint2Float(value);
    return v32.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_I2D
/* Purpose: Converts an int value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeI2D(CVMJavaInt value)
{
    CVMJavaVal64 res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeI2D);
    res.d = CVMint2Double(value);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_L2I
/* Purpose: Converts a long value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeL2I(CVMJavaLong value)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeL2I);
    return CVMlong2Int(value); 
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_L2F
/* Purpose: Converts a long value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeL2F(CVMJavaLong value)
{
    CVMJavaVal32 v32;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeL2F);
    v32.f = CVMlong2Float(value);
    return v32.raw; 
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_L2D
/* Purpose: Converts a long value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeL2D(CVMJavaLong value)
{
    CVMJavaVal64 res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeL2D);
    res.d = CVMlong2Double(value);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_F2I
/* Purpose: Converts a float value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeF2I(CVMUint32 value)
{
    CVMJavaVal32 v;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeF2I);
    v.raw = value;
    return CVMfloat2Int(v.f);

}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_F2L
/* Purpose: Converts a float value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeF2L(CVMUint32 value)
{
    CVMJavaVal32 v32;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeF2L);
    v32.raw = value;
    return CVMfloat2Long(v32.f);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_F2D
/* Purpose: Converts a float value to a double value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeF2D(CVMUint32 value)
{
    CVMJavaVal32 v32;
    CVMJavaVal64 res;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeF2D);
    v32.raw = value;
    res.d = CVMfloat2Double(v32.f);
    return res.l;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_D2I
/* Purpose: Converts a double value to an int value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeD2I(CVMJavaLong value)
{
    CVMJavaVal64 v;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeD2I);
    v.l = value;
    return CVMdouble2Int(v.d);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_D2L
/* Purpose: Converts a double value to a long value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeD2L(CVMJavaLong value)
{
    CVMJavaVal64 v;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeD2L);
    v.l = value;
    return CVMdouble2Long(v.d);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_D2F
/* Purpose: Converts a double value to a float value. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMUint32 CVMCCMruntimeD2F(CVMJavaLong value)
{
    CVMJavaVal32 v32;
    CVMJavaVal64 v64;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeD2F);
    v64.l = value;
    v32.f = CVMdouble2Float(v64.d);
    return v32.raw;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LCMP
/* Purpose: Compares two long values. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeLCmp(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeLCmp);
    return CVMlongCompare(value1, value2);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_FCMP
/* Purpose: Compares value1 with value2. */
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
     nanResult if (value1 == NaN) or (value2 == Nan) */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeFCmp(CVMUint32 value1, CVMUint32 value2,
                             CVMJavaInt nanResult)
{
    CVMJavaVal32 v1, v2;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeFCmp);
    v1.raw = value1;
    v2.raw = value2;
    return CVMfloatCompare(v1.f, v2.f, nanResult);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DCMPL
/* Purpose: Compares value1 with value2 (NaN yields a "less than" result). */
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
            -1 if (value1 == NaN) or (value2 == Nan)
               i.e. NaN yields a "less than" result. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeDCmpl(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDCmpl);
    v1.l = value1;
    v2.l = value2;
    return CVMdoubleCompare(v1.d, v2.d, -1);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_DCMPG
/* Purpose: Compares value1 with value2 (NaN yields a "greater than" result).*/
/* Returns: -1 if value1 < value2,
             0 if value1 == value2, 
             1 if value1 > value2, and
             1 if (value1 == NaN) or (value2 == Nan)
               i.e. NaN yields a "greater than" result. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaInt CVMCCMruntimeDCmpg(CVMJavaLong value1, CVMJavaLong value2)
{
    CVMJavaVal64 v1, v2;
    CVMCCMstatsInc(CVMgetEE(), CVMCCM_STATS_CVMCCMruntimeDCmpg);
    v1.l = value1;
    v2.l = value2;
    return CVMdoubleCompare(v1.d, v2.d, 1);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_THROW_CLASS
/* Purpose: Throws the specified exception. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void CVMCCMruntimeThrowClass(CVMCCExecEnv *ccee, CVMExecEnv *ee,
			     CVMClassBlock *exceptionCb,
			     const char *exceptionString)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeThrowClass);
    CVMsignalError(ee, exceptionCb, exceptionString);
    CVMCCMhandleException(ccee);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_THROW_OBJECT
/* Purpose: Throws the specified exception. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
void CVMCCMruntimeThrowObject(CVMCCExecEnv *ccee, CVMExecEnv *ee,
			      CVMObject *obj)
{
    /* The VM spec says that we should throw a NullPointerException if the
       exception object is NULL.  NOTE: The IR gen does not emit a NULL check
       for this.  The CCM runtime (i.e. this function) is expected to take care
       of it. */
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeThrowObject);
    if (obj == NULL) {
        CVMthrowNullPointerException(ee, NULL);
    } else {
        CVMgcUnsafeThrowLocalException(ee, obj); /* Throw the exception. */
    }
    CVMCCMhandleException(ccee);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_CHECKCAST
/* Purpose: Checks to see if the specified object is can be legally casted as
            an instance of the specified class. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* Throws: ClassCastException, StackOverflowError. */
/* NOTE: The obj is checked for NULL in the helper function.  The JITed
         code need not check for NULL first. */
void CVMCCMruntimeCheckCast(CVMCCExecEnv *ccee,
			    CVMClassBlock *objCb,
                            CVMClassBlock *castCb,
                            CVMClassBlock **cachedCbPtr)
{
    /* NOTE: The content of this function is based on the implementation of
        CVMgcUnsafeIsInstanceOf(), and is inlined and modified to be optimal
        for the needs of JITed code. */

    CVMExecEnv *ee = CVMcceeGetEE(ccee);

#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeCheckCast);
    if (!CVMisAssignable(ee, objCb, castCb)) {
        /* CVMisAssignable() may have thrown StackOverflowError */
        if (!CVMlocalExceptionOccurred(ee)) {
            /* Illegal Cast detected.  Report it: */
            CVMthrowClassCastException(ee, "%C", objCb);
        }
        CVMCCMhandleException(ccee);
    }

    /* NOTE: If the checkcast is successful, then we know that objCb is cast-
       safe with respect to castCb.  We will cache objCb because we may very
       well check another object of the same type against castCb again when we
       go through this code path again.  The cached ClassBlock will allow us
       to quickly determine cast-safety by a simple equivalence test instead
       of having to go through CVMisAssignable() again.
    */
    /* Only write back the objCb for methods above codeCacheDecompileStart.
     * If CVM_AOT is enabled, make sure we don't write into the AOT code. 
     */
    /* Make sure cachedCbPtr is in the codecache */
#ifdef CVM_AOT
    CVMassert(((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheEnd) ||
              ((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheAOTStart &&
	       (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheAOTEnd));
#else
    CVMassert(((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheEnd));
#endif
    if ((CVMUint8*)cachedCbPtr > CVMglobals.jit.codeCacheDecompileStart
#ifdef CVM_AOT
	&& !((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheAOTStart &&
            (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheAOTEnd)
#endif
       )
    {
	*cachedCbPtr = objCb;
    }

#if 0
    /* Do a dumpstack so we know who is failing a lot */
    if (!((CVMUint8*)cachedCbPtr > CVMglobals.jit.codeCacheDecompileStart)) {
	CVMMethodBlock* mb = ee->interpreterStack.currentFrame->mb;
	CVMconsolePrintf("--> %C.%M\n", CVMmbClassBlock(mb), mb);
	CVMdumpStack(&ee->interpreterStack,0,0,0);
    }
#endif
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_CHECK_ARRAY_ASSIGNABLE
/* Purpose: Checks to see if the an element of type 'rhsCb' can be stored
   into an array whose element type is 'elemCb'. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* Throws: ArrayStoreException, StackOverflowError. */
/* NOTE: The obj is checked for NULL inline */
void CVMCCMruntimeCheckArrayAssignable(CVMCCExecEnv *ccee,
				       CVMExecEnv *ee,
				       CVMClassBlock *elemCb,
				       CVMClassBlock *rhsCb)
{
    /* NOTE: The content of this function is based on the implementation of
       aastore in the interpreter loop */
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeCheckArrayAssignable);
    /* We have checked for equality inline. Verify that */
    CVMassert(elemCb != rhsCb);
    if (!CVMisAssignable(ee, rhsCb, elemCb)) {
        /* CVMisAssignable() may have thrown StackOverflowError */
        if (!CVMlocalExceptionOccurred(ee)) {
            /* Illegal array assignment detected.  Report it: */
            CVMthrowArrayStoreException(ee, "%C", rhsCb);
        }
        CVMCCMhandleException(ccee);
    }
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_INSTANCEOF
/* Purpose: Checks to see if the specified object is an instance of the
            specified class or its subclasses. */
/* Type: STATE_FLUSHED THROWS_SINGLE */
/* Throws: StackOverflowError. */
/* NOTE: The obj is checked for NULL in the helper function.  The JITed
         code need not check for NULL first. */
CVMJavaInt CVMCCMruntimeInstanceOf(CVMCCExecEnv *ccee,
				   CVMClassBlock *objCb,
                                   CVMClassBlock *instanceofCb,
                                   CVMClassBlock **cachedCbPtr)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMJavaInt success;

#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeInstanceOf);
    success = CVMisAssignable(ee, objCb, instanceofCb);
    /* CVMgcUnsafeIsInstanceOf() may have thrown StackOverflowError */
    if (CVMlocalExceptionOccurred(ee)) {
        CVMCCMhandleException(ccee);
    }

    /* NOTE: If the instanceof is successful, then we know that objCb is cast-
       safe with respect to instanceofCb.  We will cache objCb because we may
       very well check another object of the same type against castCb again
       when we go through this code path again.  The cached ClassBlock will
       allow us to quickly determine cast-safety by a simple equivalence test
       instead of having to go through CVMisAssignable() again.
    */
    /* Only write back the objCb for methods above codeCacheDecompileStart.
     * If CVM_AOT is enabled, make sure we don't write into the AOT code.
     */
    /* Make sure cachedCbPtr is in the codecache */
#ifdef CVM_AOT
    CVMassert(((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheEnd) ||
              ((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheAOTStart &&
	       (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheAOTEnd));
#else
    CVMassert(((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheEnd));
#endif
    if (success && 
        (CVMUint8*)cachedCbPtr > CVMglobals.jit.codeCacheDecompileStart
#ifdef CVM_AOT
	&& !((CVMUint8*)cachedCbPtr >= CVMglobals.jit.codeCacheAOTStart &&
            (CVMUint8*)cachedCbPtr < CVMglobals.jit.codeCacheAOTEnd)
#endif
       )
    {
	*cachedCbPtr = objCb;
    }
#if 0
    /* Do a dumpstack so we know who is failing a lot */
    if (success &&
	!((CVMUint8*)cachedCbPtr > CVMglobals.jit.codeCacheDecompileStart)
    {
	CVMMethodBlock* mb = ee->interpreterStack.currentFrame->mb;
	CVMconsolePrintf("--> %C.%M\n", CVMmbClassBlock(mb), mb);
	CVMdumpStack(&ee->interpreterStack,0,0,0);
    }
#endif
    return success;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_LOOKUP_INTEFACE_MB
/* Purpose: Looks up the target MethodBlock which implements the specified
            interface method. */
/* Type: STATE_FLUSHED THROWS_SINGLE */
/* Throws: IncompatibleClassChangeError. */
/* NOTE: The caller should check if the object is NULL first. */
const CVMMethodBlock *
CVMCCMruntimeLookupInterfaceMB(CVMCCExecEnv *ccee, CVMClassBlock *ocb,
                               const CVMMethodBlock *imb, CVMInt32 *guessPtr)
{
#if defined(CVM_DEBUG_ASSERTS) || defined(CVM_CCM_COLLECT_STATS)
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
#endif
    const CVMMethodBlock *mb;

    CVMClassBlock *icb;  /* interface cb for method we are calling */
    CVMInterfaces *interfaces;
    CVMUint32      interfaceCount;
    CVMInterfaceTable *itablePtr;

    CVMUint16       methodTableIndex;/* index into ocb's methodTable */
    CVMInt32 guess;

    CVMassert(CVMD_isgcUnsafe(ee));

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeLookupInterfaceMB);
    icb = CVMmbClassBlock(imb);

    interfaces = CVMcbInterfaces(ocb);
    interfaceCount = CVMcbInterfaceCountGivenInterfaces(interfaces);
    itablePtr = CVMcbInterfaceItable(interfaces);

    /*
     * Search the objects interface table, looking for the entry
     * whose interface cb matches the cb of the inteface we
     * are invoking.
     */
    guess = interfaceCount - 1;

    while ((guess >= 0) && 
           (CVMcbInterfacecbGivenItable(itablePtr,guess) != icb)) {
	guess--;
    }

    if (guess >= 0) {
        CVMassert(CVMcbInterfacecbGivenItable(itablePtr, guess) == icb);
        /* Update the guess if it was wrong, the code is not
         * for a romized class, and the code is not transition
         * code, which is also in ROM.
         */
        /* Only write back the guess for methods above 
         * codeCacheDecompileStart. If CVM_AOT is enabled,
         * make sure we don't write into the AOT code.
         */
        /* Make sure guessPtr is in the codecache */
#ifdef CVM_AOT
        CVMassert(((CVMUint8*)guessPtr >= CVMglobals.jit.codeCacheStart &&
                   (CVMUint8*)guessPtr < CVMglobals.jit.codeCacheEnd) ||
                  ((CVMUint8*)guessPtr >= CVMglobals.jit.codeCacheAOTStart &&
	           (CVMUint8*)guessPtr < CVMglobals.jit.codeCacheAOTEnd));
#else
        CVMassert(((CVMUint8*)guessPtr >= CVMglobals.jit.codeCacheStart &&
                   (CVMUint8*)guessPtr < CVMglobals.jit.codeCacheEnd));
#endif
        if (CVMcbInterfacecb(ocb, guess) == icb &&
            (CVMUint8*)guessPtr > CVMglobals.jit.codeCacheDecompileStart
#ifdef CVM_AOT
	    && !((CVMUint8*)guessPtr >= CVMglobals.jit.codeCacheAOTStart &&
                 (CVMUint8*)guessPtr < CVMglobals.jit.codeCacheAOTEnd)
#endif
           )
        {
	    *guessPtr = guess;
        }
	/*
	 * We know CVMcbInterfacecb(ocb, guess) is for the interface that 
	 * we are invoking. Use it to convert the index of the method in 
	 * the interface's methods array to the index of the method 
	 * in the class' methodTable array.
	 */
	methodTableIndex =
	    CVMcbInterfaceMethodTableIndexGivenInterfaces(
		interfaces, guess, CVMmbMethodSlotIndex(imb));
    } else if (icb == CVMsystemClass(java_lang_Object)) {
        /*
         * Per VM spec 5.4.3.4, the resolved interface method may be
         * a method of java/lang/Object.
         */ 
        methodTableIndex = CVMmbMethodTableIndex(imb);
    } else {
        CVMExecEnv *ee = CVMcceeGetEE(ccee);
        CVMthrowIncompatibleClassChangeError(ee,
            "class %C does not implement interface %C", ocb, icb);
        CVMCCMhandleException(ccee);
	methodTableIndex = 0; /* NOTREACHED, make compiler happy */
    }


    /*
     * Fetch the proper mb and it's cb.
     */
    mb = CVMcbMethodTableSlot(ocb, methodTableIndex);

    return mb;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_NEW
/* Purpose: Instantiates an object of the specified class. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_SINGLE */
/* Throws: OutOfMemoryError. */
CVMObject *
CVMCCMruntimeNew(CVMCCExecEnv *ccee, CVMClassBlock *newCb)
{
    CVMObject *obj;
    CVMExecEnv *ee = CVMcceeGetEE(ccee);

    CVMassert(CVMD_isgcUnsafe(ee));
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeNew);

    obj = CVMgcAllocNewInstance(ee, newCb);
    if (obj == NULL) {
        /* Failed to instantiate the object.  So throw an OutOfMemoryError: */
        CVMthrowOutOfMemoryError(ee, "%C", newCb);
        CVMCCMhandleException(ccee);
    }
    return obj;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_NEWARRAY
/* Purpose: Instantiates a primitive array of the specified type and size. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException. */
CVMObject *
CVMCCMruntimeNewArray(CVMCCExecEnv *ccee, CVMJavaInt length,
                      CVMClassBlock *arrCB)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMObject *arrObj;
    CVMBasicType typeCode = CVMarrayBaseType(arrCB); 

    CVMassert(CVMD_isgcUnsafe(ee));
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeNewArray);

    if (length < 0) {
        CVMthrowNegativeArraySizeException(ee, NULL);
        CVMCCMhandleException(ccee);
    }

    CVMassert(arrCB != NULL);

    /* NOTE: CVMgcAllocNewArray may block and initiate GC. */
    arrObj = (CVMObject *)CVMgcAllocNewArray(ee, typeCode, arrCB, length);

    /* Make sure we created the right beast: */
    CVMassert(arrObj == NULL ||
              CVMisArrayClassOfBasicType(CVMobjectGetClass(arrObj),
                                         CVMbasicTypeID[typeCode]));
    if (arrObj == NULL) {
        CVMthrowOutOfMemoryError(ee, "%C", arrCB);
        CVMCCMhandleException(ccee);
    }
    return arrObj;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_ANEWARRAY
/* Purpose: Instantiates an object array of the specified type and size. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException, StackOverflowError. */
CVMObject *
CVMCCMruntimeANewArray(CVMCCExecEnv *ccee, CVMJavaInt length,
		       CVMClassBlock *arrayCb)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMObject *arrObj;

    CVMassert(CVMD_isgcUnsafe(ee));
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeANewArray);

    if (length < 0) {
        CVMthrowNegativeArraySizeException(ee, NULL);
        CVMCCMhandleException(ccee);
    }

    arrObj = (CVMObject *)CVMgcAllocNewArray(ee, CVM_T_CLASS, arrayCb, length);
    if (arrObj == NULL) {
        CVMthrowOutOfMemoryError(ee, "%C", arrayCb);
        CVMCCMhandleException(ccee);
    }
    return arrObj;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_MULTIANEWARRAY
/* Purpose: Instantiates a multi dimensional object array of the specified
            dimensions. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: OutOfMemoryError, NegativeArraySizeException. */
/* 
 * CVMmultiArrayAlloc() is called from java opcode multianewarray
 * and passes the top of stack as parameter dimensions.
 * Because the width of the array dimensions is obtained via 
 * dimensions[i], dimensions has to be of the same type as 
 * the stack elements for proper access.
 */
CVMObject *
CVMCCMruntimeMultiANewArray(CVMCCExecEnv *ccee, CVMJavaInt nDimensions,
                            CVMClassBlock *arrCb, CVMStackVal32 *dimensions)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMObject *arrObj;
    CVMInt32 effectiveNDimensions;
    CVMInt32 dimCount; /* in case we encounter a 0 */
    CVMObjectICell *resultCell;

    CVMassert(CVMD_isgcUnsafe(ee));
#if CVM_DEBUG
    /* We better not be holding a microlock */
    CVMassert(ee->microLock == 0);
#endif

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeMultiANewArray);

    effectiveNDimensions = nDimensions;

    /*
     * Now go through the dimensions. Check for negative dimensions.
     * Also check for whether there is a 0 in the dimensions array
     * which would prevent the allocation from continuing.
     */
    for (dimCount = 0; dimCount < nDimensions; dimCount++) {
        CVMInt32 dim = dimensions[dimCount].j.i;
        if (dim <= 0) {
            /* Remember the first 0 in the dimensions array */
            if ((dim == 0) && (effectiveNDimensions == nDimensions)) {
                effectiveNDimensions = dimCount + 1;
                /* ... and keep checking for -1's despite a 0 */
            } else if (dim < 0) {
                CVMthrowNegativeArraySizeException(ee, NULL);
                CVMCCMhandleException(ccee);
            }
        }
    }

    /*
     * CVMmultiArrayAlloc() will do all the work
     *
     * Be paranoid about GC in this case (too many intermediate
     * results).
     */
    resultCell = CVMmiscICell(ee);
    CVMassert(CVMID_icellIsNull(resultCell));
    CVMD_gcSafeExec(ee, {
	/* 
	 * do not cast the pointer type of topOfStack
	 * because it is required for correct stack access
	 * via topOfStack[index] in CVMmultiArrayAlloc
	 */
        CVMmultiArrayAlloc(ee, effectiveNDimensions, dimensions,
                           arrCb, resultCell);
    });

    if (CVMID_icellIsNull(resultCell)) {
        /*
         * CVMmultiArrayAlloc() returned a null. This only happens
         * if an allocation failed, so throw an OutOfMemoryError
         */
        CVMthrowOutOfMemoryError(ee, NULL);
        CVMCCMhandleException(ccee);
    }

    arrObj = CVMID_icellDirect(ee, resultCell);

    /*
     * Set this to null when we're done. The misc icell is needed elsewhere,
     * and the nullness is used to prevent errors.
     */
    CVMID_icellSetNull(resultCell);

    return arrObj;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RUN_CLASS_INITIALIZER
/* Purpose: Runs the static initializer for the specified class if
            necessary.
   Result:  -Returns true if the class has been intialized.
            -Returns false if class is being intialized by current
	     thread.
	    -Otherwise attempts to intialize the class and will return
	     to the address in the current frame when intialization
	     is complete.
*/
/* Type: STATE_FLUSHED_TOTALLY THROWS_??? */
CVMBool
CVMCCMruntimeRunClassInitializer(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				 CVMClassBlock *cb)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeRunClassInitializer);

    /*
     * If static intializers still need to be run for this class, then
     * leave it up to the interpreter to do this for us first. 
     */
    if (CVMcbInitializationNeeded(cb, ee)) {
        int result;

	CVMCCMruntimeLazyFixups(ee);

        CVMD_gcSafeExec(ee, {
            CVMMethodBlock *mb;
            /* NOTE: &mb cannot be 0.  Otherwise, CVMclassInitNoCRecursion will
               invoke the interpreter loop rather than setup a transition
               frame. */
            result = CVMclassInitNoCRecursion(ee, cb, &mb);
        });

        /* No action take.  Class is already initialized. */
        if (result == 0) {
            /* Nothing to do.  Class is already initialized. */

        /* Transition frame successfully pushed. */
        } else if (result == 1) {
            CVMJITexitNative(ccee);

        /* An error occurred.  Exception was thrown. */
        } else {
            CVMCCMhandleException(ccee);
        }
    }

    /*
     * Return TRUE if <clinit> has been run. The only time this will not
     * be the case is if it is being run by the current thread.
     */
    return !CVMcbCheckInitNeeded(cb, ee);
}
#endif

#if !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_CLASS_BLOCK) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_ARRAY_CLASS_BLOCK) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETFIELD_OFFSET) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTFIELD_OFFSET) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_BLOCK) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_SPECIAL_METHOD_BLOCK) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_TABLE_OFFSET) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_NEW_CLASS_BLOCK_AND_CLINIT)||\
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETSTATIC_FB_AND_CLINIT)||\
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTSTATIC_FB_AND_CLINIT)||\
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_STATIC_MB_AND_CLINIT)


/* Purpose: Resolves the specified constant pool entry. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           NoSuchMethodError, etc. */
static void
CVMCCMruntimeResolveConstantPoolEntry(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				      CVMUint16 cpIndex, CVMConstantPool *cp)
{
    /* NOTE: CVMcpResolveEntry() will check if the constant pool is already
       resolved.  If it is, it won't go through the resolution process again.

       If the constantpool entry isn't resolved yet, CVMcpResolveEntry() will
       resolve it.  If it fails to resolve it, the appropriate exception will
       already have been thrown.
    */
    if (!CVMcpIsResolved(cp, cpIndex)) {
        CVMBool success;

	CVMCCMruntimeLazyFixups(ee);

        CVMD_gcSafeExec(ee, {
            success = CVMcpResolveEntry(ee, cp, cpIndex);
        });
        if (!success) {
            CVMCCMhandleException(ccee);
        }
    }
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_CLASS_BLOCK
/* Purpose: Resolves the specified class block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, etc. */
CVMBool
CVMCCMruntimeResolveClassBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                               CVMUint16 cpIndex,
                               CVMClassBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMClassBlock *cb;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveClassBlock);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    cb = CVMcpGetCb(cp, cpIndex);

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveClassBlock should never called for AOT code.
     * The above assertion covers that.
     */

    *cachedConstant = cb;

    return CVM_TRUE; /* no class initialization needed */
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_ARRAY_CLASS_BLOCK
/* Purpose: Resolves the specified array class block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, etc. */
CVMBool
CVMCCMruntimeResolveArrayClassBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				    CVMUint16 cpIndex,
				    CVMClassBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMClassBlock *elemCb;
    CVMClassBlock *arrCb;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveArrayClassBlock);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    elemCb = CVMcpGetCb(cp, cpIndex);

    /* CVMclassGetArrayOf() may block. CVMgcAllocNewArray() may block. */
    CVMD_gcSafeExec(ee, {
        arrCb = CVMclassGetArrayOf(ee, elemCb);
    });
    if (arrCb == NULL) {
        /* CVMclassGetArrayOf() has already thrown an exception. */
        CVMCCMhandleException(ccee);
    }

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveArrayClassBlock should never called for AOT code.
     * The above assertion covers that.
     */

    *cachedConstant = arrCb;

    return CVM_TRUE; /* no class initialization needed */
}
#endif

#if !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETFIELD_OFFSET) || \
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTFIELD_OFFSET)
/* Purpose: Resolves the specified field block constant pool entry into a
            field offset, checks if the filed has changed into a static field,
            and checks if the field is final (i.e. not writable to) if
            necessary. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
static CVMBool
CVMCCMruntimeResolveFieldOffset(CVMCCExecEnv *ccee, CVMExecEnv *ee,
				CVMUint16 cpIndex, CVMUint32 *cachedConstant,
				CVMBool isWrite)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMUint32 offset;
    CVMFieldBlock *fb;

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    fb = CVMcpGetFb(cp, cpIndex);
    offset = CVMfbOffset(fb) * sizeof(CVMJavaVal32);

#ifndef CVM_TRUSTED_CLASSLOADERS
    {
        /* Make sure that the field is not static: */
        if (!CVMfieldHasNotChangeStaticState(ee, fb, CVM_FALSE)) {
            CVMCCMhandleException(ccee);
        }
        /* Make sure we're allowed to write to the field: */
        if (isWrite &&
	    !CVMfieldIsOKToWriteTo(ee, fb, 
				   CVMeeGetCurrentFrameCb(ee), CVM_TRUE)) {
            CVMCCMhandleException(ccee);
        }
    }
#endif
    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveFieldOffset should never called for AOT code.
     * The above assertion covers that.
     */

    *cachedConstant = offset;

    return CVM_TRUE; /* no class initialization needed */
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETFIELD_OFFSET
/* Purpose: Resolves the specified field block constant pool entry into a
            field offset, and checks if the filed has changed into a static
            field. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolveGetfieldFieldOffset(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                        CVMUint16 cpIndex,
                                        CVMUint32 *cachedConstant)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveGetfieldFieldOffset);
    return CVMCCMruntimeResolveFieldOffset(ccee, ee, cpIndex,
					   cachedConstant, CVM_FALSE);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTFIELD_OFFSET
/* Purpose: Resolves the specified field block constant pool entry into a
            field offset, checks if the filed has changed into a static field,
            and checks if the field is final (i.e. not writable to). */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolvePutfieldFieldOffset(CVMCCExecEnv *ccee, CVMExecEnv *ee, 
                                        CVMUint16 cpIndex,
                                        CVMUint32 *cachedConstant)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolvePutfieldFieldOffset);
    return CVMCCMruntimeResolveFieldOffset(ccee, ee, cpIndex,
					   cachedConstant, CVM_TRUE);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_BLOCK
/* Purpose: Resolves the specified method block constant pool entry. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, NoSuchMethodError, etc. */
CVMBool
CVMCCMruntimeResolveMethodBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                CVMUint16 cpIndex,
                                CVMMethodBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMMethodBlock *mb;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveMethodBlock);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    mb = CVMcpGetMb(cp, cpIndex);

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveMethodBlock should never called for AOT code.
     * The above assertion covers that.
     */

    *cachedConstant = mb;

    return CVM_TRUE; /* no class initialization needed */
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_SPECIAL_METHOD_BLOCK
/* Purpose: Resolves the specified method block constant pool entry for
            invokespecial, and checks if the method has changed into a static
            method. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMBool
CVMCCMruntimeResolveSpecialMethodBlock(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                       CVMUint16 cpIndex,
                                       CVMMethodBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMMethodBlock *mb;
    CVMClassBlock *currentCb;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveSpecialMethodBlock);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    mb = CVMcpGetMb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
    CVMCCMruntimeLazyFixups(ee);

    /* Make sure that the method is not static: */
    if (!CVMmethodHasNotChangeStaticState(ee, mb, CVM_FALSE)) {
        CVMCCMhandleException(ccee);
    }
#endif
    /* NOTE: The following method of getting the currentCb is based on the
       assumption that inlining is not allowed for methods which have
       unresolved nodes i.e. CVMeeGetCurrentFrame(ee)->mb will yield the
       mb of the method doing the invokespecial. */
    currentCb = CVMeeGetCurrentFrameCb(ee);
    if (CVMisSpecialSuperCall(currentCb, mb)) {
        /* NOTE: In the quickener, the following test is done:
           new_mb = CVMlookupSpecialSuperMethod(ee, currClass, methodID);
           if (new_mb == mb)
                quicken to invokenonvirtual.
           else
                quicken to invokesuper and store method index (computed from
                CVMmbMethodTableIndex(new_mb)) in the opcode operand.

           In the interpreter, invokenonvirtual would invoke the mb in the CP
           entry, and invokesuper would use the stored method index to compute
           the mb in such a way that results in the same mb as new_mb above.

           Hence, for the case of the CCM runtime, regardless of whether new_mb
           equals mb or not, we effectively always end up using the value in
           new_mb anyway.  So, we can simplify logic above into just the
	   new_mb lookup.
        */
	CVMMethodTypeID methodID = CVMmbNameAndTypeID(mb);
	/* Find matching declared method in a super class. */
	mb = CVMlookupSpecialSuperMethod(ee, currentCb, methodID);

    }

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveSpecialMethodBlock should never called for AOT code.
     * The above assertion covers that.
     */

    *cachedConstant = mb;

    return CVM_TRUE; /* no class initialization needed */
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_METHOD_TABLE_OFFSET
/* Purpose: Resolves the specified method block constant pool entry into a
            method table offset for invokevirtual, and checks if the method has
            changed into a static method. */
/* Result:  Always returns TRUE since no class initialization is needed. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMMethodBlock *
CVMCCMruntimeResolveMethodTableOffset(CVMCCExecEnv *ccee, CVMExecEnv *ee,
                                      CVMUint16 cpIndex,
                                      CVMUint32 *cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMUint32 offset;
    CVMMethodBlock *mb;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeResolveMethodTableOffset);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    mb = CVMcpGetMb(cp, cpIndex);
#ifndef CVM_TRUSTED_CLASSLOADERS
    {
	CVMCCMruntimeLazyFixups(ee);
        /* Make sure that the method is not static: */
        if (!CVMmethodHasNotChangeStaticState(ee, mb, CVM_FALSE)) {
            CVMCCMhandleException(ccee);
        }
    }
#endif
    /* make sure its in the table! */
    if (CVMmbMethodTableIndex(mb) == CVM_LIMIT_NUM_METHODTABLE_ENTRIES){
	/* return mb pointer directly */
	return mb;
    }
    offset = CVMmbMethodTableIndex(mb) * sizeof(CVMMethodBlock *);

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveMethodTableOffset should never called for AOT code.
     * The above assertion covers that.
     */
 
    *cachedConstant = offset;

    return (CVMMethodBlock*)NULL; /* vtbl entry entered in cache word */
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_NEW_CLASS_BLOCK_AND_CLINIT
/* Purpose: Resolves the specified class block constant pool entry,
            checks if it is Ok to instantiate an instance of this class,
            and runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, InstantiationError, etc. */
CVMBool
CVMCCMruntimeResolveNewClassBlockAndClinit(CVMCCExecEnv *ccee,
                                           CVMExecEnv *ee,
                                           CVMUint16 cpIndex,
                                           CVMClassBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMClassBlock *cb;

    CVMCCMstatsInc(ee,
                   CVMCCM_STATS_CVMCCMruntimeResolveNewClassBlockAndClinit);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry won't return if an 
       exception was thrown.  It will bypass all this and turn control 
       over to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    cb = CVMcpGetCb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
    CVMCCMruntimeLazyFixups(ee);
    /* Make sure it is OK to instantiate the specified class: */
    if (!CVMclassIsOKToInstantiate(ee, cb)) {
        CVMCCMhandleException(ccee);
    }
#endif

    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveNewClassBlockAndClinit should never called for 
     * AOT code. The above assertion covers that.
     */

    *cachedConstant = cb;

    /* Do static initialization if necessary: */
    /* NOTE: CVMCCMruntimeRunClassInitializer() won't return if it needs to
       run the static initializer.  It will bypass all this and turn control
       over to the VM's compiled code manager. */
    return CVMCCMruntimeRunClassInitializer(ccee, ee, cb);
}
#endif

#if !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETSTATIC_FB_AND_CLINIT)||\
    !defined(CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTSTATIC_FB_AND_CLINIT)
/* Purpose: Resolves the specified static field constant pool entry,
            checks if the field has changed into a non-static field,
            checks if the field is final (i.e. not writable to) if necessarily,
            and runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
static CVMBool
CVMCCMruntimeResolveStaticFieldBlockAndClinit(CVMCCExecEnv *ccee,
                                              CVMExecEnv *ee,
                                              CVMUint16 cpIndex,
                                              void **cachedConstant,
                                              CVMBool isWrite)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMClassBlock *cb;
    CVMFieldBlock *fb;
    void *addressOfStaticField;

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    fb = CVMcpGetFb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
    CVMCCMruntimeLazyFixups(ee);
    /* Make sure that the field is static: */
    if (!CVMfieldHasNotChangeStaticState(ee, fb, CVM_TRUE)) {
        CVMCCMhandleException(ccee);
    }
    /* Make sure we aren't trying to store into a final field.  The VM spec
     * says that you can only store into a final field when initializing it.
     * The JDK seems to interpret this as meaning that a class has write access
     * to its final fields, so we do the same here (see how isFinalField gets
     * setup in quicken.c). */
    if (isWrite &&
	!CVMfieldIsOKToWriteTo(ee, fb, CVMeeGetCurrentFrameCb(ee), CVM_TRUE))
    {
	CVMCCMhandleException(ccee);
    }
#endif
    cb = CVMfbClassBlock(fb);

    /* TBD: For LVM, the cachedConstant should be set to the fb instead
       of the address of the static field if the cb is an LVM system class.
       But this needs to be accompanied by changes in codegen.jcs which are
       not as simple.  How can codegen.jcs emit the appropriate code if it
       does not know prior to resolution if this cb is an LVM system class
       or not. */
    addressOfStaticField = &CVMfbStaticField(ee, fb);
    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveStaticFieldBlockAndClinit should never called
     * for AOT code. The above assertion covers that.
     */

    *cachedConstant = addressOfStaticField;

    /* Do static initialization if necessary: */
    /* NOTE: CVMCCMruntimeRunClassInitializer() won't return if it needs to
       run the static initializer.  It will bypass all this and turn control
       over to the VM's compiled code manager. */
    return CVMCCMruntimeRunClassInitializer(ccee, ee, cb);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_GETSTATIC_FB_AND_CLINIT
/* Purpose: Resolves the specified static field constant pool entry,
            checks if the field has changed into a non-static field, and
            runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolveGetstaticFieldBlockAndClinit(CVMCCExecEnv *ccee,
                                                 CVMExecEnv *ee,
                                                 CVMUint16 cpIndex,
                                                 void **cachedConstant)
{
    CVMCCMstatsInc(ee,
        CVMCCM_STATS_CVMCCMruntimeResolveGetstaticFieldBlockAndClinit);
    return CVMCCMruntimeResolveStaticFieldBlockAndClinit(
                ccee, ee, cpIndex, cachedConstant, CVM_FALSE);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_PUTSTATIC_FB_AND_CLINIT
/* Purpose: Resolves the specified static field constant pool entry,
            checks if the field has changed into a non-static field,
            checks if the field is final (i.e. not writable to), and
            runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchFieldError,
           etc. */
CVMBool
CVMCCMruntimeResolvePutstaticFieldBlockAndClinit(CVMCCExecEnv *ccee,
                                                 CVMExecEnv *ee,
                                                 CVMUint16 cpIndex,
                                                 void **cachedConstant)
{
    CVMCCMstatsInc(ee,
        CVMCCM_STATS_CVMCCMruntimeResolvePutstaticFieldBlockAndClinit);
    return CVMCCMruntimeResolveStaticFieldBlockAndClinit(
                ccee, ee, cpIndex, cachedConstant, CVM_TRUE);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_RESOLVE_STATIC_MB_AND_CLINIT
/* Purpose: Resolves the specified static method block constant pool entry,
            and runs the static initializer if necessary. */
/* Result:  Same as CVMCCMruntimeRunClassInitializer. */
/* Type: STATE_FLUSHED_TOTALLY THROWS_MULTIPLE */
/* Throws: IllegalAccessError, IncompatibleClassChangeError, NoSuchMethodError,
           etc. */
CVMBool
CVMCCMruntimeResolveStaticMethodBlockAndClinit(CVMCCExecEnv *ccee,
                                               CVMExecEnv *ee,
                                               CVMUint16 cpIndex,
                                               CVMMethodBlock **cachedConstant)
{
    CVMConstantPool *cp = CVMeeGetCurrentFrameCp(ee);
    CVMMethodBlock *mb;
    CVMClassBlock *cb;

    CVMCCMstatsInc(ee,
        CVMCCM_STATS_CVMCCMruntimeResolveStaticMethodBlockAndClinit);

    /* NOTE: CVMCCMruntimeResolveConstantPoolEntry() won't return if an
       exception was thrown.  It will bypass all this and turn control over
       to the VM's exception handling mechanism. */
    CVMCCMruntimeResolveConstantPoolEntry(ccee, ee, cpIndex, cp);
    mb = CVMcpGetMb(cp, cpIndex);

#ifndef CVM_TRUSTED_CLASSLOADERS
    CVMCCMruntimeLazyFixups(ee);
    /* Make sure that the method is static: */
    if (!CVMmethodHasNotChangeStaticState(ee, mb, CVM_TRUE)) {
        CVMCCMhandleException(ccee);
    }
#endif
    
    CVMassert(cachedConstant != NULL);
    /* Make sure cachedConstant is in the codecache */
    CVMassert(((CVMUint8*)cachedConstant >= CVMglobals.jit.codeCacheStart &&
               (CVMUint8*)cachedConstant < CVMglobals.jit.codeCacheEnd));

    /*
     * AOT only supports ROMized classes, whose cp are always resolved.
     * CVMCCMruntimeResolveStaticMethodBlockAndClinit should never called
     * for AOT code. The above assertion covers that.
     */

    *cachedConstant = mb;

    /* Do static initialization if necessary: */
    /* NOTE: CVMCCMruntimeRunClassInitializer() won't return if it needs to
       run the static initializer.  It will bypass all this and turn control
       over to the VM's compiled code manager. */
    cb = CVMmbClassBlock(mb);
    return CVMCCMruntimeRunClassInitializer(ccee, ee, cb);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_PUTSTATIC_64_VOLATILE
/* Purpose: Performs an atomic 64-bit putstatic. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
void CVMCCMruntimePutstatic64Volatile(CVMJavaLong value, void *staticField)
{
    CVMExecEnv *ee = CVMgetEE();

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimePutstatic64Volatile);
    CVMassert(staticField != NULL);

    CVM_ACCESS_VOLATILE_LOCK(ee);
    CVMmemCopy64((CVMUint32*)staticField, (CVMUint32*)&value);
    CVM_ACCESS_VOLATILE_UNLOCK(ee);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_GETSTATIC_64_VOLATILE
/* Purpose: Performs an atomic 64-bit getstatic. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeGetstatic64Volatile(void *staticField)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMJavaLong result;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeGetstatic64Volatile);
    CVMassert(staticField != NULL);

    CVM_ACCESS_VOLATILE_LOCK(ee);
    CVMmemCopy64((CVMUint32*)&result, (CVMUint32*)staticField);
    CVM_ACCESS_VOLATILE_UNLOCK(ee);
    return result;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_PUTFIELD_64_VOLATILE
/* Purpose: Performs an atomic 64-bit putfield. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
void CVMCCMruntimePutfield64Volatile(CVMJavaLong value,
				     CVMObject *obj,
				     CVMJavaInt fieldByteOffset)
{
    CVMExecEnv *ee = CVMgetEE();
    void *instanceField = ((CVMUint8*)obj) + fieldByteOffset;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimePutfield64Volatile);
    CVMassert(obj != NULL);

    CVM_ACCESS_VOLATILE_LOCK(ee);
    CVMmemCopy64((CVMUint32*)instanceField, (CVMUint32*)&value);
    CVM_ACCESS_VOLATILE_UNLOCK(ee);
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_GETFIELD_64_VOLATILE
/* Purpose: Performs an atomic 64-bit getfield. */
/* Type: STATE_NOT_FLUSHED THROWS_NONE */
CVMJavaLong CVMCCMruntimeGetfield64Volatile(CVMObject *obj,
					    CVMJavaInt fieldByteOffset)
{
    CVMExecEnv *ee = CVMgetEE();
    void *instanceField = ((CVMUint8*)obj) + fieldByteOffset;
    CVMJavaLong result;

    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeGetfield64Volatile);
    CVMassert(obj != NULL);

    CVM_ACCESS_VOLATILE_LOCK(ee);
    CVMmemCopy64((CVMUint32*)&result, (CVMUint32*)instanceField);
    CVM_ACCESS_VOLATILE_UNLOCK(ee);
    return result;
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_MONITOR_ENTER
/* Purpose: Perform monitor enter. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* State is already flushed before we get here. */
void
CVMCCMruntimeMonitorEnter(CVMCCExecEnv *ccee, CVMExecEnv *ee, CVMObject *obj)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeMonitorEnter);
    if (obj != NULL) {
	if (!CVMfastTryLock(ee, obj)) {
	    CVMObjectICell *objICell = CVMsyncICell(ee);
	    CVMID_icellSetDirect(ee, objICell, obj);
	    obj = NULL;
	    CVMCCMruntimeLazyFixups(ee);
	    if (!CVMobjectLock(ee, objICell)) {
		CVMID_icellSetNull(objICell);
		CVMthrowOutOfMemoryError(ee, NULL);
		CVMCCMhandleException(ccee);
	    }
	    CVMID_icellSetNull(objICell);
	}
    } else {
	CVMCCMruntimeLazyFixups(ee);
        CVMthrowNullPointerException(ee, NULL);
        CVMCCMhandleException(ccee);
    }
}
#endif

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_MONITOR_EXIT
/* Purpose: Perform monitor exit. */
/* Type: STATE_FLUSHED THROWS_MULTIPLE */
/* State is already flushed before we get here. */
void
CVMCCMruntimeMonitorExit(CVMCCExecEnv *ccee, CVMExecEnv *ee, CVMObject *obj)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeMonitorExit);
    if (obj != NULL) {
	if (!CVMfastTryUnlock(ee, obj)) {
	    CVMObjectICell *objICell = CVMsyncICell(ee);
	    CVMID_icellSetDirect(ee, objICell, obj);
	    obj = NULL;
	    CVMCCMruntimeLazyFixups(ee);
	    if (!CVMobjectUnlock(ee, objICell)) {
		CVMID_icellSetNull(objICell);
		CVMthrowIllegalMonitorStateException(
                    ee, "current thread not owner");
		CVMCCMhandleException(ccee);
	    }
	    CVMID_icellSetNull(objICell);
	}
    } else {
	CVMCCMruntimeLazyFixups(ee);
        CVMthrowNullPointerException(ee, NULL);
        CVMCCMhandleException(ccee);
    }
}
#endif

#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
/* 
 * Simple Sync helper function for unlocking in Simple Sync methods
 * when there is contention on the lock. It is only needed for
 * CVM_FASTLOCK_ATOMICOPS since CVM_FASTLOCK_MICROLOCK will never
 * allow for contention on the unlock in Simple Sync methods.
 */
extern void
CVMCCMruntimeSimpleSyncUnlock(CVMExecEnv *ee, CVMObject* obj)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeSimpleSyncUnlock);
    CVMCCMruntimeLazyFixups(ee);
    CVMsimpleSyncUnlock(ee, obj);
}
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

#ifndef CVMCCM_HAVE_PLATFORM_SPECIFIC_GC_RENDEZVOUS
/* Purpose: Rendezvous with GC thread. */
/* Type: STATE_FLUSHED THROWS_NONE */
void CVMCCMruntimeGCRendezvous(CVMCCExecEnv *ccee, CVMExecEnv *ee)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeGCRendezvous);
    CVMCCMruntimeLazyFixups(ee);
    CVMD_gcRendezvous(ee, CVM_TRUE);
}
#endif

#ifdef CVM_CCM_COLLECT_STATS

/* Purpose: Allocates a new instance of the specified class and increments the
            stat for CVMCCM_STATS_CVMCCMruntimeNew as well. */
CVMObject *
CVMgcAllocNewInstanceSpecial(CVMExecEnv *ee, CVMClassBlock *cb)
{
    CVMCCMstatsInc(ee, CVMCCM_STATS_CVMCCMruntimeNew);
    CVMCCMruntimeLazyFixups(ee);
    return CVMgcAllocNewInstance(ee, cb);
}

/* Purpose: Increments the specified CCM stat. */
void
CVMCCMstatsInc(CVMExecEnv *ee, CVMCCMStatsTag tag)
{
    if (CVMglobals.ccmStats.okToCollectStats) {
        CVMassert(tag < CVMCCM_STATS_NUM_TAGS);
        CVMsysMicroLock(ee, CVM_CCM_STATS_MICROLOCK);
        CVMglobals.ccmStats.stats[tag] ++;
        CVMsysMicroUnlock(ee, CVM_CCM_STATS_MICROLOCK);
    }
}

static const char *const tagNames[] = {

    /* CCM Runtime Stats: */
    "Calls to CVMCCMruntimeIDiv       ",
    "Calls to CVMCCMruntimeIRem       ",
    "Calls to CVMCCMruntimeLMul       ",
    "Calls to CVMCCMruntimeLDiv       ",
    "Calls to CVMCCMruntimeLRem       ",
    "Calls to CVMCCMruntimeLNeg       ",
    "Calls to CVMCCMruntimeLShl       ",
    "Calls to CVMCCMruntimeLShr       ",
    "Calls to CVMCCMruntimeLUshr      ",
    "Calls to CVMCCMruntimeLAnd       ",
    "Calls to CVMCCMruntimeLOr        ",
    "Calls to CVMCCMruntimeLXor       ",
    "Calls to CVMCCMruntimeFAdd       ",
    "Calls to CVMCCMruntimeFSub       ",
    "Calls to CVMCCMruntimeFMul       ",
    "Calls to CVMCCMruntimeFDiv       ",
    "Calls to CVMCCMruntimeFRem       ",
    "Calls to CVMCCMruntimeFNeg       ",
    "Calls to CVMCCMruntimeDAdd       ",
    "Calls to CVMCCMruntimeDSub       ",
    "Calls to CVMCCMruntimeDMul       ",
    "Calls to CVMCCMruntimeDDiv       ",
    "Calls to CVMCCMruntimeDRem       ",
    "Calls to CVMCCMruntimeDNeg       ",
    "Calls to CVMCCMruntimeI2L        ",
    "Calls to CVMCCMruntimeI2F        ",
    "Calls to CVMCCMruntimeI2D        ",
    "Calls to CVMCCMruntimeL2I        ",
    "Calls to CVMCCMruntimeL2F        ",
    "Calls to CVMCCMruntimeL2D        ",
    "Calls to CVMCCMruntimeF2I        ",
    "Calls to CVMCCMruntimeF2L        ",
    "Calls to CVMCCMruntimeF2D        ",
    "Calls to CVMCCMruntimeD2I        ",
    "Calls to CVMCCMruntimeD2L        ",
    "Calls to CVMCCMruntimeD2F        ",
    "Calls to CVMCCMruntimeLCmp       ",
    "Calls to CVMCCMruntimeFCmp       ",
    "Calls to CVMCCMruntimeDCmpl      ",
    "Calls to CVMCCMruntimeDCmpg      ",
    "Calls to CVMCCMruntimeThrowClass ",
    "Calls to CVMCCMruntimeThrowObject",
    "Calls to CVMCCMruntimeCheckCast  ",
    "Calls to ...CheckArrayAssignable ",
    "Calls to CVMCCMruntimeInstanceOf ",
    "Calls to ...LookupInterfaceMB    ",
    "Calls to CVMCCMruntimeNew        ",
    "Calls to CVMCCMruntimeNewArray   ",
    "Calls to CVMCCMruntimeANewArray  ",
    "Calls to ...MultiANewArray       ",
    "Calls to ...RunClassInitializer  ",
    "Calls to ...ResolveClassBlock    ",
    "Calls to ....GetfieldFieldOffset ",
    "Calls to ....PutfieldFieldOffset ",
    "Calls to ....MethodBlock         ",
    "Calls to ....SpecialMethodBlock  ",
    "Calls to ....InterfaceMethodBlock",
    "Calls to ....MethodTableOffset   ",
    "Calls ....NewClassBlockAndClinit ",
    "....GetstaticFieldBlockAndClinit ",
    "....PutstaticFieldBlockAndClinit ",
    "....StaticMethodBlockAndClinit   ",
    "Calls to ...MonitorEnter         ",
    "Calls to CVMCCMruntimeMonitorExit",
    "Calls to ...GCRendezvous         ",
#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
    "Calls to ...SimpleSyncUnlock     ",
#endif
};

/* Purpose: Records the top N categories based on their values. */
static void
CVMCCMstatsInsertValueInTopN(CVMUint32 *categories,
                             CVMUint32 numberOfCategories,
                             CVMUint32 category,
                             CVMInt32 value)
{
    CVMCCMGlobalStats *gstats = &CVMglobals.ccmStats;
    CVMUint32 i;
    for (i = 0; i < numberOfCategories; i++) {
        /* If the current entry is uninitialized, then occupy the current
           entry: */
        if (categories[i] == CVMCCM_STATS_NUM_TAGS) {
            /* Occupy the current entry: */
            categories[i] = category;
            return;

        /* If the current entry is less than the specified value, then
           shift the entries down and occupy the current entry: */
        } else if (gstats->stats[categories[i]] < value) {
            CVMUint32 j;

            /* Shift the entries down: */
            for (j = numberOfCategories - 1; j > i; j--) {
                categories[j] = categories[j-1];
            }

            /* Occupy the current entry: */
            categories[i] = category;
            return;
        }
    }
}

/* Purpose: Dumps the collected CCM stats. */
void
CVMCCMstatsDumpStats()
{
    CVMCCMGlobalStats *gstats = &CVMglobals.ccmStats;
    CVMUint32 i;
    #define TOP_N 10
    CVMUint32 topNCategories[TOP_N];
    CVMInt32 total = 0;

    /* Initialize the top N categories: */
    for (i = 0; i < TOP_N; i++) {
        topNCategories[i] = CVMCCM_STATS_NUM_TAGS; /* Illegal category. */
    }

    /* Print the title: */
    CVMconsolePrintf("CUMMULATIVE CCM STATISTICS "
                     "(based on Dynamic Runtime Data)\n");

    CVMconsolePrintf("  Total calls made to CCMRuntime helpers:\n");
    for (i = 0; i < CVMCCM_STATS_NUM_TAGS; i++) {
        CVMInt32 value = gstats->stats[i];
        CVMconsolePrintf("    %s  %d\n", tagNames[i], value);
        total += value;
        CVMCCMstatsInsertValueInTopN(topNCategories, TOP_N, i, value);
    }

    CVMconsolePrintf("\n    %-33s  %d\n", "Total", total);

    CVMconsolePrintf("  Top %d most popular CCMRuntime helpers:\n", TOP_N);
    for (i = 0; i < TOP_N; i++) {
        CVMUint32 cat = topNCategories[i];
        if (cat < CVMCCM_STATS_NUM_TAGS) { /* cat is valid */
            CVMconsolePrintf("    %3d: %s  %d\n", i, tagNames[cat],
                             gstats->stats[cat]);
        } else {
            CVMconsolePrintf("    %3d: <none>\n", i);
        }
    }
    CVMconsolePrintf("\n");
}

#endif /* CVM_CCM_COLLECT_STATS */
