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

#ifndef _QTEAPP_KEY_H_
#define _QTEAPP_KEY_H_

/**
 * @file
 * @ingroup highui_qteapp
 *
 * @brief Functions that handle Linux/Qte application native input.
 */

#include <qapplication.h>
#include <midpEvents.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maps the key event from the native system (Qt) into a MIDP event.
 * Key mappings for Sharp are available at: <a
 * href="http://docs.Zaurus.com/downloads/keycode_qt.pdf">
 * http://docs.Zaurus.com/downloads/keycode_qt.pdf</a>. The
 * file name is <tt>sl5600_keycode_v091.pdf</tt>.
 *
 * @param key native key event
 * @return mapped MIDP key code as defined in keymap_input.h.
 * KEYMAP_KEY_INVALID if the Qt key event should be ignored.
 */
int mapKey(QKeyEvent *key);

#ifdef __cplusplus
}
#endif

#endif /* _QTEAPP_KEY_H_ */
