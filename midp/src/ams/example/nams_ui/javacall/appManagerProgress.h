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
#ifndef __APP_MANAGER_PROGRESS_H
#define __APP_MANAGER_PROGRESS_H

#include <windows.h>
#include <javacall_ams_installer.h>

typedef struct _InstallRequestData {
    javacall_ams_install_state installState;
    javacall_ams_install_data  requestData;
} InstallRequestData;


BOOL CreateProgressDialog(HINSTANCE hInstance, HWND hWndParent);

void ShowProgressDialog(BOOL fShow);

BOOL PostProgressMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif  /* __APP_MANAGER_PROGRESS_H */
