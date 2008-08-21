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

/**
 * @file
 * @ingroup PCSL
 * @brief Interface for unit testing PCSL
 *
 */

#ifndef _DONUTS_H_
#define _DONUTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define assertTrue(x,y)  assertTrueImpl((x), (y), __FILE__, __LINE__)

/**
 * Checks if the assert is true.  If it is not, the message is output 
 * and the unit test framework stops executing.
 *
 * @param message message to print if the assert is false
 * @param assertion the assertion to test
 * @param filename the name of the file where the method was invoked
 * @param lineNumber the line of the file where the method was invoked
 */
void assertTrueImpl(char *message, int assertion, 
		    char *filename, int lineNumber);

#ifdef __cplusplus
}
#endif

#endif /* _DONUTS_H_ */
