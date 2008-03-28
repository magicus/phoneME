/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include "stdio.h"
#include "lime.h"
#include "defaultLCDUI.h"
#include "javacall_keypress.h"
#include "javacall_lifecycle.h"
#if ENABLE_JSR_179
#include "javacall_location.h"
#endif
#include "javacall_logging.h"
#include "javacall_penevent.h"
#if ENABLE_JSR_75
#include "javanotify_fileconnection.h"
#endif

#if ENABLE_ON_DEVICE_DEBUG
#include "javacall_odd.h"
#endif

#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_EVENT_CLASS "EventBridge"

#define EVENT_BUFFER_SIZE 64
#define EVENT_SIZE 4

/* Can't use javacall_debug, as it is not thread safe,
 * Use this flag if needed
#define DEBUG_LIME_EVENTS */

#ifdef DEBUG_LIME_EVENTS
#define LIME_DEBUG_PRINTF javautil_printf_lime
#else
#define LIME_DEBUG_PRINTF
#endif

/* IMPL_NOTE: CR <6658788>, temporary changes to process wrong
 * repeated key events on Shift+<0..9> passed from emulator.
 * Should be reverted as soon as the emulator is fixed.
 */
#define USE_KEYTYPED_VM_EVENTS
/* IMPL_NOTE: End of temporary changes for CR <6658788>
 */

#if ENABLE_JSR_179
extern char *ExtractEventData(javacall_utf16_string event_name);
extern void ParseOrientationData(char *data);
extern void ParseAccuracyData(char *data);
extern void ParseLocationData(char *data);
extern void HandleLocationProviderStateEvent(int state);
#endif

void SendEvent(KVMEventType *evt);
javacall_bool generateSoftButtonKeys(int x, int y, javacall_penevent_type pentype);

/* global varaiable to determine if the application 
 * is running locally or via OTA */
extern javacall_bool isRunningLocal;

#if ENABLE_ON_DEVICE_DEBUG
static const char pStartOddKeySequence[] = "#1*2";
static int posInSequence = 0;
#endif

#ifdef USE_KEYTYPED_VM_EVENTS
static int latestKeyTyped = 0;
static int latestKeyDown = 0;
#endif

void CheckLimeEvent() {
    static LimeFunction *f = NULL;
    int *returnData;
    int returnLength;
    __int64 waitFor = 0;
    bool_t forever = 1;
    KVMEventType evt;
    int i;

    if (f == NULL) {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_EVENT_CLASS,
                            "getEvents");
        f->trace(f, FALSE);
    }

    LIME_DEBUG_PRINTF("Waiting for event\n");

    /* Ask the LIME server for an event */
    f->call(f, &returnData, &returnLength, EVENT_BUFFER_SIZE, forever, waitFor);

    if (returnData == NULL) {
        returnLength = 0;
    }

    LIME_DEBUG_PRINTF("getEvents return length: %d \n",returnLength);

    if ( returnLength %4 != 0 ) {
        LIME_DEBUG_PRINTF("Bad lime event size, adjusting\n");
        returnLength = (returnLength/4) * 4;
    }

    if (returnLength == 0) {
        LIME_DEBUG_PRINTF("Ignoring lime event\n");
        return;
    } else {
        /* Send events to JVM */


        for (i=0; i<returnLength; i+=EVENT_SIZE) {
            /* Decode the event */
            evt.type = returnData[i + 0];
            evt.chr = returnData[i + 1];
            evt.screenX = returnData[i + 2];
            evt.screenY = returnData[i + 3];

            SendEvent(&evt);
        }
    }
}

