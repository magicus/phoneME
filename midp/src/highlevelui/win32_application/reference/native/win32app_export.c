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
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

#include <stdio.h>
#include <windows.h>
#include <ctype.h>
#include <math.h>

#include <kni.h>

#include <midp_logging.h>
#include <win32app_export.h>
#include <gx_graphics.h>
#include <midpServices.h>
#include <midp_properties_port.h>
#include <midp_constants_data.h>
#include <keymap_input.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <anc_indicators.h>
#include <gxj_putpixel.h>
#include <midp_foreground_id.h>

#include "staticGraphics.h"
#include "midpStubsKeymapping.h"

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))

/*
 * This (x,y) coordinate pair refers to the offset of the upper
 * left corner of the display screen within the MIDP phone handset
 * graphic window
 */
#define X_SCREEN_OFFSET 30
#define Y_SCREEN_OFFSET 131

#define EMULATOR_WIDTH  (241 + 8)
#define EMULATOR_HEIGHT (635 + 24)
#define TOP_BAR_HEIGHT  11

/*
 * Defines Java code paintable region
 */
#define DISPLAY_WIDTH  	CHAM_WIDTH
#define DISPLAY_HEIGHT 	CHAM_FULLHEIGHT
#define DISPLAY_X	X_SCREEN_OFFSET
#define DISPLAY_Y	(Y_SCREEN_OFFSET + TOP_BAR_HEIGHT)

#define UNTRANSLATED_SCREEN_BITMAP (void*)0xffffffff

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)fprintf(stderr, "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

#define ASSERT(expr) (expr) ? (void)0 : (void)fprintf(stderr, \
    "%s:%d: (%s)is NOT true\n", __FILE__, __LINE__, #expr)

#define MD_KEY_HOME (KEY_MACHINE_DEP)

static HBITMAP getBitmapDCtmp = NULL;

typedef unsigned short unicode;

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
#define LED_xposition 17
#define LED_yposition 82
#define LED_width     20
#define LED_height    20

#define INSIDE(_x, _y, _r)                              \
    ((_x >= (_r).x) && (_x < ((_r).x + (_r).width)) &&  \
     (_y >= (_r).y) && (_y < ((_r).y + (_r).height)))

static LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

static void releaseBitmapDC(HDC hdcMem);
static void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop);
static HDC getBitmapDC(void *imageData);
static HPEN setPen(HDC hdc, int pixel, int dotted);
static void CreateBacklight(HDC hdc);
static void CreateEmulatorWindow();
static void invalidateLCDScreen(int x1, int y1, int x2, int y2);

/* BackLight top bar position parameters */
#define BkliteTop_xposition 0
#define BkliteTop_yposition 113
#define BkliteTop_width     241
#define BkliteTop_height    18

/* BackLight bottom bar position parameters */
#define BkliteBottom_xposition 0
#define BkliteBottom_yposition 339
#define BkliteBottom_width      241
#define BkliteBottom_height    6

/* BackLight left bar position parameters */
#define BkliteLeft_xposition 0
#define BkliteLeft_yposition 131
#define BkliteLeft_width        30
#define BkliteLeft_height    208

/* BackLight right bar position parameters */
#define BkliteRight_xposition 210
#define BkliteRight_yposition 131
#define BkliteRight_width       31
#define BkliteRight_height    208

int    blackPixel;
int    whitePixel;
int    lightGrayPixel;
int    darkGrayPixel;

HBRUSH          darkGrayBrush;
HPEN            whitePen;
HPEN            darkGrayPen;
HBRUSH          whiteBrush;

HBRUSH          BACKGROUND_BRUSH, FOREGROUND_BRUSH;
HPEN            BACKGROUND_PEN, FOREGROUND_PEN;

/* This is logical LCDUI putpixel screen buffer. */
gxj_screen_buffer gxj_system_screen_buffer;

static jboolean initialized = KNI_FALSE;
static jboolean inFullScreenMode;
static jboolean bkliteImageCreated = KNI_FALSE;
static jboolean isBklite_on = KNI_FALSE;


static int backgroundColor = RGB(182, 182, 170); /* This a win32 color value */
static int foregroundColor = RGB(0,0,0); /* This a win32 color value */

static void* lastImage = (void*)0xfffffffe;

static HWND hMainWindow    = NULL;

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

