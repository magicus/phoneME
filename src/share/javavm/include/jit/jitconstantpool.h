/*
 * @(#)jitconstantpool.h	1.21 06/10/10
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
 * Run-time constant pools are required by many target machines which
 * cannot easily form 32- or 64-bit constants using in-line immediates
 * or simple instruction combinations. Such constants typically show up as
 * - large integers, bit patterns or floating-point numbers explicit in
 *   the computation.
 * - large numbers (such as large offsets from PC or an address base
 *   register) implicit in the computation
 * - absolute addresses of external program or data objects.
 *
 * In the case of external program objects (entry points), we may wish
 * to use an absolute address for access, even though the object appears
 * to be reachable using PC-relative addressing. This is to allow relative
 * movement of the generated program without relocation of the referenct to
 * a stationary entry point.
 *
 * Addressing of run-time constants depends on the addressing modes
 * available on the target processor. Popular choices include
 * - PC-relative addressing (ARM)
 * - dedicated base register loaded at method entry (SPARC PIC?)
 * - addressing as fixed memory, with optional CSE elimination of
 *   address formation (SPARC).
 *
 * The intent of this interface (and the generic part of its implementation)
 * is to support a generic style of constant-pool management.
 */


/*
 * An entry in the constant pool.
 */
struct CVMJITConstantEntry {
    CVMUint8            isCachedConstant; /* cached value */
    CVMUint8	        hasBeenEmitted;   /* true if already emitted */
    CVMInt32		address;         /* only if hasBeenEmitted */
    CVMJITFixupElement* references;      /* ...otherwise */
    enum { shortConst, longConst } constSize;
    union{
	CVMInt32     single;
	CVMJavaVal64 longVal;
    } v;
#ifdef CVM_TRACE_JIT
    const char*		printName;
#endif
    CVMJITConstantEntry* next;
};

/* get the first CVMJITConstantEntry */
#define GET_CONSTANT_POOL_LIST_HEAD(con) ((con)->constantPool)

/* Initial value of earliestConstantRefPC */
#define MAX_LOGICAL_PC 0x7FFFFFFF

/*
 * An instruction's logical address is relative to the origin of the
 * main code-generation buffer and is used for calculating the PC-relative
 * offset to a constant, should that be desired.
 *
 * An instruction's physical address is where the image can be
 * found for later fixup, as required.
 *
 * This function returns the logical address of the constant if it is already
 * found in the constant pool and is within reach of the current logical PC.
 * Else, it returns 0, but sets up the instruction at the current logical PC
 * to be patched once the contant is dumped.
 */

CVMInt32
CVMJITgetRuntimeConstantReference32(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress, CVMInt32 cv);

CVMInt32
CVMJITgetRuntimeConstantReference64(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress, CVMJavaVal64* cvp);

#ifdef CVM_JIT_USE_FP_HARDWARE
CVMInt32
CVMJITgetRuntimeFPConstantReference32(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress, CVMInt32 v);

CVMInt32
CVMJITgetRuntimeFPConstantReference64(
    CVMJITCompilationContext* con,
    CVMInt32 logicalAddress, CVMJavaVal64* vp);
#endif

/*
 * This function creates a cached constant reference in the constant pool
 * and emits instructions to load the address of the constant into
 * a platform specific register. These cached constants are used for
 * things like fast instanceof and checkcast checks against a previously
 * successful class.
 */
#ifdef IAI_CACHEDCONSTANT
void
CVMJITgetRuntimeCachedConstant32(
    CVMJITCompilationContext* con);
#endif

/*
 * The timing of dumping parts of the constant pool is likewise very
 * target dependent. In the case of PC-relative constant pools, this
 * should be done as frequently as seems reasonable, such as after
 * every absolute branch. If you're building a single constant pool
 * then just do it once.
 */

void
CVMJITdumpRuntimeConstantPool(CVMJITCompilationContext* con,
			      CVMBool forceDump);

/*
 * Periodic check point to see if the constant pool needs to be dumped
 * any time soon
 */
extern CVMBool
CVMJITcpoolNeedDump(CVMJITCompilationContext* con);

