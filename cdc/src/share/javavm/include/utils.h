/*
 * @(#)utils.h	1.96 06/10/10
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


/*
 * Header file for various utility functions and macros.
 */

#ifndef CVM_UTILS_H
#define CVM_UTILS_H

#include "javavm/include/defs.h"
#include "javavm/include/clib.h"
#include "javavm/include/typeid.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/export/jni.h"
#include "generated/cni/sun_misc_CVM.h"

/*
 * Console I/O. Same as printf, but supports special formats like
 * %C, %P, and %F. See CVMformatStringVaList for details.
 */
void CVMconsolePrintf(const char* format, ...);

/*
 * Dump an exception using CVMconsolePrintf. Useful for failures during
 * vm startup and for debugging in gdb.
 */
void
CVMdumpException(CVMExecEnv* ee);

/*
 * Functions for accessing the debugging flags. All of the following functions
 * return the previous state of the flag.
 *
 * You can pass in more than one flag at a time to any of the functions.
 */

#define CVM_DEBUGFLAG(flag) (sun_misc_CVM_DEBUGFLAG_##flag)

#ifdef CVM_TRACE_ENABLED

extern CVMInt32
CVMcheckDebugFlags(CVMInt32 flags);
#define CVMcheckDebugFlags(flags)  (CVMglobals.debugFlags & (flags))

extern CVMInt32
CVMsetDebugFlags(CVMInt32 flags);

extern CVMInt32
CVMclearDebugFlags(CVMInt32 flags);

extern CVMInt32
CVMrestoreDebugFlags(CVMInt32 flags, CVMInt32 oldvalue);

#endif /* CVM_TRACE_ENABLED */

/*
 * Debug I/O
 */
#ifdef CVM_DEBUG
#define CVMdebugPrintf(x) CVMconsolePrintf x
#define CVMdebugExec(x)   x
#else
#define CVMdebugPrintf(x)
#define CVMdebugExec(x)
#endif /* CVM_DEBUG */

/*
 * Trace I/O
 */
#ifdef CVM_TRACE
#define CVMtrace(flags, x)   \
    (CVMcheckDebugFlags(flags) != 0 ? CVMconsolePrintf x : (void)0)
#define CVMtraceExec(flags, x) \
    if (CVMcheckDebugFlags(flags) != 0) x
#define CVMeeTrace(flags, x)   \
    (((ee)->debugFlags & (flags)) != 0 ? CVMconsolePrintf x : (void)0)
#else
#define CVMtrace(flags, x)
#define CVMtraceExec(flags, x)
#define CVMeeTrace(flags, x)
#endif /* CVM_TRACE */

#define CVMtraceOpcode(x) 	CVMeeTrace(CVM_DEBUGFLAG(TRACE_OPCODE), x)
#define CVMtraceStatus(x) 	CVMtrace(CVM_DEBUGFLAG(TRACE_STATUS), x)
#define CVMtraceFastLock(x) 	CVMeeTrace(CVM_DEBUGFLAG(TRACE_FASTLOCK), x)
#define CVMtraceDetLock(x)   	CVMeeTrace(CVM_DEBUGFLAG(TRACE_DETLOCK), x)
#define CVMtraceSysMutex(x)    	CVMeeTrace(CVM_DEBUGFLAG(TRACE_MUTEX), x)
#define CVMtraceCS(x)        	CVMtrace(CVM_DEBUGFLAG(TRACE_CS), x)
#define CVMtraceGcStartStop(x)  CVMtrace(CVM_DEBUGFLAG(TRACE_GCSTARTSTOP) | \
					 CVM_DEBUGFLAG(TRACE_GCSCAN), x)
