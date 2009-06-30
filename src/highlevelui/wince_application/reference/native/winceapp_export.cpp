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


#if ENABLE_DIRECT_DRAW
#include <ddraw.h>
#endif

#ifdef ENABLE_JSR_184
#include <swvapi.h>
#endif


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

#define KEYMAP_MD_KEY_HOME (KEYMAP_KEY_MACHINE_DEP)

/* global variables defined in midp_msgQueue_md.c */
extern int inMidpEventLoop;
extern int lastWmSettingChangeTick;

jboolean bVirtualModeEnabled = KNI_FALSE;

gxj_screen_buffer gxj_system_screen_buffer;

static HWND hwndToolbar = NULL;
static HWND hwndMain = NULL;
static HWND hwndTextActive = NULL;
static HWND hwndTextField = NULL;
static HWND hwndTextBox = NULL;
static WNDPROC oldTextFieldProc;
static WNDPROC oldTextBoxProc;
static int editBoxShown = 0;
static int editCHX, editCHY, editCHW, editCHH; /* in CHAM coordinates */
static int titleHeight = JWC_WINCE_TITLE_HEIGHT;
static RECT rcVisibleDesktop;
static HANDLE eventThread;
static HINSTANCE instanceMain;
static jboolean reverse_orientation;
static int lastKeyPressed = 0;
static HANDLE eventWindowInit;

/* IMPL_NOTE: the drawing code requires 16-bit per pixel mode */
/* Bitmap and memory DC needed for GDI draw, if DD/GAPI
   draw fails, also for the rotated screen orientation */
HBITMAP g_hBitmap = NULL;
HDC g_hMemDC = NULL;

struct ScreenAccess {
    int                 width;
    int                 height;
#if ENABLE_DIRECT_DRAW
    LPDIRECTDRAW        pDD;
    LPDIRECTDRAWSURFACE pDDSPrimary;
    LPDIRECTDRAWSURFACE pDDSMemory; // Normal paint memory surface
    LPDIRECTDRAWSURFACE pDDSDirect; // Buffer surface for winceapp_direct_flush()
    //LPDIRECTDRAWCLIPPER pDDClipper; 
#else
    int                 yPitch;
    gxj_pixel_type*     pixels;
#endif
};

static ScreenAccess g_screen;

/* IMPL_NOTE: need a better way for quitting.  */
extern int _quit_now; /* defined in Scheduler.cpp */
extern int midpPaintAllowed;

extern int wince_init_fonts(); /* DirectDraw only.  */

DWORD lastUserInputTick = 0;

static void processSkippedRefresh();
static LRESULT processKey(HWND hwnd, UINT action, int key);
static LRESULT processSystemKey(HWND hwnd, int key);
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
        if (caret != 0)
            c = KEYMAP_KEY_INVALID;
    } else if (c == KEYMAP_KEY_RIGHT) {
        /* Don't send the key to MIDP unless we are at the last character  */
        WORD w = (WORD)SendMessage(hwnd, EM_GETSEL, 0, 0L);
        int strLen = GetWindowTextLength(hwnd);
        int caret = LOWORD(w);
        if (caret < strLen)
            c = KEYMAP_KEY_INVALID;
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
    } else
        return CallWindowProc(oldproc, hwnd, msg, wp, lp);
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
                                 0, 0, 40, 40, hwndMain, 0, 
				 GetModuleHandle(NULL), NULL);
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

static void showToolbar(bool bShow) {
    ShowWindow(hwndToolbar, bShow ? SW_SHOWNORMAL : SW_HIDE);
    UpdateWindow(hwndToolbar);
}

