/*
 * @(#)String.c	1.19 06/10/10
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

#include "jvm.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"

#include "generated/offsets/java_lang_String.h"

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

/*
 * Note: String is a final class. It uses a private
 * array as character storage. String constructors
 * record the first index of character and the number
 * of total characters in the storage, and throw 
 * StringIndexOutOfBoundsException if necessary.
 * Theoretically we are safe when accessing String
 * characters as long as we don't exceed the 'count'. 
 * So we can eliminate array bounds checking for 
 * some cases. 
 */

/*
 * Class:       java/lang/String
 * Method:      getChars
 * Signature:   (II[CI)V
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_getChars(CVMExecEnv* ee, CVMStackVal32 *arguments,
                             CVMMethodBlock **p_mb)
{
    CVMObject *thisStringObject;
    CVMJavaInt srcBegin = arguments[1].j.i;
    CVMJavaInt srcEnd = arguments[2].j.i;
    CVMArrayOfChar *dst; 
    CVMJavaInt dstBegin = arguments[4].j.i;

    CVMJavaInt count;
    CVMJavaInt offset;
    CVMJavaInt dstLen;
    CVMObject *valueObject;
    CVMArrayOfChar *value;

    CVMClassBlock *dstCb;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    thisStringObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    dst = (CVMArrayOfChar *)CVMID_icellDirect(ee, &arguments[3].j.r);

    /* Copy from the Java version of String.getChars*/
    if (srcBegin < 0) {
        CVMthrowStringIndexOutOfBoundsException(
            ee, "String index out of range: %d", srcBegin);
        return CNI_EXCEPTION;
    } 

    FIELD_READ_COUNT(thisStringObject, count);
    if (srcEnd > count) {
        CVMthrowStringIndexOutOfBoundsException(
            ee, "String index out of range: %d", srcEnd);
        return CNI_EXCEPTION;
    }

    if (srcBegin > srcEnd) {
        CVMthrowStringIndexOutOfBoundsException(
            ee, "String index out of range: %d", srcEnd - srcBegin);
        return CNI_EXCEPTION;
    }

    /* Check NULL for dstObject */
    if (dst == NULL) {
        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION;
    }

    /* Check whether dst is a char array */
    dstCb = CVMobjectGetClass(dst);
    if (!CVMisArrayClass(dstCb) || CVMarrayBaseType(dstCb) != CVM_T_CHAR) {
        CVMthrowArrayStoreException(ee, NULL);
        return CNI_EXCEPTION;
    }

    /* Array bounds checking for dst*/
    dstLen = CVMD_arrayGetLength(dst);
    if (dstBegin < 0 || dstBegin > dstLen || 
        dstBegin + srcEnd - srcBegin > dstLen) {
        CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
        return CNI_EXCEPTION; 
    }

    FIELD_READ_VALUE(thisStringObject, valueObject);
    FIELD_READ_OFFSET(thisStringObject, offset);
    value = (CVMArrayOfChar *)valueObject;
    CVMD_arrayCopyChar(value, offset + srcBegin, 
                       dst, dstBegin, srcEnd - srcBegin); 
    return CNI_VOID; 
} 

/*
 * Class:       java/lang/String
 * Method:      equals
 * Signature:   (Ljava/lang/Object;)Z
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_equals(CVMExecEnv* ee, CVMStackVal32 *arguments,
                           CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisStringICell = &arguments[0].j.r;
    CVMObjectICell *anObjectICell = &arguments[1].j.r;
    CVMObject *thisStringObject = CVMID_icellDirect(ee, thisStringICell);
    CVMObject *anObject = CVMID_icellDirect(ee, anObjectICell);
    
    if (thisStringObject == anObject) {
        /* Same object, return true directly. */
        arguments[0].j.i = CVM_TRUE;
        return CNI_SINGLE;
    } else if (anObject == NULL) {
        /* 
         * If anObject is null, the Java version 
         * always returns False, unless this String
         * is also null, in which case a NullPointerException
         * is already thrown by the interpreter.
         */
        arguments[0].j.i = CVM_FALSE;
        return CNI_SINGLE; 
    }

    if (CVMobjectGetClass(anObject) == CVMsystemClass(java_lang_String)) {
        CVMJavaInt count1;
        CVMJavaInt count2;
        /* The argument anObject is a String. So compare. */
        FIELD_READ_COUNT(thisStringObject, count1);
        FIELD_READ_COUNT(anObject, count2);
        if (count1 == count2) {
            CVMObject *valueObject1;
            CVMObject *valueObject2;
            CVMArrayOfChar *v1;
            CVMArrayOfChar *v2;
            CVMJavaInt i;
            CVMJavaInt j;
            CVMJavaChar c1;
            CVMJavaChar c2;

            FIELD_READ_VALUE(thisStringObject, valueObject1);
            FIELD_READ_VALUE(anObject, valueObject2);
            v1 = (CVMArrayOfChar *)valueObject1;
            v2 = (CVMArrayOfChar *)valueObject2; 

            FIELD_READ_OFFSET(thisStringObject, i); 
            FIELD_READ_OFFSET(anObject, j);

            while (count1-- > 0) {
		CVMD_arrayReadChar(v1, i, c1);
		CVMD_arrayReadChar(v2, j, c2);
		if (c1 != c2) {
		    arguments[0].j.i = CVM_FALSE;
		    return CNI_SINGLE; 
		}
		i++;
		j++;
	    }

            arguments[0].j.i = CVM_TRUE;
            return CNI_SINGLE;
        } 
    }
    
    arguments[0].j.i = CVM_FALSE;
    return CNI_SINGLE;
}