#define INVALIDATETOPBAR()                                 \
{                                                          \
  RECT r;                                                  \
  r.left   = DISPLAY_X;                                     \
  r.top    = Y_SCREEN_OFFSET;                              \
  r.right  = DISPLAY_X + DISPLAY_WIDTH;                     \
  r.bottom = DISPLAY_Y;                                     \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

/* refresh the Top backlight bar */
#define INVALIDATE_BACKLIGHT_TOPBAR()                      \
{                                                      \
  RECT r;                                              \
  r.left   = BkliteTop_xposition;                       \
  r.top    = BkliteTop_yposition;                       \
  r.right  = BkliteTop_xposition + BkliteTop_width;    \
  r.bottom = BkliteTop_yposition + BkliteTop_height;   \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);            \
}

/* refresh the bottom backlight bar */
#define INVALIDATE_BACKLIGHT_BOTTOMBAR()                   \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteBottom_xposition;                       \
  r.top    = BkliteBottom_yposition;                       \
  r.right  = BkliteBottom_xposition + BkliteBottom_width;  \
  r.bottom = BkliteBottom_yposition + BkliteBottom_height; \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

/* refresh the left backlight bar */
#define INVALIDATE_BACKLIGHT_LEFTBAR()                                       \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteLeft_xposition;                         \
  r.top    = BkliteLeft_yposition;                         \
  r.right  = BkliteLeft_xposition + BkliteLeft_width;      \
  r.bottom = BkliteLeft_yposition + BkliteLeft_height;     \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

/* refresh the right backlight bar */
#define INVALIDATE_BACKLIGHT_RIGHTBAR()                                       \
{                                                          \
  RECT r;                                                  \
  r.left   = BkliteRight_xposition;                        \
  r.top    = BkliteRight_yposition;                        \
  r.right  = BkliteRight_xposition + BkliteRight_width;    \
  r.bottom = BkliteRight_yposition + BkliteRight_height;   \
  InvalidateRect(hMainWindow, &r, KNI_TRUE);               \
}

void win32app_init() {
    inFullScreenMode = KNI_FALSE;

    CreateEmulatorWindow();
}

/**
 * Bridge function to request logical screen to be painted
 * to the physical screen.
 * <p>
 * On win32 there are 3 bitmap buffers, putpixel screen buffer, the phone
 * bitmap that includes an LCD screen area, and the actual window buffer.
 * Paint the screen buffer on the LCD screen area of the phone bitmap.
 * On a real win32 (or high level window API) device the phone bitmap would
 * not be needed and this function would just invalidate the window and
 * when the system call back to paint the window, then the putpixel buffer
 * would be painted to the window.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void win32app_refresh(int x1, int y1, int x2, int y2) {
    int x;
    int y;
    int width;
    int height;
    gxj_pixel_type* pixels = gxj_system_screen_buffer.pixelData;
    int i;
    int j;
    gxj_pixel_type pixel;
    int r;
    int g;
    int b;
    unsigned char *destBits;
    unsigned char *destPtr;

    HDC		   hdcMem;
    HBITMAP	   destHBmp;
    BITMAPINFO	   bi;
    HGDIOBJ	   oobj;
    HDC hdc;

    REPORT_CALL_TRACE4(LC_HIGHUI,
                       "LF:STUB:win32app_refresh(%3d, %3d, %3d, %3d )\n",
                       x1, y1, x2, y2);

    if (x1 < 0) {
        x1 = 0;
    }

    if (y1 < 0) {
        y1 = 0;
    }

    if (x2 <= x1 || y2 <= y1) {
        return;
    }

    if (x2 > gxj_system_screen_buffer.width) {
        x2 = gxj_system_screen_buffer.width;
    }

    if (y2 > gxj_system_screen_buffer.height) {
        y2 = gxj_system_screen_buffer.height;
    }

    x = x1;
    y = y1;
    width = x2 - x1;
    height = y2 - y1;

    bi.bmiHeader.biSize		 = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth	 = width;
    bi.bmiHeader.biHeight	 = -height;
    bi.bmiHeader.biPlanes	 = 1;
    bi.bmiHeader.biBitCount	 = sizeof (long) * 8;
    bi.bmiHeader.biCompression	 = BI_RGB;
    bi.bmiHeader.biSizeImage	 = width * height * sizeof (long);
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed	 = 0;
    bi.bmiHeader.biClrImportant	 = 0;
	
    hdc = getBitmapDC(NULL);

    hdcMem = CreateCompatibleDC(hdc);

    destHBmp = CreateDIBSection (hdcMem, &bi, DIB_RGB_COLORS, &destBits,
                                 NULL, 0);

    if (destBits != NULL) {
	oobj = SelectObject(hdcMem, destHBmp);

        SelectObject(hdcMem, oobj);

        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                pixel = pixels[((y + j) * gxj_system_screen_buffer.width) + x + i];
                r = GXJ_GET_RED_FROM_PIXEL(pixel);
                g = GXJ_GET_GREEN_FROM_PIXEL(pixel);
                b = GXJ_GET_BLUE_FROM_PIXEL(pixel);

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
}

void win32app_finalize() {

    if (hMainWindow != NULL) {
        DestroyWindow(hMainWindow);
        hMainWindow = NULL;
    }

    if (gxj_system_screen_buffer.pixelData != NULL) {
        midpFree(gxj_system_screen_buffer.pixelData);
        gxj_system_screen_buffer.pixelData = NULL;
    }

    if (hMemDC != NULL) {
        DeleteDC(hMemDC);
    }
}

HWND win32app_get_window_handle() {
    return hMainWindow;
}

/*
 * Draw BackLight.
 * If 'active' is KNI_TRUE, the BackLight is drawn.
 * If 'active' is KNI_FALSE, the BackLight is erased.
 */
