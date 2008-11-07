/*
 *
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

package com.sun.midp.lcdui;

import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.keyboards.Keyboards;
import com.sun.midp.chameleon.skins.VirtualKeyboardSkin;

import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import java.util.Hashtable;

/**
 * This is a popup layer that show java virtual keyboard content
 */
public class VirtualKeyboard {

    /*Keyboard types*/
    public static final String LOWER_ALPHABETIC_KEYBOARD = "lower_alpha";
    public static final String UPPER_ALPHABETIC_KEYBOARD = "upper_alpha";
    public static final String NUMERIC_KEYBOARD = "numeric";
    public static final String SYBOLIC_KEYBOARD = "symbol";
    public static final String GAME_KEYBOARD = "game";

    /**
     * Backspace
     */
    public static final int KEYCODE_CLEAR = -8;
    public static final int KEYCODE_ENTER = -5;


    /* Instance of VirtualKeyboard class */
    static VirtualKeyboard virtualKeyboard = null;

    /* Listener for handling keyboard events*/
    VirtualKeyboardListener vkl;

    /* Table of existed keybords */
    Hashtable keyboardsMap = null;

    /* Keyboard line in focus */
    int line;

    /* Keyboard colomn in focus */
    int column;

    /* Current key */
    Key currentKey;

    /* Array of current keyboard keys */
    Key[][] currentKeyboard;

    /* Current type of keyboard */
    String currentKeyboardType;
   
    /* The  coefficients of keyboard's shrink */
    private double shrinkX = 1;
    private double shrinkY = 1;

    /**
     * Virtual Keyboard constructor.
     * @param listener for handling virtual keyboard events
     */
    VirtualKeyboard(VirtualKeyboardListener listener) {
        keyboardsMap = loadKeyboardMaps();

        vkl = listener;

        currentKeyboardType = LOWER_ALPHABETIC_KEYBOARD;
        currentKeyboard = (Key[][]) keyboardsMap.get(currentKeyboardType);
        line = 0;
        column = 0;

    }

    /**
     * Return instance of virtual keyboard
     * @param listener - listener for handling virtual keyboard events
     * @return instance of VirtualKeyboard class
     */
    public static VirtualKeyboard getVirtualKeyboard(VirtualKeyboardListener listener) {
        if (virtualKeyboard == null) {
            virtualKeyboard = new VirtualKeyboard(listener);
        }
        return virtualKeyboard;
    }

    /**
     * Handle input from a keyboard
     *
     * @param type    type of keypress
     * @param keyCode key code of key pressed
     */
    public boolean keyInput(int type, int keyCode) {

        boolean ret = false;

        if (type == EventConstants.RELEASED &&
                keyCode != Constants.KEYCODE_SELECT) {
            // in this case we don't want to traverse on key release

        } else {
            switch (keyCode) {
                case Constants.KEYCODE_SELECT:
                    if (currentKey != null) {
                        int key = currentKey.getKey();
                        ret = true;
                        switch (key) {
                            case Key.CAPS_KEY:
                                if (type == EventConstants.PRESSED) break;
                                if (currentKeyboardType.equals(LOWER_ALPHABETIC_KEYBOARD)) {
                                    changeKeyboad(UPPER_ALPHABETIC_KEYBOARD);
                                } else if (currentKeyboardType.equals(UPPER_ALPHABETIC_KEYBOARD)) {
                                    changeKeyboad(LOWER_ALPHABETIC_KEYBOARD);
                                }
                                break;
                            case Key.SYMBOL_MODE_KEY:
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(SYBOLIC_KEYBOARD);
                                break;
                            case Key.NUMERIC_MODE_KEY:
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(NUMERIC_KEYBOARD);
                                break;
                            case Key.ALPHA_MODE_KEY:
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(LOWER_ALPHABETIC_KEYBOARD);
                                break;
                            case Key.BACKSPACE_KEY:
                                if (type == EventConstants.RELEASED) break;

                                vkl.virtualKeyPressed(KEYCODE_CLEAR);
                                break;
                            case Key.ENTER_KEY:
                                if (type == EventConstants.RELEASED) break;

                                vkl.virtualKeyPressed(KEYCODE_ENTER);
                                break;

                            default:
                                if (type == EventConstants.PRESSED) {
                                    vkl.virtualKeyPressed(key);
                                } else {
                                    vkl.virtualKeyReleased(currentKey.getKey());
                                }
                                ret = false;
                        }
                    }
            }
        }

        if (ret) {
            vkl.repaintVirtualKeyboard();
        }
        return ret;
    }

    /**
     * paint the virtual keyboard on the screen
     *
     * @param g The graphics context to paint to
     */
    public void paint(Graphics g) {

        g.setFont(VirtualKeyboardSkin.FONT);

        for (int i = 0; i < currentKeyboard.length; i++) {

            for (int j = 0; j < currentKeyboard[i].length; j++) {
                Key key = currentKeyboard[i][j];
                key.paint(g, (key == currentKey));
            }
        }
    }
    
    /**
     * Set up new coefficients of shrink and resize keyboard. Move keys in new coordinates.
     * @param kshrinkX - coefficient of shrink on X-dimension
     * @param kshrinkY - coefficient of shrink on Y-dimension
     */
    public void resize(double kshrinkX, double kshrinkY) {
        shrinkX = kshrinkX;
        shrinkY = kshrinkY; 
        resize();       
    }
    
    /**
     * Resize keyboard with cashed coefficients. 
     */
    private void resize() {
       for (int i = 0; i < currentKeyboard.length; i++) {
           for (int j = 0; j < currentKeyboard[i].length; j++) {
               Key key = currentKeyboard[i][j];
    	       key.resize(shrinkX, shrinkY);
           } 
       }       
    }

    /**
     * Handle input from a pen tap. Parameters describe
     * the type of pen event and the x,y location in the
     * layer at which the event occurred. Important : the
     * x,y location of the pen tap will already be translated
     * into the coordinate space of the layer.
     *
     * @param type the type of pen event
     * @param x    the x coordinate of the event
     * @param y    the y coordinate of the event
     */
    public boolean pointerInput(int type, int x, int y) {

        Key newKey = null;

        for (int i = 0; i < currentKeyboard.length; i++) {
            for (int j = 0; j < currentKeyboard[i].length; j++) {
                Key key = currentKeyboard[i][j];
                if (key.pointerInput(type, x, y)) {
                    newKey = key;
                    line = i;
                    column = j;
                }
            }
        }

        switch (type) {
            case EventConstants.PRESSED:

                if (newKey != null) {
                    currentKey = newKey;
                    // press on valid key
                    keyInput(type, Constants.KEYCODE_SELECT);
                }
                break;
            case EventConstants.RELEASED:
                if (newKey != null) {
                    currentKey = newKey;
                    keyInput(type, Constants.KEYCODE_SELECT);
                }

                break;
        }

        return true;
    }

    /**
     * Load keyboard structure from resources
     * @return
     */
    private Hashtable loadKeyboardMaps() {
        return Keyboards.getKeyboards();
    }

    /**
     * Change type of keyboard
     * @param newType type of new shown keyboard
     */
    public void changeKeyboad(String newType) {
        if (keyboardsMap.containsKey(newType)) {
            currentKeyboardType = newType;
            currentKeyboard = (Key[][]) keyboardsMap.get(newType);
            line = 0;
            column = 0;
            //coefficients of shrink are cashed from previous time
            resize();
        }
    }

    /**
     * Method return true if current virtual keybpard implementation supports java virtual keyboard
     * @return status of java virtual keyboard support
     */
    public static boolean isSupportJavaKeyboard() {
        return true;
    }
}