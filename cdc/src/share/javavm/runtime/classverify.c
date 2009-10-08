/*
 * @(#)classverify.c	1.22 06/10/10
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

#ifdef CVM_CLASSLOADING

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/typeid.h"
#include "javavm/include/globals.h"
#include "javavm/include/verify.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "generated/offsets/java_lang_ClassLoader.h"

/*
 * Returns one of the VERIFY options.
 */
CVMInt32
CVMclassVerificationSpecToEncoding(char* kind)
{
    if (!strcmp(kind, "all"))
	return CVM_VERIFY_ALL;
    else if (!strcmp(kind, "remote"))
	return CVM_VERIFY_REMOTE;
    else if (!strcmp(kind, "none"))
	return CVM_VERIFY_NONE;
    else {
	return CVM_VERIFY_UNRECOGNIZED;
    }	    
}


/*
 * CVMloaderNeedsVerify - returns TRUE if the classloader requires
 * verification of its loaded classes.
 *
 * verifyTrusted should be set true if verification must be done even
 * for trusted class loaders when the verification level is REMOTE.
 */
CVMBool
CVMloaderNeedsVerify(CVMExecEnv* ee, CVMClassLoaderICell* loader,
                     CVMBool verifyTrusted) {
#ifdef CVM_TRUSTED_CLASSLOADERS	
    return CVM_FALSE;
#else
    switch (CVMglobals.classVerificationLevel) {
    case CVM_VERIFY_ALL: {
        return CVM_TRUE;
    }

    case CVM_VERIFY_NONE: {
        return CVM_FALSE;
    }

    case CVM_VERIFY_REMOTE: {
        /* We don't need to verify the NULL class loader. */
        if (loader == NULL) {
            return CVM_FALSE;
        }
        
        /* We don't need to verify if ClassLoader.noVerification is true */
        {
            CVMBool noVerification;
            CVMID_fieldReadInt(ee, loader, 
                               CVMoffsetOfjava_lang_ClassLoader_noVerification,
                               noVerification);
            if (noVerification) {
                return CVM_FALSE;
            }
        }
        
        if (verifyTrusted) {
            /* Even if the class loader is trusted, we must verify */
            return CVM_TRUE;
        }
        
        /* We don't have to verify if the class loader is trusted */
        return !CVMisTrustedClassLoader(ee, loader);
    }
        
    default: {
        CVMassert(CVM_FALSE);
        return CVM_FALSE;
    }
    } /* switch */
#endif /* CVM_TRUSTED_CLASSLOADERS */
}

CVMBool
CVMclassVerify(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine)
{
#ifdef CVM_TRUSTED_CLASSLOADERS	
    CVMcbSetRuntimeFlag(cb, VERIFIED);
    return CVM_TRUE;
#else
    if (CVMcbCheckRuntimeFlag(cb, VERIFIED)) {
	return CVM_TRUE;
    }
    CVMtraceClassLoading(("CL: Verifying class %C.\n", cb));

    /* Do some quick integrity tests on interfaces. */
    if (CVMcbIs(cb, INTERFACE)) { 
        /* Interface's superclass must be java/lang/Object. */
        CVMassert((CVMcbSuperclass(cb) != NULL) &&
                  (CVMcbSuperclass(cb) == CVMsystemClass(java_lang_Object)));

        /* The following check for interface methods is not needed. The
         * method modifier flags are checked by the class format checker 
         * (in verifyformat.c). A ClassFormatError instead of VerifyError
         * is thrown for incorrect flags.
         */
#if 0
	for (i = 0; i < CVMcbMethodCount(cb); i++) {
	    CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	    if (CVMmbIs(mb, STATIC)) {
		if (!CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb))) { 
		    /* Only internal interface methods can be static */
		    CVMthrowVerifyError(
                        ee, "Illegal static method %M in interface %C",
			mb, cb);
		    goto failed;
		}
	    }
	}
#endif
    } else if (CVMcbSuperclass(cb) != NULL) { 
	/*EMPTY*/
        /* Ensuring that final classes are not subclassed, and that
	 * final methods are not overridden.
	 * DEFERRED into PrepareMethods
	 */
    } else if (cb != CVMsystemClass(java_lang_Object)) {
	CVMthrowVerifyError(ee, "Class %C does not have superclass", cb);
	goto failed;
    }
	
    {
        jint       result;
        char          message[256];
	message[0] = 0;
	result = 
	    VerifyClass(ee, cb, message, sizeof(message), isRedefine);
        if (result < 0) {
	    if (!CVMexceptionOccurred(ee)) {
	        if (result == -1) {
		    CVMthrowVerifyError(ee, "%s", message);
	        } else if (result == -2) {
	            CVMthrowClassFormatError(ee, "%s", message);
                }
	    }
	    goto failed;
	}
    }
    CVMcbSetRuntimeFlag(cb, ee, VERIFIED);
    CVMtraceClassLoading(("CL: Done verifying class %C.\n", cb));
    return CVM_TRUE;

 failed:
    CVMtraceClassLoading(("CL: Failed verifying class %C.\n", cb));
    return CVM_FALSE;
#endif /* CVM_TRUSTED_CLASSLOADERS */
}

#endif /* CVM_CLASSLOADING */
