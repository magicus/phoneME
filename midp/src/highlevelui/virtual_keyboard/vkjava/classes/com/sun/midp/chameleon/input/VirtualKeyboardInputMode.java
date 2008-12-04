/*
 *
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
package com.sun.midp.chameleon.input;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

/**
 * An InputMode instance which allows to use java virtual keyboard.
 */
public class VirtualKeyboardInputMode extends KeyboardInputMode {

    private static String modeName;
    static {
        modeName = Resource.getString(ResourceConstants.LCDUI_TF_NATIVE_VKBD);
    }


    /**
     * Returns the display name which will represent this InputMode to
     * the user, such as in a selection list or the softbutton bar.
     *
     * @return the locale-appropriate name to represent this InputMode
     *         to the user
     */
    public String getName() {
        return modeName;
    }

    /**
     * Process the given key code as input.
     *
     * This method will return true if the key was processed successfully,
     * false otherwise.
     *
     * @param keyCode the keycode of the key which was input
     * @param longPress return true if it's long key press otherwise false
     * @return true if the key was processed by this InputMode, false
     *         otherwise.
     */
    public int processKey(int keyCode, boolean longPress) {
        int ret = super.processKey(keyCode, longPress);
        if (ret == KEYCODE_NONE) {
            if (mediator != null && mediator.isClearKey(keyCode)) {
                mediator.clear(1);
            }
        }
        return ret;
    }
}
