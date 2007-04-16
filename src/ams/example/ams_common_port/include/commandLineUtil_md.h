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


#ifndef _COMMAND_LINE_UTIL_MD_H_
#define _COMMAND_LINE_UTIL_MD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Declarations of functions common for NAMS and JAMS
 * having platform-dependent implementations.
 */

/**
 * Generates a correct MIDP home directory based on several rules. If
 * the <tt>MIDP_HOME</tt> environment variable is set, its value is used
 * unmodified. Otherwise, this function will search for the <tt>appdb</tt>
 * directory in the following order:
 * <ul>
 * <li>current directory (if the MIDP executable is in the <tt>PATH</tt>
 *     environment variable and the current directory is the right place)
 * <li>the parent directory of the midp executable
 * <li>the grandparent directory of the midp executable
 * </ul>
 * <p>
 * If <tt>cmd</tt> does not contain a directory (i.e. just the text
 * <tt>midp</tt>), the search starts from the current directory. Otherwise,
 * the search starts from the directory specified in <tt>cmd</tt> (i.e.
 * start in the directory <tt>bin</tt> if <tt>cmd</tt> is <tt>bin/midp</tt>).
 * <p>
 * <b>NOTE:</b> This is only applicable for development platforms.
 *
 * @param cmd A 'C' string containing the command used to start MIDP.
 * @return A 'C' string the found MIDP home directory, otherwise
 *         <tt>NULL</tt>, this will be a static buffer, so that it safe
 *       to call this function before midpInitialize, don't free it
 */
char* midpFixMidpHome(char *cmd);

#ifdef __cplusplus
}
#endif

#endif /* _COMMAND_LINE_UTIL_MD_H_ */
