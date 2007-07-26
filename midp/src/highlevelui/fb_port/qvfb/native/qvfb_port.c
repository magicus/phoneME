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
 * Implementation of the porting layer for QVFb application
 */

#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gxj_putpixel.h>
#include <gxj_screen_buffer.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midp_constants_data.h>

#include "qvfb_port.h"

/** @def PERROR Prints diagnostic message. */
#define PERROR(msg) REPORT_ERROR2(0, "%s: %s", msg, strerror(errno))

/** The file descriptor for reading the mouse */
static int mouseFd = -1;
/** The file descriptor for reading the keyboard */
static int keyboardFd = -1;

/** Return file descriptor of keyboard device, or -1 in none */
int getKeyboardFd() {
    return keyboardFd;
}
/** Return file descriptor of mouse device, or -1 in none */
int getMouseFd() {
    return mouseFd;
}

/** QVFb header data */
static QVFbHeader *hdr = NULL;
/** QVFb video buffer for direct drawing */
static gxj_pixel_type *qvfbPixels = NULL;

/** System offscreen buffer */
gxj_screen_buffer gxj_system_screen_buffer;

/** Allocate system screen buffer */
void initScreenBuffer(int width, int height) {
    if (gxj_init_screen_buffer(width, height) != ALL_OK) {
        fprintf(stderr, "Failed to allocate screen buffer\n");
        exit(1);
    }
}                        

/**
 * Change screen orientation to landscape or portrait,
 * depending on the current screen mode
 */
void reverseScreenOrientation() {

    // Whether current Displayable won't repaint the entire screen on
    // resize event, the artefacts from the old screen content can appear.
    // That's why the buffer content is not preserved.
    gxj_rotate_screen_buffer(KNI_FALSE);
}

/** On i386, connect to the QVFB virtual frame buffer */
void connectFrameBuffer() {
    int displayId = 0;
    char buff[30];
    key_t key;
    int shmId;
    unsigned char *shmrgn;
    char *env;

    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int bufHeight = gxj_system_screen_buffer.height;

    if ((env = getenv("QWS_DISPLAY")) != NULL) {
        displayId = atoi(env + 1); /* skip the leading colon */
    }

    sprintf(buff, "/tmp/.qtvfb_mouse-%d", displayId);
    if ((mouseFd = open(buff, O_RDONLY | O_NONBLOCK, 0)) < 0) {
        fprintf(stderr, "open of %s failed\n", buff);
        exit(1);
    }

    if ((key = ftok(buff, 'b')) == -1) {
        PERROR("ftok() failed");
        exit(1);
    }

    if ((shmId = shmget(key, 0, 0)) == -1) {
        PERROR("shmget() failed");
        exit(1);
    }

    shmrgn = (unsigned char *)shmat(shmId, 0, 0);
    if ((int)shmrgn == -1 || shmrgn == 0) {
        PERROR("shmat() failed");
        exit(1);
    }

    hdr = (QVFbHeader *) shmrgn;
    qvfbPixels = (gxj_pixel_type*)(shmrgn + hdr->dataOffset);

    fprintf(stderr, "QVFB info: %dx%d, depth=%d\n",
           hdr->width, hdr->height, hdr->depth);

    if (hdr->width < bufWidth || hdr->height < bufHeight) {
        fprintf(stderr, "QVFB screen too small. Need %dx%d\n",
               bufWidth, bufHeight);
        exit(1);
    }
    if (hdr->depth != 16) {
        fprintf(stderr, "QVFB depth must be 16. Please run qvfb -depth 16\n");
        exit(1);
    }

    sprintf(buff, "/tmp/.qtvfb_keyboard-%d", displayId);
    if ((keyboardFd = open(buff, O_RDONLY, 0)) < 0) {
        fprintf(stderr, "open of %s failed\n", buff);
        exit(1);
    }

    memset(qvfbPixels, 0, sizeof(gxj_pixel_type) * hdr->width * hdr->height);
}

/** Clear screen content */
void clearScreen() {
    memset(qvfbPixels, 0, 
	sizeof(gxj_pixel_type) * 
	    hdr->width * hdr->height);
}

/**
 * Resizes system screen buffer to fit the screen dimensions.
 * Call after frame buffer is initialized.
 */
void resizeScreenBuffer(int width, int height) {
    if (gxj_resize_screen_buffer(width, height) != ALL_OK) {
        fprintf(stderr, "Failed to reallocate screen buffer\n");
	    exit(1);
    }
}

