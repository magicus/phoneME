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

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <kni.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midp_constants_data.h>
#include <gxj_putpixel.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef ARM
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
#endif

#ifdef ENABLE_JSR_184
#include <swvapi.h>
#endif

/** @def PERROR Prints diagnostic message. */
#define PERROR(msg) REPORT_ERROR2(0, "%s: %s", msg, strerror(errno))

#include <fbapp_export.h>

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */


gxj_screen_buffer gxj_system_screen_buffer;

static void connectFrameBuffer();
static int linuxFbDeviceType;

/* The file descriptor for reading the keyboard. */ 
static int keyboardFd;

static void checkDeviceType() {
    char buff[1000];
    int fd;
    linuxFbDeviceType = LINUX_FB_VERSATILE_INTEGRATOR; /* default */

    memset(buff, 0, sizeof(buff));

    if ((fd = open("/proc/cpuinfo", O_RDONLY, 0)) >= 0) {
        if (read(fd, buff, sizeof(buff)-1) > 0) {
            if (strstr(buff, "ARM-Integrator") != NULL ||
                strstr(buff, "ARM-Versatile") != NULL) {
                linuxFbDeviceType = LINUX_FB_VERSATILE_INTEGRATOR;
            } else if (strstr(buff, "Sharp-Collie") != NULL ||
                       strstr(buff, "SHARP Poodle") != NULL) {
                linuxFbDeviceType = LINUX_FB_ZAURUS;
            } else if (strstr(buff, "Intel DBBVA0 Development") != NULL) {
                linuxFbDeviceType = LINUX_FB_INTEL_MAINSTONE;
            } else if (strstr(buff, "TI-Perseus2/OMAP730") != NULL) {
                linuxFbDeviceType = LINUX_FB_OMAP730;
            } else {
                /* plug in your device type here */
            }
        }
        close(fd);
    }
}

/**
 * Initializes the fbapp_ native resources.
 */
void fbapp_init() {
    int screenSize = sizeof(gxj_pixel_type) * CHAM_WIDTH * CHAM_HEIGHT;

    gxj_system_screen_buffer.width = CHAM_WIDTH;
    gxj_system_screen_buffer.height = CHAM_HEIGHT;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    gxj_system_screen_buffer.alphaData = NULL;

    memset(gxj_system_screen_buffer.pixelData, 0, screenSize);

    checkDeviceType();
    connectFrameBuffer();

#ifdef ENABLE_JSR_184
    engine_initialize();
#endif
}

#define CLIP_RECT(x1, y1, x2, y2)                   \
  do {                                              \
    if ((x1) < 0) { (x1) = 0; }                     \
    if ((y1) < 0) { (y1) = 0; }                     \
                                                    \
    if ((x2) > CHAM_WIDTH)  { (x2) = CHAM_WIDTH; }  \
    if ((y2) > CHAM_HEIGHT) { (y2) = CHAM_HEIGHT; } \
                                                    \
    if ((x1) > (x2)) { (x1) = (x2) = 0; }           \
    if ((y1) > (y2)) { (y1) = (y2) = 0; }           \
  } while (0)

#ifndef ARM

typedef struct _QVFbHeader {
    int width;
    int height;
    int depth;
    int lineStep;
    int dataOffset;
    int dirty_x1;
    int dirty_y1;
    int dirty_x2;
    int dirty_y2;
    int is_dirty;
} QVFbHeader;


static QVFbHeader *hdr;
static gxj_pixel_type *qvfbPixels;

/**
 * On i386, connect to the QVFB virtual frame buffer
 */
