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

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <commctrl.h> // common controls for tree view

#include "res/namsui_resource.h"

#include <javacall_memory.h>
#include <javautil_unicode.h>
#include <javacall_lcd.h>
#include <javacall_ams_suitestore.h>
#include <javacall_ams_app_manager.h>

#define TB_BUTTON_WIDTH 32

extern "C" char* _phonenum = "1234567"; // global for javacall MMS subsystem

// The main window class name.
static TCHAR g_szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR g_szTitle[] = _T("NAMS Example");

// The size of main window calibrated to get 240x320 child area to draw SJWC output to
int g_iWidth = 246, g_iHeight = 345;
int g_iChildAreaWidth = 240, g_iChildAreaHeight = 320;

HINSTANCE g_hInst = NULL;
HWND g_hMainWindow = NULL;
HMENU g_hMidletPopupMenu = NULL;
WNDPROC g_DefTreeWndProc = NULL;
HBITMAP g_hMidletTreeBgBmp = NULL;

javacall_app_id g_jAppId = 1;


// The type of a tree item
static int TVI_TYPE_UNKNOWN = 0;
static int TVI_TYPE_SUITE   = 1;
static int TVI_TYPE_MIDLET  = 2;
static int TVI_TYPE_FOLDER  = 3;

typedef struct _TVI_INFO { 
    int type; // type of the node, valid values are TVI_TYPE_SUITE, 
              // TVI_TYPE_MIDLET, TVI_TYPE_FOLDER
    javacall_const_utf16_string className; // MIDlet class name if item type is
                                           // TVI_TYPE_MIDLET
    javacall_suite_id suiteId; // id of the suite, makes sense if item type is
                               // TVI_TYPE_MIDLET and TVI_TYPE_SUITE
    javacall_app_id appId; // external application id if item type is
                           // TVI_TYPE_MIDLET and the MIDlet is running
} TVI_INFO;


// Forward declarations of functions included in this code module:

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MidletTreeWndProc(HWND, UINT, WPARAM, LPARAM);

static void RefreshScreen(int x1, int y1, int x2, int y2);
static void DrawBuffer(HDC hdc);

HWND CreateMainView();
static HWND CreateTreeView(HWND hWndParent);
HWND CreateToolbar(HWND hWndParent);
BOOL InitTreeViewItems(HWND hwndTV);
HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, TVI_INFO* pInfo);

static void InitAms();
static void CleanupAms();
static void CleanupWindows();
static void CleanupTreeView(HWND hwndTV);

LPTSTR JavacallUtf16ToTstr(javacall_const_utf16_string str);
javacall_utf16_string CloneJavacallUtf16(javacall_const_utf16_string str);


extern "C" {

javacall_result java_ams_system_start();
javacall_result java_ams_system_stop();
javacall_result
java_ams_midlet_start(javacall_suite_id suiteId,
                      javacall_app_id appId,
                      javacall_const_utf16_string className,
                      const javacall_midlet_runtime_info* pRuntimeInfo);

};

/**
 * Entry point of the Javacall executable.
 *
 * @param argc number of arguments (1 means no arguments)
 * @param argv the arguments, argv[0] is the executable's name
 *
 * @return the exit value (1 if OK)
 */
extern "C" javacall_result JavaTaskImpl(int argc, char* argv[]) {
//    javacall_events_init();
//    javacall_initialize_configurations();

    javacall_result res = java_ams_system_start();
//    java_ams_appmgr_start();

    TCHAR szMsg[128];
    wsprintf(szMsg, _T("SJWC exited, code: %d."), (int)res); 
    MessageBox(NULL, szMsg, g_szTitle, MB_OK);

//    javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN,
//        (res == 1) ? JAVACALL_OK : JAVACALL_FAIL);
    return res;
}

DWORD WINAPI javaThread(LPVOID lpParam) {
    JavaTaskImpl(0, NULL);
    return 0; 
} 

