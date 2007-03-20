/*
 * @(#)jit_arch.c	1.12 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

/* Purpose: Flush I & D caches after writing compiled code. */
void
CVMJITflushCache(void* begin, void* end)
{
    /*
     * The sa110 cache line size is 64 bytes, so we make sure we include
     * all bytes starting from the address of the first byte in the cacheline
     * that "begin" is in, up to the address of the first byte of the cacheline
     * that follows the one "end" is in. This fixes a bug in the linux kernal
     * that causes it to not flush the cache line that "end" is in if "begin"
     * is not 64-bit aligned. (The alignment of "end" is probably not 
     * necessary.)
     *
     * NOTE: "end" is exclusive of the range to flush. The byte at "end"
     * is not flushed, but the byte before it is.
     */
#undef  DCACHE_LINE_SIZE
#define DCACHE_LINE_SIZE 64
    /* round down to start of cache line */
    begin = (void*) ((unsigned long)begin & ~(DCACHE_LINE_SIZE-1));
    /* round up to start of next cache line */
    end = (void*) (((unsigned long)end + (DCACHE_LINE_SIZE-1))
		   & ~(DCACHE_LINE_SIZE-1));
#undef DCACHE_LINE_SIZE
    {
	BOOL status = FlushInstructionCache(GetCurrentProcess(),
	    begin, (char *)end - (char *)begin);
    }
}

#include "portlibs/jit/risc/include/porting/ccmrisc.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include <excpt.h>

extern CVMMethodBlock *
CVMgoNative0(CVMObject *exceptionObject, CVMExecEnv *ee,
             CVMCompiledFrame *jfp, CVMUint8 *pc);

static DWORD
check_pc(struct _EXCEPTION_POINTERS *ep, DWORD code, DWORD *addr, DWORD *pc)
{
    if (code == EXCEPTION_ACCESS_VIOLATION &&
       CVMJITcodeCacheInCompiledMethod((CVMUint8*)ep->ContextRecord->Pc))
    {
	/* Coming from compiled code. */
	/* Branch and link to throw null pointer exception glue */
	ep->ContextRecord->Lr = ep->ContextRecord->Pc + 4;
	ep->ContextRecord->Pc =
           (ULONG)CVMCCMruntimeThrowNullPointerExceptionGlue;
	return EXCEPTION_CONTINUE_EXECUTION;
    } else {
	*pc = ep->ContextRecord->Pc;
	*addr = (DWORD)ep->ExceptionRecord->ExceptionAddress;
	return EXCEPTION_EXECUTE_HANDLER;
    }
}

CVMMethodBlock *
CVMJITgoNative(CVMObject *exceptionObject, CVMExecEnv *ee,
    CVMCompiledFrame *jfp, CVMUint8 *pc)
{
    DWORD epc, eaddr;
    __try {
	return CVMgoNative0(exceptionObject, ee, jfp, pc);
    } __except (check_pc(_exception_info(), _exception_code(), &eaddr, &epc)) {
	/* We could also implement CVMexitNative with exceptions */
	NKDbgPrintfW(TEXT("exception %x in compiled code at pc %x addr %x\n"),
	    _exception_code(), epc, eaddr);
	return 0;
    }
}
