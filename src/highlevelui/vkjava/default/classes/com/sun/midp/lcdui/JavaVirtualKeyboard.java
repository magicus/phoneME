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
import java.util.TimerTask;
import java.util.Timer;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;


/**
 * This is a popup layer that show java virtual keyboard content
 */
public class JavaVirtualKeyboard implements VirtualKeyboard {

    /**
     * Backspace
     */
    public static final int KEYCODE_CLEAR = -8;
    public static final int KEYCODE_ENTER = -5;

    /* Instance of VirtualKeyboard class */
    private static VirtualKeyboard virtualKeyboard;

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


    /** The Timer to service TimerTasks. */ 
    protected Timer timer;
    protected TimerKey timerKey;


    /**
     * Virtual Keyboard constructor.
     * @param listener for handling virtual keyboard events
     */
    JavaVirtualKeyboard(VirtualKeyboardListener listener) {
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
    public static VirtualKeyboard getInstance(VirtualKeyboardListener listener) {
        if (virtualKeyboard == null) {
            virtualKeyboard = new JavaVirtualKeyboard(listener);
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
            if (keyCode == Constants.KEYCODE_SELECT) {
                if (currentKey != null) {
                    int key = currentKey.getKey();
                    ret = true;
                    switch (key) {
                    case Key.CAPS_KEY:
                        if (type == EventConstants.RELEASED) {
                            if (currentKeyboardType.equals(LOWER_ALPHABETIC_KEYBOARD)) {
                                changeKeyboard(UPPER_ALPHABETIC_KEYBOARD);
                            } else if (currentKeyboardType.equals(UPPER_ALPHABETIC_KEYBOARD)) {
                                changeKeyboard(LOWER_ALPHABETIC_KEYBOARD);
                            }
                        }
                        break;
                    case Key.SYMBOL_MODE_KEY:
                        if (type == EventConstants.RELEASED) {
                            changeKeyboard(SYBOLIC_KEYBOARD);
                        }
                        break;
                    case Key.GAME_MODE_KEY:
                        if (type == EventConstants.RELEASED) {
                            changeKeyboard(GAME_KEYBOARD);
                        }
                        break;
                    case Key.NUMERIC_MODE_KEY:
                        if (type == EventConstants.RELEASED) {
                            changeKeyboard(NUMERIC_KEYBOARD);
                        }
                        break;
                    case Key.ALPHA_MODE_KEY:
                        if (type == EventConstants.RELEASED) {
                            changeKeyboard(LOWER_ALPHABETIC_KEYBOARD);
                        }
                        break;
                    case Key.BACKSPACE_KEY:
                        key = KEYCODE_CLEAR;
                        processKey(type,key);
                        break;
                    case Key.ENTER_KEY:
                        key = KEYCODE_ENTER;
                        processKey(type, key);
                        break;
                    case Key.LEFT_ARROW_KEY:
                        key = EventConstants.SYSTEM_KEY_LEFT;
                        processKey(type, key);
                        break;
                    case Key.RIGHT_ARROW_KEY:
                        key = EventConstants.SYSTEM_KEY_RIGHT;
                        processKey(type, key);
                        break;
                    case Key.UP_ARROW_KEY:
                        key = EventConstants.SYSTEM_KEY_UP;
                        processKey(type, key);
                        break;
                    case Key.DOWN_ARROW_KEY:
                        key = EventConstants.SYSTEM_KEY_DOWN;
                        processKey(type, key);
                        break;
                    case Key.FIRE_KEY:
                        key = EventConstants.SYSTEM_KEY_FIRE;
                        processKey(type, key);
                        break;
                    case Key.SB2_KEY:
                        key = EventConstants.SOFT_BUTTON2;
                        processKey(type, key);
                        break;
                    case Key.SB1_KEY:
                        key = EventConstants.SOFT_BUTTON1;
                        processKey(type, key);
                        break;
                    case Key.GAMEA_KEY:
                        key = EventConstants.SYSTEM_KEY_GAMEA;
                        processKey(type, key);
                        break;
                    case Key.GAMEB_KEY:
                        key = EventConstants.SYSTEM_KEY_GAMEB;
                        processKey(type, key);
                        break;
                    case Key.GAMEC_KEY:
                        key = EventConstants.SYSTEM_KEY_GAMEC;
                        processKey(type, key);
                        break;
                    case Key.GAMED_KEY:
                        key = EventConstants.SYSTEM_KEY_GAMED;
                        processKey(type, key);
                        break;
                    default:
                        processKey(type, key);
                        ret = false;
                    }
                }
            }
        }
        
        if (ret && vkl != null) {
            vkl.repaintVirtualKeyboard();
        }
        return ret;
    }

    protected void processKey(int type, int keyCode) {
        if (vkl != null) {
            switch(type) {
            case EventConstants.PRESSED:
                vkl.virtualKeyPressed(keyCode);
                break;
            case EventConstants.RELEASED:
                vkl.virtualKeyReleased(keyCode);
                break;
            case EventConstants.REPEATED:
                vkl.virtualKeyRepeated(keyCode);
                break;
            }
        }
    }

    /**
     * Set listener
     *
     * @param listener  new virtual keyboard listener
     */
    public void setListener(VirtualKeyboardListener listener) {
        vkl = listener;
    }
        
    /**
     * paint the virtual keyboard on the screen
     *
     * @param g The graphics context to paint to
     */
    public void paint(Graphics g) {
        g.setFont(VirtualKeyboardSkin.FONT);
        if (currentKeyboard != null) {
            for (int i = 0; i < currentKeyboard.length; i++) {
                for (int j = 0; j < currentKeyboard[i].length; j++) {
                    Key key = currentKeyboard[i][j];
                    key.paint(g, (key == currentKey));
                }
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
        if (currentKeyboard != null) {
            for (int i = 0; i < currentKeyboard.length; i++) {
                for (int j = 0; j < currentKeyboard[i].length; j++) {
                    Key key = currentKeyboard[i][j];
                    key.resize(shrinkX, shrinkY);
                } 
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
        if (currentKeyboard == null)
            return false;
        
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
                    if (Constants.REPEAT_SUPPORTED) {
                        startTimerKey();
                    }
                    
                    // press on valid key
                    keyInput(type, Constants.KEYCODE_SELECT);
                }
                break;
            case EventConstants.RELEASED:
                if (Constants.REPEAT_SUPPORTED) {
                    releaseTimerKey();
                }
 
                if (currentKey != null) {
                    keyInput(type, Constants.KEYCODE_SELECT);
                    currentKey = null;
                }
                break;
        }

        return true;
    }

    protected void startTimerKey() {
        try {
            if (timer == null) {
                timer = new Timer();
            }
            timerKey = new TimerKey();
            timer.schedule(timerKey, Constants.REPEAT_TIMEOUT, Constants.REPEAT_PERIOD); 
        } catch (IllegalStateException e) { 
            if (Logging.REPORT_LEVEL <= Logging.INFORMATION) { 
                Logging.report(Logging.INFORMATION, LogChannels.LC_HIGHUI, 
                               "Exception caught in setTimer"); 
            } 
            releaseTimerKey(); 
        }
    }

    protected void releaseTimerKey() {
        if (timerKey != null) {
            timerKey.cancel();
            timerKey = null;
        }
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
    public void changeKeyboard(String newType) {
        if (keyboardsMap.containsKey(newType)) {
            currentKeyboardType = newType;
            currentKeyboard = (Key[][]) keyboardsMap.get(newType);
            if (currentKeyboard == null) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) { 
                    Logging.report(Logging.WARNING, LogChannels.LC_HIGHUI, 
                                   "Virtual keyboard " + newType + " is not supported"); 
                } 
            }
            line = 0;
            column = 0;
            //coefficients of shrink are cashed from previous time
            resize();
        }
    }

    /** Timer to indicate long key press */ 
    class TimerKey extends TimerTask {
        /**
         * As soon as timer occures uCallKeyPressed has to be called 
         * and timer has to be stopped 
         */ 
        public final void run() {
            if (currentKey != null) {
                keyInput(EventConstants.REPEATED, Constants.KEYCODE_SELECT);
            }
        } 
    }
    
    /**
     * Runs virtual keyboard to edit the given text.
     * Returns changed text.
     * @param text
     *  initial text or <code>null</code>;
     * @param maxChars
     *  maximum number of characters in the return string;
     * @param constraint
     *  text editing constraint id;
     * @throws IllegalArgumentException
     *  if number of characters in the initial text is larger then <code>maxChars</code> value;
     *  if <code>maxChars</code> is zero;
     *  if <code>modes</code> or <code>constraint</code> are not recognized;
     * @throws RuntimeException
     *  if editing session was interrupted; e.g. if vm is suspended editing session is interrupted.
     */
    public String editText(String text, int maxChars,
                           int constraints) throws InterruptedException {
        return null;
    }
}

