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
 * This camera sample codes uses Win32 API to show reference implementation of
 * camera features. Now, it only supports "png" snapshot encoding format.
 */
 
#include "multimedia.h"
#include "pngencoder.h"
#include <windows.h>
#include <tchar.h>
#include <vfw.h>
#include <stdio.h>

/* Debugging purpose flag to save encoded PNG data to file */
#undef SNAPSHOT_PNG_ENCODING_TO_FILE

/******************************************************************************/

/* Default camera preview x, y, w and h constants */
#define DEFAULT_PREVIEW_X   0
#define DEFAULT_PREVIEW_Y   0
#define DEFAULT_PREVIEW_W   128
#define DEFAULT_PREVIEW_H   160

/* Device acquire count that is used by acquire and release device function */
static int _acquireCount = 0;
/* Snapshot related static variables */
static javacall_bool _grabFrame = JAVACALL_FALSE;
static javacall_int64 _playerId;
static char* _pFrameBuffer = NULL;
static char* _pEncodingBuffer = NULL;
static long  _encodingDataSize = 0;
static int   _grabWidth = 0;
static int   _grabHeight = 0;

/* camera handler internal native handle */
typedef struct {
    javacall_int64  playerId;
    HWND            hCapWnd;
    int             x;
    int             y;
    int             w;
    int             h;
    javacall_bool   visible;
    javacall_bool   acquireDevice;
} camera_handle;

typedef struct {
    char    encoding[10];
    int     width;
    int     height;
} image_type;

/**
 * Parse imageTtpe string and return encoding format, width and height information
 */
static javacall_result camera_parse_image_type(const javacall_utf16* imageType, 
                                               long length, 
                                               /*OUT*/ image_type* parsedType)
{
#define DEFAULT_ENCODING_W (160)
#define DEFAULT_ENCODING_H (120)

    char* pStr;
    char* pEncoding = NULL;
    char* pWidth = NULL;
    char* pHeight = NULL;

    /**
     * There is no encoding format. Use default.
     */
    if (0 == length) {
       strcpy(parsedType->encoding, "png");
       parsedType->width = DEFAULT_ENCODING_W;
       parsedType->height = DEFAULT_ENCODING_H;
       return JAVACALL_OK;
    }

    pStr = MALLOC(length + 1); 
    if (NULL == pStr) {
        return JAVACALL_FAIL;
    }

    if (0 == WideCharToMultiByte(
                CP_ACP, 0, imageType, length, pStr, length + 1, NULL, NULL)) {
        FREE(pStr);
        return JAVACALL_FAIL;
    }
    pStr[length] = 0x0;

    pEncoding = strstr(pStr, "encoding=");
    pWidth = strstr(pStr, "width=");
    pHeight = strstr(pStr, "height=");

    if (pEncoding) {
        pEncoding += strlen("encoding=");
        strtok(pEncoding, "&");
        if (strlen(pEncoding) < sizeof(parsedType->encoding)) {
            strcpy(parsedType->encoding, pEncoding);
        }
    } else {
        FREE(pStr);
        return JAVACALL_FAIL;
    }

    if (pWidth) {
        pWidth += strlen("width=");
        strtok(pWidth, "&");
        parsedType->width = atoi(pWidth);
    } else {
        parsedType->width = 160;
    }

    if (pHeight) {
        pHeight += strlen("height=");
        strtok(pHeight, "&");
        parsedType->height = atoi(pWidth);
    } else {
        parsedType->height = 120;
    }

    FREE(pStr);

    return JAVACALL_OK;
}

/*******************************************************************************/

/**
 * VFW frame grab callback function
 */
