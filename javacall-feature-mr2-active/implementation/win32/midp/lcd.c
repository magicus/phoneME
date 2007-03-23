/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * Win32 implementation of lcd APIs and emulator.
 */


#include <stdio.h>
#include <windows.h>
#include <process.h>

#include "javacall_lcd.h"
#include "javacall_logging.h"
#include "javacall_keypress.h"
#include "javacall_penevent.h"
#include "javacall_socket.h"
#include "javacall_datagram.h"
#include "javacall_lifecycle.h"

#include "img/topbar.h"


#include "lcd.h"
#include "local_defs.h"

#include "res/resource.h"

#define SKINS_MENU_SUPPORTED

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))

#define TOP_BAR_HEIGHT         11

/*
 * Defines screen size
 */
#define DISPLAY_WIDTH          240//180
#define DISPLAY_HEIGHT         320//(198 + TOP_BAR_HEIGHT)

/*
 * This (x,y) coordinate pair refers to the offset of the upper
 * left corner of the display screen within the MIDP phone handset
 * graphic window
 */
#define X_SCREEN_OFFSET        61
#define Y_SCREEN_OFFSET        75

#define MAX_SOFTBUTTON_CHARS   12

#define MENUBAR_BORDER_HEIGHT  2
#define ARROWS_WIDTH           7
#define ARROWS_HEIGHT          7
#define ARROWS_GAP             1
#define BOTTOM_BAR_HEIGHT (MENUBAR_BORDER_HEIGHT + ARROWS_HEIGHT + \
    ARROWS_GAP + ARROWS_HEIGHT + MENUBAR_BORDER_HEIGHT)
#define UNTRANSLATED_SCREEN_BITMAP (void*)0xffffffff

#define VERT_X   (x_offset + (DISPLAY_WIDTH/2) - 4)
#define UP_Y     (y_offset + paintHeight + MENUBAR_BORDER_HEIGHT)

                   /* y of the up arrow */
#define DOWN_Y   (UP_Y + ARROWS_GAP + ARROWS_HEIGHT)

                   /* y of the down arrow */

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)printf( "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

#define ASSERT(expr) (expr) ? (void)0 : (void)printf( \
    "%s:%d: (%s)is NOT true\n", __FILE__, __LINE__, #expr)

#define MD_KEY_HOME (KEY_MACHINE_DEP)

static HBITMAP getBitmapDCtmp = NULL;

typedef unsigned short unicode;
typedef struct {
    int     num;
    unicode label[MAX_SOFTBUTTON_CHARS];
} SoftButtonLabel;

static SoftButtonLabel llabel;
static SoftButtonLabel rlabel;

typedef struct _mbs {
    HBITMAP bitmap;
    HBITMAP mask;
    int width;
    int height;
    int mutable;
    unsigned char *image;
    unsigned char *imageMask;
    char prop;
} myBitmapStruct;

/* Network Indicator position parameters */
#define LED_xposition  17
#define LED_yposition  82
#define LED_width      20
#define LED_height     20

#define INSIDE(_x, _y, _r)                              \
    ((_x >= (_r).x) && (_x < ((_r).x + (_r).width)) &&  \
     (_y >= (_r).y) && (_y < ((_r).y + (_r).height)))

static LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
static void setupMutex(void);
static void InitializePhantomWindow();
static void InitializeWindowSystem();
static void FinalizeWindowSystem();
static void releaseBitmapDC(HDC hdcMem);
static void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop);
static HDC getBitmapDC(void *imageData);
static HPEN setPen(HDC hdc, int pixel, int dotted);
static void setUpOffsets(int fullscreen);
static void CreateBacklight(HDC hdc);
static void DrawMenuBarBorder(HDC myhdc);
static void drawEmulatorScreen(javacall_bool fullscreen);
       void CreateEmulatorWindow();
static void paintVerticalScroll(HDC hdc, int scrollPosition,
                                int scrollProportion);
static void invalidateLCDScreen(int x1, int y1, int x2, int y2);
static void RefreshScreenNormal(int x1, int y1, int x2, int y2);  
static void RefreshScreenRotate(int x1, int y1, int x2, int y2);
static int mapKey(WPARAM wParam, LPARAM lParam);

#ifdef SKINS_MENU_SUPPORTED
static HMENU buildSkinsMenu(void);
static void destroySkinsMenu(void);
#endif // SKINS_MENU_SUPPORTED

/* BackLight top bar position parameters */
#define BkliteTop_xposition     0
#define BkliteTop_yposition     113
#define BkliteTop_width         241
#define BkliteTop_height        18

/* BackLight bottom bar position parameters */
#define BkliteBottom_xposition  0
#define BkliteBottom_yposition  339
#define BkliteBottom_width      241
#define BkliteBottom_height     6

/* BackLight left bar position parameters */
#define BkliteLeft_xposition    0
#define BkliteLeft_yposition    131
#define BkliteLeft_width        30
#define BkliteLeft_height       208

/* BackLight right bar position parameters */
#define BkliteRight_xposition   210
#define BkliteRight_yposition   131
#define BkliteRight_width       31
#define BkliteRight_height      208

/* thread safety */
static int tlsId;

int    blackPixel;
int    whitePixel;
int    lightGrayPixel;
int    darkGrayPixel;

HBRUSH  darkGrayBrush;
HPEN    whitePen;
HPEN    darkGrayPen;
HBRUSH  whiteBrush;

HBRUSH  BACKGROUND_BRUSH, FOREGROUND_BRUSH;
HPEN    BACKGROUND_PEN, FOREGROUND_PEN;

/* This is logical LCDUI putpixel screen buffer. */
static struct {
    javacall_pixel* hdc;
    int width;
    int height;
} VRAM;

