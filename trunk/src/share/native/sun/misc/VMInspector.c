/*
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

#ifdef CVM_INSPECTOR

#include "jni.h"
#include "jvm.h"

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/directmem.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/utils.h"

#include "javavm/include/porting/doubleword.h"

#include "generated/jni/sun_misc_VMInspector.h"

/*
    public static native void exit(int status);

    public static native boolean enableGC();
    public static native boolean disableGC();
    public static native boolean gcIsDisabled();

    // Misc debug utility functions:
    public static native Object addrToObject(long objAddr);

    // Object inspection utilities:
    public static native void dumpObject(long objAddr);
    public static native void dumpClassBlock(long cbAddr);
    public static native void dumpObjectReferences(long objAddr);
    public static native void dumpClassReferences(long cbAddr);
    public static native void dumpClassBlocks(String classname);

    // System info utilities:
    public static native void dumpSysInfo();
    public static native void dumpHeapSimple();
    public static native void dumpHeapVerbose();
    public static native void dumpHeapStats();

    // GC utilities:
    public static native void dumpObjectGCRoots(long objAddr);

    // Heap state capture utility:
    public static final int SORT_NONE = 0;
    public static final int SORT_BY_OBJ = 1;
    public static final int SORT_BY_OBJCLASS = 2;

    public static native void captureHeapState(String name);
    public static native void releaseHeapState(int id);
    public static native void releaseAllHeapState();
    public static native void listHeapStates();
    public static native void dumpHeapState(int id, int sortKey);
    public static native void compareHeapState(int id1, int id2);

    // Class utilities:
    public static native void listAllClasses();

    // Thread utilities:
    public static native void listAllThreads();
    public static native void dumpAllThreads();
    public static native void dumpStack(long eeAddr);
*/

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_exit(JNIEnv *env, jclass cls, jint status)
{
    CVMexit(status);
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_VMInspector_enableGC(JNIEnv *env, jclass cls)
{
    jboolean result = CVMgcEnableGC();
    return result;
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_VMInspector_disableGC(JNIEnv *env, jclass cls)
{
    jboolean result = CVMgcDisableGC();
    return result;
}

JNIEXPORT jboolean JNICALL 
Java_sun_misc_VMInspector_gcIsDisabled(JNIEnv *env, jclass cls)
{
    return CVMgcIsDisabled();
}

/* Purpose: Forces the GC to keep all objects alive or not. */
JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_keepAllObjectsAlive(JNIEnv *env, jclass cls,
					      jboolean keepAlive)
{
    CVMgcKeepAllObjectsAlive(keepAlive);
}

/*=========================================================================== 
 *  Misc debug utility functions: 
 */
JNIEXPORT jobject JNICALL
Java_sun_misc_VMInspector_addrToObject(JNIEnv *env, jclass cls, jlong objAddr)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMObject *obj = (CVMObject *)CVMlong2VoidPtr(objAddr);
    jobject retVal;

    if (!CVMgcIsDisabled()) {
        CVMthrowIllegalStateException(ee, NULL);
        return NULL;
    }
    if (!CVMgcIsValidObject(ee, obj)) {
        CVMthrowIllegalArgumentException(ee, NULL);
        return NULL;
    }

    /* NOTE: We create the localref with cls as the initial value to ensure
       that a localref is allocated.  This is because the JNI spec says that
       NewLocalRef() is to return a NULL if the initial value is NULL. */
    retVal =  (*env)->NewLocalRef(env, cls);
    CVMD_gcUnsafeExec(ee, {
        CVMID_icellSetDirect(ee, retVal, obj);
    });
    return retVal;
}

JNIEXPORT jlong JNICALL 
Java_sun_misc_VMInspector_objectToAddr(JNIEnv *env, jclass cls,
				       jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMObject *directObj;

    if (!CVMgcIsDisabled()) {
        CVMthrowIllegalStateException(ee, NULL);
        return 0;
    }

    CVMD_gcUnsafeExec(ee, {
	directObj = CVMID_icellDirect(ee, obj);
    });

    return CVMvoidPtr2Long(directObj);
}



/* NOTE: CVMdumpObject() already enforces that GC is disabled.  Hence we don't
   have to worry about the object pointer moving. */
JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpObject(JNIEnv *env, jclass cls, jlong objAddr)
{
    CVMObject *obj = (CVMObject *)CVMlong2VoidPtr(objAddr);
    CVMdumpObject(obj);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpClassBlock(JNIEnv *env, jclass cls, jlong cbAddr)
{
    CVMClassBlock *cb = (CVMClassBlock *)CVMlong2VoidPtr(cbAddr);
    CVMdumpClassBlock(cb);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpObjectReferences(JNIEnv *env, jclass cls,
					      jlong objAddr)
{
    CVMObject *obj = (CVMObject *)CVMlong2VoidPtr(objAddr);
    CVMdumpObjectReferences(obj);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpClassReferences(JNIEnv *env, jclass cls,
					     jstring classname)
{
    const char *classnameStr;
    jboolean isCopy = JNI_FALSE;
    classnameStr = (*env)->GetStringUTFChars(env, classname, &isCopy);
    CVMdumpClassReferences(classnameStr);
    (*env)->ReleaseStringUTFChars(env, classname, classnameStr);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpClassBlocks(JNIEnv *env, jclass cls,
 				        jstring classname)
{
    const char *classnameStr;
    jboolean isCopy = JNI_FALSE;
    classnameStr = (*env)->GetStringUTFChars(env, classname, &isCopy);
    CVMdumpClassBlocks(classnameStr);
    (*env)->ReleaseStringUTFChars(env, classname, classnameStr);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpSysInfo(JNIEnv *env, jclass cls)
{
    CVMconsolePrintf("***======================================\n");
    CVMdumpSysInfo();
    CVMconsolePrintf("***======================================\n");
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpHeapSimple(JNIEnv *env, jclass cls)
{
    CVMgcDumpHeapSimple();
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpHeapVerbose(JNIEnv *env, jclass cls)
{
    CVMgcDumpHeapVerbose();
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpHeapStats(JNIEnv *env, jclass cls)
{
    CVMgcDumpHeapStats();
}

/* NOTE: CVMdumpObjectGCRoots() already enforces that GC is disabled.
   Hence we don't have to worry about the object pointer moving. */
JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpObjectGCRoots(JNIEnv *env, jclass cls,
					   jlong objAddr)
{
    CVMObject *obj = (CVMObject *)CVMlong2VoidPtr(objAddr);
    CVMdumpObjectGCRoots(obj);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_captureHeapState(JNIEnv *env, jclass cls,
					  jstring name)
{
    const char *nameStr = NULL;
    jboolean isCopy = JNI_FALSE;
    if (name != NULL) {
        nameStr = (*env)->GetStringUTFChars(env, name, &isCopy);
    }
    CVMgcCaptureHeapState(nameStr);
    if (name != NULL) {
        (*env)->ReleaseStringUTFChars(env, name, nameStr);
    }
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_releaseHeapState(JNIEnv *env, jclass cls, jint id)
{
    CVMgcReleaseHeapState(id);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_releaseAllHeapState(JNIEnv *env, jclass cls)
{
    CVMgcReleaseAllHeapState();
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_listHeapStates(JNIEnv *env, jclass cls)
{
    CVMgcListHeapStates();
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpHeapState(JNIEnv *env, jclass cls, jint id,
				       jint sortKey)
{
    CVMgcDumpHeapState(id, sortKey);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_compareHeapState(JNIEnv *env, jclass cls, jint id1,
					  jint id2)
{
    CVMgcCompareHeapState(id1, id2);
}

static void
listClassBlocksCallback(CVMExecEnv *ee, CVMClassBlock *cb, void *data)
{
    CVMconsolePrintf("   cb %p %C\n", cb, cb);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_listAllClasses(JNIEnv *env, jclass cls)
{
    CVMExecEnv *ee = CVMgetEE();
    
    if (!CVMsysMutexTryLock(ee, &CVMglobals.heapLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the thread lock!\n");
        return;
    }

    CVMconsolePrintf("List of all loaded classes:\n");
    CVMclassIterateAllClasses(ee, listClassBlocksCallback, NULL);

    CVMsysMutexUnlock(ee, &CVMglobals.heapLock);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_listAllThreads(JNIEnv *env, jclass cls)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMUint32 numberOfThreads = 0;

    if (!CVMsysMutexTryLock(ee, &CVMglobals.threadLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the thread lock!\n");
        return;
    }

    CVMconsolePrintf("List of all threads:\n");
    CVM_WALK_ALL_THREADS(ee, threadEE, {
       CVMconsolePrintf("    Thread %d: ee 0x%x priority %d tick %d\n",
			threadEE->threadID, threadEE, threadEE->priority,
			threadEE->tickCount);
	numberOfThreads++;
    });
    if (numberOfThreads == 0) {
        CVMconsolePrintf("   none\n");
    }

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpAllThreads(JNIEnv *env, jclass cls)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMUint32 numberOfThreads = 0;

    if (!CVMsysMutexTryLock(ee, &CVMglobals.threadLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the thread lock!\n");
        return;
    }

    CVMconsolePrintf("\nStart thread dump:\n");
    CVMconsolePrintf("***======================================\n");
    CVM_WALK_ALL_THREADS(ee, threadEE, {
       CVMconsolePrintf("Thread %d: ee 0x%x priority %d tick %d\n",
			threadEE->threadID, threadEE, threadEE->priority,
			threadEE->tickCount);
       CVMdumpStack(&threadEE->interpreterStack,0,0,0);
       CVMconsolePrintf("***======================================\n");
	numberOfThreads++;
    });
    if (numberOfThreads == 0) {
        CVMconsolePrintf("   none\n");
    }
    CVMconsolePrintf("End thread dump\n\n");

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

JNIEXPORT void JNICALL 
Java_sun_misc_VMInspector_dumpStack(JNIEnv *env, jclass cls, jlong eeAddr)
{
    CVMExecEnv *ee = CVMgetEE();
    CVMExecEnv *targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eeAddr);
    CVMBool eeFound = CVM_FALSE;

    if (!CVMsysMutexTryLock(ee, &CVMglobals.threadLock)) {
        CVMconsolePrintf("Cannot acquire needed locks without blocking -- "
                         "another thread already owns the thread lock!\n");
        return;
    }

    if (ee == targetEE) {
	eeFound = CVM_TRUE;
    } else {
	CVM_WALK_ALL_THREADS(ee, threadEE, {
	    if (threadEE == targetEE) {
	        eeFound = CVM_TRUE;
            }
	});
    }

    if (!eeFound) {
	CVMconsolePrintf("Cannot dump stack: ee 0x%x is not a thread\n",
			 targetEE);
	CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
	return;
    }

    CVMconsolePrintf("Stack for thread EE 0x%x:\n", targetEE);
    CVMdumpStack(&targetEE->interpreterStack,0,0,0);

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

#endif /* CVM_INSPECTOR */
