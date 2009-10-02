/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.lcdui;

import java.util.Hashtable;

import javax.microedition.lcdui.Display;

import com.sun.midp.lcdui.VirtualKeyboard;


/**
 * Virtual keyboard controller interface
 * 
 */
public interface VirtualKeyboardManager {

    /*Keyboard types*/
    public static final String LOWER_ALPHABETIC_KEYBOARD = VirtualKeyboard.LOWER_ALPHABETIC_KEYBOARD;
    public static final String UPPER_ALPHABETIC_KEYBOARD = VirtualKeyboard.UPPER_ALPHABETIC_KEYBOARD;
    public static final String NUMERIC_KEYBOARD = VirtualKeyboard.NUMERIC_KEYBOARD;
    public static final String SYBOLIC_KEYBOARD = VirtualKeyboard.SYBOLIC_KEYBOARD;
    public static final String GAME_KEYBOARD = VirtualKeyboard.GAME_KEYBOARD;

    /**
     * 
     * Changes keyboard type
     * 
     * @param type new keyboard type
     */
    public void setKeyboardType(String type);

    /**
     * Hides virtual keyboard
     * 
     */
    public void hideVirtualKeyboard();

    /**
     * Shows virtual keyboard
     * 
     */
    public void showVirtualKeyboard();

    /**
     * Checks if virtual keyboard is visible
     * 
     * 
     * @return true if keyboard is visible or false otherwise 
     */
    public boolean isVirtualKeyboardVisible();
}
