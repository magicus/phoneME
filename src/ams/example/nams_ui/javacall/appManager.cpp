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

// MUST be disabled for WinCE
#define USE_CONSOLE

//#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <commctrl.h> // common controls for tree view

#include <time.h>

#include "res/namsui_resource.h"

#include <javacall_memory.h>
#include <javautil_unicode.h>
#include <javacall_lcd.h>
#include <javacall_ams_suitestore.h>
#include <javacall_ams_app_manager.h>
#include <javacall_keypress.h>

#ifdef USE_CONSOLE
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#endif

#define WINDOW_SUBMENU_INDEX 2

#define TB_BUTTON_WIDTH  16
#define TB_BUTTON_HEIGHT 16

#define TREE_VIEW_ICON_WIDTH  16
#define TREE_VIEW_ICON_HEIGHT 16

#define DEF_BACKGROUND_FILE _T("background.bmp")

extern "C" char* _phonenum = "1234567"; // global for javacall MMS subsystem

// The main window class name.
static TCHAR g_szWindowClass[] = _T("NAMS");

// The string that appears in the application's title bar.
static TCHAR g_szTitle[] = _T("NAMS Example");

static TCHAR g_szMidletTreeTitle[] = _T("Java MIDlets");
static TCHAR g_szMidletInfoTitle[] = _T("MIDlet Info");

static TCHAR g_szDefaultFolderName[] = _T("Folder");
static TCHAR g_szDefaultSuiteName[]  = _T("Midlet Suite");

// The size of main window calibrated to get 240x320 child area to draw SJWC output to
const int g_iWidth = 246, g_iHeight = 345;
const int g_iChildAreaWidth = 240, g_iChildAreaHeight = 300;

HINSTANCE g_hInst = NULL;

HWND g_hMainWindow = NULL;
HWND g_hMidletTreeView = NULL;
HWND g_hMidletInfoView = NULL;
HWND g_hWndToolbar = NULL;

HMENU g_hMidletPopupMenu = NULL;
HMENU g_hSuitePopupMenu = NULL;
HMENU g_hFolderPopupMenu = NULL;

WNDPROC g_DefMidletTreeWndProc = NULL;
WNDPROC g_DefMidletInfoWndProc = NULL;

HBITMAP g_hMidletTreeBgBmp = NULL;

// Copied suite, to be pasted into a new folder 
HTREEITEM g_htiCopiedSuite = NULL;

javacall_app_id g_jAppId = 1;

// TODO: place all hPrev* fields in a structure and pass it as
//  a parameter of AddSuiteToTree
static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
static HTREEITEM hPrevLev1Item = NULL; 
static HTREEITEM hPrevLev2Item = NULL; 


// The type of a tree item
const static int TVI_TYPE_UNKNOWN = 0;
const static int TVI_TYPE_SUITE   = 1;
const static int TVI_TYPE_MIDLET  = 2;
const static int TVI_TYPE_FOLDER  = 3;

typedef struct _TVI_INFO {
    int type; // type of the node, valid values are TVI_TYPE_SUITE, 
              // TVI_TYPE_MIDLET, TVI_TYPE_FOLDER
    javacall_utf16_string className; // MIDlet class name if item type is
                                     // TVI_TYPE_MIDLET
    javacall_utf16_string displayName; // Name to display, works for all types
    javacall_suite_id suiteId; // id of the suite, makes sense if item type is
                               // TVI_TYPE_MIDLET and TVI_TYPE_SUITE
    javacall_app_id appId; // external application id if item type is
                           // TVI_TYPE_MIDLET and the MIDlet is running

    javacall_folder_id folderId; // folder ID, applicable for all TVI types
} TVI_INFO;

static TVI_INFO* CreateTviInfo();
static void FreeTviInfo(TVI_INFO* pInfo);
static TVI_INFO* GetTviInfo(HWND hWnd, HTREEITEM hItem);


// Forward declarations of functions included in this code module:

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MidletTreeWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MidletInfoWndProc(HWND, UINT, WPARAM, LPARAM);

static void RefreshScreen(int x1, int y1, int x2, int y2);
static void DrawBuffer(HDC hdc);

static HWND CreateMainView();
static void CenterWindow(HWND hDlg);
static HWND CreateMidletTreeView(HWND hWndParent);
static HWND CreateMidletInfoView(HWND hWndParent);
static HWND CreateMainToolbar(HWND hWndParent);

static BOOL InitMidletTreeViewItems(HWND hwndTV);
static void AddSuiteToTree(HWND hwndTV, javacall_suite_id suiteId, int nLevel);

static HTREEITEM AddTreeItem(HWND hwndTV, LPTSTR lpszItem,
                               int nLevel, TVI_INFO* pInfo);
static void CleanupTreeView(HWND hwndTV, WNDPROC DefWndProc);

static void InitAms();
static void CleanupAms();

static void InitWindows();
static void CleanupWindows();

static void AddWindowMenuItem(javacall_const_utf16_string jsStr, void* pItemData);
static void CheckWindowMenuItem(int index, BOOL fChecked);
static void SetCheckedWindowMenuItem(void* pItemData);
static void* GetWindowMenuItemData(UINT commandId);

static void EnablePopupMenuItem(HMENU hSubMenu, UINT uIDM, BOOL fEnabled);

static int mapKey(WPARAM wParam, LPARAM lParam);

static LPTSTR JavacallUtf16ToTstr(javacall_const_utf16_string str);
static javacall_utf16_string CloneJavacallUtf16(javacall_const_utf16_string str);

/**
 * Entry point of the Javacall executable.
 *
 * @param argc number of arguments (1 means no arguments)
 * @param argv the arguments, argv[0] is the executable's name
 *
 * @return the exit value (1 if OK)
 */
extern "C" javacall_result JavaTaskImpl(int argc, char* argv[]) {
    javacall_result res = java_ams_system_start();

    wprintf(_T("SJWC exited, code: %d."), (int)res); 

    return res;
}

DWORD WINAPI javaThread(LPVOID lpParam) {
    JavaTaskImpl(0, NULL);
    return 0; 
} 