jboolean drawBackLight(int mode) {

    HDC hdc = GetDC(hMainWindow);
    jboolean result = KNI_FALSE;

    if (mode == BACKLIGHT_IS_SUPPORTED) {
        result = KNI_TRUE;
    }
    else {
        CreateBacklight(hdc);

        if ((mode == BACKLIGHT_ON) || 
            (mode == BACKLIGHT_TOGGLE && isBklite_on == KNI_FALSE)) {
            isBklite_on = KNI_TRUE;
            DrawBitmap(hdc, bklite_Top_Image, BkliteTop_xposition,
                       BkliteTop_yposition, SRCCOPY);
            DrawBitmap(hdc, bklite_Bottom_Image, BkliteBottom_xposition,
                       BkliteBottom_yposition, SRCCOPY);
            DrawBitmap(hdc, bklite_Left_Image, BkliteLeft_xposition,
                       BkliteLeft_yposition, SRCCOPY);
            DrawBitmap(hdc, bklite_Right_Image, BkliteRight_xposition,
                       BkliteRight_yposition, SRCCOPY);
            result = KNI_TRUE;
        } else if ((mode == BACKLIGHT_OFF) ||
                  (mode == BACKLIGHT_TOGGLE && isBklite_on == KNI_TRUE)){
            if (isBklite_on) {
                isBklite_on = KNI_FALSE;
                INVALIDATE_BACKLIGHT_TOPBAR();
                INVALIDATE_BACKLIGHT_BOTTOMBAR();
                INVALIDATE_BACKLIGHT_LEFTBAR();
                INVALIDATE_BACKLIGHT_RIGHTBAR();
            }
            result = KNI_TRUE;
        }
    }

    ReleaseDC(hMainWindow, hdc);
    return result;
}