static javacall_bool initialized = JAVACALL_FALSE;
static javacall_bool inFullScreenMode;
/* static javacall_bool requestedFullScreenMode; */
/* static javacall_bool drawTrustedMIDletIcon; */
/* static javacall_bool bkliteImageCreated = JAVACALL_FALSE; */
/* static javacall_bool isBklite_on = JAVACALL_FALSE; */

static int backgroundColor = RGB(182, 182, 170); /* This a win32 color value */
static int foregroundColor = RGB(0,0,0); /* This a win32 color value */

static void* lastImage = (void*)0xfffffffe;

HWND hMainWindow    = NULL;
static HWND hPhantomWindow = NULL;

static HDC hMemDC = NULL;

static TEXTMETRIC    fixed_tm, tm;
static HFONT            fonts[3][3][8];

/* The bits of the Network Indicator images */
static HBITMAP          LED_on_Image;
static HBITMAP          LED_off_Image;

static HBITMAP          hPhoneBitmap;
static HBITMAP          topbar_Image;

/* The bits of the BackLight images */
static HBITMAP          bklite_Top_Image;
static HBITMAP          bklite_Bottom_Image;
static HBITMAP          bklite_Left_Image;
static HBITMAP          bklite_Right_Image;

static int topBarHeight;
static int bottomBarHeight;
static int paintHeight;
static int x_offset = X_SCREEN_OFFSET;
static int y_offset;

static javacall_bool reverse_orientation;

/* key definitons */
typedef struct _Rectangle {
    int x;
    int y;
    int width;
    int height;
} XRectangle;

typedef struct {
    javacall_key button;
    XRectangle bounds;
    char *name;
} WKey;

#define KEY_POWER  (JAVACALL_KEY_GAME_RIGHT - 100)
#define KEY_END    (JAVACALL_KEY_GAME_RIGHT - 101)
#define KEY_SEND   (JAVACALL_KEY_GAME_RIGHT - 102)

/**
 * Do not alter the sequence of this
 * without modifying the one in .cpp
 */
const static WKey Keys[] = {
#ifdef NO_POWER_BUTTON

    /*
     * Add -DNO_POWER_BUTTON to the Makefile if you want to disable
     * the power button during user testing.
     */
{KEY_POWER,    {-10, -10,  1,  1}, "POWER"},
#else
{KEY_POWER,    {160, 59, 24, 24}, "POWER"},
#endif

//#define USE_SWAP_SOFTBUTTON
#ifndef USE_SWAP_SOFTBUTTON /* !USE_SWAP_SOFTBUTTON */
{JAVACALL_KEY_SOFT1,    {78, 420, 40, 35}, "SOFT1"},//
{JAVACALL_KEY_SOFT2,    {241, 424, 40, 35}, "SOFT2"},//
#else /* USE_SWAP_SOFTBUTTON */
{JAVACALL_KEY_SOFT2,    {78, 420, 40, 35}, "SOFT2"},//
{JAVACALL_KEY_SOFT1,    {241, 424, 40, 35}, "SOFT1"},//
#endif

{JAVACALL_KEY_UP,       {169, 421, 24, 9}, "UP"},//
{JAVACALL_KEY_DOWN,     {169, 454, 24, 9}, "DOWN"},//
{JAVACALL_KEY_LEFT,     {132, 431, 9, 24}, "LEFT"},//
{JAVACALL_KEY_RIGHT,    {218, 431, 9, 24}, "RIGHT"},//
{JAVACALL_KEY_SELECT,   {162, 434, 39, 15}, "SELECT"},//

{JAVACALL_KEY_SEND,     {60, 454, 51, 31}, "SEND"},//
{KEY_END,               {253, 454, 51, 31}, "END"},//
{JAVACALL_KEY_CLEAR,    {150, 478, 60, 28}, "CLEAR"},//

{JAVACALL_KEY_1,        {64, 500, 60, 29}, "1"},//
{JAVACALL_KEY_2,        {146, 519, 70, 26}, "2"},//
{JAVACALL_KEY_3,        {237, 500, 60, 29}, "3"},//
{JAVACALL_KEY_4,        {66, 534, 60, 29}, "4"},//
{JAVACALL_KEY_5,        {146, 554, 70, 26}, "5"},//
{JAVACALL_KEY_6,        {233, 537, 60, 29}, "6"},//
{JAVACALL_KEY_7,        {68, 569, 60, 29}, "7"},//
{JAVACALL_KEY_8,        {146, 591, 70, 26}, "8"},//
{JAVACALL_KEY_9,        {234, 575, 60, 29}, "9"},//
{JAVACALL_KEY_ASTERISK, {73, 610, 60, 29}, "*"},//
{JAVACALL_KEY_0,        {146, 628, 70, 26}, "0"},//
{JAVACALL_KEY_POUND,    {228, 612, 60, 29}, "#"},//

};

/* global variables to record the midpScreen window inside the win32 main window */
XRectangle midpScreen_bounds;
/* global variables for whether pen are dragging */
javacall_bool penAreDragging = JAVACALL_FALSE;

/**
 * Initialize LCD API
 * Will be called once the Java platform is launched
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure
 */
