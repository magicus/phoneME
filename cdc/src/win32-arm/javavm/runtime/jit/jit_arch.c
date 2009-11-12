/*
 * @(#)jit_arch.c	1.12 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"

#include "javavm/include/assert.h"
#include "javavm/include/globals.h"
#include "javavm/include/porting/jit/jit.h"


#ifdef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE

extern char CVMcodeCacheStart[];
extern char CVMcodeCacheEnd[];

/*
 * We must use a static code cache, otherwise WinCE exceptions
 * don't work, which would mean that we can't use trap-based
 * checks.
 */
void *
CVMJITallocCodeCache(CVMSize *size)
{
    *size = CVMcodeCacheEnd - CVMcodeCacheStart;
    return CVMcodeCacheStart;
}

void
CVMJITfreeCodeCache(void *start)
{
}

#endif

#include "portlibs/jit/risc/include/porting/ccmrisc.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include <excpt.h>

extern CVMMethodBlock *
CVMgoNative0(CVMObject *exceptionObject, CVMExecEnv *ee,
             CVMCompiledFrame *jfp, CVMUint8 *pc);

static DWORD
check_pc(struct _EXCEPTION_POINTERS *ep, DWORD code, DWORD *addr, DWORD *pc)
{
    CVMUint8 *fault_pc = (CVMUint8 *)ep->ContextRecord->Pc;
    if (code == EXCEPTION_ACCESS_VIOLATION) {
	if (CVMJITcodeCacheInCompiledMethod(fault_pc)) {
	    /* Coming from compiled code. */
	    /* Branch and link to throw null pointer exception glue */
	    ep->ContextRecord->Lr = ep->ContextRecord->Pc + 4;
	    ep->ContextRecord->Pc =
	       (ULONG)CVMCCMruntimeThrowNullPointerExceptionGlue;
	    return EXCEPTION_CONTINUE_EXECUTION;
	} else if (CVMJITcodeCacheInCCM(fault_pc)) {
	    /* Coming from CCM code. */
	    /* Branch to throw null pointer exception glue */
	    ep->ContextRecord->Pc =
	       (ULONG)CVMCCMruntimeThrowNullPointerExceptionGlue;
	    return EXCEPTION_CONTINUE_EXECUTION;
	}
    }

    *pc = ep->ContextRecord->Pc;
    *addr = (DWORD)ep->ExceptionRecord->ExceptionAddress;
    return EXCEPTION_EXECUTE_HANDLER;
}

CVMMethodBlock *
CVMJITgoNative(CVMObject *exceptionObject, CVMExecEnv *ee,
    CVMCompiledFrame *jfp, CVMUint8 *pc)
{
    DWORD epc = 0, eaddr = 0;
    __try {
	return CVMgoNative0(exceptionObject, ee, jfp, pc);
    } __except (check_pc(_exception_info(), _exception_code(), &eaddr, &epc)) {
	/* We could also implement CVMexitNative with exceptions */
	NKDbgPrintfW(TEXT("exception %x in compiled code at pc %x addr %x\n"),
	    _exception_code(), epc, eaddr);
	return 0;
    }
}