static void CreateBacklight(HDC hdc) {

    if (KNI_FALSE == bkliteImageCreated) {

        int cmapLen;
        BITMAPINFOHEADER* bkliteTopInfo = bkliteTop_dib;
        BITMAPINFOHEADER* bkliteBottomInfo = bkliteBottom_dib;
        BITMAPINFOHEADER* bkliteLeftInfo = bkliteLeft_dib;
        BITMAPINFOHEADER* bkliteRightInfo = bkliteRight_dib;

        /* Create top backlight bar */
        cmapLen = (bkliteTopInfo->biBitCount > 8) ? 0 :
        (bkliteTopInfo->biClrUsed ? bkliteTopInfo->biClrUsed :
         (1 << bkliteTopInfo->biBitCount));
        bklite_Top_Image = CreateDIBitmap(hdc, bkliteTopInfo, CBM_INIT,
                      ((char*)bkliteTopInfo) + bkliteTopInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteTopInfo,
                      DIB_RGB_COLORS);

        /* Create bottom backlight bar */
        cmapLen = (bkliteBottomInfo->biBitCount > 8) ? 0 :
        (bkliteBottomInfo->biClrUsed ? bkliteBottomInfo->biClrUsed :
         (1 << bkliteBottomInfo->biBitCount));
        bklite_Bottom_Image = CreateDIBitmap(hdc, bkliteBottomInfo, CBM_INIT,
                      ((char*)bkliteBottomInfo) + bkliteBottomInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteBottomInfo,
                      DIB_RGB_COLORS);

        /* Create left backlight bar */
        cmapLen = (bkliteLeftInfo->biBitCount > 8) ? 0 :
        (bkliteLeftInfo->biClrUsed ? bkliteLeftInfo->biClrUsed :
         (1 << bkliteLeftInfo->biBitCount));
        bklite_Left_Image = CreateDIBitmap(hdc, bkliteLeftInfo, CBM_INIT,
                      ((char*)bkliteLeftInfo) + bkliteLeftInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteLeftInfo,
                      DIB_RGB_COLORS);

        /* Create right backlight bar */
        cmapLen = (bkliteRightInfo->biBitCount > 8) ? 0 :
        (bkliteRightInfo->biClrUsed ? bkliteRightInfo->biClrUsed :
         (1 << bkliteRightInfo->biBitCount));
        bklite_Right_Image = CreateDIBitmap(hdc, bkliteRightInfo, CBM_INIT,
                      ((char*)bkliteRightInfo) + bkliteRightInfo->biSize
                      + 4 * cmapLen, (BITMAPINFO*)bkliteRightInfo,
                      DIB_RGB_COLORS);

        bkliteImageCreated = KNI_TRUE;

    }

}

static jint mapKey(WPARAM wParam, LPARAM lParam) {
    char keyStates[256];
    WORD temp[2];

    switch (wParam) {
    case VK_F1:
        return KEY_SOFT1;

    case VK_F2:
        return KEY_SOFT2;

    case VK_F9:
        return KEY_GAMEA;

    case VK_F10:
        return KEY_GAMEB;

    case VK_F11:
        return KEY_GAMEC;

    case VK_F12:
        return KEY_GAMED;
        break;

    case VK_UP:
        return KEY_UP;

    case VK_DOWN:
        return KEY_DOWN;

    case VK_LEFT:
        return KEY_LEFT;

    case VK_RIGHT:
        return KEY_RIGHT;
        
    /*
     * Map VK_SPACE here, but in the
     * high level Java code, we have to
     * test for special case since "space"
     * should be used for textbox's as space.
     */
    case VK_SPACE:
    case VK_RETURN:
        return KEY_SELECT;

    case VK_BACK:
        return KEY_BACKSPACE;

    case VK_HOME:
        return MD_KEY_HOME;
    
    default:
        break;
    }

    GetKeyboardState(keyStates);
    temp[0] = 0;
    temp[1] = 0;
    ToAscii((UINT)wParam, (UINT)lParam, keyStates, temp, (UINT)0);

    /* At this point only return printable characters. */
    if (temp[0] >= ' ' && temp[0] <= 127) {
        return temp[0];
    }

    return KEY_INVALID;
}

#if ENABLE_NATIVE_AMS
void nams_process_command(int command, int param);
#endif