static LRESULT PASCAL camera_grabber_callback(HWND hWnd, LPVIDEOHDR lpVHdr)
{
#define PNG_HEADER_SIZE (1000)
    int i;
    char* pOutput;
    int copyLineBytes;

    /* Free previous frame buffer */
    if (NULL != _pFrameBuffer) {
        FREE(_pFrameBuffer);
        _pFrameBuffer = NULL;
    }

    /* Free encoding buffer */
    if (NULL != _pEncodingBuffer) {
        FREE(_pEncodingBuffer);
        _pEncodingBuffer = NULL;
    }

    /* Grab current frame */
    if (JAVACALL_TRUE == _grabFrame) {
        copyLineBytes = (_grabWidth * 3);
        
        _pFrameBuffer = MALLOC(lpVHdr->dwBytesUsed);
        _pEncodingBuffer = 
            MALLOC(javautil_media_get_png_size(_grabWidth, _grabHeight));
        
        if (_pFrameBuffer && _pEncodingBuffer) {
            pOutput = _pFrameBuffer;
            /* Impl note: - Top / Bottom Reverse */
            for (i = _grabHeight - 1; i >= 0; --i) {
                memcpy(pOutput, lpVHdr->lpData + (i * copyLineBytes), copyLineBytes);
                pOutput += copyLineBytes;
            }

            _encodingDataSize = javautil_media_rgb_to_png(
                _pFrameBuffer, _pEncodingBuffer, _grabWidth, _grabHeight);
            javanotify_on_media_notification(JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
                                             _playerId, 0);

        } else {
            if (_pFrameBuffer) {
                FREE(_pFrameBuffer);
                _pFrameBuffer = NULL;
            }
            if (_pEncodingBuffer) {
                FREE(_pEncodingBuffer);
                _pEncodingBuffer = NULL;
            }
        }
        _grabFrame = JAVACALL_FALSE;
    }

    InvalidateRect(GET_MCIWND_HWND(), NULL, FALSE);
    return 0;
}

/**
 * Destroy camera window
 */
static void camera_destroy_window(camera_handle* pHandle)
{
    if (pHandle->hCapWnd) {
        capSetCallbackOnFrame(pHandle->hCapWnd, NULL);
        capDriverDisconnect(pHandle->hCapWnd);
        DestroyWindow(pHandle->hCapWnd);
        pHandle->hCapWnd = NULL;
    }
}

/**
 * Set camera preview window
 */
static HWND camera_set_preview_window(javacall_handle handle, 

                                      int x, int y, int w, int h, BOOL visible)
{
#define DEFAULT_CAPTURE_DRIVER  0   
#define DEFAULT_PREVIEW_RATE    150 
    /* ms unit => Increase this value to optimize performance */
    
    BOOL ret;
    camera_handle* pHandle = (camera_handle*)handle;
    DWORD wsVisible = TRUE == visible ? WS_VISIBLE : 0;

    camera_destroy_window(pHandle);

    JAVA_DEBUG_PRINT4("[camera] capCreateCaptureWindow %d %d %d %d\n", x, y, w, h);

    pHandle->hCapWnd = capCreateCaptureWindow(_T("Sun_Java_Cap_Window"), 
                   wsVisible | WS_CHILD | WS_CLIPSIBLINGS, 
                   x + X_SCREEN_OFFSET, y + Y_SCREEN_OFFSET + TOP_BAR_HEIGHT, 
                   w, h, GET_MCIWND_HWND(), 0xffff);

    JAVA_DEBUG_PRINT1("[camera] capCreateCaptureWindow %d\n", pHandle->hCapWnd);

    if (pHandle->hCapWnd) {
        ret = capDriverConnect(pHandle->hCapWnd, DEFAULT_CAPTURE_DRIVER);
        if (FALSE == ret) {
            JAVA_DEBUG_PRINT(
                "[camera] capDriverConnect fail - is there camera attached?\n");
            DestroyWindow(pHandle->hCapWnd);
            pHandle->hCapWnd = NULL;
            return NULL;
        }
        capSetCallbackOnFrame(pHandle->hCapWnd, camera_grabber_callback);
        capPreviewScale(pHandle->hCapWnd, TRUE);
        capPreviewRate(pHandle->hCapWnd, DEFAULT_PREVIEW_RATE);
    }

    return pHandle->hCapWnd;
}

/******************************************************************************/

/**
 * Create camera native handle
 */
static javacall_handle camera_create(javacall_int64 playerId, 
                                     javacall_media_type mediaType, 
                                     const javacall_utf16* URI, 
                                     long contentLength)
{
    camera_handle* pHandle = MALLOC(sizeof(camera_handle));
    if (NULL == pHandle) return NULL;

    pHandle->hCapWnd = NULL;
    pHandle->visible = JAVACALL_FALSE;
    pHandle->playerId = playerId;
    pHandle->acquireDevice = JAVACALL_FALSE;
    pHandle->x = DEFAULT_PREVIEW_X;
    pHandle->y = DEFAULT_PREVIEW_Y;
    pHandle->w = DEFAULT_PREVIEW_W;
    pHandle->h = DEFAULT_PREVIEW_H;

    return pHandle;
}