#if 1
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow) {
    (void)lpCmdLine;

#ifdef USE_CONSOLE
    AllocConsole();
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    intptr_t hConHandle = _open_osfhandle((intptr_t)h, _O_TEXT);
    *stdout = *_fdopen(hConHandle, "w");

    h = GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle((intptr_t)h, _O_TEXT);
    *stderr = *_fdopen(hConHandle, "w");
#endif

#else
int main(int argc, char* argv[]) {
    HINSTANCE hInstance = NULL;
    int nCmdShow = SW_SHOWNORMAL;
#endif
   
    srand((unsigned)time(NULL));

    // Store instance handle in our global variable.
    g_hInst = hInstance;

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    InitWindows();

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
    g_hMidletTreeView = CreateMidletTreeView(g_hMainWindow);
    if (g_hMidletTreeView == NULL) {
        return -1;
    }
    InitMidletTreeViewItems(g_hMidletTreeView);

    // Create MIDlet information view
    g_hMidletInfoView = CreateMidletInfoView(g_hMainWindow);

    g_hWndToolbar = CreateMainToolbar(g_hMainWindow);

    // Show the main window 
    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Finalize MIDet tree view
    CleanupTreeView(g_hMidletTreeView, g_DefMidletTreeWndProc);

    // Finalize MIDet information view
    CleanupTreeView(g_hMidletInfoView, g_DefMidletInfoWndProc);

    // Finalize Java AMS
    CleanupAms();

    CleanupWindows();

    return (int) msg.wParam;
}

/**
 *
 */
static HWND CreateMainToolbar(HWND hWndParent) {
    RECT rcClient;  // dimensions of client area 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hWndParent, &rcClient);

    TBBUTTON tbButtons[] = {
        0, IDM_MIDLET_START_STOP, TBSTATE_ENABLED, BTNS_BUTTON, 
#if defined(_WIN32) | defined(_WIN64)
            {0},
#endif
        0L, 0,

        1, IDM_SUITE_INFO, TBSTATE_ENABLED, BTNS_BUTTON,
#if defined(_WIN32) | defined(_WIN64)
            {0},
#endif
        0L, 0,

        2, IDM_SUITE_INSTALL, TBSTATE_ENABLED, BTNS_BUTTON,
#if defined(_WIN32) | defined(_WIN64)
            {0},
#endif
        0L, 0,

        3, IDM_SUITE_REMOVE, TBSTATE_ENABLED, BTNS_BUTTON,
#if defined(_WIN32) | defined(_WIN64)
            {0},
#endif
        0L, 0
    };

    HBITMAP hToolbarBmp = LoadBitmap(g_hInst,
                                     MAKEINTRESOURCE(IDB_MAIN_TOOLBAR_BUTTONS));

    if (hToolbarBmp == NULL) {
        wprintf(_T("Can't load bitmap for the toolbar!"));
        return NULL;
    }

    HWND hWndToolbar = CreateToolbarEx(hWndParent,
        WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS, 
        IDC_MAIN_TOOLBAR, 4, // g_hInst, IDB_MAIN_TOOLBAR_BUTTONS,
        NULL, (UINT)hToolbarBmp,
        tbButtons, 4, TB_BUTTON_WIDTH, TB_BUTTON_HEIGHT,
        TB_BUTTON_WIDTH, TB_BUTTON_HEIGHT,
        sizeof (TBBUTTON));

    if (!hWndToolbar) {
        wprintf(_T("Can't create a toolbar!"));
        return NULL;
    }

    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility. 
    //SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

    //SendMessage(hWndToolbar, TB_SETBITMAPSIZE, 0,
    //            (LPARAM)MAKELONG(TB_BUTTON_WIDTH, TB_BUTTON_WIDTH));

/*
    TBADDBITMAP toolbarImg;

    toolbarImg.hInst = g_hInst;
    toolbarImg.nID = IDB_MAIN_TOOLBAR_BUTTONS;

    BOOL ok = SendMessage(hWndToolbar, TB_ADDBITMAP, 1,
                          (LPARAM)(LPTBADDBITMAP)&toolbarImg);
    if (!ok) {
        wprintf(_T("Can't add a bitmap! Error = %d\n"), GetLastError());
    }

    TBBUTTON pButtons[2];

    pButtons[0].iBitmap = 0;
    pButtons[0].idCommand = IDM_HELP_ABOUT;
    pButtons[0].fsState = TBSTATE_ENABLED;
    pButtons[0].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
    pButtons[0].dwData = 0;
    pButtons[0].iString = -1;

//    ok = SendMessage(hWndToolbar, TB_ADDBUTTONS, 1, (LPARAM)(LPTBBUTTON)pButtons);
    if (!ok) {
        wprintf(_T("Can't add buttons! Error = %d\n"), GetLastError());
    }*/

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

static void InitWindows() {
    // Load backround image, just ignore if loading fails
    /*HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDB_MIDLET_TREE_BG), RT_BITMAP);
    if (!hRes) {
        DWORD res = GetLastError();
        wprintf(_T("ERROR: LoadResource() res: %d\n"), res);
    }
    g_hMidletTreeBgBmp = (HBITMAP)LoadResource(NULL, hRes);*/

//   g_hMidletTreeBgBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_MIDLET_TREE_BG));
    g_hMidletTreeBgBmp = (HBITMAP)LoadImage(g_hInst, DEF_BACKGROUND_FILE,
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

    // Load context menu shown for a suite item in the tree view
    g_hSuitePopupMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(ID_MENU_POPUP_SUITE));
    if (!g_hSuitePopupMenu) {
        MessageBox(NULL,
            _T("Can't load suite popup menu!"),
            g_szTitle,
            NULL);
    }

    // Load context menu shown for a folder item in the tree view
    g_hFolderPopupMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(ID_MENU_POPUP_FOLDER));
    if (!g_hFolderPopupMenu) {
        MessageBox(NULL,
            _T("Can't load folder popup menu!"),
            g_szTitle,
            NULL);
    }
}

static void CleanupWindows() {
    // Clean up resources allocated for MIDlet popup menu 
    DestroyMenu(g_hMidletPopupMenu);

    // Clean up resources allocated for suite popup menu 
    DestroyMenu(g_hSuitePopupMenu);

    // Clean up resources allocated for folder popup menu 
    DestroyMenu(g_hFolderPopupMenu);

    // Unregister main window class
    UnregisterClass(g_szWindowClass, g_hInst);
}

static void CleanupTreeView(HWND hwndTV, WNDPROC DefWndProc) {
    // IMPL_NOTE: memory allocated by the application is freed in MainWndProc
    // by handling WM_NOTIFY message
    TreeView_DeleteAllItems(hwndTV);

    // Return back default window procedure for the tree view
    if (DefWndProc) {
        SetWindowLongPtr(hwndTV, GWLP_WNDPROC, (LONG)DefWndProc);
    }
}

static HWND CreateMidletTreeView(HWND hWndParent) {
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree-view control 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hWndParent, &rcClient); 
    wprintf(_T("main window area w=%d, h=%d\n"), rcClient.right, rcClient.bottom);

    hwndTV = CreateWindowEx(0,
                            WC_TREEVIEW,
                            g_szMidletTreeTitle,
                            WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES |
                                TVS_HASBUTTONS | TVS_LINESATROOT,
                            0, 
                            TB_BUTTON_HEIGHT + 12,
                            rcClient.right,
                            rcClient.bottom - 12,
                            hWndParent, 
                            (HMENU)IDC_TREEVIEW_MIDLETS,
                            g_hInst, 
                            NULL); 

    if (!hwndTV) {
        MessageBox(NULL, _T("Create MIDlet tree view failed!"), g_szTitle,
            NULL);
        return NULL;
    }

    // Store default Tree View WndProc in global variable and set custom
    // WndProc.
    g_DefMidletTreeWndProc = (WNDPROC)SetWindowLongPtr(hwndTV, GWLP_WNDPROC,
        (LONG)MidletTreeWndProc);

    // Create an image list for the Tree View control.
    UINT uResourceIds[] = {
        IDB_DEF_MIDLET_ICON, IDB_DEF_MIDLET_ICON_1, IDB_DEF_MIDLET_ICON_2,
        IDB_DEF_MIDLET_ICON_3, IDB_DEF_SUITE_ICON, IDB_FOLDER_ICON
    };

#define TV_ICONS_NUM ((int) (sizeof(uResourceIds) / sizeof(uResourceIds[0])))
    
    HIMAGELIST hTreeImageList = ImageList_Create(
        TREE_VIEW_ICON_WIDTH, TREE_VIEW_ICON_HEIGHT, ILC_COLOR,
            TV_ICONS_NUM, TV_ICONS_NUM * 3);

    if (hTreeImageList == NULL) {
        wprintf(_T("Can't create an image list for TreeView!"));
        // not fatal, continue
    } else {
        // adding icons into the image list
        for (int i = 0; i < TV_ICONS_NUM; i++) {
            //HBITMAP hImg = LoadBitmap(g_hInst,
            //                          MAKEINTRESOURCE(uResourceIds[i]));
            HICON hImg = LoadIcon(g_hInst,
                                  MAKEINTRESOURCE(uResourceIds[i]));

            if (hImg == NULL) {
                wprintf(_T("Can't load an image # %d!\n"), i);
                // not fatal, continue
            } else {
                //int res = ImageList_Add(hTreeImageList, hImg, NULL);
                int res = ImageList_AddIcon(hTreeImageList, hImg);
                if (res < 0) {
                    wprintf(_T("Failed to add an image # %d ")
                            _T("to the tree view list!\n"), i);
                }
                DeleteObject(hImg);
            }
        }

        TreeView_SetImageList(hwndTV, hTreeImageList, TVSIL_NORMAL);
    }

#undef TV_ICONS_NUM

    return hwndTV;
}