static LRESULT CALLBACK
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    static int penDown = KNI_FALSE;
    static XRectangle SCREEN_BOUNDS = { 0,0,0,0 };

    int x, y;
    int i;
    PAINTSTRUCT ps;
    HDC hdc;
    int key;

    if (SCREEN_BOUNDS.width == 0) {
        SCREEN_BOUNDS.x = DISPLAY_X;
        SCREEN_BOUNDS.y = DISPLAY_Y;
        SCREEN_BOUNDS.width = DISPLAY_WIDTH;
        SCREEN_BOUNDS.height = DISPLAY_HEIGHT;
    }

    switch (iMsg) {
    case WM_CREATE:
        hdc = GetDC(hwnd);
        CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT)));
        GetTextMetrics(hdc, &fixed_tm);
        CHECK_RETURN(SelectObject(hdc, GetStockObject(SYSTEM_FONT)));
        GetTextMetrics(hdc, &tm);
        ReleaseDC(hwnd, hdc);
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        /*
         * There are 3 bitmap buffers, putpixel screen buffer, the phone
         * bitmap and the actual window buffer. Paint the phone bitmap on the
         * window. win32app_refresh has already painted the putpixel buffer on the
         * LCD screen area of the phone bitmap.
         *
         * On a real win32 (or high level window API) device the phone bitmap
         * would not be needed the code below would just paint the putpixel
         * buffer the window.
         */
        DrawBitmap(hdc, hPhoneBitmap, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;

    case WM_CLOSE:
        /*
         * Handle the "X" (close window) button by sending the AMS a shutdown
         * event.
         */
        PostQuitMessage (0);
        pMidpEventResult->type = SHUTDOWN_EVENT;
        pSignalResult->waitingFor = AMS_SIGNAL;
        return 0;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        /* The only event of this type that we want MIDP
         * to handle is the F10 key which for some reason is not
         * sent in the WM_KEY messages.
         * if we receive any other key in this message, break.
         */
        if (wParam != VK_F10) {
            break;
        }

        /* fall through */
    case WM_KEYDOWN:
    case WM_KEYUP:
        key = mapKey(wParam, lParam);

        switch (key) {
        case KEY_INVALID:
            return 0;

        case MD_KEY_HOME:
            if (iMsg == WM_KEYDOWN) {
                return 0;
            }

            pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
            pSignalResult->waitingFor = AMS_SIGNAL;
            return 0;

        default:
            break;
        }

        pMidpEventResult->type = MIDP_KEY_EVENT;
        pMidpEventResult->CHR = key;

        if (iMsg == WM_KEYUP) {
            pMidpEventResult->ACTION = RELEASED;
        } else if (lParam & 0x40000000) {
            pMidpEventResult->ACTION = REPEATED;
        } else {
            pMidpEventResult->ACTION = PRESSED;
        }

        pSignalResult->waitingFor = UI_SIGNAL;
           
        return 0;

    case WM_TIMER:
        // Stop the timer from repeating the WM_TIMER message.
        KillTimer(hwnd, wParam);

        if (wParam != EVENT_TIMER_ID) {
            // This is a push alarm
            pSignalResult->waitingFor = PUSH_ALARM_SIGNAL;
            pSignalResult->descriptor = (int)wParam;
        }

        return 0;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP: 
        /* Cast lParam to "short" to preserve sign */
        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

        if ((INSIDE(x, y, SCREEN_BOUNDS)) || (penDown)) {
            /* The max possible values for the X & Y coordinates */
            int maxX = DISPLAY_X + DISPLAY_WIDTH - 1;
            int maxY = DISPLAY_Y + DISPLAY_HEIGHT - 1;
 
            /*
             * Check to see if we have moved the mouse outside of
             * the screen bounds while holding the mouse button.
             * If so, we still need to continue to send the mouse
             * events to the MIDlet, but, with at least one
             * coordinate "pegged" to the edge of the screen.
             */
            pMidpEventResult->X_POS = min(x, maxX) - DISPLAY_X;
            if (pMidpEventResult->X_POS < 0) {
                pMidpEventResult->X_POS = 0;
            }

            pMidpEventResult->Y_POS = min(y, maxY) - DISPLAY_Y;
            if (pMidpEventResult->Y_POS < 0) {
                pMidpEventResult->Y_POS = 0;
            }

            pSignalResult->waitingFor = UI_SIGNAL;

            switch (iMsg) {
            case WM_LBUTTONDOWN:
                /*
                 * Capture the mouse to get a message when the left button is
                 * released out side of the MIDP window.
                 */
                SetCapture(hwnd);
                penDown = KNI_TRUE;
                pMidpEventResult->type = MIDP_PEN_EVENT;
                pMidpEventResult->ACTION = PRESSED;
                return 0;

            case WM_LBUTTONUP:
                ReleaseCapture();
                penDown = KNI_FALSE;
                pMidpEventResult->type = MIDP_PEN_EVENT;
                pMidpEventResult->ACTION = RELEASED;
                return 0;

            default: /* WM_MOUSEMOVE */
                pMidpEventResult->type = MIDP_PEN_EVENT;
                pMidpEventResult->ACTION = DRAGGED;
                return 0;
            }
        }

        if (iMsg == WM_MOUSEMOVE) {
            /*
             * Don't process mouse move messages that are not over the LCD
             * screen of the skin.
             */
            return 0;
        }

        for (i = 0; i < NUMBEROF(Keys); ++i) {
            if (!(INSIDE(x, y, Keys[i].bounds))) {
                continue;
            }

            /* Chameleon processes keys on the way down. */
#ifdef SOUND_SUPPORTED
            if (iMsg == WM_LBUTTONDOWN) {
                MessageBeep(MB_OK);
            }
#endif
            switch (Keys[i].button) {
            case KEY_POWER:
                if (iMsg == WM_LBUTTONUP) {
                    return 0;
                }

                pMidpEventResult->type = SHUTDOWN_EVENT;
                pSignalResult->waitingFor = AMS_SIGNAL;
                return 0;

            case KEY_END:
                if (iMsg == WM_LBUTTONUP) {
                    return 0;
                }
#if ENABLE_MULTIPLE_ISOLATES

                pSignalResult->waitingFor = AMS_SIGNAL;
                pMidpEventResult->type =
                    MIDLET_DESTROY_REQUEST_EVENT;
                pMidpEventResult->DISPLAY = gForegroundDisplayId;
                pMidpEventResult->intParam1 = gForegroundIsolateId;
#else
                pSignalResult->waitingFor = UI_SIGNAL;
                pMidpEventResult->type = DESTROY_MIDLET_EVENT;
#endif
                return 0;

            default:
                /* Handle the simulated key events. */
                pSignalResult->waitingFor = UI_SIGNAL;
                pMidpEventResult->type = MIDP_KEY_EVENT;
                pMidpEventResult->CHR  = (short)Keys[i].button;

                switch (iMsg) {
                case WM_LBUTTONDOWN:
                    pMidpEventResult->type = MIDP_KEY_EVENT;
                    pMidpEventResult->ACTION = PRESSED;
                    return 0;

                case WM_LBUTTONUP:
                    pMidpEventResult->type = MIDP_KEY_EVENT;
                    pMidpEventResult->ACTION = RELEASED;
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
        pSignalResult->status = WSAGETSELECTERROR(lParam);
        pSignalResult->descriptor = (int)wParam;

        switch (WSAGETSELECTEVENT(lParam)) {
        case FD_CONNECT:
            /* Change this to a write. */
            pSignalResult->waitingFor = NETWORK_WRITE_SIGNAL;
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "FD_CONNECT)\n");
#endif
            return 0;

        case FD_WRITE:
            pSignalResult->waitingFor = NETWORK_WRITE_SIGNAL;
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "FD_WRITE)\n");
#endif
            return 0;

        case FD_ACCEPT:
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "FD_ACCEPT, ");
#endif
        case FD_READ:
            pSignalResult->waitingFor = NETWORK_READ_SIGNAL;
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "FD_READ)\n");
#endif
            return 0;

        case FD_CLOSE:
            pSignalResult->waitingFor = NETWORK_EXCEPTION_SIGNAL;
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

    case WM_HOST_RESOLVED:
#ifdef ENABLE_TRACE_NETWORKING
        fprintf(stderr, "Got Windows event WM_HOST_RESOLVED \n");
#endif
        pSignalResult->waitingFor = HOST_NAME_LOOKUP_SIGNAL;
        pSignalResult->descriptor = (int) wParam;
        pSignalResult->status = WSAGETASYNCERROR(lParam);
        return 0;

    case WM_DEBUGGER:
        pSignalResult->waitingFor = VM_DEBUG_SIGNAL;
        return 0;

#if ENABLE_NATIVE_AMS
    case WM_TEST:
	nams_process_command(wParam, lParam);

        appManagerRequestWaiting = 1;
        return 0;
#endif

    default:
        break;
    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

static int strcasecmp(const char *a, const char *b) {
    while (*a && *b) {
        int d = tolower(*a) - tolower(*b);
        if (d) return d;
        ++a;
        ++b;
    }

    return tolower(*a) - tolower(*b);
}

static void CreateScreenBitmap(HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    BITMAPINFOHEADER* b = phone_dib;
    HBITMAP img, tmp;
    int cmapLen;

    BITMAPINFOHEADER* grayLEDInfo = grayLED_dib;
    BITMAPINFOHEADER* greenLEDInfo = greenLED_dib;
    BITMAPINFOHEADER* topbarInfo = topbar_dib;

    backgroundColor = lightGrayPixel;
    foregroundColor = blackPixel;

    hPhoneBitmap = CreateCompatibleBitmap(hdc,
                                           EMULATOR_WIDTH, EMULATOR_HEIGHT);
    CHECK_RETURN(tmp = SelectObject(hdcMem, hPhoneBitmap));

    cmapLen = (b->biBitCount > 8) ? 0 :
        (b->biClrUsed ? b->biClrUsed : (1 << b->biBitCount));

    img = CreateDIBitmap(hdc, b, CBM_INIT,
                        ((char*)b) + b->biSize + 4*cmapLen,
                       (BITMAPINFO*)b, DIB_RGB_COLORS);

    DrawBitmap(hdcMem, img, 0, 0, SRCCOPY);

    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_PEN));
    CHECK_RETURN(SelectObject(hdcMem, BACKGROUND_BRUSH));

    Rectangle(hdcMem, DISPLAY_X, DISPLAY_Y,
              DISPLAY_X + DISPLAY_WIDTH, DISPLAY_Y + DISPLAY_HEIGHT);

    DeleteObject(img);

    cmapLen = (topbarInfo->biBitCount > 8) ? 0 :
        (topbarInfo->biClrUsed ? topbarInfo->biClrUsed :
         (1 << topbarInfo->biBitCount));

    topbar_Image = CreateDIBitmap(hdc, topbarInfo, CBM_INIT,
                                  ((char*)topbarInfo) + topbarInfo->biSize +
                                  4*cmapLen, (BITMAPINFO*)topbarInfo,
                                  DIB_RGB_COLORS);
    
    DrawBitmap(hdcMem, topbar_Image, X_SCREEN_OFFSET, Y_SCREEN_OFFSET,
               SRCCOPY);

    /* Create dim (default) network indicator and draw it */
    cmapLen = (grayLEDInfo->biBitCount > 8) ? 0 :
        (grayLEDInfo->biClrUsed ? grayLEDInfo->biClrUsed :
         (1 << grayLEDInfo->biBitCount));

    LED_off_Image = CreateDIBitmap(hdc, grayLEDInfo, CBM_INIT,
                                   ((char*)grayLEDInfo) + grayLEDInfo->biSize +
                                   4*cmapLen, (BITMAPINFO*)grayLEDInfo,
                                   DIB_RGB_COLORS);
    DrawBitmap(hdcMem, LED_off_Image, LED_xposition, LED_yposition, SRCCOPY);

    /* Create bright network indicator */
    cmapLen = (greenLEDInfo->biBitCount > 8) ? 0 :
        (greenLEDInfo->biClrUsed ? greenLEDInfo->biClrUsed :
         (1 << greenLEDInfo->biBitCount));
    LED_on_Image = CreateDIBitmap(hdc, greenLEDInfo, CBM_INIT,
                                  ((char*)greenLEDInfo) + greenLEDInfo->biSize
                                  + 4 * cmapLen, (BITMAPINFO*)greenLEDInfo,
                                  DIB_RGB_COLORS);

    /* Create trusted icon */
    /*
    cmapLen = (trustIconInfo->biBitCount > 8) ? 0 :
        (trustIconInfo->biClrUsed ?
        trustIconInfo->biClrUsed :
        (1 << trustIconInfo->biBitCount));

    TrustIcon_Image = CreateDIBitmap(hdc, trustIconInfo, CBM_INIT,
                                     ((char*)trustIconInfo)
                                     + trustIconInfo->biSize
                                     + 4 * cmapLen,
                                     (BITMAPINFO*)trustIconInfo,
                                     DIB_RGB_COLORS);
    */

    /* SetTextColor(hdcMem, RGB(255,255,255)); */
    /* SetTextAlign(hdcMem, TA_TOP | TA_LEFT); */
    /* SetBkMode(hdcMem, TRANSPARENT); */
    /* TextOut(hdcMem, 8, 8, "kvm", 3); */
    /* SetBkMode(hdcMem, OPAQUE); */
    //SetBkMode(hdcMem, OPAQUE);
    //SetBkColor(hdcMem, backgroundColor);

    SelectObject(hdcMem, tmp);
    DeleteDC(hdcMem);
}

