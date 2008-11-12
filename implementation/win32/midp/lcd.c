/*
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


#include "lcd.h"
#include "skins.h"
#include "local_defs.h"

#if ENABLE_ON_DEVICE_DEBUG
#include "javacall_odd.h"
#endif

#define UNTRANSLATED_SCREEN_BITMAP (void*)0xffffffff

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)printf( "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

#define ASSERT(expr) (expr) ? (void)0 : (void)printf( \
    "%s:%d: (%s)is NOT true\n", __FILE__, __LINE__, #expr)

#define MD_KEY_HOME (KEY_MACHINE_DEP)

extern void drawTopbarImage(void);

static HBITMAP getBitmapDCtmp = NULL;

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

void CreateEmulatorWindow(void);

static void invalidateLCDScreen(int x1, int y1, int x2, int y2);
static void RefreshScreen(int x1, int y1, int x2, int y2);  
static int mapKey(WPARAM wParam, LPARAM lParam);

#ifdef SKINS_MENU_SUPPORTED
static HMENU buildSkinsMenu(void);
static void destroySkinsMenu(void);
#endif // SKINS_MENU_SUPPORTED

static HMENU hMenuExtended = NULL;
static HMENU hMenuExtendedSub = NULL;

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
typedef struct {
    javacall_pixel* hdc;
    int width;
    int height;
} SBuffer;

static SBuffer VRAM = {NULL, 0, 0};

static javacall_bool initialized = JAVACALL_FALSE;
static javacall_bool inFullScreenMode;

static int backgroundColor = RGB(182, 182, 170); /* This a win32 color value */
static int foregroundColor = RGB(0,0,0); /* This a win32 color value */

static void* lastImage = (void*)0xfffffffe;

HWND hMainWindow    = NULL;
static HWND hPhantomWindow = NULL;

static HDC hMemDC = NULL;

static TEXTMETRIC    fixed_tm, tm;
static HFONT            fonts[3][3][8];

static HBITMAP          hPhoneBitmap;

static javacall_bool reverse_orientation;

/*
 IMPL NOTE: top bar at the moment is available only as raw data
 of fixed width & height and thus available only for displays
 with the same width. Scaleable top bar will be implemented and
 then this variable will become expared.
 */
extern int topBarWidth;
extern int topBarHeight;
static javacall_bool topBarOn = JAVACALL_TRUE;

/* current skin*/
static ESkin* currentSkin;// = VSkin;

#if ENABLE_ON_DEVICE_DEBUG
static const char pStartOddKeySequence[] = "#1*2";
static int posInSequence = 0;
#endif

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
        reverse_orientation = JAVACALL_FALSE;
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
        VRAM.height = 0;
        VRAM.width = 0;
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
 * @param hardwareId unique id of hardware screen
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
javacall_pixel* javacall_lcd_get_screen(int hardwareId,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding) {
  (void)hardwareId;
    if(JAVACALL_TRUE == initialized) {
        int yOffset = topBarOn ? topBarHeight : 0;
        if(screenWidth) {
            *screenWidth = VRAM.width;
        }
        if(screenHeight) {
            *screenHeight = VRAM.height - yOffset;
        }
        if(colorEncoding) {
            *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
        }

        return VRAM.hdc + yOffset * VRAM.width;
    }

    return NULL;
}

/**
 * Get top bar offscreen buffer
 *
 * @param screenWidth output paramter to hold width of top bar
 * @param screenHeight output paramter to hold height of top bar
 *
 * @return pointer to video ram mapped memory region of size
 *         ( screenWidth * screenHeight )
 *         or <code>NULL</code> in case of failure
 */
