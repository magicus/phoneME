/*
 *  
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

#ifdef DIRECTFB
#include <linux/kd.h>
#include <termios.h>
#include <sys/vt.h>
#include <signal.h>
#include <directfb.h>
#define DFBCHECK2(x, lab)  \
    do { \
        DFBResult err = (x); \
        if (err != DFB_OK) { \
            REPORT_WARN4(LC_LOWUI, \
                "%s (%d): DFB Error: %s <%d>", \
                __FILE__, __LINE__, \
                DirectFBErrorString(err), err); \
            goto lab; \
        } \
    } while(0)

#define DFBCHECK(x) DFBCHECK2(x, dfb_err)
#define releaseInterface(x) do {if ((x) != NULL) {(x)->Release(x); (x) = NULL;}} while(0)
static void close_directfb();
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

static jboolean reverse_orientation;
/**
* True if we are in full-screen mode; false otherwise.
*/
static int isFullScreen;
static void resizeScreenBuffer();

static void connectFrameBuffer();
static int linuxFbDeviceType;

/* The file descriptor for reading the keyboard. */ 
static int keyboardFd;

/* The file descriptor for reading the mouse. */ 
static int mouseFd;

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
                       strstr(buff, "SHARP Poodle") != NULL ||
                       strstr(buff, "SHARP Shepherd") != NULL) {
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
    int screenSize = sizeof(gxj_pixel_type) * get_screen_width() * get_screen_height();
    isFullScreen = 0;

    gxj_system_screen_buffer.width = get_screen_width();
    gxj_system_screen_buffer.height = get_screen_height();

#ifndef DIRECTFB
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    memset(gxj_system_screen_buffer.pixelData, 0, screenSize);
#else 
    gxj_system_screen_buffer.pixelData = NULL;
#endif

    gxj_system_screen_buffer.alphaData = NULL;

    checkDeviceType();
    connectFrameBuffer();
    resizeScreenBuffer();
}

#define CLIP_RECT(x1, y1, x2, y2)                   \
  do {                                              \
    if ((x1) < 0) { (x1) = 0; }                     \
    if ((y1) < 0) { (y1) = 0; }                     \
                                                    \
    if ((x2) > get_screen_width())  { (x2) = get_screen_width(); }  \
    if ((y2) > get_screen_height()) { (y2) = get_screen_height(); } \
                                                    \
    if ((x1) > (x2)) { (x1) = (x2) = 0; }           \
    if ((y1) > (y2)) { (y1) = (y2) = 0; }           \
  } while (0)

static void fbapp_refresh_normal(int x1, int y1, int x2, int y2);
static void fbapp_refresh_rotate(int x1, int y1, int x2, int y2);