#define CVMtraceGcScan(x)    	CVMtrace(CVM_DEBUGFLAG(TRACE_GCSCAN), x)
#define CVMtraceGcAlloc(x)     	CVMtrace(CVM_DEBUGFLAG(TRACE_GCALLOC), x)
#define CVMtraceGcCollect(x)   	CVMtrace(CVM_DEBUGFLAG(TRACE_GCCOLLECT), x)
#define CVMtraceGcSafety(x)   	CVMtrace(CVM_DEBUGFLAG(TRACE_GCSAFETY), x)
#define CVMtraceClinit(x)   	CVMtrace(CVM_DEBUGFLAG(TRACE_CLINIT), x)
#define CVMtraceExceptions(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_EXCEPTIONS), x)
#define CVMtraceMisc(x)		CVMtrace(CVM_DEBUGFLAG(TRACE_MISC), x)
#define CVMtraceBarriers(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_BARRIERS), x)
#define CVMtraceStackmaps(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_STACKMAPS), x)
#define CVMtraceClassLoading(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_CLASSLOADING), x)
#define CVMtraceClassLookup(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_CLASSLOOKUP), x)
#define CVMtraceTypeID(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_TYPEID), x)
#define CVMtraceVerifier(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_VERIFIER), x)
#define CVMtraceWeakrefs(x)	CVMtrace(CVM_DEBUGFLAG(TRACE_WEAKREFS), x)
#define CVMtraceClassUnloading(x) CVMtrace(CVM_DEBUGFLAG(TRACE_CLASSUNLOAD), x)
#define CVMtraceClassLink(x)    CVMtrace(CVM_DEBUGFLAG(TRACE_CLASSLINK), x)
#define CVMtraceJVMTI(x)        CVMtrace(CVM_DEBUGFLAG(TRACE_JVMTI), x)

#ifdef CVM_LVM /* %begin lvm */
#define CVMtraceLVM(x)          CVMtrace(CVM_DEBUGFLAG(TRACE_LVM), x)
#endif /* %end lvm */

/*
 * Long utility routines:
 */

#if defined(CVM_DEBUG) || defined(CVM_TRACE_ENABLED)
extern void CVMlong2String(CVMInt64, char*, char*);
#endif

/*
 * Java class utility routines:
 */

#ifdef CVM_DEBUG
void
CVMdumpThread(JNIEnv* env);

/* Prints the thread name with NO additional characters to stdout.
   Prints "<unnamed thread>" if thread has NULL name. */
void
CVMprintThreadName(JNIEnv* env, CVMObjectICell* threadICell);
#endif

extern char*
CVMclassname2String(CVMClassTypeID classTypeID, char *dst, int size);

/*
 * UTF8/Unicode
 */
/*
 * Number of Unicode characters represented by the null-terminated,
 * UTF8-encoded C string
 */
extern CVMSize 
CVMutfLength(const char *);

/*
 * counted copy from UTF8 string to unicode array.
 * count is number of Unicode characters to copy.
 * returned value is number of UTF8 characters consumed.
 */
extern int
CVMutfCountedCopyIntoCharArray(
    const char * utf8Bytes, CVMJavaChar* unicodeChars, CVMSize  unicodeLength );

extern void 
CVMutfCopyIntoCharArray(const char* utf8Bytes, CVMJavaChar* unicodeChars);

extern char* 
CVMutfCopyFromCharArray(const CVMJavaChar* unicodeChars,
			char* utf8Bytes, CVMInt32 count);
extern CVMJavaChar
CVMutfNextUnicodeChar(const char** utf8String_p);

#define CVMunicodeChar2UtfLength(ch)				\
        (((ch != 0) && (ch <= 0x7f)) ? 1 /* 1 byte */		\
        : (ch <= 0x7FF) ? 2		 /* 2 byte character */	\
        : 3)				 /* 3 byte character */


/*
 * Format a string.  Implements VM-specific formats plus a subset of
 * snprintf formats.  See utils.c for details.
 */

size_t
CVMformatString(char *buf, size_t bufSize, const char *format, ...);

size_t
CVMformatStringVaList(char *buf, size_t bufSize,
    const char *format, va_list ap);

/*
 * Java String utils
 */
extern void
CVMnewStringUTF(CVMExecEnv* ee, CVMStringICell* resultICell,
		const char* utf8Bytes);
extern void
CVMnewString(CVMExecEnv* ee, CVMStringICell* resultICell,
	     const CVMJavaChar* unicodeChars, CVMUint32 len);

/*
 * Alignment
 */
/*
 * CVMalignDoubleWordUp(val), CVMalignWordUp(val), 
 * CVMalignDoubleWordDown(val), CVMalignWordDown(val)
 * val is used for address arithmetic
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMalignDoubleWordUp(val)    (((CVMAddr)(val) + 7) & ~7)
#define CVMalignWordUp(val)          (((CVMAddr)(val) + 3) & ~3)
#define CVMalignDoubleWordDown(val)  ((CVMAddr)(val) & ~7)
#define CVMalignWordDown(val)        ((CVMAddr)(val) & ~3)

/* 
 * These macros are intended to make sure the
 * correct pointer alignment is used: 4-byte on
 * 32 bit platforms and 8-byte on 64 bit platforms.
 */