#if 0
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow) {
    (void)lpCmdLine;
#else
int main(int argc, char* argv[]) {
    HINSTANCE hInstance = NULL;
    int nCmdShow = SW_SHOWNORMAL;
#endif

    // Store instance handle in our global variable.
    g_hInst = hInstance;

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    g_hMainWindow = CreateMainView();

    // Start JVM in a separate thread
    DWORD dwThreadId; 
    HANDLE hThread = CreateThread( 
        NULL,                    // default security attributes 
        0,                       // use default stack size  
        javaThread,              // thread function 
        NULL,                    // argument to thread function
        0,                       // use default creation flags 
        &dwThreadId);            // returns the thread identifier

    if (!hThread) {
        MessageBox(g_hMainWindow,
            _T("Can't start Java Thread!"),
            g_szTitle,
            NULL);

        return 1;
    }

    // Let native peer to start
    // TODO: wait for notification from the peer instead of sleep
    Sleep(1000);
    
    // Initialize Java AMS
    InitAms();

    // Create and init Java MIDlets tree view
    HWND hwndTV = CreateTreeView(g_hMainWindow);
    if (hwndTV == NULL) {
        return -1;
    }
    InitTreeViewItems(hwndTV);

    HWND hWndToolbar = CreateToolbar(g_hMainWindow);
    if (hWndToolbar == NULL) {
        return -1;
    }

    // Show the main window 
    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    CleanupTreeView(hwndTV);

    // Finalize Java AMS
    CleanupAms();

    CleanupWindows();

    return (int) msg.wParam;
}

static HWND CreateToolbar(HWND hWndParent) {
    RECT rcClient;  // dimensions of client area 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hWndParent, &rcClient); 

    HWND hWndToolbar = CreateWindowEx(0,
                            TOOLBARCLASSNAME,
                            NULL,
                            WS_VISIBLE | WS_CHILD,
                            0, 0, rcClient.right, TB_BUTTON_WIDTH,
                            hWndParent, 
                            (HMENU)IDC_MAIN_TOOLBAR,
                            g_hInst, 
                            NULL); 

    // Store default Tree View WndProc in global variable
    // and set custom WndProc.
    //g_DefTreeWndProc = (WNDPROC)SetWindowLongPtr(hWndToolbar, GWLP_WNDPROC,
    //    (LONG)MidletTreeWndProc);

    if (!hWndToolbar) {
        MessageBox(NULL, _T("Can't create a toolbar!"), g_szTitle, NULL);
        return NULL;
    }

    TBADDBITMAP toolbarImg;

    toolbarImg.hInst = g_hInst;
    toolbarImg.nID = IDB_MAIN_TOOLBAR_BUTTONS;

    SendMessage(hWndToolbar, TB_ADDBITMAP, 1,
                (LPARAM)(LPTBADDBITMAP)&toolbarImg);

    TBBUTTON pButtons[2];

    pButtons[0].iBitmap = 0;
    pButtons[0].idCommand = IDM_HELP_ABOUT;
    pButtons[0].fsState = TBSTATE_ENABLED;
    pButtons[0].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
    pButtons[0].dwData = 0;
    pButtons[0].iString = 0;

    SendMessage(hWndToolbar, TB_ADDBUTTONS, 1, (LPARAM)(LPTBBUTTON)pButtons);

    return hWndToolbar;
}

static HWND CreateMainView() {
    HWND hWnd;
    WNDCLASSEX wcex;

    // Customize main view class to assign own WndProc
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = MainWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(ID_MENU_MAIN);
    wcex.lpszClassName  = g_szWindowClass;
    wcex.hIconSm        = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL,
            _T("Can't register main view class!"),
            g_szTitle,
            NULL);

        return NULL;
    }

    hWnd = CreateWindow(
        g_szWindowClass,
        g_szTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_iWidth, g_iHeight,
        NULL,
        NULL,
        g_hInst,
        NULL
    );

    if (!hWnd) {
        MessageBox(NULL,
            _T("Create of main view failed!"),
            g_szTitle,
            NULL);

        return NULL;
    }
    
    return hWnd;
}

static void InitAms() {
    javacall_result res = java_ams_suite_storage_init();
    if (res == JAVACALL_FAIL) {
        wprintf(_T("Init of suite storage fail!\n"));
    }
}

