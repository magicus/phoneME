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

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

#include <stdio.h>
#include <windows.h>
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

/*#include "staticGraphics.h" */
#include "midpStubsKeymapping.h"

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))

/*
 * This (x,y) coordinate pair refers to the offset of the upper
 * left corner of the display screen within the MIDP phone handset
 * graphic window
 */
#define X_SCREEN_OFFSET 60
#define Y_SCREEN_OFFSET 76

#define TOP_BAR_HEIGHT  0

/*
 * Defines Java code paintable region
 */
#define DISPLAY_WIDTH   CHAM_WIDTH
#define DISPLAY_HEIGHT  CHAM_FULLHEIGHT
#define DISPLAY_X       X_SCREEN_OFFSET
#define DISPLAY_Y       (Y_SCREEN_OFFSET + TOP_BAR_HEIGHT)

#define UNTRANSLATED_SCREEN_BITMAP (void*)0xffffffff

#define CHECK_RETURN(expr) (expr) ? (void)0 : (void)fprintf(stderr, "%s returned error (%s:%d)\n", #expr, __FILE__, __LINE__)

#define ASSERT(expr) (expr) ? (void)0 : (void)fprintf(stderr, \
    "%s:%d: (%s)is NOT true\n", __FILE__, __LINE__, #expr)

#define KEYMAP_MD_KEY_HOME (KEYMAP_KEY_MACHINE_DEP)
#define KEYMAP_MD_KEY_SWITCH_APP (KEYMAP_KEY_MACHINE_DEP - 1)

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
/*
#define LED_xposition 17
#define LED_yposition 82
#define LED_width     20
#define LED_height    20
*/

#define INSIDE(_x, _y, _r)                              \
    ((_x >= (_r).x) && (_x < ((_r).x + (_r).width)) &&  \
     (_y >= (_r).y) && (_y < ((_r).y + (_r).height)))


static jboolean reverse_orientation = KNI_FALSE;

static LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

static void releaseBitmapDC(HDC hdcMem);
static void DrawBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int rop);
static HDC getBitmapDC(void *imageData);
static HPEN setPen(HDC hdc, int pixel, int dotted);
static void CreateBacklight(HDC hdc);
static void CreateEmulatorWindow();
static void resizeScreenBuffer();
static void invalidateLCDScreen(int x1, int y1, int x2, int y2);

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
static jboolean inFullScreenMode = KNI_FALSE;
static jboolean bkliteImageCreated = KNI_FALSE;
static jboolean isBklite_on = KNI_FALSE;


static int backgroundColor = RGB(182, 182, 170); /* This a win32 color value */
static int foregroundColor = RGB(0,0,0); /* This a win32 color value */

static void* lastImage = (void*)0xfffffffe;

static HWND hMainWindow    = NULL;

static HDC hMemDC = NULL;

static TEXTMETRIC    fixed_tm, tm;
static HFONT            fonts[3][3][8];

/* The phone images */
static HBITMAP          hPhoneBitmap;
static HBITMAP          hPhoneLightBitmap;


void win32app_init() {
    CreateEmulatorWindow();
}

/**
 * Utility function to request logical screen to be painted
 * to the physical screen when screen is in normal mode. 
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
static void win32app_refresh_normal(int x1, int y1, int x2, int y2) {
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

    HDC            hdcMem;
    HBITMAP        destHBmp;
    BITMAPINFO     bi;
    HGDIOBJ        oobj;
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

    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         = width;
    bi.bmiHeader.biHeight        = -height;
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

/**
 * Utility function to request logical screen to be painted
 * to the physical screen when screen is in rotated mode. 
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
static void win32app_refresh_rotate(int x1, int y1, int x2, int y2) {
    int x;
    int y;
    int width;
    int height;
    gxj_pixel_type* pixels = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type pixel;
    int r;
    int g;
    int b;
    unsigned char *destBits;
    unsigned char *destPtr;

    HDC            hdcMem;
    HBITMAP        destHBmp;
    BITMAPINFO     bi;
    HGDIOBJ        oobj;
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


    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         = height; /* width; */
    bi.bmiHeader.biHeight        = -width; /* -height; */
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

    pixels += x2-1 + y1 * win32app_get_screen_width();

      for (x = x2; x > x1; x--) {

        int y;

        for (y = y1; y < y2; y++) {            
            r = GXJ_GET_RED_FROM_PIXEL(*pixels);
            g = GXJ_GET_GREEN_FROM_PIXEL(*pixels);
            b = GXJ_GET_BLUE_FROM_PIXEL(*pixels);            
            *destPtr++ = b;
            *destPtr++ = g;
            *destPtr++ = r;            
            destPtr += sizeof(long) - 3*sizeof(*destPtr);
            pixels += win32app_get_screen_width();
         }
         pixels += -1 - height * win32app_get_screen_width();         

    }    

        SetDIBitsToDevice(hdc, y, win32app_get_screen_width() - width - x, 
			  height, width, 0, 0, 0,
                          width, destBits, &bi, DIB_RGB_COLORS);
}

    DeleteObject(oobj);
    DeleteObject(destHBmp);
    DeleteDC(hdcMem);
    releaseBitmapDC(hdc);

    invalidateLCDScreen(x1, y1, x1 + height, y1 + width);
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
    if (reverse_orientation) {
        win32app_refresh_rotate(x1, y1, x2, y2);
    } else {
        win32app_refresh_normal(x1, y1, x2, y2);
    }
}

