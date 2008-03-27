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
#ifndef _MIDP_INIT_H_
#define _MIDP_INIT_H_

/**
 * @name Initialization levels
 *
 * One of these values is passed as parameter to the midpInit()
 * and midpInitCallback() functions.
 * @{
 */
#define NO_INIT 0      /**< no initialization yet */
#define MEM_LEVEL 1    /**< memory subsystem initialized */
#define LIST_LEVEL 2   /**< storage sub-system initialized
                            (the list of midlets) */
#define REMOVE_LEVEL 2 /**< de-facto it is the same as LIST_LEVEL
                    	    - suite storage initialization.
			    Porting efforts may include defining
			    REMOVE_LEVEL as 3 and adding extra
			    functionality to midpInit */
#define VM_LEVEL 4     /**< what is required to run midlets
                            (full initialization) */
/** @} */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @ingroup ams_base
 *
 * Interfaces used to initialize the midp runtime.
 */

/**
 * Internal function to initialize MIDP to only the level required by
 * the current functionality being used. This can be called multiple
 * times with the same level or lower. See midpInitCallback for more.
 *
 * @param level level of initialization required
 *
 * @return zero for success, non-zero if out of memory
 */
int midpInit(int level);

/**
 * Internal function for retrieving current MIDP initialization level.
 *
 * @return maximum initialization level achieved
 */
int getMidpInitLevel();

/**
 * Internal function to initialize MIDP to only the level required by
 * the current functionality being used. This can be called multiple
 * times with the same level or lower.
 * <p>
 * On device with more conventional operating systems like Linux,
 * listing, removing and running MIDlets may be done by different executables
 * and the executables may be linked statically. In this case
 * we do not want the list- and remove-MIDlet executables to have to link
 * the VM code, just the run executable. If the initialize and finalize
 * functions were referenced directly, the list- and remove- executables would
 * need to link them; by calling the VM functions indirectly we avoid this.
 *
 * @param level level of initialization required
 * @param init pointer to a VM init function that returns 0 for success
 * @param final pointer to a VM finalize function
 *
 * @return zero for success, non-zero if out of memory
 */
int midpInitCallback(int level, int (*init)(void),
    void (*final)(void));

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_INIT_H_ */