static void CleanupAms() {
    javacall_result res = java_ams_suite_storage_cleanup();
    if (res == JAVACALL_FAIL) {
        wprintf(_T("Cleanup of suite storage fail!\n"));
    }
}

static void CleanupWindows() {
    // Clean up resources allocated for MIDlet popup menu 
    DestroyMenu(g_hMidletPopupMenu);

    // Unregister main window class
    UnregisterClass(g_szWindowClass, g_hInst);
}

static void CleanupTreeView(HWND hwndTV) {
    // IMPL_NOTE: memory allocated by the application is freed in MainWndProc
    // by handling WM_NOTIFY message
    TreeView_DeleteAllItems(hwndTV);

    // Return back window procedure for tree view
    SetWindowLongPtr(hwndTV, GWLP_WNDPROC, (LONG)g_DefTreeWndProc);
}

static HWND CreateTreeView(HWND hWndParent) {
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree-view control 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hWndParent, &rcClient); 
    wprintf(_T("main window area w=%d, h=%d\n"), rcClient.right, rcClient.bottom);

    hwndTV = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Java Midlets"),
                            WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES |
                                TVS_HASBUTTONS | TVS_LINESATROOT,
                            0, 
                            TB_BUTTON_WIDTH - 4,
                            rcClient.right,
                            rcClient.bottom,
                            hWndParent, 
                            (HMENU)IDC_TREEVIEW_MIDLETS,
                            g_hInst, 
                            NULL); 

    // Store default Tree View WndProc in global variable
    // and set custom WndProc.
    g_DefTreeWndProc = (WNDPROC)SetWindowLongPtr(hwndTV, GWLP_WNDPROC,
        (LONG)MidletTreeWndProc);

    if (!hwndTV) {
        MessageBox(NULL, _T("Create tree view failed!"), g_szTitle, NULL);
        return NULL;
    }


    // Load backround image, just ignore if loading fails
    /*HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDB_MIDLET_TREE_BG), RT_BITMAP);
    if (!hRes) {
        DWORD res = GetLastError();
        wprintf(_T("ERROR: LoadResource() res: %d\n"), res);
    }
    g_hMidletTreeBgBmp = (HBITMAP)LoadResource(NULL, hRes);*/

//   g_hMidletTreeBgBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_MIDLET_TREE_BG));
    g_hMidletTreeBgBmp = (HBITMAP)LoadImage(g_hInst, _T("bgd-yellow.bmp"),
        IMAGE_BITMAP, g_iChildAreaWidth, g_iChildAreaHeight, LR_LOADFROMFILE);
    if (!g_hMidletTreeBgBmp) {
        DWORD res = GetLastError();
        wprintf(_T("ERROR: LoadBitmap(IDB_MIDLET_TREE_BG) res: %d\n"), res);
    }

    // Load context menu shown for a MIDlet item in the tree view
    g_hMidletPopupMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(ID_MENU_POPUP_MIDLET));
    if (!g_hMidletPopupMenu) {
        MessageBox(NULL,
            _T("Can't load MIDlet popup menu!"),
            g_szTitle,
            NULL);
    }

    return hwndTV;
}

