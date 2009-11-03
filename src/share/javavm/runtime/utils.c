/*
 * @(#)utils.c	1.158 06/10/10
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

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/path.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/clib.h"
#include "javavm/include/utils.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/preloader.h"
#include "javavm/include/localroots.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/stacks.h"
#include "javavm/include/path_md.h"
#include "javavm/include/globalroots.h"

#include "generated/javavm/include/build_defs.h"
#include "generated/offsets/java_lang_String.h"
#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_Boolean.h"
#include "generated/offsets/java_lang_Character.h"
#include "generated/offsets/java_lang_Float.h"
#include "generated/offsets/java_lang_Double.h"
#include "generated/offsets/java_lang_Byte.h"
#include "generated/offsets/java_lang_Short.h"
#include "generated/offsets/java_lang_Integer.h"
#include "generated/offsets/java_lang_Long.h"
#include "generated/offsets/java_lang_Throwable.h"
#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif
#ifdef CVM_DEBUG
#include "generated/offsets/java_lang_Thread.h"
#endif

#include "jni_util.h"

/*
 * Console I/O. Same as printf, but supports special formats like
 * %C, %P, and %F. See CVMformatStringVaList for details.
 *
 * WARNING: may crash if you pass bad arguments. See CVMformatStringVaList
 * for details on safety checking limitations.
 */
void
CVMconsolePrintf(const char* format, ...)
{
    va_list argList;
    char buf[256];

    va_start(argList, format);
    CVMformatStringVaList(buf, sizeof(buf), format, argList);
    va_end(argList);
    fprintf(stderr, "%s", buf);
    fflush(stderr);
}

/*
 * Dump an exception using CVMconsolePrintf. Useful for failures during
 * vm startup and for debugging in gdb.
 */
void
CVMdumpException(CVMExecEnv* ee) {
    CVMClassBlock* exceptionCb;
    CVMObjectICell* exceptionICell = CVMID_getGlobalRoot(ee);
    CVMObjectICell* originalExceptionICell = CVMID_getGlobalRoot(ee);
    CVMStringICell* exceptionMessage;

    /* print the exception class name */
    CVMID_objectGetClass(ee, CVMlocalExceptionICell(ee), exceptionCb);
    CVMconsolePrintf("%C: ", exceptionCb);

    if (exceptionICell == NULL || originalExceptionICell == NULL) {
        CVMconsolePrintf("backtrace not available\n");
        goto delete_roots;
    }

    /* Fetch and clear the exception. Required if calling CVMprintStackTrace */
    CVMID_icellAssign(ee, exceptionICell,
		      CVMlocalExceptionICell(ee));
    CVMID_icellAssign(ee, originalExceptionICell,
		      CVMlocalExceptionICell(ee));
    CVMclearLocalException(ee);

 nextException:
    /* print the exception detailedMessage if there is one */
    exceptionMessage = CVMID_getGlobalRoot(ee);
    if (exceptionMessage != NULL) {
        CVMID_fieldReadRef(ee, exceptionICell, 
        	       CVMoffsetOfjava_lang_Throwable_detailMessage,
		       exceptionMessage);
        if (!CVMID_icellIsNull(exceptionMessage)) {
	    JNIEnv* env = CVMexecEnv2JniEnv(ee);
	    const char* str =
		    CVMjniGetStringUTFChars(env, exceptionMessage, NULL);
	    if (str != NULL) {
	        CVMconsolePrintf("%s", str);
	        CVMjniReleaseStringUTFChars(env, exceptionMessage, str);
	    } else {
		CVMclearLocalException(ee);
	    }
        }
        CVMconsolePrintf("\n");
        CVMID_freeGlobalRoot(ee, exceptionMessage);
    }

    /* print the exception backtrace */
    CVMprintStackTrace(ee, exceptionICell, NULL);

    /* now check for the cause of the exception, if any */
    {
        JNIEnv* env = CVMexecEnv2JniEnv(ee);
        CVMClassBlock* cb;
        CVMMethodBlock* mb;
	jobject causeICell;

        CVMD_gcUnsafeExec(ee, {
            cb = CVMobjectGetClass(CVMID_icellDirect(ee, exceptionICell));
        });
        mb = CVMclassGetNonstaticMethodBlock(cb, CVMglobals.getCauseTid);
        CVMassert(mb != NULL);

        /* Get the cause of the exception, if any. */
        causeICell = (*env)->CallObjectMethod(env, exceptionICell, mb);

        if (causeICell != NULL) {
            CVMID_icellAssign(ee, exceptionICell, causeICell);
            CVMjniDeleteLocalRef(env, causeICell);
        } else {
            CVMID_icellSetNull(exceptionICell);
        }
    }

    /* print the cause of the exception if one was found */
    if (!CVMID_icellIsNull(exceptionICell)) {
        CVMconsolePrintf("Caused by: %C: ", exceptionCb);
        goto nextException; /* print next cause if there is one */
    }

    /* restore the local exception */
    CVMID_icellAssign(ee, CVMlocalExceptionICell(ee),
		      originalExceptionICell);

    /* delete the global roots */
 delete_roots:
    if (exceptionICell != NULL) {
        CVMID_freeGlobalRoot(ee, exceptionICell);
    }
    if (originalExceptionICell != NULL) {
        CVMID_freeGlobalRoot(ee, originalExceptionICell);
    }
}

/*
 * Random number generation.
 *
 * Standard, well-known linear congruential random generator with
 *
 *     s(n+1) = 16807 * s(n) mod(2^31 - 1)
 *
 * For references, see:
 *
 * (1) "Random Number Generators: Good Ones Are Hard to Find",
 *      S.K. Park and K.W. Miller, Communications of the ACM 31:10 
 *      (Oct 1988),
 *
 * (2) "Two Fast Implementations of the 'Minimal Standard' Random 
 *     Number Generator", David G. Carta, Comm. ACM 33, 1 (Jan 1990), 
 *     pp. 87-88. 
 */
CVMInt32
CVMrandomNext()
{
    const CVMInt32 a = 16807;
    const CVMInt32 m = 2147483647;
    CVMInt32 lastRandom = CVMglobals.lastRandom;
    
    /* compute az=2^31p+q */
    CVMUint32 lo = a * (CVMInt32)(lastRandom & 0xFFFF);
    CVMUint32 hi = a * (CVMInt32)((CVMUint32)lastRandom >> 16);
    lo += (hi & 0x7FFF) << 16;
    
    /* if q overflowed, ignore the overflow and increment q */
    if (lo > m) {
	lo &= m;
	++lo;
    }
    lo += hi >> 15;
    
    /* if (p+q) overflowed, ignore the overflow and increment (p+q) */
    if (lo > m) {
	lo &= m;
	++lo;
    }
    CVMglobals.lastRandom = lo;
    return lo;
}

void
CVMrandomInit()
{
    CVMInt32 seed;
    CVMInt64 millis = CVMtimeMillis();
    seed = CVMlong2Int(millis);
    if (seed == 0) {
	seed = 32461;
    }
    CVMglobals.lastRandom = seed;
}

/*
 * Functions for accessing the debugging flags. All of the following functions
 * return the previous state of the flag.
 *
 * You can pass in more than one flag at a time to any of the functions.
 */
#ifdef CVM_TRACE_ENABLED

/* NOTE: CVMcheckDebugFlags() is a macro and is defined in utils.h. */

CVMInt32
CVMsetDebugFlags(CVMInt32 flags)
{
    CVMInt32 result = CVMcheckDebugFlags(flags);
    CVMInt32 oldValue = CVMglobals.debugFlags;
    CVMInt32 newValue = CVMglobals.debugFlags | flags;
    CVMglobals.debugFlags = newValue;
    CVMtraceReset(oldValue, newValue);
    return result;
}

CVMInt32
CVMclearDebugFlags(CVMInt32 flags)
{
    CVMInt32 result = CVMcheckDebugFlags(flags);
    CVMInt32 oldValue = CVMglobals.debugFlags;
    CVMInt32 newValue = CVMglobals.debugFlags & ~flags;
    CVMglobals.debugFlags &= ~flags;
    CVMtraceReset(oldValue, newValue);
    return result;
}

CVMInt32
CVMrestoreDebugFlags(CVMInt32 flags, CVMInt32 oldvalue)
{
    CVMInt32 result = CVMcheckDebugFlags(flags);
    CVMInt32 oldValue = CVMglobals.debugFlags;
    CVMInt32 newValue = (CVMglobals.debugFlags & ~flags) | oldvalue;
    CVMglobals.debugFlags = newValue;
    CVMtraceReset(oldValue, newValue);
    return result;
}

#endif /* CVM_TRACE_ENABLED */

