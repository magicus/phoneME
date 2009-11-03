/*
 * @(#)CVM.c	1.82 06/10/30
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
 * All methods in this file use the CNI native method interface. See
 * cni.h for details.
 */

#include "javavm/include/interpreter.h"
#ifdef CVM_DUAL_STACK
#include "javavm/include/dualstack_impl.h"
#endif
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/preloader.h"
#include "javavm/export/jvm.h"
#include "generated/offsets/java_lang_Thread.h"

#include "jni.h"
#include "generated/javavm/include/build_defs.h"

#ifdef CVM_TRACE_JIT
#include "javavm/include/jit/jitutils.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcodebuffer.h"
#endif
#ifndef CDC_10
#include "javavm/include/javaAssertions.h"
#endif
#ifdef CVM_XRUN
#include "javavm/include/xrun.h"
#endif
#include "javavm/include/porting/time.h"
#ifdef CVM_JVMTI
#include "javavm/include/jvmti_jni.h"
#endif

/* Set the systemClassLoader */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setSystemClassLoader(CVMExecEnv*ee, CVMStackVal32 *arguments,
                                     CVMMethodBlock **p_mb)
{
    jobject loaderObj = &arguments[0].j.r;
    CVMD_gcSafeExec(ee, {
        CVMclassSetSystemClassLoader(ee, loaderObj);
    });
    return CNI_VOID;
}

#ifdef CVM_DEBUG_ASSERTS
#define CVMassertOKToCopyArrayOfType(expectedType_) \
    {                                                                   \
        CVMClassBlock *srcCb, *dstCb;                                   \
        CVMClassBlock *srcElemCb, *dstElemCb;                           \
        size_t srclen, dstlen;                                          \
                                                                        \
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
        if (expectedType_ != CVM_T_CLASS) {                             \
            CVMassert(srcElemCb == dstElemCb);                          \
        } else {                                                        \
            CVMassert((srcElemCb == dstElemCb) ||                       \
                      (dstElemCb == CVMsystemClass(java_lang_Object))); \
        }                                                               \
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
#endif

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyBooleanArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                 CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfBoolean *srcArr;
    CVMArrayOfBoolean *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfBoolean *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfBoolean *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_BOOLEAN);
    CVMD_arrayCopyBoolean(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyByteArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfByte *srcArr;
    CVMArrayOfByte *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfByte *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfByte *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_BYTE);
    CVMD_arrayCopyByte(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyShortArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                               CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfShort *srcArr;
    CVMArrayOfShort *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfShort *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfShort *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_SHORT);
    CVMD_arrayCopyShort(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyCharArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfChar *srcArr;
    CVMArrayOfChar *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfChar *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfChar *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_CHAR);
    CVMD_arrayCopyChar(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyIntArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                             CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfInt *srcArr;
    CVMArrayOfInt *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfInt *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfInt *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_INT);
    CVMD_arrayCopyInt(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyLongArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfLong *srcArr;
    CVMArrayOfLong *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfLong *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfLong *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_LONG);
    CVMD_arrayCopyLong(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyFloatArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                               CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfFloat *srcArr;
    CVMArrayOfFloat *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfFloat *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfFloat *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_FLOAT);
    CVMD_arrayCopyFloat(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyDoubleArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfDouble *srcArr;
    CVMArrayOfDouble *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfDouble *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfDouble *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_DOUBLE);
    CVMD_arrayCopyDouble(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

/* Purpose: This method copies array elements from one array to another.
   Unlike System.arraycopy(), this method can only copy elements for
   non-primitive arrays with some restrictions.  The restrictions are that
   the src and dest array must be of the same type, or the destination
   array must be of type java.lang.Object[] (not just a compatible sub-type).
   The caller is responsible for doing the appropriate null checks, bounds
   checks, array element type assignment checks if necessary, and ensure that
   the passed in arguments do violate any of these checks and restrictions.
   If the condition of these checks and restrictions are not taken cared of
   by the caller, copyObjectArray() can fail in unpredictable ways.
*/
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_copyObjectArray(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                CVMMethodBlock **p_mb)
{
    jobject src  = &arguments[0].j.r;
    jint src_pos =  arguments[1].j.i;
    jobject dst  = &arguments[2].j.r;
    jint dst_pos =  arguments[3].j.i;
    jint length  =  arguments[4].j.i;
    CVMArrayOfRef *srcArr;
    CVMArrayOfRef *dstArr;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    srcArr = (CVMArrayOfRef *)CVMID_icellDirect(ee, src);
    dstArr = (CVMArrayOfRef *)CVMID_icellDirect(ee, dst);
    CVMassertOKToCopyArrayOfType(CVM_T_CLASS);
    CVMD_arrayCopyRef(srcArr, src_pos, dstArr, dst_pos, length);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_checkDebugFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
				CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_ENABLED
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMcheckDebugFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setDebugFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_ENABLED
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMsetDebugFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_clearDebugFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
				CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_ENABLED
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMclearDebugFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_restoreDebugFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_ENABLED
    CVMJavaInt flags = arguments[0].j.i;
    CVMJavaInt oldvalue = arguments[1].j.i;
    arguments[0].j.i = CVMrestoreDebugFlags(flags, oldvalue);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_checkDebugJITFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                   CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_JIT
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMcheckDebugJITFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setDebugJITFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                 CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_JIT
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMsetDebugJITFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_clearDebugJITFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                   CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_JIT
    CVMJavaInt flags = arguments[0].j.i;
    arguments[0].j.i = CVMclearDebugJITFlags(flags);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_restoreDebugJITFlags(CVMExecEnv* ee, CVMStackVal32 *arguments,
                                     CVMMethodBlock **p_mb)
{
#ifdef CVM_TRACE_JIT
    CVMJavaInt flags = arguments[0].j.i;
    CVMJavaInt oldvalue = arguments[1].j.i;
    arguments[0].j.i = CVMrestoreDebugJITFlags(flags, oldvalue);
#else
    arguments[0].j.i = 0;
#endif
    return CNI_SINGLE;
}

/*
 * executeClinit is responsible for getting the <clinit> method of the
 * specified class executed. It could do this by just using JNI to
 * invoke the <clinit> method, but this causes undesireable C recursion
 * in the interpreter. Instead we just store the mb of the <clinit>
 * method in *p_mb, and return CVM_NEW_MB to the interpreter. This
 * signals the interpreter to invoke the method stored in *p_mb.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_executeClinit(CVMExecEnv* ee, CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
    CVMClassBlock* cb = CVMgcUnsafeClassRef2ClassBlock(ee, &arguments[0].j.r);
    CVMMethodBlock* clinitmb;
    CVMD_gcSafeExec(ee, {
	clinitmb = CVMclassGetStaticMethodBlock(cb, CVMglobals.clinitTid);
    });
    CVMtraceClinit(("[Initializing %C]\n", cb));
    if (clinitmb != NULL) {
	CVMtraceClinit(("[Running static initializer for %C]\n", cb));
	/* Return the new mb */
	*p_mb = clinitmb;
	return CNI_NEW_MB;
    } else {
	return CNI_VOID;
    }
}

