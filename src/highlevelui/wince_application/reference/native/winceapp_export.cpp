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

/*
 * This file implements the launching of the stand-along CDC-HI VM
 * on the WinCE platform.
 */

#include <windows.h>
#include <windowsx.h>
#include <aygshell.h>
#include <commctrl.h>
#include <sipapi.h>
#include <tpcshell.h>


#if ENABLE_WINCE_DIRECT_DRAW
#include <ddraw.h>
#endif

#ifdef ENABLE_JSR_184
#include <swvapi.h>
#endif

#include "resources.h"


/**
 * The functions exported by gx.h use C++ linkage, hence this file
 * must be C++.
 */
#define GXDLL_EXPORTS
#include <gx.h>

extern "C" {

#include <kni.h>
#include <midpError.h>
#include <midp_logging.h>
#include <winceapp_export.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <midp_constants_data.h>
#include <keymap_input.h>
#include <commonKNIMacros.h>
#include <midpEventUtil.h>
#include <midp_foreground_id.h>
#include <midp_mastermode_port.h>
#include <anc_indicators.h>


/* global variables defined in midp_msgQueue_md.c */
extern int inMidpEventLoop;
extern int lastWmSettingChangeTick;

/* global variables shared by all wince code that deal with native widgets */
HWND hwndMenuBar       = NULL; /* The currently displayed menu bar. */
HWND hwndMenuBarSimple = NULL; /* The menu bar with only 2 soft buttons. */
HWND hwndMenuBarPopup  = NULL; /* The menu bar with a popup menu. */
HMENU hmenuPopup       = NULL; /* the menu used to display the MIDP commands */
HMENU hmenuMain        = NULL; /* Handle to the main menu */

gxj_screen_buffer gxj_system_screen_buffer;

static HWND hwndMain = NULL;
static HWND hwndTextActive = NULL;
static HWND hwndTextField = NULL;
static HWND hwndTextBox = NULL;
static WNDPROC oldTextFieldProc;
static WNDPROC oldTextBoxProc;
static int editBoxShown = 0;
static int editCHX, editCHY, editCHW, editCHH; /* in CHAM coordinates */
static int inited = 0;
static int titleHeight = JWC_WINCE_TITLE_HEIGHT;
static int menuHeight = JWC_WINCE_MENU_HEIGHT;
static RECT rcVisibleDesktop;
static int virtualKeyboardHeight = 0; /* 0 = hide */
static HANDLE eventThread;
static HINSTANCE instanceMain;
static jboolean reverse_orientation;



#if ENABLE_WINCE_DIRECT_DRAW
LPDIRECTDRAW                g_pDD = NULL;
LPDIRECTDRAWSURFACE         g_pDDSPrimary = NULL;
LPDIRECTDRAWSURFACE g_pDDSvram = NULL;
LPDIRECTDRAWSURFACE g_pDDSvramDirect = NULL; //vram surface for direct flush
LPDIRECTDRAWCLIPPER g_pDDClipper = NULL;
#else
static GXDisplayProperties gxDispProps;
#endif

/* IMPL_NOTE: need a better way for quitting.  */
extern int     _quit_now; /* defined in Scheduler.cpp */
extern int midpPaintAllowed;

int hint_is_painting = 0;
int hint_is_canvas_painting = 0;

extern int wince_init_fonts(); /* DirectDraw only.  */

int has_skipped_refresh = 0;
int must_refresh = 0;
DWORD lastPaintedTick = 0;
DWORD lastUserInputTick = 0;

static int dirty_x1, dirty_y1, dirty_x2, dirty_y2;

static void process_skipped_refresh();
static LRESULT process_key(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

static gxj_pixel_type* startDirectPaint(int &dstWidth, int &dstHeight,
                                   int &dstYPitch);
static void endDirectPaint();
static void updateEditorForRotation();

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

static LRESULT CALLBACK
myTextProc(HWND hwnd, WNDPROC oldproc, UINT msg, WPARAM wp, LPARAM lp,
           int isMultiLine) {
    int c = KEYMAP_KEY_INVALID;
    lastUserInputTick = GetTickCount();

    switch (msg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
        switch (wp) {
        case VK_UP:     c = KEYMAP_KEY_UP;    break;
        case VK_DOWN:   c = KEYMAP_KEY_DOWN;  break;
        case VK_LEFT:   c = KEYMAP_KEY_LEFT;  break;
        case VK_RIGHT:  c = KEYMAP_KEY_RIGHT; break;
        default:
            break;
        }
    }

    if (c == KEYMAP_KEY_LEFT) {
        /* Don't send the key to MIDP unless we are at the first character  */
        WORD w = (WORD)SendMessage(hwnd, EM_GETSEL, 0, 0L);
        int caret = LOWORD(w);
        if (caret != 0) {
            c = KEYMAP_KEY_INVALID;
        }
    } else if (c == KEYMAP_KEY_RIGHT) {
        /* Don't send the key to MIDP unless we are at the last character  */
        WORD w = (WORD)SendMessage(hwnd, EM_GETSEL, 0, 0L);
        int strLen = GetWindowTextLength(hwnd);
        int caret = LOWORD(w);
        if (caret < strLen) {
            c = KEYMAP_KEY_INVALID;
        }
    }

    if (isMultiLine) {
        switch (c) {
        case KEYMAP_KEY_UP:
        case KEYMAP_KEY_DOWN:
        case KEYMAP_KEY_LEFT:
        case KEYMAP_KEY_RIGHT:
            /**
             * TODO: currenrly MIDP doesn't support Forms with multi-line
             * widgets, so we don't need to do anything special.
             *
             * In the future, if we need to traverse out from a multi-line
             * text box, we need to see if, e.g.: caret is at top line in the
             * editor and user presses UP.
             */
            c = KEYMAP_KEY_INVALID;
        }
    }

    if (c != KEYMAP_KEY_INVALID) {
        pMidpEventResult->type = MIDP_KEY_EVENT;
        pMidpEventResult->CHR = c;
        pMidpEventResult->ACTION = (msg == WM_KEYDOWN) ? 
            KEYMAP_STATE_PRESSED:KEYMAP_STATE_RELEASED;
        pSignalResult->waitingFor = UI_SIGNAL;
        pMidpEventResult->DISPLAY = gForegroundDisplayId;
        sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));

        return 0;
    } else {
        return CallWindowProc(oldproc, hwnd, msg, wp, lp);
    }
}

static LRESULT CALLBACK
myTextFieldProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return myTextProc(hwnd, oldTextFieldProc, msg, wp, lp, 0);
}