#if !defined(ARM) && !defined(DIRECTFB)

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

    if (hdr->width < get_screen_width() || hdr->height < get_screen_height()) {
        fprintf(stderr, "QVFB screen too small. Need %dx%d\n",
               get_screen_width(), get_screen_height());
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
 * Resizes system screen buffer to fit the screen dimensions.
 * Call after frame buffer is initialized.
 */
static void resizeScreenBuffer() /* QVFB version */
{
    int newWidth = get_screen_width();
    int newHeight = get_screen_height();
    int screenSize = sizeof(gxj_pixel_type) * newWidth * newHeight;
    if (gxj_system_screen_buffer.pixelData != NULL) {
        if ((gxj_system_screen_buffer.width == newWidth) 
            && (gxj_system_screen_buffer.height == newHeight)) {

            memset(gxj_system_screen_buffer.pixelData, 0, screenSize);
            // no need to reallocate buffer, return
            return;

        } else {
            // check if frame buffer is big enough
            if (hdr->width < newWidth || hdr->height < newHeight) {
                fprintf(stderr, "QVFB screen too small. Need %dx%d\n",
                    newWidth, newHeight);
                exit(1);
            }

            // clear frame buffer
            memset(qvfbPixels, 0, sizeof(gxj_pixel_type) * hdr->width * hdr->height);

            // free screen buffer
            midpFree(gxj_system_screen_buffer.pixelData);
            gxj_system_screen_buffer.pixelData = NULL;

        }
    }


    gxj_system_screen_buffer.width = newWidth;
    gxj_system_screen_buffer.height = newHeight;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    gxj_system_screen_buffer.alphaData = NULL;

    memset(gxj_system_screen_buffer.pixelData, 0, screenSize);

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
    if (reverse_orientation) {
        fbapp_refresh_rotate(x1, y1, x2, y2);
        } else {
        fbapp_refresh_normal(x1, y1, x2, y2);
        }
}

static void fbapp_refresh_normal(int x1, int y1, int x2, int y2) /* QVFB version */
{
    // QVFB feature: a number of bytes per line can be different from
    // screenWidth * pixelSize, so lineStep should be used instead.
    int lineStep = hdr->lineStep / sizeof(gxj_pixel_type);
    int dstWidth =  hdr->lineStep / sizeof(gxj_pixel_type);//, dstHeight = hdr->height;
    gxj_pixel_type *dst  = (gxj_pixel_type *)qvfbPixels;
    gxj_pixel_type *src  = gxj_system_screen_buffer.pixelData;

    int srcWidth = x2 - x1;

    REPORT_CALL_TRACE4(LC_HIGHUI, "LF:fbapp_refresh(%3d, %3d, %3d, %3d )\n",
                       x1, y1, x2, y2);

    CLIP_RECT(x1, y1, x2, y2);

    // center the LCD output area
    if (hdr->width > get_screen_width()) {
        dst += (hdr->width - get_screen_width()) / 2;
    }
    if (hdr->height > get_screen_height()) {
        dst += (hdr->height - get_screen_height()) * lineStep / 2;
    }

    src += y1 * get_screen_width() + x1;
    dst += y1 * dstWidth + x1;

    for (; y1 < y2; y1++) {
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
        src += get_screen_width();
        dst += dstWidth;
    }

    hdr->dirty_x1 = 0;
    hdr->dirty_y1 = 0;
    hdr->dirty_x2 = hdr->width;
    hdr->dirty_y2 = hdr->height;
    hdr->is_dirty = 1;
}

static void fbapp_refresh_rotate(int x1, int y1, int x2, int y2) {    /* QVFB version */

    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type *)qvfbPixels;
    int srcWidth, srcHeight;
    int dstWidth =  hdr->lineStep / sizeof(gxj_pixel_type), dstHeight = hdr->height;


    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    CLIP_RECT(x1, y1, x2, y2);

    if (get_screen_width() < dstHeight || get_screen_height() < dstWidth) {
            // We are drawing into a frame buffer that's larger than what MIDP
            // needs. Center it.
            dst += (dstHeight - get_screen_width()) / 2 * dstWidth;
            dst += ((dstWidth - get_screen_height()) / 2);
        }


    dst += y1 + (get_screen_width() - x2 - 1) * dstWidth;
    src += x2-1 + y1 * get_screen_width();

    while( x2-- > x1) {

        int y;

        for (y = y1; y < y2; y++) {

            *dst++ = *src;
            src += get_screen_width();
         }

         dst += dstWidth - srcHeight;
         src += -1 - srcHeight * get_screen_width();

    }

    hdr->dirty_x1 = 0;
    hdr->dirty_y1 = 0;
    hdr->dirty_x2 = hdr->width;
    hdr->dirty_y2 = hdr->height;
    hdr->is_dirty = 1;
}



#endif /* !defined(ARM) */

#ifdef DIRECTFB

static IDirectFB *dfb = NULL;
static IDirectFBSurface *screen = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBWindow *window = NULL;
static int screen_width  = 0;
static int screen_height = 0;

static void initKeyboard() {    /* DIRECTFB version */

    DFBCHECK(window->CreateEventBuffer(window, &event_buffer));
    DFBCHECK(window->EnableEvents(window, 
        DWET_KEYDOWN | DWET_KEYUP | DWET_CLOSE | DWET_DESTROYED
        /* DEBUG: Request focus */ | DWET_GOTFOCUS | DWET_LOSTFOCUS));
    return;
    
dfb_err:;
    close_directfb();
    exit(1); /* TODO: exit from Java */
}

/* This macro calculates a position for new application window.
 * This work must be normally performed by a window manager.
 * IMPL_NOTE: remove or replace it after any wm is being used
 */
#define set_win_position(w_id, width, height, x, y)    \
    do { \
        int w = (width) - CHAM_WIDTH; \
        int h = (height) - CHAM_HEIGHT; \
        if (w > 10 && h > 10) { \
            /* initialize with window ID */ \
            /* IMPL_NOTE: remove if the random is already initialized */ \
            srand(w_id); \
            /* we use high bits because they should be more random */ \
            (x) = (int)(((double)w * rand()) / (RAND_MAX + 1.0)); \
            (y) = (int)(((double)h * rand()) / (RAND_MAX + 1.0)); \
        } else { \
            (x) = 0; \
            (y) = 0; \
        } \
    } while(0) 