static void updateDimensions() {
    SIPINFO sipinfo;
    RECT rc;

    memset(&sipinfo, 0, sizeof(sipinfo));
    sipinfo.cbSize = sizeof(SIPINFO);
    SipGetInfo(&sipinfo);

    if (bVirtualModeEnabled) {
        memcpy(&rc, &sipinfo.rcVisibleDesktop, sizeof(rc));
        if (sipinfo.fdwFlags & SIPF_ON) {
            rc.bottom = sipinfo.rcSipRect.top;
        } else {
            RECT rcToolbar;
            GetWindowRect(hwndToolbar, &rcToolbar);
            rc.bottom = rcToolbar.top;
        }
    } else {
        SHFullScreen(hwndMain, SHFS_HIDESIPBUTTON);
        GetWindowRect(hwndMain, &rc);
        rc.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    rcVisibleDesktop = rc;

    gxj_system_screen_buffer.width = rc.right - rc.left;
    gxj_system_screen_buffer.height = rc.bottom - rc.top;

    MoveWindow(hwndMain, rc.left, rc.top, gxj_system_screen_buffer.width,
        gxj_system_screen_buffer.height, TRUE);
    showToolbar(bVirtualModeEnabled);
}

static void initPutpixelSurface() {
    /* Get initial screen dimensions */
    updateDimensions();
    /* Use the dimension to initialize Putpixel surface */
    int screenSize = sizeof(gxj_pixel_type) *
         GetSystemMetrics(SM_CXSCREEN) * GetSystemMetrics(SM_CYSCREEN);
    gxj_system_screen_buffer.alphaData = 0;
    gxj_system_screen_buffer.pixelData = 
        (gxj_pixel_type *)midpMalloc(screenSize);
    memset(gxj_system_screen_buffer.pixelData, 0xff, screenSize);
}

static void releasePutpixelSurface() {
    midpFree(gxj_system_screen_buffer.pixelData);
}

static void deleteGDIObjects() {
    if (NULL != g_hMemDC) {
        DeleteDC(g_hMemDC);
        g_hMemDC = NULL;
    }
    if (NULL != g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
}

#if ENABLE_DIRECT_DRAW
static void releaseDirectDraw();

static void initDirectDraw() {
    /* Note: if DirectDraw fails to initialize, we will use GDI */
    if (DD_OK != DirectDrawCreate(NULL, &g_screen.pDD, NULL))
        return;

    if (DD_OK != g_screen.pDD->SetCooperativeLevel(hwndMain, DDSCL_NORMAL)/* ||
        DD_OK != g_screen.pDD->CreateClipper(0, &g_screen.pDDClipper, NULL)*/) {
        g_screen.pDD->Release();
        g_screen.pDD = NULL;
        return;
    }

    // Create the primary surface with 0 back buffer
    DDSURFACEDESC ddsd;
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (/*DD_OK != g_screen.pDDClipper->SetHWnd(0, hwndMain) ||*/
        DD_OK != g_screen.pDD->CreateSurface(&ddsd, &g_screen.pDDSPrimary, NULL)) {
        //g_screen.pDDClipper->Release();
        //g_screen.pDDClipper = NULL;
        g_screen.pDD->Release();
        g_screen.pDD = NULL;
        return;
    }

    if (/*DD_OK != g_screen.pDDSPrimary->SetClipper(g_screen.pDDClipper) ||*/
        DD_OK != g_screen.pDDSPrimary->GetSurfaceDesc(&ddsd)) {
        releaseDirectDraw();
        return;
    }
    g_screen.width = ddsd.dwWidth;
    g_screen.height = ddsd.dwHeight;

    wince_init_fonts();
}


static void releaseDirectDraw() {
    if (NULL == g_screen.pDD)
        return;
    if (NULL != g_screen.pDDSDirect) {
        g_screen.pDDSDirect->Release();
        g_screen.pDDSDirect = NULL;
    }
    if (NULL != g_screen.pDDSMemory) {
        g_screen.pDDSMemory->Release();
        g_screen.pDDSMemory = NULL;
    }
    if (NULL != g_screen.pDDSPrimary) {
        //g_screen.pDDSPrimary->SetClipper(NULL);
        g_screen.pDDSPrimary->Release();
        g_screen.pDDSPrimary = NULL;
    }
    /*if (NULL != g_screen.pDDClipper) {
        g_screen.pDDClipper->Release();
        g_screen.pDDClipper = NULL;
    }*/
    g_screen.pDD->Release();
    g_screen.pDD = NULL;
}

/*
 * Flushing for direct_flush();
 */
static void doFlush(int height) {
    RECT srcRect, dstRect;
    srcRect.top = 0;
    srcRect.left = 0;
    srcRect.bottom = height;
    srcRect.right = winceapp_get_screen_width();

    dstRect.top = rcVisibleDesktop.top;
    dstRect.left = 0;
    dstRect.bottom = height + rcVisibleDesktop.top;
    dstRect.right = winceapp_get_screen_width();

    if (NULL != g_screen.pDDSDirect)
        g_screen.pDDSPrimary->Blt(&dstRect, g_screen.pDDSDirect, &srcRect, 0, NULL);
}

/**
 * Create memory based DD surface
 */
static LPDIRECTDRAWSURFACE createMemorySurface(void* pVmem, int width, int height) {
    ASSERT(g_screen.pDD);
    ASSERT(pVmem);

    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE pDDS = NULL;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ZeroMemory(&ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

    ddsd.dwSize         = sizeof(ddsd);
    ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LPSURFACE |
                          DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth        = width;
    ddsd.dwHeight       = height;
    ddsd.lPitch         = (LONG)sizeof(gxj_pixel_type) * width;
    ddsd.lpSurface      = pVmem;

    // Set up the pixel format for 16-bit RGB (5-6-5).
    ddsd.ddpfPixelFormat.dwSize         = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags        = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount  = 16;
    ddsd.ddpfPixelFormat.dwRBitMask     = 0x1f << 11;
    ddsd.ddpfPixelFormat.dwGBitMask     = 0x3f << 5;
    ddsd.ddpfPixelFormat.dwBBitMask     = 0x1f;

	if (DD_OK != g_screen.pDD->CreateSurface(&ddsd, &pDDS, NULL))
        return NULL;
    else
        return pDDS;
}
#endif /* ENABLE_DIRECT_DRAW */

/**
 * Initializes the WINCE native resources.
 */

static PTCHAR    _szAppName    = TEXT(MAIN_WINDOW_CLASS_NAME);
static PTCHAR    _szTitle      = TEXT(MAIN_WINDOW_CLASS_NAME);
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
    _hInstance = hInstance;

    return RegisterClass(&wc);
}

static void CreateMenuBar() {
    SHMENUBARINFO mbi;
    
    memset(&mbi, 0, sizeof(SHMENUBARINFO));
    mbi.cbSize = sizeof(SHMENUBARINFO);
    mbi.hwndParent = hwndMain;
    mbi.dwFlags = SHCMBF_EMPTYBAR | SHCMBF_HIDDEN;
    mbi.hInstRes = _hInstance;
    
    if (SHCreateMenuBar(&mbi)) {
        hwndToolbar = mbi.hwndMB;
    }
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

    if (!_hwndMain)
        return FALSE;

    winceapp_set_window_handle(_hwndMain);
    ShowWindow(_hwndMain, CmdShow);
    UpdateWindow(_hwndMain);

#ifdef ENABLE_CDC
    /* Temporary fix to enter exclusive input mode.
     * A better solution should register for hot keys.
     */
    GXOpenInput();
#endif

    return TRUE;
}

static BOOL initWindows(HINSTANCE hInstance, int nShowCmd) {
    if (!InitApplication(hInstance))
        return FALSE;
    if (!InitInstance(hInstance, nShowCmd))
        return FALSE;
    return TRUE;
}

DWORD WINAPI CreateWinCEWindow(LPVOID lpParam) {
    static MidpReentryData newSignal;
    static MidpEvent newMidpEvent;

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


    if (!initWindows(GetModuleHandle(NULL), SW_SHOW)) {
        REPORT_ERROR(LC_AMS, "init_gui() failed");
        MessageBox(NULL, TEXT("Failed to start JWC"), TEXT("Bye"), MB_OK);
    }

    CreateMenuBar();
    initPutpixelSurface();
    if (eventWindowInit) {
        SetEvent(eventWindowInit);
    }

#if ENABLE_DIRECT_DRAW
    initDirectDraw();
#else
    if (GXOpenDisplay(hwndMain, 0) == 0)
        REPORT_ERROR(LC_HIGHUI, "GXOpenDisplay() failed");
#endif

#ifdef ENABLE_SPLASH_SCREEN
    /* Destroy the splash screen window by class name and window title */
    HWND hSplashWindow = FindWindow(_T("JavaSplash"), _T("Java"));
    if (hSplashWindow != NULL) {
        DestroyWindow(hSplashWindow);
    }
#endif

    // createEditors();

#ifdef ENABLE_JSR_184
    engine_initialize();
#endif

    MIDP_EVENT_INITIALIZE(newMidpEvent);
    while (1) {
        checkForSystemSignal(&newSignal, &newMidpEvent, 200);
    }
}

void winceapp_init() {
    eventWindowInit = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (eventWindowInit == NULL) {
        REPORT_ERROR(LC_AMS, "Creating WindowInit event failed");
    }
    eventThread = CreateThread(NULL, 0, CreateWinCEWindow, 0, 0, NULL);
    if (eventWindowInit) {
        WaitForSingleObject(eventWindowInit, 10000); // wait for 10 seconds maximum
        CloseHandle(eventWindowInit);
    }
}

static jint mapAction(UINT msg, LPARAM lp) {
    if (msg == WM_KEYUP)
        return KEYMAP_STATE_RELEASED;
    else
        return (lp&0x40000000)?KEYMAP_STATE_REPEATED:KEYMAP_STATE_PRESSED;
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
    case VK_RETURN:  return KEYMAP_KEY_SELECT;
    case VK_BACK:
    case VK_TBACK:   return KEYMAP_KEY_BACKSPACE;
    case VK_TTALK:
    case VK_THOME:   return KEYMAP_MD_KEY_HOME;
    case VK_TEND:    return KEYMAP_KEY_END;

    case VK_TSOFT1:  return KEYMAP_KEY_SOFT1;
    case VK_TSOFT2:  return KEYMAP_KEY_SOFT2;
    }
    if (wParam >= 0x20 && wParam <= 0x7F)
        /* Some ASCII keys sent by emulator or mini keyboard */
        return (jint)wParam;
    return KEYMAP_KEY_INVALID;
}

static void disablePaint() {
    if (inMidpEventLoop)
        midpPaintAllowed = 0;
}

static void enablePaint() {
    if (inMidpEventLoop) {
        midpPaintAllowed = 1;
        processSkippedRefresh();
    }
}

/**
 * Handles window messages sent to the main window.
 */
LRESULT CALLBACK winceapp_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    PAINTSTRUCT ps;
    HDC hdc;
    int cmd;
    DWORD err;
    static int ignoreCancelMode = 0;
    static HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    static bool bButtonDown = false;
    LRESULT result = 0;

    switch (msg) {
    case WM_CREATE:
        break;

    case WM_HOTKEY:
        /* If back key is overriden, back button messages are sent in
         * a WM_HOTKEY to the menu bar with the id VK_TBACK in the
         * LPARAM.
         */
        if (HIWORD(lp) == VK_TBACK && (LOWORD(lp) & MOD_KEYUP)) {
            if (editBoxShown)
                SendMessage(hwndTextActive, WM_CHAR, VK_BACK, 0);
            else {
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
        result = 1;
        break;

    case WM_SETTINGCHANGE:
        if (SETTINGCHANGE_RESET == wp || SPI_SETSIPINFO == wp || SPI_SETCURRENTIM == wp || SPI_SIPMOVE == wp) {
            updateDimensions();
            updateEditorForRotation();
            lastWmSettingChangeTick = GetTickCount();
            
            pMidpEventResult->type = ROTATION_EVENT;
            pSignalResult->waitingFor = UI_SIGNAL;
            pMidpEventResult->DISPLAY = gForegroundDisplayId;
            sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
            result = 0;
            enablePaint();
        }
        break;
    case WM_TIMER:
        break;

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
            if (!midpPaintAllowed)
                enablePaint();
            break;
        default:
            result = DefWindowProc(hwnd, msg, wp, lp);
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wp)) { /* active */
            enablePaint();
            if (editBoxShown)
                SetFocus(hwndTextActive);
        } else { /* inactive */
#if ENABLE_MULTIPLE_ISOLATES
            pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
            pSignalResult->waitingFor = AMS_SIGNAL;
#endif
            disablePaint();
        }
        result = DefWindowProc(hwnd, msg, wp, lp);
        break;

    case WM_EXITMENULOOP:
        enablePaint();
        result = DefWindowProc(hwnd, msg, wp, lp);
        break;

    case WM_CANCELMODE:
        if (!ignoreCancelMode) {
            disablePaint();
        } else {
            ignoreCancelMode--;
        }
        /* We have to do this, or else windows is unhappy. */
        result = DefWindowProc(hwnd, msg, wp, lp);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        enablePaint();
        if (editBoxShown)
            SetFocus(hwndTextActive);
        break;

    case WM_DESTROY:
        winceapp_finalize();
#ifdef ENABLE_CDC
        /* Temporary fix, leaving exclusive input mode */
        GXCloseInput();
#endif
        PostQuitMessage(0);
        exit(0);
        break;

    case WM_LBUTTONDOWN:
        //ResetEvent(hEvent);
        bButtonDown = true;
    case WM_MOUSEMOVE:
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
            if (msg == WM_LBUTTONUP) {
                bButtonDown=false;
                //SetEvent(hEvent);
            }
        }
        break;
    case WM_KEYDOWN: /* fall through */
    case WM_KEYUP:
        switch (wp) {
        case VK_RETURN:  
        case VK_BACK: 
        case VK_TBACK: 
        case VK_UP: 
        case VK_DOWN: 
        case VK_LEFT: 
        case VK_RIGHT:
        case VK_TSOFT1:
        case VK_TSOFT2:
            result = processKey(hwnd, mapAction(msg, lp), mapKey(wp, lp));
            break;
        case VK_THOME:
        case VK_TTALK:
        case VK_TEND:
            if (WM_KEYDOWN == msg)
                result = processSystemKey(hwnd, mapKey(wp, lp));
            break;
        default:
            // May need special handling for soft keys?  Not sure yet...
            if (0 != lastKeyPressed && WM_KEYUP == msg) { 
                //should use cached pressed key code for input
                result = processKey(hwnd, KEYMAP_STATE_RELEASED, lastKeyPressed);
            }
            lastKeyPressed = 0;
        }
        break;
    case WM_CHAR:
        if (wp >= 0x20 && wp <= 0x7f) {
            lastKeyPressed = wp;
            result = processKey(hwnd, mapAction(msg, lp), lastKeyPressed);
        }
        break;
    default:
        result = DefWindowProc(hwnd, msg, wp, lp);
    }
    return result;
}