#if defined(CVM_DEBUG) || defined(CVM_TRACE_ENABLED)
void CVMlong2String(CVMInt64 number, char *s, char *limit)
{
#undef MAGIC
#define MAGIC    10000000       /* slightly more than (2^64) ^ (1/3) */
    int group1, group2, group3;
    CVMInt64 group1l, group2l, group3l;
    CVMInt64 magic;
    char *sign;

    if (CVMlongGez(number)) {
        sign = "";
	/* number = -number */
	number = CVMlongNeg(number);	/* this lets -2^63 still work! */
    } else {
        sign = "-";
    }

#if 0
    group3 = -(number % MAGIC); /* low 7 digits */
    number = (number + group3)/MAGIC;
    group2 = -(number % MAGIC); /* next 7 digits */
    number = (number + group2)/MAGIC;
    group1 = -number;           /* high 7 digits */
#endif

    magic = CVMint2Long(MAGIC);
    group3l = CVMlongRem(number, magic); /* low 7 digits */
    group3l = CVMlongNeg(group3l);
    number = CVMlongAdd(number, group3l);
    number = CVMlongDiv(number, magic); /* truncate middle 7 digits */
    group2l = CVMlongRem(number, magic); /* next 7 digits */
    group2l = CVMlongNeg(group2l);
    number = CVMlongAdd(number, group2l);
    number = CVMlongDiv(number, magic); /* truncate middle 7 digits */
    group1l = CVMlongNeg(number);         /* high 7 digits */

    group1 = CVMlong2Int(group1l);
    group2 = CVMlong2Int(group2l);
    group3 = CVMlong2Int(group3l);

    if (group1 != 0) {
	(void) sprintf(s, "%s%d%07d%07d", sign, group1, group2, group3);
    } else if (group2 != 0) {
	(void) sprintf(s, "%s%d%07d", sign, group2, group3);
    } else {
	(void) sprintf(s, "%s%d", sign, group3);
    }
}
#endif


/*
 * CVMdumpThread - prints thread name and stack trace.
 */
#ifdef CVM_DEBUG
void
CVMdumpThread(JNIEnv* env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    CVMconsolePrintf("Thread \"");
    CVMprintThreadName(env, CVMcurrentThreadICell(ee));
    CVMconsolePrintf("\"\n");
#ifdef CVM_DEBUG_DUMPSTACK
    CVMdumpStack(&ee->interpreterStack, 0, 0, 0);
#endif
}

static void
CVMdumpThreadFromEENoAlloc(CVMExecEnv* ee)
{
    CVMconsolePrintf("Thread ee=0x%x, ID=%d\n", ee, ee->threadID);
#ifdef CVM_DEBUG_DUMPSTACK
    CVMdumpStack(&ee->interpreterStack, 0, 0, 0);
#endif
}

void
CVMdumpAllThreads()
{
    CVMExecEnv* ee = CVMgetEE();
    
    CVMsysMutexLock(ee, &CVMglobals.threadLock);
    CVM_WALK_ALL_THREADS(ee, currentEE, {
	/*
	 * Ideally, we should be calling CVMdumpThread() here. But we
	 * are not allowed to allocate within CVM_WALK_ALL_THREADS(), so
	 * we call a non-allocating version of a thread dump that does not
	 * print the thread's name.
	 */
	CVMdumpThreadFromEENoAlloc(currentEE);
	CVMconsolePrintf("\n");
    });
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

void
CVMprintThreadName(JNIEnv* env, CVMObjectICell* threadICell)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    jchar *charArrayElements;
    jobject nameString;
    const char* name;

    /* mixes local roots and JNI */
    CVMID_localrootBegin(ee) {
	CVMID_localrootDeclare(CVMObjectICell, nameICell);

	CVMID_fieldReadRef(ee, threadICell,
			   CVMoffsetOfjava_lang_Thread_name,
			   nameICell);
	if (!CVMID_icellIsNull(nameICell)) {
	    charArrayElements =
		(*env)->GetCharArrayElements(env, (jcharArray) nameICell,
					     NULL);
	    nameString =
		(*env)->NewString(env, charArrayElements,
				  (*env)->GetArrayLength(env,
							 (jarray) nameICell));
	    name = (*env)->GetStringUTFChars(env, nameString, NULL);
	    CVMconsolePrintf("%s", name);
	    (*env)->ReleaseStringUTFChars(env, nameString, name);
	    (*env)->ReleaseCharArrayElements(env, (jcharArray) nameICell,
					     charArrayElements, JNI_ABORT);
	} else {
	    CVMconsolePrintf("<unnamed thread>");
	}
    } CVMID_localrootEnd();
}

#endif

/*
 * CVMclassname2String - copies the class name in src to dst, converting
 * each '/' to '.' in the process.
 */

char*
CVMclassname2String(CVMClassTypeID typeID, char *dst, int size)
{
    char *buf = dst;
    CVMtypeidClassNameToCString(typeID, dst, size);
    for (; (--size > 0) && (*dst != '\0') ;  dst++) {
	if (*dst == '/') {
	    *dst = '.';
	}
    }
    return buf;
}

/*
 * Returns the size of the utf8 char.
 */
static int
CVMutfCharSize(const char* utfstring) {
    switch ((CVMUint8)utfstring[0] >> 4) {
    default:
	return 1;
	
    case 0xC: case 0xD:	
	/* 110xxxxx  10xxxxxx */
	return 2;
	
    case 0xE:
	/* 1110xxxx 10xxxxxx 10xxxxxx */
	return 3;
    } /* end of switch */
}

/*
 * Returns # of unicode chars needed to represent the utf8 string
 */
CVMSize
CVMutfLength(const char* utf8string)
{
    CVMSize length;
    for (length = 0; *utf8string != 0; length++) {
       utf8string += CVMutfCharSize(utf8string);
    }
    return length;
}

/*
 * Copy a utf8 array into a counted unicode array.
 * Return number of UTF characters moved from the source.
 * This is to allow a user to re-start the copying at a known
 * stopping point.
 */
int
CVMutfCountedCopyIntoCharArray(
    const char * utf8Bytes,
    CVMJavaChar* unicodeChars,
    CVMSize      unicodeLength
){
    int nutf = 0;

    while( unicodeLength-- > 0 ){
	char utfchar = *utf8Bytes++;
	CVMJavaChar t;
	switch ( utfchar & 0xf0 ){
	default:
	    *unicodeChars++ = utfchar;
	    nutf+= 1;
	    continue;

	case 0xC0: case 0xD0:
	    /* 110xxxxx  10xxxxxx */
	    *unicodeChars++ = ((utfchar&0x1f)<<6) | ((*utf8Bytes++)&0x3f);
	    nutf+= 2;
	    continue;
	    
	case 0xE0:
	    /* 1110xxxx 10xxxxxx 10xxxxxx */
	    t = ((utfchar&0xf)<<6) | ((*utf8Bytes++)&0x3f);
	    *unicodeChars++ = (t<<6) | ((*utf8Bytes++)&0x3f);
	    nutf+= 3;
	    continue;
	} /* end of switch */
    }
    return nutf;
}

/*
 * Copy a utf8 array into a unicode array.
 */
void 
CVMutfCopyIntoCharArray(const char* utf8Bytes, CVMJavaChar* unicodeChars)
{
    int i = 0;
    while (*utf8Bytes != 0) {
	unicodeChars[i] = CVMutfNextUnicodeChar(&utf8Bytes);
	i++;
    }
}

/*
 * Copy a unicode array into a utf8 array.  Returns pointer to end of array.
 *
 * Code stolen from JDK unicode2utf().
 */
char *
CVMutfCopyFromCharArray(const CVMJavaChar* unicodeChars,
                        char* utf8Bytes, CVMInt32 count)
{
    while (--count >= 0) {
	CVMJavaChar ch = *unicodeChars++;
        if ((ch != 0) && (ch <=0x7f)) {
            *utf8Bytes++ = (char)ch;
        } else if (ch <= 0x7FF) {
            /* 11 bits or less. */
            CVMUint8 high_five = ch >> 6;
            CVMUint8 low_six = ch & 0x3F;
            *utf8Bytes++ = high_five | 0xC0; /* 110xxxxx */
            *utf8Bytes++ = low_six | 0x80;   /* 10xxxxxx */
        } else {
            /* possibly full 16 bits. */
            CVMInt8 high_four = ch >> 12;
            CVMInt8 mid_six = (ch >> 6) & 0x3F;
            CVMInt8 low_six = ch & 0x3f;
            *utf8Bytes++ = high_four | 0xE0; /* 1110xxxx */
            *utf8Bytes++ = mid_six | 0x80;   /* 10xxxxxx */
            *utf8Bytes++ = low_six | 0x80;   /* 10xxxxxx*/
        }
    }
    return utf8Bytes;
}

/*
 * Return the next unicode character in the utf8 string and update
 * utf8String_p to point after the unicode characater return.
 *
 * This is the same as the JDK next_utf2unicode function.
 */