javacall_pixel* getTopbarBuffer(int* screenWidth, int* screenHeight) {

    if((JAVACALL_TRUE == initialized) && topBarOn) {
        if(screenWidth) {
            *screenWidth = VRAM.width;
        }
        if(screenHeight) {
            *screenHeight = topBarHeight;
        }

        return VRAM.hdc;
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
 * @param hardwareId unique id of hardware screen
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.

 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(int hardwareId, javacall_bool useFullScreen) {
  (void)hardwareId;
    inFullScreenMode = useFullScreen;
    /*
     At the moment we draw top bar only if
     display width and top bar width are equal
    */
    if(inFullScreenMode) {
        topBarOn = JAVACALL_FALSE;
    } else {
        topBarOn = (currentSkin->displayRect.width == topBarWidth) ?
            JAVACALL_TRUE : JAVACALL_FALSE;
        if (topBarOn) {
            drawTopbarImage();
        }
    }

    return JAVACALL_OK;
}

/**
 * Flush the screen raster to the display.
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 *
 * @param hardwareId unique id of hardware screen
 * @return <tt>1</tt> on success, <tt>0</tt> on failure or invalid screen
 */
javacall_result javacall_lcd_flush(int hardwareId) {
  (void)hardwareId;

    RefreshScreen(0, 0, currentSkin->displayRect.width, currentSkin->displayRect.height); 

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
 * @param hardwareId unique id of hardware screen
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int hardwareId, int ystart,
                                                        int yend) {
  (void)hardwareId;
    RefreshScreen(0, 0, currentSkin->displayRect.width, currentSkin->displayRect.height); 

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

#if ENABLE_JSR_120
extern javacall_result try_process_wma_emulator(javacall_handle handle);
#endif

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
#if ENABLE_JSR_120
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
    int midpX, midpY;

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
            } else if(VK_HOME == wParam ||
                      VK_F7 == wParam) {
                javanotify_switch_to_ams();
                break;
            } else if(VK_END == wParam ||
                      VK_F8 == wParam) {
                javanotify_shutdown();
                break;
            } else if(VK_F4 == wParam) {
                javanotify_select_foreground_app();
                break;
            /* F3 key used for rotation. */ 
            } else if(VK_F3 == wParam) {                 
                javanotify_rotation(javacall_lcd_get_current_hardwareId());
                break;
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
#if ENABLE_ON_DEVICE_DEBUG
            if (lParam & 0xf0000000) {
                /* ignore if the key is repeated */
                break;
            }

            /* assert(posInSequence == sizeof(pStartOddKeySequence) - 1) */
            if (pStartOddKeySequence[posInSequence] == key) {
                posInSequence++;
                if (posInSequence == sizeof(pStartOddKeySequence) - 1) {
                    posInSequence = 0;
                    javanotify_enable_odd();
                    break;
                }
            } else {
                posInSequence = 0;
            }
#endif
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
        midpScreen_bounds.x = currentSkin->displayRect.x;
        midpScreen_bounds.y = currentSkin->displayRect.y + 
            (topBarOn ? topBarHeight : 0);
        midpScreen_bounds.width = currentSkin->displayRect.width;
        midpScreen_bounds.height = currentSkin->displayRect.height - 
            (topBarOn ? topBarHeight : 0);
        /* coordinates of event in MIDP Screen coordinate system */
        midpX = x - currentSkin->displayRect.x;
        midpY = y - currentSkin->displayRect.y - (topBarOn ? topBarHeight : 0);
        
        if(iMsg == WM_LBUTTONDOWN && INSIDE(x, y, midpScreen_bounds) ) {
            penAreDragging = JAVACALL_TRUE;
            SetCapture(hwnd);
            javanotify_pen_event(midpX, midpY, JAVACALL_PENPRESSED);                                
            return 0;
        }
        if(iMsg == WM_LBUTTONUP && (INSIDE(x, y, midpScreen_bounds) ||  penAreDragging == JAVACALL_TRUE)) {
            if(penAreDragging == JAVACALL_TRUE) {
                penAreDragging = JAVACALL_FALSE;
                ReleaseCapture();
            }
            javanotify_pen_event(midpX, midpY, JAVACALL_PENRELEASED);                
            return 0;
        }
        if(iMsg == WM_MOUSEMOVE) {
            if(penAreDragging == JAVACALL_TRUE) {                
                javanotify_pen_event(midpX, midpY, JAVACALL_PENDRAGGED);
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

        for(i = 0; i < currentSkin->keyCnt; ++i) {
            if(!(INSIDE(x, y, currentSkin->Keys[i].bounds))) {
                continue;
            }

            /* Chameleon processes keys on the way down. */
#ifdef SOUND_SUPPORTED
            if(iMsg == WM_LBUTTONDOWN) {
                MessageBeep(MB_OK);
            }
#endif
            switch(currentSkin->Keys[i].button) {
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

            case KEY_ROTATE:
                if(iMsg == WM_LBUTTONUP) {
                    return 0;
                }
                javanotify_rotation(javacall_lcd_get_current_hardwareId());
                return 0;

            case KEY_HOME:
                if(iMsg == WM_LBUTTONUP) {
                    return 0;
                }
                javanotify_switch_to_ams();
                return 0;

            case KEY_SELECTAPP:
                if(iMsg == WM_LBUTTONUP) {
                    return 0;
                }
                javanotify_select_foreground_app();
                return 0;

            default:
                    /* Handle the simulated key events. */
                switch(iMsg) {
                case WM_LBUTTONDOWN:
                    javanotify_key_event((javacall_key)currentSkin->Keys[i].button, JAVACALL_KEYPRESSED);
                    return 0;

                case WM_LBUTTONUP:
                    javanotify_key_event((javacall_key)currentSkin->Keys[i].button, JAVACALL_KEYRELEASED);
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
        *height=bitmapInfo.bmiHeader.biHeight;
    }

}

/**
 * Loads bitmap for specified skin if necessary and fetches
 * bitmap dimentions.
 */
HBITMAP loadBitmap(ESkin* skin, int* width, int* height) {

    if (NULL == skin) {
        return NULL;
    }

    if (NULL == skin->hBitmap) {
        skin->hBitmap = (HBITMAP) LoadImage (GetModuleHandle(NULL), MAKEINTRESOURCE(skin->resourceID), 
            IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
        if (skin->hBitmap == 0) {
            printf("Cannot load skin image from resources.\n");
            return NULL;
        }
    }
    getBitmapSize(skin->hBitmap, width, height);
 
    return skin->hBitmap;
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
 * Resizes the screen back buffer
 */
static void resizeScreenBuffer(int w, int h) {
    if(VRAM.hdc != NULL) {
        if (VRAM.width * VRAM.height != w * h) {
            free(VRAM.hdc);
            VRAM.hdc = NULL;
        }
    }
    if(VRAM.hdc == NULL) {
        VRAM.hdc = (javacall_pixel*)malloc(w*h*sizeof(javacall_pixel));
    }
    if(VRAM.hdc == NULL) {
        javacall_print("resizeScreenBuffer: VRAM allocation failed");
    }

    VRAM.width = w; 
    VRAM.height = h;
}

/**
 * Set current skin
 */
static void setCurrentSkin(ESkin* newSkin) {
    if (NULL == newSkin) {
        printf("failed to change emulator skin\n");    
    }
    currentSkin = newSkin;

    if(inFullScreenMode) {
        topBarOn = JAVACALL_FALSE;
    } else {
        topBarOn = (currentSkin->displayRect.width == topBarWidth) ?
            JAVACALL_TRUE : JAVACALL_FALSE;
    }

    resizeScreenBuffer(currentSkin->displayRect.width, 
        currentSkin->displayRect.height);
    /* update skin image */
    {
        int w, h;
        RECT wr, r;
        GetWindowRect(hMainWindow, &wr);
        GetClientRect(hMainWindow, &r);
        hPhoneBitmap = loadBitmap(currentSkin, &w, &h);
        r.bottom = r.top + h;
        r.right = r.left + w;
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & (~WS_MAXIMIZEBOX), 
            NULL != hMenuExtended);
        if (hPhoneBitmap != NULL) {
            MoveWindow(hMainWindow, wr.left, wr.top, 
                r.right - r.left, r.bottom - r.top, TRUE);
        }
        if (topBarOn) {
            drawTopbarImage();
        }
    }
}


/**
 * Create the menu
 */
#ifdef SKINS_MENU_SUPPORTED

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

extern char* _phonenum;
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

    static char caption[32];
    
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
#endif
    sprintf(caption, "%s Sun Anycall", _phonenum);

    hwnd = CreateWindow(szAppName,            /* window class name       */
                        caption,              /* window caption          */
                        WS_OVERLAPPEDWINDOW & /* window style; disable   */
                        (~WS_MAXIMIZEBOX),    /* the 'maximize' button   */
                        50,        /* initial x position      */
                        30,        /* initial y position      */
                        0,                 /* initial x size          */
                        0,               /* initial y size          */
                        NULL,                 /* parent window handle    */
                        hMenu,                /* window menu handle      */
                        hInstance,            /* program instance handle */
                        NULL);                /* creation parameters     */

    hMainWindow = hwnd;

    setCurrentSkin(&VSkin);

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
        SetWindowOrgEx(hMemDC, -currentSkin->displayRect.x, -currentSkin->displayRect.y, NULL);
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
    if(temp[0] >= ' ' && temp[0] < 127) {
        return temp[0];
    }

    return 0;
}

/**
 * Utility function to request logical screen to be painted
 * to the physical screen.
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
static void RefreshScreen(int x1, int y1, int x2, int y2) {
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
    BITMAPINFO     bi;
    HGDIOBJ    oobj;
    HDC hdc;

    int screenWidth = VRAM.width;
    int screenHeight = VRAM.height;
    javacall_pixel* screenBuffer = VRAM.hdc;

    if(x1 < 0) {
        x1 = 0;
    }

    if(y1 < 0) {
        y1 = 0;
    }

    if(x2 <= x1 || y2 <= y1) {
        return;
    }

    if(x2 > screenWidth) {
        x2 = screenWidth;
    }

    if(y2 > screenHeight) {
        y2 = screenHeight;
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

    destHBmp = CreateDIBSection (hdcMem, &bi, DIB_RGB_COLORS, &destBits, NULL, 0);


    if(destBits != NULL) {
        oobj = SelectObject(hdcMem, destHBmp);
        SelectObject(hdcMem, oobj);

        for(j = 0; j < height; j++) {
            for(i = 0; i < width; i++) {
                pixel = screenBuffer[((y + j) *screenWidth) + x + i];
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
    releaseBitmapDC(hdc);

    invalidateLCDScreen(x1, y1, x2, y2);
    UpdateWindow(hMainWindow);
}

 
/**
 * Changes display orientation
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_reverse_orientation(int hardwareId) {
  (void)hardwareId;
    reverse_orientation = !reverse_orientation;    
    if (reverse_orientation) {
        setCurrentSkin(&HSkin);
    } else {
        setCurrentSkin(&VSkin);
    }
    return reverse_orientation;
}

 
/**
 * Handles clamshell event. 
 */
void javacall_lcd_handle_clamshell() {
	/*Not implemented*/
}
 
/**
 * Returns display orientation
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_get_reverse_orientation(int hardwareId) {
  (void)hardwareId;
     return reverse_orientation;
}

/**
 * checks the implementation supports native softbutton label.
 * 
 * @retval JAVACALL_TRUE   implementation supports native softbutton layer
 * @retval JAVACALL_FALSE  implementation does not support native softbutton layer
 */
javacall_bool javacall_lcd_is_native_softbutton_layer_supported () {
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
javacall_result javacall_lcd_set_native_softbutton_label(const javacall_utf16* label,
                                                         int len,
                                                         int index){
     return JAVACALL_FAIL;
}

/**
 * Returns available display width
 * @param hardwareId unique id of hardware screen
 */
int javacall_lcd_get_screen_width(int hardwareId) {
  (void)hardwareId;
    return currentSkin->displayRect.width;
}

/**
 * Returns available display height
 * @param hardwareId unique id of hardware screen
 */
int javacall_lcd_get_screen_height(int hardwareId) {
  (void)hardwareId;
    return topBarOn ? (currentSkin->displayRect.height - topBarHeight) : 
        currentSkin->displayRect.height;
}

/**
 * get currently enabled hardware display id
 */
int javacall_lcd_get_current_hardwareId() {
  return 0;
}
/** 
 * Get display device name by id
 * @param hardwareId unique id of hardware screen
 */
char* javacall_lcd_get_display_name(int hardwareId) {
  (void)hardwareId;
  return NULL;
}


/**
 * Check if the display device is primary
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_primary(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_TRUE;
}

/**
 * Check if the display device is build-in
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_buildin(int hardwareId) {
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Check if the display device supports pointer events
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_pen_supported(int hardwareId) {
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Check if the display device supports pointer motion  events
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_pen_motion_supported(int hardwareId){
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Get display device capabilities
 * @param hardwareId unique id of hardware screen
 */
int javacall_lcd_get_display_capabilities(int hardwareId) {
  return 255;
}

static int screen_ids[] =
{
  0
};

/**
 * Get the list of screen ids
 * @param return number of screens 
 * @return the lit of ids 
 */
int* javacall_lcd_get_display_device_ids(int* n) {
  *n = 1;
    return screen_ids;
}
