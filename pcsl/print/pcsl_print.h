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

#ifndef _PCSLPRINT_H_
#define _PCSLPRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup print Diagnostic Printing Interfaces
 */

/**
 * @defgroup print_high_interface High Level Interface
 * @ingroup print
 */

/**
 * @defgroup print_low_interface Low Level Interface
 * @ingroup print
 */

/**
 * @file
 * @ingroup print
 */

/**
 * @addtogroup print_high_interface
 * @brief Interface for handling print operations. \n
 * ##include <pcsl_print.h>
 * @{
 *
 * The print service can be used to output text either to
 * stdout or to a file.
 *
 */

/**
 * Prints out a string to a system specific output stream
 * @param s a NULL terminated character buffer to be printed
 *
 */
void pcsl_print(const char* s);

/**
 * Prints out an array of characters to a system specific output stream
 * @param s address of the first character to print
 * @param length number of characters to print
 *
 */
void pcsl_print_chars(const char* s, int length);

/** @} */   //End of group High Level Interface

/**
 *
 * @addtogroup print_low_interface
 * @brief Low Level Interface using standard io functions \n
 * ##include <stdio.h>
 *
 * @{
 * If you are using the supplied <b>stdout</b> print module, then the target
 * platform needs to supply two io functions, commonly available in the
 * standard C library and defined in <stdio.h>. These two functions are: \n
 *
 * <b>int fprintf(FILE *stream,  const  char  *format, ...);</b> \n
 * <b>int fflush(FILE *stream);</b> \n
 *
 * The supplied <b>file</b> print module, is dependent solely on \n
 * pcsl_file_open(), pcsl_file_write() and pcsl_file_close().\n
 * Refer to File's @ref file_low_interface, specifically to functions open(),
 * close() and write().
 *
 */
/** @} */   //End of group Low Level Interface

#ifdef __cplusplus
}
#endif

#endif /* _PCSLPRINT_H_ */