/**
 * Invert screen rotation flag
 */
jboolean win32app_reverse_orientation() {
    reverse_orientation = !reverse_orientation;
    resizeScreenBuffer();
    return reverse_orientation;
}

/**
 * Get screen rotation flag
 */
jboolean win32app_get_reverse_orientation() {
    return reverse_orientation;
}

/**
 * Handle clamshell events
 */
void win32app_handle_clamshell_event() {
}

/**
 * Set full screen mode on/off
 */
void win32app_set_fullscreen_mode(jboolean mode) {
    if (inFullScreenMode != mode) {
        inFullScreenMode = mode;
        resizeScreenBuffer();
    }
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


/**
 * Flushes the offscreen buffer directly to the device screen.
 * The size of the buffer flushed is defined by offscreen buffer width
 * and passed in height. 
 * Offscreen_buffer must be aligned to the top-left of the screen and
 * its width must be the same as the device screen width.
 * @param graphics The Graphics instance associated with the screen.
 * @param offscreen_buffer The offscreen buffer to be flushed
 * @param h The height to be flushed
 * @return KNI_TRUE if direct_flush was successful, KNI_FALSE - otherwise
 */
jboolean win32app_direct_flush(const java_graphics *g, 
		    	       const java_imagedata *offscreen_buffer, 
                               int h) {
  return KNI_FALSE;
}

/*
 * Draw BackLight.
 * If 'active' is KNI_TRUE, the BackLight is drawn.
 * If 'active' is KNI_FALSE, the BackLight is erased.
 */
jboolean drawBackLight(AncBacklightState mode) {

    HDC hdc = GetDC(hMainWindow);
    jboolean result = KNI_FALSE;

    if (mode == ANC_BACKLIGHT_SUPPORTED) {
        result = KNI_TRUE;
    }
    else {
        CreateBacklight(hdc);
        if ((mode == ANC_BACKLIGHT_ON) || 
            (mode == ANC_BACKLIGHT_TOGGLE && isBklite_on == KNI_FALSE)) {
            isBklite_on = KNI_TRUE;
            DrawBitmap(hdc, hPhoneLightBitmap, 0, 0, SRCCOPY);
            invalidateLCDScreen(0, 0, win32app_get_screen_width(), 
				win32app_get_screen_height());
            result = KNI_TRUE;
        } else if ((mode == ANC_BACKLIGHT_OFF) ||
                  (mode == ANC_BACKLIGHT_TOGGLE && isBklite_on == KNI_TRUE)){
            if (isBklite_on) {
                isBklite_on = KNI_FALSE;
                InvalidateRect(hMainWindow, NULL, KNI_TRUE);
            }
            result = KNI_TRUE;
        }
    }

    ReleaseDC(hMainWindow, hdc);
    return result;
}

static jint mapKey(WPARAM wParam, LPARAM lParam) {
    char keyStates[256];
    WORD temp[2];

    switch (wParam) {
    case VK_F1:
        return KEYMAP_KEY_SOFT1;

    case VK_F2:
        return KEYMAP_KEY_SOFT2;

    case VK_F3:
        return KEYMAP_KEY_SCREEN_ROT;

    case VK_F6:
        return KEYMAP_KEY_VIRT_KEYB;

    case VK_F9:
        return KEYMAP_KEY_GAMEA;

    case VK_F4:
        return KEYMAP_MD_KEY_SWITCH_APP;

    case VK_F10:
        return KEYMAP_KEY_GAMEB;

    case VK_F11:
        return KEYMAP_KEY_GAMEC;

    case VK_F12:
        return KEYMAP_KEY_GAMED;
        break;

    case VK_UP:
        return KEYMAP_KEY_UP;

    case VK_DOWN:
        return KEYMAP_KEY_DOWN;

    case VK_LEFT:
        return KEYMAP_KEY_LEFT;

    case VK_RIGHT:
        return KEYMAP_KEY_RIGHT;
        
    /*
     * Map VK_SPACE here, but in the
     * high level Java code, we have to
     * test for special case since "space"
     * should be used for textbox's as space.
     */
    case VK_SPACE:
    case VK_RETURN:
        return KEYMAP_KEY_SELECT;

    case VK_BACK:
        return KEYMAP_KEY_BACKSPACE;

    case VK_HOME:
    case VK_F7:
        return KEYMAP_MD_KEY_HOME;
    
    case VK_END:
    case VK_F8:
        return KEYMAP_KEY_END;

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

    return KEYMAP_KEY_INVALID;
}

#if ENABLE_NATIVE_APP_MANAGER
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
        case KEYMAP_KEY_INVALID:
            return 0;

        case KEYMAP_MD_KEY_SWITCH_APP:
            if (iMsg == WM_KEYDOWN) {
                return 0;
            }
            pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
            pMidpEventResult->intParam1 = 1;
            pSignalResult->waitingFor = AMS_SIGNAL;
            return 0;

        case KEYMAP_KEY_SCREEN_ROT:
                if (iMsg == WM_KEYDOWN) {
                    return 0;
            }
            pMidpEventResult->type = ROTATION_EVENT;
            pSignalResult->waitingFor = UI_SIGNAL;
            return 0;

        case KEYMAP_KEY_VIRT_KEYB:
                if (iMsg == WM_KEYDOWN) {
                    return 0;
            }
            pMidpEventResult->type = VIRTUAL_KEYBOARD_EVENT;
            pSignalResult->waitingFor = UI_SIGNAL;
            return 0;

        case KEYMAP_MD_KEY_HOME:
            if (iMsg == WM_KEYDOWN) {
                return 0;
            }

            pMidpEventResult->type = SELECT_FOREGROUND_EVENT;
            pMidpEventResult->intParam1 = 0;
            pSignalResult->waitingFor = AMS_SIGNAL;
            return 0;

        default:
            break;
        }

        pMidpEventResult->type = MIDP_KEY_EVENT;
        pMidpEventResult->CHR = key;

        if (iMsg == WM_KEYUP) {
            pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
        } else if (lParam & 0x40000000) {
            pMidpEventResult->ACTION = KEYMAP_STATE_REPEATED;
        } else {
            pMidpEventResult->ACTION = KEYMAP_STATE_PRESSED;
        }

        pSignalResult->waitingFor = UI_SIGNAL;
           
        return 0;

    case WM_TIMER:
        /* Stop the timer from repeating the WM_TIMER message. */
        KillTimer(hwnd, wParam);

        if (wParam != EVENT_TIMER_ID) {
            /* This is a push alarm */
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
            int maxX, maxY;
 
            /*
             * Check to see if we have moved the mouse outside of
             * the screen bounds while holding the mouse button.
             * If so, we still need to continue to send the mouse
             * events to the MIDlet, but, with at least one
             * coordinate "pegged" to the edge of the screen.
             */
            if (reverse_orientation) {
                maxX = DISPLAY_Y + win32app_get_screen_width() - 1;
                maxY = DISPLAY_X + win32app_get_screen_height() - 1;
                pMidpEventResult->X_POS = 
		  min(win32app_get_screen_width() - y + DISPLAY_Y, 
		      maxX - DISPLAY_Y);
                pMidpEventResult->Y_POS = min(x, maxY) - DISPLAY_X;
            } else {
                maxX = DISPLAY_X + win32app_get_screen_width() - 1;
                maxY = DISPLAY_Y + win32app_get_screen_height() - 1;
                pMidpEventResult->X_POS = min(x, maxX) - DISPLAY_X;
                pMidpEventResult->Y_POS = min(y, maxY) - DISPLAY_Y;       
            }
            if (pMidpEventResult->X_POS < 0) {
                pMidpEventResult->X_POS = 0;
            }

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
                pMidpEventResult->ACTION = KEYMAP_STATE_PRESSED;
                return 0;

            case WM_LBUTTONUP:
                ReleaseCapture();
                penDown = KNI_FALSE;
                pMidpEventResult->type = MIDP_PEN_EVENT;
                pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
                return 0;

            default: /* WM_MOUSEMOVE */
                pMidpEventResult->type = MIDP_PEN_EVENT;
                pMidpEventResult->ACTION = KEYMAP_STATE_DRAGGED;
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
            case KEYMAP_KEY_POWER:
                if (iMsg == WM_LBUTTONUP) {
                    return 0;
                }

                pMidpEventResult->type = SHUTDOWN_EVENT;
                pSignalResult->waitingFor = AMS_SIGNAL;
                return 0;

            case KEYMAP_KEY_END:
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
                    pMidpEventResult->ACTION = KEYMAP_STATE_PRESSED;
                    return 0;

                case WM_LBUTTONUP:
                    pMidpEventResult->type = MIDP_KEY_EVENT;
                    pMidpEventResult->ACTION = KEYMAP_STATE_RELEASED;
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

#if ENABLE_NATIVE_APP_MANAGER
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

/**
 * Return screen width
 */
int win32app_get_screen_width() {
    if (reverse_orientation) {
        return inFullScreenMode ? CHAM_FULLHEIGHT : CHAM_HEIGHT;
    } else {
        return inFullScreenMode ? CHAM_FULLWIDTH : CHAM_WIDTH;
    }
}

/**
 * Return screen height
 */
int win32app_get_screen_height() {
    if (reverse_orientation) {
        return inFullScreenMode ? CHAM_FULLWIDTH : CHAM_WIDTH;
    } else {
        return inFullScreenMode ? CHAM_FULLHEIGHT : CHAM_HEIGHT;
    }
}

/**
 * Resizes the screen back buffer
 */
static void resizeScreenBuffer() {

    int newWidth = win32app_get_screen_width();
    int newHeight = win32app_get_screen_height();
    int newScreenSize = sizeof(gxj_pixel_type) * newWidth * newHeight;
    if (gxj_system_screen_buffer.pixelData != NULL) {
        if (newScreenSize != sizeof(gxj_pixel_type) 
                * gxj_system_screen_buffer.width 
                * gxj_system_screen_buffer.height) {
            /* free screen buffer */
            midpFree(gxj_system_screen_buffer.pixelData);
            gxj_system_screen_buffer.pixelData = NULL;
        }
    }


    gxj_system_screen_buffer.width = newWidth;
    gxj_system_screen_buffer.height = newHeight;
    gxj_system_screen_buffer.alphaData = NULL;

    if (gxj_system_screen_buffer.pixelData == NULL) {
        gxj_system_screen_buffer.pixelData = 
            (gxj_pixel_type *)midpMalloc(newScreenSize);
    }

    if (gxj_system_screen_buffer.pixelData == NULL) {
        REPORT_CRIT(LC_HIGHUI, "gxj_system_screen_buffer allocation failed");
    } else {
        memset(gxj_system_screen_buffer.pixelData, 0, newScreenSize);
    }
}

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
        printf("Cannot load background image from resources.\n");
        return NULL;
    }
    getBitmapSize(hBitmap, width, height);
    return hBitmap;
}

static void CreateBacklight(HDC hdc) {

    int width, height;
    if (KNI_FALSE == bkliteImageCreated) {
        hPhoneLightBitmap = loadBitmap("phone_hilight.bmp",&width,&height);
        bkliteImageCreated = KNI_TRUE;
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

    unsigned int width ; /* = EMULATOR_WIDTH; */
    unsigned int height; /* = EMULATOR_HEIGHT; */
    static char caption[32];

    hPhoneBitmap = loadBitmap("phone.bmp",&width,&height);

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
    gxj_system_screen_buffer.pixelData = NULL;
    gxj_system_screen_buffer.width = 0;
    gxj_system_screen_buffer.height = 0;
    gxj_system_screen_buffer.alphaData = NULL;
    resizeScreenBuffer();

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