extern CVMJavaChar
CVMutfNextUnicodeChar(const char** utf8String_p)
{
    CVMUint8* ptr = (CVMUint8*)(*utf8String_p);
    CVMUint8 ch, ch2, ch3;
    int length = 1;		/* default length */
    CVMJavaChar result = 0x80;	/* default bad result; */
    switch ((ch = ptr[0]) >> 4) {
        default:
	    result = ch;
	    break;

	case 0x8: case 0x9: case 0xA: case 0xB: case 0xF:
	    /* Shouldn't happen. */
	    break;

	case 0xC: case 0xD:	
	    /* 110xxxxx  10xxxxxx */
	    if (((ch2 = ptr[1]) & 0xC0) == 0x80) {
		unsigned char high_five = ch & 0x1F;
		unsigned char low_six = ch2 & 0x3F;
		result = (high_five << 6) + low_six;
		length = 2;
	    } 
	    break;

	case 0xE:
	    /* 1110xxxx 10xxxxxx 10xxxxxx */
	    if (((ch2 = ptr[1]) & 0xC0) == 0x80) {
		if (((ch3 = ptr[2]) & 0xC0) == 0x80) {
		    unsigned char high_four = ch & 0x0f;
		    unsigned char mid_six = ch2 & 0x3f;
		    unsigned char low_six = ch3 & 0x3f;
		    result = (((high_four << 6) + mid_six) << 6) + low_six;
		    length = 3;
		} else {
		    length = 2;
		}
	    }
	    break;
	} /* end of switch */

    *utf8String_p = (char *)(ptr + length);
    return result;
}

/*
 * Allocate a java/lang/String and copy the specifed utf8 string into it.
 */
void
CVMnewStringUTF(CVMExecEnv* ee, CVMStringICell* stringICell,
		const char* utf8Bytes)
{
#undef STACK_CHAR_BUF_SIZE
#define STACK_CHAR_BUF_SIZE 128
    CVMJavaChar  buf[STACK_CHAR_BUF_SIZE];
    CVMJavaChar* unicodeChars;
    CVMSize len = CVMutfLength(utf8Bytes);

    if (len > STACK_CHAR_BUF_SIZE) {
	unicodeChars = (CVMJavaChar *)malloc(len * sizeof(CVMJavaChar));
	if (unicodeChars == 0) {
	    CVMID_icellSetNull(stringICell);
	    return;
	}
    } else {
	unicodeChars = buf;
    }
    CVMutfCopyIntoCharArray(utf8Bytes, unicodeChars);
    CVMnewString(ee, stringICell, unicodeChars, (CVMUint32)len);
    if (unicodeChars != buf) {
	free(unicodeChars);
    }
}

/*
 * Allocate a java/lang/String and copied to specifed unicode string into it.
 */
void
CVMnewString(CVMExecEnv* ee, CVMStringICell* stringICell,
	     const CVMJavaChar* unicodeChars, CVMUint32 len)
{
    /* 
     * Allocate the java/lang/String object first, and store it away
     * in stringICell.
     */
    CVMID_allocNewInstance( 
	    ee, (CVMClassBlock*)CVMsystemClass(java_lang_String), stringICell);

    if (CVMID_icellIsNull(stringICell)) {
	goto finish;
    }

    {
	CVMArrayOfCharICell* charArrayICell =
	    (CVMArrayOfCharICell*)CVMmiscICell(ee);
	CVMassert(CVMID_icellIsNull(charArrayICell));

	/*
	 * Allocate the char array. 
	 */
	CVMID_allocNewArray(
		ee, CVM_T_CHAR, 
		(CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CHAR],
		len, (CVMObjectICell*)charArrayICell);

	if (CVMID_icellIsNull(charArrayICell)) {
	    /* null result if anything failed. */
	    CVMID_icellSetNull(stringICell);
	} else {
	    /*
	     * Fill in the char array.
	     */
	    CVMD_gcUnsafeExec(ee, {
		CVMArrayOfChar* charArray =
		    CVMID_icellDirect(ee, charArrayICell);
		CVMD_arrayWriteBodyChar(unicodeChars, charArray, 0, len);
	    });
	    /*
	     * Assign all the fields of the String object. Note that
	     * directObj may have moved during the array allocation,
	     * so we need to recache it. 
	     */
	    CVMID_fieldWriteRef(ee, stringICell, 
				CVMoffsetOfjava_lang_String_value,
				(CVMObjectICell*)charArrayICell);
	    CVMID_fieldWriteInt(ee, stringICell,
				CVMoffsetOfjava_lang_String_offset,
				0);
	    CVMID_fieldWriteInt(ee, stringICell, 
				CVMoffsetOfjava_lang_String_count,
				len);
	    /*
	     * Set this to null when we're done. The misc icell is
	     * needed elsewhere, and the nullness is used to detect
	     * errors.
	     */
	    CVMID_icellSetNull(charArrayICell);
	}
    }

 finish:
    CVMassert(!CVMlocalExceptionOccurred(ee));
}

/*
 * Build a formatted string.  Special formats include:
 *
 * code	     arg		      result
 *
 * %P    CVMFrame*	   The class name, method name, source file name
 *                         and line number of the pc in the frame.
 * %O    CVMObject*	   The class name and object hash.
 * %I    CVMObjectICell*   The class name and object hash.
 * %C    CVMClassBlock*	   CVMclassname2String(CVMcbClassName(cb))
 * %F	 CVMFieldBlock*	   CVMtypeidFieldNameToCString(CVMfbNameAndTypeID(fb))
 * %M	 CVMMethodBlock*   CVMtypeidMethodNameToCString(CVMmbNameAndTypeID(mb))
 *
 * -%C,%F, and %M support the '!' modifier if you want to pass the typeid
 *  rather than the cb, fb, or mb.
 * -Supports all ANSI formats except for %n.
 * -Does not support field modifiers on %C, %F, and %M fields.
 *
 * Returns total size of formatted result, untruncated.
 *
 * WARNING: This function is not bulletproof like sprintf. You must obey
 * the following rules to avoid a crash.
 * 1. No single expanded field, other than %C, %F, and %M fields, can
 *    expand bigger than bufSize. This means, for example, if you pass
 *    a format of "%20d" and a bufSize of 10, the buffer will still get
 *    20 bytes stored in, thus corrupting memory. It also means if you
 *    pass a format of "%s" with a string argument larger than bufsize,
 *    you will also corrupt memory.
 * 2. You cannot pass in a bufSize of 0 if you use %O, %C, %M, or %F.
 */

#undef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#ifdef CVM_CSTACKANALYSIS
/*
 * Instead of putting stack redzone check in CVMformatStringVaList,
 * a dummy recursive function is a place holder to put stack redzone
 check
 * for a one level deep recursion that only happens in a debug build
 * through CVMformatStringVaList->CVMpc2string->CVMformatString->
 * CVMformatStringVaList
 */
static CVMBool
CVMCstackDummy2CVMformatStringVaList(CVMExecEnv* ee, void *frame, char p,
                                     char *tmp, size_t bufSize)
{
    /* C stack checks */
    if (!CVMCstackCheckSize(
            ee, CVM_REDZONE_CVMCstackCVMpc2string,
	    "CVMformatStringVaList2CVMpc2string", CVM_TRUE))
    {
        return CVM_FALSE;
    }
    if (p == '!') {
	CVMframeIterate2string((CVMFrameIterator *)frame, tmp, tmp + bufSize);
    } else {
	CVMframe2string((CVMFrame *)frame, tmp, tmp + bufSize);
    }
    return CVM_TRUE;
}

static CVMBool
CVMCstackDummy2CVMconsolePrintf(CVMExecEnv* ee,
                                CVMObjectICell* indirectObj,
                                const CVMClassBlock* cb)
{
    /* C stack checks */
    if (!CVMCstackCheckSize( 
            ee, CVM_REDZONE_CVMCstackCVMID_objectGetClassAndHashSafe,
            "CVMformatStringVaList2CVMID_objectGetClassAndHashSafe", CVM_TRUE))
    {
        return CVM_FALSE;
    }
    CVMID_objectGetClass(ee, indirectObj, cb);
    return CVM_TRUE;
}

static CVMInt32
CVMCstackDummy2CVMobjectGetHashSafe(CVMExecEnv* ee,
                                    CVMObjectICell* indirectObj)
{
    /* C stack checks */
    if (!CVMCstackCheckSize(ee,
        CVM_REDZONE_CVMCstackCVMID_objectGetClassAndHashSafe,
        "CVMformatStringVaList2CVMGetHashSafe", CVM_TRUE)) {
        return 0;
    }
    return CVMobjectGetHashSafe(ee, indirectObj);
}
#endif /* CVM_CSTACKANALYSIS */

