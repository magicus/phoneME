/*
 * @(#)CharToByteISO8859_1.c	1.11 06/10/10
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

#include "generated/offsets/sun_io_CharToByteConverter.h"
#include "generated/offsets/sun_io_CharToByteISO8859_1.h"

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#undef FIELD_WRITE_BADINPUTLENGTH
#define FIELD_WRITE_BADINPUTLENGTH(value)                     \
    CVMD_fieldWriteInt(                                       \
        converterObject,                                      \
        CVMoffsetOfsun_io_CharToByteConverter_badInputLength, \
        value)

#undef FIELD_WRITE_CHAROFF
#define FIELD_WRITE_CHAROFF(value)                     \
    CVMD_fieldWriteInt(                                \
        converterObject,                               \
        CVMoffsetOfsun_io_CharToByteConverter_charOff, \
        value)

#undef FIELD_WRITE_BYTEOFF
#define FIELD_WRITE_BYTEOFF(value)                     \
    CVMD_fieldWriteInt(                                \
        converterObject,                               \
        CVMoffsetOfsun_io_CharToByteConverter_byteOff, \
        value)

#undef FIELD_WRITE_HIGHHALFZONECODE
#define FIELD_WRITE_HIGHHALFZONECODE(value)                     \
    CVMD_fieldWriteInt(                                         \
        converterObject,                                        \
        CVMoffsetOfsun_io_CharToByteISO8859_1_highHalfZoneCode, \
        value);

CNIEXPORT CNIResultCode
CNIsun_io_CharToByteISO8859_11_convert(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                       CVMMethodBlock **p_mb) 
{
    CVMObjectICell *conObjICell = &arguments[0].j.r;
    CVMObjectICell *input = &arguments[1].j.r;
    CVMJavaInt inOff = arguments[2].j.i;
    CVMJavaInt inEnd = arguments[3].j.i;
    CVMObjectICell *output = &arguments[4].j.r;
    CVMJavaInt outOff = arguments[5].j.i;
    CVMJavaInt outEnd = arguments[6].j.i;

    CVMObject *converterObject = CVMID_icellDirect(ee, conObjICell);
    CVMArrayOfChar *inArr = (CVMArrayOfChar *)CVMID_icellDirect(ee, input);
    CVMArrayOfByte *outArr = (CVMArrayOfByte *)CVMID_icellDirect(ee, output);
    CVMJavaChar inElem;

    CVMBool subMode;
    CVMJavaChar highHalfZoneCode;
    CVMObject *subBytesObject;
    CVMArrayOfByte *subBytes;
    CVMJavaInt subBytesSize;

    CVMJavaInt inBound;
    CVMJavaInt outBound;
    CVMJavaInt inLen = (inArr)->length;
    CVMJavaInt outLen = (outArr)->length;
    CVMJavaInt inLimit = MIN(inEnd, inLen);
    CVMJavaInt outLimit = MIN(outEnd, outLen);
    CVMJavaInt bound;
    CVMJavaInt outStart = outOff;

    /* 
     * We need to check bounds in the following order so that
     * we can throw the right exception that is compatible
     * with the original Java version.
     */
    if (inOff >= inEnd) {
        FIELD_WRITE_CHAROFF(inOff);
        FIELD_WRITE_BYTEOFF(outOff);
        arguments[0].j.i = 0;
        return CNI_SINGLE;
    } else if (inOff < 0 || inOff >= inLen) {
        FIELD_WRITE_CHAROFF(inOff);
        FIELD_WRITE_BYTEOFF(outOff);
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        return CNI_EXCEPTION;
    } else if (outOff >= outEnd) {
        FIELD_WRITE_CHAROFF(inOff);
        FIELD_WRITE_BYTEOFF(outOff);
        CVMthrowConversionBufferFullException(ee, NULL);
        return CNI_EXCEPTION;
    } else if (outOff < 0 || outOff >= outLen) {
        FIELD_WRITE_CHAROFF(inOff);
        FIELD_WRITE_BYTEOFF(outOff);
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        return CNI_EXCEPTION;
    }

    CVMD_fieldReadInt(converterObject,
                      CVMoffsetOfsun_io_CharToByteConverter_subMode,
                      subMode);
    CVMD_fieldReadInt(converterObject,
                      CVMoffsetOfsun_io_CharToByteISO8859_1_highHalfZoneCode,
                      highHalfZoneCode); 
    CVMD_fieldReadRef(converterObject,
                      CVMoffsetOfsun_io_CharToByteConverter_subBytes,
                      subBytesObject);
    subBytes = (CVMArrayOfByte *)subBytesObject;
    subBytesSize = (subBytes)->length;

    /* 
     * Check if previous converted string ends 
     * with highHalfZoneCode. 
     */
    if (highHalfZoneCode != 0) {
        /* Reset highHalfZoneCode */ 
        FIELD_WRITE_HIGHHALFZONECODE(0);

        /* 
         * We already checked input array bounds. 
         */

        CVMD_arrayReadChar(inArr, inOff, inElem);
        if (inElem >= 0xdc00 && inElem <= 0xdfff) {
            /* This is a legal UTF16 sequence. */
            if (subMode) {
                if (outOff + subBytesSize > outLimit) {
                    goto finish;
                }
                /* Copy subBytes into the output array */ 
                CVMD_arrayCopyByte(subBytes, 0, outArr, outOff, subBytesSize); 
                inOff += 1;
                outOff += subBytesSize;
            } else {
                FIELD_WRITE_BADINPUTLENGTH(1);
                FIELD_WRITE_CHAROFF(inOff);
                FIELD_WRITE_BYTEOFF(outOff);
                CVMthrowUnknownCharacterException(ee, NULL);
                return CNI_EXCEPTION;
            }
        } else {
            /* This is illegal UTF16 sqeuence. */
            FIELD_WRITE_BADINPUTLENGTH(0);
            FIELD_WRITE_CHAROFF(inOff);
            FIELD_WRITE_BYTEOFF(outOff);
            CVMthrowMalformedInputException(
                ee, "Previous converted string ends with"
                " <High Half Zone Code> of UTF16, but this string does"
                " not begin with <Low Half Zone>"); 
            return CNI_EXCEPTION;
        }
    }

    /* Calculate bound */
    inBound = inLimit - inOff;
    outBound = outLimit - outOff;
    bound = inOff + MIN(inBound, outBound);

    /* Loop until we hit the bound */
    while (inOff < bound) {
        CVMJavaInt newInBound;
        CVMJavaInt newOutBound;

	/* Get the input character */
	CVMD_arrayReadChar(inArr, inOff, inElem);
	
	/* Is this character mappable? */
	if (inElem <= 0x00FF) {
	    CVMD_arrayWriteByte(outArr, outOff, (CVMJavaByte)inElem);
	    inOff ++;
	    outOff ++;
	}
	/* Is this a high surrogate? */
	else if (inElem >= 0xD800 && inElem <= 0xDBFF) {
	    
	    /* Is this the last character in the input? */
	    if (inOff + 1 == inEnd) {
		FIELD_WRITE_HIGHHALFZONECODE(inElem);
		inOff ++;
		goto finish;
	    }
	    
	    /* 
	     * Is there a low surrogate following? 
	     * Don't forget the array bounds check. 
	     */
	    if (inOff + 1 >= inLen) {
		FIELD_WRITE_CHAROFF(inOff);
		FIELD_WRITE_BYTEOFF(outOff);
		CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
		return CNI_EXCEPTION;
	    }
	    CVMD_arrayReadChar(inArr, inOff+1, inElem);
	    if (inElem >= 0xDC00 && inElem <= 0xDFFF) {
		/* 
		 * We have a valid surrogate pair. Too bad we don't map
		 * surrogates. Is subsititution enable?
		 */
		if (subMode) {
		    
		    /* Check bound */
		    if (outOff + subBytesSize > outLimit) {
			goto finish;
		    }
		    
		    CVMD_arrayCopyByte(subBytes, 0, outArr, 
				       outOff, subBytesSize);
		    inOff += 2;
		    outOff += subBytesSize;
		    
		    /* Recalculate bound */
		    if (subBytesSize != 2) {
			newInBound = inLimit - inOff;
			newOutBound = outLimit - outOff;
			bound = inOff + MIN(newInBound, newOutBound);
		    }
		} else {
		    FIELD_WRITE_BADINPUTLENGTH(2);
		    FIELD_WRITE_CHAROFF(inOff);
		    FIELD_WRITE_BYTEOFF(outOff);
		    CVMthrowUnknownCharacterException(ee, NULL);
		    return CNI_EXCEPTION;
		}
	    } else {
		/* We have a malformed surrogate pair */
		FIELD_WRITE_BADINPUTLENGTH(1);
		FIELD_WRITE_CHAROFF(inOff);
		FIELD_WRITE_BYTEOFF(outOff);
		CVMthrowMalformedInputException(ee, NULL);
		return CNI_EXCEPTION;
	    } 
	}
	/* Is this an unaccompanied low surrogate? */
	else if (inElem >= 0xDC00 && inElem <= 0xDFFF) {
	    FIELD_WRITE_BADINPUTLENGTH(1);
	    FIELD_WRITE_CHAROFF(inOff);
	    FIELD_WRITE_BYTEOFF(outOff);
	    CVMthrowMalformedInputException(ee, NULL);
	    return CNI_EXCEPTION;
	}
	/* Unmappable and not part of a surrogate */
	else {
	    /* Is subsititution enabled? */
	    if (subMode) {
		/* Check bound */
		if (outOff + subBytesSize > outLimit) {
		    goto finish;
		}
		
		CVMD_arrayCopyByte(subBytes, 0, outArr, 
				   outOff, subBytesSize);
		inOff += 1;
		outOff += subBytesSize; 
		
		/* Recalculate bound */
		if (subBytesSize != 1) {
		    newInBound = inLimit - inOff;
		    newOutBound = outLimit - outOff;
		    bound = inOff + MIN(newInBound, newOutBound);
		}
	    } else {
		FIELD_WRITE_BADINPUTLENGTH(1);
		FIELD_WRITE_CHAROFF(inOff);
		FIELD_WRITE_BYTEOFF(outOff);
		CVMthrowUnknownCharacterException(ee, NULL);
		return CNI_EXCEPTION;
	    }
	}
    }
   
 finish: 
    /* Don't forget to write back inOff and outOff. */
    FIELD_WRITE_CHAROFF(inOff);
    FIELD_WRITE_BYTEOFF(outOff);

    /* Check array bounds and throw appropriate exception. */
    if (inOff < inEnd) { /* we didn't copy everything */
        if (inOff >= inLen || outEnd > outLen) {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
            return CNI_EXCEPTION;
        }

        while (inOff < inEnd) {
            CVMD_arrayReadChar(inArr, inOff, inElem);
            if (inElem <= 0x00FF) {
                CVMthrowConversionBufferFullException(ee, NULL);
                return CNI_EXCEPTION;
            } else if (inElem >= 0xD800 && inElem <= 0xDBFF) {
                if (inOff+1 == inEnd) {
                    inOff++;
                    FIELD_WRITE_HIGHHALFZONECODE(inElem);
                } else {
                    CVMD_arrayReadChar(inArr, inOff+1, inElem);
                    if (inElem >= 0xDC00 && inElem <= 0xDFFF) {
                        if (subMode) {
                            if (subBytesSize > 0) {
                                CVMthrowConversionBufferFullException(ee, NULL);
                                return CNI_EXCEPTION;
                            }
                            inOff += 2;
                        } else {
                            FIELD_WRITE_BADINPUTLENGTH(2);
                            CVMthrowUnknownCharacterException(ee, NULL);
                            return CNI_EXCEPTION;
                        }
                    } else {
                        FIELD_WRITE_BADINPUTLENGTH(1);
                        CVMthrowMalformedInputException(ee, NULL);
                        return CNI_EXCEPTION;
                   }
                }
            } else if (inElem >= 0xDC00 && inElem <= 0xDFFF) {
                FIELD_WRITE_BADINPUTLENGTH(1);
                CVMthrowMalformedInputException(ee, NULL);
                return CNI_EXCEPTION;
            } else {
                if (subMode) {
                    if (subBytesSize > 0) {
                        CVMthrowConversionBufferFullException(ee, NULL);
                        return CNI_EXCEPTION;
                    }
                    inOff++;
                } else {
                    FIELD_WRITE_BADINPUTLENGTH(1);
                    CVMthrowUnknownCharacterException(ee, NULL);
                    return CNI_EXCEPTION;
                }
            }
        }

        FIELD_WRITE_CHAROFF(inOff);
    }

    arguments[0].j.i = outOff - outStart;
    return CNI_SINGLE; 
}