/*
 * Class:       java/lang/String
 * Method:      compareTo
 * Signature:   (Ljava/lang/String;)I
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_compareTo(CVMExecEnv* ee, CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb) 
{
    CVMObjectICell *thisStringICell = &arguments[0].j.r;
    CVMObjectICell *anotherStringICell = &arguments[1].j.r;
    CVMObject* thisStringObject = 
        CVMID_icellDirect(ee, thisStringICell);
    CVMObject* anotherStringObject = 
        CVMID_icellDirect(ee, anotherStringICell);
    
    CVMJavaInt len1;
    CVMJavaInt len2;
    CVMJavaInt n;

    CVMObject *valueObject1;
    CVMObject *valueObject2;
    CVMArrayOfChar *v1;
    CVMArrayOfChar *v2;

    CVMJavaInt i;
    CVMJavaInt j;

    CVMJavaChar c1;
    CVMJavaChar c2;

    /* 
     * If anObject is null, a NullPointerException
     * should be thrown. The interpreter already
     * did CHECK_NULL for this String.
     */
    if (anotherStringObject == NULL) {
        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION;
    }

    /* Get count, value, and offset */
    FIELD_READ_COUNT(thisStringObject, len1);
    FIELD_READ_COUNT(anotherStringObject, len2);
    n = MIN(len1, len2);

    FIELD_READ_VALUE(thisStringObject, valueObject1);
    FIELD_READ_VALUE(anotherStringObject, valueObject2);
    v1 = (CVMArrayOfChar *)valueObject1;
    v2 = (CVMArrayOfChar *)valueObject2; 

    FIELD_READ_OFFSET(thisStringObject, i);
    FIELD_READ_OFFSET(anotherStringObject, j);

    /* Now compare */
    if (i == j) {
        /* This is an optimized case */
        while (n-- > 0) {
	    CVMD_arrayReadChar(v1, i, c1);
	    CVMD_arrayReadChar(v2, i, c2);
	    if (c1 != c2) {
		arguments[0].j.i = (CVMJavaInt)c1 - (CVMJavaInt)c2;
		return CNI_SINGLE;
	    }
	    i++;
        }
    } else {
        while (n-- > 0) {
	    CVMD_arrayReadChar(v1, i, c1);
	    CVMD_arrayReadChar(v2, j, c2);
	    i++;
	    j++;
	    if (c1 != c2) {
		arguments[0].j.i = (CVMJavaInt)c1 - (CVMJavaInt)c2;
		return CNI_SINGLE;
	    }
	}
    }
    arguments[0].j.i = len1 - len2;
    return CNI_SINGLE;
}

/*
 * The SE6 version of hashCode is in Java and caches the hash
 * in String.hash. It calls out to hashCode0 to compute the hash.
 */
#undef STRING_HASHCODE
#if !(JAVASE >= 16)
#define STRING_HASHCODE CNIjava_lang_String_hashCode
#else
#define STRING_HASHCODE CNIjava_lang_String_hashCode0
#endif

/*
 * Class:       java/lang/String
 * Method:      hashCode
 * Signature:   ()I
 */
CNIEXPORT CNIResultCode
STRING_HASHCODE(CVMExecEnv* ee, CVMStackVal32 *arguments,
                             CVMMethodBlock **p_mb)
{
    CVMObjectICell * thisICell = &arguments[0].j.r;
    CVMObject *thisObject = CVMID_icellDirect(ee, thisICell);
    CVMObject *valueObject;
    CVMArrayOfChar *value;
    CVMJavaInt offset;
    CVMJavaInt count;
    CVMJavaChar c;
    CVMJavaInt h = 0;

    FIELD_READ_VALUE(thisObject, valueObject);
    FIELD_READ_OFFSET(thisObject, offset);
    FIELD_READ_COUNT(thisObject, count);
    value = (CVMArrayOfChar *)valueObject; 

    while (count-- > 0 ) {
	CVMD_arrayReadChar(value, offset, c); 
	h = 31*h + (CVMJavaInt)c;
	offset ++;
    }
    arguments[0].j.i = h;
    return CNI_SINGLE; 
}

/*
 * SE6 has a newer version of indexOf(int, int) that does some extra
 * handling not currently done in stringIndexOfHelper1.
*/
#if !(JAVASE >= 16)

/*
 * Helper function for indexOf(I)I and indexOf(II)I.
 */