static HWND CreateMidletInfoView(HWND hWndParent) {
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree-view control 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hWndParent, &rcClient); 
    wprintf(_T("main window area w=%d, h=%d\n"), rcClient.right, rcClient.bottom);

    hwndTV = CreateWindowEx(0,
                            WC_TREEVIEW,
                            g_szMidletInfoTitle,
                            WS_CHILD | WS_BORDER,
                            0, 
                            TB_BUTTON_HEIGHT + 12,
                            rcClient.right,
                            rcClient.bottom - 12,
                            hWndParent, 
                            (HMENU)IDC_TREEVIEW_MIDLET_INFO,
                            g_hInst, 
                            NULL); 

    if (!hwndTV) {
        MessageBox(NULL, _T("Create MIDlet info view failed!"), g_szTitle,
            NULL);
        return NULL;
    }

    // Store default Tree View WndProc in global variable and set custom
    // WndProc.
    g_DefMidletInfoWndProc = (WNDPROC)SetWindowLongPtr(hwndTV, GWLP_WNDPROC,
        (LONG)MidletInfoWndProc);

    return hwndTV;
}

static TVI_INFO* CreateTviInfo() {
    TVI_INFO* pInfo = (TVI_INFO*)javacall_malloc(sizeof(TVI_INFO));
    if (pInfo != NULL) {
        pInfo->type = TVI_TYPE_UNKNOWN;
        pInfo->className = NULL;
        pInfo->displayName = NULL;
        pInfo->suiteId = JAVACALL_INVALID_SUITE_ID;
        pInfo->appId = JAVACALL_INVALID_FOLDER_ID;
        pInfo->folderId = JAVACALL_INVALID_APP_ID;
    }
    return pInfo;
}

static void FreeTviInfo(TVI_INFO* pInfo) {
    if (pInfo) {
        if (pInfo->className) {
            javacall_free(pInfo->className);
        }
        if (pInfo->displayName) {
            javacall_free(pInfo->displayName);
        }
        javacall_free(pInfo);
    }
}