size_t
CVMformatStringVaList(char* buf, size_t bufSize, const char* format,
		      va_list ap)
{
    CVMExecEnv *ee = CVMgetEE();
    const char* f = format;
    const char* s;
    char* tmp;
    size_t      length  = 0;
    int         result = 0;
    char gbuf[256];
    char format2[17]; /* must be bigger than the biggest known format.
		       * "%-+ #<num>.<num>wc" is the biggest. If we
		       * limit <num> to a resonable 4 digits, this gives
		       * us a total size of 17.
		       */

    if (buf == NULL) {
	/* Just sizing result */
	tmp = NULL;
	bufSize = 0;
    } else {
	if (sizeof(gbuf) >= bufSize) {
	    tmp = gbuf;
	} else {
	    tmp = (char*)malloc(bufSize);
	    if (tmp == NULL) {
		char* errStr = "** CVMformatStringVaList: out of memory **";
		strncpy(buf, errStr, bufSize - 1);
		buf[strlen(buf)] = '\0';
		return 0;
	    }
	}
	/* save room for terminating \0 */
	bufSize -= 1;
    }

    while (*f != '\0') {
	char* p;
	char c;
	size_t l;

	p = strchr(f, '%');
	l = (p == NULL ? strlen(f) : p - f);

	/* copy everything up to the % */
	if (bufSize > 0) {
	    l = MIN(bufSize, l);
	    strncpy(buf, f, l);
	    buf += l;
	    bufSize -= l;
            length += l;
	}
	if (p == NULL) {
	    /* There are no more %'s, so we are done */
	    break;
	}

	/* skip modifier fields */
	f = p;  /* f points to the % */
	p++; /* skip the % */
	while ((c = *p) != 0) {
	    if (c == '%') {
		break;
	    }
	    /* alphabetic characters usually mark the end of the modifiers
	     * fields, except for a few alphabetic modifiers we suport.
	     */
	    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		CVMBool needBreak = CVM_TRUE;
		switch (c) {
		    case 'w':
		    case 'h':
		    case 'l':
		    case 'L': {
			needBreak = CVM_FALSE;
		    }
		}
		if (needBreak) {	
		    break;
		}
	    }
	    p++;
	}

	/* copy everything from the % char up to and including the field
	 * type char into format2. We need to do this since we don't
	 * want the sprintf calls to look past the field type char.
	 */
	l = p - f + 1;
	CVMassert(l + 1 <= sizeof(format2));
	strncpy(format2, f, l);
	format2[l] = 0;

	switch (c) {
	case 'd':
	case 'i':
	case 'o':
	case 'u':
	case 'x':
	case 'X': {
	    int i = va_arg(ap, int);
	    result = sprintf(tmp, format2, i);
	    goto copy_string;
	}
	/* 
	 * Scalar variables intended to hold memory dimensions
	 * have to be of the type CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms.
	 */
	case 'p': {
	    CVMAddr a = va_arg(ap, CVMAddr);
	    result = sprintf(tmp, format2, a);
	    goto copy_string;
	}
	case 'f':
	case 'e':
	case 'E':
	case 'g':
	case 'G': {
	    double f1 = va_arg(ap, double);
	    result = sprintf(tmp, format2, f1);
	    goto copy_string;
	}
	case 'c':  {
	    char c = va_arg(ap, int);
	    result = sprintf(tmp, format2, c);
	    goto copy_string;
	}
	case 's': {
	    const char* c = va_arg(ap, const char*);
	    /*
	     * If the format string is a simple "%s", then we just copy
	     * the argument string directly into buf and also check for
	     * overflow. Otherwise, we role the dice and let sprintf
	     * handle it. This means that if you ever use a format modifier
	     * with %s, you better make sure your buffer is big enough because
	     * we don't do any overflow checking here.
	     */
            if (c == NULL) {
                f = p + 1;
                continue;
            } else if (format2[1] == 's') {
		l = strlen(c);
		if (bufSize > 0) {
		    l = MIN(bufSize, l);
		    strncpy(buf, c, l);
		    buf += l;
		    bufSize -= l;
                    length += l;
		}
		f = p + 1;
		continue;
	    } else {
		result = sprintf(tmp, format2, c);
		goto copy_string;
	    }
	}
	case 'n':
	    CVMassert(CVM_FALSE);
	    break;
	case '%':
	    result = sprintf(tmp, format2);
	    goto copy_string;
	case 'P': {
	    /*
	     * This format is only valid for DEBUG builds.
	     */
#ifdef CVM_DEBUG
	    void*  frame = va_arg(ap, void*);
	    /* C stack checks */
#ifdef CVM_CSTACKANALYSIS
            if (!CVMCstackDummy2CVMformatStringVaList(ee, frame, p[-1],
						      tmp, bufSize)) {
#else
            if (!CVMCstackCheckSize(ee,
        	                    CVM_REDZONE_CVMCstackCVMpc2string,
        	                    "CVMformatStringVaList2CVMpc2string", 
				    CVM_TRUE)) {
#endif /* CVM_CSTACKANALYSIS */
		/* exception already thrown */ 
		CVMclearLocalException(ee);
   		result = sprintf(tmp, 
		    "<PC information unavailable due to C stack overflow>");
    	    } else {
#ifndef CVM_CSTACKANALYSIS
		if (p[-1] == '!') {
		    CVMframeIterate2string((CVMFrameIterator *)frame,
			tmp, tmp + bufSize);
		} else {
		    CVMframe2string((CVMFrame *)frame, tmp, tmp + bufSize);
		}
#endif /* !CVM_CSTACKANALYSIS */
		result = (int)strlen(tmp);
	    }
#else
	    (void)va_arg(ap, CVMFrame*);
	    result = sprintf(tmp, "<%%P Unavailable>");
#endif
	    goto copy_string;
	}
	case 'O': {
	    CVMObject*           directObj = va_arg(ap, CVMObject*);
	    CVMInt32 hashValue = 0;
	    const CVMClassBlock* cb = CVMobjectGetClass(directObj);
	    CVMClassTypeID       tid = CVMcbClassName(cb);
	    /* "12" is to leave room for the "@%#x" used for the
	     * object address. */
	    CVMclassname2String(tid, tmp, (int)(bufSize - 12));
	    l = strlen(tmp);
	    if (CVMD_isgcUnsafe(ee)) {
		hashValue = CVMobjectGetHashNoSet(ee, directObj);
	    } else {
		CVMD_gcUnsafeExec(ee, {
		    hashValue = CVMobjectGetHashNoSet(ee, directObj);
		});
	    }
	    result = sprintf(tmp+l, "@%#x", hashValue);
	    goto copy_string;
	}
	case 'I': {
	    CVMObjectICell*      indirectObj = va_arg(ap, CVMObjectICell*);
	    const CVMClassBlock* cb;
	    CVMClassTypeID       tid;

	    if (indirectObj == NULL || CVMID_icellIsNull(indirectObj)) {
		result = sprintf(tmp, "<null>");

#ifdef CVM_CSTACKANALYSIS
            } else {
               /* C stack checks for a one level deep recursion in
                 * both CVMID_objectGetClass and CVMobjectGetHashSafe */
                CVMInt32 value;
                if (!CVMCstackDummy2CVMconsolePrintf(ee, indirectObj,
                    cb)) {
                    /* exception already thrown */
                    CVMclearLocalException(ee);
                    result = sprintf(tmp,
                        "<class name unavailable due to C stack overflow>");
                    goto copy_string;
                }
                tid = CVMcbClassName(cb);
                /* "12" is to leave room for the "@%#x" used for the
                 * object address. */
                CVMclassname2String(tid, tmp, bufSize - 12);
                l = strlen(tmp);
                value = CVMCstackDummy2CVMobjectGetHashSafe(ee, indirectObj);
                if (value == 0)
                    result = sprintf(tmp, "<null>");
                else
                    result = sprintf(tmp+l, "@%#x", value);
            }  
#else
	    } else if (!CVMCstackCheckSize(ee,
                CVM_REDZONE_CVMCstackCVMID_objectGetClassAndHashSafe,
                "CVMformatStringVaList2CVMID_objectGetClassAndHashSafe",
                CVM_TRUE)) {
                /* C stack checks for a one level deep recursion in
	         * both CVMID_objectGetClass and CVMobjectGetHashSafe. 
                 * Exception is already thrown if fails. */
                CVMclearLocalException(ee);
   		result = sprintf(tmp, 
		    "<class name unavailable due to C stack overflow>");
    	    } else {
    	  	CVMID_objectGetClass(ee, indirectObj, cb);
		tid = CVMcbClassName(cb);
		/* "12" is to leave room for the "@%#x" used for the
		 * object address. */
		CVMclassname2String(tid, tmp, (int)(bufSize - 12));
		l = strlen(tmp);
		result = sprintf(tmp+l, "@%#x", 
		    		 CVMobjectGetHashSafe(ee, indirectObj));
	    }
#endif /* CVM_CSTACKANALYSIS */
	    goto copy_string;
	}
	case 'C': {
	    CVMClassTypeID       tid;
	    if (*(p-1) == '!') {
		tid = va_arg(ap, CVMClassTypeID);
	    } else {
		const CVMClassBlock* cb = va_arg(ap, const CVMClassBlock*);
		tid = CVMcbClassName(cb);
	    }
	    CVMclassname2String(tid, tmp, (int)bufSize);
	    goto copy_string;
	}
	case 'F': {
	    CVMFieldTypeID       tid;
	    if (*(p-1) == '!') {
		tid = va_arg(ap, CVMFieldTypeID);
	    } else {
		const CVMFieldBlock* fb = va_arg(ap, const CVMFieldBlock*);
		tid = CVMfbNameAndTypeID(fb);
	    }
	    CVMtypeidFieldNameToCString(tid, tmp, (int)bufSize);
	    l = strlen(tmp);
	    if (bufSize - l > 0) {
		tmp[l] = ':';
		l++;
	    }
	    CVMtypeidFieldTypeToCString(tid, tmp + l, (int)(bufSize - l));
	    goto copy_string;
	}
	case 'M': {
	    CVMMethodTypeID       tid;
	    if (*(p-1) == '!') {
		tid = va_arg(ap, CVMMethodTypeID);
	    } else {
		const CVMMethodBlock* mb = va_arg(ap, const CVMMethodBlock*);
		tid = CVMmbNameAndTypeID(mb);
	    }
	    CVMtypeidMethodNameToCString(tid, tmp, (int)bufSize);
	    l = strlen(tmp);
	    CVMtypeidMethodTypeToCString(tid, tmp + l, (int)(bufSize - l));
	    goto copy_string;
	}
	default:
	    CVMassert(CVM_FALSE);
	}

    copy_string:
	if (result < 0) {
	    goto finish;
	}
	s = tmp;
	l = strlen(s);
	if (bufSize > 0) {
	    l = MIN(bufSize, l);
	    strncpy(buf, s, l);
	    buf += l;
	    bufSize -= l;
            length += l;
	}
	f = p + 1;
    }

    if (buf != NULL) {
	*buf = '\0';
    }
   
 finish:
    if (tmp != NULL && tmp != gbuf) {
	free(tmp);
    }

    if (result < 0) {
	CVMassert(CVM_FALSE);
	return 0;
    } else {
	return length;
    }
}

