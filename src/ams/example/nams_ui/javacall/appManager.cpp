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


extern "C" char* _phonenum = "1234567"; // global for javacall MMS subsystem


// The main window class name.
static TCHAR g_szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR g_szTitle[] = _T("NAMS Example");

// The size of main window calibrated to get 240x320 child area to draw SJWC output to
int g_iWidth = 246, g_iHeight = 345;

static HMENU IDC_TREEVIEW_MIDLETS = (HMENU) 1;

// The type of a tree item
static WORD TVI_TYPE_NONE   = 0;
static WORD TVI_TYPE_SUITE  = 1;
static WORD TVI_TYPE_MIDLET = 2;
static WORD TVI_TYPE_FOLDER = 3;

HINSTANCE g_hInst = NULL;
HWND g_hMainWindow = NULL;
HMENU g_hMidletPopupMenu = NULL;
WNDPROC g_DefTreeWndProc = NULL;
HBITMAP g_hMidletTreeBgBmp = NULL;

javacall_app_id g_jAppId = 1;

// Forward declarations of functions included in this code module:

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MidletTreeWndProc(HWND, UINT, WPARAM, LPARAM);

static void RefreshScreen(int x1, int y1, int x2, int y2);
static void DrawBuffer(HDC hdc);

HWND CreateMainView();
HWND CreateTreeView(HWND hwndParent);
BOOL InitTreeViewItems(HWND hwndTV);
HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, LPARAM lParam);

void InitJavacallAms();
void CleanupJavacallAms();

LPTSTR JavacallUtf16ToTstr(javacall_utf16_string str);


extern "C" {

javacall_result java_ams_system_start();
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

    // Store instance handle in our global variable
    g_hInst = hInstance;

    HWND hWnd = CreateMainView();
    g_hMainWindow = hWnd;

    // Start JVM in a separate thread
    DWORD dwThreadId; 
    HANDLE hThread; 
    hThread = CreateThread( 
        NULL,                    // default security attributes 
        0,                       // use default stack size  
        javaThread,              // thread function 
        NULL,                    // argument to thread function
        0,                       // use default creation flags 
        &dwThreadId);            // returns the thread identifier

    if (!hThread) {
        MessageBox(hWnd,
            _T("Can't start Java Thread!"),
            g_szTitle,
            NULL);

        return 1;
    }

    // Let native peer to start
    // TODO: wait for notification from the peer instead of sleep
    Sleep(1000);
    
    // Initialize Java AMS
    InitJavacallAms();

    // Create and init Java MIDlets tree view
    HWND hwndTV = CreateTreeView(hWnd);
    InitTreeViewItems(hwndTV);

    // Show the main window 
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up resources allocated for MIDlet popup menu 
    DestroyMenu(g_hMidletPopupMenu);

    // Finalize Java AMS
    CleanupJavacallAms();

    return (int) msg.wParam;
}

HWND CreateMainView() {
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
    wcex.lpszMenuName   = NULL;
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

void InitJavacallAms() {
    javacall_result res = java_ams_suite_storage_init();
    if (res == JAVACALL_FAIL) {
        wprintf(_T("Init of suite storage fail!\n"));
    }
}

void CleanupJavacallAms() {
    javacall_result res = java_ams_suite_storage_cleanup();
    if (res == JAVACALL_FAIL) {
        wprintf(_T("Cleanup of suite storage fail!\n"));
    }
}

HWND CreateTreeView(HWND hwndParent) {
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree-view control 

    // Ensure that the common control DLL is loaded. 
    InitCommonControls(); 

    // Get the dimensions of the parent window's client area, and create 
    // the tree-view control. 
    GetClientRect(hwndParent, &rcClient); 
    wprintf(_T("main window area w=%d, h=%d\n"), rcClient.right, rcClient.bottom);

    hwndTV = CreateWindowEx(0,
                            WC_TREEVIEW,
                            TEXT("Java Midlets"),
                            WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES |
                                TVS_HASBUTTONS
                                /* | WS_CAPTION*/,
                            0, 
                            0, 
                            rcClient.right,
                            rcClient.bottom,
                            hwndParent, 
                            IDC_TREEVIEW_MIDLETS,
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
   g_hMidletTreeBgBmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_MIDLET_TREE_BG));
//   g_hMidletTreeBgBmp = (HBITMAP)LoadImage(g_hInst, _T("bgd-yellow.bmp"), IMAGE_BITMAP, 240, 320, 0);
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


    // Add all folders to the tree view
   

    // Iterrate over all suites and add them to the tree view
    
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
              LPARAM lInfo;
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

              lInfo = MAKELPARAM(TVI_TYPE_SUITE, (WORD) pSuiteIds[i]);
              AddItemToTree(hwndTV, pszSuiteName, 1, lInfo);

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
                          wprintf(_T("MIDlet label=%s\n"), pszMIDletName);

                          lInfo = MAKELPARAM(TVI_TYPE_MIDLET, (WORD) j);
                          AddItemToTree(hwndTV, pszMIDletName, 2, lInfo);
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

LPTSTR JavacallUtf16ToTstr(javacall_utf16_string str) {
    LPTSTR result = NULL;
#ifdef UNICODE 
    javacall_int32 len;
    javacall_result res = javautil_unicode_utf16_ulength(str, &len);
    if (res == JAVACALL_OK) {
        const buf_len = (len + 1) * sizeof(WCHAR);
        result = (LPTSTR)javacall_malloc(buf_len);
        memcpy(result, str, buf_len);
    }
#else
# error "Only Unicode platforms are supported for now"
#endif
   return result;
}


HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel, LPARAM lParam) {
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

    tvi.lParam = lParam; 
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
    hPrev = (HTREEITEM)SendMessage(hwndTV, 
                                   TVM_INSERTITEM, 
                                   0,
                                   (LPARAM)(LPTVINSERTSTRUCT)&tvins); 

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
//    TCHAR greeting[] = _T("Native Application Manager");

    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        DrawBuffer(hdc);
/*
        TextOut(hdc,
            5, 5,
            greeting, _tcslen(greeting));
*/

        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}