javacall_result javacall_lcd_init(void) {
    if(!initialized) {
        /* set up the offsets for non-full screen mode */
        setUpOffsets(JAVACALL_FALSE);
        inFullScreenMode = JAVACALL_FALSE;
        penAreDragging = JAVACALL_FALSE;
        initialized = JAVACALL_TRUE;
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
    if(initialized) {
        /* Clean up thread local data */
        void* ptr = (void*) TlsGetValue(tlsId);

        if(ptr != NULL) {
            /* Must free TLS data before freeing the TLS ID */
            free(ptr);
            TlsFree(tlsId);
        }
    }
    
    if(VRAM.hdc != NULL) {
        free(VRAM.hdc);
        VRAM.hdc = NULL;
    }
    if(hPhantomWindow != NULL) {
        DestroyWindow(hPhantomWindow);
        hPhantomWindow = NULL;
    }
    if(hMemDC != NULL) {
        DeleteDC(hMemDC);
    }

    if(hMainWindow != NULL) {
        DestroyWindow(hMainWindow);
        hMainWindow = NULL;
    }

    initialized = JAVACALL_FALSE;

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
    if(JAVACALL_TRUE == initialized) {
        if(screenWidth) {
            *screenWidth = javacall_lcd_get_screen_width();
        }
        if(screenHeight) {
            *screenHeight = javacall_lcd_get_screen_height();
        }
        if(colorEncoding) {
            *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
        }

        if(inFullScreenMode || reverse_orientation) {
            return VRAM.hdc;
        } else {			
            return VRAM.hdc + javacall_lcd_get_screen_width()*TOP_BAR_HEIGHT;
        }
    }

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

    inFullScreenMode = useFullScreen;
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
    if (reverse_orientation) { 
        RefreshScreenRotate(0, 0, DISPLAY_HEIGHT, DISPLAY_WIDTH); 
    } else { 
        RefreshScreenNormal(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT); 
    } 
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
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int ystart,
                                                        int yend) {

    //RefreshScreen(0,ystart, DISPLAY_WIDTH, yend);
    if (reverse_orientation) {         
         RefreshScreenRotate(0,0, DISPLAY_HEIGHT, DISPLAY_WIDTH); 
    } else { 
         RefreshScreenNormal(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT); 
    }

    return JAVACALL_OK;
}

HWND midpGetWindowHandle() {
    if(hMainWindow == NULL) {
        if(hPhantomWindow == NULL) {
            InitializePhantomWindow();
        }
        return hPhantomWindow;
    }
    return hMainWindow;
}

static void InitializePhantomWindow() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    char szAppName[] = "no_window";
    WNDCLASSEX  wndclass;
    HWND hwnd;

    wndclass.cbSize        = sizeof (wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm       = NULL;

    RegisterClassEx (&wndclass);

    hwnd = CreateWindow (szAppName,               /* window class name       */
                         "MIDP",                  /* window caption          */
                         WS_DISABLED,             /* window style; disable   */
                         CW_USEDEFAULT,           /* initial x position      */
                         CW_USEDEFAULT,           /* initial y position      */
                         0,                       /* initial x size          */
                         0,                       /* initial y size          */
                         NULL,                    /* parent window handle    */
                         NULL,                    /* window menu handle      */
                         hInstance,               /* program instance handle */
                         NULL);                   /* creation parameters     */

    hPhantomWindow = hwnd;
}

static void setUpOffsets(int fullscreen) {
    switch(fullscreen) {
    case 1:
        topBarHeight    = 0; // full screen mode includes the top bar.
        bottomBarHeight = 0;
        break;
    case 0:
        topBarHeight    = TOP_BAR_HEIGHT;
        bottomBarHeight = BOTTOM_BAR_HEIGHT;
        break;
    }

    paintHeight = (DISPLAY_HEIGHT - (topBarHeight + bottomBarHeight));

    if (reverse_orientation) {
        y_offset = Y_SCREEN_OFFSET;
    } else {
        y_offset = Y_SCREEN_OFFSET + topBarHeight;
    }
    
}

/**
 *
 */
int handleNetworkStreamEvents(WPARAM wParam,LPARAM lParam) {
    switch(WSAGETSELECTEVENT(lParam)) {
    case FD_CONNECT:
            /* Change this to a write. */
        javanotify_socket_event(
                               JAVACALL_EVENT_SOCKET_OPEN_COMPLETED,
                               (javacall_handle)wParam,
                               (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "FD_CONNECT)\n");
#endif
        return 0;
    case FD_WRITE:
        javanotify_socket_event(
                               JAVACALL_EVENT_SOCKET_SEND,
                               (javacall_handle)wParam,
                               (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "FD_WRITE)\n");
#endif
        return 0;

    case FD_ACCEPT:
        {
            //accept
            SOCKET clientfd = 0;
            javanotify_server_socket_event(
                                          JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED,
                                          (javacall_handle)wParam,
                                          (javacall_handle)clientfd,
                                          (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "FD_ACCEPT, ");
#endif
        }
    case FD_READ:
        javanotify_socket_event(
                               JAVACALL_EVENT_SOCKET_RECEIVE,
                               (javacall_handle)wParam,
                               (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "FD_READ)\n");
#endif
        return 0;

    case FD_CLOSE:
        javanotify_socket_event(
                               JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED,
                               (javacall_handle)wParam,
                               (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "FD_CLOSE)\n");
#endif
        return 0;

    default:
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "unsolicited event %d)\n",
                WSAGETSELECTEVENT(lParam));
#endif
        break;
    }
    return 0;
}

extern javacall_result try_process_wma_emulator(javacall_handle handle);

/**
 *
 */
int handleNetworkDatagramEvents(WPARAM wParam,LPARAM lParam) {
    switch(WSAGETSELECTEVENT(lParam)) {

    case FD_WRITE:
        javanotify_datagram_event(
                                 JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED,
                                 (javacall_handle)wParam,
                                 (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "[UDP] FD_WRITE)\n");
#endif
        return 0;
    case FD_READ:
#ifdef ENABLE_JSR_120
        if (JAVACALL_FALSE != try_process_wma_emulator((javacall_handle)wParam)) {
            return 0;
        }
#endif

        javanotify_datagram_event(
                                 JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED,
                                 (javacall_handle)wParam,
                                 (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "[UDP] FD_READ)\n");
#endif
        return 0;
    case FD_CLOSE:
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "[UDP] FD_CLOSE)\n");
#endif
        return 0;
    default:
#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "[UDP] unsolicited event %d)\n",
                WSAGETSELECTEVENT(lParam));
#endif
        break;
    }//end switch
    return 0;
}


/**
 *
 */
static LRESULT CALLBACK
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    static int penDown = JAVACALL_FALSE;
    int x, y;
    int i;
    PAINTSTRUCT ps;
    HDC hdc;
    HANDLE  *mutex;
    int key;
    //check if udp or tcp
    int level = SOL_SOCKET;
    int opttarget;
    int optname;
    int optsize = sizeof(optname);

    switch(iMsg) {

#ifdef SKINS_MENU_SUPPORTED
    case WM_COMMAND:

        switch(wParam & 0xFFFF) {
        case EXMENU_ITEM_SHUTDOWN:
            printf("EXMENU_ITEM_SHUTDOWN ...  \n");
            javanotify_shutdown();
            break;

        case EXMENU_ITEM_PAUSE:
            javanotify_pause();
            break;

        case EXMENU_ITEM_RESUME:
            javanotify_resume();
            break;

        default:
            return DefWindowProc (hwnd, iMsg, wParam, lParam);
        } /* end of switch */
        return 0;
#endif

    case WM_CREATE:
        hdc = GetDC(hwnd);
        CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT)));
        GetTextMetrics(hdc, &fixed_tm);
        CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
        GetTextMetrics(hdc, &tm);
        ReleaseDC(hwnd, hdc);
        return 0;

    case WM_PAINT:
        mutex = (HANDLE*) TlsGetValue(tlsId);
        WaitForSingleObject(*mutex, INFINITE);

        hdc = BeginPaint(hwnd, &ps);

        /*
         * There are 3 bitmap buffers, putpixel screen buffer, the phone
         * bitmap and the actual window buffer. Paint the phone bitmap on the
         * window. LCDUIrefresh has already painted the putpixel buffer on the
         * LCD screen area of the phone bitmap.
         *
         * On a real win32 (or high level window API) device the phone bitmap
         * would not be needed the code below would just paint the putpixel
         * buffer the window.
         */
        DrawBitmap(hdc, hPhoneBitmap, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);

        ReleaseMutex(*mutex);
        return 0;

    case WM_CLOSE:
        /*
         * Handle the "X" (close window) button by sending the AMS a shutdown
         * event.
         */
#ifdef SKINS_MENU_SUPPORTED
        destroySkinsMenu();
#endif
        javanotify_shutdown();
        PostQuitMessage(0);
        return 0;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        /* The only event of this type that we want MIDP
         * to handle is the F10 key which for some reason is not
         * sent in the WM_KEY messages.
         * if we receive any other key in this message, break.
         */
        if(wParam != VK_F10) {
            break;
        }
        // patch for VK_F10
        if (WM_SYSKEYUP == iMsg) {
            iMsg = WM_KEYUP;
        } else if (WM_SYSKEYDOWN == iMsg) {
            iMsg = WM_KEYDOWN;
        }
        /* fall through */
    case WM_KEYDOWN:
        {
        /* Impl note: to send pause and resume notifications */
            static int isPaused;
            if(VK_F5 == wParam) {
                if(!isPaused) {
                    javanotify_pause();
                } else {
                    javanotify_resume();
                } 
                isPaused =!isPaused;
                break;
            } else if(VK_HOME == wParam) {
                javanotify_switch_to_ams();
                break;
            } else if(VK_F4 == wParam) {
                javanotify_select_foreground_app();
                break;
            /* F3 key used for rotation. */ 
            } else if(VK_F3 == wParam) {                 
                    javanotify_rotation();
            }
        }
    case WM_KEYUP:
        key = mapKey(wParam, lParam);

        switch(key) {
        case /* MIDP_KEY_INVALID */ 0:
            return 0;

           /*
           case MD_KEY_HOME:
               if (iMsg == WM_KEYDOWN) {
               return 0;
           }

           pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
           pSignalResult->waitingFor = AMS_SIGNAL;
           return 0;
           */
        default:
            break;
        }

        if(iMsg == WM_KEYUP) {
            javanotify_key_event((javacall_key)key, JAVACALL_KEYRELEASED);
        } else if(iMsg == WM_KEYDOWN) {
            if (lParam & 0xf0000000) {
                javanotify_key_event((javacall_key)key, JAVACALL_KEYREPEATED);
            } else {
                javanotify_key_event((javacall_key)key, JAVACALL_KEYPRESSED);
            }
        }

        return 0;

    case WM_TIMER:
        // Stop the timer from repeating the WM_TIMER message.
        KillTimer(hwnd, wParam);

        return 0;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        /* Cast lParam to "short" to preserve sign */
        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

/** please uncomment following line if you want to test pen input in win32 emulator.
 **  with ENABLE_PEN_EVENT_NOTIFICATION defined, you should also modify constants.xml
 **  in midp workspace in order to pass related tck cases.
 **/
#define ENABLE_PEN_EVENT_NOTIFICATION 1    
#ifdef ENABLE_PEN_EVENT_NOTIFICATION        
        midpScreen_bounds.x = x_offset;
        midpScreen_bounds.y = y_offset;
        midpScreen_bounds.width = VRAM.width;
        midpScreen_bounds.height = (inFullScreenMode? VRAM.height: VRAM.height - TOP_BAR_HEIGHT);

        if(iMsg == WM_LBUTTONDOWN && INSIDE(x, y, midpScreen_bounds) ) {
            penAreDragging = JAVACALL_TRUE;
            SetCapture(hwnd);
            if (reverse_orientation) {
                javanotify_pen_event(javacall_lcd_get_screen_width() - y + y_offset, x - x_offset, JAVACALL_PENPRESSED);                                
            } else {
                javanotify_pen_event(x-x_offset, y-y_offset, JAVACALL_PENPRESSED);
            }
            return 0;
        }
        if(iMsg == WM_LBUTTONUP && (INSIDE(x, y, midpScreen_bounds) ||  penAreDragging == JAVACALL_TRUE)) {
            if(penAreDragging == JAVACALL_TRUE) {
                penAreDragging = JAVACALL_FALSE;
                ReleaseCapture();
            }
            if (reverse_orientation) {
                javanotify_pen_event(javacall_lcd_get_screen_width() - y + y_offset, x - x_offset, JAVACALL_PENRELEASED);                
            } else {
                javanotify_pen_event(x-x_offset, y-y_offset, JAVACALL_PENRELEASED);
            }
            return 0;
        }
        if(iMsg == WM_MOUSEMOVE) {
            if(penAreDragging == JAVACALL_TRUE) {                
                if (reverse_orientation) {
                    javanotify_pen_event(javacall_lcd_get_screen_width() - y + y_offset, x - x_offset, JAVACALL_PENDRAGGED);                
                } else {
                    javanotify_pen_event(x-x_offset, y-y_offset, JAVACALL_PENDRAGGED);
                }
            }
            return 0;
        }
#else
        if(iMsg == WM_MOUSEMOVE) {
            /*
             * Don't process mouse move messages that are not over the LCD
             * screen of the skin.
             */
            return 0;
        }
#endif

        for(i = 0; i < NUMBEROF(Keys); ++i) {
            if(!(INSIDE(x, y, Keys[i].bounds))) {
                continue;
            }

            /* Chameleon processes keys on the way down. */
#ifdef SOUND_SUPPORTED
            if(iMsg == WM_LBUTTONDOWN) {
                MessageBeep(MB_OK);
            }
#endif
            switch(Keys[i].button) {
            case KEY_POWER:
                if(iMsg == WM_LBUTTONUP) {
                    return 0;
                }
                javanotify_shutdown();
                return 0;

            case KEY_END:
                if(iMsg == WM_LBUTTONUP) {
                    return 0;
                }
                    /* NEED REVISIT: destroy midlet instead of shutdown? */
                javanotify_shutdown();
                return 0;

            default:
                    /* Handle the simulated key events. */
                switch(iMsg) {
                case WM_LBUTTONDOWN:
                    javanotify_key_event((javacall_key)Keys[i].button, JAVACALL_KEYPRESSED);
                    return 0;

                case WM_LBUTTONUP:
                    javanotify_key_event((javacall_key)Keys[i].button, JAVACALL_KEYRELEASED);
                    return 0;

                default:
                    break;
                } /* switch iMsg */
            } /* switch key */
        } /* for */

        return 0;

    case WM_NETWORK:

#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "Got WM_NETWORK(");
        fprintf(stderr, "descriptor = %d, ", (int)wParam);
        fprintf(stderr, "status = %d, ", WSAGETSELECTERROR(lParam));
#endif
        optname = SO_TYPE;
        if(0 != getsockopt((SOCKET)wParam, SOL_SOCKET,  optname,(char*)&opttarget, &optsize)) {
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "getsocketopt error)\n");
#endif
        }

        if(opttarget == SOCK_STREAM) { // TCP socket

            return handleNetworkStreamEvents(wParam,lParam);

        } else {
            return handleNetworkDatagramEvents(wParam,lParam);
        };

        break;

    case WM_HOST_RESOLVED:
#ifdef ENABLE_TRACE_NETWORKING
        fprintf(stderr, "Got Windows event WM_HOST_RESOLVED \n");
#endif
        javanotify_socket_event(
                               JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED,
                               (javacall_handle)wParam,
                               (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
        return 0;

#if 0 /* media */
    case WM_DEBUGGER:
        pSignalResult->waitingFor = VM_DEBUG_SIGNAL;
        return 0;
    case WM_MEDIA:
        pSignalResult->waitingFor = MEDIA_EVENT_SIGNAL;
        pSignalResult->descriptor = (int)wParam;
        pSignalResult->pResult = (void*)lParam;
        return 0;
#endif /* media */
    default:
        break;
    } /* end of external switch */

    return DefWindowProc (hwnd, iMsg, wParam, lParam);

} /* end of WndProc */

/**
 *
 */


void getBitmapSize(HBITMAP img, int* width, int* height){
    BITMAPINFO bitmapInfo=        {{sizeof(BITMAPINFOHEADER)}};
    GetDIBits(GetDC(NULL),img,1,0,0,&bitmapInfo,DIB_RGB_COLORS);
    if (width!=NULL) {
        *width=bitmapInfo.bmiHeader.biWidth;
    }
    if (height!=NULL) {
        *height=bitmapInfo.bmiHeader.biHeight+28;
    }

}
HBITMAP loadBitmap(char* path, int* width, int* height){
    HBITMAP hBitmap;

    hBitmap = (HBITMAP) LoadImage (GetModuleHandle(NULL), path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (NULL == hBitmap) {
        printf("Cannot load background image from %s. Using default.\n",path);
        hBitmap = (HBITMAP) LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP_PHONE), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    }
    if (hBitmap == 0) {
        printf("Cannot load background image from resources.\n");
        return NULL;
    }
    getBitmapSize(hBitmap, width, height);
    return hBitmap;
}



static void setupMutex() {
    HANDLE  *mutex;

    tlsId = TlsAlloc();
    mutex = malloc(sizeof(HANDLE));
    if(mutex == NULL) {
        return;
    }

    TlsSetValue(tlsId, mutex);
    *mutex = CreateMutex(0, JAVACALL_FALSE, "hPhoneBitmapMutex");


}

/**
 * Initializes the screen back buffer
 */
static void initScreenBuffer(int w, int h) {
    VRAM.width = javacall_lcd_get_screen_width(); 
    VRAM.height = DISPLAY_HEIGHT;
    VRAM.hdc = (javacall_pixel*)malloc(w*h*sizeof(javacall_pixel));
    if(VRAM.hdc == NULL) {
        javacall_print("initScreenBuffer: VRAM allocation failed");
    }
}

/**
 * Create the menu
 */
#ifdef SKINS_MENU_SUPPORTED
static HMENU hMenuExtended = NULL;
static HMENU hMenuExtendedSub = NULL;

HMENU buildSkinsMenu(void) {
    BOOL ok;

    hMenuExtended = CreateMenu();
    hMenuExtendedSub = CreateMenu();

    /* Create Life cycle menu list */

    ok = InsertMenuA(hMenuExtendedSub, 0, MF_BYPOSITION,
                     EXMENU_ITEM_SHUTDOWN, EXMENU_TEXT_SHUTDOWN);
    ok = InsertMenuA(hMenuExtendedSub, 0, MF_BYPOSITION,
                     EXMENU_ITEM_RESUME, EXMENU_TEXT_RESUME);
    ok = InsertMenuA(hMenuExtendedSub, 0, MF_BYPOSITION,
                     EXMENU_ITEM_PAUSE, EXMENU_TEXT_PAUSE);

    /* Add Life Cycle menu */
    ok = InsertMenuA(hMenuExtended, 0,
                     MF_BYPOSITION | MF_POPUP,
                     (UINT) hMenuExtendedSub,
                     EXMENU_TEXT_MAIN);

    (void) ok;
    return hMenuExtended;
}

static void destroySkinsMenu(void) {
    if(hMenuExtendedSub) {
        DestroyMenu(hMenuExtendedSub);
        hMenuExtendedSub = NULL;
    }

    if(hMenuExtended) {
        DestroyMenu(hMenuExtended);
        hMenuExtended = NULL;
    }

}
#endif // SKINS_MENU_SUPPORTED

extern int _phonenum;
/**
 * Create Emulator Window
 */
void CreateEmulatorWindow() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    static char szAppName[] = "MIDP stack";
    WNDCLASSEX  wndclass ;
    HWND hwnd;
    HDC hdc;
    HMENU hMenu = NULL;
    static WORD graybits[] = {0xaaaa, 0x5555, 0xaaaa, 0x5555,
        0xaaaa, 0x5555, 0xaaaa, 0x5555};

    unsigned int width ;//REMREM = EMULATOR_WIDTH;
    unsigned int height; //REMREM = EMULATOR_HEIGHT;
    static char caption[32];

    hPhoneBitmap = loadBitmap("phone.bmp",&width,&height);
    printf("[CreateEmulatorWindow] Window size %dx%d\n",width, height);
    sprintf(caption, "+%d Sun Anycall", _phonenum);

    (void) javacall_lcd_init();
    
    wndclass.cbSize        = sizeof (wndclass) ;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH) BACKGROUND_BRUSH;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = szAppName ;
    wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION) ;

    RegisterClassEx (&wndclass) ;