static javacall_result camera_destroy(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * Close camera native handle (and destroy preview window)
 */
static javacall_result camera_close(javacall_handle handle)
{
    camera_handle* pHandle = (camera_handle*)handle;

    if (pHandle->hCapWnd) {
        capSetCallbackOnFrame(pHandle->hCapWnd, NULL);
        capDriverDisconnect(pHandle->hCapWnd);
        DestroyWindow(pHandle->hCapWnd);
        pHandle->hCapWnd = NULL;
    }

    FREE(pHandle);

    return JAVACALL_OK;
}

/**
 * Acquire camera device (support only 1 device)
 */
static javacall_result camera_acquire_device(javacall_handle handle)
{
    camera_handle* pHandle = (camera_handle*)handle;

    if (JAVACALL_TRUE == pHandle->acquireDevice) {
        return JAVACALL_OK;
    } else if (0 == _acquireCount) {
        _acquireCount = 1;
        pHandle->acquireDevice = JAVACALL_TRUE;
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/**
 * Release camera device
 */
static javacall_result camera_release_device(javacall_handle handle)
{
    camera_handle* pHandle = (camera_handle*)handle;

    if (JAVACALL_TRUE == pHandle->acquireDevice) {
        pHandle->acquireDevice = JAVACALL_FALSE;
        _acquireCount = 0;
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/**
 * Start previewing
 */
static javacall_result camera_start(javacall_handle handle)
{
    camera_handle* pHandle = (camera_handle*)handle;

    JAVA_DEBUG_PRINT2("[camera] camera_start %d %d\n", 
        pHandle->hCapWnd, pHandle->visible);

    /* Preview start only if there is preview window and visible is TRUE */
    if (NULL != pHandle->hCapWnd && JAVACALL_TRUE == pHandle->visible) {
        if (TRUE == capPreview(pHandle->hCapWnd, TRUE))
            return JAVACALL_OK;
        else
            return JAVACALL_FAIL;
    } else if (NULL == pHandle->hCapWnd) {
        /* If there is no camera preview window, 
        * create it with default position and size */
        HWND hCapWnd = camera_set_preview_window(
            handle, pHandle->x, pHandle->y, pHandle->w, pHandle->h, FALSE);
        if (NULL == hCapWnd) 
            return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Stop previewing
 */
static javacall_result camera_stop(javacall_handle handle)
{
    camera_handle* pHandle = (camera_handle*)handle;
    BOOL ret = FALSE;

    JAVA_DEBUG_PRINT1("[camera] camera_stop %d %d\n", pHandle->hCapWnd);

    if (NULL != pHandle->hCapWnd) {
        ret = capPreview(pHandle->hCapWnd, FALSE);
    }

    return (TRUE == ret ? JAVACALL_OK : JAVACALL_FAIL);
}

/**
 * Pause previewing
 */
static javacall_result camera_pause(javacall_handle handle)
{
    return camera_stop(handle);
}

/**
 * Resume previewing
 */
static javacall_result camera_resume(javacall_handle handle)
{
    return camera_start(handle);
}

/**
 * Don't do anything
 */
static long camera_get_time(javacall_handle handle)
{
    return -1;
}

/**
 * Don't do anything
 */
static long camera_set_time(javacall_handle handle, long ms)
{
    return -1;
}
 
/**
 * Don't do anything
 */
static long camera_get_duration(javacall_handle handle)
{
    return -1;
}

/*******************************************************************************/

#define IMAGEWIDTH(lpd)     ((LPBITMAPINFOHEADER)lpd)->biWidth
#define IMAGEHEIGHT(lpd)    ((LPBITMAPINFOHEADER)lpd)->biHeight
#define IMAGEBITS(lpd)      ((LPBITMAPINFOHEADER)lpd)->biBitCount
#define IMAGEBICLRUSED(lpd) ((LPBITMAPINFOHEADER)lpd)->biClrUsed

/**
 * Set camera snapshot size
 */
static javacall_result camera_set_video_size(javacall_handle handle, 
                                             long width, long height)
{
    camera_handle* pHandle = (camera_handle*)handle;
    javacall_result ret = JAVACALL_FAIL;
    BITMAPINFO* pBmpInfo;

    JAVA_DEBUG_PRINT2("[camera] camera_set_video_size %d %d\n", width, height);

    /* Convert to supported width & height */
    if (width <= 160) {
        width = 160;
        height = 120;
    } else if (width <= 320) {
        width = 320;
        height = 240;
    } else {
        width = 640;
        height = 480;
    }

    if (pHandle->hCapWnd) {
        int size = capGetVideoFormatSize(pHandle->hCapWnd);
        pBmpInfo = MALLOC(size);
        if (NULL == pBmpInfo) return ret;

        ((LPBITMAPINFOHEADER)pBmpInfo)->biSize= sizeof(BITMAPINFOHEADER);
        if (0 != capGetVideoFormat(pHandle->hCapWnd, pBmpInfo, size)) {
            IMAGEBITS(pBmpInfo) = 24;
            IMAGEBICLRUSED(pBmpInfo) = 0;
            IMAGEWIDTH(pBmpInfo) = width;
            IMAGEHEIGHT(pBmpInfo) = height;
            capSetVideoFormat(pHandle->hCapWnd, pBmpInfo, size);
            ret = JAVACALL_OK;
        }
        FREE(pBmpInfo);
    }

    return ret;
}

/**
 * Get original size of camera capture source
 */
static javacall_result camera_get_video_size(javacall_handle handle, 
                                             long* width, long* height)
{
    camera_handle* pHandle = (camera_handle*)handle;
    javacall_result ret = JAVACALL_FAIL;
    BITMAPINFO* pBmpInfo;

    if (pHandle->hCapWnd) {
        int size = capGetVideoFormatSize(pHandle->hCapWnd);
        pBmpInfo = MALLOC(size);
        if (NULL == pBmpInfo) return ret;

        ((LPBITMAPINFOHEADER)pBmpInfo)->biSize= sizeof(BITMAPINFOHEADER);
        if (0 != capGetVideoFormat(pHandle->hCapWnd, pBmpInfo, size)) {
            if (width) { *width = IMAGEWIDTH(pBmpInfo); }
            if (height) { *height = IMAGEHEIGHT(pBmpInfo); }
            /* IMPL_NOTE - Set to 24 bit RGB format */
            if (IMAGEBITS(pBmpInfo) != 24) {
                IMAGEBITS(pBmpInfo) = 24;
                IMAGEBICLRUSED(pBmpInfo) = 0;
                capSetVideoFormat(pHandle->hCapWnd, pBmpInfo, size);
            }
            ret = JAVACALL_OK;
        }

        FREE(pBmpInfo);
    }

    if (width && height) {
      JAVA_DEBUG_PRINT2("[camera] camera_get_video_size %d %d\n", *width, *height);
    }

    return ret;
}

/**
 * Set camera preview position and size
 */
static javacall_result camera_set_video_location(javacall_handle handle, 
                                                 long x, long y, long w, long h)
{
    camera_handle* pHandle = (camera_handle*)handle;

    JAVA_DEBUG_PRINT4(
        "[camera] camera_set_video_location %d %d %d %d\n", x, y, w, h);

    pHandle->x = x;
    pHandle->y = y;
    pHandle->w = w;
    pHandle->h = h;
    if (pHandle->hCapWnd) {
        camera_set_preview_window(handle, x, y, w, h, TRUE);
    }
    
    return JAVACALL_OK;
}

/**
 * Show or hide camera capture preview
 */
static javacall_result camera_set_video_visible(javacall_handle handle, 
                                                javacall_bool visible)
{
    camera_handle* pHandle = (camera_handle*)handle;
    
    JAVA_DEBUG_PRINT2("[camera] camera_set_video_visible %d %d\n", handle, visible);

    /* Store visible state to cache */
    pHandle->visible = visible;

    /* If there is no camera preview window, 
    create it with default position and size */
    if (NULL == pHandle->hCapWnd) {
        HWND hCapWnd = camera_set_preview_window(
            handle, pHandle->x, pHandle->y, pHandle->w, pHandle->h, 
            (JAVACALL_TRUE == visible) ? TRUE : FALSE);
        if (NULL == hCapWnd) return JAVACALL_FAIL;
    }

    /* Show preview or hide */
    if (JAVACALL_TRUE == visible) {
        ShowWindow(pHandle->hCapWnd, SW_SHOWNORMAL);
    } else {
        ShowWindow(pHandle->hCapWnd, SW_HIDE);
    }

    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Clear snapshot related resources
 */
static void camera_init_snapshot_context()
{
    _grabFrame = JAVACALL_FALSE;
    _encodingDataSize = 0;

    if (_pFrameBuffer) {
        FREE(_pFrameBuffer);
        _pFrameBuffer = NULL;
    }

    if (_pEncodingBuffer) {
        FREE(_pEncodingBuffer);
        _pEncodingBuffer = NULL;
    }
}

/**
 * Start snapshot
 */
static javacall_result camera_start_video_snapshot(javacall_handle handle, 
                                                   const javacall_utf16* imageType,
                                                   long length)
{
    camera_handle* pHandle = (camera_handle*)handle;
    long width;
    long height;
    image_type parsedType;

    /* Init all variables related with snapshot */
    camera_init_snapshot_context();

    if (JAVACALL_FAIL == camera_parse_image_type(imageType, length, &parsedType)) {
        return JAVACALL_FAIL;
    }

    if (0 != strcmp(parsedType.encoding, "png")) {
        return JAVACALL_FAIL;
    }

    /* IMPL_NOTE - If there is no capture window, create it with non-visible state */
    if (NULL == pHandle->hCapWnd) {
        camera_set_preview_window(handle, 
            pHandle->x, pHandle->y, pHandle->w, pHandle->h, FALSE);
    }

    /* Set frame size */
    if (parsedType.width != 0 && parsedType.height != 0) {
        camera_set_video_size(handle, parsedType.width, parsedType.height);
    }
    
    /* Get real frame size */
    if (JAVACALL_FAIL == camera_get_video_size(handle, &width, &height)) {
        return JAVACALL_FAIL;
    }

    _grabWidth = width;
    _grabHeight = height;

    JAVA_DEBUG_PRINT2("[camera] camera_start_video_snapshot %d %d\n", width, height);

    if (pHandle->hCapWnd && pHandle->acquireDevice) {
        BOOL ret;
        _grabFrame = JAVACALL_TRUE;
        _playerId = pHandle->playerId;

        /* Start to grab */
        
        ret = capGrabFrameNoStop(pHandle->hCapWnd);
        if (FALSE == ret) {
            camera_init_snapshot_context();
            return JAVACALL_FAIL;
        }
        #if 0
        /* Encoding to PNG format and store data and size to cache */
        if (_pFrameBuffer && _pEncodingBuffer) {
            return JAVACALL_OK;
        } 
        #endif
        
        return JAVACALL_WOULD_BLOCK;
    }

    return JAVACALL_FAIL;
}

/**
 * Get snapshot encoded data size
 */
static javacall_result camera_get_video_snapshot_data_size(javacall_handle handle, 
                                                           long* size)
{
    *size = _encodingDataSize;
    return JAVACALL_OK;
}

/**
 * Copy encoded snapshot data to buffer
 */
static javacall_result camera_get_video_snapshot_data(javacall_handle handle, 
                                                      char* buffer, long size)
{
    if (size < _encodingDataSize) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    if (_pEncodingBuffer) {
        memcpy(buffer, _pEncodingBuffer, _encodingDataSize);
        camera_init_snapshot_context(); 
        /* Destroy old data to prepare new snapshot */
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/*******************************************************************************/

/**
 * Camera basic javacall function interface
 */
static media_basic_interface _camera_basic_itf = {
    camera_create,
    camera_close,
    camera_destroy,
    camera_acquire_device,
    camera_release_device,
    camera_start,
    camera_stop,
    camera_pause,
    camera_resume,
    NULL,
    NULL,
    camera_get_time,
    camera_set_time,
    camera_get_duration
};

/**
 * Camera video javacall function interface
 */
static media_video_interface _camera_video_itf = {
    camera_get_video_size,
    camera_set_video_visible,
    camera_set_video_location
};

/**
 * Camera snapshot javacall function interface
 */
static media_snapshot_interface _camera_snapshot_itf = {
    camera_start_video_snapshot,
    camera_get_video_snapshot_data_size,
    camera_get_video_snapshot_data
};

/******************************************************************************/
 
/* Global camera interface */
media_interface g_camera_itf = {
    &_camera_basic_itf,
    NULL,
    &_camera_video_itf,
    &_camera_snapshot_itf
}; 

