/*
 * @(#)classinitialize.c	1.50 06/10/10
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
#include "javavm/include/preloader.h"

#include "javavm/export/jni.h"

static int
CVMprivateClassInit(CVMExecEnv* ee, CVMClassBlock* cb, CVMMethodBlock **p_mb);

/* 
 * Perform the class initialization as described in the VM spec,
 * 2nd Edition (2.17.5). Link the class if necessary.
 *
 * This is the public version that will use JNI to call 
 * Class.runStaticInitializers.
 */
CVMBool
CVMclassInit(CVMExecEnv* ee, CVMClassBlock* cb)
{
    CVMBool res = (CVMprivateClassInit(ee, cb, 0) == 0);
    if (res == CVM_TRUE) {
        /* Only try to resolve the cp entries if the 
         * initialization succeeded.
         */
        CVMcpResolveCbEntriesWithoutClassLoading(ee, cb);
    }
    return res;
}

/* 
 * Perform the class initialization as described in the VM spec,
 * 2nd Edition (2.17.5). Link the class if necessary.
 *
 * This is the version that interpreter loop will use to intialize
 * classes. It avoids C recursion by not using JNI to invoke
 * Class.runStaticInitializers. Instead it munges the interpreter
 * state so that upon return the interperer will start executiong 
 * in Class.runStaticInitializers.
 */
int
CVMclassInitNoCRecursion(CVMExecEnv* ee, CVMClassBlock* cb,
			 CVMMethodBlock **p_mb)
{
    int res = CVMprivateClassInit(ee, cb, p_mb);
    if (res != -1) {
        /* Only try to resolve the cp entries if the 
         * initialization succeeded.
         */
        CVMcpResolveCbEntriesWithoutClassLoading(ee, cb);
    }
    return res;
}

/*
 * Perform the class initialization as described in the VM spec,
 * 2nd Edition (2.17.5). Link the class if necessary.
 *
 * This version supports both CVMclassInit and CVMclassInitNoCRecursion.
 * If p_mb == 0, then is uses JNI to invoke Class.runStaticInitializers.
 *
 * If p_mb != 0 then it avoids C recursion by pushing a transition
 * frame that knows how to invoke Class.runStaticInitializers and then
 * munges the interpreter state so that upon return the interpeter 
 * starts executing at the beginning of Class.runStaticInitializers.
 *
 * We could have avoided the transition frame and just setup a JavaFrame
 * for Class.runStaticInitializers, but this would require some
 * extra logic in handle_return, which would slow down all method returns.
 *
 * Please note that in one case Class.runStaticInitializers is actually
 * invoked from here and in other caes the interpreter state is modified
 * so that upon return the interpreter will start executing 
 * Class.runStaticInitializers.
 * 
 * RESULT if p_mb != NULL:
 *   0 - No action taken. Class is already initialized.
 *   1 - Transition frame successfully pushed.
 *  -1 - An error occured. Exception was thrown.
 * 
 * RESULT if p_mb == NULL:
 *   0 - Initialization successful or class is already initialized 
 *  -1 - An error occured. Exception was thrown.
 */
static int
CVMprivateClassInit(CVMExecEnv* ee, CVMClassBlock* cb, CVMMethodBlock **p_mb)
{
    if (!CVMcbInitializationNeeded(cb, ee)) {
	/* nothing to do */
	return 0;
    }

    CVMtraceClinit(("[InitClass() called for %C]\n", cb));

    CVMassert(CVMD_isgcSafe(ee));

    /* Don't need to initialize or verify primitive or array classes */
    if (CVMcbIs(cb, PRIMITIVE) || CVMisArrayClass(cb)) {
	CVMcbSetInitializationDoneFlag(ee, cb);
	return 0;
    } else {
	/* Get the mb for Class.runStaticInitializers. */
	CVMMethodBlock* mb = CVMglobals.java_lang_Class_runStaticInitializers;

#ifdef CVM_CLASSLOADING
	/*
	 * Link the class hierarchy.
	 */
	if (!CVMclassLink(ee, cb, CVM_FALSE)) {
	    return -1;
	}
#endif

	if (!CVMcbHasStaticsOrClinit(cb)) {
	    /* If this class doesn't have a static initializer, then we're done.
	     *
	     * WARNING: It is not safe to do this until after the LINKED flag
	     * has been set.  But the LINKED flag should be set by now.
	     */
	    CVMassert(CVMcbCheckRuntimeFlag(cb, LINKED));
	    CVMcbSetInitializationDoneFlag(ee, cb);
	    return 0;
	}

	if (p_mb != NULL) {
            CVMBool success = CVM_TRUE;
	    /* 
	     * We need to do the special invocation of runStaticInitializers 
	     * that avoids C recursion. This section of code must run gcUnsafe
	     * because of the stack manipulation it does.
	     */
	    CVMD_gcUnsafeExec(ee, {
		CVMFrame* frame = (CVMFrame *)CVMpushTransitionFrame(ee, mb);
                if (frame != NULL) {
                    /* Push Class object of the class that we want to run
                     * static initializers on. */
                    CVMID_icellAssignDirect(ee, &frame->topOfStack->j.r,
                                            CVMcbJavaInstance(cb));
                    ++frame->topOfStack;
                    *p_mb = mb;
                } else {
                    success = CVM_FALSE;   /* exception already thrown */
                }
	    });
            return success ? 1 : -1;
	} else {
	    /* This is a normal invocation of runStaticInitializers that
	     * just uses JNI to enter the interpreter.
	     */
	    JNIEnv*  env = CVMexecEnv2JniEnv(ee);
	    int retVal;

	    /* Call Class.runStaticInitializers. We need to disable remote
	     * exceptions during this call, since we will be executing
	     * java code that we don't want to abort if a remote
	     * exception occurrs.
	     */
	    CVMdisableRemoteExceptions(ee);
	    (*env)->CallVoidMethod(env, CVMcbJavaInstance(cb), mb);
	    if (CVMlocalExceptionOccurred(ee)) {
		retVal = -1;
	    } else {
		retVal = 0;
	    }
	    CVMenableRemoteExceptions(ee);
	    return retVal;
	}
    }
}