/*
 * If the class is dynamically loaded and has a <clinit> method, then
 * free up the memory allocated for the <clinit> method. Note the mb,
 * stays around, but all the code and other data located in the jmd
 * is freed up.
 *
 * (Formerly, the entire body of this was bracketed by the
 * ifdef CVM_CLASSLOADING block. But now that we want to
 * free the stackmaps even for preloaded classes, we need to
 * go through the motions anyway.)
 */

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_freeClinit(CVMExecEnv* ee, CVMStackVal32 *arguments,
			   CVMMethodBlock **p_mb)
{
    /*
     * Both JVMTI and JVMPI require that the jmd for the clinit
     * not be freed.
     */
#if !defined(CVM_JVMTI) && !defined(CVM_JVMPI)
    CVMClassBlock* cb = CVMgcUnsafeClassRef2ClassBlock(ee, &arguments[0].j.r);
    CVMMethodBlock* clinitmb;
    CVMJavaMethodDescriptor* jmd;

    CVMD_gcSafeExec(ee, {
	clinitmb = CVMclassGetStaticMethodBlock(cb, CVMglobals.clinitTid);
	if (clinitmb != NULL) {
	    if (CVMmbIsJava(clinitmb)) {
                CVMStackMaps *maps;
		jmd = CVMmbJmd(clinitmb);
                /* Need to acquire the heapLock before accessing the
                 * stackmaps list to avoid contention with a gc thread,
                 * which may also manipulate the list.
                 */
                CVMsysMutexLock(ee, &CVMglobals.heapLock);
		if ((maps = CVMstackmapFind(ee, clinitmb)) != NULL) {
                    CVMstackmapDestroy(ee, maps);
		}
                CVMsysMutexUnlock(ee, &CVMglobals.heapLock);

		if ((!CVMcbIsInROM(cb)) 
		    || (CVMjmdFlags(jmd) & CVM_JMD_DID_REWRITE)) {
		    CVMmbJmd(clinitmb) = NULL;
		    free(jmd);
		}
		if (!CVMcbIsInROM(cb)) {
		    CVMclassFreeLocalVariableTableFieldIDs(ee, clinitmb);
		}
	    }
	}
    });
#endif
    return CNI_VOID;
}

/*
 * executeLoadSuperClasses is responsible for getting the
 * Class.loadSuperClasses() executed for
 * Launcher.defineClassPrivate(), which doesn't have access to it from
 * java. It could do this by just using JNI to invoke
 * theClass.loadSuperClasses() method, but this causes undesireable C
 * recursion in the interpreter. Instead we just store the mb of the
 * Class.loadSuperClasses() method in *p_mb, and return CVM_NEW_MB to the
 * interpreter. This signals the interpreter to invoke the method
 * stored in *p_mb. 
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_executeLoadSuperClasses(
    CVMExecEnv* ee, CVMStackVal32 *arguments, CVMMethodBlock **p_mb)
{
    /* Return the new mb */
    *p_mb = CVMglobals.java_lang_Class_loadSuperClasses;
    return CNI_NEW_MB;
}

