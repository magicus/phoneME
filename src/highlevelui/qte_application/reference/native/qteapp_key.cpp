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
 * KEYMAP_KEY_INVALID if the Qt key should be ignored.
 */
extern "C"
int mapKey(QKeyEvent *key) {
    int raw = key->key();
    int unicode;
    QString qstring;

    switch (raw) {

    case Qt::Key_Up:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEYMAP_KEY_GAME_UP;// customItem Game key shift UP
        } else {
            unicode = KEYMAP_KEY_UP;
        }
        break;

    case Qt::Key_Down:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEYMAP_KEY_GAME_DOWN;// customItem Game key shift DOWN
        } else {
            unicode = KEYMAP_KEY_DOWN;
        }
        break;

    case Qt::Key_Left:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEYMAP_KEY_GAME_LEFT;// customItem Game key shift LEFT
        } else {
            unicode = KEYMAP_KEY_LEFT;
        }
        break;

    case Qt::Key_Right:
        if (key->state() == Qt::ShiftButton) {
            unicode = KEYMAP_KEY_GAME_RIGHT;// customItem Game key shift RIGHT
        } else {
            unicode = KEYMAP_KEY_RIGHT;
        }
        break;

#ifdef QT_KEYPAD_MODE
    // Keypad buttons
    case Qt::Key_Context1:
        unicode = KEYMAP_KEY_SOFT1;
        break;

    case Qt::Key_Context2:
        unicode = KEYMAP_KEY_SOFT2;
        break;

    case Qt::Key_Context4:
        unicode = KEYMAP_KEY_SCREEN_ROT;
        break;

    case Qt::Key_Context3:
        unicode = KEYMAP_KEY_VIRT_KEYB;
        break;

    case Qt::Key_Back:
        unicode = KEYMAP_KEY_BACKSPACE;
        break;

    case Qt::Key_Call:
        unicode = KEYMAP_KEY_SEND;
        break;

    case Qt::Key_Hangup:
        unicode = KEYMAP_KEY_END;
        break;

    case Qt::Key_Select:
        unicode = KEYMAP_KEY_SELECT;
        break;
#endif

    // Select
    case Qt::Key_Return:
    case Qt::Key_F33:
    case Qt::Key_Enter:
        unicode = KEYMAP_KEY_SELECT;
        break;

    // The cancel key
    case Qt::Key_Escape:
         unicode = KEYMAP_KEY_END;
         break;

    // Soft button 1
    case Qt::Key_F1:
        unicode = KEYMAP_KEY_SOFT1;
        break;

    // Soft button 2
    case Qt::Key_F2:
        unicode = KEYMAP_KEY_SOFT2;
        break;
    // rotation
    case Qt::Key_F3:
        unicode = KEYMAP_KEY_SCREEN_ROT;
        break;
    // virtual keyboard
    case Qt::Key_F6:
        unicode = KEYMAP_KEY_VIRT_KEYB;
        break;
    // Calendar
    case Qt::Key_F9:
        unicode = KEYMAP_KEY_GAMEA;
        break;

    // Addressbook
    case Qt::Key_F10:
        unicode = KEYMAP_KEY_GAMEB;
        break;

    // The menu key
    case Qt::Key_F11:
        unicode = KEYMAP_KEY_GAMEC;
        break;

    // Mail key
    case Qt::Key_F12:
        unicode = KEYMAP_KEY_GAMED;
        break;

    case Qt::Key_F22:          // Fn key
    case Qt::Key_Shift:        // Left shift
    case Qt::Key_Control:
    case Qt::Key_Meta:         // Right shift
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_F35:          // (Press and hold)
    case Qt::Key_F14:          // (Press and hold)
    case Qt::Key_Delete:
        unicode = KEYMAP_KEY_INVALID;
        key->ignore();
        break;

    default:
        qstring = key->text(); // Obtain the UNICODE

        if (qstring == QString::null) {
            unicode = KEYMAP_KEY_INVALID;
        } else {
            // Transfer the unicode (from QChar to uchar)
            unicode = qstring[0].unicode();
        }
    }

    return unicode;
}
