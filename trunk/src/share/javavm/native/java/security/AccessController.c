/*
 * @(#)AccessController.c	1.28 06/10/10
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
#include "javavm/include/globalroots.h"
#include "generated/offsets/java_lang_Thread.h"

CNIEXPORT CNIResultCode
CNIjava_security_AccessController_computeContext(CVMExecEnv* ee,
    CVMStackVal32 *arguments, CVMMethodBlock **p_mb)
{
    CVMInt32 n = 0;
    CVMArrayOfBooleanICell *isPrivilegedRef =
	(CVMArrayOfBooleanICell *)&arguments[0].j.r;
    CVMArrayOfRefICell *contextRef =
	(CVMArrayOfRefICell *)&arguments[1].j.r;
    CVMObjectICell *privContext = NULL;
    CVMObjectICell *pd, *prevPD = NULL;
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);
    CVMBool isPrivileged = CVM_FALSE;
    CVMFrameIterator iter;

    /* doPrivileged frames */
    CVMMethodBlock *doPrivilegedAction1Mb =
	CVMglobals.java_security_AccessController_doPrivilegedAction1;
    CVMMethodBlock *doPrivilegedExceptionAction1Mb =
	CVMglobals.java_security_AccessController_doPrivilegedExceptionAction1;
    CVMMethodBlock *doPrivilegedAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedAction2;
    CVMMethodBlock *doPrivilegedExceptionAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2;

    CVMMethodBlock *mb;
    CVMClassBlock *cb;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    CVMframeIterateInit(&iter, frame);

    while (CVMframeIterateNext(&iter)) {
	mb = CVMframeIterateGetMb(&iter);
	cb = CVMmbClassBlock(mb);
	pd = CVMcbProtectionDomain(cb);
	if (pd != NULL) {
	    CVMBool isSamePD = CVM_FALSE;
	    if (prevPD != NULL) {
		CVMObject *prevPDObject = CVMID_icellDirect(ee, prevPD);
		CVMObject *pdObject = CVMID_icellDirect(ee, pd);
		isSamePD = (prevPDObject == pdObject);
	    }    
	    if (!isSamePD) {
		++n;
		prevPD = pd;
	    }    
	}
	if (mb == doPrivilegedAction2Mb ||
	    mb == doPrivilegedExceptionAction2Mb) {
	    /* For the reflection case, we still need the doPrivileged
	     * frame. During the quickening process, there is no way
	     * to turn doPrivileged call to special opcode and to mark 
	     * the run frame if the doPrivileged call is invoked by
	     * reflection. In order to find the caller, doPrivileged 
	     * frame should exist.
	     */
	    /*
	     * context is the 2rd argument to doPrivileged.
	     * Alternatively, we could probably overload
	     * CVMframeReceiverObj() because doPrivileged
	     * is not synchronized.
	     */
	    {
                CVMStackVal32 *locals = CVMframeIterateGetLocals(&iter);
		privContext = &locals[1].j.r;
	    }

	    CVMframeIterateNext(&iter);
	    {
		/* This also handles the inlined case */
		CVMMethodBlock *mb1 = CVMframeIterateGetMb(&iter);
		if (mb1 == doPrivilegedAction1Mb ||
		    mb1 == doPrivilegedExceptionAction1Mb ) {
		    CVMframeIterateNext(&iter);
		    mb1 = CVMframeIterateGetMb(&iter);
		}
		pd = CVMcbProtectionDomain(CVMmbClassBlock(mb1));
	    }
	    isPrivileged = CVM_TRUE;
	}

	if (isPrivileged) {
	    if (pd != NULL) {
		CVMBool isSamePD = CVM_FALSE;
		if (prevPD != NULL) {
		    CVMObject *prevPDObject = CVMID_icellDirect(ee, prevPD);
		    CVMObject *pdObject = CVMID_icellDirect(ee, pd);
		    isSamePD = (prevPDObject == pdObject);
		}
		if (!isSamePD)
		    ++n;
	    }
	    break;
	}
    }
    {
	CVMArrayOfBoolean *isPrivArr = CVMID_icellDirect(ee, isPrivilegedRef);
	CVMArrayOfRef *ctxArr = CVMID_icellDirect(ee, contextRef);
	CVMObject *ctx = (privContext == NULL) ? NULL :
	    CVMID_icellDirect(ee, privContext);

	CVMD_arrayWriteRef(ctxArr, 0, ctx);
	CVMD_arrayWriteBoolean(isPrivArr, 0, isPrivileged);
    }
    arguments[0].j.i = n;
    return CNI_SINGLE;
}