size_t
CVMformatString(char *buf, size_t bufSize, const char *format, ...)
{
    size_t result;
    va_list ap;
    va_start(ap, format);
    result = CVMformatStringVaList(buf, bufSize, format, ap);
    va_end(ap);
    return result;
}

/*
 * Returns true if the two classes are in the same package. 
 */
CVMBool
CVMisSameClassPackage(CVMExecEnv* ee,
		      CVMClassBlock* class1, CVMClassBlock* class2) 
{
    /*
     * Determine if both classes have the same classloader.
     */
    if (CVMcbClassLoader(class1) != CVMcbClassLoader(class2)) {
	return CVM_FALSE; 
    } else {
	return CVMtypeidIsSameClassPackage(CVMcbClassName(class1),
					   CVMcbClassName(class2));
    }
}

/*
 * Convenience functions
 */

/* %comment c013 */

CVMClassBlock*
CVMgcSafeClassRef2ClassBlock(CVMExecEnv* ee, CVMClassICell *clazz)
{
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMAddr cbPtr;
    CVMID_fieldReadAddr(ee, clazz,
			CVMoffsetOfjava_lang_Class_classBlockPointer,
			cbPtr);
    return (CVMClassBlock*)cbPtr;
}

CVMClassBlock *
CVMgcUnsafeClassRef2ClassBlock(CVMExecEnv *ee, CVMClassICell *clazz)
{
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMAddr cbPtr;
    CVMObject *directClass = CVMID_icellDirect(ee, clazz);
    CVMD_fieldReadAddr(directClass,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       cbPtr);
    return (CVMClassBlock *)cbPtr;
}

CVMBool
CVMgcSafeJavaWrap(CVMExecEnv* ee, jvalue v,
		  CVMBasicType fromType,
		  CVMObjectICell* result)
{
    switch (fromType) {
    case CVM_T_BOOLEAN:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Boolean),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_Boolean_value,
			    v.z);
	return CVM_TRUE;
    case CVM_T_CHAR:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Character),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_Character_value,
			    v.c);
	return CVM_TRUE;
    case CVM_T_FLOAT:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Float),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteFloat(ee, result,
			      CVMoffsetOfjava_lang_Float_value,
			      v.f);
	return CVM_TRUE;
    case CVM_T_DOUBLE:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Double),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteDouble(ee, result,
			       CVMoffsetOfjava_lang_Double_value,
			       v.d);
	return CVM_TRUE;
    case CVM_T_BYTE:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Byte),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_Byte_value,
			    v.b);
	return CVM_TRUE;
    case CVM_T_SHORT:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Short),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_Short_value,
			    v.s);
	return CVM_TRUE;
    case CVM_T_INT:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Integer),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_Integer_value,
			    v.i);
	return CVM_TRUE;
    case CVM_T_LONG:
	/* NOTE: casting away const is safe */
	CVMID_allocNewInstance(ee,
			       (CVMClassBlock*)
			       CVMsystemClass(java_lang_Long),
			       result);
	if (CVMID_icellIsNull(result)) {
	    CVMthrowOutOfMemoryError(ee, "allocating Java wrapper object");
	    return CVM_FALSE;
	}
	CVMID_fieldWriteLong(ee, result,
			     CVMoffsetOfjava_lang_Long_value,
			     v.j);
	return CVM_TRUE;
    case CVM_T_VOID:
	CVMID_icellSetNull(result);
	return CVM_TRUE;
    default:
	CVMassert(CVM_FALSE);
	return CVM_FALSE;
    }
}

CVMBool
CVMgcSafeJavaUnwrap(CVMExecEnv* ee, CVMObjectICell* obj,
		    jvalue* v, CVMBasicType* toType,
		    CVMClassBlock* exceptionCb)
{
    CVMBool result;

    CVMD_gcUnsafeExec(ee, {
	CVMObjectICell* nonnullObj = CVMID_NonNullICellPtrFor(obj);
	/*
	 * obj may be NULL. Leave the job of throwing the exception to
	 * CVMgcUnsafeJavaUnwrap().
	 */
	result = CVMgcUnsafeJavaUnwrap(ee, CVMID_icellDirect(ee, nonnullObj),
				       v, toType, exceptionCb);
    });

    return result;
}

CVMBool
CVMgcUnsafeJavaUnwrap(CVMExecEnv* ee, CVMObject* obj,
		      jvalue* v, CVMBasicType* toType,
		      CVMClassBlock* exceptionCb)
{
    CVMClassBlock* objectClassBlock;

    if (obj == NULL) {
	if (exceptionCb == NULL) {
	    /* NullPointerException or IllegalArgumentException?  What
	       do most callers want? NOTE: I think I tested this
	       against the JDK and found that it throws
	       NullPointerException here even though the specification
	       for Method.invoke says that if the unwrapping
	       conversion fails it should throw IllegalArgumentException. */
	    CVMthrowNullPointerException(ee, "unwrapping Java value");
	} else {
	    CVMsignalError(ee, exceptionCb, "unwrapping Java value");
	}
	return CVM_FALSE;
    }

    objectClassBlock = CVMobjectGetClass(obj);

    /* %comment rt008 */

    if (objectClassBlock == CVMsystemClass(java_lang_Boolean)) {
	CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_Boolean_value, v->z);
	*toType = CVM_T_BOOLEAN;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Character)) {
	CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_Character_value, v->c);
	*toType = CVM_T_CHAR;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Float)) {
	CVMD_fieldReadFloat(obj, CVMoffsetOfjava_lang_Float_value, v->f);
	*toType = CVM_T_FLOAT;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Double)) {
	CVMD_fieldReadDouble(obj, CVMoffsetOfjava_lang_Double_value, v->d);
	*toType = CVM_T_DOUBLE;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Byte)) {
	CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_Byte_value, v->b);
	*toType = CVM_T_BYTE;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Short)) {
	CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_Short_value, v->s);
	*toType = CVM_T_SHORT;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Integer)) {
	CVMD_fieldReadInt(obj, CVMoffsetOfjava_lang_Integer_value, v->i);
	*toType = CVM_T_INT;
    } else if (objectClassBlock == CVMsystemClass(java_lang_Long)) {
	CVMD_fieldReadLong(obj, CVMoffsetOfjava_lang_Long_value, v->j);
	*toType = CVM_T_LONG;
    } else {
	if (exceptionCb == NULL) {
	    CVMthrowIllegalArgumentException(ee, "unwrapping Java value");
	} else {
	    CVMsignalError(ee, exceptionCb, "unwrapping Java value");
	}
	return CVM_FALSE;
    }
    return CVM_TRUE;

    /* %comment rt009 */

}

/* JDK compatibility */

int
jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    return (int)CVMformatStringVaList(str, count, fmt, args);
}

int
jio_snprintf(char *str, size_t count, const char *fmt, ...)
{
    va_list argList;
    size_t n;

    va_start(argList, fmt);
    n = CVMformatStringVaList(str, count, fmt, argList);
    va_end(argList);
    return (int)n;
}

int
jio_fprintf(FILE *fp, const char *fmt, ...)
{
    va_list argList;
    char *buf = NULL; 
    size_t n;
    CVMExecEnv *ee = CVMgetEE();

    CVMCstackGetBuffer(ee, buf); /* Lock EE-buffer */
    if (buf == NULL) {
	printf("<io information unavailable due to low memory>\n");
        return 0; 
    }

    va_start(argList, fmt);
    n = CVMformatStringVaList(buf, CVM_CSTACK_BUF_SIZE, fmt, argList);
    va_end(argList);
    /* We only output to stdout for now */
    printf("%s", buf);
    CVMCstackReleaseBuffer(ee); /* Unlock EE-buffer */
    return (int)n;
}

