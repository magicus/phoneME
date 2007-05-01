/*
 *
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

#ifndef _FIND_MIDLET_H_
#define _FIND_MIDLET_H_

#include <midpString.h>
#include <suitestore_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Find a class name of a MIDlet in suite by number.
 *
 * @param id suite ID of the MIDlet
 * @param midletNumber number of the MIDlet starting at 1
 * @param res [out] receives class name or null string if not found
 *
 * @return 0 if ok
 * NULL_LEN if not found
 * OUT_OF_MEM_LEN if out of memory
 */
int find_midlet_class(SuiteIdType id, int midletNumber, pcsl_string* res);

#ifdef __cplusplus
}
#endif

#endif /* _FIND_MIDLET_H_ */