void SendEvent (KVMEventType *evt) {
    char *event_data = NULL;
    /* copied from DefaultEventHandler.java */

    static int MIDP_KEY_EVENT     = 1;
    static int MIDP_PEN_EVENT     = 2;
    static int MIDP_COMMAND_EVENT = 3;
    static int MIDP_SYSTEM_EVENT  = 4;

    /* for jsr135's audio building block, -- by hsy */
    static int MM_EOM_EVENT       = 8;

    static int PRESSED            = 1;
    static int RELEASED           = 2;
    static int REPEATED           = 3;
    static int DRAGGED            = 3;
    static int TYPED              = 4;
#ifdef INCLUDE_I18N
    static int IME                = 5;
#endif

    /* translated from LocationEventCodes.java */
#define STATE                           4000
#define STATE_AVAILABLE                 4001
#define STATE_TEMPORARILY_UNAVAILABLE   4002
#define STATE_OUT_OF_SERVICE            4003

#define ORIENTATION                     4100
#define LOCATION                        4200
#define ACCURACY                        4300

    switch (evt->type) {
    case penDownKVMEvent:
        if (!generateSoftButtonKeys(evt->screenX,evt->screenY,JAVACALL_PENPRESSED))
            javanotify_pen_event(evt->screenX,evt->screenY,JAVACALL_PENPRESSED);
        break;

    case penUpKVMEvent:
        if (!generateSoftButtonKeys(evt->screenX,evt->screenY,JAVACALL_PENRELEASED))
            javanotify_pen_event(evt->screenX,evt->screenY,JAVACALL_PENRELEASED);
        break;

    case penMoveKVMEvent:
        javanotify_pen_event(evt->screenX,evt->screenY,JAVACALL_PENDRAGGED);
        break;

    case keyDownKVMEvent:
#ifdef USE_KEYTYPED_VM_EVENTS
        /* Regular key events are sent over keyTypedKVMEvent */
        if ((evt->chr >= ' ') && (evt->chr <= 127)) {
            latestKeyTyped = 0;
            latestKeyDown = evt->chr;
            break;
        }
#endif
        if (evt->chr == KEY_USER2) {
            javanotify_rotation();
        } else if ((evt->chr != KEY_END)) {
            javanotify_key_event(evt->chr, JAVACALL_KEYPRESSED);
        } else if (isRunningLocal == JAVACALL_FALSE) {
            javanotify_switch_to_ams();
        } else {
            javanotify_shutdown();
        }
        break;

    case keyUpKVMEvent:
        if (evt->chr == KEY_USER2) {
            // ignore
        } else if ((evt->chr != KEY_END)) {
#if ENABLE_ON_DEVICE_DEBUG
            /* assert(posInSequence == sizeof(pStartOddKeySequence) - 1) */
            if (pStartOddKeySequence[posInSequence] == evt->chr) {
                posInSequence++;
                if (posInSequence == sizeof(pStartOddKeySequence) - 1) {
                    posInSequence = 0;
                    javanotify_enable_odd();
                    break;
                }
            } else {
                posInSequence = 0;
            }
#endif
#ifdef USE_KEYTYPED_VM_EVENTS
            /* Regular key events are sent over keyTypedKVMEvent */
            if ((evt->chr >= ' ') && (evt->chr <= 127)) {
                if (0 == latestKeyTyped) {
                    if (evt->chr == latestKeyDown) {
                        /* Support skin presses - when there is no keytyped
                         * event between key down/up events */
                        javanotify_key_event(latestKeyDown, JAVACALL_KEYPRESSED);
                        javanotify_key_event(latestKeyDown, JAVACALL_KEYRELEASED);
                    }
                } else if (evt->chr == latestKeyTyped || evt->chr == latestKeyDown) {
                    javanotify_key_event(latestKeyTyped, JAVACALL_KEYRELEASED);
                }
                latestKeyDown = 0;
                latestKeyTyped = 0;
                break;
            }
#endif
            javanotify_key_event(evt->chr, JAVACALL_KEYRELEASED);
        }
        break;

    case keyRepeatKVMEvent:
        if (evt->chr == KEY_USER2) {
            // ignore
        } else if (evt->chr != KEY_END) {
#ifdef USE_KEYTYPED_VM_EVENTS
            /* For compound key presses, like shift + digit, WTK produces repeated
             * key events for base key only, e.g. for the digit. It is not good,
             * since, for example for '!' it will produce '1' repeated key event.
             * It can be fixed in assumption that repeated key event can happen
             * to previously pressed and not yet released key only */
            if ((evt->chr >= ' ') && (evt->chr <= 127)) {
                if (latestKeyTyped != 0) {
                    javanotify_key_event(latestKeyTyped, JAVACALL_KEYREPEATED);
                }
            } else {
                javanotify_key_event(evt->chr, JAVACALL_KEYREPEATED);
            }
#else
            javanotify_key_event(evt->chr, JAVACALL_KEYREPEATED);
#endif
        }
        break;

    case keyTypedKVMEvent:
        if (evt->chr == KEY_USER2) {
            // ignore
        } else if ((evt->chr != KEY_END)) {
#ifdef USE_KEYTYPED_VM_EVENTS
            /* Multiple key presses are not supported,
             * non-regular key press means the previous key is released */
            if (0 != latestKeyTyped && latestKeyTyped != evt->chr) {
                javanotify_key_event(latestKeyTyped, JAVACALL_KEYRELEASED);
                latestKeyTyped = 0;
            }
            /* Send regular key events received from the keyboard
             * using standard MIDP mechanism */
            if ((evt->chr >= ' ') && (evt->chr <= 127)) {
                /* Don't produce multiple press events for held keypad keys,
                 * but do it for other typed chars, since WTK doesn't support
                 * repeated key presses for them */
                if (evt->chr != latestKeyTyped || 0 == latestKeyDown) {

                    javanotify_key_event(evt->chr, JAVACALL_KEYPRESSED);
                    latestKeyTyped = evt->chr;
                }
            }
#else /* Temporary solution, will not work on all cases but provides
       * a solution for text entry */
            if (evt->chr != 8) {
                javanotify_key_event(evt->chr, JAVACALL_KEYPRESSED);
                javanotify_key_event(evt->chr, JAVACALL_KEYRELEASED);
            }
#endif
        } else if (isRunningLocal == JAVACALL_FALSE) {
            javanotify_switch_to_ams();
        } else {
            javanotify_shutdown();
        }
        break;

#ifdef INCLUDE_I18N
    case imeKVMEvent: {
            /*StoreKVMEvent(MIDP_KEY_EVENT, 2, IME,
                          instantiateStringFromUnicode(evt->str, evt->len));
            midpFree(evt->str); */
            break;
        }
#endif

    case commandKVMEvent:
        /* StoreKVMEvent(MIDP_COMMAND_EVENT, 1, evt->chr); */
        break;

    case mmEOMEvent:
        /* StoreKVMEvent(MM_EOM_EVENT, 2, (int)evt->screenX, evt->chr); */
        break;

    case systemKVMEvent:
        switch (evt->chr) {
        case VK_SUSPEND_ALL:
            javanotify_pause();
            break;
        case VK_RESUME_ALL:
            javanotify_resume();
            break;
        case VK_SHUTDOWN:
            javanotify_shutdown();
            break;

#if ENABLE_JSR_179
        case STATE_AVAILABLE:
            HandleLocationProviderStateEvent(JAVACALL_LOCATION_AVAILABLE);
            break;
        case STATE_OUT_OF_SERVICE:
            HandleLocationProviderStateEvent(JAVACALL_LOCATION_OUT_OF_SERVICE);
            break;
        case STATE_TEMPORARILY_UNAVAILABLE:
            HandleLocationProviderStateEvent(JAVACALL_LOCATION_TEMPORARILY_UNAVAILABLE);
            break;
        case LOCATION:
            event_data = ExtractEventData(L"4200");
            if (event_data) {
                ParseLocationData(event_data);
            }
            break;
        case ORIENTATION:
            event_data = ExtractEventData(L"4100");
            if (event_data)
                ParseOrientationData(event_data);
            break;
        case ACCURACY:
            event_data = ExtractEventData(L"4300");
            if (event_data)
                ParseAccuracyData(event_data);
            break;
#endif

#if ENABLE_JSR_75
        case FILE_SYSTEM_MOUNTED:
            javanotify_fileconnection_root_changed();
            break;
        case FILE_SYSTEM_UNMOUNTED:
            javanotify_fileconnection_root_changed();
            break;
#endif

        default:
            break;
        } /* switch(evt->chr) */

    default: /* do nothing, but continue in loop */
        break;
    } /* switch (evt->type) */
}
