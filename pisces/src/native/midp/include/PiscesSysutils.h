/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
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


#ifndef PISCES_SYSUTILS_H
#define PISCES_SYSUTILS_H

/* for PISCESmalloc */
#include <pcsl_memory.h>

/* for memcpy */
#include <string.h>

/* for sqrt, sin, cos */
#include <jvm.h>

jboolean readAndClearMemErrorFlag();
jboolean readMemErrorFlag();
void     setMemErrorFlag();

#define PISCESmalloc(x) pcsl_mem_malloc((x))
#define PISCESfree(x) pcsl_mem_free((x))
#define PISCEScalloc(x, y) pcsl_mem_calloc((x), (y))
#define PISCESrealloc(x,y) pcsl_mem_realloc((x), (y))

#define PISCESclear_mem(buffer,count) memset(buffer,0,count)

#define PISCESsqrt(x) jvm_sqrt((x))
#define PISCESsin(x) jvm_sin((x))
#define PISCEScos(x) jvm_cos((x))

#ifdef _MSC_VER
typedef unsigned __int64    ulong64;
#else
typedef unsigned long long  ulong64;
#endif

/* a definition for assert(). Does nothing! */
#define assert(z)

#define ASSERT_ALLOC(memptr) \
    if (NULL == (memptr)) {  \
        setMemErrorFlag();   \
        return;              \
    }

#define ASSERT_ALLOC_BOOLEAN(memptr) \
    if (NULL == (memptr)) {          \
        setMemErrorFlag();           \
        return XNI_FALSE;            \
    }

#define ASSERT_ALLOC_POINTER(memptr) \
    if (NULL == (memptr)) {            \
        setMemErrorFlag();           \
        return NULL;                 \
    }

#endif /* PISCES_SYSUTILS_H */