BOOL InitTreeViewItems(HWND hwndTV)  {
    javacall_suite_id* pSuiteIds;
    int suiteNum;
    javacall_result res = JAVACALL_FAIL;
    javacall_ams_suite_info* pSuiteInfo;
    const size_t ciTviInfoSize = sizeof(TVI_INFO);


    // TODO: Add all folders to the tree view
   

    /* Iterrate over all suites and add them to the tree view */
    
    res = java_ams_suite_get_suite_ids(&pSuiteIds, &suiteNum);

    if (res != JAVACALL_OK) {
        wprintf(_T("ERROR: java_ams_suite_get_suite_ids() returned: %d\n"), res);
        return FALSE;
    }

    wprintf(_T("Total suites found: %d\n"), suiteNum);

#if 1
    for (int i = 0; i < suiteNum; i++) {
        res = java_ams_suite_get_info(pSuiteIds[i], &pSuiteInfo);
        if (res == JAVACALL_OK) {
          if (pSuiteInfo != NULL) {
              TVI_INFO* pInfo;
              javacall_utf16_string jsLabel;

              // TODO: add support for disabled suites
              // javacall_bool enabled = suiteInfo[i].isEnabled;

              // TODO: take into account folder ID
              // javacall_int32 fid = suiteInfo[i].folderId;

              jsLabel = (pSuiteInfo->displayName != NULL) ?
                  pSuiteInfo->displayName : pSuiteInfo->suiteName;

              LPTSTR pszSuiteName = JavacallUtf16ToTstr(jsLabel);
              pszSuiteName = pszSuiteName ? pszSuiteName : _T("Midlet Suite");
              wprintf(_T("Suite label=%s\n"), pszSuiteName);

              pInfo = (TVI_INFO*)javacall_malloc(ciTviInfoSize);
              memset(pInfo, 0, ciTviInfoSize);
              pInfo->type = TVI_TYPE_SUITE;
              pInfo->suiteId = pSuiteIds[i];
              AddItemToTree(hwndTV, pszSuiteName, 1, pInfo);

              javacall_ams_midlet_info* pMidletsInfo;
              javacall_int32 midletNum;
              res = java_ams_suite_get_midlets_info(pSuiteIds[i], &pMidletsInfo,
                  &midletNum);
              if (res == JAVACALL_OK) {
//                  if (midletNum > 1) {
                      wprintf(_T("Total MIDlets in the suite %d\n"), midletNum);

                      for (int j = 0; j < midletNum; j++) {
                          jsLabel = (pMidletsInfo[j].displayName != NULL) ?
                              pMidletsInfo[j].displayName :
                                  pMidletsInfo[j].className;

                          if (jsLabel == NULL) {
                              continue;
                          }

       	                  LPTSTR pszMIDletName = JavacallUtf16ToTstr(jsLabel);
                          wprintf(_T("MIDlet label=%s, className=\n"), pszMIDletName, pMidletsInfo[j].className);

                          pInfo = (TVI_INFO*)javacall_malloc(ciTviInfoSize);
                          memset(pInfo, 0, ciTviInfoSize);
                          pInfo->type = TVI_TYPE_MIDLET;
                          pInfo->suiteId = pSuiteIds[i];
                          pInfo->className = CloneJavacallUtf16(pMidletsInfo[j].className);
                          AddItemToTree(hwndTV, pszMIDletName, 2, pInfo);
                      }
//                  }
                      if (midletNum > 0) {
                          java_ams_suite_free_midlets_info(pMidletsInfo, midletNum);
                      }
              } else {
                  wprintf(_T("ERROR: java_ams_suite_get_midlets_info() returned: %d\n"), res);
              }
          } else {
              wprintf(_T("ERROR: suite info is null\n"));
          }
          java_ams_suite_free_info(pSuiteInfo);
        } else {
            wprintf(_T("ERROR: java_ams_suite_get_info() returned: %d\n"), res);
        }
    } // end for

    if (suiteNum > 0) {
        java_ams_suite_free_suite_ids(pSuiteIds, suiteNum);
    }
#else
    // Items for testing
    AddItemToTree(hwndTV, _T("MIDlet 1"), 1, 0);
    AddItemToTree(hwndTV, _T("MIDlet 2"), 1, 0);
#endif

    return TRUE;
}

LPTSTR JavacallUtf16ToTstr(javacall_const_utf16_string str) {
    LPTSTR result = NULL;
#ifdef UNICODE 
    javacall_int32 len;
    javacall_result res = javautil_unicode_utf16_ulength(str, &len);
    if (res == JAVACALL_OK) {
        const size_t bufLen = (len + 1) * sizeof(WCHAR);
        result = (LPTSTR)javacall_malloc(bufLen);
        memcpy(result, str, bufLen);
    }
#else
# error "Only Unicode platforms are supported for now"
#endif
    return result;
}