static LRESULT CALLBACK
myTextBoxProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return myTextProc(hwnd, oldTextBoxProc, msg, wp, lp, 1);
}

static void createEditors() {
    /* Create the Text controls for text input. They are not visible yet. */
    DWORD dwStyle = WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL;
    hwndTextField = CreateWindow(TEXT("edit"),  TEXT(""), dwStyle,
                                 0, 0, 40, 40, hwndMain, 0, GetModuleHandle(NULL), NULL);
    SendMessage(hwndTextField, EM_SETLIMITTEXT, 2048, 0);
    oldTextFieldProc = (WNDPROC)GetWindowLong(hwndTextField, GWL_WNDPROC);
    SetWindowLong(hwndTextField, GWL_WNDPROC, (LONG)&myTextFieldProc);
    dwStyle = WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE;
    hwndTextBox = CreateWindow(TEXT("edit"), TEXT(""), dwStyle,
                               0, 40, 100, 100, hwndMain, 0,
                               GetModuleHandle(NULL), NULL);
    SendMessage(hwndTextBox, EM_SETLIMITTEXT, 1024 * 10, 0);
    oldTextBoxProc = (WNDPROC)GetWindowLong(hwndTextBox, GWL_WNDPROC);
    SetWindowLong(hwndTextBox, GWL_WNDPROC, (LONG)&myTextBoxProc);
}

static void updateVisibleDesktop() {
    SIPINFO sipinfo;
    memset(&sipinfo, 0, sizeof(sipinfo));
    sipinfo.cbSize = sizeof(SIPINFO);
    SHSipInfo(SPI_GETSIPINFO, 0, &sipinfo, 0);
    rcVisibleDesktop = sipinfo.rcVisibleDesktop;
}

#if ENABLE_WINCE_DIRECT_DRAW
static void init_DirectDraw() {
    /**
     * Note: if DirectDraw fails to initialize, we will use GDI to
     *  draw to do the screenBuffer->LCD copying.
     */
    HRESULT hRet;
    hRet = DirectDrawCreate(NULL, &g_pDD, NULL);
    if (hRet != DD_OK) {
        return;
    }

    hRet = g_pDD->SetCooperativeLevel(hwndMain, DDSCL_NORMAL);
    if (hRet != DD_OK) {
        g_pDD->Release();
        g_pDD = NULL;
        return;
    }

    //wince_init_fonts();
}


static void release_DirectDraw() {
    if (NULL != g_pDD) {
    	if (NULL != g_pDDSPrimary) {
		g_pDDSPrimary->SetClipper(NULL);
    		g_pDDSPrimary->Release();
    		g_pDDSPrimary = NULL;
    	}

    	if (NULL != g_pDDSvram) {
    	    g_pDDSvram->Release();
    	    g_pDDSvram = NULL;
    	}

    	if (NULL != g_pDDSvramDirect) {
    	    g_pDDSvramDirect->Release();
    	    g_pDDSvramDirect = NULL;
	}

	if (g_pDDClipper) {
           g_pDDClipper->Release();
           g_pDDClipper = NULL;
       }

    	g_pDD->Release();
    	g_pDD = NULL;
    }
    return;
}

/**
 * Create primary surface
 */
static void create_primary_surface(LPDIRECTDRAWSURFACE* pDDSurface) {
    ASSERT(g_pDD);
    ASSERT(pDDSurface);

    HRESULT hRet;
    DDSURFACEDESC ddsd;

    // Create the primary surface with 0 back buffer
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE ;
    hRet = g_pDD->CreateSurface(&ddsd, pDDSurface, NULL);
    
    if (hRet != DD_OK) {
        *pDDSurface = NULL;
    }
}

/**
 * Attach clipper to the surface
 */
static void attach_clipper(LPDIRECTDRAWSURFACE pDDSurface, 
    LPDIRECTDRAWCLIPPER* ppDDClipper) {
    
    ASSERT(g_pDD);
    ASSERT(pDDSurface);
    ASSERT(ppDDClipper);

    LPDIRECTDRAWCLIPPER pDDClipper = NULL;
    HRESULT hRet;
    
    do {
        hRet = g_pDD->CreateClipper(0, &pDDClipper, NULL);
        if (hRet != DD_OK) {
            break;
        }
        hRet = pDDClipper->SetHWnd(0, hwndMain);
        if (hRet != DD_OK) {
            break;
        }
        hRet = pDDSurface->SetClipper(pDDClipper);
    } while(0);

    if (hRet != DD_OK) {
        if (pDDClipper) {
            pDDClipper->Release();
            pDDClipper = NULL;
        }
    }

    *ppDDClipper = pDDClipper;
}

/**
 * Attach video memory to memory based DD surface
 */
