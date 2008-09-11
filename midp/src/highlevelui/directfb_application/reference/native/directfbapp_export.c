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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <linux/kd.h>
#include <termios.h>
#include <sys/vt.h>
#include <signal.h>

#include <directfb.h>

#include <kni.h>
#include <gxj_putpixel.h>
#include <midp_logging.h>
#include <midp_constants_data.h>

#include <directfbapp_export.h>

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
#define releaseInterface(x) do { \
        if ((x) != NULL) {(x)->Release(x); (x) = NULL;} \
    } while(0)

/** DirectFB objects */
static IDirectFB *dfb = NULL;
static IDirectFBSurface *screen = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBWindow *window = NULL;
static int screen_width  = 0;
static int screen_height = 0;

/** Gets width of the DirectFB application screen */
int directfbapp_get_screen_width() {
    return screen_width;
}

/** Gets height of the DirectFB application screen */
int directfbapp_get_screen_height() {
    return screen_height;
}

/**
 * Refreshs screen with offscreen buffer content and
 * returns new offscreen buffer for painting
 */
char *directfbapp_refresh(int x1, int y1, int x2, int y2) {
    int pitch;
    char *dst;
    int width;
    DFBRegion reg;

    /* DEBUG: to be deleted after debugging */
    if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 ||
            x1 > screen_width || x2 > screen_width ||
            y1 > screen_height || y2 > screen_height) {
        char b[50];
        sprintf(b, "%d %d %d %d", x1, x2, y1, y2);
        REPORT_ERROR1(LC_LOWUI, "Invalid rectangle for refresh: %s", b);
        // TODO: Should be fixed to return the current back buffer 
        return NULL;
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
    DFBCHECK(screen->Lock(screen, DSLF_WRITE, (void **)(void *)&dst, &pitch));
    if (pitch != (int)sizeof(gxj_pixel_type) * screen_width) {
        REPORT_ERROR(LC_LOWUI,
            "Invalid pixel format: Supports only 16-bit, 5:6:5 display");
        goto dfb_err;
    }
    return dst;
dfb_err:
    return NULL;
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


/** Opens application window and return pointer to offscreen buffer for  */
char *directfbapp_open_window() {

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
    unsigned int win_id;
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
    if ((lconfig.flags & (DLCONF_WIDTH | DLCONF_HEIGHT)) == (DLCONF_WIDTH | DLCONF_HEIGHT)) {
        DFBCHECK(window->GetID(window, &win_id));
        set_win_position(win_id, lconfig.width, lconfig.height, win_x, win_y);
        DFBCHECK(window->MoveTo(window, win_x, win_y));
    }
    DFBCHECK(window->RaiseToTop(window));
    DFBCHECK(window->SetOpacity(window, 0xff));
    DFBCHECK(window->RequestFocus(window));
    DFBCHECK(window->GetSurface(window, &screen));
    DFBCHECK(screen->GetSize(screen, &screen_width, &screen_height));
    DFBCHECK(screen->Lock(screen, DSLF_WRITE, (void**)(void*)&dst, &pitch));
    if (pitch != (int)sizeof(gxj_pixel_type) * screen_width) {
        REPORT_ERROR(LC_LOWUI,
            "Invalid pixel format: Supports only 16-bit, 5:6:5 display");
        goto dfb_err;
    }
    return dst;

dfb_err:;
    directfbapp_finalize();
    exit(1); /* TODO: exit from Java */
    /* return NULL; */
}

/**
 * Closes application window.
 */
void directfbapp_close_window() {
    if (window != NULL) {
        window->Close(window);
        sleep(1); /* wait while the window is destroying */
    }
}

/**
 * Checks for events from keyboard. Gotten event must be retrieved 
 * by <code>directfbapp_get_event</code>.
 * Processes events: DWET_GOTFOCUS and DWET_LOSTFOCUS.
 */
int directfbapp_event_is_waiting() {
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
                    directfbapp_finalize();
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
 * <code>directfbapp_event_is_waiting</code> returned true.
 */
void directfbapp_get_event(void *event) {
    if (directfbapp_event_is_waiting()) {
        DFBCHECK(event_buffer->GetEvent(event_buffer, DFB_EVENT(event)));
        return;
    }
    REPORT_ERROR(LC_LOWUI, "Invalid sequence of calls: no events waiting");
dfb_err:;
}

/** Finalizes native resources of the DirectFB application */
void directfbapp_finalize() {
    directfbapp_close_window();
    releaseInterface(event_buffer);
    releaseInterface(screen);
    releaseInterface(window);
    releaseInterface(dfb);
}

/** Enables events listening for DirectFB application */
void directfbapp_enable_events() {
    DFBCHECK(window->CreateEventBuffer(window, &event_buffer));
    DFBCHECK(window->EnableEvents(window,
        DWET_KEYDOWN | DWET_KEYUP | DWET_CLOSE | DWET_DESTROYED
        /* DEBUG: Request focus */ | DWET_GOTFOCUS | DWET_LOSTFOCUS));
    return;

dfb_err:;
    directfbapp_finalize();
    exit(1); /* TODO: exit from Java */
}
