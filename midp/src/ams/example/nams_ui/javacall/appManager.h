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

/** Width of the client area of the main window */
#define MAIN_WINDOW_CHILD_AREA_WIDTH  240
/** Height of the client area of the main window */
#define MAIN_WINDOW_CHILD_AREA_HEIGHT 300

/** The string that appears in the application's title bar */
const TCHAR g_szTitle[] = _T("NAMS Example");

/*
 * IMPL_NOTE: all hPrev* fields should be saved into a structure, and then it
 *            should be passed as a parameter to AddSuiteToTree
 */
extern HTREEITEM hPrev;
extern HTREEITEM hPrevLev1Item;
extern HTREEITEM hPrevLev2Item;

/**
 * The type of a tree item.
 */
typedef enum {    
    TVI_TYPE_UNKNOWN,
    TVI_TYPE_SUITE,
    TVI_TYPE_MIDLET,
    TVI_TYPE_FOLDER,
    TVI_TYPE_PERMISSION,
    TVI_TYPE_DIALOG
} tvi_type;

/**
 * The type of data associated with a tree item.
 */
typedef struct _TVI_INFO {
    tvi_type type; // type of the node, valid values are TVI_TYPE_SUITE,
                   // TVI_TYPE_MIDLET, TVI_TYPE_FOLDER, TVI_TYPE_PERMISSION
                   // and TVI_TYPE_DIALOG

    javacall_utf16_string className; // MIDlet class name if item type is
                                     // TVI_TYPE_MIDLET

    javacall_utf16_string displayName; // Name to display, works for all types
                                       // but TVI_TYPE_PERMISSION
                                       // and TVI_TYPE_DIALOG

    javacall_suite_id suiteId; // id of the suite, makes sense if item type is
                               // TVI_TYPE_MIDLET, TVI_TYPE_SUITE and
                               // TVI_TYPE_PERMISSION

    javacall_app_id appId; // external application ID if item type is
                           // TVI_TYPE_MIDLET and the MIDlet is running
                           // or type is TVI_TYPE_DIALOG

    javacall_folder_id folderId; // folder ID, applicable for all TVI types but
                                 // TVI_TYPE_PERMISSION and TVI_TYPE_DIALOG

    javacall_ams_permission permId; // permission ID, used if the type is
                                    // TVI_TYPE_PERMISSION

    javacall_ams_permission_val permValue; // permission value, used if the
                                           // type is TVI_TYPE_PERMISSION

    BOOL modified;  // indicates whether the item was modified,
                    // i.e. the suite storage should be updated accordingly

} TVI_INFO;

/**
 * Creates and initializes a new TVI_INFO structure.
 *
 * @return pointer to the created structure if succeeded, NULL if failed
 */
TVI_INFO* CreateTviInfo();

/**
 * Frees the memory allocated for the given TVI_INFO structure.
 *
 * @param pointer to the structure to free
 */
void FreeTviInfo(TVI_INFO* pInfo);

/**
 * Retrieves the data associated with the given tree view item.
 *
 * @param hWnd  handle to the tree view control
 * @param hItem handle to the tree item whose info is being retrieved
 *
 * @return data associated with the given item if succeeded, NULL if failed
 */
TVI_INFO* GetTviInfo(HWND hWnd, HTREEITEM hItem);

/**
 * Adds a new item into the tree view control.
 *
 * @param hwndTV handle to the tree view control into which an item
 *               should be added
 * @param lpszItem text to be displayed for the new item
 * @param nLevel nesting level of the new item
 * @param pInfo data that will be associated with the new item
 *
 * @return handle to the newly added item if succeeded, NULL if failed
 */
HTREEITEM AddTreeItem(HWND hwndTV, LPTSTR lpszItem,
                      int nLevel, TVI_INFO* pInfo);
/**
 * Helper function for mouse events.
 *
 * @param hWnd handle to the window that received a mouse event
 * @param lParam lParam passed to the window procedure handling the mouse event
 *
 * @return the tree item that was clicked or NULL if there is no such item
 */
HTREEITEM HitTest(HWND hWnd, LPARAM lParam);

/**
 * Retrieves the default window procedure of the tree control.
 *
 * @return pointer to the default window procedure of the tree control
 */
WNDPROC GetDefTreeWndProc();

/**
 * Draws the background image.
 *
 * @param dc device context where to draw
 * @param dwRop raster operation to use when drawing
 */
void DrawBackground(HDC hdc, DWORD dwRop);

/**
 * Draws a tree view control together with a background.
 *
 * @param hWnd handle to the tree view control to draw
 * @param uMsg original message (WM_PAINT) to be passed to the default window
 *             procedure of the tree view
 * @param wParam original value of wParam to be passed to the default window
 *               procedure of the tree view
 * @param lParam original value of lParam to be passed to the default window
 *               procedure of the tree view
 */
void PaintTreeWithBg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * Forces screen update.
 *
 * @param x1 left coordinate of the rectangle to update
 * @param y1 top coordinate of the rectangle to update
 * @param x2 right coordinate of the rectangle to update
 * @param y2 bottom coordinate of the rectangle to update
 */
void RefreshScreen(int x1, int y1, int x2, int y2);

/**
 * Handles notification that a midlet has just terminated.
 *
 * @param appID application ID of the terminated midlet
 */
void MIDletTerminated(javacall_app_id appId);

/**
 * Closes the installation dialog.
 *
 * @param hwndDlg handle
 */
void CloseInstallerDlg(HWND hwndDlg);

#endif  /* __APP_MANAGER_H */
