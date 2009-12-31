/*
 * @(#)mem_mgr.c	1.7 06/10/10
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
#ifdef CVM_USE_MEM_MGR

#include "javavm/include/mem_mgr.h"
#include "javavm/include/defs.h"
#include "javavm/include/clib.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/sync.h"
#include "javavm/include/utils.h"
#include "javavm/include/porting/memory.h"
#include "javavm/include/preloader.h"

typedef enum {
    CVM_MEM_ALLOC_MALLOC,
    CVM_MEM_ALLOC_MMAP,
    CVM_MEM_ALLOC_MEMALIGN
} CVMMemAllocType;

struct CVMMemHandle {};

CVM_STRUCT_TYPEDEF(CVMMemDirtyPages);
struct CVMMemDirtyPages {
    CVMUint8 *memMap;   /* map of dirty pages */
    CVMInt32 numberOfDirtypages;  
};

CVM_STRUCT_TYPEDEF(CVMMemPrivateData);

struct CVMMemPrivateData {
    void *data0;	/* unadjusted data pointer */
    union {
        void *data;	/* adjusted data pointer */
        CVMAddr start;  /* the start of the registered region */
    } dataStart;
    CVMAddr end;        /* the end of the registered region */
    CVMMemType type;
    CVMMemMonMode mode; /* monitor mode */
    CVMMemAllocType allocType;
    CVMMemPrivateData **prev;
    CVMMemPrivateData *next;
    CVMMemPrivateData *nextWriteNotify;
    CVMMemDirtyPages *map;
    CVMMemHandle h;
    struct {} metaData;
    /*
     * meta data follows
     * DO NOT ADD FIELDS HERE
     */
};

/* a list that contains all registered regions */
CVMMemPrivateData *memList = NULL;

CVMMemPrivateData *writeNotifyList = NULL;
CVMMemPrivateData *lastWriteRegion = NULL;

CVMMemTypeDescriptor CVMcustomMemType[15];

int CVM_MEM_CUSTOM_NUM_TYPES = 0;

#define m2d(m) ((CVMMemPrivateData *) \
    ((char *)(m) - CVMoffsetof(CVMMemPrivateData, metaData)))
#define d2m(d) (&(d)->metaData)
#define h2d(h0) ((CVMMemPrivateData *) \
    ((char *)(h0) - CVMoffsetof(CVMMemPrivateData, h)))
#define d2h(d) (&(d)->h)
#define h2m(h) d2m(h2d((h)))
#define memGetStart(h) h2d(h)->dataStart.start
#define memGetMap(h) h2d(h)->map

const int pagesize = 0x1000;

CVMMemHandle *
CVMmemAllocWithMetaData(size_t dataSize, int align,
    CVMMemType mt, CVMMemFlag mf,
    size_t metaDataSize)
{
    void *p0;
    void *p;
    CVMMemPrivateData *h;
    CVMMemAllocType allocType;
    CVMMemHandle **hp = NULL;

    if (dataSize < 16 * 1024) {
	if (align > 8) {
	    allocType = CVM_MEM_ALLOC_MEMALIGN,
	    p0 = CVMmemalignAlloc(align, dataSize);
	    if (p0 != NULL && mf == CVM_MEM_CLEAR) {
		memset(p0, 0, dataSize);
		p = p0;
	    }
	} else {
	    size_t n = dataSize + 8;
	    allocType = CVM_MEM_ALLOC_MALLOC;
	    if (mf == CVM_MEM_CLEAR) {
		p0 = calloc(1, n);
	    } else {
		p0 = malloc(n);
	    }
	    if (p0 != NULL && (mf & CVM_MEM_FAST_ADDR2HANDLE) != 0) {
		/*
		 * Speed vs. memory trade-off.  We can
		 * waste 8-16 bytes to speedup reverse
		 * lookups.
		 */
		p = (char *)p0 + 8;
		if (((CVMAddr)p & (pagesize-1)) == 0) {
		    p0 = realloc(p, dataSize + 16);
		    if (p0 != NULL) {
			p = (void *)((char *)p + 8);
			if (((CVMAddr)p & (pagesize-1)) == 0) {
			    p += 8;
			}
		    }
		}
		hp = (CVMMemHandle **)p - 1;
	    }
	}
    } else {
	allocType = CVM_MEM_ALLOC_MMAP;
#if 0
	p0 = CVMmapNamedAnonMemory(CVMmemTypeInfo[mt].shortName,
	    align, dataSize);
#else
	p0 = NULL;
#endif
    }

    if (p0 == NULL) {
	return NULL;
    }

    h = (CVMMemPrivateData *)malloc(sizeof *h + metaDataSize);
    h->data0 = p0;
    h->dataStart.data = p;
    h->type = mt;
    h->allocType = allocType;
    if (hp != NULL) {
	*hp = d2h(h);
    }
    return &h->h;
}

