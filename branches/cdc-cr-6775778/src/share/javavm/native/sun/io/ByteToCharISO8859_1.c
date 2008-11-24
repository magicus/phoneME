/*
 * @(#)ByteToCharISO8859_1.c	1.11 06/10/10
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
#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"

#include "generated/offsets/sun_io_ByteToCharConverter.h"

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#undef FIELD_WRITE_CHAROFF
#define FIELD_WRITE_CHAROFF(value)                     \
    CVMD_fieldWriteInt(                                \
        converterObject,                               \
        CVMoffsetOfsun_io_ByteToCharConverter_charOff, \
        value)

#undef FIELD_WRITE_BYTEOFF
#define FIELD_WRITE_BYTEOFF(value)                     \
    CVMD_fieldWriteInt(                                \
        converterObject,                               \
        CVMoffsetOfsun_io_ByteToCharConverter_byteOff, \
        value)

CNIEXPORT CNIResultCode
CNIsun_io_ByteToCharISO8859_11_convert(CVMExecEnv* ee,
				       CVMStackVal32 *arguments,
                                       CVMMethodBlock **p_mb)
{
    CVMObject* converterObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMObjectICell *input = &arguments[1].j.r;
    CVMJavaInt inOff = arguments[2].j.i;
    CVMJavaInt inEnd = arguments[3].j.i;
    CVMObjectICell *output = &arguments[4].j.r;
    CVMJavaInt outOff = arguments[5].j.i;
    CVMJavaInt outEnd = arguments[6].j.i;

    CVMArrayOfByte *inArr = (CVMArrayOfByte *)CVMID_icellDirect(ee, input);
    CVMArrayOfChar *outArr = (CVMArrayOfChar *)CVMID_icellDirect(ee, output);
    CVMJavaByte inElem;
    CVMJavaChar outElem;

    CVMJavaInt inLen = inArr->length;
    CVMJavaInt outLen = outArr->length;
    CVMJavaInt inBound = MIN(inEnd, inLen) - inOff;
    CVMJavaInt outBound = MIN(outEnd, outLen) - outOff;
    CVMJavaInt bound = inOff + MIN(inBound, outBound);
    CVMJavaInt outStart = outOff; /* Record outOff */

    /* 
     * We need to check bounds in the following order so that
     * we can throw the right exception that is compatible
     * with the original Java version.
     */
    if (inOff >= inEnd) {
        FIELD_WRITE_CHAROFF(outOff);
        FIELD_WRITE_BYTEOFF(inOff);
        arguments[0].j.i = 0;
        return CNI_SINGLE;
    } else if (inOff < 0 || inOff >= inLen) {
        int tmp = inOff + (outEnd - outOff); 
        FIELD_WRITE_CHAROFF(outOff);
        FIELD_WRITE_BYTEOFF(inOff);
        if (tmp < inEnd && inOff >= tmp) {
            CVMthrowConversionBufferFullException(ee, NULL);
        } else {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        }
        return CNI_EXCEPTION;
    } else if (outOff >= outEnd) {
        FIELD_WRITE_CHAROFF(outOff);
        FIELD_WRITE_BYTEOFF(inOff);
        CVMthrowConversionBufferFullException(ee, NULL);
        return CNI_EXCEPTION;
    } else if (outOff < 0 || outOff >= outLen) {
        FIELD_WRITE_CHAROFF(outOff);
        FIELD_WRITE_BYTEOFF(inOff);
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        return CNI_EXCEPTION;
    }

    /* Start conversion until we hit the bound */
    while (inOff < bound) {
	CVMD_arrayReadByte(inArr, inOff, inElem);
	outElem = (CVMJavaChar)(0xff & inElem); 
	CVMD_arrayWriteChar(outArr, outOff, outElem);
	inOff++;
	outOff++;
    }                              

    FIELD_WRITE_CHAROFF(outOff);
    FIELD_WRITE_BYTEOFF(inOff);

    /* Check array bounds and throw appropriate exception. */
    if (inOff < inEnd) { /* we didn't copy everything */
        if (inOff >= inLen || outEnd > outLen) {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
            return CNI_EXCEPTION;
        } else {
            CVMthrowConversionBufferFullException(ee, NULL);
            return CNI_EXCEPTION;
        }
    }

    arguments[0].j.i = outOff - outStart;

    return CNI_SINGLE;
}