static void initFrameBuffer() { /* DIRECTFB version */
    DFBSurfaceDescription dsc;
    DFBWindowDescription wdesc;
    DFBDisplayLayerConfig lconfig;
    static char *argv_array[] = {
        "CVM",
        "--dfb:system=FBDev"
            ",force-windowed"   /* use windows instead of surfaces */
            ",no-vt-switch"     /* do not switch between Linux' VT */
            ",no-cursor"        /* do not use pointer */
            // ",no-deinit-check" /* do not check for deinit */
        ,NULL
    };
    int argc = sizeof argv_array / sizeof argv_array[0] - 1;
    char **argv = argv_array;
    IDirectFBDisplayLayer *dlayer;
    char *dst;
    int pitch;
    int win_id;
    int win_x, win_y;
    
    DFBCHECK(DirectFBInit(&argc, &argv));
    DFBCHECK(DirectFBCreate(&dfb));
    DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_NORMAL));
    
    DFBCHECK(dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &dlayer));
    DFBCHECK(dlayer->GetConfiguration(dlayer, &lconfig));
    wdesc.caps = DWCAPS_DOUBLEBUFFER;
    wdesc.surface_caps = DSCAPS_DOUBLE;
    wdesc.pixelformat = DSPF_RGB16;
    wdesc.width = CHAM_WIDTH;
    wdesc.height = CHAM_HEIGHT;
    wdesc.flags = DWDESC_CAPS | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_PIXELFORMAT |
        DWDESC_SURFACE_CAPS;
    DFBCHECK(dlayer->CreateWindow(dlayer, &wdesc, &window));
    releaseInterface(dlayer);
    if (lconfig.flags & (DLCONF_WIDTH | DLCONF_HEIGHT) == (DLCONF_WIDTH | DLCONF_HEIGHT)) {
        DFBCHECK(window->GetID(window, &win_id));
        set_win_position(win_id, lconfig.width, lconfig.height, win_x, win_y);
        DFBCHECK(window->MoveTo(window, win_x, win_y));
    }
    DFBCHECK(window->RaiseToTop(window));
    DFBCHECK(window->SetOpacity(window, 0xff));
    DFBCHECK(window->RequestFocus(window));
    DFBCHECK(window->GetSurface(window, &screen));
    DFBCHECK(screen->GetSize(screen, &screen_width, &screen_height));
    DFBCHECK(screen->Lock(screen, DSLF_WRITE, (void**)&dst, &pitch));
    if (pitch != (int)sizeof(gxj_pixel_type) * screen_width) {
        REPORT_ERROR(LC_LOWUI, 
            "Invalid pixel format: Supports only 16-bit, 5:6:5 display");
        goto dfb_err;
    }
    gxj_system_screen_buffer.width = screen_width;
    gxj_system_screen_buffer.height = screen_height;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)dst;
    return;
    
dfb_err:;
    close_directfb();
    exit(1); /* TODO: exit from Java */
}

static void connectFrameBuffer() {  /* DIRECTFB version */
    initFrameBuffer();
    initKeyboard();
}

void fbapp_refresh(int x1, int y1, int x2, int y2) {    /* DIRECTFB version */
    int pitch;
    char *dst;
    int width;
    DFBRegion reg, *preg;

    /* DEBUG: to be deleted after debugging */
    if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 ||
            x1 > screen_width || x2 > screen_width ||
            y1 > screen_height || y2 > screen_height) {
        char b[50];
        sprintf(b, "%d %d %d %d", x1, x2, y1, y2);
        REPORT_ERROR1(LC_LOWUI, "Invalid rectangle for refresh: %s", b);
        return;
    }
    
    if (x1 >= x2) {
        width = sizeof(gxj_pixel_type);
        x2 = x1 + 1;
    } else {
        width = (x2 - x1) * sizeof(gxj_pixel_type);
    }
    if (y1 >= y2) {
        y2 = y1 + 1;
    }
    reg.x1 = x1;
    reg.y1 = y1;
    reg.x2 = x2;
    reg.y2 = y2;
    
    DFBCHECK(screen->Unlock(screen));
    DFBCHECK(screen->Flip(screen, &reg, DSFLIP_BLIT));
    DFBCHECK(screen->Lock(screen, DSLF_WRITE, (void**)&dst, &pitch));
    if (pitch != (int)sizeof(gxj_pixel_type) * screen_width) {
        REPORT_ERROR(LC_LOWUI, 
            "Invalid pixel format: Supports only 16-bit, 5:6:5 display");
        goto dfb_err;
    }
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)dst;
dfb_err:;
}

/**
 * Closes application window.
 */
void fbapp_close_window() {
    window->Close(window);
    sleep(1); /* wait while the window is destroying */
}

static void close_directfb() {
    releaseInterface(event_buffer);
    releaseInterface(screen);
    releaseInterface(window);
    releaseInterface(dfb);
}