#ifdef FOR_EXAMPLE
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_arraycopy(CVMExecEnv* ee, CVMStackVal32 *arguments,
			  CVMMethodBlock **p_mb)
{
    CVMObjectICell *src = &arguments[0].j.r;
    CVMJavaInt src_position = arguments[1].j.i;
    CVMObjectICell *dst = &arguments[2].j.r;
    CVMJavaInt dst_position = arguments[3].j.i;
    CVMJavaInt length = arguments[4].j.i;

    /* For now, until we have an optimized, unsafe version of arraycopy */
    CVMD_gcSafeExec(ee, {
	JNIEnv *env = CVMexecEnv2JniEnv(ee);
	JVM_ArrayCopy(env, NULL, src, src_position, dst, dst_position, length);
    });

    return CNI_VOID;
}
#endif

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_disableRemoteExceptions(CVMExecEnv* ee,
					CVMStackVal32 *arguments,
					CVMMethodBlock **p_mb)
{
    CVMdisableRemoteExceptions(ee);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_enableRemoteExceptions(CVMExecEnv* ee,
				       CVMStackVal32 *arguments,
				       CVMMethodBlock **p_mb)
{
    CVMenableRemoteExceptions(ee);
    /* Check if remote exception have thrown */
    if (CVMexceptionOccurred(ee)) {
	return CNI_EXCEPTION;
    } else {
	return CNI_VOID;
    }
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_throwRemoteException(CVMExecEnv* ee,
				     CVMStackVal32 *arguments,
				     CVMMethodBlock **p_mb)
{
#ifdef CVM_REMOTE_EXCEPTIONS_SUPPORTED
    CVMObjectICell *threadICell = &arguments[0].j.r;
    CVMObjectICell *exceptionICell = &arguments[1].j.r;
    CVMJavaLong eetop;
    CVMExecEnv *targetEE;

    CVMD_fieldReadLong(CVMID_icellDirect(ee, threadICell),
		       CVMoffsetOfjava_lang_Thread_eetop,
		       eetop);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetop);

    /* Have to be invoked through Thread.stop1() that ensures ee != NULL */
    CVMassert(ee != NULL);
    CVMgcUnsafeThrowRemoteException(ee, targetEE, 
				    CVMID_icellDirect(ee, exceptionICell));

    /* %comment: rt037 */
#else
    CVMassert(CVM_FALSE);
#endif
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_throwLocalException(CVMExecEnv* ee,
				    CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
    CVMObjectICell *exceptionICell = &arguments[0].j.r;

    CVMgcUnsafeThrowLocalException(ee, CVMID_icellDirect(ee, exceptionICell));

    return CNI_EXCEPTION;
}

static void isMIDPClass(CVMExecEnv *ee, CVMStackVal32 *arguments,
                        CVMMethodBlock **p_mb, CVMBool checkImpl)
{
#ifndef CVM_DUAL_STACK
    arguments[0].j.i = CVM_FALSE;
#else
    CVMClassBlock* cb;
    CVMClassLoaderICell* loaderICell;
 
    /* 
     * Get the caller. Note we only look one frame up here because
     * there is no frame pushed for the CNI method.
     */
    cb = CVMgetCallerClass(ee, 1);
    loaderICell = (cb == NULL) ? NULL : CVMcbClassLoader(cb);
    
    arguments[0].j.i = CVMclassloaderIsMIDPClassLoader(
        ee, loaderICell, checkImpl);
#endif
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_callerCLIsMIDCLs(CVMExecEnv* ee,
                                   CVMStackVal32 *arguments,
                                   CVMMethodBlock **p_mb)
{
    isMIDPClass(ee, arguments, p_mb, CVM_TRUE);
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_callerIsMidlet(CVMExecEnv* ee,
                                   CVMStackVal32 *arguments,
                                   CVMMethodBlock **p_mb)
{
    isMIDPClass(ee, arguments, p_mb, CVM_FALSE);
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_isMIDPContext(CVMExecEnv* ee,
                              CVMStackVal32 *arguments,
                              CVMMethodBlock **p_mb)
{
#ifndef CVM_DUAL_STACK
   arguments[0].j.i = CVM_FALSE;
#else
    CVMBool result = CVM_FALSE;
    int i;

    /* 
     * Check each caller on the stack to if it is was loaded by a MIDP class
     * class loader. Note we start one frame up here because
     * there is no frame pushed for the CNI method.
     */
    for (i = 1; ; i++) {
        CVMClassBlock* cb;
        CVMClassLoaderICell* loaderICell;
 
        cb = CVMgetCallerClass(ee, i);
        if (NULL == cb) {
            break;
        }

        loaderICell = CVMcbClassLoader(cb);
        if (NULL == loaderICell) {
            continue;
        }

        result = CVMclassloaderIsMIDPClassLoader(ee, loaderICell, CVM_TRUE);
        if (result) {
            break;
        }
    }

    arguments[0].j.i = result;
#endif
    return CNI_SINGLE;
}

/* %begin lvm */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_inMainLVM(CVMExecEnv* ee,
			  CVMStackVal32 *arguments,
			  CVMMethodBlock **p_mb)
{
#ifdef CVM_LVM
    CVMD_gcSafeExec(ee, {
	arguments[0].j.i = (CVMLVMinMainLVM(ee))?(JNI_TRUE):(JNI_FALSE);
    });
#else
    arguments[0].j.i = JNI_TRUE;
#endif

    return CNI_SINGLE;
}
/* %end lvm */

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_gcDumpHeapSimple(CVMExecEnv* ee,
				 CVMStackVal32 *arguments,
				 CVMMethodBlock **p_mb)
{
#ifdef CVM_INSPECTOR
    CVMD_gcSafeExec(ee, {
	CVMgcDumpHeapSimple();
    });
#endif

    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_gcDumpHeapVerbose(CVMExecEnv* ee,
				  CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
#ifdef CVM_INSPECTOR
    CVMD_gcSafeExec(ee, {
	CVMgcDumpHeapVerbose();
    });
#endif

    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_gcDumpHeapStats(CVMExecEnv* ee,
				CVMStackVal32 *arguments,
				CVMMethodBlock **p_mb)
{
#ifdef CVM_INSPECTOR
    CVMD_gcSafeExec(ee, {
	CVMgcDumpHeapStats();
    });
#endif

    return CNI_VOID;
}

#ifdef CVM_DEBUG
#include "javavm/include/porting/system.h"
#include "javavm/include/porting/time.h"

#define TRACE_SIZE 1000
static CVMInt64 millis[TRACE_SIZE];
static int id[TRACE_SIZE];
static int indx;
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_trace(CVMExecEnv *ee,
		      CVMStackVal32 *arguments,
		      CVMMethodBlock **p_mb)
{
    CVMJavaInt i = arguments[0].j.i;
    CVMInt64 l = CVMtimeMillis();

    if (i < 0) {
	if (indx > 0) {
	    int j;
	    for (j = 1; j < indx; ++j) {
		CVMconsolePrintf("t%d - t%d--> %dms\n", id[j], id[j-1],
		    CVMlong2Int(CVMlongSub(millis[j], millis[j-1])));
	    }
	}
	indx = 0;
	i = -i;
    }
    if (indx < TRACE_SIZE) {
	millis[indx] = l;
	id[indx] = i;
	++indx;
    }

    return CNI_VOID;
}
#else
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_trace(CVMExecEnv *ee,
		      CVMStackVal32 *arguments,
		      CVMMethodBlock **p_mb)
{
    return CNI_VOID;
}
#endif /* !DEBUG */


CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setDebugEvents(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
#ifdef CVM_JVMTI
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiDebugEventsEnabled(ee) = arguments[0].j.i;
    }
#endif
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_postThreadExit(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    CVMD_gcSafeExec(ee, {
	CVMpostThreadExitEvents(ee);
    });
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setContextArtificial(CVMExecEnv* ee, CVMStackVal32 *arguments,
				     CVMMethodBlock **p_mb)
{
    CVMframeSetContextArtificial(ee);
    return CNI_VOID;
}

/*
 * Inflates an object's monitor and marks it sticky so it's never freed.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_objectInflatePermanently(CVMExecEnv* ee,
					 CVMStackVal32 *arguments,
					 CVMMethodBlock **p_mb)
{
    CVMObjectICell *indirectObj = &arguments[0].j.r;
    CVMObjMonitor *mon;

    mon = CVMobjectInflatePermanently(ee, indirectObj);
    if (mon != NULL) {
	arguments[0].j.i = CVM_TRUE;
    } else {
	arguments[0].j.i = CVM_FALSE;
    }

    return CNI_SINGLE;
}


/*
 * enable/disable compilations by current thread
 */
CNIEXPORT CNIResultCode 
CNIsun_misc_CVM_setThreadNoCompilationsFlag(CVMExecEnv* ee,
					    CVMStackVal32 *arguments,
					    CVMMethodBlock **p_mb)
{
#ifdef CVM_JIT
    CVMBool noCompilations = arguments[0].j.i;
    ee->noCompilations = noCompilations;
#endif    
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_getCallerClass(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    CVMJavaInt skip = arguments[0].j.i;

    CVMClassBlock* cb = CVMgetCallerClass(ee, skip);
    CVMObject* result;
    if (cb == NULL) {
        result = NULL;
    } else {
        result = CVMID_icellDirect(ee, CVMcbJavaInstance(cb));
    }
    CVMID_icellSetDirect(ee, &arguments[0].j.r, result);
    return CNI_SINGLE;
}

/*
 * Is the compiler built in?
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_isCompilerSupported(CVMExecEnv* ee,
				    CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_JIT
    arguments[0].j.i = CVM_TRUE;
#else
    arguments[0].j.i = CVM_FALSE;
#endif
    return CNI_SINGLE;
}

/*
 * Request a dump of the profiling data collected by the compiler if available.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_dumpCompilerProfileData(CVMExecEnv* ee,
				        CVMStackVal32 *arguments,
				        CVMMethodBlock **p_mb)
{
#if defined(CVM_JIT) && defined(CVM_JIT_PROFILE)
    CVMD_gcSafeExec(ee, {
        CVMJITcodeCacheDumpProfileData();
    });
#endif
    return CNI_VOID;
}

/*
 * Dump misc. stats
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_dumpStats(CVMExecEnv* ee,
			  CVMStackVal32 *arguments,
			  CVMMethodBlock **p_mb)
{
    /* Insert any stats you want here */
#ifdef CVM_USE_MEM_MGR
    /* Dump the dirty page info that the Memory Manager collected
     * for the monitored regions.
     */
    CVMmemManagerDumpStats();
#endif
    return CNI_VOID;
}

/*
 * Mark the code buffer
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_markCodeBuffer(CVMExecEnv* ee,
			       CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
#ifdef CVM_JIT
    CVMJITmarkCodeBuffer();
#if 0
    CVMconsolePrintf("MARKED THIS JITBUFFER SPOT, %d BYTES IN USE\n",
		     CVMglobals.jit.codeCacheBytesAllocated);
#endif
#endif

#ifdef CVM_USE_MEM_MGR
    /* the .bss region */
    CVMmemRegisterBSS();
#endif /* CVM_USE_MEM_MGR */

    return CNI_VOID;
}

/*
 * Enable compilation
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_initializeJITPolicy(CVMExecEnv* ee,
			          CVMStackVal32 *arguments,
			          CVMMethodBlock **p_m)
{
#ifdef CVM_JIT
#if defined(CVM_AOT) || defined(CVM_MTASK)
    CVMjitProcessOptionsAndPolicyInit(ee, &CVMglobals.jit);
#endif
#endif

    return CNI_VOID;
}

/*
 * Initialize pre-compiled code.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_initializeAOTCode(CVMExecEnv* ee,
                                  CVMStackVal32 *arguments,
                                  CVMMethodBlock **p_m)
{
#ifdef CVM_AOT
    CVMD_gcSafeExec(ee, {
        arguments[0].j.i = CVMJITinitializeAOTCode();
    });
#else
    CVMD_gcSafeExec(ee, {
        arguments[0].j.i = CVM_TRUE;
    });
#endif
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_maskInterrupts(CVMExecEnv* ee,
			       CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    CVMD_gcSafeExec(ee, {
	arguments[0].j.i = CVMmaskInterrupts(ee);
    });
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_unmaskInterrupts(CVMExecEnv* ee,
			       CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    CVMD_gcSafeExec(ee, {
	CVMunmaskInterrupts(ee);
    });
    return CNI_VOID;
}


CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseVerifyOptions(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    jobject opts  = &arguments[0].j.r;
    char* kind = CVMconvertJavaStringToCString(ee, opts);
    CVMBool result;
    if (kind == NULL) {
	result = CVM_FALSE;
    } else {
	int verification = CVMclassVerificationSpecToEncoding(kind);
	if (verification != CVM_VERIFY_UNRECOGNIZED) {
	    CVMglobals.classVerificationLevel = verification;
	    result = CVM_TRUE;
	} else {
	    result = CVM_FALSE;
	}
	free(kind);
    }
    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseSplitVerifyOptions(CVMExecEnv* ee,
                                        CVMStackVal32 *arguments,
                                        CVMMethodBlock **p_mb)
{
    CVMBool result;
#ifdef CVM_SPLIT_VERIFY
    jobject opts  = &arguments[0].j.r;
    char* value = CVMconvertJavaStringToCString(ee, opts);
    if (value == NULL) {
	result = CVM_FALSE;
    } else {
        if (!strcmp(value, "true")) {
            CVMglobals.splitVerify = CVM_TRUE;
            result = CVM_TRUE;
        } else if (!strcmp(value, "false")) {
            result = CVM_TRUE;
            CVMglobals.splitVerify = CVM_FALSE;
        } else {
            result = CVM_FALSE;
        }
	free(value);
    }
#else
    result = CVM_FALSE;
#endif
    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseXoptOptions(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    jobject opts  = &arguments[0].j.r;
    char* kind = CVMconvertJavaStringToCString(ee, opts);
    CVMBool result;
    if (kind == NULL) {
	result = CVM_FALSE;
    } else {
	/* parse -Xopt options here */
	result = CVMoptParseXoptOptions(kind);
	free(kind);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseXssOption(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    jobject opts  = &arguments[0].j.r;
    char* kind = CVMconvertJavaStringToCString(ee, opts);
    CVMBool result;
    if (kind == NULL) {
	result = CVM_FALSE;
    } else {
	/* parse -Xss options here */
	result = CVMoptParseXssOption(kind);
	free(kind);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseXgcOptions(CVMExecEnv* ee, CVMStackVal32 *arguments,
				CVMMethodBlock **p_mb)
{
    jobject opts  = &arguments[0].j.r;
    char* kind = CVMconvertJavaStringToCString(ee, opts);
    CVMBool result;
    if (kind == NULL) {
	result = CVM_FALSE;
    } else {
	/* parse -Xgc options here */
#ifdef CVM_MTASK
	result = CVMgcParseXgcOptions(ee, kind);
#else
	result = CVM_FALSE;
#endif
	free(kind);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_parseAssertionOptions(CVMExecEnv* ee, CVMStackVal32 *arguments,
				      CVMMethodBlock **p_mb)
{
    jobject opts  = &arguments[0].j.r;
    char* str = CVMconvertJavaStringToCString(ee, opts);
    CVMBool result;
    if (str == NULL) {
	result = CVM_FALSE;
    } else {
	/* parse assertion related options here */
#ifdef CDC_10
	result = CVM_FALSE;
#else
	/* java assertion handling */
	if (!strncmp(str, "-ea:", 4)) {
	    CVMBool success = CVMJavaAssertions_addOption(
		str + 4, CVM_TRUE,
		&CVMglobals.javaAssertionsClasses,
		&CVMglobals.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-enableassertions:", 18)) {
	    CVMBool success = CVMJavaAssertions_addOption(
                str + 18, CVM_TRUE,
		&CVMglobals.javaAssertionsClasses,
		&CVMglobals.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-da:", 4)) {
	    CVMBool success = CVMJavaAssertions_addOption(
                str + 4, CVM_FALSE,
		&CVMglobals.javaAssertionsClasses,
		&CVMglobals.javaAssertionsPackages);
	    if (!success) goto addOption_failed;
	} else if (!strncmp(str, "-disableassertions:", 18)) {
	    CVMBool success;
	    success = CVMJavaAssertions_addOption(
                str + 19, CVM_FALSE,
		&CVMglobals.javaAssertionsClasses,
		&CVMglobals.javaAssertionsPackages);
            if (!success) {
	addOption_failed:
                CVMconsolePrintf("out of memory "
				 "while parsing assertion option\n");
		result = CVM_FALSE;
		goto done;
            }
	} else if (!strcmp(str, "-ea") |
		   !strcmp(str, "-enableassertions")) {
	    CVMglobals.javaAssertionsUserDefault = CVM_TRUE;
	} else if (!strcmp(str, "-da") |
		   !strcmp(str, "-disableassertions")) {
	    CVMglobals.javaAssertionsUserDefault = CVM_FALSE;
	} else if (!strcmp(str, "-esa") |
		   !strcmp(str, "-enablesystemassertions")) {
	    CVMglobals.javaAssertionsSysDefault = CVM_TRUE;
	} else if (!strcmp(str, "-dsa") |
		   !strcmp(str, "-disablesystemassertions")) {
	    CVMglobals.javaAssertionsSysDefault = CVM_FALSE;
	}
	result = CVM_TRUE;
#endif
done:
	free(str);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_agentlibSupported(CVMExecEnv* ee,
			      CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
#ifdef CVM_AGENTLIB
    arguments[0].j.i = CVM_TRUE;
#else
    arguments[0].j.i = CVM_FALSE;
#endif

    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_agentlibInitialize(CVMExecEnv* ee,
			      CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
#ifdef CVM_AGENTLIB
    CVMJavaInt numArgs = arguments[0].j.i;
    
    if (CVMAgentInitTable(&CVMglobals.agentTable, numArgs)) {
	arguments[0].j.i = CVM_TRUE;
    } else {
	arguments[0].j.i = CVM_FALSE;
    }
#else
    arguments[0].j.i = CVM_FALSE;
#endif

    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_agentlibProcess(CVMExecEnv* ee, CVMStackVal32 *arguments,
			    CVMMethodBlock **p_mb)
{
    jobject agentArgStr  = &arguments[0].j.r;

    char* agentArg = CVMconvertJavaStringToCString(ee, agentArgStr);
    CVMBool result = CVM_FALSE;

    if (agentArg == NULL) {
	result = CVM_FALSE;
    } else {
#ifdef CVM_AGENTLIB
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMAgentlibArg_t agentlibArgument;
	agentlibArgument.is_absolute = CVM_FALSE;
	agentlibArgument.str = agentArg;
	if (!strncmp(agentArg, "-agentpath:", 11)) {
	    agentlibArgument.is_absolute = CVM_TRUE;
	}
	CVMD_gcSafeExec(ee, {
		if ((*env)->PushLocalFrame(env, 16) == JNI_OK) {
		    result = CVMAgentHandleArgument(&CVMglobals.agentTable,
						    env,
						    &agentlibArgument);
		    (*env)->PopLocalFrame(env, NULL);
		}
	    });
#else
	result = CVM_FALSE;
#endif
	free(agentArg);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_xrunSupported(CVMExecEnv* ee,
			      CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
#ifdef CVM_XRUN
    arguments[0].j.i = CVM_TRUE;
#else
    arguments[0].j.i = CVM_FALSE;
#endif

    return CNI_SINGLE;
}


CNIEXPORT CNIResultCode
CNIsun_misc_CVM_xrunInitialize(CVMExecEnv* ee,
			      CVMStackVal32 *arguments,
			      CVMMethodBlock **p_mb)
{
#ifdef CVM_XRUN
    CVMJavaInt numArgs = arguments[0].j.i;
    
    if (CVMXrunInitTable(&CVMglobals.onUnloadTable, numArgs)) {
	arguments[0].j.i = CVM_TRUE;
    } else {
	arguments[0].j.i = CVM_FALSE;
    }
#else
    arguments[0].j.i = CVM_FALSE;
#endif

    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_xrunProcess(CVMExecEnv* ee, CVMStackVal32 *arguments,
			    CVMMethodBlock **p_mb)
{
    jobject xrunArgStr  = &arguments[0].j.r;
    char* xrunArg = CVMconvertJavaStringToCString(ee, xrunArgStr);
    CVMBool result = CVM_FALSE;

    if (xrunArg == NULL) {
	result = CVM_FALSE;
    } else {
#ifdef CVM_XRUN
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	CVMD_gcSafeExec(ee, {
	    if ((*env)->PushLocalFrame(env, 16) == JNI_OK) {
  	        result = CVMXrunHandleArgument(&CVMglobals.onUnloadTable,
					       env,
					       xrunArg);
		(*env)->PopLocalFrame(env, NULL);
	    }
        });
#else
	result = CVM_FALSE;
#endif
	free(xrunArg);
    }

    arguments[0].j.i = result;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_xdebugSet(CVMExecEnv* ee, CVMStackVal32 *arguments,
			  CVMMethodBlock **p_mb)
{
    arguments[0].j.i = CVM_FALSE;
#ifdef CVM_JVMTI
    /*
     * NOTE: JVMTI uses -Xdebug to signal that this is a debugging
     * session vs. profiling.  This flag causes several jvmti 
     * capabilities to be turned off.  See jvmtiCapabilities.c
     */
    CVMjvmtiSetDebugOption(CVM_TRUE); 
    arguments[0].j.i = CVM_TRUE; 
#endif

    return CNI_SINGLE;
}

#include "javavm/include/localroots.h"
#include "jni_util.h"


CNIEXPORT CNIResultCode
CNIsun_misc_CVM_00024Preloader_getClassLoaderNames(CVMExecEnv* ee,
    CVMStackVal32 *arguments,
    CVMMethodBlock **p_mb)
{
    CNIResultCode result = CNI_SINGLE;

    CVMID_localrootBeginGcUnsafe(ee) {
	CVMID_localrootDeclareGcUnsafe(CVMObjectICell, namesICell);

	CVMD_gcSafeExec(ee, {
	    JNIEnv* env = CVMexecEnv2JniEnv(ee);
	    if ((*env)->PushLocalFrame(env, 16) == JNI_OK) {

		jstring names = JNU_NewStringPlatform(env,
		    CVMpreloaderGetClassLoaderNames(ee));

		if ((*env)->ExceptionOccurred(env)) {
		    result = CNI_EXCEPTION;
		} else {
		    CVMID_icellAssign(ee, namesICell, names);
		}

		(*env)->PopLocalFrame(env, NULL);
	    }
	});

	if (result != CNI_EXCEPTION) {
	    CVMID_icellAssignDirect(ee, &arguments[0].j.r, namesICell);
	}

    } CVMID_localrootEndGcUnsafe();

    return result;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_00024Preloader_registerClassLoader0(CVMExecEnv* ee,
    CVMStackVal32 *arguments,
    CVMMethodBlock **p_mb)
{
    CVMpreloaderRegisterClassLoaderUnsafe(ee,
	arguments[0].j.i, /* index */
	&arguments[1].j.r /* cl */
    );
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_nanoTime(CVMExecEnv* ee, CVMStackVal32 *arguments, CVMMethodBlock **p_mb)
{
    jlong time;
#ifdef CVM_JVMTI
    time = CVMtimeNanosecs();
#else
    time = (jlong)(((CVMInt64)CVMtimeMillis()) * 1000000);
#endif
    CVMlong2Jvm((CVMAddr*)&arguments[0].j, time);
    return CNI_DOUBLE;
}

#if 1

/*
 * The following should never be called. These APIs are only called
 * in JIT'd code that support CVMJIT_SIMPLE_SYNC_METHODS, and in this
 * case they are implemented as intrinsics emitters.
 */

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_simpleLockGrab(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb) {
    CVMassert(CVM_FALSE);
    arguments[0].j.i = CVM_FALSE;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_simpleLockRelease(CVMExecEnv* ee, CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
    CVMassert(CVM_FALSE);
    return CNI_SINGLE;
}

#else

/*
 * CNI versions of CVM.simpleLockGrab and CVM.simpleLockRelease.
 * These are disabled by default and are only used to gather stats
 * about how often the simple sync methods are called. In order
 * to use them you must also declare these methods native in CVM.java 
 * and disabled the intrinsics in ccmintrinsics_risc.c.
 */
#define CVM_MAX_SYNC_PROF_MBS 50

static CVMMethodBlock* mbs[CVM_MAX_SYNC_PROF_MBS];
static int slow[CVM_MAX_SYNC_PROF_MBS];
static int fast[CVM_MAX_SYNC_PROF_MBS];

static int findMB(CVMMethodBlock* mb) {
    int i = 0;
    while (i < CVM_MAX_SYNC_PROF_MBS) {
	if (mbs[i] == mb) {
	    return i;
	}
	if (mbs[i] == NULL) {
	    mbs[i] = mb;
	    return i;
	}
	i++;
    }
    CVMassert(CVM_FALSE);
    return 0;
}

extern void dumpMBs() {
    int i = 0;
    CVMconsolePrintf("   fast       slow\n");
    while (mbs[i] != NULL) {
	CVMconsolePrintf("%10d %10d %C.%M\n", fast[i], slow[i],
			 CVMmbClassBlock(mbs[i]), mbs[i]);
	i++;
    }
}

/* NOTE: this code is disabled. See comment above */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_simpleLockGrab(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
    jobject obj = &arguments[0].j.r;
    CVMObject* directObj = CVMID_icellDirect(ee, obj);
    CVMMethodBlock* mb = ee->interpreterStack.currentFrame->mb;
    int mbslot = findMB(mb);
    if (CVMglobals.objGlobalMicroLock.lockWord == CVM_MICROLOCK_UNLOCKED &&
	CVMobjMonitorState(directObj) == CVM_LOCKSTATE_UNLOCKED)
    {
	fast[mbslot]++;
    }  else {
	slow[mbslot]++;
	//CVMdumpStack(&ee->interpreterStack,0,0,10);
	//CVMconsolePrintf("\n");
    }
    
    arguments[0].j.i = CVM_FALSE;
    return CNI_SINGLE;
}

/* NOTE: this code is disabled. See comment above */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_simpleLockRelease(CVMExecEnv* ee, CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
    return CNI_VOID;
}

#endif

/* Gets the VM build options as a Java string. */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_getBuildOptionString(CVMExecEnv* ee, CVMStackVal32 *arguments,
				     CVMMethodBlock **p_mb)
{
    jobject result = NULL;

    CVMD_gcSafeExec(ee, {
        JNIEnv *env = CVMexecEnv2JniEnv(ee);
        if ((*env)->PushLocalFrame(env, 4) == 0) {
	    result = (*env)->NewStringUTF(env, CVM_BUILD_OPTIONS);
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

/*
 * Sets java.net.URLConnection.defaultUseCaches to the boolean
 * argument passed in.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setURLConnectionDefaultUseCaches(CVMExecEnv* ee,
						 CVMStackVal32 *arguments,
						 CVMMethodBlock **p_mb)
{
    CVMClassBlock* cb = CVMsystemClass(java_net_URLConnection);
    CVMFieldTypeID fieldTypeID = CVMtypeidLookupFieldIDFromNameAndSig(
        ee, "defaultUseCaches", "Z");
    CVMFieldBlock* fb = CVMclassGetStaticFieldBlock(cb, fieldTypeID);
    CVMassert(fb != NULL);
    CVMfbStaticField(ee, fb).i = arguments[0].j.i;
    return CNI_VOID;
}

/*
 * Clears the ucp field of the URLClassLoader passed in, which allows
 * the JarFiles opened by the URLClassLoader to be gc'd and closed,
 * even if the URLClassLoader is kept live.
 */
#include "generated/offsets/java_net_URLClassLoader.h"
extern const CVMClassBlock java_net_URLConnection_Classblock;
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_clearURLClassLoaderUcpField(CVMExecEnv* ee,
					    CVMStackVal32 *arguments,
					    CVMMethodBlock **p_mb)
{
    CVMObjectICell* classLoaderICell = &arguments[0].j.r;
    CVMD_fieldWriteRef(CVMID_icellDirect(ee, classLoaderICell),
			 CVMoffsetOfjava_net_URLClassLoader_ucp,
			 NULL);
    return CNI_VOID;
}

/*
 * Returns Throwable.backtrace, a private field.
 */
#include "generated/offsets/java_lang_Throwable.h"
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_getExceptionBackTrace(CVMExecEnv* ee,
                                      CVMStackVal32 *arguments,
                                      CVMMethodBlock **p_mb)
{
    CVMObjectICell* throwableICell = &arguments[0].j.r;
    CVMObject* backtrace;
    CVMD_fieldReadRef(CVMID_icellDirect(ee, throwableICell),
                      CVMoffsetOfjava_lang_Throwable_backtrace,
                      backtrace);
    CVMID_icellSetDirect(ee, &arguments[0].j.r, backtrace);
    return CNI_SINGLE;
}

/*
 * Set ThreadGroup.saveThreadStarterClassFlag.
 */
#include "generated/offsets/java_lang_ThreadGroup.h"
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setSaveThreadStarterClassFlag(CVMExecEnv* ee,
                                              CVMStackVal32 *arguments,
                                              CVMMethodBlock **p_mb)
{
    CVMObjectICell* groupICell = &arguments[0].j.r;
    CVMBool value = arguments[1].j.i;
    CVMD_fieldWriteInt(CVMID_icellDirect(ee, groupICell),
                       CVMoffsetOfjava_lang_ThreadGroup_saveThreadStarterClassFlag,
                       value);
    return CNI_VOID;
}

/*
 * Get Thread.threadStarterClass, a private field.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_getThreadStarterClass(CVMExecEnv* ee,
                                       CVMStackVal32 *arguments,
                                       CVMMethodBlock **p_mb)
{
    CVMObjectICell* threadICell = &arguments[0].j.r;
    CVMObject* backtrace;
    CVMD_fieldReadRef(CVMID_icellDirect(ee, threadICell),
                      CVMoffsetOfjava_lang_Thread_threadStarterClass,
                      backtrace);
    CVMID_icellSetDirect(ee, &arguments[0].j.r, backtrace);
    return CNI_SINGLE;
}

/*
 * Do a minimal GC if System.gc() called directly by MIDlet
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_gc(CVMExecEnv* ee,
                   CVMStackVal32 *arguments,
                   CVMMethodBlock **p_mb)
{
    CVMD_gcSafeExec(ee, {
            CVMgcRunGCMin(ee);
    });
    return CNI_VOID;
}          

#include "generated/offsets/java_lang_ClassLoader.h"

/*
 * Used to set java.lang.ClassLoader.noVerification.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_setNoVerification(CVMExecEnv* ee,
                                  CVMStackVal32 *arguments,
                                  CVMMethodBlock **p_mb)
{
    if (CVMglobals.classVerificationLevel != CVM_VERIFY_ALL) {
        CVMObjectICell* classLoaderICell = &arguments[0].j.r;
        CVMBool noVerification = arguments[1].j.i;
        CVMD_fieldWriteInt(CVMID_icellDirect(ee, classLoaderICell),
                           CVMoffsetOfjava_lang_ClassLoader_noVerification,
                           noVerification);
    }
    return CNI_VOID;
}

/******************************************************
 * CVM.nullifyRefsToDeadApp() can be used to forcefully 
 * nullify references to application objects after setting
 * the corresponding classloader using CVM.setDeadLoader().
 ******************************************************/

static void
nullifyRefCallBack(CVMObject** refPtr, void* data) {
    CVMObject* ref = *refPtr;
    CVMClassBlock *cb;

    if (ref == NULL || CVMobjectIsInROM(ref)) {
        return;
    }

    cb = CVMobjectGetClass(ref);

    if (data == NULL) {
        if (cb != CVMsystemClass(java_lang_Class)) {
            /* CVMconsolePrintf("===nullify application, size: %d\tClass: %C\n",
                    CVMobjectSizeGivenClass(*refPtr,cb), cb); */
            *refPtr = NULL;
        }
    } else {

        CVMClassLoaderICell* currLoader = CVMcbClassLoader(cb);
        CVMClassLoaderICell* deadLoader = (CVMClassLoaderICell*)data;
        CVMObject* deadLoaderObj;
        CVMObject* currLoaderObj;

        if (currLoader != NULL) {
            deadLoaderObj = (CVMObject*)deadLoader->ref_DONT_ACCESS_DIRECTLY;
            currLoaderObj = (CVMObject*)currLoader->ref_DONT_ACCESS_DIRECTLY;
            if (currLoaderObj == deadLoaderObj) {
                /* CVMconsolePrintf("===nullify size: %d\tClass: %C\n",
                        CVMobjectSizeGivenClass(*refPtr,cb), cb); */
                *refPtr = NULL;
            }
        }
    }
    return;
}


static CVMBool
iterateHeapCallBack(CVMObject* obj, CVMClassBlock* cb,
                    CVMUint32 size, void* data)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMGCOptions gcOpts = {
        /* isUpdatingObjectPointers */ CVM_FALSE,
        /* discoverWeakReferences   */ CVM_FALSE,
#if defined(CVM_DEBUG) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
        /* isProfilingPass          */ CVM_FALSE
#endif
    };
    CVMGCOptions *gcOptsPtr = &gcOpts;

    /* Here we might want to check if CVMcbClassLoader(cb) is the same
     * as the class loader passed in data, and perform different actions
     * for application objects, and objects of classes not loaded by the
     * application's class loader.
     * Nullifying all refernces in application classes, is not safe!
     * Nullifying references in none application objects is more risky
     * than nullifying references in application objects, but provides
     * better protection against system memory leak bugs. For now, we
     * nullify all heap references to application objects.
     */
    CVMobjectWalkRefsWithSpecialHandling(ee, gcOptsPtr, obj,
                                         CVMobjectGetClassWord(obj), {
        if (*refPtr != 0) {
            (*nullifyRefCallBack)(refPtr, data);
        }
    }, nullifyRefCallBack, data);
    return CVM_TRUE;
}

extern void
CVMgcScanStatics(CVMExecEnv *ee, CVMGCOptions* gcOpts,
                 CVMRefCallbackFunc callback, void* data);

static void
scanClassCallBack(CVMExecEnv* ee, CVMClassBlock* cb, void* data)
{
    CVMClassLoaderICell* currLoader = CVMcbClassLoader(cb); 
    CVMClassLoaderICell* deadLoader = (CVMClassLoaderICell*)data;
     if ((currLoader != NULL) && (deadLoader->ref_DONT_ACCESS_DIRECTLY ==
                                 currLoader->ref_DONT_ACCESS_DIRECTLY)) {

       /* 
        * Nullifying any static reference from application classes, is
        * highly benficial for application leaking by thread. The leaking
        * thread holds the class loader, and thus the classes, and thus
        * any static data.
        * Nullifying application statics is low risk.
        */
        CVMscanClassIfNeeded(ee, cb, nullifyRefCallBack, NULL);
    } else {
       /* 
        * Nullifying static references to application objects, is
        * a guard aginst system bugs. Use this for testing to reduce the likelihood
        * of such bugs remaining in the shipping product. Consider turning it off
        * in final product, since the risk of causing NPE in the system may outweigh
        * the potential benefit.
        */
        CVMscanClassIfNeeded(ee, cb, nullifyRefCallBack, data);
    }
}

int nullifyRefs = 0;

static CVMBool scanRefAction(CVMExecEnv *ee, void *data)
{
    CVMgcClearClassMarks(ee, NULL);

    /* scan preloader statics */
    /* Nullifying static references to application objects, is
     * a guard aginst system bugs. Use this for testing to reduce the likelihood
     * of such bugs remaining in the shipping product. Consider turning it off
     * in final product, since the risk of causing NPE in the system may outweigh
     * the potential benefit.
     */
    /* CVMconsolePrintf("===nullify preloader statics\n"); */
    {
        CVMAddr  numRefStatics = CVMpreloaderNumberOfRefStatics();
        CVMAddr* refStatics    = CVMpreloaderGetRefStatics();
        CVMwalkContiguousRefs(refStatics, numRefStatics,
                              nullifyRefCallBack,
                              data);
    }

    /* scan dynamic loaded class statics */
    /* CVMconsolePrintf("===nullify dynamic classes statics\n"); */
    /*CVMsetDebugFlags(CVM_DEBUGFLAG(TRACE_GCSCAN));*/
    CVMclassIterateAllClasses(ee, scanClassCallBack, data);
    /*CVMclearDebugFlags(CVM_DEBUGFLAG(TRACE_GCSCAN));*/

    /* iterate the heap objects */
    /* CVMconsolePrintf("===nullify heap references\n"); */
    CVMgcimplIterateHeap(ee, iterateHeapCallBack, data);

    return CVM_TRUE;
}

CNIEXPORT CNIResultCode
CNIsun_misc_CVM_nullifyRefsToDeadApp0(CVMExecEnv* ee,
                                CVMStackVal32 *arguments,
                                CVMMethodBlock **p_mb)
{
    CVMClassLoaderICell* deadLoader = (CVMClassLoaderICell*)&arguments[0].j.r;

    if (nullifyRefs) {

#ifdef CVM_JIT
        CVMD_gcSafeExec(ee, {
            CVMsysMutexLock(ee, &CVMglobals.jitLock);
        });
#endif

        CVMD_gcSafeExec(ee, {
            CVMsysMutexLock(ee, &CVMglobals.heapLock);

            CVMgcStopTheWorldAndDoAction(ee, deadLoader, NULL,
                scanRefAction, NULL, NULL, NULL);

            CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
        });

#ifdef CVM_JIT
        CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
    }

    return CNI_VOID;
}

/* Set the 'ignoreInterruptedException' flag in the target
 * thread's 'ee'. The VM will ignore the InterruptedException
 * handler in the target thread's non-system code.
 */
CNIEXPORT CNIResultCode
CNIsun_misc_CVM_ignoreInterruptedException(CVMExecEnv* ee,
                                CVMStackVal32 *arguments,
                                CVMMethodBlock **p_mb)
{
    CVMObjectICell *threadICell = &arguments[0].j.r;
    CVMJavaLong eetop;
    CVMExecEnv *targetEE;

    CVMD_gcSafeExec(ee, {
        CVMsysMutexLock(ee, &CVMglobals.threadLock);
    });

    CVMD_fieldReadLong(CVMID_icellDirect(ee, threadICell),
                       CVMoffsetOfjava_lang_Thread_eetop,
                       eetop);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetop);

    if (targetEE != NULL) {
        targetEE->ignoreInterruptedException = CVM_TRUE;
    }

    CVMD_gcSafeExec(ee, {
        CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    });

    return CNI_VOID;
}

