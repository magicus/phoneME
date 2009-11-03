/*
 * @(#)jit_md.c	1.8 06/10/23
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

#include "javavm/include/globals.h"
#include "javavm/include/assert.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitutils.h"
#include "javavm/include/porting/jit/jit.h"

#ifdef CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE
#include <sys/mman.h>

/* The following is needed to supported an executable code cache
   with some Unix releases.
*/
void *
CVMJITallocCodeCache(CVMSize *size)
{
    void* s = mmap(0, *size, 
          PROT_EXEC | PROT_READ | PROT_WRITE, 
          MAP_PRIVATE | MAP_ANON, -1, 0);
    if (s == MAP_FAILED) {
        return NULL;
    }
    return s;
}

void
CVMJITfreeCodeCache(void *start)
{
    munmap(start, CVMglobals.jit.codeCacheSize);
}

#endif /* CVMJIT_HAVE_PLATFORM_SPECIFIC_ALLOC_FREE_CODECACHE */

#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
#include <sys/mman.h>

/*
 * Enable gc rendezvous points in compiled code by denying access to
 * the page that is accessed at each rendezvous point. This will
 * cause a SEGV and handleSegv() will setup the gc rendezvous
 * when this happens.
 */
void
CVMJITenableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
    /* Since we offset CVMglobals.jit.gcTrapAddr by 
       CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words, we need to readjust and
       also map out twice this many words (positive and negative offsets)
    */
    int result =
	mprotect(CVMglobals.jit.gcTrapAddr - CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 sizeof(void*) * 2 * CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 PROT_NONE);
    CVMassert(result == 0);
    (void)result;
}

/*
 * Disable gc rendezvous points be making the page readable again.
 */
void
CVMJITdisableRendezvousCallsTrapbased(CVMExecEnv* ee)
{
    int result =
	mprotect(CVMglobals.jit.gcTrapAddr - CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 sizeof(void*) * 2 * CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET,
		 PROT_READ | PROT_WRITE);
    CVMassert(result == 0);
    (void)result;
}

#endif /* CVMJIT_TRAP_BASED_GC_CHECKS */

#ifdef CVM_AOT
#include <unistd.h>
#include <sys/mman.h>
#include "javavm/include/porting/io.h"
#include "javavm/include/securehash.h"

/* codecache start addr + codecache size */
#define AOT_HEADER_SIZE (2 * sizeof(CVMInt32))
#define TOTAL_CODECACHE_SIZE \
    (jgs->codeCacheSize + AOT_HEADER_SIZE)

/* Find existing AOT code. If there is no AOT code found,
 * allocate a big consecutive code cache for both AOT and 
 * JIT compilation.
 */
CVMInt32 CVMfindAOTCode()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    char *aotfile = jgs->aotFile;
    int fd;

    if (jgs->recompileAOT) {
        goto notFound;
    }

    CVMassert(aotfile != NULL);

    fd = open(aotfile, O_RDONLY);

    if (fd != -1) {
        /* Found persistent code store. */
        CVMInt32 codeSize;
        char buf[CVM_SHA_OUTPUT_LENGTH+1];
	void* codeStart;
	const char* cvmsha1hash = CVMgetSha1Hash();
        CVMUint32 addr = 0;

        /* Read the codecache start address */
        read(fd, &addr, sizeof(CVMInt32));
        
	/* Read the code size */
        read(fd, &codeSize, sizeof(CVMInt32));

        /* 
         * Check if the AOT code is compatible with the
         * the current CVM executable by comparing the SHA1 hash
         * value embedded in the AOT code with the one in the 
         * CVM. The current CVM's SHA1 hash value is computed 
         * against the CVM archive at build time. The hash 
         * value is included in the generated cvm_sha1.c, which 
         * is compiled and linked into the executable. In the saved 
         * AOT file, the SHA1 hash value is located after the AOT 
         * code (see CVMJITcodeCachePersist() for the layout). 
         * It's the hash value of the executable that dump the
         * AOT code.
         */
        if (lseek(fd, codeSize, SEEK_CUR) == -1) {
            goto notFound;
        } else {
            int sumlength = strlen(cvmsha1hash);
            read(fd, buf, sumlength);
            buf[sumlength] = '\0';
            if (strcmp(buf, cvmsha1hash)) {
	        CVMtraceJITStatus((
                    "AOT code is not compatibale. Recompile AOT code.\n"));
                goto notFound;
            }
        }

        /* 
         * If we get here, the AOT code is compatible with
         * the current executable. mmap the codecache.
         */
        lseek(fd, 0, SEEK_SET); /* seek to the beginning, just to be safe */
#ifdef CVMAOT_USE_FIXED_ADDRESS
        /*
         * 'addr' was the codecache start address that was used
         * when compiling AOT code. 'addr-headersize' was the 
         * page-aligned address returned by mmap() when the 
         * codecache was allocated for AOT compilation. We want
         * to map the current codecache at the same address.
         * Since mmap requires the address to be aligned at page
         * boundary when MAP_FIXED is used, so we need to use
         * 'addr-headersize' instead of 'addr'.
         */
        addr -= AOT_HEADER_SIZE;
        /* This is to check if we can safely map at the same address
         * that's specified in the AOT file.
         */
        codeStart = mmap((void*)addr, codeSize + AOT_HEADER_SIZE,
                         PROT_EXEC|PROT_READ,
                         MAP_PRIVATE, fd, 0);
	if (codeStart != (void*)addr) {
	    if (codeStart != MAP_FAILED) {
	        /* Failed to map the AOT code at the desired address. 
                 * Unmap the file and set 'codeStart' to MAP_FAILED. */ 
                CVMtraceJITStatus((
                    "JS: Failed to map AOT code at 0x%x.\n", addr));
                munmap(codeStart, codeSize);
                codeStart = MAP_FAILED;
	    }
        }
#else
        codeStart = mmap(0, codeSize + AOT_HEADER_SIZE,
                         PROT_EXEC|PROT_READ,
			 MAP_PRIVATE, fd, 0);
#endif

        if (codeStart != MAP_FAILED) {
	    jgs->codeCacheAOTStart = (CVMUint8*)(codeStart + AOT_HEADER_SIZE);
            jgs->codeCacheAOTEnd = jgs->codeCacheAOTStart + codeSize;
	    jgs->codeCacheAOTCodeExist = CVM_TRUE;
            jgs->codeCacheDecompileStart = jgs->codeCacheAOTEnd;
            jgs->codeCacheNextDecompileScanStart = jgs->codeCacheAOTEnd;
	    CVMtraceJITStatus((
                "JS: Found AOT code. Codecache addr:0x%x, size: %dbytes.\n",
		jgs->codeCacheAOTStart, codeSize));
            return codeSize;
        }
        CVMtraceJITStatus((
            "JS: Failed to map AOT code. Recompile AOT code.\n"));
    }