/*
 * Assign value 'val' to property 'key' in properties object 'props'.
 * Use method 'putID' to put the property in.
 *
 * return CVM_TRUE on success, CVM_FALSE on any failure.
 */
CVMBool
CVMputProp(JNIEnv* env, jmethodID putID, 
	   jobject props, const char* key, const char* val)
{
    if (val != NULL) {
        jstring jkey;
        jstring jval;
	jobject r;
        jkey = (*env)->NewStringUTF(env, key);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
        jval = (*env)->NewStringUTF(env, val);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
	r = (*env)->CallObjectMethod(env, props, putID, jkey, jval);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
        (*env)->DeleteLocalRef(env, jkey);
        (*env)->DeleteLocalRef(env, jval);
        (*env)->DeleteLocalRef(env, r);
    }
    return CVM_TRUE;
}

/*
 * Assign value 'val' to property 'key' in properties object 'props'.
 * Use method 'putID' to put the property in.
 *
 * Do the conversion of 'key' and 'value' using the platform specific
 * C string conversion.
 *
 * return CVM_TRUE on success, CVM_FALSE on any failure.
 */
CVMBool
CVMputPropForPlatformCString(JNIEnv* env, jmethodID putID, 
			     jobject props, const char* key, const char* val)
{
    if (val != NULL) {
        jstring jkey;
        jstring jval;
	jobject r;
        jkey = JNU_NewStringPlatform(env, key);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
        jval = JNU_NewStringPlatform(env, val);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
	r = (*env)->CallObjectMethod(env, props, putID, jkey, jval);
	if ((*env)->ExceptionOccurred(env)) return CVM_FALSE;
        (*env)->DeleteLocalRef(env, jkey);
        (*env)->DeleteLocalRef(env, jval);
        (*env)->DeleteLocalRef(env, r);
    }
    return CVM_TRUE;
}

/*
 * Parse a size for options such as -Xms and -Xss. Supports M, m, K, and k
 * as shorthand for megabytes and kilobytes.
 */
CVMInt32 
CVMoptionToInt32(const char* optionValueString)
{
    char* tmp = (char*)optionValueString;
    CVMInt32 n = strtol(tmp, &tmp, 10);

    switch (*tmp) {
    case 'M': case 'm':
	n *= 1024 * 1024;
	tmp++;
	break;
    case 'K': case 'k':
	n *= 1024;
	tmp++;
	break;
    case '\0':
	break;
    default:
	return -1;
    }

    if (*tmp == 0) {
	return n;
    } else {
	return -1;
    }
}

/*
 * This method parses a list of sub options which were
 * previously extracted from a VM command line option
 * "-X<subsystem>:<option1>,<option2>,...".  The string will
 * not contain the "-X<subsystem>:" part.  
 *
 * Separates and stores the sub options into an array.
 *
 * Also takes care of the option of appending options if they
 * already existed.
 *
 * Returns CVM_TRUE if successful, CVM_FALSE otherwise.
 */
CVMBool
CVMinitParsedSubOptions(CVMParsedSubOptions* opts, const char* subOptionsStr)
{
    char* subPtrBegin;
    char* subPtr;
    char* subStr;
    int i, numSub;

    /*
     * Count the number of entries in the subOptionsStr string, and allocate
     * a large enough array for it.
     *
     * Duplicate the string, so that we can corrupt it later on with
     * embedded '\0's.
     */
    if (subOptionsStr == NULL) {
	/*
	 * Nothing to do, parsing succeeds.
	 */
	return CVM_TRUE;
    }
    subPtrBegin = strdup(subOptionsStr);
    if (subPtrBegin == NULL) {
	goto outOfMemory;
    }
    numSub = 0;
    subPtr = subPtrBegin;
    while (subPtr != NULL) {
	numSub++;
	subPtr = strchr(subPtr, ',');
	if (subPtr != NULL) {
	    subPtr += 1;
	}
    }
    /* There are 'numSub' options */
    if (opts->options != NULL) {
	/* If there are extra options, append them to the existing ones */
	/* They will be processed left to right, so the new ones
	   will automatically override the old ones */
	CVMInt32 newNumOptions = opts->numOptions + numSub;
	char** oldOptions = opts->options;
	char** newOptions = (char **)calloc(newNumOptions, 
					    sizeof(const char *));
	if (newOptions == NULL) {
	    /* Out of memory */
	    free(subPtrBegin);
	    goto outOfMemory;
	} else {
	    /* Copy the old options */
	    memmove(newOptions, oldOptions, 
		    opts->numOptions * sizeof(const char *));
	    free(oldOptions);
	}
	/* Mark the starting point as the last of the old options */
	i = opts->numOptions;
	/* Now override */
	opts->options = newOptions;
	opts->numOptions = newNumOptions;
    } else {
	opts->numOptions = numSub;
	opts->options = (char **)calloc(numSub, sizeof(const char *));
	if (opts->options == NULL) {
	    /* Out of memory */
	    free(subPtrBegin);
	    goto outOfMemory;
	}
	/* Start at the beginning */
	i = 0;
    }
    /* Now that we've counted, rewind, and get the options */
    subPtr = subPtrBegin;
    while (subPtr != NULL) {
	subStr = subPtr;
	subPtr = strchr(subPtr, ',');
	if (subPtr != NULL) {
	    *subPtr = '\0';
	    subPtr += 1;
	}
	opts->options[i++] = subStr;
    }
    CVMassert(i == opts->numOptions);
    return CVM_TRUE;

 outOfMemory:
    CVMconsolePrintf("Out of memory while parsing options.\n");
    return CVM_FALSE;
}

void
CVMdestroyParsedSubOptions(CVMParsedSubOptions* opts)
{
    if (opts->numOptions > 0) {
	CVMassert(opts->options != NULL);
	CVMassert(opts->options[0] != NULL);
	/* The first option points to the beginning of the strdup()'ed
	   options string. Free it. */
	free(opts->options[0]);
	/* If we appended any, we have freed the old version */

	/* And now the set of options we allocated */
	free(opts->options);
#ifdef CVM_DEBUG
	opts->numOptions = 0;
	opts->options = NULL;
#endif
    }
}

/*
 * Get sub option named 'subOptionName'.
 * If 'subOptionName' is of the form 'subOptionName=val', return "val". 
 * If 'subOptionName' is a flag instead, just return subOptionName.
 * Return NULL if not found.
 */
const char *
CVMgetParsedSubOption(const CVMParsedSubOptions* subOptions,
		      const char* subOptionName)
{
    int i;
    size_t len = strlen(subOptionName);
    /* Start from right to left to get the last mentioned option */
    for (i = subOptions->numOptions - 1; i >= 0; i--) {
	char* option = subOptions->options[i];
	if (!strncmp(option, subOptionName, len)) {
	    /* Name prefix matches. Return the value */
	    option += len;
	    if (*option == '\0') {
		return subOptionName;
	    } else if (*option == '=') {
		return option + 1;
	    }
	}
    }
    return NULL;
}

/*
 * Takes all of the parsed suboptions and uses them to populate the
 * what is pointed to by the valuePtr of each known suboption. Uses
 * the suboption defaultValue if no parsed suboption is provided.
 * Does range checking of the parsed suboption using the min and max values.
 */