/**
 *  Processes messages for the MIDlet tree window.
 *
 */
LRESULT CALLBACK
MidletTreeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    POINT pnt;

    switch (message) {

    case WM_RBUTTONDOWN:
    {
        TV_HITTESTINFO tvH;

        tvH.pt.x = LOWORD(lParam);
        tvH.pt.y = HIWORD(lParam);

//        wprintf (_T("Click position (%d, %d)\n"), tvH.pt.x, tvH.pt.y);

        HTREEITEM item = TreeView_HitTest(hWnd, &tvH);
        if ((item != NULL) && ((tvH.flags & TVHT_ONITEM) != 0))
        {
//            wprintf (_T("Hit flags hex=%x\n"), tvH.flags);

            ClientToScreen(hWnd, (LPPOINT) &tvH.pt);
            // Get the first shortcut menu in the menu template.
            // This is the menu that TrackPopupMenu displays
            HMENU hMenu = GetSubMenu(g_hMidletPopupMenu, 0);
            if (hMenu) {
                TrackPopupMenu(hMenu, 0, tvH.pt.x, tvH.pt.y, 0, hWnd, NULL);
            } else {
                MessageBox(NULL, _T("Can't show context menu!"), g_szTitle,
                    MB_OK);
            }
        }

        break;
    }

    case WM_COMMAND:
        // Test for the identifier of a command item.
        switch(LOWORD(wParam))
        {
            case IDM_MIDLET_LAUNCH:
            {
                HTREEITEM hItem   = TreeView_GetSelection(hWnd);
                HTREEITEM hParent = TreeView_GetParent(hWnd, hItem);

                if (hItem != NULL && hParent != NULL) {
                   TVITEM tvi;
                   TVITEM tvp;

                   if (TreeView_GetItem(hWnd, &tvi) &&
                           TreeView_GetItem(hWnd, &tvp)) {

                       WORD wItemType   = LOWORD(tvi.lParam);
                       WORD wParentType = LOWORD(tvp.lParam);

                       if (wItemType == TVI_TYPE_MIDLET &&
                               wParentType == TVI_TYPE_SUITE) {

                           wprintf(_T("Launching MIDlet...\n"));

                           WORD wMidletId = HIWORD(tvi.lParam);
                           WORD wSuiteId  = HIWORD(tvp.lParam);

                           javacall_result res;
                           javacall_ams_midlet_info* pMidletsInfo;
                           javacall_int32 midletNum;

                           res = java_ams_suite_get_midlets_info(wSuiteId,
                               &pMidletsInfo, &midletNum);
                           if (res == JAVACALL_OK && midletNum > 0) {
                               if (wMidletId >= 0 && wMidletId < midletNum) {

/*
                                   res = java_ams_midlet_start(-1, 1,
                                       L"com.sun.midp.installer.DiscoveryApp",
                                       NULL);
*/
                                   res = java_ams_midlet_start(wSuiteId,
                                       g_jAppId++,
                                       pMidletsInfo[wMidletId].className,
                                       NULL);

                                   wprintf(
                                       _T("java_ams_midlet_start res: %d\n"),
                                       res);

                                   if (res == JAVACALL_OK) {
                                       // Hide MIDlet tree view window to show
                                       // the MIDlet's output in the main window
                                       ShowWindow(hWnd, SW_HIDE);
                                       UpdateWindow(hWnd);
                                   }
                               }
                               java_ams_suite_free_midlets_info(pMidletsInfo,
                                   midletNum);
                           }
                       }
                   }
                }
                break;
            }

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

    case WM_PAINT:
    {
        CallWindowProc(g_DefTreeWndProc, hWnd, message, wParam, lParam);

        hdc = BeginPaint(hWnd, &ps);

        HDC hdcMem = CreateCompatibleDC(hdc);  
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_hMidletTreeBgBmp);  

        BITMAP bm;   
        GetObject(g_hMidletTreeBgBmp, sizeof(bm), &bm);  
 
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);  
 
        SelectObject(hdcMem, hbmOld);  
        DeleteDC(hdcMem); 


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

    wprintf(_T("x2 = %d, y2 = %d\n"), x2, y2);

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

    destHBmp = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, (void**)&destBits, NULL, 0);


    if (destBits != NULL) {
        wprintf(_T("OK!!!\n"));
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