CVMMemHandle *
CVMmemRegisterWithMetaData(CVMAddr start, CVMAddr end,
    CVMMemType mt,
    size_t metaDataSize)
{
    CVMMemPrivateData *privateData;

    privateData = (CVMMemPrivateData*)malloc(
            sizeof(CVMMemPrivateData) + metaDataSize);
    if (privateData == NULL) {
        return NULL;
    }

    privateData->dataStart.start = start;
    privateData->end = end;
    privateData->type = mt;
    privateData->mode = CVM_MEM_MON_NONE;
    privateData->map = NULL;
    privateData->next = memList;
    if (memList != NULL) {
        memList->prev = &privateData;
    }
    memList = privateData;
    return &privateData->h;
}

CVMMemMonMode
CVMmemGetMonitorMode(CVMMemHandle *h)
{
    return h2d(h)->mode;
}

#define ALIGNED(addr) \
    (CVMAddr)((CVMUint32)addr & ~(CVMgetPagesize() - 1))
#define ALIGNEDNEXT(addr) \
    (CVMAddr)(((CVMUint32)addr + CVMgetPagesize() - 1) & \
                  ~(CVMgetPagesize() - 1))

/* A mutex lock for the write notify list. */
CVMMutex *wnlLock = NULL;

void
CVMmemEnableWriteNotify(CVMMemHandle *h, CVMUint32* start, CVMUint32* end)
{
    CVMMemPrivateData *r = h2d(h);
    CVMAddr alignedStart = ALIGNED(start);
    CVMAddr alignedEnd = ALIGNEDNEXT(end);

    if (wnlLock == NULL) {
        wnlLock = malloc(sizeof(CVMMutex));
        CVMmutexInit(wnlLock);
    }
    CVMmutexLock(wnlLock);

    /* Add to the write notify list. */
    if (writeNotifyList == NULL) {
        r->nextWriteNotify = NULL;
    } else {
        r->nextWriteNotify = writeNotifyList;
    }
    writeNotifyList = r;

    CVMmutexUnlock(wnlLock);

    /* Protect the aligned region that contains the start and end to
     * enable write notify.
     */
    CVMmprotect((void*)alignedStart, (void*)alignedEnd, CVM_TRUE);
}

void
CVMmemDisableWriteNotify(CVMMemHandle *h)
{
    CVMMemPrivateData *c;

    CVMassert(wnlLock != NULL);

    CVMmutexLock(wnlLock);

    c = writeNotifyList;
    while (c != NULL) {
        if (d2h(c) == h) {
            CVMMemPrivateData *region;
            CVMAddr start = c->dataStart.start;
            CVMAddr end = c->end;

            CVMAddr alignedStart = ALIGNED(start);
            CVMAddr alignedEnd = ALIGNEDNEXT(end);

            /* Found the region. Unprotect it. */

            /* Now traverse the list again to check if part of the
             * aligned pages (first page and last page) belong to 
             * other regions.
             */
            region = writeNotifyList;
            while (region != NULL) {
		if (region->end < start &&
                    region->end >= alignedStart) {
                    alignedStart = ALIGNEDNEXT(start);
		}
                if (region->dataStart.start > end &&
                    region->dataStart.start <= alignedEnd) {
                    alignedEnd = ALIGNED(end);
                }
                region = region->next;
            }
            /* Unprotect the adjusted aligned region, so the next
             * write within the range would not cause a signal.
             */
            CVMmprotect((void*)alignedStart, (void*)alignedEnd, CVM_FALSE);
            CVMmutexUnlock(wnlLock);
            return;
        }
        c = c->nextWriteNotify;
    }
    CVMmutexUnlock(wnlLock);
}

static CVMBool
markDirtyPage(void *addr, CVMMemHandle *h)
{
    CVMMemDirtyPages *map = memGetMap(h);
    CVMAddr start = memGetStart(h);
    int pageNo = ((CVMUint32)addr - (CVMUint32)ALIGNED(start)) /
                        CVMgetPagesize() - 1;
    CVMBool isFirstWrite = CVM_FALSE;

    CVMassert(map != NULL);
    if (map->memMap[pageNo] == 0) {
        map->memMap[pageNo] = 0x1;
        map->numberOfDirtypages ++;
        isFirstWrite = CVM_TRUE;
    }

    return isFirstWrite;
}

/* Traverse the private list to find if the address belongs to any
 * region. Notify the write if necessary. An address may still be 
 * a valid address even there is no corresponding region found,
 * because we can only protect/unprotect aligned range. The return 
 * value tells if the address is valid.
 */
