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
#ifndef __JSROP_MEMORY_MD_H
#define __JSROP_MEMORY_MD_H

#if defined __cplusplus 
    /* by C++98, C memory routines are placed at cstdlib */
    #include <cstdlib>
#else
    /* by C90 and C99, memory routines are placed at stdlib.h */
    #include <stdlib.h>
#endif

#define JAVAME_MALLOC(size)        malloc((size))
#define JAVAME_FREE(addr)          free((addr))
#define JAVAME_CALLOC(x, y)        calloc(x, y)
#define JAVAME_REALLOC(addr, size) realloc(addr, size)


#endif /* __JSROP_MEMORY_H */