#ifdef SKINS_MENU_SUPPORTED
    hMenu = buildSkinsMenu();

    if(hMenu != NULL) height += 24;
#endif

    hwnd = CreateWindow(szAppName,            /* window class name       */
                        caption,              /* window caption          */
                        WS_OVERLAPPEDWINDOW & /* window style; disable   */
                        (~WS_MAXIMIZEBOX),    /* the 'maximize' button   */
                        50,        /* initial x position      */
                        30,        /* initial y position      */
                        /* window made smaller to hide the external
                           screen part since it's not in use for now 
                        */
                        (width/2),                 /* initial x size          */
                        (height),               /* initial y size          */
                        NULL,                 /* parent window handle    */
                        hMenu,                /* window menu handle      */
                        hInstance,            /* program instance handle */
                        NULL);                /* creation parameters     */

    hMainWindow = hwnd;

    /* create back buffer from mutable image, include the bottom bar. */
    initScreenBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    /* colors chosen to match those used in topbar.h */
    whitePixel = 0xffffff;
    blackPixel = 0x000000;
    lightGrayPixel = RGB(182, 182, 170);
    darkGrayPixel = RGB(109, 109, 85);

    foregroundColor = blackPixel;
    backgroundColor = lightGrayPixel;

    /* brushes for borders and menu hilights */
    darkGrayBrush = CreateSolidBrush(darkGrayPixel);
    darkGrayPen = CreatePen(PS_SOLID, 1, darkGrayPixel);
    whiteBrush = CreateSolidBrush(whitePixel);
    whitePen = CreatePen(PS_SOLID, 1, whitePixel);

    BACKGROUND_BRUSH = CreateSolidBrush(backgroundColor);
    BACKGROUND_PEN   = CreatePen(PS_SOLID, 1, backgroundColor);
    FOREGROUND_BRUSH = CreateSolidBrush(foregroundColor);
    FOREGROUND_PEN   = CreatePen(PS_SOLID, 1, foregroundColor);

    hdc = GetDC(hwnd);
    setupMutex();
    ReleaseDC(hwnd, hdc);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
}