CVMBool 
CVMMemWriteNotify(int pid, CVMUint32 *addr, void*pc)
{
    CVMMemPrivateData *r = writeNotifyList;
    CVMMemHandle *h = NULL;
    CVMMemType type;
    CVMMemMonMode mode;
    CVMBool isValid = CVM_FALSE;

    /* First check if it's from the same region where the last
     * write happened.
     */
    if (lastWriteRegion != NULL && 
        (CVMAddr)addr >= ALIGNED(lastWriteRegion->dataStart.start) &&
        (CVMAddr)addr <= ALIGNEDNEXT(lastWriteRegion->end)) {
        isValid = CVM_TRUE;
        if ((CVMAddr)addr >= lastWriteRegion->dataStart.start &&
            (CVMAddr)addr <= lastWriteRegion->end) {
            h = d2h(lastWriteRegion);
            goto found_region;
        }
    }

    while (r != NULL) {
        if ((CVMAddr)addr >= ALIGNED(r->dataStart.start) && 
            (CVMAddr)addr <= ALIGNEDNEXT(r->end)) {
	    /* The address is a valid address */
            isValid = CVM_TRUE;
            /* Is the address belong to any write notify region */
            if ((CVMAddr)addr >= r->dataStart.start &&
                (CVMAddr)addr <= r->end) {
                h = d2h(r);
                lastWriteRegion = r;
                goto found_region;
            }
        }

        r = r->nextWriteNotify;
    }
    
    return isValid;

found_region:
    CVMassert(h != NULL);
    type = CVMmemGetType(h);
    mode = CVMmemGetMonitorMode(h);
    if (type != -1 &&
        (mode == CVM_MEM_MON_FIRST_WRITE ||
         mode == CVM_MEM_MON_ALL_WRITES)) {
        CVMBool isFirstWrite = markDirtyPage(addr, h);
        if (mode == CVM_MEM_MON_ALL_WRITES ||
            (mode == CVM_MEM_MON_FIRST_WRITE && isFirstWrite)) {
            if (type < CVM_MEM_NUM_TYPES) {
                CVMmemType[type].writeNotify(pid, addr, pc, h);
            } else {
	        /* it's a custom type. Look at CVMcustomMemType table. */
                CVMcustomMemType[type - CVM_MEM_NUM_TYPES]. writeNotify(
                                    pid, addr, pc, h);
            }
        }
    }
    return isValid;
}

void
CVMmemSetMonitorMode(CVMMemHandle *h, CVMMemMonMode mode)
{
    CVMMemPrivateData *d = h2d(h);
    d->mode = mode; /* set the new mode */
    if (mode == CVM_MEM_MON_NONE) {
        /* disable write notify */      
        CVMmemDisableWriteNotify(h);
    } else if (mode == CVM_MEM_MON_FIRST_WRITE ||
               mode == CVM_MEM_MON_ALL_WRITES) {
        if (d->map == NULL) {
            CVMMemDirtyPages *map;
            map = (CVMMemDirtyPages*)malloc(
                          sizeof(CVMMemDirtyPages));
            if (map != NULL) {
                map->memMap = (CVMUint8*)calloc(
                    sizeof(CVMUint8), 
                    (ALIGNEDNEXT(d->end) - ALIGNED(d->dataStart.start)) /
                                    CVMgetPagesize());
	        if (map->memMap == NULL) {
                    free(map);
                    return;
                }
                map->numberOfDirtypages = 0;
                d->map = map;
   	    } else {
                return;
            }
        }
        CVMmemEnableWriteNotify(
            h, (CVMUint32*)d->dataStart.start, (CVMUint32*)d->end);
    }
    return;
}

void
CVMmemFree(CVMMemHandle *h)
{
    CVMMemPrivateData *d = h2d(h);

    /* FIXME: We should also destroy the private data and remove it
     *        from the link lists.
     */
    switch (d->allocType) {
    case CVM_MEM_ALLOC_MALLOC:
	free(d->dataStart.data);
	break;
    case CVM_MEM_ALLOC_MMAP:
#if 0
	CVMunmapAnonMemory(d->dataStart.data);
#endif
	break;
    case CVM_MEM_ALLOC_MEMALIGN:
	CVMmemalignFree(d->dataStart.data);
	break;
    default:
	CVMpanic("CVMmemFree: unknown alloc type\n");
    }
    free(d);
}

void *
CVMmemGetMetaData(CVMMemHandle *h)
{
    return h2m(h);
}

/*void *
CVMmemGetAddress(CVMMemHandle *h)
{
    const CVMMemPrivateData *d = h2d(h);
    return d->data;
}*/

CVMMemType
CVMmemGetType(const CVMMemHandle *h)
{
    return h2d(h)->type;
}