#endif /* ifdef DIRECTFB */

#if defined(ARM) && !defined(DIRECTFB)

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
      /* IMPL_NOTE - CDC disabled.*/
#if 0 /* Don't draw into the screen area outside of the main midp window. */
        int n;
        gxj_pixel_type *p = fb.data;
        gxj_pixel_type color = (gxj_pixel_type)GXJ_RGB2PIXEL(0xa0, 0xa0, 0x80);
        for (n = w * h; n>0; n--) {
            *p ++ = color;
        }
#endif
    }
}

static void connectFrameBuffer()  /* ARM version */
{
    initKeyboard();
    initFrameBuffer();
}

/**
 * Resizes system screen buffer to fit the screen dimensions.
 * Call after frame buffer is initialized.
 */
static void resizeScreenBuffer() /* ARM version */
{
    int newWidth = get_screen_width();
    int newHeight = get_screen_height();
    int screenSize = sizeof(gxj_pixel_type) * newWidth * newHeight;

    if (gxj_system_screen_buffer.pixelData != NULL) {
        if ((gxj_system_screen_buffer.width == newWidth) 
            && (gxj_system_screen_buffer.height == newHeight)) {

            memset(gxj_system_screen_buffer.pixelData, 0, screenSize);
            // no need to reallocate buffer, return
            return;

        } else {
            // check if frame buffer is big enough
            if (fb.width < newWidth || fb.height < newHeight) {
                fprintf(stderr, "Device screen too small. Need %dx%d\n",
                    newWidth, newHeight);
                exit(1);
            }

            // clear frame buffer
            {
                int n;
                gxj_pixel_type *p = fb.data;
                gxj_pixel_type color = (gxj_pixel_type)GXJ_RGB2PIXEL(0xa0, 0xa0, 0x80);
                for (n = fb.width * fb.height; n>0; n--) {
                    *p ++ = color;
                }
            }

            // free screen buffer
            midpFree(gxj_system_screen_buffer.pixelData);
            gxj_system_screen_buffer.pixelData = NULL;

        }
    }


    gxj_system_screen_buffer.width = newWidth;
    gxj_system_screen_buffer.height = newHeight;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    gxj_system_screen_buffer.alphaData = NULL;

    memset(gxj_system_screen_buffer.pixelData, 0, screenSize);

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
#if 0
        fbapp_refresh_rotate(x1, y1, x2, y2);
#endif
        break;
    case LINUX_FB_INTEL_MAINSTONE:
    case LINUX_FB_VERSATILE_INTEGRATOR:
    case LINUX_FB_OMAP730:
    default:
         if (reverse_orientation) {
            fbapp_refresh_rotate(x1, y1, x2, y2);
         } else {
            fbapp_refresh_normal(x1, y1, x2, y2);
         }
    }
}

static void fbapp_refresh_normal(int x1, int y1, int x2, int y2) /* ARM version */
{
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width, dstHeight = fb.height;

    if (linuxFbDeviceType == LINUX_FB_OMAP730) {
        // Needed by the P2 board
        // Max screen size is 176x220 but can only display 176x208
        dstHeight = get_screen_height();
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

    if (get_screen_width() < dstWidth || get_screen_height() < dstHeight) {
        // We are drawing into a frame buffer that's larger than what MIDP
        // needs. Center it.
        dst += ((dstHeight - get_screen_height()) / 2) * dstWidth;
        dst += (dstWidth - get_screen_width()) / 2;
    }

    if (srcWidth == dstWidth && srcHeight == dstHeight &&
        x1 == 0 && y1 == 0) {
        // copy the entire screen with one memcpy
        memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type) * srcHeight);
    } else {
        src += y1 * get_screen_width() + x1;
        dst += y1 * dstWidth + x1;

        for (; y1 < y2; y1++) {
            memcpy(dst, src, srcWidth * sizeof(gxj_pixel_type));
            src += get_screen_width();
            dst += dstWidth;
        }
    }
}


static void fbapp_refresh_rotate(int x1, int y1, int x2, int y2)  /* ARM version */
{
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)fb.data;
    int srcWidth, srcHeight;
    int dstWidth = fb.width, dstHeight = fb.height;

    srcWidth = x2 - x1;
    srcHeight = y2 - y1;

    CLIP_RECT(x1, y1, x2, y2);

    if (get_screen_width() < dstHeight || get_screen_height() < dstWidth) {
            // We are drawing into a frame buffer that's larger than what MIDP
            // needs. Center it.
            dst += (dstHeight - get_screen_width()) / 2 * dstWidth;
            dst += ((dstWidth - get_screen_height()) / 2);
        }


    dst += y1 + (get_screen_width() - x2 - 1) * dstWidth;
    src += x2-1 + y1 * get_screen_width();

    while( x2-- > x1) {


        int y;

        for (y = y1; y < y2; y++) {

            *dst++ = *src;
            src += get_screen_width();
         }

         dst += dstWidth - srcHeight;
         src += -1 - srcHeight * get_screen_width();

    }

}