static LRESULT processKey(HWND hwnd, UINT action, int key) {
    lastUserInputTick = GetTickCount();

    switch (key) {
    case KEYMAP_KEY_INVALID:
        break;
    default:
        pMidpEventResult->ACTION = action;
        pMidpEventResult->type = MIDP_KEY_EVENT;
        pMidpEventResult->CHR = key;

        pSignalResult->waitingFor = UI_SIGNAL;
        pMidpEventResult->DISPLAY = gForegroundDisplayId;
        sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
    }
    return 0;
}

static LRESULT processSystemKey(HWND hwnd, int key) {
    switch (key) {
    case KEYMAP_MD_KEY_HOME:
        pSignalResult->waitingFor = AMS_SIGNAL;
        pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
        pMidpEventResult->intParam1 = 0;
        sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
        break;

    case KEYMAP_KEY_END:
        pSignalResult->waitingFor = AMS_SIGNAL;
        pMidpEventResult->type = MIDLET_DESTROY_REQUEST_EVENT;
        pMidpEventResult->DISPLAY = gForegroundDisplayId;
        pMidpEventResult->intParam1 = gForegroundIsolateId;
        sendMidpKeyEvent(pMidpEventResult, sizeof(*pMidpEventResult));
        break;
	}
	return 0;
}

