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
 * Utility functions to handle QVFb input events
 */

#include <kni.h>

#include <sys/time.h> 
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <midpServices.h>
#include <midpEvents.h> 
#include <midpEventUtil.h> 
#include <fbapp_export.h>
#include <keymap_input.h>

#include "qvfb_keymapping.h"

/**
 * Search raw keycode in the mapping table,
 *
 * @param key raw keycode to search mapping for
 * @return MIDP keycode if the mapping is possible, 
 * KEYMAP_KEY_INVALID otherwise
 */
static int search_raw_keycode(unsigned key) {
    KeyMapping *km;
    for (km = mapping; km->midp_keycode != KEYMAP_KEY_INVALID; km++) {
        if (km->raw_keycode == key) {
            return km->midp_keycode;
        }
    }
    return KEYMAP_KEY_INVALID;
}

/** Map raw QVFb keycode to a proper MIDP key constant */
static int map_raw_keycode(unsigned int unicode) {
    int code = (int)(unicode & 0xffff);
    int key = KEYMAP_KEY_INVALID;

    if (code == 0) {
        /* This is function or arrow keys */
        code = (int)((unicode >> 16) & 0xffff);
        key = search_raw_keycode(code);
    } else {
        key = search_raw_keycode(code);
        /* letter keys have no mapping, the code is returned instead */
        if (key == KEYMAP_KEY_INVALID && code >= ' ' && code < 127) {
            key = code;
        }
    }
    return key;
}

/**
 * On i386 platforms read data from the QVFB keyboard pipe.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_key_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    int midpKeyCode;
    jboolean isPressed;
    jboolean repeatSupport;
    struct QVFbKeyEvent {
        unsigned int unicode;
        unsigned int modifiers;
        int press;
        int repeat;
    } qvfbKeyEvent;
    size_t nread;

    /* IMPL_NOTE: We don't handle repeats, but this seems OK. When you hold */
    /* down a key, QVFB passes a stream of simulated keyups an keydowns */
    /* IMPL_NOTE: this function is called only when we are sure that there is 
     * data to read, so we don't have to check the return value.
     * In fact, if there is an error in the stream (the number of bytes
     * available is different from sizeof(qvfbKeyEvent)),
     * there is no way to recover from that error because we will not know where
     * the next, valid qvfbKeyEvent would begin. */
    nread = read(fbapp_get_keyboard_fd(), &qvfbKeyEvent, sizeof(qvfbKeyEvent));
    (void)nread;
    midpKeyCode = map_raw_keycode(qvfbKeyEvent.unicode);
    /* There is a patch. When a button was released qvfbKeyEvent.press equales*/
    /* a big number instead of 0. But when a button was pressed this value equals 1.*/
    isPressed = (qvfbKeyEvent.press > 0) ? KNI_TRUE : KNI_FALSE;

    repeatSupport = KNI_FALSE;
   
    fbapp_map_keycode_to_event(
        pNewSignal, pNewMidpEvent,
        midpKeyCode, isPressed, repeatSupport);
}

#ifndef max
#define max(x,y)        x > y ? x :y
#endif

#ifndef min
#define min(x,y)        x > y ? y: x
#endif

/**
 * On i386 platforms read data from the QVFB mouse pipe.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handle_pointer_port(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    int maxX, maxY, screenX, screenY, d1, d2;
    int n;
    int id;
    static const int mouseBufSize = 12;
    unsigned char mouseBuf[mouseBufSize];
    int mouseIdx = 0;
    jboolean pressed = KNI_FALSE;
    
    static struct {
        int x;
        int y;
    } pointer;


    do {    
        n = read(fbapp_get_mouse_fd(), mouseBuf + mouseIdx, 
                mouseBufSize - mouseIdx);
        if ( n > 0 )
	    mouseIdx += n;
    } while ( n > 0 );

    /* mouse package dump */
    /*    for (n = 0; n < mouseIdx; n++) { */
    /*        printf("%02x ", mouseBuf[n]); */
    /*        fflush(stdout); */
    /*    } */
    /*    printf("\n"); */

    /* unexpected data size.  Broken package, no handling - just return */
    if (mouseIdx < mouseBufSize)
        return;

    pNewMidpEvent->type = MIDP_PEN_EVENT;
    id = fbapp_get_current_hardwareId();
    screenX = fbapp_get_screen_x(id);
    screenY = fbapp_get_screen_y(id);
    maxX = fbapp_get_screen_width(id);
    maxY = fbapp_get_screen_height(id);

    d1 = (((int)mouseBuf[3]) << 24) +
        (((int)mouseBuf[2]) << 16) +
        (((int)mouseBuf[1]) << 8) +
        (int)mouseBuf[0];

    d2 = (((int)mouseBuf[7]) << 24) +
        (((int)mouseBuf[6]) << 16) +
        (((int)mouseBuf[5]) << 8) +
        (int)mouseBuf[4];
    
    if (fbapp_get_reverse_orientation(id)) {
        pNewMidpEvent->X_POS = min(maxX - d2, maxX) + screenX;
        pNewMidpEvent->Y_POS = min(d1 - screenY, maxY);
    } else {
        pNewMidpEvent->X_POS = min(d1 - screenX, maxX);
        pNewMidpEvent->Y_POS = min(d2 - screenY, maxY);
    }

    if (pNewMidpEvent->X_POS < 0) {
        pNewMidpEvent->X_POS = 0;
    }


    if (pNewMidpEvent->Y_POS < 0) {
        pNewMidpEvent->Y_POS = 0;
    }

        
    pressed = mouseBuf[8]  ||
        mouseBuf[9]  ||
        mouseBuf[10] ||
        mouseBuf[11];
    
    pNewMidpEvent->ACTION = 
        ( pointer.x != pNewMidpEvent->X_POS ||
          pointer.y != pNewMidpEvent->Y_POS ) ?
        ( pressed ? KEYMAP_STATE_DRAGGED : -1 ) :
        ( pressed ? KEYMAP_STATE_PRESSED : KEYMAP_STATE_RELEASED );

    if ( pNewMidpEvent->ACTION != -1 ) {
        pNewSignal->waitingFor = UI_SIGNAL;
    }
        
    /*    printf("mouse event: pNewMidpEvent->X_POS =%d  pNewMidpEvent->Y_POS =%d pNewMidpEvent->ACTION = %d\n", */
    /*           pNewMidpEvent->X_POS, pNewMidpEvent->Y_POS, pNewMidpEvent->ACTION); */
    
    /* keep the previous coordinates to detect dragged event */
    pointer.x = pNewMidpEvent->X_POS;
    pointer.y = pNewMidpEvent->Y_POS;
}

/**
 * Each keyboard events from QVFb presets a single key press or release,
 * so the implementation has no pending keys.
 *
 * @return false
 */
jboolean has_pending_key_port() {
    return KNI_FALSE;
}

/**
 * Stubbed implementation for repeated keys handling,
 * QVFb has own support for repeated key presses 
 */
void handle_repeated_key_port(int midpKeyCode, jboolean isPressed) {
    (void)midpKeyCode;
    (void)isPressed;
}