#ifdef CVM_64
#define CVMalignAddrUp(val)	     (((CVMAddr)(val) + 7) & ~7)
#define CVMalignAddrDown(val)	     ((CVMAddr)(val) & ~7)
#else
#define CVMalignAddrUp(val)          (((CVMAddr)(val) + 3) & ~3)
#define CVMalignAddrDown(val)        ((CVMAddr)(val) & ~3)
#endif

/*
 * Returns true if the two classes are in the same package. 
 */
CVMBool CVMisSameClassPackage(CVMExecEnv* ee,
			      CVMClassBlock* class1, CVMClassBlock* class2); 

/* This requires that CLAZZ be a non-NULL CVMClassICell pointer and
   point to a valid Java class object. */
extern CVMClassBlock*
CVMgcSafeClassRef2ClassBlock(CVMExecEnv* ee, CVMClassICell *clazz);

/* This requires that CLAZZ be a non-NULL CVMClassICell pointer and
   point to a valid Java class object. */
extern CVMClassBlock *
CVMgcUnsafeClassRef2ClassBlock(CVMExecEnv *ee, CVMClassICell *clazz);

/*
 * Random number generator
 */

/*
 * Initialize the seed
 */
extern void
CVMrandomInit();

/*
 * Get the next random number
 */
extern CVMInt32
CVMrandomNext();


/* Utility function to wrap jvalues (jni.h) in Java objects like
   Integer and Double. This version should only be called from GC-safe
   code; there is no such thing as a GC-unsafe wrapping function,
   since it allocates memory internally and must therefore become
   GC-safe. V (a union) has the appropriate version of its fields set.
   FROMTYPE, an CVMBasicType (basictypes.h) indicates the type of V.
   RESULT must be a valid ICell. Upon success CVM_TRUE is returned,
   and RESULT points either to a newly-created wrapper object for V's
   value or to NULL in the case of a void return value.  Upon failure
   RESULT is set to NULL, CVM_FALSE is returned, and an
   OutOfMemoryError is thrown in the VM. */
CVMBool
CVMgcSafeJavaWrap(CVMExecEnv* ee, jvalue v,
		  CVMBasicType fromType,
		  CVMObjectICell* result);

/* Utility function to unwrap Java objects like Integer and Double
   into jvalues (jni.h). This version should only be called from
   GC-safe code. OBJ must be an appropriately-typed Java object.  The
   appropriate field of V is set with the bits of OBJ's contents, and
   TOTYPE is set to the basic type corresponding to the type of the
   object. Returns CVM_TRUE upon success. Returns CVM_FALSE upon
   failure and throws an exception in the VM. The exception type is
   either the exceptionCb or, if this is NULL,
   IllegalArgumentException (if the conversion failed) or
   NullPointerException (if OBJ was NULL). Note that both V and TOTYPE
   may be mutated in the case of failure. */
CVMBool
CVMgcSafeJavaUnwrap(CVMExecEnv* ee, CVMObjectICell* obj,
		    jvalue* v, CVMBasicType* toType,
		    CVMClassBlock* exceptionCb);

/* Utility function to unwrap Java objects like Integer and Double
   into jvalues (jni.h). This version should only be called from
   GC-unsafe code and is guaranteed not to become GC-safe internally.
   OBJ must be an appropriately-typed Java object. The appropriate
   field of V is set with the bits of OBJ's contents, and TOTYPE is
   set to the basic type corresponding to the type of the
   object. Returns CVM_TRUE upon success. Returns CVM_FALSE upon
   failure and throws an exception in the VM. The exception type is
   either the exceptionCb or, if this is NULL,
   IllegalArgumentException (if the conversion failed) or
   NullPointerException (if OBJ was NULL). Note that both V and TOTYPE
   may be mutated in the case of failure. */
CVMBool
CVMgcUnsafeJavaUnwrap(CVMExecEnv* ee, CVMObject* obj,
		      jvalue* v, CVMBasicType* toType,
		      CVMClassBlock* exceptionCb);

/*
 * Property support
 */

/*
 * Assign value 'val' to property 'key' in properties object 'props'.
 * Use method 'putID' to put the property in.
 *
 * return CVM_TRUE on success, CVM_FALSE on any failure.
 */
CVMBool
CVMputProp(JNIEnv* env, jmethodID putID, 
	   jobject props, const char* key, const char* val);

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
			     jobject props, const char* key, const char* val);

/* Purpose: Packs the specified size up to the next specified packing
            increment.  The packing increment must be an exponential value of
            2 e.g. 2, 4, 8, etc. */