/**
 * Finalize the WINCE native resources.
 */
void winceapp_finalize() {
#if ENABLE_DIRECT_DRAW
    releaseDirectDraw();
#else
    GXCloseDisplay();
#endif
    deleteGDIObjects();
#ifdef ENABLE_JSR_184
    engine_uninitialize();
#endif

    CloseHandle(eventThread);
    releasePutpixelSurface();
}

static void processSkippedRefresh() {
    winceapp_refresh(0, 0, winceapp_get_screen_width(), winceapp_get_screen_height());
}

int isScreenFullyVisible() {
    if (JWC_WINCE_SMARTPHONE)
        /* No SIP window or screen rotation on SmartPhone (presumably ...) */
        return 1;
    else {
        /* This is false if the screen has been rotated or SIP is up */
        int w = rcVisibleDesktop.right - rcVisibleDesktop.left;
        int h = rcVisibleDesktop.bottom - rcVisibleDesktop.top;
        return (w >= winceapp_get_screen_width() &&
                h >= winceapp_get_screen_height());
    }
}

#if ENABLE_DIRECT_DRAW
int isScreenRotated() {
    DEVMODE devMode;
    devMode.dmSize = sizeof(devMode);
    devMode.dmFields = DM_DISPLAYORIENTATION;
    devMode.dmDisplayOrientation = DMDO_0;
    ChangeDisplaySettingsEx(NULL, &devMode, NULL, CDS_TEST, NULL);
    return (devMode.dmDisplayOrientation != DMDO_0);
}
#endif

