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

#ifndef _MIDP_START_VM_H
#define _MIDP_START_VM_H


/**
 * @defgroup eventsystem Event System Porting Interface
 * @ingroup events
 */

/**
 * @file
 * @ingroup eventsystem
 *
 * @brief Porting interface for starting up VM in master or slave mode.
 */

#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Runs the VM in either master or slave mode depending on the
 * platform. It does not return until the VM is finished. In slave mode
 * it will contain a system event loop.
 *
 * @param classPath string containing the class path
 * @param mainClass string containing the main class for the VM to run.
 * @param argc the number of arguments to pass to the main method
 * @param argv the arguments to pass to the main method
 *
 * @return exit status of the VM
 */
extern int midpRunVm(JvmPathChar* classPath,
                     char* mainClass,
                     int argc,
                     char** argv);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_START_VM_H */