CNIEXPORT CNIResultCode
CNIjava_security_AccessController_fillInContext(CVMExecEnv* ee,
    CVMStackVal32 *arguments, CVMMethodBlock **p_mb)
{
    CVMArrayOfRefICell *context =
	(CVMArrayOfRefICell *)&arguments[0].j.r;
#ifdef CVM_DEBUG_ASSERTS
    CVMJavaInt count = arguments[1].j.i;
#endif
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);
    CVMBool isPrivileged = CVM_FALSE;
    CVMObjectICell *prevPD = NULL;
    CVMInt32 i = 0;
    CVMFrameIterator iter;
    CVMArrayOfRef *ctx;
    CVMObject *pdObject,*prevPDObject;

    /* doPrivileged frames */
    CVMMethodBlock *doPrivilegedAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedAction2;
    CVMMethodBlock *doPrivilegedExceptionAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2;
    CVMMethodBlock *doPrivilegedAction1Mb =
        CVMglobals.java_security_AccessController_doPrivilegedAction1;
    CVMMethodBlock *doPrivilegedExceptionAction1Mb =
        CVMglobals.java_security_AccessController_doPrivilegedExceptionAction1;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    CVMframeIterateInit(&iter, frame);

    while (CVMframeIterateNext(&iter)) {
	CVMObjectICell *pd;
	CVMMethodBlock *mb = CVMframeIterateGetMb(&iter);
	CVMClassBlock *cb = CVMmbClassBlock(mb);
	
	pd = CVMcbProtectionDomain(cb);
	if (pd != NULL) {
	    CVMBool isSamePD = CVM_FALSE;
	    if (prevPD != NULL) {
		prevPDObject = CVMID_icellDirect(ee, prevPD);
		pdObject = CVMID_icellDirect(ee, pd);
		isSamePD = (prevPDObject == pdObject);
	    }
	    if (!isSamePD) {
		ctx = CVMID_icellDirect(ee, context);
		pdObject = CVMID_icellDirect(ee, pd);
		CVMD_arrayWriteRef(ctx, i, pdObject);
		++i;
		prevPD = pd;
	    }
	}      

	if (mb == doPrivilegedAction2Mb ||
	    mb == doPrivilegedExceptionAction2Mb) {
	    CVMMethodBlock *mb1;
	    CVMframeIterateNext(&iter);
	    mb1 = CVMframeIterateGetMb(&iter);
	    if (mb1 == doPrivilegedExceptionAction1Mb ||
		mb1 == doPrivilegedAction1Mb) {
		CVMframeIterateNext(&iter);
		mb1 = CVMframeIterateGetMb(&iter);
	    }
	    pd = CVMcbProtectionDomain(CVMmbClassBlock(mb1));
	    isPrivileged = CVM_TRUE;
	}

	if (isPrivileged) {
	    if (pd != NULL) {
	        CVMBool isSamePD = CVM_FALSE;
                if (prevPD != NULL) {
                    prevPDObject = CVMID_icellDirect(ee, prevPD);
                    pdObject = CVMID_icellDirect(ee, pd);
                    isSamePD = (prevPDObject == pdObject);
                }
                if (!isSamePD) {
                    ctx = CVMID_icellDirect(ee, context);
                    pdObject = CVMID_icellDirect(ee, pd);
                    CVMD_arrayWriteRef(ctx, i, pdObject);
                    ++i;
                }
	    }
	    break;
	}
    }
    CVMassert(i == count);
    return CNI_VOID;
}

CNIEXPORT CNIResultCode
CNIjava_security_AccessController_getInheritedAccessControlContext(
    CVMExecEnv* ee, CVMStackVal32 *arguments, CVMMethodBlock **p_mb)
{
    CVMThreadICell *threadRef = CVMcurrentThreadICell(ee);

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    {
	CVMObject *thread = CVMID_icellDirect(ee, threadRef);
	CVMObject *obj;

	CVMD_fieldReadRef(thread,
	    CVMoffsetOfjava_lang_Thread_inheritedAccessControlContext,
	    obj);
	CVMID_icellSetDirect(ee, &arguments[0].j.r, obj);
	return CNI_SINGLE;
    }
}