static BOOL startDirectPaint() {
#if ENABLE_DIRECT_DRAW
    if (g_screen.pDD == NULL || isScreenRotated() || !isScreenFullyVisible() || editBoxShown)
        /* DDraw is not very reliable on a rotated screen. Use GDI instead. */
        return FALSE;

    if (g_screen.pDDSMemory == NULL) {
        g_screen.pDDSMemory = createMemorySurface(gxj_system_screen_buffer.pixelData,
            winceapp_get_screen_width(), winceapp_get_screen_height());
        if (g_screen.pDDSMemory == NULL)
            return FALSE;
    }
    
    if (DD_OK != g_screen.pDDSPrimary->IsLost()) {
        if (DD_OK != g_screen.pDDSPrimary->Restore())
            return FALSE;

        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(ddsd);
        g_screen.pDDSPrimary->GetSurfaceDesc(&ddsd);
        g_screen.width = ddsd.dwWidth;
        g_screen.height = ddsd.dwHeight;
    }
#else
    if (editBoxShown)
        return FALSE;
    if (g_screen.width == 0) {
        GXDisplayProperties dp = GXGetDisplayProperties();
        g_screen.width = dp.cxWidth;
        g_screen.height = dp.cyHeight;
        g_screen.yPitch = dp.cbyPitch;
    }
    g_screen.pixels = (gxj_pixel_type*)GXBeginDraw();
    if (NULL == g_screen.pixels)
        return FALSE;
#endif
    return TRUE;
}