#endif /* defined(ARM) && !defined(DIRECTFB) */

#ifndef DIRECTFB
/**
 * Returns the file descriptor for reading the keyboard.
 */
int fbapp_get_keyboard_fd() {
  return keyboardFd;
}
#else
/**
 * Checks for events from keyboard. Gotten event must be retrieved 
 * by <code>fbapp_get_event</code>.
 * Processes events: DWET_GOTFOCUS and DWET_LOSTFOCUS.
 */
int fbapp_event_is_waiting() {
    DFBWindowEvent event;
    
    for (;;) {
        if (event_buffer->HasEvent(event_buffer) == DFB_OK) {
            DFBCHECK(event_buffer->PeekEvent(event_buffer, DFB_EVENT(&event)));
            if (event.type == DWET_KEYUP || event.type == DWET_KEYDOWN) {
                return 1;
            } else {
                DFBCHECK(event_buffer->GetEvent(event_buffer, DFB_EVENT(&event)));
                switch (event.type) {
                case DWET_GOTFOCUS:
                    DFBCHECK2(window->RaiseToTop(window), dfb_err1);
                    DFBCHECK2(window->SetOpacity(window, 0xff), dfb_err1);
                    break;
                case DWET_LOSTFOCUS:
                    DFBCHECK2(window->SetOpacity(window, 0x7f), dfb_err1);
                    break;
                case DWET_DESTROYED:
                    close_directfb();
                    printf("Destroy my window...\n");
                    exit(0); /* IMPL_NOTE: exit from Java */
                    break;
                case DWET_CLOSE:
                    printf("Closing my window...\n");
                    DFBCHECK2(window->Destroy(window), dfb_err1);
                    break;
                default:
                    break;
                }
            }
        } else {
            return 0;
        }
    dfb_err1:;
    }
    
dfb_err:;
    return 0;
}


/**
 * Retrieves next event from queue. Must be called when 
 * <code>fbapp_event_is_waiting</code> returned true.
 */
void fbapp_get_event(void *event) {
    
    if (fbapp_event_is_waiting()) {
        DFBCHECK(event_buffer->GetEvent(event_buffer, DFB_EVENT(event)));
        return;
    }
    REPORT_ERROR(LC_LOWUI, "Invalid sequence of calls: no events waiting");
dfb_err:;
}

#endif /* !defined(DIRECTFB) */
/**
 * Returns the file descriptor for reading the mouse.
 */
int fbapp_get_mouse_fd() {
  return mouseFd;
}

/**
 * Returns the type of the frame buffer device.
 */
int fbapp_get_fb_device_type() {
  return linuxFbDeviceType;
}

/**
 * Invert screen orientation flag
 */
jboolean fbapp_reverse_orientation() {
    reverse_orientation = !reverse_orientation;
    gxj_system_screen_buffer.width = get_screen_width();
    gxj_system_screen_buffer.height = get_screen_height();
    return reverse_orientation; 
}

/*
 * Return screen orientation flag
 */
jboolean fbapp_get_reverse_orientation()
{
    return reverse_orientation;
}        

/**
 * Set full screen mode on/off
 */
void fbapp_set_fullscreen_mode(int mode) {
    if (isFullScreen != mode) {
        isFullScreen = mode;
        resizeScreenBuffer();
    }
}

int get_screen_width() {
    if (reverse_orientation) {
        return (isFullScreen == 1) ? CHAM_FULLHEIGHT : CHAM_HEIGHT;
    } else {
        return (isFullScreen == 1) ? CHAM_FULLWIDTH : CHAM_WIDTH;
    }

}

int get_screen_height() {
    if (reverse_orientation) {
        return (isFullScreen == 1) ? CHAM_FULLWIDTH : CHAM_WIDTH;
    } else {
        return (isFullScreen == 1) ? CHAM_FULLHEIGHT : CHAM_HEIGHT;
    }

}

/**
 * Finalize the fbapp_ native resources.
 */
void fbapp_finalize() {
#if defined(ARM) && !defined(DIRECTFB)
    restoreConsole();
#endif

#ifdef DIRECTFB
    close_directfb();
#endif

#ifdef ENABLE_JSR_184
    engine_uninitialize();
#endif
}