javacall_utf16_string CloneJavacallUtf16(javacall_const_utf16_string str) {
    javacall_utf16_string result = NULL;
    javacall_int32 len;
    javacall_result res = javautil_unicode_utf16_ulength(str, &len);
    if (res == JAVACALL_OK) {
        const size_t bufLen = (len + 1) * sizeof(javacall_utf16);
        result = (javacall_utf16_string)javacall_malloc(bufLen);
        memcpy(result, str, bufLen);
    }
    return result;
}



HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, TVI_INFO* pInfo) {
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti;

    if (lpszItem == NULL) {
        return NULL;
    }

    tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE*/ | TVIF_PARAM; 

    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]); 

    // Assume the item is not a parent item, so give it a 
    // document image. 
/*
    tvi.iImage = g_nDocument; 
    tvi.iSelectedImage = g_nDocument; 
*/

    tvi.lParam = (LPARAM)pInfo;
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 

    // Set the parent item based on the specified level. 
    if (nLevel == 1) 
        tvins.hParent = TVI_ROOT; 
    else if (nLevel == 2) 
        tvins.hParent = hPrevRootItem; 
    else 
        tvins.hParent = hPrevLev2Item; 

    // Add the item to the tree-view control.
/*
    hPrev = (HTREEITEM)SendMessage(hwndTV, 
                                   TVM_INSERTITEM,
                                   0,
                                   (LPARAM)(LPTVINSERTSTRUCT)&tvins);
*/
    hPrev = TreeView_InsertItem(hwndTV, &tvins);


    // Save the handle to the item. 
    if (nLevel == 1) 
        hPrevRootItem = hPrev; 
    else if (nLevel == 2) 
        hPrevLev2Item = hPrev; 

    // The new item is a child item. Give the parent item a
    // closed folder bitmap to indicate it now has child items. 
/*
    if (nLevel > 1)
    { 
        hti = TreeView_GetParent(hwndTV, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        tvi.iImage = g_nClosed; 
        tvi.iSelectedImage = g_nClosed; 
        TreeView_SetItem(hwndTV, &tvi); 
    } 
*/

    return hPrev; 
}

/**
 *  Processes messages for the main window.
 *
 */
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_COMMAND: {
        switch(LOWORD(wParam)) {
            case IDM_MIDLET_START_STOP: {
                break;
            }
            case IDM_SUITE_EXIT: {
                (void)java_ams_system_stop();
                // TODO: wait for notification from the SJWC thread instead of sleep
                Sleep(1000);
                PostQuitMessage(0);
                break;
            }
            case IDM_HELP_ABOUT: {
                MessageBox(hWnd, _T("Cool Application Manager"),
                           _T("About"), MB_OK | MB_ICONINFORMATION);
                break;
            }
        }
        break;
    }

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        DrawBuffer(hdc);

        EndPaint(hWnd, &ps);
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pHdr = (LPNMHDR)lParam;
        switch (pHdr->code)
        {
            case TVN_DELETEITEM:
                if(pHdr->idFrom == IDC_TREEVIEW_MIDLETS) {
                    TVITEM tvi = ((LPNMTREEVIEW)lParam)->itemOld;
                    javacall_free(tvi.pszText);
                    javacall_free((void*)tvi.lParam);
                }
            break;
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void DrawBackground(HDC hdc, DWORD dwRop) {
    if (g_hMidletTreeBgBmp != NULL) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_hMidletTreeBgBmp);

        BITMAP bm;
        GetObject(g_hMidletTreeBgBmp, sizeof(bm), &bm);
 
        //wprintf(_T(">>> bm.bmWidth = %d, bm.bmHeight = %d\n"), bm.bmWidth, bm.bmHeight);
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, dwRop);
 
        SelectObject(hdcMem, hbmOld);  
        DeleteDC(hdcMem); 
    }
}

/*
void DrawItem() {
    RECT rc;
    *(HTREEITEM*)&rc = hTreeItem;
    SendMessage(hwndTreeView, TVM_GETITEMRECT, FALSE, (LPARAM)&rc);
}
*/

