/*
 * @(#)ccmintrinsics.c	1.18 06/10/10
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

#include "javavm/include/ccee.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitirnode.h"
#include "fdlibm.h"
#include "javavm/export/jvm.h"
#include "generated/offsets/java_lang_String.h"


#ifdef CVMJIT_INTRINSICS

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#undef FIELD_READ_COUNT
#define FIELD_READ_COUNT(obj, val) \
    CVMD_fieldReadInt(obj,         \
        CVMoffsetOfjava_lang_String_count, val)

#undef FIELD_READ_VALUE
#define FIELD_READ_VALUE(obj, val) \
    CVMD_fieldReadRef(obj,         \
        CVMoffsetOfjava_lang_String_value, val)

#undef FIELD_READ_OFFSET
#define FIELD_READ_OFFSET(obj, val) \
    CVMD_fieldReadInt(obj,          \
        CVMoffsetOfjava_lang_String_offset, val)

#ifndef CVMCCM_DISABLE_SHARED_STRING_CHARAT_INTRINSIC
/* Purpose: Intrinsic version of String.charAt(). */
CVMJavaChar
CVMCCMintrinsic_java_lang_String_charAt(CVMCCExecEnv *ccee,
                                        CVMObject *self, CVMJavaInt index)
{
    CVMJavaInt count;
    CVMJavaInt offset;
    CVMObject *value;
    CVMJavaChar c;
    
    CVMassert(CVMD_isgcUnsafe(CVMcceeGetEE(ccee)));

    FIELD_READ_COUNT(self, count);
    
    /* The following also checks for (index < 0) by casting index into a
       CVMUint32 before comparing against count: */
    if (((CVMUint32)index) >= count) {
        goto errorCase;
    }

    FIELD_READ_VALUE(self, value);
    FIELD_READ_OFFSET(self, offset);
    CVMD_arrayReadChar((CVMArrayOfChar*)value, index+offset, c);
    return c;

errorCase:
    /* NOTE: We put the error case at the bottom to increase cache locality
       for the non-error case above. */
    {
        CVMExecEnv *ee = CVMcceeGetEE(ccee);
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowStringIndexOutOfBoundsException(
            ee, "String index out of range: %d", index);
        CVMCCMhandleException(ccee);
    }
    return 0;
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_COMPARETO_INTRINSIC
/* Purpose: Intrinsic version of String.compareTo(). */
CVMJavaInt
CVMCCMintrinsic_java_lang_String_compareTo(CVMCCExecEnv *ccee,
                                           CVMObject *self, CVMObject *other)
{
    CVMObject *value1, *value2;
    CVMJavaInt n, offset1, offset2;
    CVMJavaInt len1, len2;

    CVMassert(CVMD_isgcUnsafe(CVMcceeGetEE(ccee)));

    /* If other is null, a NullPointerException should be thrown.  The
       caller is responsible for NULL checking self. */
    if (other == NULL) {
        goto errorCase;
    }

    /* Get count, value, and offset */
    FIELD_READ_COUNT(self, len1);
    FIELD_READ_COUNT(other, len2);
    n = MIN(len1, len2);

    FIELD_READ_VALUE(self, value1);
    FIELD_READ_VALUE(other, value2);
    FIELD_READ_OFFSET(self, offset1);
    FIELD_READ_OFFSET(other, offset2);

    /* Now compare */
#ifdef CVMGC_HAS_NO_CHAR_READ_BARRIER
    {
        CVMJavaChar *c1p, *c2p, *last1;
        CVMAddr scratch;

        /* NOTE: CVMDprivate_arrayElemLoc() is used here because we know for
           sure that we won't be becoming GC safe during the string comparison
           below.  This is only done as a speed optimization here, and should
           not be used elsewhere without careful consideration of the
           conequences.
        */
        c1p = (CVMJavaChar *)
	    CVMDprivate_arrayElemLoc((CVMArrayOfChar*)value1, offset1);
        c2p = (CVMJavaChar *)
	    CVMDprivate_arrayElemLoc((CVMArrayOfChar*)value2, offset2);
        last1 = c1p + n;

        scratch = ((CVMAddr)c1p ^ (CVMAddr)c2p) & 0x3;
        if (scratch == 0) {
            CVMUint32 *ilast1;
	    CVMUint32 *i1p, *i2p;

            /* c1p and c2p are both equally aligned.  Do aligned
               comparison: */

            /* First compare the leading char is appropriate: */
            if ((((CVMAddr)c1p & 0x3) != 0) && (c1p != last1)) {
                CVMJavaChar c1 = *c1p++;
                CVMJavaChar c2 = *c2p++;
                if (c1 != c2) {
                    return (CVMJavaInt)c1 - (CVMJavaInt)c2;
                }
            }

            /* Now c1p and c2p should be word aligned: */
            ilast1 = (CVMUint32 *)((CVMAddr)last1 & ~0x3);
	    /* Avoid ++ on a cast pointer by assigning them here */
	    i1p = (CVMUint32*)c1p;
	    i2p = (CVMUint32*)c2p;
            while (i1p < ilast1) {
                if (*i1p++ != *i2p++) {
                    goto intMismatch;
                }
            }

	    /*
	     * set c1p, c2p to current pointers in case we need them
	     * for doing character compares.
	     */
	    c1p = (CVMJavaChar *)i1p;
	    c2p = (CVMJavaChar *)i2p;
            /* Compare the trailing char if appropriate: */
            goto doCharCompare;

        intMismatch:
            /* Back up the int comparison and go let the char comparison take
               care of it: */
            c1p = (CVMJavaChar *)(i1p - 1);
            c2p = (CVMJavaChar *)(i2p - 1);
            goto doCharCompare;

        } else {
        doCharCompare:
            /* c1p and c2p are not equally aligned.  Do unaligned comparison:*/
            while (c1p < last1) {
                CVMJavaChar c1 = *c1p++;
                CVMJavaChar c2 = *c2p++;
                if (c1 != c2) {
                    return (CVMJavaInt)c1 - (CVMJavaInt)c2;
                }
            }
            return len1 - len2;
        }
    }
#else
    {
        CVMJavaInt last = offset1 + n;
        if (offset1 == offset2) {
            /* This is an optimized case */
            while (offset1 < last) {
                CVMJavaChar c1, c2;
                CVMD_arrayReadChar((CVMArrayOfChar*)value1, offset1, c1);
                CVMD_arrayReadChar((CVMArrayOfChar*)value2, offset1, c2);
                if (c1 != c2) {
                    return (CVMJavaInt)c1 - (CVMJavaInt)c2;
                }
                offset1++;
            }
        } else {
            while (offset1 < last) {
                CVMJavaChar c1, c2;
                CVMD_arrayReadChar((CVMArrayOfChar*)value1, offset1, c1);
                CVMD_arrayReadChar((CVMArrayOfChar*)value2, offset2, c2);
                if (c1 != c2) {
                    return (CVMJavaInt)c1 - (CVMJavaInt)c2;
                }
                offset1++;
                offset2++;
            }
        }
    }
    return len1 - len2;
#endif

errorCase:
    /* NOTE: We put the error case at the bottom to increase cache locality
       for the non-error case above. */
    {
        CVMExecEnv *ee = CVMcceeGetEE(ccee);
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowNullPointerException(ee, NULL);
        CVMCCMhandleException(ccee);
    }
    return 0;
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_EQUALS_INTRINSIC
/* Purpose: Intrinsic version of String.equals(). */
CVMJavaBoolean
CVMCCMintrinsic_java_lang_String_equals(CVMObject *self, CVMObject *other)
{
    CVMassert(CVMD_isgcUnsafe(CVMgetEE()));
    if (self == other) {
        /* Same object, return true directly. */
        return CVM_TRUE;
    } else if (other == NULL) {
        /* 
         * If anObject is null, the Java version 
         * always returns False, unless this String
         * is also null, in which case a NullPointerException
         * is already thrown by the interpreter.
         */
        return CVM_FALSE; 
    }

    if (CVMobjectGetClass(other) == CVMsystemClass(java_lang_String)) {
        CVMJavaInt count1, count2;

        FIELD_READ_COUNT(self, count1);
        FIELD_READ_COUNT(other, count2);
        if (count1 == count2) {
            CVMObject *value1, *value2;
            CVMJavaInt offset1, offset2;

            FIELD_READ_VALUE(self, value1);
            FIELD_READ_VALUE(other, value2);
            FIELD_READ_OFFSET(self, offset1); 
            FIELD_READ_OFFSET(other, offset2);

#ifdef CVMGC_HAS_NO_CHAR_READ_BARRIER
            {
                CVMJavaChar *c1p, *c2p, *last1;
                CVMAddr scratch;

                /* NOTE: CVMDprivate_arrayElemLoc() is used here because we
                   know for sure that we won't be becoming GC safe during the
                   string comparison below.  This is only done as a speed
                   optimization here, and should not be used elsewhere without
                   careful consideration of the conequences.
                */
                c1p = (CVMJavaChar *)
		    CVMDprivate_arrayElemLoc((CVMArrayOfChar*)value1, offset1);
                c2p = (CVMJavaChar *)
		    CVMDprivate_arrayElemLoc((CVMArrayOfChar*)value2, offset2);
                last1 = c1p + count1;

                scratch = ((CVMAddr)c1p ^ (CVMAddr)c2p) & 0x3;
                if (scratch == 0) {
                  CVMUint32 *ilast1;
		  CVMUint32 *i1p, *i2p;

                    /* c1 and c2 are both equally aligned.  Do aligned
                       comparison: */

                    /* First compare the leading char is appropriate: */
                    if ((((CVMAddr)c1p & 0x3) != 0) && (c1p != last1)) {
                        if (*c1p++ != *c2p++) {
                            return CVM_FALSE;
                        }
                    }

                    /* Now c1 and c2 should be word aligned: */
		    /* Avoid ++ on a cast pointer by assigning them here */
		    i1p = (CVMUint32*)c1p;
		    i2p = (CVMUint32*)c2p;
                    ilast1 = (CVMUint32 *)((CVMAddr)last1 & ~0x3);
                    while (i1p < ilast1) {
                        if (*i1p++ != *i2p++) {
                            return CVM_FALSE;
                        }
                    }

                    /* Compare the trailing char if appropriate: */
                    if ((CVMJavaChar*)i1p < last1){ 
			c1p = (CVMJavaChar*)i1p;
			c2p = (CVMJavaChar*)i2p;
			if (*c1p != *c2p) {
			    return CVM_FALSE;
			}
                    }
                    return CVM_TRUE;

                } else {
                    /* c1 and c2 are not equally aligned.  Do unaligned
                       comparison: */
                    while (c1p < last1) {
                        if (*c1p++ != *c2p++) {
                            return CVM_FALSE;
                        }
                    }
                    return CVM_TRUE;
                }
            }
#else
            while (count1-- > 0) {
                CVMJavaChar c1, c2;
                CVMD_arrayReadChar((CVMArrayOfChar*)value1, offset1, c1);
                CVMD_arrayReadChar((CVMArrayOfChar*)value2, offset2, c2);
                if (c1 != c2) {
                    return CVM_FALSE;
                }
                offset1++;
                offset2++;
            }
            return CVM_TRUE;
#endif
        } 
    }
    return CVM_FALSE;
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_GETCHARS_INTRINSIC
/* Purpose: Intrinsic version of String.getChars(). */
void
CVMCCMintrinsic_java_lang_String_getChars(CVMCCExecEnv *ccee,
                                          CVMObject *self,
                                          CVMJavaInt srcBegin,
                                          CVMJavaInt srcEnd,
                                          CVMArrayOfChar *dst,
                                          CVMJavaInt dstBegin)
{
    CVMJavaInt count;
    CVMJavaInt offset;
    CVMJavaInt dstLen = 0;
    CVMObject *value;
    CVMClassBlock *dstCb = NULL;

    CVMassert(CVMD_isgcUnsafe(CVMcceeGetEE(ccee)));

    FIELD_READ_COUNT(self, count);
    if (srcEnd > count || srcBegin < 0 || srcBegin > srcEnd) {
        goto errorCase;
    }

    /* Check NULL for dstObject */
    if (dst == NULL) {
        goto errorCase;
    }

    /* Check whether dst is a char array */
    dstCb = CVMobjectGetClass(dst);
    if (!CVMisArrayClass(dstCb) || CVMarrayBaseType(dstCb) != CVM_T_CHAR) {
        goto errorCase;
    }

    /* Array bounds checking for dst*/
    dstLen = CVMD_arrayGetLength(dst);
    if ((((CVMUint32)dstBegin) > dstLen) || 
        (dstBegin + srcEnd - srcBegin > dstLen)) {
        goto errorCase;
    }

    FIELD_READ_VALUE(self, value);
    FIELD_READ_OFFSET(self, offset);
    CVMD_arrayCopyChar((CVMArrayOfChar*)value, offset + srcBegin, 
                       dst, dstBegin, srcEnd - srcBegin); 
    return;

errorCase:
    {
        CVMExecEnv *ee = CVMcceeGetEE(ccee);
        CVMCCMruntimeLazyFixups(ee);
    
        if (srcBegin < 0) {
            CVMthrowStringIndexOutOfBoundsException(
                ee, "String index out of range: %d", srcBegin);
        } else if (srcEnd > count) {
            CVMthrowStringIndexOutOfBoundsException(
                ee, "String index out of range: %d", srcEnd);
        } else if (srcBegin > srcEnd) {
            CVMthrowStringIndexOutOfBoundsException(
                ee, "String index out of range: %d", srcEnd - srcBegin);
        } else if (dst == NULL) {
            CVMthrowNullPointerException(ee, NULL);
        } else if (!CVMisArrayClass(dstCb) ||
                   CVMarrayBaseType(dstCb) != CVM_T_CHAR) {
            CVMthrowArrayStoreException(ee, NULL);
        } else if (dstBegin < 0 || dstBegin > dstLen || 
                   dstBegin + srcEnd - srcBegin > dstLen) {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        }
        CVMCCMhandleException(ccee);
    }
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_II_INTRINSIC
/* Purpose: Intrinsic version of String.indexOf(int ch, int fromIndex). */
static CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_II(CVMObject* thisObj,
					    CVMJavaInt ch,
					    CVMJavaInt fromIndex)
{
    CVMObject *valueObj;
    CVMArrayOfChar *valueArr;
    CVMJavaInt offset;
    CVMJavaInt count;
    CVMJavaInt max;
    CVMJavaInt i;
    CVMJavaChar c;

    FIELD_READ_COUNT(thisObj, count);
 
    if (fromIndex >= count) {
        return -1;
    }

    if (fromIndex < 0) {
	fromIndex = 0;
    }
   
    FIELD_READ_OFFSET(thisObj, offset);
    FIELD_READ_VALUE(thisObj, valueObj);
    valueArr = (CVMArrayOfChar *)valueObj;

    max = offset + count;
    i = offset + fromIndex;
    while (i < max) {
	CVMD_arrayReadChar(valueArr, i, c);
	if (c == ch) {
	    return i - offset;
	}
	i++;
    }
    /* Not found. Return -1. */
    return -1;
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_I_INTRINSIC
/* Purpose: Intrinsic version of String.indexOf(int ch). */
static CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_I(CVMObject* thisObj,
					   CVMJavaInt ch)
{
    return CVMCCMintrinsic_java_lang_String_indexOf_II(thisObj, ch, 0);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_STRING_I_INTRINSIC
/* Purpose: Intrinsic version of String.indexOf(String str, int fromIndex). */
static CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_STRING_I(CVMCCExecEnv *ccee,
						  CVMObject* thisObj,
						  CVMObject *strObj,
						  CVMJavaInt fromIndex)
{
   CVMObject *vObj1;
   CVMObject *vObj2;
   CVMArrayOfChar *v1;
   CVMArrayOfChar *v2;
   CVMJavaInt offset;
   CVMJavaInt strOffset;
   CVMJavaInt count;
   CVMJavaInt strCount;
   CVMJavaInt max;
   CVMJavaChar first;
   CVMJavaChar c1;
   CVMJavaChar c2;
   CVMJavaInt i; /* Iterates v1 searching for the first char of v2 */
   CVMJavaInt j; /* Iterates v1 searching for the rest of v2 */
   CVMJavaInt k; /* Iterates v2 */
   CVMJavaInt end;
   CVMassert(CVMD_isgcUnsafe(CVMcceeGetEE(ccee)));

    /* If strObj is null, a NullPointerException should be thrown. */
    if (strObj == NULL) {
        CVMExecEnv *ee = CVMcceeGetEE(ccee);
        CVMCCMruntimeLazyFixups(ee);
	CVMthrowNullPointerException(ee, NULL);
        CVMCCMhandleException(ccee);
	return -1;
    }

   /* The caller is responsible for NULL checking for strICell. */
   FIELD_READ_COUNT(thisObj, count);
   FIELD_READ_COUNT(strObj, strCount);

   if (fromIndex >= count) {
       if (strCount == 0) {
           /* 
            * Special case: return the length of the first string
            * when searching for an empty String, 4276204 & 6205369
            */
           return count;
       }
       return -1;
   }
   if (fromIndex < 0) {
       fromIndex = 0;
   }
   if (strCount == 0) {
       return fromIndex;
   }

   FIELD_READ_VALUE(thisObj, vObj1);
   FIELD_READ_VALUE(strObj, vObj2);
   FIELD_READ_OFFSET(thisObj, offset);
   FIELD_READ_OFFSET(strObj, strOffset);
   v1 = (CVMArrayOfChar *)vObj1;
   v2 = (CVMArrayOfChar *)vObj2;

   CVMD_arrayReadChar(v2, strOffset, first);
   max = offset + (count - strCount);
   i = offset + fromIndex;

   while (i <= max) {
       /* Search for first character. */
       CVMD_arrayReadChar(v1, i, c1);
       if (c1 == first) {
	   /* Found first character, now look at the rest of v2 */
	   j = i + 1;
	   end = i + strCount;
	   k = strOffset + 1;
	   while(j < end) {
	       CVMD_arrayReadChar(v1, j, c1);
	       CVMD_arrayReadChar(v2, k, c2);
	       if (c1 != c2) {
		   goto continueSearch;
	       }
	       j++;
	       k++;
	   }
	   /* Found */
	   return i - offset;
       }
   continueSearch:
       i++;
   }
   /* If we are here, then we have failed to find v2 in v1.*/
   return -1;
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_STRING_INTRINSIC
/* Purpose: Intrinsic version of String.indexOf(String str). */
static CVMJavaInt
CVMCCMintrinsic_java_lang_String_indexOf_STRING(CVMCCExecEnv *ccee,
						CVMObject* thisObj,
						CVMObject *strObj)
{
    return CVMCCMintrinsic_java_lang_String_indexOf_STRING_I(ccee, thisObj,
							     strObj, 0);
}
#endif

#undef MIN
#undef FIELD_READ_COUNT
#undef FIELD_READ_VALUE
#undef FIELD_READ_OFFSET

#ifdef CVM_DEBUG_ASSERTS
#define CVMassertOKToCopyArrayOfType(expectedType_) \
    {                                                                   \
        CVMClassBlock *srcCb, *dstCb;                                   \
        CVMClassBlock *srcElemCb, *dstElemCb;                           \
        size_t srclen, dstlen;                                          \
        CVMArrayOfAnyType *srcArr;                                      \
        CVMArrayOfAnyType *dstArr;                                      \
                                                                        \
        srcArr = (CVMArrayOfAnyType *)src;                              \
        dstArr = (CVMArrayOfAnyType *)dst;                              \
        CVMassert(srcArr != NULL);                                      \
        CVMassert(dstArr != NULL);                                      \
                                                                        \
        srcCb = CVMobjectGetClass(srcArr);                              \
        dstCb = CVMobjectGetClass(dstArr);                              \
        CVMassert(CVMisArrayClass(srcCb));                              \
        CVMassert(CVMisArrayClass(dstCb));                              \
                                                                        \
        srcElemCb = CVMarrayElementCb(srcCb);                           \
        dstElemCb = CVMarrayElementCb(dstCb);                           \
        CVMassert(srcElemCb == dstElemCb);                              \
        CVMassert(CVMarrayElemTypeCode(srcCb) == expectedType_);        \
                                                                        \
        srclen = CVMD_arrayGetLength(srcArr);                           \
        dstlen = CVMD_arrayGetLength(dstArr);                           \
                                                                        \
        CVMassert(!(length < 0));                                       \
        CVMassert(!(src_pos < 0));                                      \
        CVMassert(!(dst_pos < 0));                                      \
        CVMassert(!(length + src_pos > srclen));                        \
        CVMassert(!(length + dst_pos > dstlen));                        \
    }
#else
#define CVMassertOKToCopyArrayOfType(expectedType_)
#endif /* CVM_DEBUG_ASSERTS */

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYBOOLEANARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyBooleanArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyBooleanArray(CVMArrayOfBoolean *src,
    jint src_pos, CVMArrayOfBoolean *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_BOOLEAN);
    CVMD_arrayCopyBoolean(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYBYTEARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyByteArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyByteArray(CVMArrayOfByte *src,
    jint src_pos, CVMArrayOfByte *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_BYTE);
    CVMD_arrayCopyByte(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYSHORTARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyShortArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyShortArray(CVMArrayOfShort *src,
    jint src_pos, CVMArrayOfShort *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_SHORT);
    CVMD_arrayCopyShort(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYCHARARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyCharArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyCharArray(CVMArrayOfChar *src,
    jint src_pos, CVMArrayOfChar *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_CHAR);
    CVMD_arrayCopyChar(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYINTARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyIntArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyIntArray(CVMArrayOfInt *src,
    jint src_pos, CVMArrayOfInt *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_INT);
    CVMD_arrayCopyInt(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYLONGARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyLongArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyLongArray(CVMArrayOfLong *src,
    jint src_pos, CVMArrayOfLong *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_LONG);
    CVMD_arrayCopyLong(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYFLOATARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyFloatArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyFloatArray(CVMArrayOfFloat *src,
    jint src_pos, CVMArrayOfFloat *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_FLOAT);
    CVMD_arrayCopyFloat(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYDOUBLEARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyDoubleArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyDoubleArray(CVMArrayOfDouble *src,
    jint src_pos, CVMArrayOfDouble *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_DOUBLE);
    CVMD_arrayCopyDouble(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYOBJECTARRAY_INTRINSIC
/* Purpose: Intrinsic version of CVM.copyObjectArray(). */
void
CVMCCMintrinsic_sun_misc_CVM_copyObjectArray(CVMArrayOfRef *src,
    jint src_pos, CVMArrayOfRef *dst, jint dst_pos, jint length)
{
    CVMassertOKToCopyArrayOfType(CVM_T_CLASS);
    if (length != 0)
	CVMD_arrayCopyRef(src, src_pos, dst, dst_pos, length);
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_SYSTEM_ARRAYCOPY_INTRINSIC
/* Purpose: Intrinsic version of System.arraycopy(). */
void
CVMCCMintrinsic_java_lang_System_arraycopy(CVMCCExecEnv *ccee,
    CVMArrayOfAnyType *srcArr, jint src_pos,
    CVMArrayOfAnyType *dstArr, jint dst_pos, jint length)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMClassBlock *srcCb, *dstCb;
    CVMClassBlock *srcElemCb, *dstElemCb;
    size_t srclen;
    size_t dstlen;

    CVMassert(CVMD_isgcUnsafe(ee));

    if ((srcArr == 0) || (dstArr == 0)) {
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowNullPointerException(ee, NULL);
        goto bail;
    }
    srcCb = CVMobjectGetClass(srcArr);
    dstCb = CVMobjectGetClass(dstArr);

    /*
     * First check whether we are dealing with source and destination
     * arrays here. If not, bail out quickly.
     */
    if (!CVMisArrayClass(srcCb) || !CVMisArrayClass(dstCb)) {
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowArrayStoreException(ee, NULL);
        goto bail;
    }

    srcElemCb = CVMarrayElementCb(srcCb);
    dstElemCb = CVMarrayElementCb(dstCb);

    srclen = CVMD_arrayGetLength(srcArr);
    dstlen = CVMD_arrayGetLength(dstArr);

    /*
     * If we are trying to copy between
     * arrays that don't have the same type of primitive elements,
     * bail out quickly.
     *
     * The more refined assignability checks for reference arrays
     * will be performed in the CVM_T_CLASS case below.
     */
    /*
     * We could make this check slightly faster by comparing
     * type-codes of the elements of the array, if we had a
     * quick way of getting to that.
     */
    if ((srcElemCb != dstElemCb) && (CVMcbIs(srcElemCb, PRIMITIVE) ||
                                     CVMcbIs(dstElemCb, PRIMITIVE))) {
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowArrayStoreException(ee, NULL);
        goto bail;
    } else if ((length < 0) || (src_pos < 0) || (dst_pos < 0) ||
               (length + src_pos > srclen) ||
               (length + dst_pos > dstlen)) {
        CVMCCMruntimeLazyFixups(ee);
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        goto bail;
    }
    
    switch(CVMarrayElemTypeCode(srcCb)) {
    case CVM_T_BOOLEAN: 
        CVMD_arrayCopyBoolean((CVMArrayOfBoolean*)srcArr, src_pos,
                              (CVMArrayOfBoolean*)dstArr, dst_pos,
                              length);
        break;
    case CVM_T_BYTE: 
        CVMD_arrayCopyByte((CVMArrayOfByte*)srcArr, src_pos,
                           (CVMArrayOfByte*)dstArr, dst_pos,
                           length);
        break;
        
    case CVM_T_SHORT:
        CVMD_arrayCopyShort((CVMArrayOfShort*)srcArr, src_pos,
                            (CVMArrayOfShort*)dstArr, dst_pos,
                            length);
        break;
    case CVM_T_CHAR:
        CVMD_arrayCopyChar((CVMArrayOfChar*)srcArr, src_pos,
                           (CVMArrayOfChar*)dstArr, dst_pos,
                           length);
        break;
    case CVM_T_INT:
        CVMD_arrayCopyInt((CVMArrayOfInt*)srcArr, src_pos,
                          (CVMArrayOfInt*)dstArr, dst_pos,
                          length);
        break;
    case CVM_T_LONG:
        CVMD_arrayCopyLong((CVMArrayOfLong*)srcArr, src_pos,
                           (CVMArrayOfLong*)dstArr, dst_pos,
                           length);
        break;
    case CVM_T_FLOAT: 
        CVMD_arrayCopyFloat((CVMArrayOfFloat*)srcArr, src_pos,
                            (CVMArrayOfFloat*)dstArr, dst_pos,
                            length);
        break;
    case CVM_T_DOUBLE: 
        CVMD_arrayCopyDouble((CVMArrayOfDouble*)srcArr, src_pos,
                             (CVMArrayOfDouble*)dstArr, dst_pos,
                             length);
        break;
    case CVM_T_CLASS: {
        /*
         * Do a quick check here for the easy case of the same
         * array type, or copying into an array of objects. No
         * need for element-wise checks in that case.  
         */
        if ((srcElemCb == dstElemCb) ||
            (dstElemCb == CVMsystemClass(java_lang_Object))) {
            CVMD_arrayCopyRef((CVMArrayOfRef*)srcArr, src_pos,
                              (CVMArrayOfRef*)dstArr, dst_pos,
                              length);
        } else {
            /* Do it the hard way. */
            CVMcopyRefArrays(ee, 
                             (CVMArrayOfRef*)srcArr, src_pos, 
                             (CVMArrayOfRef*)dstArr, dst_pos, 
                             dstElemCb, length);
        }
        break;
    case CVM_T_VOID: 
        CVMassert(CVM_FALSE);
    }
    }

bail:

    /* Offer a gc-safe checkpoint just to be nice. */
    if (length >= 100) {
        CVMCCMruntimeLazyFixups(ee);
        CVMD_gcSafeCheckPoint(ee, {}, {});
    }

    /* NOTE: Normally the callee would pop the arguments.  But in this case,
       we can skip it because we expect to be called from glue code that will
       take care of it:
       ee->interpreterStack.currentFrame->topOfStack = arguments;
    */
    if (CVMlocalExceptionOccurred(ee)) {
        CVMCCMhandleException(ccee);
    }
}
#endif

#ifndef CVMCCM_DISABLE_SHARED_THREAD_CURRENTTHREAD_INTRINSIC
/* Purpose: Intrinsic version of String.charAt(). */
CVMObject *
CVMCCMintrinsic_java_lang_Thread_currentThread(CVMCCExecEnv *ccee)
{
    CVMExecEnv *ee = CVMcceeGetEE(ccee);
    CVMObject *threadObj;
    CVMassert(CVMD_isgcUnsafe(ee));
    threadObj = CVMID_icellDirect(ee, CVMcurrentThreadICell(ee));
    return threadObj;
}
#endif

/* NOTE: See "How to fill out the CVMJITIntrinsicConfig entries below?"
         in jitintrinsic.h for details on how to fill out the intrinsic
         config entries.
*/

CVMJIT_INTRINSIC_CONFIG_BEGIN(CVMJITdefaultIntrinsicsList)
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_COS_INTRINSIC
    {
        "java/lang/StrictMath", "cos", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmCos,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_SIN_INTRINSIC
    {
        "java/lang/StrictMath", "sin", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmSin,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_TAN_INTRINSIC
    {
        "java/lang/StrictMath", "tan", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmTan,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_ACOS_INTRINSIC
    {
        "java/lang/StrictMath", "acos", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmAcos,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_ASIN_INTRINSIC
    {
        "java/lang/StrictMath", "asin", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmAsin,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_ATAN_INTRINSIC
    {
        "java/lang/StrictMath", "atan", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmAtan,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_EXP_INTRINSIC
    {
        "java/lang/StrictMath", "exp", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmExp,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_LOG_INTRINSIC
    {
        "java/lang/StrictMath", "log", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmLog,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_SQRT_INTRINSIC
    {
        "java/lang/StrictMath", "sqrt", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmSqrt,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_CEIL_INTRINSIC
    {
        "java/lang/StrictMath", "ceil", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmCeil,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_FLOOR_INTRINSIC
    {
        "java/lang/StrictMath", "floor", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmFloor,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_ATAN2_INTRINSIC
    {
        "java/lang/StrictMath", "atan2", "(DD)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmAtan2,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_POW_INTRINSIC
    {
        "java/lang/StrictMath", "pow", "(DD)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmPow,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_IEEEREMAINDER_INTRINSIC
    {
        "java/lang/StrictMath", "IEEEremainder", "(DD)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmRemainder,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRICTMATH_RINT_INTRINSIC
    {
        "java/lang/StrictMath", "rint", "(D)D",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_NULL_FLAGS,
        (void*)CVMfdlibmRint,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_CHARAT_INTRINSIC
    {
        "java/lang/String", "charAt", "(I)C",
        CVMJITINTRINSIC_IS_NOT_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_String_charAt,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_COMPARETO_INTRINSIC
    {
        "java/lang/String", "compareTo", "(Ljava/lang/String;)I",
        CVMJITINTRINSIC_IS_NOT_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_String_compareTo,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_EQUALS_INTRINSIC
    {
        "java/lang/String", "equals", "(Ljava/lang/Object;)Z",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_equals,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_GETCHARS_INTRINSIC
    {
        "java/lang/String", "getChars", "(II[CI)V",
        CVMJITINTRINSIC_IS_NOT_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_String_getChars,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_I_INTRINSIC
    {
        "java/lang/String", "indexOf", "(I)I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_I,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_II_INTRINSIC
    {
        "java/lang/String", "indexOf", "(II)I",
        CVMJITINTRINSIC_IS_NOT_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_II,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_STRING_INTRINSIC
    {
        "java/lang/String", "indexOf", "(Ljava/lang/String;)I",
        CVMJITINTRINSIC_IS_NOT_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_STRING,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_STRING_INDEXOF_STRING_I_INTRINSIC
    {
        "java/lang/String", "indexOf", "(Ljava/lang/String;I)I",
        CVMJITINTRINSIC_IS_NOT_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_String_indexOf_STRING_I,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_SYSTEM_CURRENTTIMEMILLIS_INTRINSIC
    {
        "java/lang/System", "currentTimeMillis", "()J",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)Java_java_lang_System_currentTimeMillis,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_SYSTEM_ARRAYCOPY_INTRINSIC
    {
        "java/lang/System",
        "arraycopy", "(Ljava/lang/Object;ILjava/lang/Object;II)V",
        CVMJITINTRINSIC_IS_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MAJOR_SPILL |
        CVMJITINTRINSIC_NEED_STACKMAP | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS |
        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME,
        CVMJITIRNODE_THROWS_EXCEPTIONS,
        (void*)CVMCCMintrinsic_java_lang_System_arraycopy,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_THREAD_CURRENTTHREAD_INTRINSIC
    {
        "java/lang/Thread", "currentThread", "()Ljava/lang/Thread;",
        CVMJITINTRINSIC_IS_STATIC | CVMJITINTRINSIC_ADD_CCEE_ARG |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_NO_CP_DUMP,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_java_lang_Thread_currentThread,
    },
#endif

#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYBOOLEANARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyBooleanArray", "([ZI[ZII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyBooleanArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYBYTEARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyByteArray", "([BI[BII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyByteArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYSHORTARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyShortArray", "([SI[SII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyShortArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYCHARARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyCharArray", "([CI[CII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyCharArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYINTARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyIntArray", "([II[III)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyIntArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYLONGARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyLongArray", "([JI[JII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyLongArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYFLOATARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyFloatArray", "([FI[FII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyFloatArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYDOUBLEARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyDoubleArray", "([DI[DII)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyDoubleArray,
    },
#endif
#ifndef CVMCCM_DISABLE_SHARED_CVM_COPYOBJECTARRAY_INTRINSIC
    {
        "sun/misc/CVM", "copyObjectArray",
        "([Ljava/lang/Object;I[Ljava/lang/Object;II)V",
        CVMJITINTRINSIC_IS_STATIC |
        CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_NEED_MINOR_SPILL |
        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED | CVMJITINTRINSIC_CP_DUMP_OK |
        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS,
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT,
        (void*)CVMCCMintrinsic_sun_misc_CVM_copyObjectArray,
    },
#endif

CVMJIT_INTRINSIC_CONFIG_END(CVMJITdefaultIntrinsicsList, NULL)

#endif /* CVMJIT_INTRINSICS */