CVMBool
CVMprocessSubOptions(const CVMSubOptionData* knownSubOptions,
		     const char* optionName,
		     CVMParsedSubOptions *parsedSubOptions)
{
    int i = 0;
    
    /* First verify that all the parsedSubOptions are legal options. */
    for (i = 0; i < parsedSubOptions->numOptions; i++) {
	const char* option = parsedSubOptions->options[i];
	CVMBool isValid = CVM_FALSE;
	int j = 0;
	while (knownSubOptions[j].name != NULL) {
	    size_t len = strlen(knownSubOptions[j].name);
	    if (!strncmp(option, knownSubOptions[j].name, len)) {
		/* Name matches. Verify garbage doesn't trail the name */
		if (option[len] == '\0' || option[len] == '=') {
		    isValid = CVM_TRUE;
		    break;
		}
	    }
	    j++;
	}
	if (!isValid) {
	    CVMconsolePrintf("Illegal %s option: \"%s\"\n",
			     optionName, option);
	    return CVM_FALSE;
	}
    }

    /* Now get the value for each known option from the parsedSubOptions. */
    i = 0;
    while (knownSubOptions[i].name != NULL) {
	CVMBool invalidOption = CVM_FALSE;
	const char* valueString =
	    CVMgetParsedSubOption(parsedSubOptions, knownSubOptions[i].name);
	switch (knownSubOptions[i].kind) {
	case CVM_BOOLEAN_OPTION: {
	    CVMBool value;
	    if (valueString == NULL) {
		value = knownSubOptions[i].data.intData.defaultValue;
	    } else {
		if (!strcmp(valueString, "true")) {
		    value = CVM_TRUE;
		} else if (!strcmp(valueString, "false")) {
		    value = CVM_FALSE;
		} else {
		    invalidOption = CVM_TRUE;
		    break;
		}
	    }
	    *(CVMBool*)knownSubOptions[i].valuePtr = value;
	    break;
	}
	case CVM_INTEGER_OPTION: {
	    CVMInt32 value;
	    if (valueString == NULL) {
		value = knownSubOptions[i].data.intData.defaultValue;
	    } else {
		value = CVMoptionToInt32(valueString);
		if (value < 0 ||
		    value < knownSubOptions[i].data.intData.minValue ||
		    value > knownSubOptions[i].data.intData.maxValue)
	        {
		    invalidOption = CVM_TRUE;
		    break;
		}
	    }
	    *(CVMInt32*)knownSubOptions[i].valuePtr = value;
	    break;
	}
	case CVM_PERCENT_OPTION: {
	    CVMInt32 value;
	    if (valueString == NULL) {
		value = knownSubOptions[i].data.intData.defaultValue;
	    } else {
		char* tmp = (char*)valueString;
		value = strtol(valueString, &tmp, 10);
		if (*tmp != '%' || strlen(tmp) != 1) {
		    invalidOption = CVM_TRUE;
		    break;
		}
		if (value < knownSubOptions[i].data.intData.minValue ||
		    value > knownSubOptions[i].data.intData.maxValue)
		{
		    invalidOption = CVM_TRUE;
		    break;
		}
	    }
	    *(CVMInt32*)knownSubOptions[i].valuePtr = value;
	    break;
	}
	case CVM_STRING_OPTION: {
	    /* Don't worry about strdup failures. We will quickly fail
	     * on some other allocation later on if stdrup can't succeed.
	     */
	    if (valueString == NULL) {
		*(const char**)knownSubOptions[i].valuePtr = 
		    knownSubOptions[i].data.strData.defaultValue;
	    } else {
		*(char**)knownSubOptions[i].valuePtr = strdup(valueString);
	    }
	    break;
	}
	case CVM_MULTI_STRING_OPTION: {
	    CVMInt32 value;
	    if (valueString == NULL) {
		value = knownSubOptions[i].data.multiStrData.defaultValue;
	    } else {
		int limit =
		    knownSubOptions[i].data.multiStrData.numPossibleValues;
		const char **vals =
		    knownSubOptions[i].data.multiStrData.possibleValues;
		for (value = 0; value < limit; value++) {
		    if (!strcmp(valueString, vals[value])) {
			break;
		    }
		}
		if (value == limit) {
		    invalidOption = CVM_TRUE;
		    break;
		}
	    }
	    *(CVMInt32*)knownSubOptions[i].valuePtr = value;
	    break;
	}

        case CVM_ENUM_OPTION: {
            CVMUint32 value = 0;
	    if (valueString == NULL) {
		value = knownSubOptions[i].data.multiStrData.defaultValue;
	    } else {
		char token = valueString[0];
		const CVMSubOptionEnumData *enums =
		    knownSubOptions[i].data.enumData.possibleValues;
		int numberOfEnums =
		    knownSubOptions[i].data.enumData.numPossibleValues;

		if (token == '+' || token == '-') {
		    value = knownSubOptions[i].data.enumData.defaultValue;
		    valueString++;
		} else {
		    /* The specified enums will override the default.  So, we
		       start with a value of 0, and the first enum value will
		       be added to it: */
		    token = '+';
		}

		while (!invalidOption && (valueString[0] != '\0')) {
		    const char *nextString = valueString;
		    /* Find the location of the next delimiter */
		    char nextToken;
		    int length = 0;
		    int i;

		    nextToken = nextString[0];
		    while (nextToken != '+' && nextToken != '-' &&
			   nextToken != '\0') {
			length++;
			nextString++;
			nextToken = nextString[0];
		    }

		    /* See if this matches an enum value: */
		    for (i = 0; i < numberOfEnums; i++) {
			if (strncmp(valueString, enums[i].name, length) == 0) {
			    CVMUint32 enumValue = enums[i].value;
			    if (token == '+') {
				value |= enumValue;
			    } else {
				CVMassert(token == '-');
				value &= ~enumValue;
			    }
			    break;
			}
		    }
		    if (i == numberOfEnums) {
			invalidOption = CVM_TRUE;
			break;
		    }

		    token = nextToken;
		    valueString = nextString;
		    if (token != '\0') {
			valueString++; /* Skip the token. */
		    }
		}
	    }
            *(CVMInt32*)knownSubOptions[i].valuePtr = value;
	    break;
        }

	default: CVMassert(CVM_FALSE);
	}
	if (invalidOption) {
	    /*
	     * If the strings are the same then that means the =<value>
	     * part was missing.
	     */
	    if (!strcmp(knownSubOptions[i].name, valueString)) {
		valueString = "";
	    }
	    CVMconsolePrintf("Illegal value for option \"%s\": \"%s\"\n",
			     knownSubOptions[i].name, valueString);
	    return CVM_FALSE;
	}
	i++;
    }
    return CVM_TRUE;
}

/*
 * Print the value of each suboption in the table. Used after calling
 * CVMprocessSubOptions() to print the "configuration".
 */
void
CVMprintSubOptionValues(const CVMSubOptionData* knownSubOptions)
{
    int i = 0;
    while (knownSubOptions[i].name != NULL) {
	CVMconsolePrintf("\t%s (%s): ",
			 knownSubOptions[i].description,
			 knownSubOptions[i].name);
	switch (knownSubOptions[i].kind) {
	case CVM_BOOLEAN_OPTION: {
	    CVMBool value = *(CVMBool*)knownSubOptions[i].valuePtr;
	    CVMconsolePrintf(value ? "true\n" : "false\n");
	    break;
	}
	case CVM_INTEGER_OPTION: {
	    CVMInt32 value = *(CVMInt32*)knownSubOptions[i].valuePtr;
	    CVMconsolePrintf("%d\n", value);
	    break;
	}
	case CVM_PERCENT_OPTION: {
	    CVMInt32 value = *(CVMInt32*)knownSubOptions[i].valuePtr;
	    CVMconsolePrintf("%d%%\n", value);
	    break;
	}
	case CVM_STRING_OPTION: {
	    const char* value = *(const char**)knownSubOptions[i].valuePtr;
	    if (value == NULL) {
		value = "<unset>";
	    }
	    CVMconsolePrintf("%s\n", value);
	    break;
	}
	case CVM_MULTI_STRING_OPTION: {
	    CVMInt32 value = *(CVMInt32*)knownSubOptions[i].valuePtr;
	    const char* valueString =
		knownSubOptions[i].data.multiStrData.possibleValues[value];
	    CVMconsolePrintf("%s\n", valueString);
	    break;
	}
        case CVM_ENUM_OPTION: {
            CVMUint32 value = *(CVMInt32*)knownSubOptions[i].valuePtr;
            const CVMSubOptionEnumData *enums =
                knownSubOptions[i].data.enumData.possibleValues;
            int numberOfEnums =
                knownSubOptions[i].data.enumData.numPossibleValues;
            int i;
            CVMBool atLeastOneValuePrinted = CVM_FALSE;
            for (i = 0; i < numberOfEnums; i++) {
                if ((enums[i].value & value) == enums[i].value &&
                    /* Eliminate 'none', 'default', and 'all': */
                    (enums[i].value != 0) &&
                    (strcmp(enums[i].name, "default") != 0) &&
                    (strcmp(enums[i].name, "all") != 0)) {
                    if (atLeastOneValuePrinted) {
                        CVMconsolePrintf("+");
                    }
                    CVMconsolePrintf("%s", enums[i].name);
                    atLeastOneValuePrinted = CVM_TRUE;
                }
            }
            if (!atLeastOneValuePrinted) {
                CVMconsolePrintf("none");
            }
            CVMconsolePrintf("\n");
	    break;
        }
	default: CVMassert(CVM_FALSE);
	}
	i++;
    }
}

/*
 * Prints a usage string by using the data in the suboption table.
 */
void
CVMprintSubOptionsUsageString(const CVMSubOptionData* knownSubOptions)
{
    int i = 0;
    while (knownSubOptions[i].name != NULL) {
	CVMconsolePrintf("\t%s=", knownSubOptions[i].name);
	switch (knownSubOptions[i].kind) {
	case CVM_BOOLEAN_OPTION: {
	    CVMconsolePrintf("{true|false}");
	    break;
	}
	case CVM_INTEGER_OPTION: {
	    CVMconsolePrintf("<%d..%d>",
			     knownSubOptions[i].data.intData.minValue,
			     knownSubOptions[i].data.intData.maxValue);
	    break;
	}
	case CVM_PERCENT_OPTION: {
	    CVMconsolePrintf("<%d%%..%d%%>",
			     knownSubOptions[i].data.intData.minValue,
			     knownSubOptions[i].data.intData.maxValue);
	    break;
	}
	case CVM_STRING_OPTION: {
	    CVMconsolePrintf(knownSubOptions[i].data.strData.helpSyntax);
	    break;
	}
	case CVM_MULTI_STRING_OPTION: {
	    int k;
	    int limit = knownSubOptions[i].data.multiStrData.numPossibleValues;

	    CVMconsolePrintf("{");
	    
	    for(k = 0; k < limit; k++) {
		if (k != 0) {
		    CVMconsolePrintf("|");
		}
		CVMconsolePrintf("%s", 
                    knownSubOptions[i].data.multiStrData.possibleValues[k]);
	    }
	    CVMconsolePrintf("}");
	    
	    break;
	}
        case CVM_ENUM_OPTION: {
            const CVMSubOptionEnumData *enums =
                knownSubOptions[i].data.enumData.possibleValues;
            int numberOfEnums =
                knownSubOptions[i].data.enumData.numPossibleValues;
            int k;

            CVMconsolePrintf("{{+|-}{");
            for(k = 0; k < numberOfEnums; k++) {
                if (k != 0) {
                    CVMconsolePrintf("|");
                }
                CVMconsolePrintf("%s", enums[k].name);
            }
            CVMconsolePrintf("}}");
            break;
        }
	default: CVMassert(CVM_FALSE);
	}
	CVMconsolePrintf(" - %s\n", knownSubOptions[i].description);
	i++;
    }
}

