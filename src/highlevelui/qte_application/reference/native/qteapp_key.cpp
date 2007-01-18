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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qapplication.h>
#include <qevent.h>
#include <qstring.h>

#include <midpEvents.h>
#include <keymap_input.h>
#include <midp_logging.h>

/**
 * Map the key from native system (Qt) into something useful
 * for MIDP. Key mapping from Sharp available at:
 * http://docs.Zaurus.com/downloads/keycode_qt.pdf 
 * file name: sl5600_keycode_v091.pdf
 *
 * @param key Qt key event
 * @return mapped MIDP key code as defined in keymap_input.h.
 *	 KEY_INVALID if the Qt key should be ignored.
 */
extern "C"
int mapKey(QKeyEvent *key) {
    int raw = key->key();
    int unicode;
    QString qstring;

    switch (raw) {

    case Qt::Key_Up:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEY_GAME_UP;// customItem Game key shift UP
        } else {
            unicode = KEY_UP;
        }
        break;

    case Qt::Key_Down:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEY_GAME_DOWN;// customItem Game key shift DOWN
        } else {
            unicode = KEY_DOWN;
        }
        break;

    case Qt::Key_Left:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEY_GAME_LEFT;// customItem Game key shift LEFT
        } else {
            unicode = KEY_LEFT;
        }
        break;

    case Qt::Key_Right:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEY_GAME_RIGHT;// customItem Game key shift RIGHT
        } else {
            unicode = KEY_RIGHT;
        }
        break;

#ifdef QT_KEYPAD_MODE
    // Keypad buttons
    case Qt::Key_Context1:
        unicode = KEY_SOFT1;
        break;

    case Qt::Key_Context2:
        unicode = KEY_SOFT2;
        break;

    case Qt::Key_Context4:
        unicode = KEY_SCREEN_ROT;
        break;

    case Qt::Key_Back:
        unicode = KEY_BACKSPACE;
        break;

    case Qt::Key_Call:
        unicode = KEY_SEND;
        break;

    case Qt::Key_Hangup:
        unicode = KEY_END;
        break;

    case Qt::Key_Select:
        unicode = KEY_SELECT;
        break;
#endif

    // Select
    // Map Key_Space here, but in the
    // high level Java code, we have to
    // test for special case since "space"
    // should be used for textbox's as space.
    case Qt::Key_Return:
    case Qt::Key_F33:
    case Qt::Key_Enter:
        unicode = KEY_SELECT;
        break;

    case Qt::Key_Space:
#if REPORT_LEVEL < LOG_DISABLED
        if (key->state() == Qt::ShiftButton) {
            unicode = KEY_DEBUG_TRACE1;
        } else {
            unicode = KEY_SELECT;
        }
#else
        unicode = KEY_SELECT;
#endif
        break;

    // The cancel key
    case Qt::Key_Escape:
         unicode = KEY_END;
         break;

    // Soft button 1
    case Qt::Key_F1:
        unicode = KEY_SOFT1;
        break;

    // Soft button 2
    case Qt::Key_F2:
        unicode = KEY_SOFT2;
        break;
    // rotation
    case Qt::Key_F3:
        unicode = KEY_SCREEN_ROT;
        break;
    // Calendar
    case Qt::Key_F9:
        unicode = KEY_GAMEA;
        break;

    // Addressbook
    case Qt::Key_F10:
        unicode = KEY_GAMEB;
        break;

    // The menu key
    case Qt::Key_F11:
        unicode = KEY_GAMEC;
        break;

    // Mail key
    case Qt::Key_F13:
        unicode = KEY_GAMED;
        break;

    case Qt::Key_F22:          // Fn key
    case Qt::Key_Shift:        // Left shift
    case Qt::Key_Control:
    case Qt::Key_Meta:         // Right shift
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_F35:          // (Press and hold)
    case Qt::Key_F14:          // (Press and hold)
        unicode = KEY_INVALID;
	    key->ignore();
        break;

    default:
        qstring = key->text(); // Obtain the UNICODE

        if (qstring == QString::null) {
            unicode = KEY_INVALID;
        } else {
            // Transfer the unicode (from QChar to uchar)
            unicode = qstring[0].unicode();
        }
    }

    return unicode;
}