/**
 *
 */
static HDC getBitmapDC(void *imageData) {
    HDC hdc;
    HANDLE  *mutex = (HANDLE*) TlsGetValue(tlsId);

    if(mutex == NULL) {
        mutex = (HANDLE*)malloc(sizeof(HANDLE));
        if(mutex == NULL) {
            return NULL;
        }
        TlsSetValue(tlsId, mutex);
        *mutex = CreateMutex(0, JAVACALL_FALSE, "hPhoneBitmapMutex");
    }

    if(lastImage != imageData) {
        if(hMemDC != NULL) {
            DeleteDC(hMemDC);
        }

        hdc = GetDC(hMainWindow);
        hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hMainWindow, hdc);
        lastImage = imageData;
    }

    WaitForSingleObject(*mutex, INFINITE);

    if(imageData == NULL) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hMemDC, hPhoneBitmap));
        SetWindowOrgEx(hMemDC, -x_offset, -(Y_SCREEN_OFFSET), NULL);
    } else if(imageData == UNTRANSLATED_SCREEN_BITMAP) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hMemDC, hPhoneBitmap));
    } else {
        myBitmapStruct *bmp = (myBitmapStruct *)imageData;
        if(bmp->mutable) {
            getBitmapDCtmp = SelectObject(hMemDC, bmp->bitmap);
        }
    }

    return hMemDC;
}