/**
 * Initializes the screen back buffer
 */
static void initScreenBuffer(int w, int h) {
    gxj_system_screen_buffer.width = w;
    gxj_system_screen_buffer.height = h;
    gxj_system_screen_buffer.pixelData = 
      (gxj_pixel_type*)midpMalloc(w*h*sizeof(gxj_pixel_type));
    gxj_system_screen_buffer.alphaData = NULL;
    
    if (gxj_system_screen_buffer.pixelData == NULL) {
    	REPORT_CRIT(LC_HIGHUI, "gxj_system_screen_buffer allocation failed");
    }
}

static void CreateEmulatorWindow() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEX  wndclass ;
    HWND hwnd;
    HDC hdc;
    HMENU hMenu = NULL;
    static WORD graybits[] = {0xaaaa, 0x5555, 0xaaaa, 0x5555,
                              0xaaaa, 0x5555, 0xaaaa, 0x5555};

    unsigned int width = EMULATOR_WIDTH, height = EMULATOR_HEIGHT;

    if (initialized) {
        return;
    } else {
        initialized = KNI_TRUE;
    }

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
    wndclass.lpszClassName = MAIN_WINDOW_CLASS_NAME;
    wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION) ;

    RegisterClassEx (&wndclass) ;
#ifdef SKINS_MENU_SUPPORTED
    hMenu = buildSkinsMenu();

    if (hMenu != NULL) height += 24;
