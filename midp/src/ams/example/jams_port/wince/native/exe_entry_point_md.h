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

#ifndef _EXE_ENTRY_POINT_MD_H_
#define _EXE_ENTRY_POINT_MD_H_

#if !defined _EXE_ENTRY_POINT_MD_H_
# error "Never include <exe_entry_point_md.h> directly; use <exe_entry_point.h> instead."
#endif

/**
 * Entry point of the executable.
 *
 * @param hInstance handle to the current instance of the application.
 * @param hPrevInstance handle to the previous instance of the application. For a
 *                      Win32-based application, this parameter is always NULL. 
 * @param lpCmdLine pointer to a null-terminated string that specifies the command
 *                  line for the application, excluding the program name.
 * @param nShowCmd specifies how the window is to be shown.
 *
 * @return the exit value
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPWSTR lpCmdLine, int nShowCmd);

#endif /* _EXE_ENTRY_POINT_MD_H_ */
