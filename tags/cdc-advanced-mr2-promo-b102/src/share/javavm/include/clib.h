/*
 * @(#)clib.h	1.40 06/10/10
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

#ifndef _INCLUDED_PORTING_CLIB_H
#define _INCLUDED_PORTING_CLIB_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/limits.h"
#include "javavm/include/porting/ansi/time.h"
#include "javavm/include/porting/ansi/ctype.h"
#include "javavm/include/porting/ansi/errno.h"

/*
 * Porting layer for C library sorts of things
 */

#include "javavm/include/assert.h"

/*
 * Support for debug stubs.
 */
#include "javavm/include/porting_debug.h"

#endif /* _INCLUDED_PORTING_CLIB_H */