#endif

    hwnd = CreateWindow(MAIN_WINDOW_CLASS_NAME, /* window class name     */
                        PROJECT_NAME,         /* window caption    */
                        WS_OVERLAPPEDWINDOW & /* window style; disable   */
                        (~WS_MAXIMIZEBOX),    /* the 'maximize' button   */
                        CW_USEDEFAULT,        /* initial x position      */
                        CW_USEDEFAULT,        /* initial y position      */
                        width,                /* initial x size          */
                        height,               /* initial y size          */
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
    CreateScreenBitmap(hdc);
    ReleaseDC(hwnd, hdc);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
}

static HDC getBitmapDC(void *imageData) {
    HDC hdc;

    if (lastImage != imageData) {
        if (hMemDC != NULL) {
            DeleteDC(hMemDC);
        }

        hdc = GetDC(hMainWindow);
        hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hMainWindow, hdc);
        lastImage = imageData;
    }

    if (imageData == NULL) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hMemDC, hPhoneBitmap));
        SetWindowOrgEx(hMemDC, -DISPLAY_X, -DISPLAY_Y, NULL);
    } else if (imageData == UNTRANSLATED_SCREEN_BITMAP) {
        CHECK_RETURN(getBitmapDCtmp = SelectObject(hMemDC, hPhoneBitmap));
    } else {
        myBitmapStruct *bmp = (myBitmapStruct *)imageData;
        if (bmp->mutable) {
            getBitmapDCtmp = SelectObject(hMemDC, bmp->bitmap);
        }
    }

    return hMemDC;
}

