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

#ifndef QT_NO_QWS_KEYBOARD

#include "lfpport_qte_inputmode.h"
#include <moc_lfpport_qte_inputmode.cpp>
#include <qteapp_export.h>
#include <stdio.h>

static const int keyMapSize[] = {1, 11, 4, 4, 4, 4, 4, 5, 4, 5};
static const int key0[] = {Qt::Key_0}; // 0
static const int key1[] = {Qt::Key_1, Qt::Key_Period, Qt::Key_Colon,
                           Qt::Key_Slash, Qt::Key_Minus, Qt::Key_Comma,
                           Qt::Key_At, Qt::Key_Question, Qt::Key_Exclam,
                           Qt::Key_Asterisk, Qt::Key_NumberSign}; // 1 .:/-,@?!*#
static const int key2[] = {Qt::Key_A, Qt::Key_B, Qt::Key_C, Qt::Key_2}; // abc2
static const int key3[] = {Qt::Key_D, Qt::Key_E, Qt::Key_F, Qt::Key_3}; //def3
static const int key4[] = {Qt::Key_G, Qt::Key_H, Qt::Key_I, Qt::Key_4}; // ghi4
static const int key5[] = {Qt::Key_J, Qt::Key_K, Qt::Key_L, Qt::Key_5}; // jkl5
static const int key6[] = {Qt::Key_M, Qt::Key_N, Qt::Key_O, Qt::Key_6}; // mno6
static const int key7[] = {Qt::Key_P, Qt::Key_Q, Qt::Key_R, Qt::Key_S, Qt::Key_7}; // pqrs7
static const int key8[] = {Qt::Key_T, Qt::Key_U, Qt::Key_V, Qt::Key_8}; // tuv8
static const int key9[] = {Qt::Key_W, Qt::Key_X, Qt::Key_Y, Qt::Key_Z, Qt::Key_9}; //wxyz9

static const int* keyMap[] = { key0, key1, key2, key3, key4, key5, key6, key7, key8, key9 };

void KeyTimerHandler::processKey() {
    parent->accept();
};


AbcKeyboardFilter::AbcKeyboardFilter()
    : QWSServer::KeyboardFilter() {
    server = qteapp_get_server();
    tHandler = new KeyTimerHandler(this);
    timer = new QTimer(tHandler, "KeyTimer");
    tHandler->connect(timer, SIGNAL(timeout()), SLOT(processKey()));
    caps = false;
    clear();
}

bool AbcKeyboardFilter::filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat) {
    bool ret = false;
    (void)modifiers;
    if (keycode == Qt::Key_Asterisk) {
        // handle caps lock '*'
        if (isPress) {
            accept();
            caps = !caps;
        }
        ret = true;
    } else if (keycode == Qt::Key_NumberSign) {
        // handle  '#'. Should send space
        if (isPress) {
            accept();
            // send key press
            server->sendKeyEvent(Qt::Key_Space, Qt::Key_Space, 0, 1, autoRepeat);
            // send key release
            server->sendKeyEvent(Qt::Key_Space, Qt::Key_Space, 0, 0, autoRepeat);
        }
        ret = true;
    } else if (keycode >= Qt::Key_0 &&
               keycode <= Qt::Key_9) {
        if (autoRepeat) {
            if (code == unicode) {
                clear();
                server->sendKeyEvent(unicode, unicode, 0, 1, 0);
            }
        } else {
            if (isPress) {
                if (code != unicode) {
                    accept();
                }
                counter++;
                code = unicode;
                if (timer->isActive()) timer->stop();
                timer->start(CLICK_DELAY, false);
                ret = true;
            } else if (code == unicode) {
                ret = true;
            }
        } 
    } else {
        accept();
    }
    return ret;
}

void AbcKeyboardFilter::accept() {
    if (code != -1) {
        unsigned id = code - Qt::Key_0;
        if (id < sizeof(keyMap)) {
            const int* line = keyMap[id];
            code = line[counter % keyMapSize[id]];
            int unicode = code;
            // if symbol is letter set proper caps lock 
            if (!caps && code >= Qt::Key_A && code <= Qt::Key_Z) {
                unicode += 32;
            }
            // send key press
            server->sendKeyEvent(unicode, code, 0, 1, 0);
            // send key release
            server->sendKeyEvent(unicode, code, 0, 0, 0);
        }
    }
    clear();
}

void AbcKeyboardFilter::clear() {
    code = -1;
    counter = -1;
    if (timer->isActive()) timer->stop();
}

#endif