/**
 *
 */
static void releaseBitmapDC(HDC hdcMem) {
    HANDLE  *mutex = (HANDLE*) TlsGetValue(tlsId);
    SelectObject(hdcMem, getBitmapDCtmp);
    getBitmapDCtmp = NULL;
    ReleaseMutex(*mutex);
}

/**
 *
 */
static void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop) {
    BITMAP bm;
    HDC hdcMem;
    POINT ptSize, ptOrg;
    HBITMAP tmp;

    ASSERT(hdc != NULL);
    ASSERT(hBitmap != NULL);
    hdcMem = CreateCompatibleDC(hdc);
    ASSERT(hdcMem != NULL);
    CHECK_RETURN(tmp = SelectObject(hdcMem, hBitmap));

    SetMapMode(hdcMem, GetMapMode(hdc));
    GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.x = 0;
    ptOrg.y = 0;

    DPtoLP(hdcMem, &ptOrg, 1);

    BitBlt(hdc, x, y, ptSize.x, ptSize.y,
           hdcMem, ptOrg.x, ptOrg.y, rop);

    SelectObject(hdcMem, tmp);
    DeleteDC(hdcMem);
}

/**
 *
 */
static void invalidateLCDScreen(int x1, int y1, int x2, int y2) {
    /*
    RECT r;

    if (x1 < x2) {
        r.left = x1 + x_offset;
        r.right = x2 + x_offset;
    } else {
        r.left = x2 + x_offset;
        r.right = x1 + x_offset;
    }
    if (y1 < y2) {
        r.top = y1 + y_offset;
        r.bottom = y2 + y_offset;
    } else {
        r.top = y2 + y_offset;
        r.bottom = y1 + y_offset;
    }

    InvalidateRect(hMainWindow, &r, JAVACALL_TRUE);
    */
    /* Invalidate entire screen */
    InvalidateRect(hMainWindow, NULL, JAVACALL_FALSE);

    if(hMemDC != NULL) {
        DeleteDC(hMemDC);
        lastImage = (void*)0xfffffffe;
        hMemDC = NULL;
    }
}