static void AddSuiteToTree(HWND hwndTV, javacall_suite_id suiteId, int nLevel) {
    javacall_result res;
    javacall_utf16_string jsLabel;
    TVI_INFO* pInfo;
    javacall_ams_suite_info* pSuiteInfo;
    javacall_ams_midlet_info* pMidletsInfo;
    javacall_int32 midletNum;


    res = java_ams_suite_get_info(suiteId, &pSuiteInfo);
    if (res == JAVACALL_OK) {
            // TODO: add support for disabled suites
            // javacall_bool enabled = suiteInfo[s].isEnabled;

            jsLabel = (pSuiteInfo->displayName != NULL) ?
                pSuiteInfo->displayName : pSuiteInfo->suiteName;

            LPTSTR pszSuiteName = (jsLabel != NULL) ?
                JavacallUtf16ToTstr(jsLabel) :
                g_szDefaultSuiteName;
            wprintf(_T("Suite label=%s\n"), pszSuiteName);

            TCHAR szSuiteLabel[127];
            if (pSuiteInfo->numberOfMidlets > 1) {
                wsprintf(szSuiteLabel, _T("%s (%d)"),
                         pszSuiteName,
                         pSuiteInfo->numberOfMidlets);
            } else {
                wsprintf(szSuiteLabel, _T("%s"), pszSuiteName);
            }

            if (pszSuiteName && (pszSuiteName != g_szDefaultSuiteName)) {
                javacall_free(pszSuiteName);
            }

            pInfo = CreateTviInfo();
            if (pInfo) {
                pInfo->type = TVI_TYPE_SUITE;
                pInfo->suiteId = suiteId;
                if (jsLabel) {
                    pInfo->displayName = CloneJavacallUtf16(jsLabel);
                }
            }
            AddTreeItem(hwndTV, szSuiteLabel, nLevel, pInfo);

            res = java_ams_suite_get_midlets_info(suiteId, &pMidletsInfo,
                &midletNum);
            if (res == JAVACALL_OK) {
                    wprintf(_T("Total MIDlets in the suite %d\n"), midletNum);

                    for (int m = 0; m < midletNum; m++) {
                        // we have nothing to do if class name is not defined
                        if (pMidletsInfo[m].className == NULL) {
                            continue;
                        }

                        jsLabel = (pMidletsInfo[m].displayName != NULL) ?
                            pMidletsInfo[m].displayName :
                            pMidletsInfo[m].className;

       	                LPTSTR pszMIDletName = JavacallUtf16ToTstr(jsLabel);
                        wprintf(_T("MIDlet label='%s', className='%s'\n"),
                            pszMIDletName,
                            (LPWSTR)pMidletsInfo[m].className);

                        pInfo = CreateTviInfo();
                        if (pInfo) {
                            pInfo->type = TVI_TYPE_MIDLET;
                            pInfo->suiteId = suiteId;
                            pInfo->className = CloneJavacallUtf16(
                                pMidletsInfo[m].className);
                            pInfo->displayName = CloneJavacallUtf16(jsLabel);
                        }
                        AddTreeItem(hwndTV, pszMIDletName, nLevel + 1, pInfo);

                        if (pszMIDletName) {
                            javacall_free(pszMIDletName);
                        }
                    }
                    if (midletNum > 0) {
                        java_ams_suite_free_midlets_info(pMidletsInfo, midletNum);
                    }
            } else {
                wprintf(_T("ERROR: java_ams_suite_get_midlets_info() returned: %d\n"), res);
            }

            java_ams_suite_free_info(pSuiteInfo);
    } else {
        wprintf(_T("ERROR: java_ams_suite_get_info(suiteId=%d) returned: %d\n"), (int)suiteId, res);
    }
}
                                                       
static BOOL InitMidletTreeViewItems(HWND hwndTV)  {
    javacall_suite_id* pSuiteIds;
    javacall_suite_id* pFolderSuiteIds;
    int suiteNum, folderNum, folderSuiteNum;
    javacall_result res;
    javacall_ams_folder_info* pFoldersInfo;
    TVI_INFO* pInfo;


    res = java_ams_suite_get_suite_ids(&pSuiteIds, &suiteNum);
    if (res != JAVACALL_OK) {
        wprintf(_T("ERROR: java_ams_suite_get_suite_ids() returned: %d\n"), res);
        return FALSE;
    }
    wprintf(_T("Total suites found: %d\n"), suiteNum);


    /* Add folder and all their content */

    res = java_ams_suite_get_all_folders_info(&pFoldersInfo, &folderNum);
    if (res == JAVACALL_OK) {
        wprintf(_T("Total folders found: %d\n"), folderNum);

        for (int f = 0; f < folderNum; f++) {
            LPTSTR pszFolderName = (pFoldersInfo[f].folderName) ?
                JavacallUtf16ToTstr(pFoldersInfo[f].folderName) :
                g_szDefaultFolderName;
            wprintf(_T("Folder label=%s\n"), pszFolderName);

            pInfo = CreateTviInfo();
            if (pInfo) {
                pInfo->type = TVI_TYPE_FOLDER;
                pInfo->folderId = pFoldersInfo[f].folderId;
                if (pFoldersInfo[f].folderName) {
                    pInfo->displayName = CloneJavacallUtf16(pFoldersInfo[f].folderName);
                }
            } 
            AddTreeItem(hwndTV, pszFolderName, 1, pInfo);

            if (pszFolderName && (pszFolderName != g_szDefaultFolderName)){
                javacall_free(pszFolderName);
            }

            res = java_ams_suite_get_suites_in_folder(pFoldersInfo[f].folderId,
               &pFolderSuiteIds, &folderSuiteNum);
            if (res == JAVACALL_OK) {
                wprintf(_T("Adding suites to the folder...\n"));
                for (int fs = 0; fs < folderSuiteNum; fs++) {
                    AddSuiteToTree(hwndTV, pFolderSuiteIds[fs], 2);

                    // Mark the suite as already added to the tree
                    for (int i = 0; i < suiteNum;  i++) {
                        if (pSuiteIds[i] == pFolderSuiteIds[fs]) {
                            pSuiteIds[i] = JAVACALL_INVALID_SUITE_ID;
                            break;
                        }
                    }
               
                }
                if (folderSuiteNum > 0) {
                    java_ams_suite_free_suite_ids(pFolderSuiteIds, folderSuiteNum);
                }
           }
       }
       if (folderNum > 0) {
           java_ams_suite_free_all_folders_info(pFoldersInfo, folderNum);
       }
    }

    /* Add suites that are not in any folder */

    wprintf(_T("Adding unclassifed suites to the root...\n"));   
    for (int s = 0; s < suiteNum; s++) {
        // Skip the suite if it's already in the tree
        if (pSuiteIds[s] == JAVACALL_INVALID_SUITE_ID) {
            continue;
        }
        AddSuiteToTree(hwndTV, pSuiteIds[s], 1);
    } // end for

    if (suiteNum > 0) {
        java_ams_suite_free_suite_ids(pSuiteIds, suiteNum);
    }

    return TRUE;
}

