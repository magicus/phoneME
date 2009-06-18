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
#ifndef __JUMP_PROCESS_H
#define __JUMP_PROCESS_H

#if defined __cplusplus 
extern "C" { 
#endif /* __cplusplus */
    
/**
 * Creates an Isolate process and returns the process ID of the 
 * created process. A negative return value indicates that the process cannot
 * be created.
 */
extern int jumpProcessCreate(int argc, char** argv);

/**
 * Is process 'pid' alive? 1, yes, 0, no.
 */
extern int jumpProcessIsAlive(int pid);

/**
 * Returns the process ID of this process as set by jumpProcessSetId().
 */
extern int jumpProcessGetId(void);

/**
 * Sets the process ID of this process for jumpProcessGetId() to
 * return.  This must be called before starting any threads which will
 * share the process ID.
 */
extern void jumpProcessSetId(int pid);

/**
 * Returns the process ID of the executive process.
 */
extern int jumpProcessGetExecutiveId(void);

/**
 * Sets the process ID of the executive process.
 */
extern void jumpProcessSetExecutiveId(int execPid);

#if defined __cplusplus 
}
#endif /* __cplusplus */
    
#endif /* __JUMP_PROCESS_H */
