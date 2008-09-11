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
#ifndef __APP_MANAGER_H
#define __APP_MANAGER_H

#include "res/appManager_resource.h"

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include <javacall_defs.h>
#include <javacall_ams_suitestore.h>

#ifdef __cplusplus
extern "C" {
#endif


// TODO: place all hPrev* fields in a structure and pass it as
//  a parameter of AddSuiteToTree
extern HTREEITEM hPrev;
extern HTREEITEM hPrevLev1Item;
extern HTREEITEM hPrevLev2Item;


// The type of a tree item
typedef enum {    
    TVI_TYPE_UNKNOWN,
    TVI_TYPE_SUITE,
    TVI_TYPE_MIDLET,
    TVI_TYPE_FOLDER,
    TVI_TYPE_PERMISSION
} tvi_type;

typedef struct _TVI_INFO {
    tvi_type type; // type of the node, valid values are TVI_TYPE_SUITE,
                   // TVI_TYPE_MIDLET, TVI_TYPE_FOLDER and TVI_TYPE_PERMISSION

    javacall_utf16_string className; // MIDlet class name if item type is
                                     // TVI_TYPE_MIDLET

    javacall_utf16_string displayName; // Name to display, works for all types
                                       // but TVI_TYPE_PERMISSION

    javacall_suite_id suiteId; // id of the suite, makes sense if item type is
                               // TVI_TYPE_MIDLET, TVI_TYPE_SUITE and
                               // TVI_TYPE_PERMISSION

    javacall_app_id appId; // external application id if item type is
                           // TVI_TYPE_MIDLET and the MIDlet is running

    javacall_folder_id folderId; // folder ID, applicable for all TVI types but
                                 // TVI_TYPE_PERMISSION

    javacall_ams_permission permId; // permission ID, used if the type is
                                    // TVI_TYPE_PERMISSION

    javacall_ams_permission_val permValue; // permission value, used if the
                                           // type is TVI_TYPE_PERMISSION

    BOOL modified;  // indicates whether the item was modified,
                    // i.e. the suite storage should be updated accordingly

} TVI_INFO;

TVI_INFO* CreateTviInfo();
void FreeTviInfo(TVI_INFO* pInfo);
TVI_INFO* GetTviInfo(HWND hWnd, HTREEITEM hItem);

HTREEITEM AddTreeItem(HWND hwndTV, LPTSTR lpszItem,
                               int nLevel, TVI_INFO* pInfo);

HTREEITEM HitTest(HWND hWnd, LPARAM lParam);

WNDPROC GetDefTreeWndProc();

void DrawBackground(HDC hdc, DWORD dwRop);
void PaintTreeWithBg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL PostProgressMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

void RemoveMIDletFromRunningList(javacall_app_id appId);
void SwitchToAppManager();

#ifdef __cplusplus
}
#endif

#endif  /* __APP_MANAGER_H */