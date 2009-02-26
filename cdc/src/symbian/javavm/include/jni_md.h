/*
 * @(#)jni_md.h	1.5 06/10/10
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
 * Machine-dependent JNI definitions.
 */

#ifndef _SYMBIAN_JNI_MD_H
#define _SYMBIAN_JNI_MD_H

#ifdef __GCC32__
#include "portlibs/gcc_32_bit/jni.h"
#else /* gcc */
#ifdef __cplusplus
#define JNIEXPORT extern "C" EXPORT_C
#define JNIIMPORT extern "C" IMPORT_C 
#else /* C++ */
#define JNIEXPORT EXPORT_C
#define JNIIMPORT IMPORT_C
#endif /* C++ */
#ifndef JNICALL
#define JNICALL
#endif
#endif /* gcc */

#define JNI_LIB_PREFIX "lib"
#define JNI_LIB_SUFFIX ".so"

#endif /* _SYMBIAN_JNI_MD_H */