static void attach_vmem_to_memory_surface(void* pVmem, int width, int height, 
    LPDIRECTDRAWSURFACE* ppDDSurface) {

    ASSERT(g_pDD);
    ASSERT(pVmem);
    ASSERT(ppDDSurface);

    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE pDDS = NULL;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ZeroMemory(&ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LPSURFACE
        | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.lPitch = (LONG)sizeof(gxj_pixel_type) * width;
    ddsd.lpSurface = pVmem;

    // Set up the pixel format for 16-bit RGB (5-6-5).
    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd.ddpfPixelFormat.dwRBitMask    = 0x1f << 11;
    ddsd.ddpfPixelFormat.dwGBitMask    = 0x3f << 5;
    ddsd.ddpfPixelFormat.dwBBitMask    = 0x1f;

    HRESULT hRet = g_pDD->CreateSurface(&ddsd, &pDDS, NULL);
    if (hRet != DD_OK) {
        *ppDDSurface = NULL;
    } else {
        *ppDDSurface = pDDS;
    }
}
#endif /* ENABLE_WINCE_DIRECT_DRAW */

/**
 * Initializes the WINCE native resources.
 */

static PTCHAR    _szAppName    = TEXT("SunJWCWindow");
static PTCHAR    _szTitle      = TEXT("SunJWCWindow");
static HINSTANCE _hInstance;

static BOOL InitApplication(HINSTANCE hInstance) {
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc = (WNDPROC)winceapp_wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _szAppName;

    return RegisterClass(&wc);
}

static BOOL InitInstance(HINSTANCE hInstance, int CmdShow) {
    HWND _hwndMain = CreateWindowEx(WS_EX_CAPTIONOKBTN,
                               _szAppName,
                               _szTitle,
                               WS_VISIBLE,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               NULL, NULL, hInstance, NULL);

    if (!_hwndMain) {
        return FALSE;
    }

    winceapp_set_window_handle(_hwndMain);
    ShowWindow(_hwndMain, CmdShow);
    UpdateWindow(_hwndMain);
    return TRUE;
}

static BOOL init_windows(HINSTANCE hInstance, int nShowCmd) {
    if (!InitApplication(hInstance)) {
        return FALSE;
    }
    if (!InitInstance(hInstance, nShowCmd)) {
        return FALSE;
    }
    return TRUE;
}

DWORD WINAPI CreateWinCEWindow(LPVOID lpParam) {
    static MidpReentryData newSignal;
    static MidpEvent newMidpEvent;

    int screenSize = sizeof(gxj_pixel_type) * CHAM_WIDTH * CHAM_HEIGHT;

/* IMPL_NOTE:  Need a better way to load the library */
#ifdef CVM_PRELOAD_LIB
    instanceMain = LoadLibrary(TEXT("cvmi.dll"));
#else
#ifdef CVM_DEBUG
    instanceMain = LoadLibrary(TEXT("libmidp_g.dll"));
#else
    instanceMain = LoadLibrary(TEXT("libmidp.dll"));
#endif
#endif

    gxj_system_screen_buffer.width = CHAM_WIDTH;
    gxj_system_screen_buffer.height = CHAM_HEIGHT;
    gxj_system_screen_buffer.alphaData = 0;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    memset(gxj_system_screen_buffer.pixelData, 0xff, screenSize);

    if (!init_windows(GetModuleHandle(NULL), SW_SHOW)) {
        REPORT_ERROR(LC_AMS, "init_gui() failed");
        MessageBox(NULL, TEXT("Failed to start JWC"), TEXT("Bye"), MB_OK);
    }

    updateVisibleDesktop();

#if ENABLE_WINCE_DIRECT_DRAW
    init_DirectDraw();
#else
    if (GXOpenDisplay(hwndMain, 0) == 0) {
        REPORT_ERROR(LC_HIGHUI, "GXOpenDisplay() failed");
    }
    gxDispProps = GXGetDisplayProperties();
#endif

    createEditors();

#ifdef ENABLE_JSR_184
    engine_initialize();
#endif

    MIDP_EVENT_INITIALIZE(newMidpEvent);
    while (1) {
        checkForSystemSignal(&newSignal, &newMidpEvent, 200);
    }
}

void winceapp_init() {
    eventThread = CreateThread(NULL, 0, CreateWinCEWindow, 0, 0, NULL);
}

static jint mapKey(WPARAM wParam, LPARAM lParam) {
    switch (wParam) {
    case VK_F9:  return KEYMAP_KEY_GAMEA; /* In PPC emulator only  */
    case VK_F10: return KEYMAP_KEY_GAMEB; /* In PPC emulator only */
    case VK_F11: return KEYMAP_KEY_GAMEC; /* In PPC emulator only */
    case VK_F12: return KEYMAP_KEY_GAMED; /* In PPC emulator only */

    case VK_UP:    return KEYMAP_KEY_UP;
    case VK_DOWN:  return KEYMAP_KEY_DOWN;
    case VK_LEFT:  return KEYMAP_KEY_LEFT;
    case VK_RIGHT: return KEYMAP_KEY_RIGHT;

    case VK_SPACE:
    case VK_RETURN:
        return KEYMAP_KEY_SELECT;

    case VK_BACK:
        return KEYMAP_KEY_BACKSPACE;

    }

    if (wParam >= ' ' && wParam <= 127) {
        /* Some ASCII keys sent by emulator or mini keyboard */
        return (jint)wParam;
    }

    return KEYMAP_KEY_INVALID;
}

static void disablePaint() {
    if (inMidpEventLoop) {
        midpPaintAllowed = 0;
    }
}

static void enablePaint() {
    if (inMidpEventLoop) {
        midpPaintAllowed = 1;
        has_skipped_refresh = 1;
        dirty_x1 = 0;
        dirty_y1 = 0;
        dirty_x2 = CHAM_WIDTH;
        dirty_y2 = CHAM_HEIGHT;
        process_skipped_refresh();
    }
}

static DWORD mainThreadID;
/**
 * Handles window messages sent to the main window.
 */
LRESULT CALLBACK winceapp_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    PAINTSTRUCT ps;
    HDC hdc;
    int cmd;
    DWORD err;
    static int ignoreCancelMode = 0;

    switch (msg) {
    case WM_CREATE:
        {
            SHMENUBARINFO mbi;

            /* Create the menu bar with two simple soft buttons */
            ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
            mbi.cbSize = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hwnd;
            mbi.nToolBarId = ID_MENU_SIMPLE;
            mbi.hInstRes = instanceMain;

            if (SHCreateMenuBar(&mbi) == FALSE) {
                /* Couldn't create the menu bar.  Fail creation of the window. */
                return -1;
            } else {
                hwndMenuBarSimple = mbi.hwndMB;

                /* In order to make the Back button work properly,
                 * it's necessary to override it and then call the
                 * appropriate SH API
                 */
                SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
                            MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                                       SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
            }

            /* Create the menu bar with a popup menu. This is used if
             * the current MIDP screen has more than 2 Commands.
             */
            ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
            mbi.cbSize = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hwnd;
            mbi.nToolBarId = ID_MENU_POPUP;
            mbi.hInstRes = instanceMain;

            if (SHCreateMenuBar(&mbi) == FALSE) {
                /* Couldn't create the menu bar.  Fail creation of the window. */
                return -1;
            } else {

                /* Get a handle to the popup menu, which will host the
                 * Commands of the current MIDP screen.
                 */
                TBBUTTONINFO tbbi = {0};
                tbbi.cbSize = sizeof(tbbi);
                tbbi.dwMask = TBIF_LPARAM;
                hwndMenuBarPopup = mbi.hwndMB;

                if (!SendMessage(hwndMenuBarPopup, TB_GETBUTTONINFO, IDM_SOFTBTN_1, (LPARAM)&tbbi)) {
                  err = GetLastError();
                }

                hmenuPopup = (HMENU)tbbi.lParam;

                /* In order to make the Back button work properly,
                 * it's necessary to override it and then call the
                 * appropriate SH API
                 */
                SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
                            MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                                       SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
            }

            hwndMenuBar = hwndMenuBarSimple;
            ShowWindow(hwndMenuBarSimple, SW_SHOW);
            ShowWindow(hwndMenuBarPopup, SW_HIDE);
        }
        return 0;

    case WM_HOTKEY:
        /* If back key is overriden, back button messages are sent in
         * a WM_HOTKEY to the menu bar with the id VK_TBACK in the
         * LPARAM.
         */
        if (HIWORD(lp) == VK_TBACK && (LOWORD(lp) & MOD_KEYUP)) {
            if (editBoxShown) {
                SendMessage(hwndTextActive, WM_CHAR, VK_BACK, 0);
            } else {
#if ENABLE_MULTIPLE_ISOLATES
                if (gForegroundIsolateId == midpGetAmsIsolateId()) {
                    SHNavigateBack();
                } else {
                    pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
                    pSignalResult->waitingFor = AMS_SIGNAL;
                }
#else
                SHNavigateBack();
#endif
            }
        }
        return 1;

    case WM_SETTINGCHANGE: {
        updateVisibleDesktop();
        updateEditorForRotation();
        lastWmSettingChangeTick = GetTickCount();
        /* Handle Virtual Keyboard change */
        RECT virtualKeyboardRect = {0, 0};
        HWND hWndInputPanel = FindWindow(TEXT("SipWndClass"), NULL);
        if (hWndInputPanel != NULL) {
            if (IsWindowVisible(hWndInputPanel)) {
                GetWindowRect(hWndInputPanel, &virtualKeyboardRect);
            }
        }
        virtualKeyboardHeight = virtualKeyboardRect.bottom - virtualKeyboardRect.top;
    }
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_TIMER:
        return 0;

    case WM_COMMAND:
        switch ((cmd = GET_WM_COMMAND_ID(wp, lp))) {
        case IDOK:
#if ENABLE_MULTIPLE_ISOLATES
            /* On PocketPC devices that don't have a lot of hardware
             * buttons, this is a good way for implementing
             * the 'task switch' event - click on the OK button on
             * the window title
             */
            if (gForegroundIsolateId == midpGetAmsIsolateId()) {
                SetForegroundWindow(GetDesktopWindow());
                /* SHNavigateBack(); */
            } else {
                pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
                pSignalResult->waitingFor = AMS_SIGNAL;
            }
#else
            /* IMPL_NOTE: we ask the user if the current Midlet should be
             * terminated.
             */
            SendMessage(hwnd, WM_CLOSE, 0, 0);
#endif
            /* for some reason windows has already sent us a CANCELMODE message
             * before we come to here. Let's re-enable painting.
             */
            if (!midpPaintAllowed) {
                enablePaint();
            }
            return 0;
        case IDM_SOFTBTN_0:
        case IDM_SOFTBTN_1:
            pMidpEventResult->type = MIDP_KEY_EVENT;
            pMidpEventResult->CHR = (cmd==IDM_SOFTBTN_0) ? 
                KEYMAP_KEY_SOFT1:KEYMAP_KEY_SOFT2;
            pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
            pSignalResult->waitingFor = UI_SIGNAL;             
            pMidpEventResult->DISPLAY = gForegroundDisplayId;
            sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));

            /* for some reason windows will send us a CANCELMODE message
             * afterwards
             */
            ignoreCancelMode = 1;
            return 0;
        default:
            if (cmd >= ID_DYNAMIC_MENU) {
                 pMidpEventResult->type = MIDP_COMMAND_EVENT;
                 pMidpEventResult->COMMAND = (cmd - ID_DYNAMIC_MENU);
                 pSignalResult->waitingFor = UI_SIGNAL;
                 pMidpEventResult->DISPLAY = gForegroundDisplayId;
                 sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
                 if (hwndMenuBar != hwndMenuBarSimple) {
                     ShowWindow(hwndMenuBar, SW_HIDE);
                    hwndMenuBar = hwndMenuBarSimple;
                    ShowWindow(hwndMenuBar, SW_SHOW);
                 }
                /* for some reason windows will send us a CANCELMODE message
                 * afterwards
                 */
                ignoreCancelMode = 1;
                return 0;
            } else {
                return DefWindowProc(hwnd, msg, wp, lp);
            }
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wp)) { /* active */
            enablePaint();
            if (editBoxShown) {
                SetFocus(hwndTextActive);
            }
        } else { /* inactive */
#if ENABLE_MULTIPLE_ISOLATES
            pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
            pSignalResult->waitingFor = AMS_SIGNAL;
#endif
            disablePaint();
        }
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_EXITMENULOOP:
        enablePaint();
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_CANCELMODE:
        if (!ignoreCancelMode) {
            disablePaint();
        } else {
            ignoreCancelMode = 0;
        }
        /* We have to do this, or else windows is unhappy. */
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        enablePaint();
        if (editBoxShown) {
            SetFocus(hwndTextActive);
        }
        return 0;

    case WM_CLOSE:
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_DESTROY:
        PostQuitMessage(0);
        exit(0);
        return 0;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        {
            lastUserInputTick = GetTickCount();

            POINT pen_position ={LOWORD(lp), HIWORD(lp)};
            if (msg == WM_MOUSEMOVE) {
                pMidpEventResult->ACTION = KEYMAP_STATE_DRAGGED;
            } else if (msg == WM_LBUTTONUP) {
                pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
            } else {
                pMidpEventResult->ACTION = KEYMAP_STATE_PRESSED;
            }
            pMidpEventResult->type = MIDP_PEN_EVENT;
            pMidpEventResult->X_POS = pen_position.x;
            pMidpEventResult->Y_POS = pen_position.y;

            pSignalResult->waitingFor = UI_SIGNAL;
            pMidpEventResult->DISPLAY = gForegroundDisplayId;
            sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
        }
        return 0;

    case WM_KEYDOWN: /* fall through */
    case WM_KEYUP:
        return process_key(hwnd, msg, wp, lp);
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static LRESULT process_key(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    int key;
    lastUserInputTick = GetTickCount();

    switch (key = mapKey(wp, lp)) {
    case KEYMAP_KEY_INVALID:
        break;
    default:
        pMidpEventResult->type = MIDP_KEY_EVENT;
        pMidpEventResult->CHR = key;

        if (msg == WM_KEYUP) {
            pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
        } else if (lp & 0x40000000) {
            pMidpEventResult->ACTION = KEYMAP_STATE_REPEATED;
        } else {
            pMidpEventResult->ACTION = KEYMAP_STATE_PRESSED;
        }
        pSignalResult->waitingFor = UI_SIGNAL;
        pMidpEventResult->DISPLAY = gForegroundDisplayId;
        sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
    }

    return 0;
}