CVMMemHandle *
CVMmemFind(void *addr)
{
    CVMMemPrivateData *privateData = memList;
    while (privateData != NULL) {
        if ((CVMAddr)addr >= privateData->dataStart.start &&
            (CVMAddr)addr <= privateData->end) {
            return d2h(privateData);
        } else {
            privateData = privateData->next;
        }
    }
    return NULL;
}

const CVMMemHandle *
CVMmemAddress2Handle(void *addr)
{
    CVMMemHandle *h;
    CVMAddr a = (CVMAddr)addr;
    if ((a & (pagesize-1)) == 0) {
	/* lookup handle */
	h = CVMmemFind(addr);
	CVMassert(h != NULL && h2d(h)->dataStart.data == addr);
    } else {
	h = ((CVMMemHandle **)a)[-1];
    }
    return h;
}

static void
CVMmemHeapWriteNotify(int pid, void *addr, void *pc, CVMMemHandle *h)
{

    /* Mark the dirty page */

    /* Trace */

    return;
}

const CVMMemTypeDescriptor CVMmemType[CVM_MEM_NUM_TYPES] = {
    {"Exe Text", NULL, NULL, NULL, NULL},
    {"Exe Data", NULL, NULL, NULL, NULL},
    {"Exe BSS",  NULL, NULL, CVMmemBssWriteNotify, CVMmemBssReportWrites},
    {"DLL Text", NULL, NULL, NULL, NULL},
    {"DLL Data", NULL, NULL, NULL, NULL},
    {"DLL BSS",  NULL, NULL, NULL, NULL},
    {"Java Heap",  NULL, NULL, CVMmemHeapWriteNotify, NULL},
    {"Java Stack Chunk",  NULL, NULL, NULL, NULL},
    {"Native Stack",  NULL, NULL, NULL, NULL},
    {"Code Cache",  NULL, NULL, NULL, NULL},
    {"Long-Lived JIT",  NULL, NULL, NULL, NULL},
    {"Compiled Method",  NULL, NULL, NULL, NULL},
};

CVMMemType
CVMmemAddCustomMemoryType(const char *name, 
    size_t (*describeAddr)(CVMAddr, char *, size_t, int),
    void (*dump)(FILE *, int verbosityLevel),
    void (*writeNotify)(int pid, void *addr, void *pc, CVMMemHandle *h),
    void (*report)())
{
    CVMMemType newType = CVM_MEM_CUSTOM_NUM_TYPES + 
                                 CVM_MEM_NUM_TYPES;

    CVMcustomMemType[CVM_MEM_CUSTOM_NUM_TYPES].name = name;
    CVMcustomMemType[CVM_MEM_CUSTOM_NUM_TYPES].describeAddr =
        describeAddr;
    CVMcustomMemType[CVM_MEM_CUSTOM_NUM_TYPES].dump = dump;
    CVMcustomMemType[CVM_MEM_CUSTOM_NUM_TYPES].writeNotify = 
        writeNotify;
    CVMcustomMemType[CVM_MEM_CUSTOM_NUM_TYPES].report = report;

    CVM_MEM_CUSTOM_NUM_TYPES ++;
    
    return newType;
}

void
CVMmemManagerDumpStats()
{
    CVMMemPrivateData *d = memList;
    CVMconsolePrintf("Memory status:\n");
    while (d != NULL) {
        if (d->map != NULL) {
            CVMMemType type = d->type;
            int totalPage = (ALIGNEDNEXT(d->end) -  
                             ALIGNED(d->dataStart.start)) / CVMgetPagesize();
            CVMMemDirtyPages *dmap = d->map;
            if (type < CVM_MEM_NUM_TYPES) {
                CVMconsolePrintf("%s: Total Page = %d, Dirty Page = %d\n", 
                                 CVMmemType[type].name,
                                 totalPage, dmap->numberOfDirtypages);
                if (CVMmemType[type].report != NULL) {
                    CVMmemType[type].report();
                }
            } else {
                CVMconsolePrintf("%s: Total Page = %d, Dirty Page = %d\n", 
                        CVMcustomMemType[type - CVM_MEM_NUM_TYPES].name,
                        totalPage, dmap->numberOfDirtypages);
                if (CVMcustomMemType[type - CVM_MEM_NUM_TYPES].report != NULL) {
                    CVMcustomMemType[type - CVM_MEM_NUM_TYPES].report();
                }
            }
        }
        d = d->next;
    }
}

void
CVMmemManagerDestroy()
{
    /* Destroy all the private data. */
    if (memList != NULL) {
        CVMMemPrivateData *d = memList;
        while (d != NULL) {
            CVMMemPrivateData *next = d->next;
            free(d);
            d = next;
        }
    }

    /* Destroy all the locks */
    if (wnlLock != NULL) {
        CVMmutexDestroy(wnlLock);
        free(wnlLock);
    }
}

#endif /* CVM_USE_MEM_MGR */