/**
 *  Processes messages for the MIDlet tree window.
 *
 */
LRESULT CALLBACK
MidletTreeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    javacall_result res;

    switch (message) {

    case WM_RBUTTONDOWN:
    {
        TV_HITTESTINFO tvH;

        tvH.pt.x = LOWORD(lParam);
        tvH.pt.y = HIWORD(lParam);

//        wprintf (_T("Click position (%d, %d)\n"), tvH.pt.x, tvH.pt.y);

        HTREEITEM hItem = TreeView_HitTest(hWnd, &tvH);
        if (hItem && (tvH.flags & TVHT_ONITEM))
        {
//            wprintf (_T("Hit flags hex=%x\n"), tvH.flags);

            // Mark the item as selected
            TreeView_SelectItem(hWnd, hItem);

            // Convert the coordinates to global ones
            ClientToScreen(hWnd, (LPPOINT) &tvH.pt);

            TVITEM tvi;
            tvi.hItem = hItem;
            tvi.mask = TVIF_HANDLE | TVIF_PARAM;
            if (TreeView_GetItem(hWnd, &tvi)) {
                if (((TVI_INFO*)tvi.lParam)->type == TVI_TYPE_SUITE) {
                    // Get the first shortcut menu in the menu template.
                    // This is the menu that TrackPopupMenu displays
                    HMENU hMenu = GetSubMenu(g_hMidletPopupMenu, 0);
                    if (hMenu) {
                        TrackPopupMenu(hMenu, 0, tvH.pt.x, tvH.pt.y, 0, hWnd,
                            NULL);
                    } else {
                        MessageBox(NULL, _T("Can't show context menu!"),
                            g_szTitle, MB_OK);
                    }
                }
            }
        }

        break;
    }

    case WM_LBUTTONDBLCLK: 
    {
        HTREEITEM hItem = TreeView_GetSelection(hWnd);
        if (hItem) {
            TVITEM tvi;
            tvi.hItem = hItem;
            tvi.mask = TVIF_HANDLE | TVIF_PARAM;
            if (TreeView_GetItem(hWnd, &tvi)) {
                TVI_INFO* pInfo = (TVI_INFO*)tvi.lParam;
                if (pInfo->type == TVI_TYPE_MIDLET) {
                    wprintf(_T("Launching MIDlet (suiteId=%d, class=%S, appId=%d)...\n"),
                        pInfo->suiteId, pInfo->className, g_jAppId);

                    res = java_ams_midlet_start(pInfo->suiteId, g_jAppId,
                        pInfo->className, NULL);

                    wprintf(_T("java_ams_midlet_start res: %d\n"), res);

                    if (res == JAVACALL_OK) {
                        // Update application ID
                        pInfo->appId = g_jAppId;
                        g_jAppId++;

                        // Hide MIDlet tree view window to show
                        // the MIDlet's output in the main window
                        ShowWindow(hWnd, SW_HIDE);
                        UpdateWindow(hWnd);
                     }
                }
            }
        }

        break;
    }

    case WM_COMMAND:
        // Test for the identifier of a command item.
        switch(LOWORD(wParam))
        {
            case IDM_MIDLET_START_STOP:
                break;

            case IDM_MIDLET_INFO:
                wprintf(_T("Show MIDlet info...\n"));
                break;

            case IDM_MIDLET_REMOVE:
                wprintf(_T("Removing MIDlet...\n"));
                break; 

            case IDM_MIDLET_UPDATE:
                wprintf(_T("Updating MIDlet...\n"));
                break; 

            case IDM_MIDLET_SETTINGS:
                wprintf(_T("Show setting for the MIDlet...\n"));
                break;
  
            default:
                break;
        }
        break;

    case WM_ERASEBKGND: {
        DrawBackground((HDC)wParam, SRCCOPY);
        return 1;
    }

    case WM_PAINT:
    {
        //HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        //SelectObject((HDC)wParam, (HGDIOBJ)hBrush);

        CallWindowProc(g_DefTreeWndProc, hWnd, message, wParam, lParam);
        //DrawItem(hItemWnd);

        hdc = BeginPaint(hWnd, &ps);

        //wprintf(_T(">>> left = %ld, top = %ld, right = %ld, bottom = %ld\n"),
        //        ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

        DrawBackground(hdc, SRCAND);

        EndPaint(hWnd, &ps);

        break;
    }


    default:
        return CallWindowProc(g_DefTreeWndProc, hWnd, message, wParam, lParam);
    }

    return 0;
}