/**
 * Finalize the WINCE native resources.
 */
void winceapp_finalize() {
    GXCloseDisplay();

#ifdef ENABLE_JSR_184
    engine_uninitialize();
#endif

    CloseHandle(eventThread);
    midpFree(gxj_system_screen_buffer.pixelData);
}

static void process_skipped_refresh() {
    winceapp_refresh(0, 0, CHAM_WIDTH, CHAM_HEIGHT);         
}

int isScreenFullyVisible() {
    if (JWC_WINCE_SMARTPHONE) {
        /* No SIP window or screen rotation on SmartPhone (presumably ...) */
        return 1;
    } else {
        /* This is false if the screen has been rotated or SIP is up */
        int w = rcVisibleDesktop.right - rcVisibleDesktop.left;
        int h = rcVisibleDesktop.bottom - rcVisibleDesktop.top;
        return (w >= CHAM_WIDTH && h >= CHAM_HEIGHT);
    }
}

#if ENABLE_WINCE_DIRECT_DRAW
int isScreenRotated() {
    if (JWC_WINCE_SMARTPHONE) {
        /* No SIP window or screen rotation on SmartPhone (presumably ...) */
        return 0;
    } else {
        DEVMODE devMode;
        devMode.dmSize = sizeof(devMode);
        devMode.dmFields = DM_DISPLAYORIENTATION;
        devMode.dmDisplayOrientation = DMDO_0;
        ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_TEST, NULL);

        return (devMode.dmDisplayOrientation != DMDO_0);
    }
}
#endif

