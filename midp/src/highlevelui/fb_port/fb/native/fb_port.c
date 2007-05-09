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
 * Implementation of the porting layer for generic fb application
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

/* The following is needed for accessing /dev/fb */
#include <linux/fb.h>

/* The following is needed for accessing /dev/tty? in RAW mode */
#include <linux/kd.h>
#include <termios.h>
#include <sys/vt.h>
#include <signal.h>
#ifdef VESA_NO_BLANKING
#include <linux/input.h>
#endif

#include <gxj_putpixel.h>
#include <gxj_screen_buffer.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midp_constants_data.h>

#include <fbapp_device_type.h>
#include <fbapp_export.h>
#include "fb_port.h"

/**
 * By default use fast copying of rotated pixel data.
 * The assumptions required by fast copying implementation are supposed.
 */
#ifndef ENABLE_FAST_COPY_ROTATED
#define ENABLE_FAST_COPY_ROTATED    1
#endif

/** @def PERROR Prints diagnostic message. */
#define PERROR(msg) REPORT_ERROR2(0, "%s: %s", msg, strerror(errno))

/** The file descriptor for reading the mouse */
static int mouseFd = -1;
/** The file descriptor for reading the keyboard */
static int keyboardFd = -1;

/** Cached Linux/FrameBuffer device type retrieved from fb application */
static LinuxFbDeviceType linuxFbDeviceType =
    LINUX_FB_VERSATILE_INTEGRATOR; 

/** Return file descriptor of keyboard device, or -1 in none */
int getKeyboardFd() {
    return keyboardFd;
}
/** Return file descriptor of mouse device, or -1 in none */
int getMouseFd() {
    return mouseFd;
}

/** System offscreen buffer */
gxj_screen_buffer gxj_system_screen_buffer;

static struct termios origTermData;
static struct termios termdata;

/** Allocate system screen buffer according to the screen geometry */
void initScreenBuffer(int width, int height) {
    if (gxj_init_screen_buffer(width, height) != ALL_OK) {
        fprintf(stderr, "Failed to allocate screen buffer\n");
	    exit(1);
    }
}

/**
  * Retrieve Linux/Fb device type from the fb application.
  * Note: The methods introduces cyclic dependencies between fb_port and
  * fb_application libraries to not replicate the method checkDeviceType()
  * for each fb_port library implementation.
  */
static void initLinuxFbDeviceType() {
    linuxFbDeviceType = fbapp_get_fb_device_type();
}

/** Inits keyboard device */
static void initKeyboard() {
    struct vt_stat vtStat;
    char dev[30];

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        sprintf(dev,"%s", "/dev/input/event0");
        keyboardFd = open(dev, O_RDONLY | O_NDELAY, 0);

        if (keyboardFd < 0) {
            fprintf(stderr, "Failed to open %s\n", dev);
            return;
        }
    } else {
        sprintf(dev,"%s", "/dev/tty0");
        keyboardFd = open(dev, O_RDWR | O_NDELAY, 0);

        if (keyboardFd < 0) {
            fprintf(stderr, "Failed to open %s\n", dev);
            return;
        }

        // save for restore
        ioctl(keyboardFd, VT_GETSTATE, &vtStat);

        // Switch to virtual terminal 7, so that we won't be competing for
        // keyboard with the getty process. This is similar to X11.
        ioctl(keyboardFd, VT_ACTIVATE, 7);
        close(keyboardFd);
        keyboardFd = open(dev, O_RDWR | O_NDELAY, 0);

        tcgetattr(keyboardFd, &origTermData);
        tcgetattr(keyboardFd, &termdata);

        // Disable cursor
        ioctl(keyboardFd, KDSETMODE, KD_GRAPHICS);
        // Set keyboard to RAW mode
        ioctl(keyboardFd, KDSKBMODE, K_RAW);

        termdata.c_iflag     = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
        termdata.c_oflag     = 0;
        termdata.c_cflag     = CREAD | CS8;
        termdata.c_lflag     = 0;
        termdata.c_cc[VTIME] = 0;
        termdata.c_cc[VMIN]  = 0;
        cfsetispeed(&termdata, 9600);
        cfsetospeed(&termdata, 9600);
        if (tcsetattr(keyboardFd, TCSANOW, &termdata) < 0) {
            PERROR("tcsetattr" );
        }
    }
}

/** Frees and close grabbed keyboard device */
static void restoreConsole() {
    if (keyboardFd >= 0) {
        ioctl(keyboardFd, KDSKBMODE, K_XLATE);
        ioctl(keyboardFd, KDSETMODE, KD_TEXT);
        ioctl(keyboardFd, VT_ACTIVATE, 1);
        tcsetattr(keyboardFd, TCSANOW, &origTermData);
        close(keyboardFd);
        keyboardFd = -1;
    }
}

