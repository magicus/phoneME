/*
 * @(#)javaAssertions.h	1.5 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"

/* Add a command-line option.  A name ending in "..." applies to a package and
   any subpackages; other names apply to a single class. */
extern CVMBool
CVMJavaAssertions_addOption(
    const char* name, CVMBool enable,
    CVMJavaAssertionsOptionList** javaAssertionsClasses,
    CVMJavaAssertionsOptionList** javaAssertionsPackages);

/* free assertion command line options */
extern void
CVMJavaAssertions_freeOptions();

/* Return true if command-line options have enabled assertions for the named
   class.  Should be called only after all command-line options have been
   processed.  Note:  this only consults command-line options and does not
   account for any dynamic changes to assertion status. */
extern CVMBool
CVMJavaAssertions_enabled(const char* classname, CVMBool systemClass);

/* Create an instance of java.lang.AssertionStatusDirectives and fill in the
   fields based on the command-line assertion options. */
extern jobject
CVMJavaAssertions_createAssertionStatusDirectives(CVMExecEnv* ee);
