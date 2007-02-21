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

/**
 * @file
 *
 * Utility functions to handle received system signals.
 */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <midpServices.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <fbapp_export.h>
#include <keymap_input.h>
#include <midp_logging.h>
#include <jvm.h>
#ifdef DIRECTFB
#include <directfb.h>
#endif

#include "mastermode_handle_signal.h"
#include "mastermode_keymapping.h"
#include "timer_queue.h"

/* OMAP730 keyboard events are bit-based. Thus single native event can produce
   several MIDP ones. This flag indicates there are one or more key bits still
   not converted into MIDP events */
jboolean hasPendingKeySignal = KNI_FALSE;

/**
 * Prepare read/write/exception descriptor sets with data from
 * socket list for suceeded select() query
 *
 * @param socketsList list of sockets registered for read/write notifications
 * @param pRead_fds set of descriptors to check for read signals
 * @param pWrite_fds set of descriptors to check for write signals
 * @param pExcept_fds set of descriptors to check for exception signals
 * @param pNum_fds upper bound of checked descriptor values
 */
void setSockets(const SocketHandle* socketsList,
        /*OUT*/ fd_set* pRead_fds, /*OUT*/ fd_set* pWrite_fds,
        /*OUT*/ fd_set* pExcept_fds, /*OUT*/ int* pNum_fds) {

    if (socketsList != NULL) {
        const SocketHandle *socket = (const SocketHandle *)socketsList;
        for(; socket != NULL; socket = socket->next) {
            if (socket->check_flags & CHECK_READ) {
                FD_SET(socket->fd, pRead_fds);
            }
            if (socket->check_flags & CHECK_WRITE) {
                FD_SET(socket->fd, pWrite_fds);
            }
            FD_SET(socket->fd, pExcept_fds);
            if (*pNum_fds <= socket->fd) {
               *pNum_fds = socket->fd + 1;
            }
        }
    }
}

/**
 * Handle received socket signal and prepare reentry data
 * to unblock a thread waiting for the signal.
 *
 * @param socketsList list of sockets registered for read/write notifications
 * @param pRead_fds set of descriptors to check for read signals
 * @param pWrite_fds set of descriptors to check for write signals
 * @param pExcept_fds set of descriptors to check for exception signals
 * @param pNewSignal reentry data to unblock a threads waiting for a socket signal
 */
void handleSockets(const SocketHandle* socketsList,
        fd_set* pRead_fds, fd_set* pWrite_fds, fd_set* pExcept_fds,
        /*OUT*/ MidpReentryData* pNewSignal) {

    if (socketsList != NULL) {
        /* Handle socket events */
        const SocketHandle *socket = (const SocketHandle *)socketsList;
        for(; socket != NULL; socket = socket->next) {
            if (FD_ISSET(socket->fd, pExcept_fds)) {
                pNewSignal->descriptor = (int)socket;
                pNewSignal->waitingFor = NETWORK_EXCEPTION_SIGNAL;
                break;
            }
            if ((socket->check_flags & CHECK_READ) &&
                    FD_ISSET(socket->fd, pRead_fds)) {
                pNewSignal->descriptor = (int)socket;
                pNewSignal->waitingFor = NETWORK_READ_SIGNAL;
                break;
            }
            if ((socket->check_flags & CHECK_WRITE) &&
                    FD_ISSET(socket->fd, pWrite_fds)) {
                pNewSignal->descriptor = (int)socket;
                pNewSignal->waitingFor = NETWORK_WRITE_SIGNAL;
                break;
            }
        } /* for */
    } /* socketList != NULL */
}