/** Inits frame buffer device */
void initFrameBuffer() {
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    const char *dev = "/dev/fb0";
    int fd;
    int dw, w, dh, h;

    fd = open(dev, O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Can't open framebuffer device %s\n", dev);
        exit(1);
    }

#ifdef VESA_NO_BLANKING
    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        // Disable the screen from powering down
        ioctl(fd, FBIOBLANK, VESA_NO_BLANKING);
    }
#endif

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) ||
        ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        PERROR("reading /dev/fb0");
        exit(1);
    }

    fb.depth = vinfo.bits_per_pixel;
    fb.lstep = finfo.line_length;
    fb.xoff  = vinfo.xoffset;
    fb.yoff  = vinfo.yoffset;
    fb.width = vinfo.xres;
    fb.height= vinfo.yres;

    if (fb.depth != 16) {
        fprintf(stderr, "Supports only 16-bit, 5:6:5 display\n");
        exit(1);
    }

    dw = w = vinfo.xres;
    dh = h = vinfo.yres;

    fb.dataoffset = fb.yoff * fb.lstep + fb.xoff * fb.depth / 8;
    fb.mapsize = finfo.smem_len;

    fb.data = (unsigned short *)mmap(0, fb.mapsize,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    fb.data += fb.dataoffset;

    if ((int)fb.data == -1) {
        PERROR("mapping /dev/fb0");
        exit(1);
    } else {
      /* IMPL_NOTE - CDC disabled.*/
#if 0 /* Don't draw into the screen area outside of the main midp window. */
        int n;
        gxj_pixel_type *p = fb.data;
        gxj_pixel_type color =
            (gxj_pixel_type)GXJ_RGB2PIXEL(0xa0, 0xa0, 0x80);
        for (n = w * h; n > 0; n--) {
            *p ++ = color;
        }
#endif
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

/** Initialize frame buffer video device */
void connectFrameBuffer() {
    initLinuxFbDeviceType();
    initKeyboard();
    initFrameBuffer();
}

/** Clear screen content */
void clearScreen() {
    int n;
    gxj_pixel_type *p = fb.data;
    gxj_pixel_type color =
	(gxj_pixel_type)GXJ_RGB2PIXEL(0xa0, 0xa0, 0x80);
    for (n = fb.width * fb.height; n > 0; n--) {
	    *p ++ = color;
    }
}

/**
 * Resizes system screen buffer to fit new screen geometry
 * and set system screen dimensions accordingly.
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
    if (fb.width < width || fb.height < height) {
        fprintf(stderr, "Device screen too small. Need %dx%d\n",
            width, height);
        exit(1);
    }
}


/** Get x-coordinate of screen origin */
int getScreenX(int screenRotated) {
    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int x = 0;
    int LCDwidth = screenRotated ? fb.height : fb.width;
    if (LCDwidth > bufWidth) {
        x = (LCDwidth - bufWidth) / 2;
    }
    return x;
}

/** Get y-coordinate of screen origin */
int getScreenY(int screenRotated) {
    int bufHeight = gxj_system_screen_buffer.height;
    int y = 0;
    int LCDheight = screenRotated ? fb.width : fb.height;
    if (LCDheight > bufHeight) {
        y = (LCDheight - bufHeight) / 2;
    }
    return y;
}

/** Refresh screen from offscreen buffer */
void refreshScreenNormal(int x1, int y1, int x2, int y2) {
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width;
    int dstHeight = fb.height;

    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int bufHeight = gxj_system_screen_buffer.height;

    // Check if frame buffer is big enough
    checkScreenBufferSize(bufWidth, bufHeight);    

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        // Needed by the P2 board
        // Max screen size is 176x220 but can only display 176x208
        dstHeight = bufHeight;
    }

    // Make sure the copied lines are 4-byte aligned for faster memcpy
    if ((x1 & 1) == 1) x1 -= 1;
    if ((x2 & 1) == 1) x2 += 1;
    
    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    if (bufWidth < dstWidth || bufHeight < dstHeight) {
        // We are drawing into a frame buffer that's larger than what MIDP
        // needs. Center it.
        dst += ((dstHeight - bufHeight) / 2) * dstWidth;
        dst += (dstWidth - bufWidth) / 2;
    }

    if (srcWidth == dstWidth && srcHeight == dstHeight &&
        x1 == 0 && y1 == 0) {
        // copy the entire screen with one memcpy
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type) * srcHeight);
    } else {
        src += y1 * bufWidth + x1;
        dst += y1 * dstWidth + x1;

        for (; y1 < y2; y1++) {
            memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
            src += bufWidth;
            dst += dstWidth;
        }
    }
}

