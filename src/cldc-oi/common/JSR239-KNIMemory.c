/*
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
 */

#include "JSR239-KNIInterface.h"

#include <string.h>
#include <pcsl_memory.h>

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif


void
JSR239_memcpy(void *dst, void *src, int nbytes) {
    memcpy(dst, src, nbytes);
}

/* Return a block of memory 'size' bytes long, 8-byte aligned. */

#ifdef DEBUG
static int mallocs = 0;
#endif

void *
JSR239_malloc(int size) {
    void *ptr = pcsl_mem_calloc(size, 1);

#ifdef DEBUG
    ++mallocs;
    printf("JSR239_malloc(%d) -> 0x%x, mallocs = %d\n", size, ptr, mallocs);
#endif
    return ptr;
}

/* Free a block of memory allocated by JSR239_malloc. */

void
JSR239_free(void *ptr) {
#ifdef DEBUG
    --mallocs;
    printf("JSR239_free(0x%x), mallocs = %d\n", ptr, mallocs);
#endif
    pcsl_mem_free(ptr);
}
