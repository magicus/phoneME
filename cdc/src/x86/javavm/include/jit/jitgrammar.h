/*
 * @(#)jitgrammar.h	1.4 03/11/14
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */

#ifndef _INCLUDED_JITGRAMMAR_H
#define _INCLUDED_JITGRAMMAR_H

#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jitirblock.h"
/* SVMC_JIT d022609 (ML) 2004-05-13. renamed file such that clients
   in `share' part can refer to it transparently. */
/* #include "portlibs/jit/risc/include/porting/jitriscemitter.h" */
#include "javavm/include/jit/jitarchemitter.h"

#include "javavm/include/jit/jitregman.h"

/**************************************************************
 * Data structures for the codegen-time expression/semantic stack.
 **************************************************************/

#ifdef CVM_DEBUG_ASSERTS
enum CVMJITStackTag {
    CVMJITStackTagConstant32,
    CVMJITStackTagConstantAddr,
    CVMJITStackTagResource,
    CVMJITStackTagALURhs,
    CVMJITStackTagMemSpec,
    CVMJITStackTagAddress
};
#endif

struct CVMJITStackElement {
#ifdef CVM_DEBUG_ASSERTS
    enum CVMJITStackTag tag;
#endif
    union {
        CVMInt32      i;
        CVMAddr       a;
        CVMRMResource *r;
        CVMCPUALURhs  *aluRhs;
        CVMCPUMemSpec *memSpec;
        CVMAddr       p;
    }u;
};

#ifdef CVM_JIT_GENERATE_GC_RENDEZVOUS
/*
 * CVMJITcheckGC is called by regman to emit a gc checkpoint.
 */
extern void
CVMJITcheckGC(CVMJITCompilationContext* con, CVMJITIRBlock *b);
#endif /* CVM_JIT_GENERATE_GC_RENDEZVOUS */

#endif
