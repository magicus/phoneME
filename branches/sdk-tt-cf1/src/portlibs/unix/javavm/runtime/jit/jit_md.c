/*
 * @(#)jit_md.c	1.8 06/10/23
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

#include "javavm/include/globals.h"
#include "javavm/include/assert.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"

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
		 PROT_READ);
    CVMassert(result == 0);
    (void)result;
}

#endif /* CVMJIT_TRAP_BASED_GC_CHECKS */

#ifdef CVM_AOT
#include <unistd.h>
#include <sys/mman.h>
#include "javavm/include/porting/io.h"
#include "javavm/include/securehash.h"

/* Find the AOT code from the persistent storage. */
CVMInt32 CVMfindAOTCodeCache()
{
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    const CVMProperties *sprops = CVMgetProperties();
    char *aotfile;
    int fd;

    aotfile = (char*)malloc(strlen(sprops->library_path) +
                            strlen("/cvm.aot") + 1);
    *aotfile = '\0';
    strcat(aotfile, sprops->library_path);
    strcat(aotfile, "/cvm.aot");

    fd = open(aotfile, O_RDONLY);
    free(aotfile);
    if (fd != -1) {
        /* Found persistent code store. */
        CVMInt32 codeSize;
        char buf[CVM_SHA_OUTPUT_LENGTH+1];
	void* codeStart;
	const char* cvmsha1hash = CVMgetSha1Hash();

        /* The first word is the size of the pre-compiled code. */
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
            goto failed;
        } else {
            int sumlength = strlen(cvmsha1hash);
            read(fd, buf, sumlength);
            buf[sumlength] = '\0';
            if (strcmp(buf, cvmsha1hash)) {
                CVMconsolePrintf("AOT code is outdated.\n");
                goto failed;
            }
        }

        /* 
         * If we get here, the AOT code is compatible with
         * the current executable. mmap the codecache.
         */
        lseek(fd, 0, SEEK_SET); /* seek to the beginning, just to be safe */
        codeStart = mmap(0, codeSize,
                            PROT_EXEC|PROT_READ,
                            MAP_PRIVATE, fd, 0);
        if (codeStart != MAP_FAILED) {
            jgs->codeCacheAOTStart = (CVMUint8*)codeStart + sizeof(CVMInt32);
            jgs->codeCacheAOTEnd = jgs->codeCacheAOTStart + codeSize;
	    jgs->codeCacheAOTCodeExist = CVM_TRUE;
            jgs->codeCacheDecompileStart = jgs->codeCacheAOTEnd;
            jgs->codeCacheNextDecompileScanStart = jgs->codeCacheAOTEnd;
            return codeSize;
        }
    }

failed:
    jgs->codeCacheAOTStart = 0;
    jgs->codeCacheAOTEnd = 0;
    jgs->codeCacheAOTCodeExist = CVM_FALSE;
    return 0;
}

/* 
 * The compiled code above the codeCacheDecompileStart will be saved
 * into persistent storage if there is no previouse saved AOT code, 
 * and will be reloaded next time.
 * We write the AOT code size as the first word. The saved code
 * cache looks like the following:
 *
 *  ------------------------------------------------------
 *  |size|                                               |
 *  |-----                                               |
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
    /*CVMUint8* cbuf = jgs->codeCacheStart;*/
    const CVMProperties *sprops = CVMgetProperties();
    char *aotfile;

    if (jgs->codeCacheAOTCodeExist) {
        return;
    }

    aotfile = (char*)malloc(strlen(sprops->library_path) +
                            strlen("/cvm.aot") + 1);
    *aotfile = '\0';
    strcat(aotfile, sprops->library_path);
    strcat(aotfile, "/cvm.aot");
    fd = open(aotfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    free(aotfile);

    if (fd == -1) {
        CVMconsolePrintf("Could not create AOT file\n");
    } else {
        CVMUint8* start = jgs->codeCacheStart;
        CVMUint32 end = (CVMUint32)jgs->codeCacheDecompileStart;
        CVMInt32  codeSize = end - (CVMUint32)start;
	const char* cvmsha1hash = CVMgetSha1Hash();

        /* write code size */
        write(fd, &codeSize, sizeof(CVMInt32));
        /* write code */
	write(fd, start, codeSize);
	/* write CVM SHA-1 checksum value */
	write(fd, cvmsha1hash, strlen(cvmsha1hash));

        close(fd);
    }
}
#endif