// LCD UI stuff

/* This is logical LCDUI putpixel screen buffer. */
typedef struct {
    javacall_pixel* hdc;
    int width;
    int height;
} SBuffer;

static SBuffer VRAM = {NULL, 0, 0};

extern "C" {

/**
 * Initialize LCD API
 * Will be called once the Java platform is launched
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure
 */
javacall_result javacall_lcd_init(void) {
    if (VRAM.hdc == NULL) {
        VRAM.hdc = (javacall_pixel*)
            malloc(g_iWidth * g_iHeight * sizeof(javacall_pixel));
        if (VRAM.hdc == NULL) {
            wprintf(_T("javacall_lcd_init(): VRAM allocation failed!\n"));
        }

        VRAM.width  = g_iWidth;
        VRAM.height = g_iHeight;
    }

    return JAVACALL_OK;
}

/**
 * The function javacall_lcd_finalize is called by during Java VM shutdown,
 * allowing the  * platform to perform device specific lcd-related shutdown
 * operations.
 * The VM guarantees not to call other lcd functions before calling
 * javacall_lcd_init( ) again.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void) {
    if (VRAM.hdc != NULL) {
        VRAM.height = 0;
        VRAM.width = 0;
        free(VRAM.hdc);
        VRAM.hdc = NULL;
    }

    if (g_hMainWindow != NULL) {
        DestroyWindow(g_hMainWindow);
        g_hMainWindow = NULL;
    }

    return JAVACALL_OK;
}

/**
 * Get screen raster pointer
 *
 * @param screenType can be any of the following types:
 * <ul>
 *   <li> <code>JAVACALL_LCD_SCREEN_PRIMARY</code> -
 *        return primary screen size information </li>
 *   <li> <code>JAVACALL_LCD_SCREEN_EXTERNAL</code> -
 *        return external screen size information if supported </li>
 * </ul>
 * @param screenWidth output paramter to hold width of screen
 * @param screenHeight output paramter to hold height of screen
 * @param colorEncoding output paramenter to hold color encoding,
 *        which can take one of the following:
 *              -# JAVACALL_LCD_COLOR_RGB565
 *              -# JAVACALL_LCD_COLOR_ARGB
 *              -# JAVACALL_LCD_COLOR_RGBA
 *              -# JAVACALL_LCD_COLOR_RGB888
 *              -# JAVACALL_LCD_COLOR_OTHER
 *
 * @return pointer to video ram mapped memory region of size
 *         ( screenWidth * screenHeight )
 *         or <code>NULL</code> in case of failure
 */
javacall_pixel* javacall_lcd_get_screen(javacall_lcd_screen_type screenType,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding) {
    if (g_hMainWindow != NULL) {
        if (screenWidth != NULL) {
            *screenWidth = VRAM.width;
        }

        if (screenHeight != NULL) {
            *screenHeight = VRAM.height;
        }

        if (colorEncoding != NULL) {
            *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
        }

        wprintf(_T("VRAM.hdc ok\n"));
        return VRAM.hdc;
    }

    wprintf(_T("NULL !!!\n"));
    return NULL;
}

/**
 * Set or unset full screen mode.
 *
 * This function should return <code>JAVACALL_FAIL</code> if full screen mode
 * is not supported.
 * Subsequent calls to <code>javacall_lcd_get_screen()</code> will return
 * a pointer to the relevant offscreen pixel buffer of the corresponding screen
 * mode as well s the corresponding screen dimensions, after the screen mode has
 * changed.
 *
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.

 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(javacall_bool useFullScreen) {
    return JAVACALL_OK;
}

/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure or invalid screen
 */
javacall_result javacall_lcd_flush() {
    RefreshScreen(0, 0, g_iWidth, g_iHeight); 
    return JAVACALL_OK;
}
/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation: It should be implemented on
 * platforms that support it
 *
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int ystart, int yend) {
    RefreshScreen(0, 0, g_iWidth, g_iHeight); 
    return JAVACALL_OK;
}