static void connectFrameBuffer() {
    int displayId = 0;
    char buff[30];
    key_t key;
    int shmId;
    unsigned char *shmrgn;
    char *env;

    if ((env = getenv("QWS_DISPLAY")) != NULL) {
        displayId = atoi(env + 1); /* skip the leading colon */
    }

    sprintf(buff, "/tmp/.qtvfb_mouse-%d", displayId);
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

    if (hdr->width < CHAM_WIDTH || hdr->height < CHAM_HEIGHT) {
        fprintf(stderr, "QVFB screen too small. Need %dx%d\n",
               CHAM_WIDTH, CHAM_HEIGHT);
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

/**
 * Bridge function to request a repaint
 * of the area specified.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void fbapp_refresh(int x1, int y1, int x2, int y2) /* QVFB version */
{
    int y;
    gxj_pixel_type *dstpixels = (gxj_pixel_type *)qvfbPixels;
    // QVFB feature: a number of bytes per line can be different from
    // screenWidth * pixelSize, so lineStep should be used instead.
    int lineStep = hdr->lineStep / sizeof(gxj_pixel_type);
    gxj_pixel_type *dst;
    gxj_pixel_type *src;
    REPORT_CALL_TRACE4(LC_HIGHUI, "LF:fbapp_refresh(%3d, %3d, %3d, %3d )\n",
                       x1, y1, x2, y2);

    CLIP_RECT(x1, y1, x2, y2);

    // center the LCD output area
    if (hdr->width > CHAM_WIDTH) {
        dstpixels += (hdr->width - CHAM_WIDTH) / 2;
    }
    if (hdr->height > CHAM_HEIGHT) {
        dstpixels += (hdr->height - CHAM_HEIGHT) * lineStep / 2;
    }

    // Copy the pixels
    for (y=y1; y < y2; y++) {
        src = &gxj_system_screen_buffer.pixelData[y*gxj_system_screen_buffer.width + x1];
        dst = &dstpixels[y * lineStep + x1];

        memcpy(dst, src, (x2 - x1) * sizeof(gxj_pixel_type));
    }

    hdr->dirty_x1 = 0;
    hdr->dirty_y1 = 0;
    hdr->dirty_x2 = hdr->width;
    hdr->dirty_y2 = hdr->height;
    hdr->is_dirty = 1;
}

#endif /* !defined(ARM) */

#ifdef ARM

static void fbapp_refresh_normal(int x1, int y1, int x2, int y2);
static void fbapp_refresh_rotate(int x1, int y1, int x2, int y2);

static struct termios origTermData;
static struct termios termdata;

static void initKeyboard() {

    struct vt_stat vtStat;
    char dev[30];

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        sprintf(dev,"%s", "/dev/input/event0");
        keyboardFd = open(dev, O_RDONLY | O_NDELAY, 0);

        if (keyboardFd < 0) {
            fprintf(stderr, "failed to open %s\n", dev);
            return;
        }
    } else {
        sprintf(dev,"%s", "/dev/tty0");
        keyboardFd = open(dev, O_RDWR | O_NDELAY, 0);

        if (keyboardFd < 0) {
            fprintf(stderr, "failed to open %s\n", dev);
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

static void restoreConsole()
{
    if (keyboardFd >= 0) {
        ioctl(keyboardFd, KDSKBMODE, K_XLATE);
        ioctl(keyboardFd, KDSETMODE, KD_TEXT);
        ioctl(keyboardFd, VT_ACTIVATE, 1);
        tcsetattr(keyboardFd, TCSANOW, &origTermData);
        close(keyboardFd);
        keyboardFd = -1;
    }
}


struct {
    unsigned short *data;
    int width;
    int height;
    int depth;
    int lstep;
    int xoff;
    int yoff;
    int dataoffset;
    int mapsize;
} fb;

static void initFrameBuffer() {
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

    fb.data = (unsigned short *)mmap(0, fb.mapsize, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, fd, 0);
    fb.data += fb.dataoffset;

    if ((int)fb.data == -1) {
        PERROR("mapping /dev/fb0");
        exit(1);
    } else {
        int n;
        gxj_pixel_type *p = fb.data;
        gxj_pixel_type color = (gxj_pixel_type)GXJ_RGB2PIXEL(0xa0, 0xa0, 0x80);
        for (n = w * h; n>0; n--) {
            *p ++ = color;
        }
    }
}

static void connectFrameBuffer()  /* ARM version */
{
    initKeyboard();
    initFrameBuffer();
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
void fbapp_refresh(int x1, int y1, int x2, int y2) /* ARM version */
{
    CLIP_RECT(x1, y1, x2, y2);

    switch (linuxFbDeviceType) {
    case LINUX_FB_ZAURUS:
        fbapp_refresh_rotate(x1, y1, x2, y2);
        break;
    case LINUX_FB_INTEL_MAINSTONE:
    case LINUX_FB_VERSATILE_INTEGRATOR:
    case LINUX_FB_OMAP730:
    default:
        fbapp_refresh_normal(x1, y1, x2, y2);
    }
}

static void fbapp_refresh_normal(int x1, int y1, int x2, int y2)
{
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width, dstHeight = fb.height;

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        // Needed by the P2 board
        // Max screen size is 176x220 but can only display 176x208
        dstHeight = CHAM_HEIGHT;
    }

    // Make sure the copied lines are 4-byte aligned for faster memcpy
    if ((x1 & 2) == 1) {
        x1 -= 1;
    }
    if ((x2 & 2) == 1) {
        x2 += 1;
    }
    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    if (CHAM_WIDTH < dstWidth || CHAM_HEIGHT < dstHeight) {
        // We are drawing into a frame buffer that's larger than what MIDP
        // needs. Center it.
        dst += ((dstHeight - CHAM_HEIGHT) / 2) * dstWidth;
        dst += (dstWidth - CHAM_WIDTH) / 2;
    }

    if (srcWidth == dstWidth && srcHeight == dstHeight &&
        x1 == 0 && y1 == 0) {
        // copy the entire screen with one memcpy
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type) * srcHeight);
    } else {
        src += y1 * CHAM_WIDTH + x1;
        dst += y1 * dstWidth + x1;

        for (; y1 < y2; y1++) {
            memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
            src += CHAM_WIDTH;
            dst += dstWidth;
        }
    }
}

// Rotate the display bt 90 degrees clock-wise (Zaurus).
// IMPL NOTE: there's must be a way to flip it in hardware, but let's
// write a software copy-loop just for fun!
static void fbapp_refresh_rotate(int x1, int y1, int x2, int y2)
{
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width, dstHeight = fb.height;

    // Force 2-byte alignment for faster writes
    if ((y1 & 2) == 1) {
        y1 -= 1;
    }
    if ((y2 & 2) == 1) {
        y2 += 1;
    }

    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    if (CHAM_WIDTH < dstHeight || CHAM_HEIGHT < dstWidth) {
        // We are drawing into a frame buffer that's larger than what MIDP
        // needs. Center it.
        dst += (dstHeight - CHAM_WIDTH) / 2 * dstWidth;
        dst += ((dstWidth - CHAM_HEIGHT) / 2);
    }

    dst += x1 * dstWidth + (CHAM_HEIGHT - y2);

    for (; x1 < x2; x1++) {
        gxj_pixel_type *s    = src + ((y2-1) * CHAM_WIDTH) + x1;
        gxj_pixel_type *sEnd = src + ( y1    * CHAM_WIDTH) + x1;
        gxj_pixel_type *d    = dst;

        while (s >= sEnd) {
            *d++ = *s;
            s -= CHAM_WIDTH;
        }

        dst += dstWidth;
    }
}

#endif /* defined(ARM) */

/**
 * Returns the file descriptor for reading the keyboard. 
 */
int fbapp_get_keyboard_fd() {
  return keyboardFd;
}

/**
 * Returns the type of the frame buffer device.
 */
int fbapp_get_fb_device_type() {
  return linuxFbDeviceType;
}

/**
 * Finalize the fbapp_ native resources.
 */
void fbapp_finalize() {
#ifdef ARM
    restoreConsole();
#endif

#ifdef ENABLE_JSR_184
    engine_uninitialize();
#endif
}
