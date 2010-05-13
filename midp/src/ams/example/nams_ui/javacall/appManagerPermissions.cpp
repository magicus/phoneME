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

#include "appManager.h"
#include "appManagerPermissions.h"

static javacall_ams_permission_val g_jpvPermissionValues[] =
    {JAVACALL_AMS_PERMISSION_VAL_BLANKET_DENIED,
     JAVACALL_AMS_PERMISSION_VAL_ONE_SHOT,
     JAVACALL_AMS_PERMISSION_VAL_SESSION,
     JAVACALL_AMS_PERMISSION_VAL_BLANKET_GRANTED};

const int PERMISSION_VAL_NUM = ((int) (sizeof(g_jpvPermissionValues) / sizeof(g_jpvPermissionValues[0])));

static LPTSTR g_szPermissionNames[PERMISSION_VAL_NUM] =
    {_T("Deny"), _T("One shot"), _T("Session"), _T("Allow")};


static BOOL UpdateTreeItem(HWND hwndTV, HTREEITEM hItem);


BOOL UpdateTreeItem(HWND hwndTV, HTREEITEM hItem) {
    TVITEM tvi;
    TVI_INFO* pInfo;
    int idx;

    pInfo = GetTviInfo(hwndTV, hItem);

    if (pInfo) {
        if (pInfo->type == TVI_TYPE_PERMISSION) {
            idx = PermissionValueToIndex(pInfo->permValue);
            if ((idx >= 0) && (idx < PERMISSION_VAL_NUM)) {
                tvi.iImage = tvi.iSelectedImage = idx;
                tvi.hItem = hItem;
                tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

                return TreeView_SetItem(hwndTV, &tvi);
            }
        }
    }

    return FALSE;
}

/**
 *  Processes messages for the permissions window.
 *
 */
LRESULT CALLBACK
PermissionWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    javacall_result res;
    HTREEITEM hItem, hParent;

    switch (message) {

    case WM_COMMAND: {
        TVI_INFO* pInfo;

        WORD wCmd = LOWORD(wParam);

        switch (wCmd) {

        case IDOK: {
            hItem = TreeView_GetRoot(hWnd);
            while (hItem) {
                pInfo = GetTviInfo(hWnd, hItem);
                if (pInfo && (pInfo->type == TVI_TYPE_PERMISSION)) {
                    if (pInfo->modified) {
                        res = javanotify_ams_suite_set_permission(
                            pInfo->suiteId, pInfo->permId, pInfo->permValue);

                        wprintf(
                            _T("javanotify_ams_suite_set_permission(%d, %d, %d)")
                            _T(" returned %d\n"),
                            (int)pInfo->suiteId, (int)pInfo->permId,
                            (int)pInfo->permValue, (int)res);
                    }
                }
                                                      
                hItem = TreeView_GetNextSibling(hWnd, hItem);
            }

            break;
        }

        case IDM_SUITE_SETTINGS: {
            TVI_INFO* pInfo = (TVI_INFO*)lParam;
            javacall_ams_permission_val
                jpvPermissions[JAVACALL_AMS_NUMBER_OF_PERMISSIONS];
            javacall_suite_id suiteId;

            // Clear old content
            TreeView_DeleteAllItems(hWnd);

            // Set the position info to default
            hPrev = (HTREEITEM)TVI_FIRST; 
            hPrevLev1Item = NULL; 
            hPrevLev2Item = NULL;

            if (pInfo) {
                suiteId = pInfo->suiteId;

                wprintf(_T("Displaying settings for suite id=%d...\n"),
                        (int)suiteId);

                res = javanotify_ams_suite_get_permissions(suiteId,
                                                           jpvPermissions);
                if (res == JAVACALL_OK) {
                    javacall_ams_permission_val jpvVal;
                    TCHAR szBuf[127];
                    int nIndex;

                    for (int p = 0; p < JAVACALL_AMS_NUMBER_OF_PERMISSIONS; p++) {
                        jpvVal = jpvPermissions[p];

                        if (jpvVal == JAVACALL_AMS_PERMISSION_VAL_INVALID ||
                            jpvVal == JAVACALL_AMS_PERMISSION_VAL_NEVER ||
                            jpvVal == JAVACALL_AMS_PERMISSION_VAL_ALLOW) {
                            continue;
                        }

                        nIndex = PermissionValueToIndex(jpvVal);
                        if ((nIndex < 0) || (nIndex >= PERMISSION_VAL_NUM)) {
                            continue;
                        }

                        wsprintf(szBuf, _T("Permission %d"), p);

                        pInfo = CreateTviInfo();
                        pInfo->type = TVI_TYPE_PERMISSION;
                        pInfo->suiteId = suiteId;
                        pInfo->permId = (javacall_ams_permission)p;
                        pInfo->permValue = jpvVal;

                        AddTreeItem(hWnd, szBuf, 1, pInfo);

                        for (int n = PERMISSION_VAL_NUM; n > 0; n--) {
                            pInfo = CreateTviInfo();
                            pInfo->type = TVI_TYPE_PERMISSION;
                            pInfo->suiteId = suiteId;
                            pInfo->permId = (javacall_ams_permission)p;
                            pInfo->permValue = g_jpvPermissionValues[n - 1];

                            AddTreeItem(hWnd, g_szPermissionNames[n - 1], 2, pInfo);
                        }
                    }
                }
            }
            break;
        }

        } // end of switch (wCmd)

        break;
    }

    case WM_LBUTTONDBLCLK: {
        hItem = HitTest(hWnd, lParam);
        if (hItem)
        {
            hParent = TreeView_GetParent(hWnd, hItem);

            if (hParent) {
                // collapse child items upon double click on any of them
                TreeView_Expand(hWnd, hParent, TVE_COLLAPSE);
            } else {
                // imitate default tree view behavior if the click is
                // on a root item
                TreeView_Expand(hWnd, hItem, TVE_TOGGLE);
            }
        }

        break;
    }

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
        hItem = HitTest(hWnd, lParam);
        if (hItem)
        {
            hParent = TreeView_GetParent(hWnd, hItem);

            // If the hitted item represents permission value then
            // change value of the permission itslef
            if (hParent) {
                TVI_INFO* pParentInfo = GetTviInfo(hWnd, hParent);
                TVI_INFO* pInfo = GetTviInfo(hWnd, hItem);

                pParentInfo->permValue = pInfo->permValue;
                pParentInfo->modified = TRUE;

                UpdateTreeItem(hWnd, hParent);
            }
        }

        break;
    }

    case WM_ERASEBKGND: {
        hdc = (HDC)wParam;
        DrawBackground(hdc, SRCCOPY);
        return 1;
    }

    case WM_PAINT: {
        PaintTreeWithBg(hWnd, message, wParam, lParam);
        break;
    }

    default:
        return CallWindowProc(GetDefTreeWndProc(), hWnd, message, wParam,
            lParam);
    }

    return 0;
}

int PermissionValueToIndex(javacall_ams_permission_val jpPermission) {
    for (int i = 0; i < PERMISSION_VAL_NUM; i++) {
        if (g_jpvPermissionValues[i] == jpPermission) {
            return i;
        }
    }
    return -1;
}
