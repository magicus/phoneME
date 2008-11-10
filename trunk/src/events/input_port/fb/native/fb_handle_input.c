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
 * Utility functions to handle received system signals
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <jvm.h>
#include <midpServices.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <fbapp_export.h>
#include <keymap_input.h>
#include <timer_queue.h>
#include <midp_logging.h>

#ifdef DIRECTFB
#include <directfb.h>
#include <directfbapp_export.h>
#endif

#include "fb_handle_input.h"
#include "fb_keymapping.h"
#include "fb_read_key.h"

/* IMPL_NOTE: OMAP730 keyboard events are bit-based. Thus single native
     event can produce several MIDP ones. This flag indicates there are
     one or more key bits still not converted into MIDP events */

/** Implement input keys state */
InputKeyState keyState;

/** Function to read key events from a platform input device */
static fReadKeyEvent readKeyEvent = NULL;

/** Handle key repeat timer alarm on timer wakeup */
static void handle_repeat_key_timer_alarm(TimerHandle *timer) {
    static MidpEvent newMidpEvent;
    static int midp_keycode;
    if (timer != NULL) {
        midp_keycode = (int)(get_timer_data(timer));

        newMidpEvent.type = MIDP_KEY_EVENT;
        newMidpEvent.CHR = midp_keycode;
        newMidpEvent.ACTION = KEYMAP_STATE_REPEATED;

        midpStoreEventAndSignalForeground(newMidpEvent);

        timer = remove_timer(timer);
        set_timer_wakeup(timer, get_timer_wakeup(timer) + REPEAT_PERIOD);
        add_timer(timer);
    }
}

/**
 * Support repeated key presses for a platforms with no own support for it.
 *
 * One of the possible implementations is timer-based generation of MIDP
 * events for a keys pressed and not released for a certain time interval.
 *
 * @param midpKeyCode MIDP keycode of the pressed/released key
 * @param isPressed true if the key is pressed, false if released
 */
void handle_repeated_key_port(int midpKeyCode, jboolean isPressed) {
    if (isPressed) {
        jlong current_time = JVM_JavaMilliSeconds();
        new_timer(current_time + REPEAT_TIMEOUT,
            (void*)midpKeyCode, handle_repeat_key_timer_alarm);
    } else {
        delete_timer_by_userdata((void*)midpKeyCode);
    }
}

/**
 * Set key mapping, input device mode and function to read
 * platform key events depending on input device type 
 */
static void init_key_device() {
    static int done = 0;
    if (!done) {
        done = KNI_TRUE;
        memset(&keyState, 0, sizeof(keyState));
        switch (fbapp_get_fb_device_type()) {
        case LINUX_FB_OMAP730:
            mapping = omap_730_keys;
            bitscale_mode = KNI_TRUE;
            readKeyEvent = read_omap730_key_event;
            break;
        case LINUX_FB_ZAURUS:
            mapping = zaurus_sl5500_keys;
            bitscale_mode = KNI_FALSE;
            readKeyEvent = read_char_key_event;
            break;
        case LINUX_FB_VERSATILE_INTEGRATOR:
        default:
            mapping = versatile_integrator_keys;
            bitscale_mode = KNI_FALSE;
            readKeyEvent = read_char_key_event;
            break;
        }
#ifdef DIRECTFB
        /* DirectFB provides generic function to read input key events */
        readKeyEvent = read_directfb_key_event;
#endif
    }
}

/**
 * Find key mapping entry by raw key value regarding the bits
 * changed since the previous check
 *
 */
static void find_raw_key_mapping() {
    int down = 0;

    /* cache a few state fields for brewity */
    unsigned changedBits = keyState.changedBits;
    unsigned key = keyState.key;
    KeyMapping *km;

    /* find MIDP key code corresponding to native key */
    if (bitscale_mode) {
        /* key is bit-mask, seach for mapping entry */
        for (km = keyState.km; km->midp_keycode != KEYMAP_KEY_INVALID &&
                ((changedBits & km->raw_keydown) == 0); km++)
            /* break at the first newly pressed/released key */
            ;

        /* check if found the corresponding entry */
        if (km->midp_keycode != KEYMAP_KEY_INVALID) {
            down = key & km->raw_keydown;
            changedBits &= ~km->raw_keydown;
        } else {
            /* no entry found, reset search */
            changedBits = 0;
        }
    } else {
        /* key is key code */
        for (km = mapping; km->midp_keycode != KEYMAP_KEY_INVALID; km++) {
            if (km->raw_keydown == key) {
                down = 1;
                break;
            } else if (km->raw_keyup == key) {
                down = 0;
                break;
            }
        }
    }

    /* update keys state */
    keyState.km = km;
    keyState.changedBits = changedBits;
    keyState.hasPendingKeySignal = (changedBits != 0);

    /* down key state could be set by read function */
    if (keyState.down < 0) {
        keyState.down = down;
    }
}

/**
 * ARM/DIRECTFB version to read keyboard events from /dev/tty0.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_key_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    int midpKeyCode;
    jboolean isPressed;
    jboolean repeatSupport;

    init_key_device();
    if (readKeyEvent()) {
        find_raw_key_mapping();

        midpKeyCode = keyState.km->midp_keycode;
        isPressed = keyState.down ? KNI_TRUE : KNI_FALSE;
        repeatSupport = KNI_TRUE;

        fbapp_map_keycode_to_event(
            pNewSignal, pNewMidpEvent,
            midpKeyCode, isPressed, repeatSupport);
   }
}

/**
 * ARM version to read mouse events.
 * By default it's empty because currently it does not supported pointer events
 * It should be implemented if required
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_pointer_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    (void)pNewSignal;
    (void)pNewMidpEvent;
}

/**
 * An input devices can produce bit-based keyboard events. Thus single
 * native event can produce several MIDP ones. This function detects
 * whether are one or more key bits still not converted into MIDP events
 *
 * @return true when pending key exists, false otherwise
 */
jboolean has_pending_key_port() {
    return bitscale_mode ? 
        keyState.hasPendingKeySignal :
        KNI_FALSE;
}