static void endDirectPaint() {
#if ENABLE_DIRECT_DRAW
#else
    GXEndDraw();
    g_screen.pixels = NULL;
#endif
}

void GDIBitBlt(gxj_pixel_type *src, int x1, int x2, int y1, int y2) {
    HDC hDC = GetDC(hwndMain);

    static BOOL previousOrientation = winceapp_get_reverse_orientation();
    BOOL orientationChanged = winceapp_get_reverse_orientation() != previousOrientation;

    if (orientationChanged || g_hBitmap == NULL) {
        deleteGDIObjects();
        g_hBitmap = CreateCompatibleBitmap(hDC,
            winceapp_get_screen_width(), winceapp_get_screen_height());
        g_hMemDC = CreateCompatibleDC(hDC);
     }

    SelectObject(g_hMemDC, g_hBitmap);
    SetBitmapBits(
        g_hBitmap,
        winceapp_get_screen_width() * winceapp_get_screen_height()
        * sizeof(gxj_pixel_type),
        src);
    BitBlt(hDC, x1, y1, x2 - x1, y2 - y1, g_hMemDC, x1, y1, SRCCOPY);

    ReleaseDC(hwndMain, hDC);
    previousOrientation = winceapp_get_reverse_orientation();
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
    if (NULL == hwndMain || !midpPaintAllowed)
        return;

    if (x2 > winceapp_get_screen_width())
        x2 = winceapp_get_screen_width();
    
    if (y2 > winceapp_get_screen_height())
        y2 = winceapp_get_screen_height();

    /* Make sure the copied lines are 4-byte aligned for faster memcpy */
    if ((x1 & 0x01) == 1)
        x1 -= 1;
    if ((x2 & 0x01) == 1)
        x2 += 1;

    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;

    if (!startDirectPaint()) {
        /* Switching to GDI */
        GDIBitBlt(src, x1, x2, y1, y2);
        return;
    }

    int maxY = g_screen.height - titleHeight;
    if (y2 > maxY)
        y2 = maxY;
    if (x2 > g_screen.width)
        x2 = g_screen.width;
    
#if ENABLE_DIRECT_DRAW
    if (x2 <= x1 || y2 <= y1) 
        // it is possible srcHeight < 0 when SIP shown and java refresh 
        // the lower part screen(system menu up/dn,etc.)
        return;

    RECT srcRect, dstRect;
    srcRect.top = y1;
    srcRect.left = x1;
    srcRect.bottom = y2;
    srcRect.right = x2;

    dstRect.top = y1 + rcVisibleDesktop.top;
    dstRect.left = x1;
    dstRect.bottom = y2 + rcVisibleDesktop.top;
    dstRect.right = x2;

    g_screen.pDDSPrimary->Blt(&dstRect, g_screen.pDDSMemory, &srcRect, 0, NULL); 

#else /* !ENABLE_DIRECT_DRAW */
    int srcWidth = x2 - x1;
    int srcHeight = y2 - y1;

    gxj_pixel_type *dst = (gxj_pixel_type*)(((int)g_screen.pixels) +
        (titleHeight + y1) * g_screen.yPitch) + x1;

    src += y1 * winceapp_get_screen_width() + x1;

    if (x1 == 0 && srcWidth == winceapp_get_screen_width() && g_screen.width == srcWidth &&
        g_screen.yPitch == g_screen.width * 2) {
	/* assuming srcHeight==full height ? */ 
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type) * srcHeight);
    } else
        for (; y1 < y2; y1++) {
            memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
            src += winceapp_get_screen_width();
            dst = (gxj_pixel_type*)( ((int)dst) + g_screen.yPitch );
        }
