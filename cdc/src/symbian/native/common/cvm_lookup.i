/*
 * @(#)cvm_lookup.i	1.7 06/10/04
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
 */

#include <stddef.h>
#include <string.h>
#include <jni.h>

typedef void (*TFunc)();
typedef TFunc lookup_t(const char *);

#define ENTRY0(f)	extern int f()
#define ENTRY(f)	extern JNICALL void f()
#define ENTRY_SEP	;
#include "cvm_jni_funcs.i"
ENTRY_SEP

#undef ENTRY
#undef ENTRY0
#undef ENTRY_SEP

#define ENTRY(f)	{ #f, (TFunc)&f }
#define ENTRY0(f)	ENTRY(f)
#define ENTRY_SEP	,

static struct {
    const char *name;
    TFunc f;
} funcs[] = {
#include "cvm_jni_funcs.i"
};

#define CONCAT3(x,y,z) x ## y ## z
#define NAME(x) CONCAT3(__,x,_lookup)
EXPORT_C TFunc NAME(CVM_LIBRARY_NAME)(const char *name)
{
    int n = sizeof funcs / sizeof funcs[0];
    int i;
    for (i = 0; i < n; ++i) {
	if (strcmp(funcs[i].name, name) == 0) {
	    return funcs[i].f;
	}
    }
    return NULL;
}