#if !defined(ARM) && !defined(DIRECTFB)
/**
 * On i386 platforms read data from the QVFB keyboard pipe.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handleKey(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    int key;
    struct {
        unsigned int unicode;
        unsigned int modifiers;
        int press;
        int repeat;
    } input;

    read(fbapp_get_keyboard_fd(), &input, sizeof(input));

    if (input.modifiers != 0) {
        return;
    }

    key = (int)(input.unicode & 0xffff);
    if (key == 0) {
        /* This is function or arrow keys */
        key = (int)((input.unicode >> 16) & 0xffff);
        switch (key) {
        case 0x1030: /* F1    */   key = KEY_SOFT1;   break;
        case 0x1031: /* F2    */   key = KEY_SOFT2;   break;
        case 0x1032: /* F3    */   key = KEY_SCREEN_ROT; break;
        case 0x1033: /* F4    */   key = MD_KEY_SWITCH_APP; break;
        case 0x1038: /* F9    */   key = KEY_GAMEA;   break;
        case 0x1039: /* F10   */   key = KEY_GAMEB;   break;
        case 0x103a: /* F11   */   key = KEY_GAMEC;   break;
        case 0x103b: /* F12   */   key = KEY_GAMED;   break;
        case 0x1012: /* LEFT  */   key = KEY_LEFT;    break;
        case 0x1013: /* UP    */   key = KEY_UP;      break;
        case 0x1014: /* RIGHT */   key = KEY_RIGHT;   break;
        case 0x1015: /* DOWN  */   key = KEY_DOWN;    break;
        case 0x1010: /* HOME  */   key = MD_KEY_HOME; break;
        case 0x1011: /* END   */   key = KEY_END;     break;
        default:
            return;
        }
    } else {
        switch (key) {
        case 0x000d: /* enter */   key = KEY_SELECT;     break;
        case 0x0008: /* backsp*/   key = KEY_BACKSPACE;  break;
        default:
            if (key < ' ' && key > 127) {
                return;
            }
        }
    }

    switch (key) {
    case MD_KEY_HOME:
        pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
        pNewMidpEvent->intParam1 = 0; 
        pNewSignal->waitingFor = AMS_SIGNAL;
        break;
    case MD_KEY_SWITCH_APP:
        pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
        pNewMidpEvent->intParam1 = 1; 
        pNewSignal->waitingFor = AMS_SIGNAL;
        break;
    case KEY_SCREEN_ROT:
        if (input.press) {
            pNewMidpEvent->type = ROTATION_EVENT;
            pNewSignal->waitingFor = UI_SIGNAL;
        }
        break;
    case KEY_END:
        if (input.press) {
#if ENABLE_MULTIPLE_ISOLATES
            pNewSignal->waitingFor = AMS_SIGNAL;
            pNewMidpEvent->type =
                MIDLET_DESTROY_REQUEST_EVENT;
            pNewMidpEvent->DISPLAY = gForegroundDisplayId;
            pNewMidpEvent->intParam1 = gForegroundIsolateId;
#else
            pNewSignal->waitingFor = UI_SIGNAL;
            pNewMidpEvent->type = DESTROY_MIDLET_EVENT;
#endif
        }
        break;

    default:
        pNewMidpEvent->type = MIDP_KEY_EVENT;
        pNewMidpEvent->CHR = key;

        /* Note: we don't handle repeats, but this seems OK. When you hold
         * down a key, QVFB passes a stream of simulated keyups an keydowns */
        if (input.press) {
            pNewMidpEvent->ACTION = PRESSED;
        } else {
            pNewMidpEvent->ACTION = RELEASED;
        }

        pNewSignal->waitingFor = UI_SIGNAL;
    }
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
void handlePointer(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    int maxX, maxY;
    int n;
    static const int mouseBufSize = 12;
    unsigned char mouseBuf[mouseBufSize];
    int mouseIdx = 0;
    jboolean pressed = KNI_FALSE;
    
    static struct {
        int x;
        int y;
    } pointer;

    do {
        n = read(fbapp_get_mouse_fd(), mouseBuf + mouseIdx, mouseBufSize - mouseIdx);
	if ( n > 0 )
	    mouseIdx += n;
    } while ( n > 0 );

    // mouse package dump
    //    for (n = 0; n < mouseIdx; n++) {
    //        printf("%02x ", mouseBuf[n]);
    //        fflush(stdout);
    //    }
    //    printf("\n");

    // unexpected data size.  Broken package, no handling - just return
    if (mouseIdx < mouseBufSize)
        return;

    pNewMidpEvent->type = MIDP_PEN_EVENT;

    maxX = get_screen_width();
    maxY = get_screen_height();
    if (fbapp_get_reverse_orientation()) {
        pNewMidpEvent->X_POS = min(maxX - (int)mouseBuf[4], maxX);
        pNewMidpEvent->Y_POS = min((int)mouseBuf[0], maxY);
    } else {
        pNewMidpEvent->X_POS = min((int)mouseBuf[0], maxX);
        pNewMidpEvent->Y_POS = min((int)mouseBuf[4], maxY);
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
        ( pressed ? DRAGGED : -1 ) :
        ( pressed ? PRESSED : RELEASED );

    if ( pNewMidpEvent->ACTION != -1 ) {
        pNewSignal->waitingFor = UI_SIGNAL;
    }
        
    //    printf("mouse event: pNewMidpEvent->X_POS =%d  pNewMidpEvent->Y_POS =%d pNewMidpEvent->ACTION = %d\n",
    //           pNewMidpEvent->X_POS, pNewMidpEvent->Y_POS, pNewMidpEvent->ACTION);
    
    // keep the previous coordinates to detect dragged event
    pointer.x = pNewMidpEvent->X_POS;
    pointer.y = pNewMidpEvent->Y_POS;
}
#endif /* !defined(ARM) && !defined(DIRECTFB) */

#if defined(ARM) || defined(DIRECTFB)

/** Handle key repeat timer alarm on timer wakeup */
static void handleRepeatKeyTimerAlarm(TimerHandle *timer) {
    static MidpEvent newMidpEvent;
    static int midp_keycode;
    if (timer != NULL) {
        midp_keycode = (int)(get_timer_data(timer));

        newMidpEvent.type = MIDP_KEY_EVENT;
        newMidpEvent.CHR = midp_keycode;
        newMidpEvent.ACTION = REPEATED;

        midpStoreEventAndSignalForeground(newMidpEvent);

        timer = remove_timer(timer);

        set_timer_wakeup(timer, get_timer_wakeup(timer) + REPEAT_PERIOD);
        add_timer(timer);
    }
}

/**
 * ARM/DIRECTFB version to read keyboard events from /dev/tty0.
 *
 * @param pNewSignal        reentry data to unblock threads waiting for a signal
 * @param pNewMidpEvent     a native MIDP event to be stored to Java event queue
 */
void handleKey(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    /* OMAP730 keyboard: This variable contains key mask from last native event */
    static unsigned changedBits = 0;
    static KeyMapping *km;
    static unsigned key = 0;
    int n;

#ifdef DIRECTFB
    int event_keyup = 0;
    DFBWindowEvent keyboard_event;
#endif /* defined(DIRECTFB) */

    jlong current_time = JVM_JavaMilliSeconds();

#ifdef DIRECTFB
    fbapp_get_event(&keyboard_event);
#endif /* defined(DIRECTFB) */

    switch (fbapp_get_fb_device_type()) {
    case LINUX_FB_OMAP730: {
        if (!hasPendingKeySignal) {
#ifndef DIRECTFB
            InputEvent event;
#endif /* !defined(DIRECTFB) */

            km = mapping = omap_730_keys;
#ifndef DIRECTFB
            n = read(fbapp_get_keyboard_fd(), &event, sizeof(InputEvent));

            if ( n < (int)sizeof(InputEvent)) {
                REPORT_ERROR2(LC_CORE,
                    "Invalid key input event, received %d bytes instead of %d",
                    n, (int)sizeof(InputEvent));
                return;
            }
            changedBits = event.value ^ key;
            key = event.value;
#else
            event_keyup = keyboard_event.type == DWET_KEYUP;
            changedBits = keyboard_event.key_code ^ key;
            key = keyboard_event.key_code;
            n = 1;
#endif /* !defined(DIRECTFB) */
        } else {
            n = 1;
        }
        break;
    }
    case LINUX_FB_ZAURUS:
        {
#ifndef DIRECTFB
          unsigned char c;
#endif /* !defined(DIRECTFB) */
          mapping = zaurus_sl5500_keys;
#ifndef DIRECTFB
          n = read(fbapp_get_keyboard_fd(), &c, sizeof(c));
          key = c;
#else
            event_keyup = keyboard_event.type == DWET_KEYUP;
            key = keyboard_event.key_code;
            n = 1;
#endif /* !defined(DIRECTFB) */
        }
        break;
    case LINUX_FB_VERSATILE_INTEGRATOR:
    default:
        {
#ifndef DIRECTFB
          unsigned char c;
#endif /* !defined(DIRECTFB) */
          mapping = versatile_integrator_keys;
#ifndef DIRECTFB
          n = read(fbapp_get_keyboard_fd(), &c, sizeof(c));
          key = c;
#else
            event_keyup = keyboard_event.type == DWET_KEYUP;
            key = keyboard_event.key_code;
            n = 1;
#endif /* !defined(DIRECTFB) */
        }
    }

    if (n > 0) {
        int down = 0;

        /* find MIDP key code corresponding to native key */
        switch (fbapp_get_fb_device_type()) {
        case LINUX_FB_OMAP730: {
            /* key is bit-mask. seach for mapping entry */
            for (; km->midp_keycode != KEY_INVALID &&
                     (changedBits & km->raw_keydown) == 0; km++)
                ;

            /* check if found the corresponding entry */
            if (km->midp_keycode != KEY_INVALID) {
                down = key & km->raw_keydown;
                changedBits &= ~km->raw_keydown;
            } else {
                /* no entry found. reset search */
                changedBits = 0;
            }
            /* check if there are outstanding bits */
            hasPendingKeySignal = changedBits != 0;
        }
            break;
        case LINUX_FB_ZAURUS:
        case LINUX_FB_VERSATILE_INTEGRATOR:
        default:
            /* key is key code */
            for (km = mapping; km->midp_keycode != KEY_INVALID; km++) {
                if (km->raw_keydown == key) {
                    down = 1;
                    break;
                } else if (km->raw_keyup == key) {
                    down = 0;
                    break;
                }
            }
        }
        
#ifdef DIRECTFB
        /* DirectFB sends key-up codes via DWET_KEYUP event 
           with ordinary (like key-down) key code */
        if (event_keyup) {
            down = 0;
        }
#endif
        if (km->midp_keycode == MD_KEY_HOME) {
            if (down) {
                pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
                pNewMidpEvent->intParam1 = 0;
                pNewSignal->waitingFor = AMS_SIGNAL;
            } else {
                // ignore it
            }

        } else if (km->midp_keycode == MD_KEY_SWITCH_APP) {
            if (down) {
                pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
                pNewMidpEvent->intParam1 = 1;
                pNewSignal->waitingFor = AMS_SIGNAL;
            } else {
                // ignore it
            }
        } else if (km->midp_keycode == KEY_SCREEN_ROT) {
            if (down) {
                pNewMidpEvent->type = ROTATION_EVENT;
                pNewSignal->waitingFor = UI_SIGNAL;
            } else {
                // ignore it
            }
        }
        else if (km->midp_keycode == KEY_END) {
            if (down) {
#if ENABLE_MULTIPLE_ISOLATES
                pNewSignal->waitingFor = AMS_SIGNAL;
                pNewMidpEvent->type =
                    MIDLET_DESTROY_REQUEST_EVENT;
                pNewMidpEvent->DISPLAY = gForegroundDisplayId;
                pNewMidpEvent->intParam1 = gForegroundIsolateId;
#else
                pNewSignal->waitingFor = UI_SIGNAL;
                pNewMidpEvent->type = DESTROY_MIDLET_EVENT;
#endif
            } else {
                // ignore it
            }
        } else if (km->midp_keycode != KEY_INVALID) {
            pNewMidpEvent->type = MIDP_KEY_EVENT;
            pNewMidpEvent->CHR = km->midp_keycode;
            if (down) {
                new_timer(current_time + REPEAT_TIMEOUT,
                          (void*)km->midp_keycode, handleRepeatKeyTimerAlarm);
                pNewMidpEvent->ACTION = PRESSED;
            } else {
                delete_timer_by_userdata((void*)km->midp_keycode);
                pNewMidpEvent->ACTION = RELEASED;
            }

            pNewSignal->waitingFor = UI_SIGNAL;
        }
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
void handlePointer(MidpReentryData* pNewSignal, MidpEvent* pNewMidpEvent) {
    (void)pNewSignal;
    (void)pNewMidpEvent;
}

#endif /* defined(ARM) || defined(DIRECTFB) */
