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
 * Functions to read key events from platform devices
 */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DIRECTFB
#include <directfb.h>
#include <directfbapp_export.h>
#endif

#include <kni.h>
#include <midp_logging.h>
#include <fbapp_export.h>

#include "fb_handle_input.h"
#include "fb_read_key.h"

/** Update input keys state from OMAP730 keypad device */
jboolean read_omap730_key_event() {
    if (!keyState.hasPendingKeySignal) {
        // Platform structure to read key event to
        struct FbKeyEvent {
            struct timeval time;
            unsigned short type;
            unsigned short code;
            unsigned value;
        } fbKeyEvent;
        size_t fbKeyEventSize =
            sizeof(fbKeyEvent);

        int readBytes = read(
            fbapp_get_keyboard_fd(), &fbKeyEvent, fbKeyEventSize);

        if (readBytes < (int)fbKeyEventSize) {
            REPORT_ERROR2(LC_CORE,
                "Invalid key input event, received %d bytes instead of %d",
                readBytes, (int)fbKeyEventSize);
                    return KNI_FALSE;
        }
        keyState.km = mapping;
        keyState.changedBits = fbKeyEvent.value ^ keyState.key;
        keyState.key = fbKeyEvent.value;
    }
    keyState.down = -1;
    return KNI_TRUE;
}

/** Update input keys state reading single char from keyboard device */
jboolean read_char_key_event() {
    unsigned char c;
    int readBytes = read(fbapp_get_keyboard_fd(), &c, sizeof(c));
    keyState.key = c;
    keyState.down = -1;
    return (readBytes > 0) ?
        KNI_TRUE : KNI_FALSE;
}

#ifdef DIRECTFB
/** Update input keys state from DirectFB keyboard device */
jboolean read_directfb_key_event() {
    DFBWindowEvent dfbEvent;
    directfbapp_get_event(&dfbEvent);

    // IMPL_NOTE: DirectFB sends key-up codes via DWET_KEYUP
    //   event with ordinary (like key-down) key code
    keyState.down = (dfbEvent.type != DWET_KEYUP);

    keyState.changedBits = dfbEvent.key_code ^ keyState.key;
    keyState.key = dfbEvent.key_code;
    return KNI_TRUE;
}
#endif
