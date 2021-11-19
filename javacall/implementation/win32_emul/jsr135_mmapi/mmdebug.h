/*
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
 * 
 */

#ifndef __JAVACALL_MULTIMEDIA_DEBUG_H
#define __JAVACALL_MULTIMEDIA_DEBUG_H

#define JC_MM_ASSERT( e ) assert( e )

#if ( defined(DEBUG) || defined(_DEBUG) ) && !defined(USE_JC_MM_DEBUG)
    #define USE_JC_MM_DEBUG
#endif

#ifdef USE_JC_MM_DEBUG

#include <stdio.h>

#define JC_MM_DEBUG_PRINTF                printf

#define JC_MM_DEBUG_PRINT(x)              printf((x))
#define JC_MM_DEBUG_PRINT1(x, a)          printf((x), (a))
#define JC_MM_DEBUG_PRINT2(x, a, b)       printf((x), (a), (b))
#define JC_MM_DEBUG_PRINT3(x, a, b, c)    printf((x), (a), (b), (c))
#define JC_MM_DEBUG_PRINT4(x, a, b, c, d) printf((x), (a), (b), (c), (d))

#define JC_MM_DEBUG_ERROR_POSITION() \
    printf("[ERROR] [JAVACALL_MM] %s() line %d. ", __FUNCTION__, __LINE__)

#define JC_MM_DEBUG_WARNING_POSITION() \
    printf("[WARNING] [JAVACALL_MM] %s() line %d. ", __FUNCTION__, __LINE__)

#define JC_MM_DEBUG_INFO_POSITION() \
    printf("[INFO] [JAVACALL_MM] %s() line %d. ", __FUNCTION__, __LINE__)

#define JC_MM_DEBUG_ERROR_PRINT(x)        { JC_MM_DEBUG_ERROR_POSITION(); printf((x));}
#define JC_MM_DEBUG_ERROR_PRINT1(x, a)    { JC_MM_DEBUG_ERROR_POSITION(); printf((x), (a));}
#define JC_MM_DEBUG_ERROR_PRINT2(x, a, b) { JC_MM_DEBUG_ERROR_POSITION(); printf((x), (a), (b));}

#define JC_MM_DEBUG_WARNING_PRINT(x)              { JC_MM_DEBUG_WARNING_POSITION(); printf((x));}
#define JC_MM_DEBUG_WARNING_PRINT1(x, a)          { JC_MM_DEBUG_WARNING_POSITION(); printf((x), (a));}
#define JC_MM_DEBUG_WARNING_PRINT2(x, a, b)       { JC_MM_DEBUG_WARNING_POSITION(); printf((x), (a), (b));}
#define JC_MM_DEBUG_WARNING_PRINT3(x, a, b, c)    { JC_MM_DEBUG_WARNING_POSITION(); printf((x), (a), (b), (c));}
#define JC_MM_DEBUG_WARNING_PRINT4(x, a, b, c, d) { JC_MM_DEBUG_WARNING_POSITION(); printf((x), (a), (b), (c), (d));}

#define JC_MM_DEBUG_INFO_PRINT(x)              { JC_MM_DEBUG_INFO_POSITION(); printf((x));}
#define JC_MM_DEBUG_INFO_PRINT1(x, a)          { JC_MM_DEBUG_INFO_POSITION(); printf((x), (a));}
#define JC_MM_DEBUG_INFO_PRINT2(x, a, b)       { JC_MM_DEBUG_INFO_POSITION(); printf((x), (a), (b));}
#define JC_MM_DEBUG_INFO_PRINT3(x, a, b, c)    { JC_MM_DEBUG_INFO_POSITION(); printf((x), (a), (b), (c));}
#define JC_MM_DEBUG_INFO_PRINT4(x, a, b, c, d) { JC_MM_DEBUG_INFO_POSITION(); printf((x), (a), (b), (c), (d));}

#else /* !USE_JC_MM_DEBUG */

#define JC_MM_DEBUG_PRINTF  {}

#define JC_MM_DEBUG_PRINT(x)  {}
#define JC_MM_DEBUG_PRINT1(x, a)  {}
#define JC_MM_DEBUG_PRINT2(x, a, b) {}
#define JC_MM_DEBUG_PRINT3(x, a, b, c)  {}
#define JC_MM_DEBUG_PRINT4(x, a, b, c, d)  {}

#define JC_MM_DEBUG_ERROR_PRINT(x) {}
#define JC_MM_DEBUG_ERROR_PRINT1(x, a) {}
#define JC_MM_DEBUG_ERROR_PRINT2(x, a, b) {}

#define JC_MM_DEBUG_WARNING_PRINT(x) {}
#define JC_MM_DEBUG_WARNING_PRINT1(x, a) {}
#define JC_MM_DEBUG_WARNING_PRINT2(x, a, b) {}
#define JC_MM_DEBUG_WARNING_PRINT3(x, a, b, c) {}
#define JC_MM_DEBUG_WARNING_PRINT4(x, a, b, c, d) {}

#define JC_MM_DEBUG_INFO_PRINT_STRING(x, a) {}
#define JC_MM_DEBUG_INFO_PRINT(x) {}
#define JC_MM_DEBUG_INFO_PRINT1(x, a) {}
#define JC_MM_DEBUG_INFO_PRINT2(x, a, b) {}
#define JC_MM_DEBUG_INFO_PRINT3(x, a, b, c) {}
#define JC_MM_DEBUG_INFO_PRINT4(x, a, b, c, d) {}

#endif /* USE_JC_MM_DEBUG */

#endif /* __JAVACALL_MULTIMEDIA_DEBUG_H */