static void releaseBitmapDC(HDC hdcMem)
{
    SelectObject(hdcMem, getBitmapDCtmp);
    getBitmapDCtmp = NULL;
}

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

static void invalidateLCDScreen(int x1, int y1, int x2, int y2) {
    RECT r;

    if (x1 < x2) {
        r.left = x1 + DISPLAY_X;
        r.right = x2 + DISPLAY_X;
    } else {
        r.left = x2 + DISPLAY_X;
        r.right = x1 + DISPLAY_X;
    }
    if (y1 < y2) {
        r.top = y1 + DISPLAY_Y;
        r.bottom = y2 + DISPLAY_Y;
    } else {
        r.top = y2 + DISPLAY_Y;
        r.bottom = y1 + DISPLAY_Y;
    }

    InvalidateRect(hMainWindow, &r, KNI_TRUE);

    if (hMemDC != NULL) {
        DeleteDC(hMemDC);
        lastImage = (void*)0xfffffffe;
        hMemDC = NULL;
    }
}

static HPEN setPen(HDC hdc, int pixel, int dotted) {
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, backgroundColor);

    if (dotted) {
        SetBkMode(hdc, TRANSPARENT);
        return SelectObject(hdc, CreatePen(PS_DOT, 1, pixel));
    } else {
        return SelectObject(hdc, CreatePen(PS_SOLID, 1, pixel));
    }
}