CVMUint32 CVMpackSizeBy(CVMUint32 sizeToPack, CVMUint32 packingIncrement);
#define CVMpackSizeBy(size, inc) \
    ((size + (inc - 1)) & (~(inc - 1)))

/*
 * Suboption parsing - Suboptions are a comma separated list of options
 * such as those that occur after -Xjit or -Xgc. Parsing them takes two
 * steps. The first is to parse them into a separate string for each
 * suboption. The 2nd is to parse the argument for each suboption.
 *
 * The suboption argument parsing can be done in one of two was. The first
 * is to do table driven parsing and the 2nd is to just do selective
 * parsing by making a call that will get a suboption argument string
 * by name,
 */

/*
 * Convert an option string to an interger.
 */
extern CVMInt32 
CVMoptionToInt32(const char* optionString);

/*
 * CVMParsedSubOptions - a data structure used to store all of the 
 * suboptons in a parsed suboptions string.
 */

typedef struct {
    CVMUint32 numOptions;
    char** options;
} CVMParsedSubOptions;

extern CVMBool
CVMinitParsedSubOptions(CVMParsedSubOptions* subOptions,
			const char* subOptionsString);
extern void
CVMdestroyParsedSubOptions(CVMParsedSubOptions *opts);

extern const char *
CVMgetParsedSubOption(const CVMParsedSubOptions* subOptions,
		      const char* subOptionName);

/*
 * CVMSubOptionData - Used for table driven parsing of suboptions. Each
 * entry in the table is of type CVMSubOptionData and provides information
 * about the sub options including it's name, description, kind, allowed
 * values, default value, and where the parsed argument value is to be stored.
 */

typedef enum {
    CVM_NULL_OPTION = 0,
    CVM_INTEGER_OPTION,
    CVM_BOOLEAN_OPTION,
    CVM_PERCENT_OPTION,
    CVM_STRING_OPTION,
    CVM_MULTI_STRING_OPTION,
    CVM_ENUM_OPTION
} CVMSubOptionKindEnum;

typedef struct {
    const char *name;
    CVMUint32 value;
} CVMSubOptionEnumData;

typedef struct {
    const char* name;        /* name as it appears in the command line */
    const char* description; /* textual description */
    CVMSubOptionKindEnum kind;
    union {
	/* Number valued options */
	struct {
	    int       minValue;
	    CVMAddr   maxValue;
	    CVMAddr   defaultValue;
	} intData;
	/* String value options */
	struct {
	    int   ignored1;
	    const char* helpSyntax;
	    const char* defaultValue;
	} strData;
	/* Multi String valued options */
	struct {
	    int numPossibleValues;
	    const char** possibleValues;
	    CVMAddr defaultValue;
	} multiStrData;
        /* Enum value options */
        struct {
            int numPossibleValues;
            const CVMSubOptionEnumData* possibleValues;
            CVMAddr defaultValue;
        } enumData;
    } data;
    const void* valuePtr; /* where parsed value should be stored */
} CVMSubOptionData;

/*
 * Takes all of the parsed suboptions and uses them to populate the
 * what is pointed to by the valuePtr of each known suboption. Uses
 * the suboption defaultValue if no parsed suboption is provided.
 * Does range checking of the parsed suboption using the min and max values.
 */
extern CVMBool
CVMprocessSubOptions(const CVMSubOptionData* knownSubOptions,
		     const char* optionName,
		     CVMParsedSubOptions *parsedSubOptions);

/*
 * Print the value of each suboption in the table. Used after calling
 * CVMprocessSubOptions() to print the "configuration".
 */
extern void
CVMprintSubOptionValues(const CVMSubOptionData* knownSubOptions);

/*
 * Prints a usage string by using the data in the suboption table.
 */
extern void
CVMprintSubOptionsUsageString(const CVMSubOptionData* knownSubOptions);

/*
 * Grouping of platform-independent code for properly setting
 * the java_home, lib_dir and boot class path values, to be
 * invoked with the properly subdirectory values.
 */

extern CVMBool
CVMinitPathValues(void *propsPtr, CVMpathInfo *pathInfo,
                  char **userBootclasspath);

extern void CVMdestroyPathInfo(CVMpathInfo *);

/*
 * Free up the memory allocated in CVMinitPathValues().
 */
extern void
CVMdestroyPathValues(void *propsPtr);

/*
 * Convert 'stringobj' to a C string. Return NULL if unsuccessful.
 */
extern char*
CVMconvertJavaStringToCString(CVMExecEnv* ee, jobject stringobj);

#endif /* CVM_UTILS_H */