static gxj_pixel_type*
startDirectPaint(int &dstWidth, int &dstHeight, int &dstYPitch) {
    gxj_pixel_type *dst = NULL;

#if ENABLE_WINCE_DIRECT_DRAW
    if (isScreenRotated() || !isScreenFullyVisible() || editBoxShown) {
        /* DDraw is not very reliable on an rotated screen. Use GDI instead. */
        return NULL;
    }

    if (g_pDD == NULL) {
        init_DirectDraw();
        if (g_pDD == NULL) {
        /* DirectDraw failed to initialize. Let's use GDI to Blit to the LCD. */
        return NULL;
    }
    }

    if (g_pDDSPrimary == NULL) {
        create_primary_surface(&g_pDDSPrimary);
        if (g_pDDSPrimary == NULL) {
            return NULL;
        }
        attach_clipper(g_pDDSPrimary, &g_pDDClipper);
    }

    if (g_pDDSvram == NULL) {
        attach_vmem_to_memory_surface(gxj_system_screen_buffer.pixelData, CHAM_WIDTH, CHAM_HEIGHT, &g_pDDSvram);
        if (g_pDDSvram == NULL) {            
            return NULL;
        }
    }

    DDSURFACEDESC surfaceDesc;
    HRESULT hRet = S_OK;
    memset(&surfaceDesc, 0 , sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    hRet = g_pDDSPrimary->GetSurfaceDesc(&surfaceDesc);
    dst = (gxj_pixel_type*)surfaceDesc.lpSurface;
    if (hRet == DD_OK) {
        dstWidth = surfaceDesc.dwWidth;
        dstHeight = surfaceDesc.dwHeight;
        dstYPitch = surfaceDesc.lPitch;
    } else {
        // Release the DD resources. Maybe we'd get lucky and can allocate
        // it next time.
        if (hRet == DDERR_SURFACELOST) { //restore it before release it
            HRESULT hr2 = g_pDDSPrimary->Restore();
	    }
        g_pDDSPrimary->SetClipper(NULL);
        if (g_pDDClipper) {
	        g_pDDClipper->Release();
	        g_pDDClipper = NULL;
        }
        g_pDDSPrimary->Release();
        g_pDDSPrimary = NULL;
        if (g_pDDSvram) {
	        g_pDDSvram->Release();
	        g_pDDSvram = NULL;
        }
        if (g_pDDSvramDirect) {
	        g_pDDSvramDirect->Release();
	        g_pDDSvramDirect = NULL;
        }
        return NULL;
    }
#else
    if (editBoxShown) {
        return NULL;
    }
    dstWidth = gxDispProps.cxWidth;
    dstHeight = gxDispProps.cyHeight;
    dstYPitch = gxDispProps.cbyPitch;
    dst = (gxj_pixel_type*)GXBeginDraw();
#endif

    return dst;
}

static void endDirectPaint() {
#if ENABLE_WINCE_DIRECT_DRAW
    g_pDDSPrimary->Unlock(NULL);
#else
    GXEndDraw();
#endif
}

static struct {
    HDC         hdcMem;
    HBITMAP     destHBmp;
    BITMAPINFO  bi;
    HGDIOBJ     oobj;
    unsigned char *destBits;
} gb = {NULL};

static void gdiRefreshBitmap(HDC hdc, int x, int y, int width, int height,
                             int dx, int dy) {
    if (gb.hdcMem == NULL) {
        gb.bi.bmiHeader.biSize          = sizeof(gb.bi.bmiHeader);
        gb.bi.bmiHeader.biWidth         = CHAM_WIDTH;
        gb.bi.bmiHeader.biHeight        = -CHAM_HEIGHT;
        gb.bi.bmiHeader.biPlanes        = 1;
        gb.bi.bmiHeader.biBitCount      = sizeof (long) * 8;
        gb.bi.bmiHeader.biCompression   = BI_RGB;
        gb.bi.bmiHeader.biSizeImage     = CHAM_WIDTH * CHAM_HEIGHT * sizeof (long);
        gb.bi.bmiHeader.biXPelsPerMeter = 0;
        gb.bi.bmiHeader.biYPelsPerMeter = 0;
        gb.bi.bmiHeader.biClrUsed       = 0;
        gb.bi.bmiHeader.biClrImportant  = 0;

        gb.hdcMem = CreateCompatibleDC(hdc);
        
        if (gb.hdcMem != NULL) {
        gb.destHBmp = CreateDIBSection(gb.hdcMem, &gb.bi, DIB_RGB_COLORS,
                                       (void**)(&gb.destBits), NULL, 0);

        if (gb.destBits != NULL) {
                memset(gb.destBits, 0xFF, gb.bi.bmiHeader.biSizeImage); //fill it with white pixels            
            gb.oobj = SelectObject(gb.hdcMem, gb.destHBmp);
            SelectObject(gb.hdcMem, gb.oobj);
        }
	}
    }

        x = 0;
        y = 0;
        width = CHAM_WIDTH;
        height = CHAM_HEIGHT;

    gxj_pixel_type* pixels = gxj_system_screen_buffer.pixelData;
    int i, j;

    for (j = 0; j < height; j++) {
        pixels = &gxj_system_screen_buffer.pixelData[(y + j) * CHAM_WIDTH + x];
        unsigned char *destPtr =
            gb.destBits + (((y + j) * CHAM_WIDTH + x) * sizeof(long));

        for (i = 0; i < width; i++) {
            gxj_pixel_type pixel = *pixels++;
            int r = GXJ_GET_RED_FROM_PIXEL(pixel);
            int g = GXJ_GET_GREEN_FROM_PIXEL(pixel);
            int b = GXJ_GET_BLUE_FROM_PIXEL(pixel);

            *destPtr++ = b; /* dest pixels is to be in BGRA order */
            *destPtr++ = g;
            *destPtr++ = r;
             destPtr++;
        }
    }

    // x is always 0 and width is always VRAM.width 
    SetDIBitsToDevice(hdc, dx, dy + y, CHAM_WIDTH, height, 
                     x, CHAM_HEIGHT - y - height, 0, CHAM_HEIGHT, gb.destBits, &gb.bi, DIB_RGB_COLORS);
}

/**
 * Bridge function to request a repaint
 * of the area specified.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void winceapp_refresh(int x1, int y1, int x2, int y2) {
    if (!midpPaintAllowed) {
        return;
    }

    if(x2 > CHAM_WIDTH) {
        x2 = CHAM_WIDTH;
    }
    
    if(y2 > CHAM_HEIGHT) {
        y2 = CHAM_HEIGHT;
    }

    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    int srcWidth, srcHeight;
    int dstWidth, dstHeight, dstYPitch;

    /* Make sure the copied lines are 4-byte aligned for faster memcpy */
    if ((x1 & 0x01) == 1) {
        x1 -= 1;
    }
    if ((x2 & 0x01) == 1) {
        x2 += 1;
    }

    gxj_pixel_type *dst = startDirectPaint(dstWidth, dstHeight, dstYPitch);
    int maxY = dstHeight - titleHeight - menuHeight - virtualKeyboardHeight;
    
#if ENABLE_WINCE_DIRECT_DRAW
    if (y2 > maxY) {
        y2 = maxY;
    }
    if (x2 > dstWidth) {
        x2 = dstWidth;
    }

    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    if (srcWidth <= 0 || srcHeight <= 0)  {
        endDirectPaint();
        // it is possible srcHeight<0 when SIP shown and java refresh 
        // the lower part screen(system menu up/dn,etc.)
        return;
    }

    RECT srcRect, dstRect;
    srcRect.top = y1;
    srcRect.left = x1;
    srcRect.bottom = y2;
    srcRect.right = x2;

    dstRect.top = y1 + rcVisibleDesktop.top;
    dstRect.left = x1;
    dstRect.bottom = y2 + rcVisibleDesktop.top;
    dstRect.right = x2;

    HRESULT ret = g_pDDSPrimary->Blt(&dstRect, g_pDDSvram, &srcRect, 0, NULL); 

#else /* !ENABLE_WINCE_DIRECT_DRAW */

    if (dst != NULL) {
        srcWidth = x2 - x1;
        srcHeight = y2 - y1;
        
        if (y2 > maxY) {
            y2 = maxY;
        }
        if (x2 > dstWidth) {
            x2 = dstWidth;
        }

        dst = (gxj_pixel_type*)( ((int)dst) + titleHeight * dstYPitch);

        src += y1 * CHAM_WIDTH + x1;
        dst = (gxj_pixel_type*)( ((int)dst) + dstYPitch * y1 );
        dst += x1;

        if (x1 == 0 && srcWidth == CHAM_WIDTH && dstWidth == srcWidth &&
            dstYPitch == dstWidth * 2) {
            memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type) * (srcHeight - virtualKeyboardHeight /* assuming srcHeight=full height ?*/));
        } else {
            for (; y1 < y2; y1++) {
                memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
                src += CHAM_WIDTH;
                dst = (gxj_pixel_type*)( ((int)dst) + dstYPitch );
            }
        }
    } 