static LPTSTR JavacallUtf16ToTstr(javacall_const_utf16_string str) {
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

static javacall_utf16_string CloneJavacallUtf16(javacall_const_utf16_string str) {
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

/**
 *
 */
static HTREEITEM AddTreeItem(HWND hwndTV, LPTSTR lpszItem,
                               int nLevel, TVI_INFO* pInfo) {
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hti;

    if (lpszItem == NULL) {
        return NULL;
    }

    tvi.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
               TVIF_IMAGE | TVIF_SELECTEDIMAGE;

    // Set the text of the item. 
    tvi.pszText = lpszItem;
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);


    tvi.state = 0;
    tvi.stateMask = 0;

    // Make folders marked out by bold font style
    if (pInfo && pInfo->type == TVI_TYPE_FOLDER) {
        tvi.state |= TVIS_BOLD;
        tvi.stateMask |= TVIS_BOLD;
    }

    // Assume the item is not a parent item, so give it a 
    // document image.
    tvi.iImage = tvi.iSelectedImage =
        (int) ((rand() / (double)RAND_MAX) * 4); // a midlet's icon by default

    if (pInfo != NULL) {
        if (pInfo->type == TVI_TYPE_SUITE) {
            tvi.iImage = tvi.iSelectedImage = 4;
        } else if (pInfo->type == TVI_TYPE_FOLDER){
            tvi.iImage = tvi.iSelectedImage = 5;
        }
    }

    tvi.lParam = (LPARAM)pInfo;
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 

    // Set the parent item based on the specified level. 
    if (nLevel == 1) {
        tvins.hParent = TVI_ROOT;
    } else if (nLevel == 2) {
        tvins.hParent = hPrevLev1Item;
    } else {
        tvins.hParent = hPrevLev2Item;
    }

    // Add the item to the tree-view control.
    hPrev = TreeView_InsertItem(hwndTV, &tvins);

    // Save the handle to the item. 
    if (nLevel == 1) {
        hPrevLev1Item = hPrev; 
    } else if (nLevel == 2) {
        hPrevLev2Item = hPrev; 
    }

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
        WORD wCmd = LOWORD(wParam);

        switch(wCmd) {
            case IDM_WINDOW_APP_MANAGER: {
                java_ams_midlet_switch_background();

                ShowWindow(g_hMidletTreeView, SW_SHOWNORMAL);

                if (g_hWndToolbar != NULL) {
                    ShowWindow(g_hWndToolbar, SW_SHOWNORMAL);
                }

                // change selected window in the menu
                HMENU hWindowSubmenu = GetSubMenu(GetMenu(g_hMainWindow),
                    WINDOW_SUBMENU_INDEX);
                for (int i = 1; i < GetMenuItemCount(hWindowSubmenu); i++) {
                    CheckWindowMenuItem(i, FALSE);
                }

                CheckWindowMenuItem(0, TRUE);
                DrawMenuBar(g_hMainWindow);

                break;
            }

            case IDM_SUITE_INSTALL:
            case IDM_SUITE_REMOVE:
            case IDM_SUITE_INFO:
            case IDM_MIDLET_INFO:
            case IDM_MIDLET_START_STOP: {
                // Delegate message processing to MIDlet tree view
                if (g_hMidletTreeView) {
                    PostMessage(g_hMidletTreeView, message, wParam, lParam);
                }
                break;
            }

            case IDM_SUITE_EXIT: {
                (void)java_ams_system_stop();

                // TODO: wait for notification from the SJWC thread instead of sleep
                Sleep(1000);

                // Hide the main window 
                ShowWindow(hWnd, SW_HIDE);

                PostQuitMessage(0);
                break;
            }

            case IDM_HELP_ABOUT: {
                TCHAR szBuf[127];
                wsprintf(szBuf, _T("About %s"), g_szTitle);
                MessageBox(hWnd, _T("Cool Application Manager"), szBuf,
                           MB_OK | MB_ICONINFORMATION);
                break;
            }

            default: {
                if (wCmd >= IDM_WINDOW_FIRST_ITEM &&
                        wCmd < IDM_WINDOW_LAST_ITEM) {
                    // This is a command from "Window" menu, so
                    // we are switching to the selected window.
                    TVI_INFO* pInfo = (TVI_INFO*)GetWindowMenuItemData(wCmd);
                    if (pInfo != NULL) {
                        javacall_result res = java_ams_midlet_switch_foreground(
                            pInfo->appId);
                        if (res == JAVACALL_OK) {
                            ShowWindow(g_hMidletTreeView, SW_HIDE);
                            if (g_hWndToolbar != NULL) {
                                ShowWindow(g_hWndToolbar, SW_HIDE);
                            }
                            SetCheckedWindowMenuItem(pInfo);
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    case WM_PAINT: {
        hdc = BeginPaint(hWnd, &ps);

        DrawBuffer(hdc);

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP: {
        int key = mapKey(wParam, lParam);

        if (message == WM_KEYUP) {
            javanotify_key_event((javacall_key)key, JAVACALL_KEYRELEASED);
        } else {
            if (lParam & 0xf0000000) {
                javanotify_key_event((javacall_key)key, JAVACALL_KEYREPEATED);
            } else {
                javanotify_key_event((javacall_key)key, JAVACALL_KEYPRESSED);
            }
        }

        break;
    }

    case WM_NOTIFY: {
        LPNMHDR pHdr = (LPNMHDR)lParam;
        switch (pHdr->code)
        {
            case TVN_DELETEITEM:
                if(pHdr->idFrom == IDC_TREEVIEW_MIDLETS) {
                    TVITEM tvi = ((LPNMTREEVIEW)lParam)->itemOld;

                    wprintf(_T("Cleaning tree items up (struct=%p)...\n"),
                        tvi.lParam);

                    FreeTviInfo((TVI_INFO*)tvi.lParam);
                }
            break;
        }
        break;
    }

    case WM_CLOSE:
    case WM_DESTROY:
          PostMessage(hWnd, WM_COMMAND, (WPARAM)MAKELONG(IDM_SUITE_EXIT, 0), 0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

/**
 *
 */
static void DrawBackground(HDC hdc, DWORD dwRop) {
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


static void EnablePopupMenuItem(HMENU hSubMenu, UINT uIDM, BOOL fEnabled) {
    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fState = (fEnabled) ? MFS_ENABLED : MFS_DISABLED;

    (void)SetMenuItemInfo(hSubMenu, uIDM, FALSE, &mii);
}

/**
 *
 */
static void CheckWindowMenuItem(int index, BOOL fChecked) {
    HMENU hWindowSubmenu = GetSubMenu(GetMenu(g_hMainWindow),
                                      WINDOW_SUBMENU_INDEX);
    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_ENABLED;

    if (fChecked) {
        mii.fState |= MFS_CHECKED;
    }

    (void)SetMenuItemInfo(hWindowSubmenu, index, TRUE, &mii);
}

/**
 *
 */
static void SetCheckedWindowMenuItem(void* pItemData) {
    HMENU hWindowSubmenu = GetSubMenu(GetMenu(g_hMainWindow),
                                      WINDOW_SUBMENU_INDEX);
    int numberOfItems = GetMenuItemCount(hWindowSubmenu);

    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA;

    for (int i = 0; i < numberOfItems; i++) {
        BOOL fRes = GetMenuItemInfo(hWindowSubmenu, i, TRUE, &mii);
        if (fRes && mii.dwItemData == (DWORD)pItemData) {
            CheckWindowMenuItem(i, TRUE);
        } else {
            CheckWindowMenuItem(i, FALSE);
        }
    }

    DrawMenuBar(g_hMainWindow);
}

/**
 *
 */
static void* GetWindowMenuItemData(UINT commandId) {
    HMENU hWindowSubmenu = GetSubMenu(GetMenu(g_hMainWindow),
                                      WINDOW_SUBMENU_INDEX);
    int numberOfItems = GetMenuItemCount(hWindowSubmenu);

    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_DATA;

    for (int i = 0; i < numberOfItems; i++) {
        BOOL fRes = GetMenuItemInfo(hWindowSubmenu, i, TRUE, &mii);
        if (fRes && mii.wID == commandId) {
            return (void*)mii.dwItemData;
        }
    }

    return NULL;
}

/**
 *
 */
static void AddWindowMenuItem(javacall_const_utf16_string jsStr,
                              void* pItemData) {
    LPTSTR pszMIDletName = JavacallUtf16ToTstr(jsStr);
                    
    HMENU hWindowSubmenu = GetSubMenu(GetMenu(g_hMainWindow),
                                      WINDOW_SUBMENU_INDEX);
    int index = GetMenuItemCount(hWindowSubmenu);

    MENUITEMINFO mii;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED | MFS_CHECKED;
    mii.wID = IDM_WINDOW_FIRST_ITEM + index;
    mii.hSubMenu = NULL;
    mii.hbmpChecked = NULL; 
    mii.hbmpUnchecked = NULL;
    mii.dwItemData = (DWORD)pItemData;
    mii.dwTypeData = pszMIDletName;
    mii.cch = lstrlen(mii.dwTypeData);
    mii.hbmpItem = NULL;

    (void)InsertMenuItem(hWindowSubmenu, index, TRUE, &mii);

    for (int i = 0; i < index; i++) {
        CheckWindowMenuItem(i, FALSE);
    }

    DrawMenuBar(g_hMainWindow);

    javacall_free(pszMIDletName);
}

static TVI_INFO* GetTviInfo(HWND hWnd, HTREEITEM hItem) {
    TVITEM tvi;
    tvi.hItem = hItem;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;

    if (TreeView_GetItem(hWnd, &tvi)) {
        return (TVI_INFO*)tvi.lParam;
    }
    return NULL;
}

/**
 * Helper function.
 */
static BOOL StartMidlet(HWND hTreeWnd) {
    javacall_result res;
    HTREEITEM hItem = TreeView_GetSelection(hTreeWnd);
    TVI_INFO* pInfo = GetTviInfo(hTreeWnd, hItem);

    if (pInfo != NULL) {
        if (pInfo->type == TVI_TYPE_MIDLET && 
            // check whether the MIDlet is already running
            pInfo->appId == JAVACALL_INVALID_APP_ID) { 

            wprintf(_T("Launching MIDlet (suiteId=%d, class=%s, appId=%d)...\n"),
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
                ShowWindow(hTreeWnd, SW_HIDE);

                if (g_hWndToolbar != NULL) {
                    ShowWindow(g_hWndToolbar, SW_HIDE);
                }

                // Adding a new item to "Windows" menu
                javacall_const_utf16_string jsLabel =
                    (pInfo->displayName != NULL) ?
                        pInfo->displayName : pInfo->className;
                AddWindowMenuItem(jsLabel, pInfo);

                return TRUE;
            }
        }
    }

    return FALSE;
}

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

    case WM_RBUTTONDOWN: {
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

            TVI_INFO* pInfo = GetTviInfo(hWnd, hItem);
            if (pInfo) {
                HMENU hSubMenu;
                switch(pInfo->type)
                {
                    case TVI_TYPE_SUITE:
                        hSubMenu = GetSubMenu(g_hSuitePopupMenu, 0);
                        break;

                    case TVI_TYPE_MIDLET:
                        hSubMenu = GetSubMenu(g_hMidletPopupMenu, 0);
                        break;

                    case TVI_TYPE_FOLDER: {
                        hSubMenu = GetSubMenu(g_hFolderPopupMenu, 0);

                        // Enable or disable "Paste" popup menu item
                        BOOL fPaste = (g_htiCopiedSuite != NULL) ?
                            TRUE : FALSE;
                        EnablePopupMenuItem(hSubMenu, IDM_FOLDER_PASTE,
                            fPaste);

                        // Enable or disable "Remove All" popup menu item
                        BOOL fClear = (TreeView_GetChild(hWnd, hItem)) ?
                            TRUE : FALSE;
                        EnablePopupMenuItem(hSubMenu, IDM_FOLDER_REMOVE_ALL,
                            fClear);

                        break;
                    }

                    default:
                        hSubMenu = NULL;
                        break;
                }
                if (hSubMenu) {
                    TrackPopupMenu(hSubMenu, 0, tvH.pt.x, tvH.pt.y, 0, hWnd,
                        NULL);
                }
            }
        }

        break;
    }

    case WM_LBUTTONDBLCLK: {
        StartMidlet(hWnd);

        break;
    }

    case WM_COMMAND:
        // Test for the identifier of a command item.
        switch(LOWORD(wParam))
        {
            case IDM_SUITE_INFO:
            case IDM_MIDLET_INFO:
                if (g_hMidletInfoView) {
                    HTREEITEM hItem = TreeView_GetSelection(hWnd);
                    TVI_INFO* pInfo = GetTviInfo(hWnd, hItem);
                    if (pInfo) {
                        ShowWindow(hWnd, SW_HIDE);
                        // Delegate message processing to MIDlet info view
                        PostMessage(g_hMidletInfoView, (UINT)LOWORD(wParam),
                                    (WPARAM)pInfo, 0);
                    }
                }
                break;

            case IDM_MIDLET_START_STOP:
                StartMidlet(hWnd);
                break;

            case IDM_SUITE_CUT:
                g_htiCopiedSuite = TreeView_GetSelection(hWnd);
                break;

            case IDM_FOLDER_PASTE: {
                if (g_htiCopiedSuite) {
                    TVI_INFO* pInfoSuite = GetTviInfo(hWnd, g_htiCopiedSuite);

                    HTREEITEM htiPasted = TreeView_GetSelection(hWnd);
                    TVI_INFO* pInfoFolder = GetTviInfo(hWnd, htiPasted);

                    if (pInfoSuite && 
                            (pInfoSuite->type == TVI_TYPE_SUITE) &&
                            pInfoFolder &&
                            (pInfoFolder->type == TVI_TYPE_FOLDER)) {

                        javacall_suite_id suiteId = pInfoSuite->suiteId;

                        res = java_ams_suite_move_to_folder(
                            suiteId, pInfoFolder->folderId);

                        if (res == JAVACALL_OK) {
                            TreeView_DeleteItem(hWnd, g_htiCopiedSuite);

                            // Set right place to insert the suite and all
                            // it's content (MIDlets belonging to it)
                            hPrev = TVI_LAST; 
                            hPrevLev1Item = htiPasted;
                            hPrevLev2Item = NULL;

                            AddSuiteToTree(hWnd, suiteId, 2);
                       }
                    }

                    g_htiCopiedSuite = NULL;
                }
                break;
            }

            case IDM_SUITE_INSTALL: {
                javacall_result res = java_ams_midlet_start(-1,
                    g_jAppId,
                    L"com.sun.midp.installer.DiscoveryApp",
                    NULL);

                if (res == JAVACALL_OK) {
                    // IMPL_NOTE: the following code must be refactored

                    // Update application ID
                    g_jAppId++;

                    // Hide MIDlet tree view window to show
                    // the MIDlet's output in the main window
                    ShowWindow(g_hMidletTreeView, SW_HIDE);

                    if (g_hWndToolbar != NULL) {
                        ShowWindow(g_hWndToolbar, SW_HIDE);
                    }

                    // Adding a new item to "Windows" menu
                    AddWindowMenuItem(L"Installer", NULL);
                }
                break;
            }

            case IDM_SUITE_REMOVE: {
                HTREEITEM hItem = TreeView_GetSelection(hWnd);
                TVI_INFO* pInfo = GetTviInfo(hWnd, hItem);

                if (pInfo && (pInfo->type == TVI_TYPE_SUITE)) {
                    TCHAR szBuf[127];                 
                    wsprintf(szBuf, _T("Remove '%s' suite?"), pInfo->displayName);

                    int nMBRes = MessageBox(hWnd, szBuf, g_szTitle,
                        MB_ICONQUESTION | MB_OKCANCEL);

                    if (nMBRes == IDOK) {
                        res = java_ams_suite_remove(pInfo->suiteId);
                        if (res == JAVACALL_OK) {
                            TreeView_DeleteItem(hWnd, hItem);
                        }
                    }
                }
                break;
            }

            case IDM_FOLDER_REMOVE_ALL: {
                HTREEITEM hItem = TreeView_GetSelection(hWnd);
                TVI_INFO* pInfo = GetTviInfo(hWnd, hItem);

                if (pInfo && (pInfo->type == TVI_TYPE_FOLDER)) {
                    TCHAR szBuf[127];                 
                    wsprintf(szBuf, _T("Remove all MIDlets from '%s' folder?"),
                        pInfo->displayName);

                    int nMBRes = MessageBox(hWnd, szBuf, g_szTitle,
                        MB_ICONQUESTION | MB_OKCANCEL);

                    if (nMBRes == IDOK) {
                        int suiteNum;
                        javacall_suite_id* pSuiteIds;

                        res = java_ams_suite_get_suites_in_folder(
                            pInfo->folderId, &pSuiteIds, &suiteNum);
                        if (res == JAVACALL_OK) {
                            if (suiteNum > 0) {
                                javacall_suite_id* pDelIds = 
                                    (javacall_suite_id*)javacall_malloc(
                                        sizeof(javacall_suite_id) * suiteNum);
                                int nDelNum = 0;

                                // Delete suites from the DB
                                for (int s = 0; s < suiteNum; s++) {
                                    res = java_ams_suite_remove(pSuiteIds[s]);
                                    if (res == JAVACALL_OK) {
                                        pDelIds[nDelNum++] = pSuiteIds[s];
                                    }
                                }
                                java_ams_suite_free_suite_ids(pSuiteIds,
                                    suiteNum);

                                // Delete the suites from the tree
                                HTREEITEM hChild = TreeView_GetChild(
                                    hWnd, hItem);
                                while (hChild) {
                                    TVI_INFO* pChildInfo = GetTviInfo(
                                        hWnd, hChild);

                                    // Have to go to next sibling right now
                                    // because once an item is deleted, its 
                                    // handle is invalid and cannot be used
                                    HTREEITEM hNextChild = TreeView_GetNextSibling(
                                        hWnd, hChild);
                                                      
                                    for (int r = 0; r < nDelNum; r++) {
                                        if (pChildInfo && 
                                                (pChildInfo->suiteId == 
                                                    pDelIds[r])) {
                                            TreeView_DeleteItem(hWnd, hChild);
                                        }
                                    }

                                    hChild = hNextChild;
                                }

                                javacall_free(pDelIds);
                            }
                        }
                    }
                }

                break;
            }
  
            default:
                break;
        }
        break;

    case WM_ERASEBKGND: {
        hdc = (HDC)wParam;
        DrawBackground(hdc, SRCCOPY);
        return 1;
    }

    case WM_PAINT: {
        RECT r; // = {0, 0, 240, 300};
        GetClientRect(hWnd, &r); 

        InvalidateRect(hWnd, &r, FALSE);
 
        CallWindowProc(g_DefMidletTreeWndProc, hWnd, message, wParam, lParam);

        //HDC hdc1 = (HDC)wParam;
        //HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        //SelectObject(hdc1, (HGDIOBJ)hBrush);
        //FillRect(hdc1, &r, hBrush);

        //hdc = BeginPaint(hWnd, &ps);
        hdc = GetDC(hWnd);
        DrawBackground(hdc, SRCAND);
        ReleaseDC(hWnd, hdc);
        //EndPaint(hWnd, &ps);

        break;
    }

    default:
        return CallWindowProc(g_DefMidletTreeWndProc, hWnd, message, wParam,
            lParam);
    }

    return 0;
}

/**
 *  Processes messages for the MIDlet information window.
 *
 */
LRESULT CALLBACK
MidletInfoWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
//    PAINTSTRUCT ps;
    HDC hdc;
    javacall_result res;

    switch (message) {

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: {
        wprintf(_T("Hiding info window...\n"));

        ShowWindow(hWnd, SW_HIDE);
        TreeView_DeleteAllItems(hWnd);

        // Show back MIDlet tree view
        ShowWindow(g_hMidletTreeView, SW_SHOWNORMAL);

        break;
    }

    case IDM_SUITE_INFO:
    case IDM_MIDLET_INFO: {
        TVI_INFO* pInfo = (TVI_INFO*)wParam;
        if (pInfo) {
            switch (pInfo->type) {
            case TVI_TYPE_SUITE: {
                javacall_ams_suite_info* pSuiteInfo;
                res = java_ams_suite_get_info(pInfo->suiteId, &pSuiteInfo);

                wprintf(
                    _T("java_ams_suite_get_info(suiteId=%d) returned: %d\n"),
                    (int)pInfo->suiteId, (int)res);

                if (res == JAVACALL_OK) {
                    TCHAR szBuf[256];

                    // Set position info to default
                    hPrev = (HTREEITEM)TVI_FIRST; 
                    hPrevLev1Item = NULL; 
                    hPrevLev2Item = NULL; 

                    wsprintf(szBuf, _T("Suite: %s"), pInfo->displayName);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

                    if (pSuiteInfo->suiteVendor) {
                        wsprintf(szBuf, _T("Vendor: %s"),
                            pSuiteInfo->suiteVendor);
                        AddTreeItem(hWnd, szBuf, 1, NULL);
                    }

                    if (pSuiteInfo->suiteVersion) {
                        wsprintf(szBuf, _T("Version: %s"),
                            pSuiteInfo->suiteVersion);
                        AddTreeItem(hWnd, szBuf, 1, NULL);
                    }

//                    javacall_int32 folderId;

                    LPTSTR pszPreinstalled =
                        (pSuiteInfo->isPreinstalled == JAVACALL_TRUE) ?
                        _T("Yes") : _T("No");
                    wsprintf(szBuf, _T("Preinstalled: %s"), pszPreinstalled);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

                    LPTSTR pszTrusted =
                        (pSuiteInfo->isTrusted == JAVACALL_TRUE) ?
                        _T("Yes") : _T("No");
                    wsprintf(szBuf, _T("Trusted: %s"), pszTrusted);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

                    LPTSTR pszEnabled =
                        (pSuiteInfo->isEnabled == JAVACALL_TRUE) ?
                        _T("Yes") : _T("No");
                    wsprintf(szBuf, _T("Enabled: %s"), pszEnabled);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

                    wsprintf(szBuf, _T("Number of MIDlets: %d"),
                        (int)pSuiteInfo->numberOfMidlets);
                    AddTreeItem(hWnd, szBuf, 1, NULL);


                    wsprintf(szBuf, _T("JAD size: %d"),
                        (int)pSuiteInfo->jadSize);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

                    wsprintf(szBuf, _T("JAR size: %d"),
                        (int)pSuiteInfo->jarSize);
                    AddTreeItem(hWnd, szBuf, 1, NULL);

//                    long installTime;

                    java_ams_suite_free_info(pSuiteInfo);
                }

                break;
            }

            default: {
                AddTreeItem(hWnd, _T("Not implemented yet!"), 1, NULL);

                break;
            }

            }  // end case

            ShowWindow(hWnd, SW_SHOWNORMAL);
        } // end if (pInfo)
        break;
    }

    case WM_ERASEBKGND: {
        hdc = (HDC)wParam;
        DrawBackground(hdc, SRCCOPY);
        return 1;
    }

    case WM_PAINT: {
        RECT r; // = {0, 0, 240, 300};
        GetClientRect(hWnd, &r); 

        InvalidateRect(hWnd, &r, FALSE);

        CallWindowProc(g_DefMidletInfoWndProc, hWnd, message, wParam, lParam);

        hdc = GetDC(hWnd);
        DrawBackground(hdc, SRCAND);
        ReleaseDC(hWnd, hdc);

        break;
    }

    default:
        return CallWindowProc(g_DefMidletInfoWndProc, hWnd, message, wParam,
            lParam);
    }

    return 0;
}

/**
 *
 */
static void CenterWindow(HWND hDlg) {
    HWND hwndOwner;
    RECT rc, rcDlg, rcOwner;

    // Get the owner window and dialog box rectangles.
    if ((hwndOwner = GetParent(hDlg)) == NULL)  {
        hwndOwner = GetDesktopWindow();
    }

    GetWindowRect(hwndOwner, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);
    CopyRect(&rc, &rcOwner);

    // Offset the owner and dialog box rectangles so that
    // right and bottom values represent the width and
    // height, and then offset the owner again to discard
    // space taken up by the dialog box.
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    // The new position is the sum of half the remaining
    // space and the owner's original position.
    SetWindowPos(hDlg,
                 HWND_TOP,
                 rcOwner.left + (rc.right / 2),
                 rcOwner.top + (rc.bottom / 2),
                 0, 0,          // ignores size arguments
                 SWP_NOSIZE);
    ShowWindow(hDlg, SW_SHOWNORMAL);
}

/**
 *
 */
static int mapKey(WPARAM wParam, LPARAM lParam) {
    BYTE keyStates[256];
    WORD temp[2];

    switch(wParam) {
    case VK_F1:
        return JAVACALL_KEY_SOFT1;

    case VK_F2:
        return JAVACALL_KEY_SOFT2;

    case VK_F9:
        return JAVACALL_KEY_GAMEA;

    case VK_F10:
        return JAVACALL_KEY_GAMEB;

    case VK_F11:
        return JAVACALL_KEY_GAMEC;

    case VK_F12:
        return JAVACALL_KEY_GAMED;
        break;

    case VK_UP:
        return JAVACALL_KEY_UP;

    case VK_DOWN:
        return JAVACALL_KEY_DOWN;

    case VK_LEFT:
        return JAVACALL_KEY_LEFT;

    case VK_RIGHT:
        return JAVACALL_KEY_RIGHT;

    case VK_RETURN:
        return JAVACALL_KEY_SELECT;

    case VK_BACK:
        return JAVACALL_KEY_BACKSPACE;

    case VK_HOME:
    case VK_F7:
//        return MD_KEY_HOME;

    default:
        break;
    }

    GetKeyboardState(keyStates);
    temp[0] = 0;
    temp[1] = 0;
    ToAscii((UINT)wParam, (UINT)lParam, keyStates, temp, (UINT)0);

    /* At this point only return printable characters. */
    if (temp[0] >= ' ' && temp[0] < 127) {
        return temp[0];
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
        VRAM.hdc = (javacall_pixel*) malloc(g_iChildAreaWidth *
            g_iChildAreaHeight * sizeof(javacall_pixel));
        if (VRAM.hdc == NULL) {
            wprintf(_T("javacall_lcd_init(): VRAM allocation failed!\n"));
        }

        VRAM.width  = g_iChildAreaWidth;
        VRAM.height = g_iChildAreaHeight;
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
    RefreshScreen(0, 0, g_iChildAreaWidth, g_iChildAreaHeight); 
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
    RefreshScreen(0, 0, g_iChildAreaWidth, g_iChildAreaHeight); 
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
    return g_iChildAreaWidth;
}

/**
 * Returns available display height
 */
int javacall_lcd_get_screen_height() {
    return g_iChildAreaHeight;
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