notFound:
    {
        int fd;
        void* alignedAddr;

        /* Couldn't find saved AOT code. Allocate one big consecutive
           code cache for both AOT and JIT compilation. */
        jgs->codeCacheSize += jgs->aotCodeCacheSize;
        fd = open("/dev/zero", O_RDWR);
        alignedAddr = mmap(0, TOTAL_CODECACHE_SIZE,
                           PROT_EXEC|PROT_READ|PROT_WRITE,
                           MAP_PRIVATE, fd, 0);
        close(fd);

        if (alignedAddr == MAP_FAILED) {
	    CVMconsolePrintf("Failed to mmap AOT code cache.\n");
	    jgs->codeCacheStart = NULL;
        } else {
            jgs->codeCacheStart = alignedAddr + AOT_HEADER_SIZE;
            jgs->codeCacheAOTStart = jgs->codeCacheStart;
            jgs->codeCacheAOTEnd = &jgs->codeCacheStart[jgs->aotCodeCacheSize];
        }
        jgs->codeCacheAOTCodeExist = CVM_FALSE;
        return 0;
    }
}


/* 
 * The compiled code above the codeCacheDecompileStart will be saved
 * into persistent storage if there is no previouse saved AOT code, 
 * and will be reloaded next time.
 *
 * We write the start address of the AOT code as the first word.
 * This allows it to be forced to be mapped to the same location if
 * necessary on subsequent runs. See CVMAOT_USE_FIXED_ADDRESS.
 * The size is also written, along with a checksum to make sure
 * it is the correct AOT code to be used with this vm build.
 *
 *  ------------------------------------------------------
 *  |codecache start addr|codecache size|                |
 *  |-------------------------------------               |
 *  |                                                    |
 *  |                 compiled code                      |
 *  .                                                    .
 *  .                                                    .
 *  .                                                    .
 *  |                                                    |
 *  ------------------------------------------------------
 *  | CVM SHA-1 checksum                                 |
 *  ------------------------------------------------------
 */
void
CVMJITcodeCachePersist()
{
    int fd;
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    char *aotfile = jgs->aotFile;

    CVMassert(aotfile != NULL);

    if (jgs->aotCompileFailed || jgs->codeCacheAOTCodeExist) {
        return;
    }

    fd = open(aotfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fd == -1) {
        CVMconsolePrintf("Could not create AOT file: %s.\n", aotfile);
        CVMabort();
    } else {
        CVMUint8* start = jgs->codeCacheStart;
        CVMUint32 end = (CVMUint32)jgs->codeCacheDecompileStart;
        CVMInt32  codeSize = end - (CVMUint32)start;
	const char* cvmsha1hash = CVMgetSha1Hash();

        CVMtraceJITStatus((
	    "JS: Writing AOT code. CodeCache addr:0x%x, size:%dbytes.\n",
	    start, codeSize));
        /* write codecache start address */
        write(fd, &start, sizeof(CVMInt32));
        /* write code size */
        write(fd, &codeSize, sizeof(CVMInt32));
        /* write code */
	write(fd, start, codeSize);
	/* write CVM SHA-1 checksum value */
	write(fd, cvmsha1hash, strlen(cvmsha1hash));

        close(fd);
        CVMtraceJITStatus(("JS: Finished writing AOT code.\n"));
    }
}

/* Destroy AOT code cache. The return value indicates if
 * we need to free the JIT code cache separately.
 */
CVMBool
CVMJITAOTcodeCacheDestroy()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit; 
    if (jgs->codeCacheAOTStart == jgs->codeCacheStart) {
        /* We just generated the AOT code on this run */
        munmap(jgs->codeCacheAOTStart, TOTAL_CODECACHE_SIZE);
        return CVM_FALSE; /* no need to free JIT code cache */
    } else {
        munmap(jgs->codeCacheAOTStart - AOT_HEADER_SIZE, 
               jgs->codeCacheAOTEnd - 
               (jgs->codeCacheAOTStart - AOT_HEADER_SIZE));
        return CVM_TRUE; /* need to free JIT code cache */
    }
}

#endif
