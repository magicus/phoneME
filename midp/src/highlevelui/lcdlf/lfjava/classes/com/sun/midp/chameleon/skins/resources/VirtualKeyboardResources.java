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

package com.sun.midp.chameleon.skins.resources;

import com.sun.midp.chameleon.skins.SkinPropertiesIDs;
import com.sun.midp.chameleon.skins.VirtualKeyboardSkin;

/** Resources for virtual keyboard layer */
public class VirtualKeyboardResources {
    /** Flag indicated if resources have been already loaded */
    private static boolean init;

    /** Private constructor */
    private VirtualKeyboardResources() {
    }

    /** Load pti resources. Do nothing if they have been already loaded */
    public static void load() {
        load(false);
    }

    /**
     *  Load keyboard resources.
     * @param reload if true resources are being loaded even if the
     * initialization has been already done. In case of false don't
     * reload the resources
     */
    public static void load(boolean reload) {
        if (init && !reload) {
            return;
        }

        VirtualKeyboardSkin.KEY = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_KEY);

        VirtualKeyboardSkin.BG = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BG);

        VirtualKeyboardSkin.BTN_UP_SEL = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_UP_SEL);

        VirtualKeyboardSkin.BTN_UP_UN = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_UP_UN);

        VirtualKeyboardSkin.BTN_LEFT_SEL = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_LEFT_SEL);

        VirtualKeyboardSkin.BTN_LEFT_UN = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_LEFT_UN);

        VirtualKeyboardSkin.BTN_MID_SEL = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_MID_SEL);

        VirtualKeyboardSkin.BTN_MID_UN = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_MID_UN);

        VirtualKeyboardSkin.BTN_RIGHT_SEL = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_RIGHT_SEL);

        VirtualKeyboardSkin.BTN_RIGHT_UN = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_RIGHT_UN);

        VirtualKeyboardSkin.BTN_DOWN_SEL = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_DOWN_SEL);

        VirtualKeyboardSkin.BTN_DOWN_UN = SkinResources.getImage(
                SkinPropertiesIDs.KEYBOARD_BTN_DOWN_UN);
        
        init = true;
    }
}