#endif /* ENABLE_DIRECT_DRAW */
    endDirectPaint();
}


jboolean winceapp_direct_flush(const java_graphics *g,
			       const java_imagedata *srcImageDataPtr, 
			       int height) {
    if (srcImageDataPtr == NULL || srcImageDataPtr->pixelData == NULL || !midpPaintAllowed)
        return KNI_FALSE;

    gxj_pixel_type* src = (gxj_pixel_type *)&(srcImageDataPtr->pixelData->elements[0]);
    int width = srcImageDataPtr->width;

#if ENABLE_DIRECT_DRAW /* ENABLE_DIRECT_DRAW */
    // direct_flush must be fast. So Deny to flush, if DD is not initialized
    if (g_screen.pDD == NULL)
        return KNI_FALSE;

    static gxj_pixel_type * lastSrc = NULL; // last flush src

    if (lastSrc != src && g_screen.pDDSDirect != NULL) {
        g_screen.pDDSDirect->Release();
        g_screen.pDDSDirect = NULL;
    }

    if (g_screen.pDDSDirect == NULL) {
        g_screen.pDDSDirect = createMemorySurface(src,
            winceapp_get_screen_width(), winceapp_get_screen_height());

        if (g_screen.pDDSDirect == NULL) {
            return KNI_FALSE;
        }
        lastSrc = src;
    }

    if (height > winceapp_get_screen_height())
        height = winceapp_get_screen_height();

    doFlush(height);
    return KNI_TRUE;
#else  /* !ENABLE_DIRECT_DRAW */
    return KNI_FALSE; // No DirectDraw
#endif /* ENABLE_DIRECT_DRAW */    
}