#endif /* ENABLE_WINCE_DIRECT_DRAW */
        endDirectPaint();
}

jboolean lcd_direct_flush(gxj_pixel_type *src, int height, int width) {
    if (!midpPaintAllowed) {
        return FALSE;
    }


    int dstWidth, dstHeight, dstYPitch;
    jboolean success = KNI_FALSE;

    gxj_pixel_type *dst = startDirectPaint(dstWidth, dstHeight, dstYPitch);
#if ENABLE_WINCE_DIRECT_DRAW /* ENABLE_WINCE_DIRECT_DRAW */    
    static gxj_pixel_type * lastSrc = NULL; //last flush src
    static DWORD lastTime = 0; //last flush time
    static int lastHeight =0; //unflushed accumulated height
    DWORD nowTime;

    if (!midpPaintAllowed) {
        return KNI_TRUE;
    }

    if (isScreenRotated()) return KNI_FALSE; //rotated screen doesn't support directDraw

    nowTime = GetTickCount();
    if (lastSrc == src) {
        if (nowTime-lastTime > 40) { //25 frames/s
            if (lastHeight > height) {
		height = lastHeight;
	    }
        } else {
            if (lastHeight < height) { 
		lastHeight =height;
	    }
            return KNI_TRUE;
        }
    } else {
        if (g_pDDSvramDirect) {
	    g_pDDSvramDirect->Release();
	    g_pDDSvramDirect = NULL;
        }
	init_DirectDraw();
	attach_vmem_to_memory_surface(src, CHAM_WIDTH, CHAM_HEIGHT, &g_pDDSvramDirect);
        lastSrc = src;
    }
    lastHeight = 0;
    lastTime = nowTime;

    if (g_pDDSvramDirect == NULL) {
	attach_vmem_to_memory_surface(src, CHAM_WIDTH, CHAM_HEIGHT, &g_pDDSvramDirect);
    }
    if (g_pDDSvramDirect == NULL) {
	return KNI_FALSE;
    }

    if (height > CHAM_HEIGHT) {
        height = CHAM_HEIGHT;
    }

    do {
        if (KNI_FALSE == startDirectPaint(dstWidth, dstHeight, dstYPitch)) {
            break;
        }

        if (dstWidth == CHAM_WIDTH 
            && height <= dstHeight 
            && width == CHAM_WIDTH 
            && dstYPitch == (int)(dstWidth * sizeof(gxj_pixel_type))) {

            RECT srcRect;
            srcRect.top = 0;
            srcRect.left = 0;
            srcRect.bottom = height;
            srcRect.right = width;

            RECT dstRect;
            dstRect.top = rcVisibleDesktop.top;
            dstRect.left = 0;
            dstRect.bottom = height + rcVisibleDesktop.top;
            dstRect.right = width;
            
            HRESULT ret = g_pDDSPrimary->Blt(&dstRect, g_pDDSvramDirect, &srcRect, 0, NULL);
            if (ret == DD_OK)  success = KNI_TRUE;
        }
    } while(0);
#else  /* !ENABLE_WINCE_DIRECT_DRAW */
    if (dst != NULL) {
        if (dstWidth == CHAM_WIDTH && height <= dstHeight &&
            dstYPitch == (int)(dstWidth * sizeof(gxj_pixel_type))) {
            int bytes = dstYPitch * (height - virtualKeyboardHeight);
            dst = (gxj_pixel_type*)( ((int)dst) + titleHeight * dstYPitch);
            memcpy(dst, src, bytes);
            success = KNI_TRUE;
        } else {
            /* Take the slow path of drawing first into gxj_system_screen_buffer
             * and then copied to LCD using winceapp_refresh();
             */
        }
    }        
#endif /* ENABLE_WINCE_DIRECT_DRAW */    
    endDirectPaint();    
    return success;
}