/** Check if screen buffer is not bigger than frame buffer device */
static void checkScreenBufferSize(int width, int height) {
    // Check if frame buffer is big enough
    if (hdr->width < width || hdr->height < height) {
        fprintf(stderr, "QVFB screen too small. Need %dx%d\n",
            width, height);
        exit(1);
    }
}

/** Get x-coordinate of screen origin */
int getScreenX(int screenRotated) {
    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int x = 0;
    int LCDwidth = screenRotated ? hdr->height : hdr->width;
    if (LCDwidth > bufWidth) {
        x = (LCDwidth - bufWidth) / 2;
    }
    return x;
}

/** Get y-coordinate of screen origin */
int getScreenY(int screenRotated) {
    int bufHeight = gxj_system_screen_buffer.height;
    int y = 0;
    int LCDheight = screenRotated ? hdr->width : hdr->height;
    if (LCDheight > bufHeight) {
        y = (LCDheight - bufHeight) / 2;
    }
    return y;
}

/** Refresh screen with offscreen bufer content */
void refreshScreenNormal(int x1, int y1, int x2, int y2) {
    // QVFB feature: a number of bytes per line can be different from
    // screenWidth * pixelSize, so lineStep should be used instead.
    int lineStep = hdr->lineStep / sizeof(gxj_pixel_type);
    int dstWidth =  hdr->lineStep / sizeof(gxj_pixel_type);
    gxj_pixel_type *dst  = (gxj_pixel_type *)qvfbPixels;
    gxj_pixel_type *src  = gxj_system_screen_buffer.pixelData;

    int srcWidth = x2 - x1;

    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int bufHeight = gxj_system_screen_buffer.height;

    REPORT_CALL_TRACE4(LC_HIGHUI, "LF:fbapp_refresh(%3d, %3d, %3d, %3d )\n",
                       x1, y1, x2, y2);

    // Check if frame buffer is big enough
    checkScreenBufferSize(bufWidth, bufHeight);

    // Center the LCD output area
    if (hdr->width > bufWidth) {
        dst += (hdr->width - bufWidth) / 2;
    }
    if (hdr->height > bufHeight) {
        dst += (hdr->height - bufHeight) * lineStep / 2;
    }

    src += y1 * bufWidth + x1;
    dst += y1 * dstWidth + x1;

    for (; y1 < y2; y1++) {
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
        src += bufWidth;
        dst += dstWidth;
    }

    hdr->dirty_x1 = 0;
    hdr->dirty_y1 = 0;
    hdr->dirty_x2 = hdr->width;
    hdr->dirty_y2 = hdr->height;
    hdr->is_dirty = 1;
}

/** Refresh rotated screen with offscreen bufer content */
void refreshScreenRotated(int x1, int y1, int x2, int y2) {

    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type *)qvfbPixels;
    int srcWidth, srcHeight;
    int lineStep =  hdr->lineStep / sizeof(gxj_pixel_type);

    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int bufHeight = gxj_system_screen_buffer.height;

    int x;
    int srcInc;
    int dstInc;

    // Check if frame buffer is big enough
    checkScreenBufferSize(bufHeight, bufWidth);

    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    // Center the LCD output area
    if (bufWidth < hdr->height) {
        dst += (hdr->height - bufWidth) / 2 * lineStep;
    }
    if (bufHeight < hdr->width) {
        dst += ((hdr->width - bufHeight) / 2);
    }

    src += x1 + y1 * bufWidth;
    dst += y1 + (bufWidth - x1 - 1) * lineStep;

    srcInc = bufWidth - srcWidth;      // increment for src pointer at the end of row
    dstInc = srcWidth * lineStep + 1;  // increment for dst pointer at the end of column

    while(y1++ < y2) {
        for (x = x1; x < x2; x++) {
            *dst = *src++;
            dst -= lineStep;
         }
         dst += dstInc;
         src += srcInc;
    }

    hdr->dirty_x1 = 0;
    hdr->dirty_y1 = 0;
    hdr->dirty_x2 = hdr->width;
    hdr->dirty_y2 = hdr->height;
    hdr->is_dirty = 1;
}

/** Frees native reources allocated for frame buffer */
void finalizeFrameBuffer() {
    gxj_free_screen_buffer();
}