char * strdup(const char *s) {
    char *result = (char*)malloc(strlen(s) + 1);
    if (result)
        strcpy(result, s);
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
#if ENABLE_DIRECT_DRAW
    /*  pDDS and cachedHDC must both be NULL or both be non-NULL */
    static LPDIRECTDRAWSURFACE pDDS = NULL;
    static HDC cachedHDC = NULL;
    static gxj_pixel_type *cachedBuffer = NULL;

    if (g_screen.pDD == NULL)
        /* DirectDraw failed to initialize. 
         * Let's use GDI to Blit to the LCD. 
         */
        return NULL;

    if (buffer == cachedBuffer && cachedHDC != NULL && !pDDS->IsLost())
        /* Note: after screen rotation has happened, the pDDS surface may
         * be lost, even if it's using a client-defined pixel buffer!
         */
        return cachedHDC;

    if (pDDS != NULL) {
        pDDS->ReleaseDC(cachedHDC);
        pDDS->Release();
        pDDS = NULL;
        cachedHDC = NULL;
    }
    
    pDDS = createMemorySurface(buffer, width, height);

    if (pDDS == NULL) {
        cachedHDC = NULL;
        return NULL;
    }

    if (DD_OK != pDDS->GetDC(&cachedHDC)) {
        pDDS->Release();
        pDDS = NULL;
        cachedHDC = NULL;
        return NULL;
    }

    cachedBuffer = buffer;
    return cachedHDC;
#else
    return NULL;
#endif /* ENABLE_DIRECT_DRAW */
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

    hwndTextActive = multiline ? hwndTextBox : hwndTextField;

    editCHX = x;
    editCHY = y;
    editCHW = w;
    editCHH = h;

    if (!JWC_WINCE_SMARTPHONE) {
        int diff = rcVisibleDesktop.right - rcVisibleDesktop.left - winceapp_get_screen_width();
        if (diff > 0)
            x += diff / 2;
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
        int diff = rcVisibleDesktop.right - rcVisibleDesktop.left - winceapp_get_screen_width();
        if (diff > 0)
            x += diff / 2;
        if (editBoxShown)
            SetWindowPos(hwndTextActive, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
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
	  /* 0-terminated */
            GetWindowText(hwndTextActive, (LPTSTR)tmp, strLen+1);
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
	    /* 0-terminated */
            GetWindowText(hwndTextActive, (LPTSTR)tmp, strLen+1);
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
    if (!KNI_IsNullHandle(chars))
        memcpy(JavaCharArray(chars), tmp, strLen * sizeof(jchar));
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
    return reverse_orientation;
}

/**
 * Handle clamshell event
 */
void winceapp_hadle_clamshell_event() {
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
int winceapp_get_screen_width() {
    return gxj_system_screen_buffer.width;
}

/**
 * Return screen height
 */
int winceapp_get_screen_height() {
    return gxj_system_screen_buffer.height;
}
} /* extern "C" */