#if ENABLE_FAST_COPY_ROTATED
/**
 * Fast rotated copying of screen buffer area to the screen memory.
 * The copying is optimized for 32bit architecture with read caching
 * to copy 16bit pixel data with 90 CCW rotation.
 *
 * The data is copied by 2x2 pixel blocks to operate with machine words.
 * Source data is traversed by lines to benefit from read caching.
 *
 * IMPL_NOTE ASSUMPTIONS:
 * 1) Source and target buffers are word aligned
 * 2) Source and target widths are even, so column pixels are equally aligned
 *
 * @param src pointer to source pixel data to start copying from
 * @param dst pointer to destination pixel data to start copying to
 * @param x1 x-coordinate of the left upper corner of the copied area
 * @param y1 y-coordinate of the left upper corner of the copied area
 * @param x2 x-coordinate of the right lower corner of the copied area
 * @param y2 y-coordinate of the right lower corner of the copied area
 * @param bufWidth width of the source screen buffer
 * @param dstWidth width of the screen
 * @param srcInc source pointer increment at the end of source row
 * @param dstInc dest pointer increment at the end of source row
 */
static void fast_copy_rotated(short *src, short *dst, int x1, int y1, int x2, int y2,
        int bufWidth, int dstWidth, int srcInc, int dstInc) {

  int x;
  unsigned a, b;
  while(y1 < y2) {
    y1 += 2;
    for (x = x1; x < x2; x += 2) {
      a = *(unsigned*)src;
      b = *(unsigned*)(src + bufWidth);
      src += 2;
      
      *(unsigned*)dst = (b << 16) | (a & 0x0000ffff);
      dst -= dstWidth;
      *(unsigned*)dst = (b & 0xffff0000) | (a >> 16);
      dst -= dstWidth;
    }
    dst += dstInc + 1;
    src += srcInc + bufWidth;
  }
}

#else /* ENABLE_FAST_COPY_ROTATED */
/**
 * Simple rotated copying of screen buffer area to the screen memory.
 * Source data is traversed by lines to benefit from read caching,
 * the target data is rotated by 90 CCW.
 *
 * IMPL_NOTE:
 *   The method can be used if IMPL_NOTE ASSUMPTIONS for fast copy
 *   routine can not be satisfied. On ARM 201MHz fast copying is about
 *   2x better than the simple one.
 *
 * @param src pointer to source pixel data to start copying from
 * @param dst pointer to destination pixel data to start copying to
 * @param x1 x-coordinate of the left upper corner of the copied area
 * @param y1 y-coordinate of the left upper corner of the copied area
 * @param x2 x-coordinate of the right lower corner of the copied area
 * @param y2 y-coordinate of the right lower corner of the copied area
 * @param bufWidth width of the source screen buffer
 * @param dstWidth width of the screen
 * @param srcInc source pointer increment at the end of source row
 * @param dstInc dest pointer increment at the end of source row
 */
static void simple_copy_rotated(short *src, short *dst, int x1, int y1, int x2, int y2,
		int bufWidth, int dstWidth, int srcInc, int dstInc) {

    int x;
    (void)bufWidth;
    while(y1++ < y2) {
        for (x = x1; x < x2; x++) {
            *dst = *src++;
            dst -= dstWidth;
         }
         dst += dstInc;
         src += srcInc;
    }
}
#endif /* ENABLE_FAST_COPY_ROTATED */

/** Refresh rotated screen with offscreen buffer content */
void refreshScreenRotated(int x1, int y1, int x2, int y2) {

    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width;
    int dstHeight = fb.height;

    int srcInc;
    int dstInc;

    // System screen buffer geometry
    int bufWidth = gxj_system_screen_buffer.width;
    int bufHeight = gxj_system_screen_buffer.height;

    // Check if frame buffer is big enough
    checkScreenBufferSize(bufHeight, bufWidth);

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        // Needed by the P2 board
        // Max screen size is 176x220 but can only display 176x208
        dstHeight = bufWidth;
    }

    // Make sure the copied lines are 4-byte aligned for faster memcpy
    if ((x1 & 1) == 1) x1 -= 1;
    if ((x2 & 1) == 1) x2 += 1;
    if ((y1 & 1) == 1) y1 -= 1;
    if ((y2 & 1) == 1) y2 += 1;

    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    if (bufWidth < dstHeight || bufHeight < dstWidth) {
            // We are drawing into a frame buffer that's larger than what MIDP
            // needs. Center it.
            dst += (dstHeight - bufWidth) / 2 * dstWidth;
            dst += ((dstWidth - bufHeight) / 2);
        }

    src += x1 + y1 * bufWidth;
    dst += y1 + (bufWidth - x1 - 1) * dstWidth;

    srcInc = bufWidth - srcWidth;      // increment for src pointer at the end of row
    dstInc = srcWidth * dstWidth + 1;  // increment for dst pointer at the end of column

#if ENABLE_FAST_COPY_ROTATED
    fast_copy_rotated(src, dst, x1, y1, x2, y2,
        bufWidth, dstWidth, srcInc, dstInc);
#else
    simple_copy_rotated(src, dst, x1, y1, x2, y2,
        bufWidth, dstWidth, srcInc, dstInc);
#endif
}

/** Frees allocated resources and restore system state */
void finalizeFrameBuffer() {
    gxj_free_screen_buffer();
    restoreConsole();
}
