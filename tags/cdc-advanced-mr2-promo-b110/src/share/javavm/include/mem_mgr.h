/*
 * @(#)mem_mgr.h	1.8 06/10/10
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
 * Memory manager
 */

#ifndef CVM_MEM_MGR_H
#define CVM_MEM_MGR_H

#ifdef CVM_USE_MEM_MGR

#include "javavm/include/defs.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/ansi/stdio.h"

/*
 * Set protection of memory mapping.
 */
void
CVMmprotect(void *startAddr, void *endAddr, CVMBool readOnly);

/*
 * Get page size.
 */
CVMInt32 CVMgetPagesize();

typedef enum {
    CVM_MEM_EXE_TEXT,
    CVM_MEM_EXE_DATA,
    CVM_MEM_EXE_BSS,
    CVM_MEM_DLL_TEXT,
    CVM_MEM_DLL_DATA,
    CVM_MEM_DLL_BSS,
    CVM_MEM_JAVA_HEAP,
    CVM_MEM_JAVA_STACK,
    CVM_MEM_NATIVE_STACK,
    CVM_MEM_CODE_CACHE,
    CVM_MEM_LONG_LIVED_JIT,
    CVM_MEM_COMPILED_METHOD,
    CVM_MEM_NUM_TYPES,
    CVM_MEM_UNKNOWN = -1
} CVMMemType;

/* opaque handle */
typedef struct CVMMemHandle CVMMemHandle;

typedef struct {
    const char *name;
    size_t (*describeAddr)(CVMAddr, char *buf, size_t len, int verbosityLevel);
    void (*dump)(FILE *, int verbosityLevel);
    void (*writeNotify)(int pid, void *addr, void *pc, CVMMemHandle *h);
    void (*report)();
} CVMMemTypeDescriptor;

extern const CVMMemTypeDescriptor CVMmemType[CVM_MEM_NUM_TYPES];

CVMMemType
CVMmemAddCustomMemoryType(const char *name, 
    size_t (*describeAddr)(CVMAddr, char *buf, size_t len, int verbosityLevel),
    void (*dump)(FILE *, int verbosityLevel),
    void (*writeNotify)(int pid, void *addr, void *pc, CVMMemHandle *h),
    void (*report)());

typedef enum {
    CVM_MEM_CLEAR,
    CVM_MEM_ZERO = CVM_MEM_CLEAR,
    CVM_MEM_NO_CLEAR,
    CVM_MEM_NO_ZERO = CVM_MEM_NO_CLEAR,
    CVM_MEM_FAST_ADDR2HANDLE = 0x100
} CVMMemFlag;

size_t
CVMmemGetPageSize();

CVMMemHandle *
CVMmemAllocWithMetaData(size_t dataSize, int align,
    CVMMemType mt, CVMMemFlag mf,
    size_t metaDataSize);

CVMMemHandle *
CVMmemRegisterWithMetaData(CVMAddr start, CVMAddr end,
    CVMMemType mt,
    size_t metaDataSize);

CVMMemHandle *
CVMmemReserve(size_t dataSize, int align);

CVMMemHandle *
CVMmemOccypyWithMetaData(CVMMemHandle *parent, CVMAddr start, size_t dataSize,
    CVMMemType mt, CVMMemFlag mf,
    size_t metaDataSize);

void
CVMmemFree(CVMMemHandle *);

void *
CVMmemGetMetaData(CVMMemHandle *);

typedef enum {
   CVM_MEM_MON_NONE,
   CVM_MEM_MON_FIRST_WRITE,
   CVM_MEM_MON_ALL_WRITES
} CVMMemMonMode;

CVMMemMonMode
CVMmemGetMonitorMode(CVMMemHandle *h);

extern void
CVMmemSetMonitorMode(CVMMemHandle *, CVMMemMonMode);

extern void
CVMmemEnableWriteNotify(CVMMemHandle *, CVMUint32* start, CVMUint32* end);

extern void
CVMmemDisableWriteNotify(CVMMemHandle *);

CVMMemHandle *
CVMmemFind(void *addr);

#ifdef CVM_USE_MEM_MGR
void *
CVMmemGetAddress(CVMMemHandle *h);

CVMMemType
CVMmemGetType(const CVMMemHandle *h);
#else
#define CVMmemGetAddress(h)	((void *)h)
#define CVMmemGetType(h)	CVM_MEM_UNKNOWN
#endif

extern CVMBool 
CVMMemWriteNotify(int pid, CVMUint32 *addr, void*pc);

extern void
CVMmemManagerDumpStats();

extern void
CVMmemManagerDestroy();

#endif /* CVM_USE_MEM_MGR */

#endif /* CVM_MEM_MGR_H */