/**
 *
 */
static int mapKey(WPARAM wParam, LPARAM lParam) {
    char keyStates[256];
    WORD temp[2];

    switch(wParam) {
#ifndef USE_SWAP_SOFTBUTTON /* !USE_SWAP_SOFTBUTTON */
    case VK_F1:
        return JAVACALL_KEY_SOFT1;

    case VK_F2:
        return JAVACALL_KEY_SOFT2;
#else /* USE_SWAP_SOFTBUTTON */
    case VK_F1:
        return JAVACALL_KEY_SOFT2;

    case VK_F2:
        return JAVACALL_KEY_SOFT1;
#endif

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
//        return MD_KEY_HOME;

    default:
        break;
    }

    GetKeyboardState(keyStates);
    temp[0] = 0;
    temp[1] = 0;
    ToAscii((UINT)wParam, (UINT)lParam, keyStates, temp, (UINT)0);

    /* At this point only return printable characters. */
    if(temp[0] >= ' ' && temp[0] < 127) {
        return temp[0];
    }

    return 0;
}

/**
 *
 */
static void RefreshScreenNormal(int x1, int y1, int x2, int y2) {
    int x;
    int y;
    int width;
    int height;
    javacall_pixel* pixels = VRAM.hdc;
    int i;
    int j;
    javacall_pixel pixel;
    int r;
    int g;
    int b;
    int count;
    unsigned char *destBits;
    unsigned char *destPtr;

    HDC        hdcMem;
    HBITMAP    destHBmp;
    BITMAPINFO     bi;
    HGDIOBJ    oobj;
    HDC hdc;

    if(x1 < 0) {
        x1 = 0;
    }

    if(y1 < 0) {
        y1 = 0;
    }

    if(x2 <= x1 || y2 <= y1) {
        return;
    }

    if(x2 > VRAM.width) {
        x2 = VRAM.width;
    }

    if(y2 > VRAM.height) {
        y2 = VRAM.height;
    }

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

    hdc = getBitmapDC(NULL);

    hdcMem = CreateCompatibleDC(hdc);
  
    if(!inFullScreenMode) {
        unsigned char* raw_image = (unsigned char*)(_topbar_dib_data.info);
        for(count=(TOP_BAR_HEIGHT*DISPLAY_WIDTH-1); count >=0 ; count--) {
            unsigned int r,g,b;
            r=*raw_image++;
            g=*raw_image++;
            b=*raw_image++;
            VRAM.hdc[count] = RGB2PIXELTYPE(r,g,b);
        }
    }


    destHBmp = CreateDIBSection (hdcMem, &bi, DIB_RGB_COLORS, &destBits, NULL, 0);

    if(destBits != NULL) {
        oobj = SelectObject(hdcMem, destHBmp);
        SelectObject(hdcMem, oobj);


        for(j = 0; j < height; j++) {
            for(i = 0; i < width; i++) {
                pixel = pixels[((y + j) * VRAM.width) + x + i];
                r = GET_RED_FROM_PIXEL(pixel);
                g = GET_GREEN_FROM_PIXEL(pixel);
                b = GET_BLUE_FROM_PIXEL(pixel);

                destPtr = destBits + ((j * width + i) * sizeof (long));

                *destPtr++ = b; /* dest pixels seem to be in BGRA order */
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
    releaseBitmapDC(hdc);

    invalidateLCDScreen(x1, y1, x2, y2);
    UpdateWindow(hMainWindow);
}

/**
  * Utility function to request logical screen to be painted
  * to the physical screen when screen is in rotated mode. 
  * @param x1 top-left x coordinate of the area to refresh
  * @param y1 top-left y coordinate of the area to refresh
  * @param x2 bottom-right x coordinate of the area to refresh
  * @param y2 bottom-right y coordinate of the area to refresh
  */
  void RefreshScreenRotate(int x1, int y1, int x2, int y2) {
    int x;
    int y;
    int width;
    int height;    
    javacall_pixel* pixels = VRAM.hdc;
    javacall_pixel pixel;
    int r;
    int g;
    int b;
    unsigned char *destBits;
    unsigned char *destPtr;
    int count;
  
    HDC            hdcMem;
    HBITMAP        destHBmp;
    BITMAPINFO     bi;
    HGDIOBJ        oobj;
    HDC hdc;
      
    if (x1 < 0) {
        x1 = 0;
    }
 
    if (y1 < 0) {
        y1 = 0;
    }
  
    if (x2 <= x1 || y2 <= y1) {
        return;
    }
  
    if (x2 > VRAM.width) {
        x2 = VRAM.width;
    }
  
    if (y2 > VRAM.height) {
        y2 = VRAM.height;
    }    
  
    x = x1;
    y = y1;
    width = x2 - x1;
    height = y2 - y1;
  
    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         = height;
    bi.bmiHeader.biHeight        = -width;
    bi.bmiHeader.biPlanes        = 1;
    bi.bmiHeader.biBitCount      = sizeof (long) * 8;
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = width * height * sizeof (long);
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;
  
    hdc = getBitmapDC(NULL);
  
    hdcMem = CreateCompatibleDC(hdc);
 
      
  
    destHBmp = CreateDIBSection (hdcMem, &bi, DIB_RGB_COLORS, &destBits,
                                   NULL, 0);
  
    if (destBits != NULL) {
        oobj = SelectObject(hdcMem, destHBmp);
  
        SelectObject(hdcMem, oobj);
  
        destPtr = destBits;
 
        //pixels += (TOP_BAR_HEIGHT*DISPLAY_WIDTH-1) + x2-1 + y1 * javacall_lcd_get_screen_width();

		pixels +=  x2-1 + y1 * javacall_lcd_get_screen_width();
  
        for (x = x2; x > x1; x--) {
  
        int y;
 
        for (y = y1; y < y2; y++) {            
             r = GET_RED_FROM_PIXEL(*pixels);
             g = GET_GREEN_FROM_PIXEL(*pixels);
             b = GET_BLUE_FROM_PIXEL(*pixels);            
             *destPtr++ = b;
             *destPtr++ = g;
             *destPtr++ = r;            
             destPtr += sizeof(long) - 3*sizeof(*destPtr);
             pixels += javacall_lcd_get_screen_width();
        }
        pixels += -1 - height * javacall_lcd_get_screen_width();         
  
      }    
 
      SetDIBitsToDevice(hdc, y, javacall_lcd_get_screen_width() - width - x, height, width, 0, 0, 0,
                           width, destBits, &bi, DIB_RGB_COLORS);
 }
  
      DeleteObject(oobj);
      DeleteObject(destHBmp);
      DeleteDC(hdcMem);
      releaseBitmapDC(hdc);
  
      invalidateLCDScreen(x1, y1, x1 + height, y1 + width);
      UpdateWindow(hMainWindow);
  }
 
 
 
javacall_bool javacall_lcd_reverse_orientation() {
      reverse_orientation = !reverse_orientation;    
      if (reverse_orientation) {
        y_offset = Y_SCREEN_OFFSET;
      } else {
        y_offset = Y_SCREEN_OFFSET + topBarHeight;
      }
      VRAM.width = javacall_lcd_get_screen_width();
      VRAM.height = DISPLAY_HEIGHT;
      return reverse_orientation;
}
 
javacall_bool javacall_lcd_get_reverse_orientation() {
     return reverse_orientation;
}
  
int javacall_lcd_get_screen_height() {
     if (reverse_orientation) {
         return DISPLAY_WIDTH;
     } else {
         if(inFullScreenMode) {
          return DISPLAY_HEIGHT;
         } else {
          return DISPLAY_HEIGHT - TOP_BAR_HEIGHT;
         }
     }
}
  
int javacall_lcd_get_screen_width() {
     if (reverse_orientation) {
         return DISPLAY_HEIGHT;
     } else {
         return DISPLAY_WIDTH;
     }
}