/*
 * Given the address of the CVMProperties area, initialize
 * our classpath, loadable library directory and
 * java_home values.
 */
CVMBool
CVMinitPathValues(void *propsPtr, CVMpathInfo *pathInfo,
                  char **userBootclasspath)
{
    char *p, *p0, *pEnd;
    int i;
    size_t size, len, libPathLen;
    CVMProperties *props = (CVMProperties *)propsPtr;
    static const char *const jarNames[] = { CVM_JARFILES NULL };
    char *basePath;
    char *libPath;
    char *dllPath;
    char *preBootclasspath;
    char *postBootclasspath;

    /* Sanity check */
    CVMassert(propsPtr != NULL && pathInfo->basePath != NULL &&
              pathInfo->libPath != NULL && pathInfo->dllPath != NULL);

    basePath = pathInfo->basePath;
    libPath = pathInfo->libPath;
    dllPath = pathInfo->dllPath;
    preBootclasspath = pathInfo->preBootclasspath;
    postBootclasspath = pathInfo->postBootclasspath;

    /* Determine how much space to allocate to hold the different
     * paths we want to record. We will store the values for
     * java_home, the path to the dynamic library directory, the
     * contents of the classpath, and then path to any ext_dir
     * value.
     */
    len = strlen(basePath);
    /* Enough space for java_home, including a string terminator */
    size = len + 1;
    /* Space for dll_dir, including a string terminator */
    size += strlen(dllPath) + 1;
    /* Now add space for the entries in the sysclasspath */
    if (preBootclasspath != NULL) {
        size += strlen(preBootclasspath) + 1;
    }
    if (postBootclasspath != NULL) {
        size += strlen(postBootclasspath) + 1;
    }
    if (*userBootclasspath != NULL) {
        /* user has set bootclasspath use it instead of sysclasspath */
        size += strlen(*userBootclasspath);
    } else {
        libPathLen = strlen(libPath);
        for (i = 0; jarNames[ i ] != NULL; i++) {
            /* If in the midst of the path, add a path delimiter */
            if (i > 0 ) {
                size += strlen(CVM_PATH_CLASSPATH_SEPARATOR);
            }
            /* Space for the next entry, including two path delimiters */
            size += libPathLen + sizeof(char) + strlen(jarNames[i]);
        }
    }
    /* Now the final terminator for the sysclasspath. */
    size++;
#ifdef CVM_HAS_JCE
    /*
     * If this is the optional security package build with
     * JCE support, we need to place the sunjce_provider.jar
     * file onto the ext_dirs path. This is another path entry
     * (including two path separators) and terminator.
     */
    size += strlen(libPath) + sizeof(char) + strlen("ext") + 1;
#endif /* CVM_HAS_JCE */
    
    p0 = p = (char *)malloc(size);
    if (p == NULL) {
        return CVM_FALSE;
    }
    memset(p, 0, size);
    memset(props, 0, sizeof (CVMProperties));
    /* Record java_home */
    strcpy(p, basePath);
    props->java_home = p;
    p += strlen(p) + 1;

    /* Record path to native libraries */
    strcpy(p, dllPath);
    props->dll_dir = p;
    props->library_path = p;

    p += strlen(p) + 1;
    /* Record boot class path */
    *p = '\0';
    if (*userBootclasspath == NULL) {
        if (preBootclasspath != NULL) {
            strcat(p, preBootclasspath);
            if (jarNames[0] != NULL) {
                /* at least one jar so add separator */
                strcat(p, CVM_PATH_CLASSPATH_SEPARATOR);
            }
        }
        for (i = 0; jarNames[i] != NULL; i++) {
            if (i > 0) {
                strcat(p, CVM_PATH_CLASSPATH_SEPARATOR);
            }
            strcat(p, libPath);
            pEnd = p+strlen(p);
            *pEnd++ = CVM_PATH_LOCAL_DIR_SEPARATOR;
            strcpy(pEnd, jarNames[i]);
        }
        if (postBootclasspath != NULL) {
            if (i > 0 || preBootclasspath != NULL) {
                /* had at least one path so add separator */
                strcat(p, CVM_PATH_CLASSPATH_SEPARATOR);
            }
            strcat(p, postBootclasspath);
        }
    } else {
        /* user has provided a bootclasspath so use it */
        if (preBootclasspath != NULL) {
            strcat(p, preBootclasspath);
            strcat(p, CVM_PATH_CLASSPATH_SEPARATOR);
            pEnd = p+strlen(p);
        }
        strcat(p, *userBootclasspath);
        if (postBootclasspath != NULL) {
            strcat(p, CVM_PATH_CLASSPATH_SEPARATOR);
            strcat(p, postBootclasspath);
        }
    }
    if (*p == '\0' && *userBootclasspath == NULL
        && preBootclasspath == NULL && postBootclasspath == NULL)
    {
        props->sysclasspath = NULL;
    } else {
        props->sysclasspath = p;
        /* return a copy back to caller */
        *userBootclasspath = strdup(props->sysclasspath);
    }
    p += strlen(p) + 1;
#ifdef CVM_HAS_JCE
    /*
     * For the security optional package, set the ext_dirs
     * value to "lib/ext" so that we can see the sunjce_provider.jar
     * jarfile.
     */
    strcpy(p, libPath);
    pEnd = p+strlen(p);
    *pEnd++ = CVM_PATH_LOCAL_DIR_SEPARATOR;
    strcpy(pEnd, "ext");
    props->ext_dirs = p;
    CVMassert((pEnd + 4) == (p + strlen(p) + 1));
    p = pEnd + 4;
#else
    props->ext_dirs = "";
#endif /* CVM_HAS_JCE */
    CVMassert(p - p0 <= size);
    return CVM_TRUE;
}

void
CVMdestroyPathValues(void *propsPtr)
{
    CVMProperties *props = (CVMProperties *)propsPtr;
    if (props != NULL) {
        free((char *)props->java_home);
    }
    return;
} 

void CVMdestroyPathInfo(CVMpathInfo *pathInfo)
{
    if (pathInfo->basePath != NULL) {
        free(pathInfo->basePath);
    }
    if (pathInfo->dllPath != NULL) {
        free(pathInfo->dllPath);
    }
    if (pathInfo->libPath != NULL) {
        free(pathInfo->libPath);
    }
    if (pathInfo->preBootclasspath != NULL) {
        free(pathInfo->preBootclasspath);
    }
    if (pathInfo->postBootclasspath != NULL) {
        free(pathInfo->postBootclasspath);
    }
}


/*
 * GC-unsafe conversion of a Java string into a C string
 */
char*
CVMconvertJavaStringToCString(CVMExecEnv* ee, jobject stringobj)
{
    CVMObject* strDirect;
    CVMObject* charObj;
    CVMInt32   offset;
    CVMInt32   length;
    CVMArrayOfChar* theChars;
    char*      buffer;
    int        i;

    strDirect = CVMID_icellDirect(ee, stringobj);

#ifdef JAVASE
    /* FIXME - investigate the need for this. Supposedly SE will crash
       if NULL is returned. This fix is just a hack.
    */
    if (strDirect == NULL) { 
	buffer = malloc(16);
	*buffer = '\0';
        return buffer;
    }
#endif

    CVMD_fieldReadInt(strDirect, CVMoffsetOfjava_lang_String_offset, offset);
    CVMD_fieldReadInt(strDirect, CVMoffsetOfjava_lang_String_count, length);
    CVMD_fieldReadRef(strDirect, CVMoffsetOfjava_lang_String_value, charObj);
    theChars = (CVMArrayOfChar*)charObj;

    buffer = calloc(1, length + 1);

    if (buffer == NULL) {
	return NULL;
    }

    for (i = 0; i < length; i++) {
        CVMJavaChar unicode = theChars->elems[i+offset];
        if (unicode <= 0x007f) {
            buffer[i] = unicode;
        } else {
	    free(buffer);
	    return NULL;
        }
    }

    buffer[length] = 0;
    return buffer;
}