/**
 * Changes display orientation
 */
javacall_bool javacall_lcd_reverse_orientation() {
    return JAVACALL_FALSE;
}
 
/**
 * Returns display orientation
 */
javacall_bool javacall_lcd_get_reverse_orientation() {
     return JAVACALL_FALSE;
}

/**
 * checks the implementation supports native softbutton label.
 * 
 * @retval JAVACALL_TRUE   implementation supports native softbutton layer
 * @retval JAVACALL_FALSE  implementation does not support native softbutton layer
 */
javacall_bool javacall_lcd_is_native_softbutton_layer_supported() {
    return JAVACALL_FALSE;
}


/**
 * The following function is used to set the softbutton label in the native
 * soft button layer.
 * 
 * @param label the label for the softbutton
 * @param len the length of the label
 * @param index the corresponding index of the command
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result
javacall_lcd_set_native_softbutton_label(const javacall_utf16* label,
                                         int len,
                                         int index) {
     return JAVACALL_FAIL;
}

/**
 * Returns available display width
 */
int javacall_lcd_get_screen_width() {
    return g_iWidth;
}

/**
 * Returns available display height
 */
int javacall_lcd_get_screen_height() {
    return g_iHeight;
}

}; // extern "C"

/**
 * Utility function to request logical screen to be painted
 * to the physical screen.
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
static void DrawBuffer(HDC hdc) {
    int x;
    int y;
    int width;
    int height;
    int i;
    int j;
    javacall_pixel pixel;
    int r;
    int g;
    int b;
    unsigned char *destBits;
    unsigned char *destPtr;

    HDC        hdcMem;
    HBITMAP    destHBmp;
    BITMAPINFO bi;
    HGDIOBJ    oobj;

    int screenWidth = VRAM.width;
    int screenHeight = VRAM.height;
    javacall_pixel* screenBuffer = VRAM.hdc;

    int x1 = 0;
    int y1 = 0;
    int x2 = screenWidth;
    int y2 = screenHeight;

    // wprintf(_T("x2 = %d, y2 = %d\n"), x2, y2);

    x = x1;
    y = y1;
    width = x2 - x1;
    height = y2 - y1;

    bi.bmiHeader.biSize      = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth     = width;
    bi.bmiHeader.biHeight    = -(height);
    bi.bmiHeader.biPlanes    = 1;
    bi.bmiHeader.biBitCount  = sizeof (long) * 8;
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = width * height * sizeof (long);
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;

    hdcMem = CreateCompatibleDC(hdc);

    destHBmp = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS,
                                (void**)&destBits, NULL, 0);


    if (destBits != NULL) {
        oobj = SelectObject(hdcMem, destHBmp);
        SelectObject(hdcMem, oobj);

        for(j = 0; j < height; j++) {
            for(i = 0; i < width; i++) {
                pixel = screenBuffer[((y + j) * screenWidth) + x + i];
                r = GET_RED_FROM_PIXEL(pixel);
                g = GET_GREEN_FROM_PIXEL(pixel);
                b = GET_BLUE_FROM_PIXEL(pixel);

                destPtr = destBits + ((j * width + i) * sizeof (long));

                *destPtr++ = b;
                *destPtr++ = g;
                *destPtr++ = r;
            }
        }

        SetDIBitsToDevice(hdc, x, y, width, height, 0, 0, 0,
                          height, destBits, &bi, DIB_RGB_COLORS);
    }

    DeleteObject(oobj);
    DeleteObject(destHBmp);
    DeleteDC(hdcMem);
}

static void RefreshScreen(int x1, int y1, int x2, int y2) {
    InvalidateRect(g_hMainWindow, NULL, FALSE);
    UpdateWindow(g_hMainWindow);
}
