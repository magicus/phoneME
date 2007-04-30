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

#include <midp_logging.h>
#include <midp_mastermode_port.h>
#include <keymap_input.h>

#include <armsdapp_ui.h>

/*
 * This function is called by the VM periodically. It has to check if
 * system has sent a signal to MIDP and return the result in the
 * structs given.
 *
 * Values for the <timeout> paramater:
 *  >0 = Block until a signal sent to MIDP, or until <timeout> milliseconds
 *       has elapsed.
 *   0 = Check the system for a signal but do not block. Return to the
 *       caller immediately regardless of the if a signal was sent.
 *  -1 = Do not timeout. Block until a signal is sent to MIDP.
 */
void checkForSystemSignal(MidpReentryData* pNewSignal,
                          MidpEvent* pNewMidpEvent,
                          jlong timeout) {

    jboolean forever = 0;
    jlong end = midp_getCurrentTime() + timeout;

    if (timeout < 0) {
        forever == 1;
    }

    do {
        /* 
         * There's no easy way in ARMulator to suspend the CPU until
         * an event is fired from the keypad. However, the profiler
         * knows about this and does not count the time spent inside
         * this function. 
         */
        int key;
        int eventCHR = *EVENT_CHAR_REG;   /* This must be read first */
        int eventType = *EVENT_TYPE_REG;  /* This must be read last */

        switch (eventType) {
        case ADS_keyDownKVMEvent:
        case ADS_keyUpKVMEvent:
            switch (eventCHR) {
            case ADS_KEY_CLEAR:
                pNewMidpEvent->type = SELECT_FOREGROUND_EVENT;
                pNewSignal->waitingFor = AMS_SIGNAL;                
                break;
            case ADS_KEY_POWER:
                pNewMidpEvent->type = SHUTDOWN_EVENT;
                pNewSignal->waitingFor = AMS_SIGNAL;
                break;              
            default:
                pNewMidpEvent->type = MIDP_KEY_EVENT;
                pNewMidpEvent->ACTION = (eventType == ADS_keyDownKVMEvent) ?
                    KEYMAP_STATE_ PRESSED : KEYMAP_STATE_RELEASED;

                switch (eventCHR) {
                case ADS_KEY_0:        key = KEYMAP_KEY_0;         break;
                case ADS_KEY_1:        key = KEYMAP_KEY_1;         break;
                case ADS_KEY_2:        key = KEYMAP_KEY_2;         break;
                case ADS_KEY_3:        key = KEYMAP_KEY_3;         break;
                case ADS_KEY_4:        key = KEYMAP_KEY_4;         break;
                case ADS_KEY_5:        key = KEYMAP_KEY_5;         break;
                case ADS_KEY_6:        key = KEYMAP_KEY_6;         break;
                case ADS_KEY_7:        key = KEYMAP_KEY_7;         break;
                case ADS_KEY_8:        key = KEYMAP_KEY_8;         break;
                case ADS_KEY_9:        key = KEYMAP_KEY_9;         break;

                case ADS_KEY_ASTERISK: key = KEYMAP_KEY_ASTERISK;  break;
                case ADS_KEY_POUND:    key = KEYMAP_KEY_POUND;     break;

                case ADS_KEY_UP:       key = KEYMAP_KEY_UP;        break;
                case ADS_KEY_DOWN:     key = KEYMAP_KEY_DOWN;      break;
                case ADS_KEY_LEFT:     key = KEYMAP_KEY_LEFT;      break;
                case ADS_KEY_RIGHT:    key = KEYMAP_KEY_RIGHT;     break;
                case ADS_KEY_SELECT:   key = KEYMAP_KEY_SELECT;    break;

                case ADS_KEY_SOFT1:    key = KEYMAP_KEY_SOFT1;     break;
                case ADS_KEY_SOFT2:    key = KEYMAP_KEY_SOFT2;     break;
                case ADS_KEY_CLEAR:    key = KEYMAP_KEY_BACKSPACE; break;

                case ADS_KEY_SEND:     key = KEYMAP_KEY_SEND;      break;
                case ADS_KEY_END:      key = KEYMAP_KEY_END;       break;
                default:               key = KEYMAP_KEY_INVALID;
                }
                pNewMidpEvent->CHR = key;
                pNewSignal->waitingFor = UI_SIGNAL;
            }
            return; /* Received one key. Let's process it. */
        default:
            break;
        }
    } while (forever || midp_getCurrentTime() < end);
}
