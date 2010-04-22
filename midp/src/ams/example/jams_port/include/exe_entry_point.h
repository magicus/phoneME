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

#ifndef _EXE_ENTRY_POINT_H_
#define _EXE_ENTRY_POINT_H_

/*
 * Entry point of a midp executable is declared in this file.
 * This entry point is platform-dependent, for the provided implementations it is:
 *
 * default/
 * int main(int argc, char* argv[]);
 * 
 * wince/
 * int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
 *                    LPWSTR lpCmdLine, int nShowCmd);
 *
 * javacall/
 * javacall_result JavaTaskImpl(int argc, char* argv[]);
 *
 * Note that for javacall executables the actual function that will be called
 * by the Javacall library is JavaTask(). It is exported by the javacall_common
 * library and after some common tasks are done it calls JavaTaskImpl().
 */

#include <exe_entry_point_md.h>

#endif /* _EXE_ENTRY_POINT_H_ */
