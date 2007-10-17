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


#include <stdio.h> /* for fprintf(stderr,...) */
#include <dlfcn.h>
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif

#define DSO_LIB_PREFIX "lib"

#ifdef ENABLE_JAVACALL_DEBUG
#define DSO_DEBUG_SUFFIX "_g"
#else
#define DSO_DEBUG_SUFFIX
#endif

#define DSO_LIB_SUFFIX ".so"

#define decl_javanotify(name_, args_, actuals_)  \
    static void (*name_##_func_addr) args_ = NULL; \
    void jn_local_##name_ args_ { \
        static char dso_name[] = DSO_LIB_PREFIX "jsr120" DSO_DEBUG_SUFFIX DSO_LIB_SUFFIX; \
        if (name_##_func_addr == NULL) { \
            void *dso_handle = dlopen(dso_name, RTLD_LAZY); \
            if (dso_handle == NULL) { \
                dso_handle = RTLD_DEFAULT; \
            } \
            name_##_func_addr = dlsym(dso_handle, "javanotify_" #name_); \
        } \
        if (name_##_func_addr == NULL) { \
            fprintf(stderr, "error: cannot resolve symbol: javanotify_%s\n", #name_); \
            return; \
        } \
        (*name_##_func_addr) actuals_; \
    } \

#define call_javanotify(name_, params_) \
    jn_local_##name_ params_