static CVMJavaInt
stringIndexOfHelper1(CVMExecEnv* ee,  CVMObjectICell *thisICell,
                     CVMJavaInt ch, CVMJavaInt fromIndex) 
{
    CVMObject *thisObj = CVMID_icellDirect(ee, thisICell);
    CVMObject *valueObj;
    CVMArrayOfChar *valueArr;
    CVMJavaInt offset;
    CVMJavaInt count;
    CVMJavaInt max;
    CVMJavaInt i;
    CVMJavaChar c;

    FIELD_READ_COUNT(thisObj, count);
 
    /* The argument fromIndex is always >=0. Guaranteed by caller.*/ 
    if (fromIndex >= count) {
        return -1;
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

/*
 * Class:       java/lang/String
 * Method:      indexOf
 * Signature:   (I)I
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_indexOf__I(CVMExecEnv* ee, CVMStackVal32 *arguments,
                               CVMMethodBlock **p_mb)
{
   CVMObjectICell *thisICell = &arguments[0].j.r;
   CVMJavaInt ch = arguments[1].j.i;

   arguments[0].j.i = stringIndexOfHelper1(ee, thisICell, ch, 0);
   return CNI_SINGLE;
}

/*
 * Class:       java/lang/String
 * Method:      indexOf
 * Signature:   (II)I
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_indexOf__II(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisICell = &arguments[0].j.r;
    CVMJavaInt ch = arguments[1].j.i;
    CVMJavaInt fromIndex = arguments[2].j.i;

    if (fromIndex < 0) {
        fromIndex = 0;
    }
 
    arguments[0].j.i = stringIndexOfHelper1(ee, thisICell, ch, fromIndex);
    return CNI_SINGLE;
}

#endif /* !JAVASE */

/*
 * Helper function for indexOf(Ljava/lang/String;)I
 * and indexOf(Ljava/lang/String;I)I.
 */
static CVMJavaInt
stringIndexOfHelper2(CVMExecEnv* ee, CVMObjectICell *thisICell,
                     CVMObjectICell *strICell, CVMJavaInt fromIndex)
{
   CVMObject *thisObj = CVMID_icellDirect(ee, thisICell);
   CVMObject *strObj = CVMID_icellDirect(ee, strICell);
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

   /* The caller is responsible for NULL checking for strICell. */
   FIELD_READ_COUNT(thisObj, count);
   FIELD_READ_COUNT(strObj, strCount);

   if (fromIndex >= count) {
       if (strCount == 0) {
           /* 
            * Special case: return the length of the first string
            * when searching for an empty String, 4276204
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

/*
 * Class:       java/lang/String
 * Method:      indexOf
 * Signature:   (Ljava/lang/String;)I
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_indexOf__Ljava_lang_String_2(CVMExecEnv* ee,
                                                 CVMStackVal32 *arguments,
                                                 CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisICell = &arguments[0].j.r;
    CVMObjectICell *strICell = &arguments[1].j.r;
  
    /* NULL check for the argument string */ 
    if (CVMID_icellIsNull(strICell)) {
        CVMthrowNullPointerException(ee, NULL);
        return CNI_EXCEPTION;
    }

    arguments[0].j.i = stringIndexOfHelper2(ee, thisICell, strICell, 0);
    return CNI_SINGLE;
}

/*
 * Class:       java/lang/String
 * Method:      indexOf
 * Signature:   (Ljava/lang/String;I)I
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_indexOf__Ljava_lang_String_2I(CVMExecEnv* ee, 
                                                  CVMStackVal32 *arguments,
                                                  CVMMethodBlock **p_mb)
{
   CVMObjectICell *thisICell = &arguments[0].j.r;
   CVMObjectICell *strICell = &arguments[1].j.r;
   CVMJavaInt fromIndex = arguments[2].j.i;

   /* NULL check for the argument string */
   if (CVMID_icellIsNull(strICell)) {
       CVMthrowNullPointerException(ee, NULL);
       return CNI_EXCEPTION;
   }

   arguments[0].j.i = stringIndexOfHelper2(ee, thisICell, strICell, fromIndex);
   return CNI_SINGLE;
}

/*
 * Class:       java/lang/String
 * Method:      intern
 * Signature:   ()Ljava/lang/String;
 */
CNIEXPORT CNIResultCode
CNIjava_lang_String_intern(CVMExecEnv* ee, CVMStackVal32 *arguments,
                           CVMMethodBlock **p_mb)
{
    CVMObjectICell *thisString = &arguments[0].j.r;
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMObjectICell *result = NULL;

    CVMD_gcSafeExec(ee, {
        if ((*env)->PushLocalFrame(env, 4) == 0) {
            result = (CVMObjectICell *)JVM_InternString(
                env, (jobject)thisString);
            if (result != NULL) {
                CVMID_icellAssign(ee, &arguments[0].j.r, result);
            }
            (*env)->PopLocalFrame(env, NULL);
        }
    });

    if (CVMexceptionOccurred(ee)) {
        return CNI_EXCEPTION;
    }

    return CNI_SINGLE;
}
