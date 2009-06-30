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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <kni.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midp_constants_data.h>
#include <midp_foreground_id.h>
#include <midp_input_port.h>

#include <fbapp_export.h>
#include <fbport_export.h>

#if ENABLE_MULTIPLE_DISPLAYS
#include <lcdlf_export.h>
#endif /* ENABLE_MULTIPLE_DISPLAYS */

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

#if ENABLE_ON_DEVICE_DEBUG
static const char pStartOddKeySequence[] = "#1*2";
static int posInSequence = 0;
#endif

/**
  * Indicates screen orientation state,
  * true for rotated screen, false for normal orientation
  */
static jboolean reverse_orientation;

/** True if we are in full-screen mode; false otherwise */
static int isFullScreen;

/** Dynamically evaluated type of the frame buffer device */
static LinuxFbDeviceType linuxFbDeviceType;

/** Detect frame buffer device type of the host Linux system */
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
    isFullScreen = 0;
    reverse_orientation = 0;
    linuxFbDeviceType = LINUX_FB_VERSATILE_INTEGRATOR;

    checkDeviceType();
    initScreenBuffer(
        fbapp_get_screen_width(0), fbapp_get_screen_height(0));
    connectFrameBuffer();
}

/** Returns the file descriptor for reading the mouse. */
int fbapp_get_mouse_fd() {
    return getMouseFd();
}

/** Returns the file descriptor for reading the keyboard. */
int fbapp_get_keyboard_fd() {
    return getKeyboardFd();
}

/**
 * Returns the type of the frame buffer device.
 */
int fbapp_get_fb_device_type() {
  return linuxFbDeviceType;
}

/** Invert screen orientation flag */
jboolean fbapp_reverse_orientation(int hardwareId) {
    (void)hardwareId;
    reverse_orientation = !reverse_orientation;
    reverseScreenOrientation();
    return reverse_orientation;
}

/** Handle clamshell event */
void fbapp_handle_clamshell_event() {
}

/**Set full screen mode on/off */
void fbapp_set_fullscreen_mode(int hardwareId, int mode) {
    if (isFullScreen != mode) {
        isFullScreen = mode;
        resizeScreenBuffer(
            fbapp_get_screen_width(hardwareId),
            fbapp_get_screen_height(hardwareId));
        clearScreen();
    }
}

/** Return screen width */
int fbapp_get_screen_width(int hardwareId) {
    (void)hardwareId;
    if (reverse_orientation) {
        return (isFullScreen == 1) ?
            CHAM_FULLHEIGHT : CHAM_HEIGHT;
    } else {
        return (isFullScreen == 1) ?
            CHAM_FULLWIDTH : CHAM_WIDTH;
    }
}

/** Return screen height */
int fbapp_get_screen_height(int hardwareId) {
    (void)hardwareId;
    if (reverse_orientation) {
        return (isFullScreen == 1) ?
            CHAM_FULLWIDTH : CHAM_WIDTH;
    } else {
        return (isFullScreen == 1) ?
            CHAM_FULLHEIGHT : CHAM_HEIGHT;
    }
}

/** Return screen x */
int fbapp_get_screen_x(int hardwareId) {
    (void)hardwareId;
    return getScreenX(reverse_orientation);
}

/** Return screen x */
int fbapp_get_screen_y(int hardwareId) {
    (void)hardwareId;
    return getScreenY(reverse_orientation);
}

/** Return screen orientation flag */
jboolean fbapp_get_reverse_orientation(int hardwareId) {
    (void)hardwareId;
    return reverse_orientation;
}

