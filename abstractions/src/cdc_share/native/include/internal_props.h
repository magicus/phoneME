/*
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
 */
#ifndef __INTERNAL_PROPS_H
#define __INTERNAL_PROPS_H

#if defined __cplusplus 
extern "C" { 
#endif /* __cplusplus */

/**
 * Native counterpart for
 * <code>com.sun.j2me.main.Configuration.getProperty()</code>.
 * Puts requested property value in UTF-8 format into the provided buffer
 * and returns pointer to it.
 *
 * @param key property key.
 * @param buffer pre-allocated buffer where property value will be stored.
 * @param length buffer size in bytes.
 * @return pointer to the filled buffer on success,
 *         <code>NULL</code> otherwise.
 */
const char* getInternalProp(const char* key, char* buffer, int length);

#if defined __cplusplus 
} 
#endif /* __cplusplus */
#endif /* __INTERNAL_PROPS_H */