char * strdup(const char *s) {
    char *result = (char*)malloc(strlen(s)+1);
    if (result) {
        strcpy(result, s);
    }
    return result;
}

void winceapp_set_window_handle(HWND hwnd) {
    hwndMain = hwnd;
}

HWND winceapp_get_window_handle() {
    return hwndMain;
}

/* To be compatible with PCSL win32 socket library. */
HWND win32app_get_window_handle() {
    return winceapp_get_window_handle();
}

/*
 * Draw BackLight.
 * bIf 'active' is KNI_TRUE, the BackLight is drawn.
 * If 'active' is KNI_FALSE, the BackLight is erased.
 */
jboolean drawBackLight(AncBacklightState mode) {
    (void)mode;
    return KNI_FALSE;
}

/**
 * Use DirectDraw to return an HDC object for invoking GDI functions
 * on the pixel buffer (could be a screen back buffer or a
 * MIDP off-screen image). The horizontal pitch is assumed to
 * be the same as the (width * sizeof(gxj_pixel_type).
 */
HDC getScreenBufferHDC(gxj_pixel_type *buffer, int width, int height) {
#if ENABLE_WINCE_DIRECT_DRAW
    /*  pDDS and cachedHDC must both be NULL or both be non-NULL */
    static LPDIRECTDRAWSURFACE pDDS = NULL;
    static HDC cachedHDC = NULL;
    static gxj_pixel_type *cachedBuffer;

    DDSURFACEDESC ddsd;
    HRESULT hRet;

    if (buffer == cachedBuffer && cachedHDC != NULL && !pDDS->IsLost()) {
        /* Note: after screen rotation has happened, the pDDS surface may
         * be lost, even if it's using a client-defined pixel buffer!
         */
        return cachedHDC;
    }

    if (pDDS != NULL && (buffer != cachedBuffer || pDDS->IsLost())) {
        pDDS->ReleaseDC(cachedHDC);
        pDDS->Release();
        pDDS = NULL;
        cachedHDC = NULL;
    }

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LPSURFACE |
                   DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = width;
    ddsd.dwHeight= height;
    ddsd.lPitch  = (LONG)sizeof(gxj_pixel_type) * width;
    ddsd.lpSurface = buffer;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;

    /* Set up the pixel format for 16-bit RGB (5-6-5). */
    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags= DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd.ddpfPixelFormat.dwRBitMask    = 0x1f << 11;
    ddsd.ddpfPixelFormat.dwGBitMask    = 0x3f << 5;
    ddsd.ddpfPixelFormat.dwBBitMask    = 0x1f;

    /* Create the surface */
    hRet = g_pDD->CreateSurface(&ddsd, &pDDS, NULL);
    if (hRet != DD_OK) {
        pDDS = NULL;
        cachedHDC = NULL;
        return NULL;
    }

    hRet = pDDS->GetDC(&cachedHDC);
    if (hRet != DD_OK) {
        pDDS->Release();
        pDDS = NULL;
        cachedHDC = NULL;
        return NULL;
    }

    cachedBuffer = buffer;
    return cachedHDC;
#else    
    return NULL;
#endif /* ENABLE_WINCE_DIRECT_DRAW */
    
}

/**
 * Returns the file descriptor for reading the keyboard.
 */
int fbapp_get_keyboard_fd() {
  return 0;
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_enableNativeEditor) {
    int x = KNI_GetParameterAsInt(1);
    int y = KNI_GetParameterAsInt(2);
    int w = KNI_GetParameterAsInt(3);
    int h = KNI_GetParameterAsInt(4);
    jboolean multiline = KNI_GetParameterAsBoolean(5);

    if (multiline) {
        hwndTextActive = hwndTextBox;
    } else {
        hwndTextActive = hwndTextField;
    }

    editCHX = x;
    editCHY = y;
    editCHW = w;
    editCHH = h;

    if (!JWC_WINCE_SMARTPHONE) {
        int diff = (rcVisibleDesktop.right - rcVisibleDesktop.left)
                   - CHAM_WIDTH;
        if (diff > 0) {
            x += diff / 2;
        }
    }

    editBoxShown = 1;
    SetWindowPos(hwndTextActive, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
    SetFocus(hwndTextActive);
    KNI_ReturnVoid();
}

static void updateEditorForRotation() {
    int x = editCHX;
    int y = editCHY;
    int w = editCHW;
    int h = editCHH;

    if (!JWC_WINCE_SMARTPHONE) {
        int diff = (rcVisibleDesktop.right - rcVisibleDesktop.left)
                   - CHAM_WIDTH;
        if (diff > 0) {
            x += diff / 2;
        }

        if (editBoxShown) {
            SetWindowPos(hwndTextActive, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
        }
    }
}

typedef struct Java_java_lang_String _JavaString;
#define getJavaStringPtr(handle) (unhand(_JavaString,(handle)))

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_setNativeEditorContent) {
#if 0
    int cursorIndex = KNI_GetParameterAsInt(2);
    _JavaString *jstr;
    jchar *p;
    int strLen;

    if (editBoxShown) {
        KNI_StartHandles(1);
        KNI_DeclareHandle(str);
        KNI_GetParameterAsObject(1, str);

        jstr = getJavaStringPtr(str);
        p = jstr->value->elements + jstr->offset;
        strLen = KNI_GetStringLength(str);

        /* This is OK: we know that the Java string that gets passed here
         * is always on the heap.
         */
        jchar saved = p[strLen];
        p[strLen] = 0;
        int oldSize = GetWindowTextLength(hwndTextActive);

        SendMessage(hwndTextActive, EM_SETSEL, (WPARAM)0, (LPARAM)oldSize);
        SendMessage(hwndTextActive, EM_REPLACESEL, 0, (LPARAM)((LPSTR)p));
        SendMessage(hwndTextActive, EM_SETSEL, cursorIndex, cursorIndex);
        p[strLen] = saved; /* restore corruption of the heap! */

        KNI_EndHandles();
    }
#endif
    KNI_ReturnVoid();
}

/* Hide the text editor. */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_disableNativeEditor) {
    if (editBoxShown) {
        ShowWindow(hwndTextActive, SW_HIDE);
        int oldSize = GetWindowTextLength(hwndTextActive);
        SendMessage(hwndTextActive, EM_SETSEL, (WPARAM)0, (LPARAM)oldSize);
        SendMessage(hwndTextActive, EM_REPLACESEL, 0, (LPARAM)(TEXT("")));
        editBoxShown = 0;
    }
    KNI_ReturnVoid();
}

