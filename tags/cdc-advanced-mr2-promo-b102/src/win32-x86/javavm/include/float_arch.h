/*
 * @(#)float_arch.h	1.6 06/10/10
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

#ifndef _LINUX_FLOAT_ARCH_H
#define _LINUX_FLOAT_ARCH_H

#undef JAVA_COMPLIANT_f2i
#undef JAVA_COMPLIANT_f2l
#undef NAN_CHECK_f2i
#undef NAN_CHECK_f2l
#undef BOUNDS_CHECK_f2l

#define USE_NATIVE_FREM
#undef USE_ANSI_FMOD
#define USE_NATIVE_FCOMPARE
#undef USE_ANSI_FCOMPARE

extern float floatRem(float, float);

#define CVMfloatRem(op1, op2) \
    floatRem((op1), (op2))

#define CVMcopySign(x, y)	_copysign((x), (y))
#define CVMisFinite(x)		_finite((x))

#endif /* _LINUX_FLOAT_ARCH_H */