/** Clip rectangle requested for refresh according to screen geometry */
static void clipRect(int hardwareId, int *x1, int *y1, int *x2, int *y2) {
    int width = fbapp_get_screen_width(hardwareId);
    int height = fbapp_get_screen_height(hardwareId);

    if (*x1 < 0) { *x1 = 0; }
    if (*y1 < 0) { *y1 = 0; }
    if (*x2 > width)  { *x2 = width; }
    if (*y2 > height) { *y2 = height; }
    if (*x1 > *x2) { *x1 = *x2 = 0; }
    if (*y1 > *y2) { *y1 = *y2 = 0; }
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
void fbapp_refresh(int hardwareId, int x1, int y1, int x2, int y2) {
    clipRect(hardwareId, &x1, &y1, &x2, &y2);
    if (!reverse_orientation) {
        refreshScreenNormal(x1, y1, x2, y2);
    } else {
        refreshScreenRotated(x1, y1, x2, y2);
    }
}

/**
 * Map MIDP keycode value into proper MIDP event parameters
 * and platform signal attributes to unblock Java threads
 * waiting for this input event
 *
 * IMPL_NOTE: In general it is not specific for frame buffer application,
 *   however applications of other type can use rather different events
 *   mappings not based on MIDP keycodes or input keyboard events at all.
 *
 * @param pNewSignal reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent a native MIDP event to be stored to Java event queue
 * @param midpKeyCode MIDP keycode of the input event
 * @param isPressed true if the key is pressed, false if released
 * @param repeatedKeySupport true if MIDP should support repeated key
 *   presses on its own, false if platform supports repeated keys itself
 */
void fbapp_map_keycode_to_event(
        MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent,
        int midpKeyCode, jboolean isPressed, jboolean repeatedKeySupport) {

    switch(midpKeyCode) {
    case KEYMAP_MD_KEY_HOME:
        if (isPressed) {
            pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
            pNewMidpEvent->intParam1 = 0;
            pNewSignal->waitingFor = AMS_SIGNAL;
        } else {
            /* ignore it */
        }
        break;

    case KEYMAP_MD_KEY_SWITCH_APP:
        if (isPressed) {
            pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
            pNewMidpEvent->intParam1 = 1;
            pNewSignal->waitingFor = AMS_SIGNAL;
        } else {
            /* ignore it */
        }
        break;

    case KEYMAP_KEY_SCREEN_ROT:
        if (isPressed) {
            pNewMidpEvent->type = ROTATION_EVENT;
            pNewSignal->waitingFor = UI_SIGNAL;
        } else {
            /* ignore it */
        }
        break;

    case KEYMAP_KEY_VIRT_KEYB:
        if (isPressed) {
            pNewMidpEvent->type = VIRTUAL_KEYBOARD_EVENT;
            pNewSignal->waitingFor = UI_SIGNAL;
        } else {
            /* ignore it */
        }
        break;

    case KEYMAP_KEY_END:
        if (isPressed) {
            pNewSignal->waitingFor = AMS_SIGNAL;
            pNewMidpEvent->type = MIDLET_DESTROY_REQUEST_EVENT;

#if ENABLE_MULTIPLE_DISPLAYS  
            pNewMidpEvent->DISPLAY = gForegroundDisplayIds[lcdlf_get_current_hardwareId()];  
#else  
            pNewMidpEvent->DISPLAY = gForegroundDisplayId;  
#endif /* ENABLE_MULTIPLE_DISPLAYS */  
            pNewMidpEvent->intParam1 = gForegroundIsolateId;
        } else {
            /* ignore it */
        }
        break;

    case KEYMAP_KEY_INVALID:
        /* ignore it */
        break;

    default:
#if ENABLE_ON_DEVICE_DEBUG
        if (isPressed) {
            /* assert(posInSequence == sizeof(pStartOddKeySequence) - 1) */
            if (pStartOddKeySequence[posInSequence] == midpKeyCode) {
                posInSequence++;
                if (posInSequence == sizeof(pStartOddKeySequence) - 1) {
                    posInSequence = 0;
                    pNewSignal->waitingFor = AMS_SIGNAL;
                    pNewMidpEvent->type = MIDP_ENABLE_ODD_EVENT;
                    break;
                }
            } else {
                posInSequence = 0;
            }
        }
#endif
        pNewSignal->waitingFor = UI_SIGNAL;
        pNewMidpEvent->type = MIDP_KEY_EVENT;
        pNewMidpEvent->CHR = midpKeyCode;
        pNewMidpEvent->ACTION = isPressed ? 
            KEYMAP_STATE_PRESSED : KEYMAP_STATE_RELEASED;
        if (repeatedKeySupport) {
            handle_repeated_key_port(midpKeyCode, isPressed);
        }
        break;
    }
}

/**
 * Finalize the fb application native resources.
 */
void fbapp_finalize() {
    clearScreen();
    finalizeFrameBuffer();
}

/** get currently enabled hardware display id */  
int fbapp_get_current_hardwareId() {  
    return 0;  
}  
  
/** 
 * Get display device name by id 
 */  
char * fbapp_get_display_name(int hardwareId) {  
   (void)hardwareId;  
   return 0;  
}  
  
  
/** 
 * Check if the display device is primary 
 */  
jboolean fbapp_is_display_primary(int hardwareId) {  
    (void)hardwareId;
    return KNI_TRUE;

}  
  
/** 
 * Check if the display device is build-in 
 */  
jboolean fbapp_is_display_buildin(int hardwareId) {  
  (void)hardwareId;  
  return KNI_TRUE;
}  
  
/** 
 * Check if the display device supports pointer events 
 */  
jboolean fbapp_is_display_pen_supported(int hardwareId) {
   (void)hardwareId;  
    return KNI_TRUE;
}  
  
/** 
 * Check if the display device supports pointer motion  events 
 */  
jboolean fbapp_is_display_pen_motion_supported(int hardwareId) {  
   (void)hardwareId;  
    return KNI_TRUE;
}  
  
/** 
 * Get display device capabilities 
 */  
int fbapp_get_display_capabilities(int hardwareId) {  
    (void)hardwareId;
    return 255;
}  
  
static jint display_device_ids[] = {0};
  
jint* fbapp_get_display_device_ids(jint* n) {  
    *n = 1; 
    return display_device_ids;
}  
  
void fbapp_display_device_state_changed(int hardwareId, int state) {  
    (void)hardwareId;
    (void)state;
}  