/* Return the content of the editor in a Java string. */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorContent) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(chars);
#if 0
    if (editBoxShown) {
        int strLen = GetWindowTextLength(hwndTextActive);
        jchar *tmp = (jchar*)midpMalloc((strLen + 1) * sizeof(jchar));
        if (tmp) {
            GetWindowText(hwndTextActive, (LPTSTR)tmp, strLen+1); /* 0-terminated */
            SNI_NewArray(SNI_CHAR_ARRAY, strLen, chars);
            if (!KNI_IsNullHandle(chars)) {
                memcpy(JavaCharArray(chars), tmp, strLen*sizeof(jchar));
            }
            midpFree((void*)tmp);
        }
    }
#endif
    KNI_EndHandlesAndReturnObject(chars);
}

/* Return the position of the caret, so that we can remember the caret
 * location of each text box while we traverse them.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorCursorIndex) {
    int caret = 0;
    if (editBoxShown) {
        WORD w = (WORD)SendMessage(hwndTextActive, EM_GETSEL, 0, 0L);
        caret = LOWORD(w);
    }
    KNI_ReturnInt(caret);
}

/* This is called by MIDPWindow when it's above to draw the "wash" layer
 * (to grey-out the current Form before drawing the menu).
 *
 * At this point, we don't know what the current active TextFieldLFImpl is.
 * So we just send a message to the FormLFImpl.uCallPeerStateChanged()
 * method, who will have a better idea about the active TextFieldLFImpl.
 *
 * We must withdraw the text editor now. We save its content to a malloc'ed
 * buffer to pass to FormLFImpl.uCallPeerStateChanged().
 *
 * TODO: there's probably a better way to handle this.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_MIDPWindow_disableAndSyncNativeEditor) {
    MidpEvent event;

    MIDP_EVENT_INITIALIZE(event);
    if (editBoxShown) {
        int strLen = GetWindowTextLength(hwndTextActive);
        jchar *tmp = (jchar*)midpMalloc((strLen + 1) * sizeof(jchar));
        if (tmp) {
            GetWindowText(hwndTextActive, (LPTSTR)tmp, strLen+1); /* 0-terminated */
            WORD w = (WORD)SendMessage(hwndTextActive, EM_GETSEL, 0, 0L);
            int caret = LOWORD(w);

            event.type = MIDP_PEER_CHANGED_EVENT;
            event.intParam1 = (int)tmp;
            event.intParam2 = strLen;
            event.intParam3 = caret;
            midpStoreEventAndSignalForeground(event);
        }
    }

    ShowWindow(hwndTextActive, SW_HIDE);
    editBoxShown = 0;
    KNI_ReturnVoid();
}

/* This is called by the last active TextFieldLFImpl to retrieve
 * the contents of the native editor, which was saved by
 * MIDPWindow.disableAndSyncNativeEditor() into a malloc'ed buffer above.
 * See disableAndSyncNativeEditor above for more info.
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_mallocToJavaChars) {
    int strLen = KNI_GetParameterAsInt(2);
    jchar *tmp = (jchar*)KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(chars);
#if 0
    SNI_NewArray(SNI_CHAR_ARRAY, strLen, chars);
    if (!KNI_IsNullHandle(chars)) {
        memcpy(JavaCharArray(chars), tmp, strLen*sizeof(jchar));
    }
#endif
    midpFree((void*)tmp);
    KNI_EndHandlesAndReturnObject(chars);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midlet_MIDletPeer_dismissNativeEditors) {
    if (editBoxShown) {
        ShowWindow(hwndTextActive, SW_HIDE);
        editBoxShown = 0;
    }
    KNI_ReturnVoid();
}

/**
 * Invert screen rotation flag
 */
jboolean winceapp_reverse_orientation() {
    reverse_orientation = !reverse_orientation;
    gxj_system_screen_buffer.width = get_screen_width();
    gxj_system_screen_buffer.height = get_screen_height();
    return reverse_orientation;
}

/**
 * Invert screen rotation flag
 */
jboolean winceapp_get_reverse_orientation() {
    return reverse_orientation;
}

/**
 * Return screen width
 */
int get_screen_width() {
    if (reverse_orientation) {
        return CHAM_HEIGHT;
    } else {
        return CHAM_WIDTH;
    }

}

/**
 * Return screen height
 */
int get_screen_height() {
    if (reverse_orientation) {
        return CHAM_WIDTH;
    } else {
        return CHAM_HEIGHT;
    }
}

static TCHAR menus[2][20];

void native_set_softbutton(int index, jchar *label, int labelLen) {
    int i;
    if (labelLen > 19) {
        labelLen = 19;
    }
    for (i=0; i<labelLen; i++) {
        menus[index][i] = (TCHAR)label[i];
    }
    menus[index][labelLen] = 0;

    TBBUTTONINFO tbbi = {0};
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = menus[index];
    tbbi.cchText = labelLen;
    SendMessage(hwndMenuBar, TB_SETBUTTONINFO, index+IDM_SOFTBTN_0,
                (LPARAM)&tbbi);
}

} /* extern "C" */
