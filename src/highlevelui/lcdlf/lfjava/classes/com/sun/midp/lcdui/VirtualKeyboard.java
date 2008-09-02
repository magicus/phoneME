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
import com.sun.midp.chameleon.skins.ScreenSkin;
import com.sun.midp.chameleon.skins.VirtualKeyboardSkin;
import com.sun.midp.util.ResourceHandler;
import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.SecurityToken;

import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import java.util.*;

/**
 * This is a popup layer that show virtual keyboard content
 */

public class VirtualKeyboard {


    /**
     * indicates whether the virtual keyboard is opened automatically
     */
    public static final boolean USE_VIRTUAL_KEYBOARD_OPEN_AUTO = false;

    /**
     * Backspace
     */
    public static final int KEYCODE_CLEAR = -8;

    /**
     * instance of the virtual keyboard listener
     */
    VirtualKeyboardListener vkl;

    // keyboard dimensions
    public int kbX;
    public int kbY;
    public int kbWidth;
    public int kbHeight;

    int fontH;       // height of 'M'
    int buttonW;     // width of keyboard
    int buttonH;     // height of keyboard

    int currentChar = 0;
    public int currentKeyboardIndex = 0; // abc

    int textfieldHeight = 0; // height of text field area, including adornments

    char itemIndexWhenPressed;
    char PRESS_OUT_OF_BOUNDS = 0;

    /**
     * array of all available keys n the keyboard
     */
    boolean inShift = false;
    boolean textKbd = false;
    Font f;

    /**
     * Virtual Keyboard constructor.
     *
     * @param keys            array of available keys for the keyboard
     * @param vkl             the virtual keyboard listener
     * @param displayTextArea flag to indicate whether to display the text area
     */
    public VirtualKeyboard(Vector maps,
                           VirtualKeyboardListener vkl,
                           boolean displayTextArea) {
        
        this.keyboardMaps = maps;

        this.vkl = vkl;

        textKbd = displayTextArea;
        if (textKbd) {
            changeKeyboad(1);
            PADDING = 4;
        } else {
            changeKeyboad(0);
        }


        kbWidth = ScreenSkin.WIDTH;

        kbHeight = 0;

        for (int h = 0; h < keyboardMaps.size(); h++) {
            Vector keyboard = (Vector) keyboardMaps.elementAt(h);
            for (int i = 0; i < keyboard.size(); i++) {

                Vector line = (Vector) keyboard.elementAt(i);
                for (int j = 0; j < line.size(); j++) {
                    Key key = (Key) (line.elementAt(j));
                    int keyBottom = key.getY() + key.getHeight();
                    if (keyBottom > kbHeight)
                        kbHeight = keyBottom;
                }
            }
        }

        if (textKbd) {
            kbHeight += 2 * PADDING;
        }

        f = Font.getFont(Font.FACE_PROPORTIONAL, // or SYSTEM
                Font.STYLE_PLAIN,
                Font.SIZE_SMALL);
        fontH = f.getHeight();


        kbX = 0;
        kbY = ScreenSkin.HEIGHT / 2 - kbHeight;
    }



    /**
     * Checks if the virtual keyboard is opened automatically.
     *
     * @return <code>true</code> if the virtual keyboard is opened automatically,
     *         <code>false</code> otherwise.
     */
    static boolean isAutoOpen() {
        return USE_VIRTUAL_KEYBOARD_OPEN_AUTO;
    }

    /**
     * traverse the virtual keyboard according to key pressed.
     *
     * @param type    type of keypress
     * @param keyCode key code of key pressed
     */
    public boolean traverse(int type, int keyCode) {

        boolean ret = false;

        if (type == EventConstants.RELEASED &&
                keyCode != Constants.KEYCODE_SELECT) {
            // in this case we don't want to traverse on key release

        } else {
            switch (keyCode) {
                case Constants.KEYCODE_RIGHT: {
                    int pos = currentLine.indexOf(currentKey);
                    pos++;
                    if (pos >= currentLine.size()) {
                        pos = 0;
                    }
                    currentKey = (Key) currentLine.elementAt(pos);
                    ret = true;
                }
                ;
                break;
                case Constants.KEYCODE_LEFT: {
                    int pos = currentLine.indexOf(currentKey);
                    pos--;
                    if (pos < 0) {
                        pos = currentLine.size() - 1;
                    }
                    currentKey = (Key) currentLine.elementAt(pos);
                    ret = true;
                }
                ;
                break;
                case Constants.KEYCODE_UP:
                    int xpos = currentLine.indexOf(currentKey);
                    int ypos = currentKeyboard.indexOf(currentLine);

                    ypos--;

                    if (ypos < 0) {
                        ypos = currentKeyboard.size() - 1;
                    }
                    currentLine = (Vector) currentKeyboard.elementAt(ypos);
                    if (xpos >= currentLine.size())
                        xpos = currentLine.size() - 1;
                    currentKey = (Key) currentLine.elementAt(xpos);
                    ret = true;
                    break;
                case Constants.KEYCODE_DOWN:
                    xpos = currentLine.indexOf(currentKey);
                    ypos = currentKeyboard.indexOf(currentLine);

                    ypos++;

                    if (ypos >= currentKeyboard.size()) {
                        ypos = 0;
                    }
                    currentLine = (Vector) currentKeyboard.elementAt(ypos);
                    if (xpos >= currentLine.size())
                        xpos = currentLine.size() - 1;
                    currentKey = (Key) currentLine.elementAt(xpos);
                    ret = true;
                    break;
                case Constants.KEYCODE_SELECT:
                    if (currentKey != null) {
                        int key = currentKey.getKey();

                        switch (key) {
                            case SHIFT_META_KEY: //"Shift" - one shot upper case
                                if (type == EventConstants.PRESSED) break;
                                if (currentKeyboardIndex == 1) {  // lower case
                                    changeKeyboad(2);
                                    inShift = true;
                                } else if (currentKeyboardIndex == 2) {  // lower case
                                    changeKeyboad(1);
                                    inShift = false;
                                }
                                break;
                            case CAPS_META_KEY: //"CapsL"  (caps lock)
                                if (type == EventConstants.PRESSED) break;
                                if (currentKeyboardIndex == 1) {  // lower case
                                    changeKeyboad(2);
                                } else if (currentKeyboardIndex == 2) {  // upper case
                                    changeKeyboad(1);
                                }
                                break;
                            case MODE_SYMBOL_META_KEY: //"Mode"
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(3); // Symbol
                                break;
                            case MODE_NUMERIC_META_KEY: //"Mode"  
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(0); // Numeric
                                break;
                            case MODE_ALPHA_META_KEY: //"Mode"  
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad(1); // Alpha
                                break;
                            case BACKSPACE_META_KEY: //"backspace"
                                if (type == EventConstants.RELEASED) break;
                                vkl.virtualKeyPressed(KEYCODE_CLEAR);
                                break;
                            case OK_META_KEY: //"ok"
                                if (type == EventConstants.PRESSED) break;
                                break;
                            case CANCEL_META_KEY: //"cancel"
                                if (type == EventConstants.PRESSED) break;
                                break;

                            default:
                                if (type == EventConstants.PRESSED) {
                                    currentKey.setStatus(Key.PRESSED);
                                    vkl.virtualKeyPressed(currentKey.getKey());
                                    if (inShift) {
                                        //shift is a one-shot upper case
                                        inShift = false;
                                        if (textKbd) {
                                            changeKeyboad(1);
                                        } //hk : still need a keyboard displayed
                                        else {
                                            changeKeyboad(0);
                                        }
                                    }
                                } else {
                                    currentKey.setStatus(Key.RELEASED);
                                    vkl.virtualKeyReleased(currentKey.getKey());
                                }
                        }
                    }
                    ret = true;
            }
        }

        // triggers paint()
        vkl.repaintVirtualKeyboard();
        return ret;
    }

    /**
     * paint the virtual keyboard on the screen
     *
     * @param g The graphics context to paint to
     */
    public void paint(Graphics g) {

        g.setFont(f);

        g.translate(0, 2 * PADDING);
        for (int i = 0; i < currentKeyboard.size(); i++) {
            Vector line = (Vector) currentKeyboard.elementAt(i);
            for (int j = 0; j < line.size(); j++) {
                Key key = (Key) (line.elementAt(j));
                key.paint(g);
                if (key == currentKey) {
                    key.paintSelection(g);
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


        Vector newLine = null;
        Key newKey = null;

        if (textfieldHeight > 0) {
            y -= (textfieldHeight + 2 * PADDING);
        }

        for (int i = 0; i < currentKeyboard.size(); i++) {
            Vector line = (Vector) currentKeyboard.elementAt(i);
            for (int j = 0; j < line.size(); j++) {
                Key key = (Key) (line.elementAt(j));
                if (key.pointerInput(type, x, y)) {
                    newKey = key;
                    newLine = line;
                }
            }
        }

        switch (type) {
            case EventConstants.PRESSED:

                if (newKey != null) {
                    currentKey.setStatus(Key.RELEASED);
                    currentKey = newKey;
                    currentLine = newLine;
                    // press on valid key
                    traverse(type, Constants.KEYCODE_SELECT);
                    vkl.repaintVirtualKeyboard();

                }
                break;
            case EventConstants.RELEASED:
                if (newKey != null) {
                    currentKey.setStatus(Key.RELEASED);
                    currentKey = newKey;
                    currentLine = newLine;
                    traverse(type, Constants.KEYCODE_SELECT);
                    vkl.repaintVirtualKeyboard();

                }

                break;
        }
        // return true always as menuLayer will capture all of the pointer inputs
        return true;
    }

    /**
     * Prepare key map
     */
    public static Vector prepareKeyMapTextField() {

        Vector Maps = new Vector();

        Image icon = VirtualKeyboardSkin.KEY;
        Image otherIcon = VirtualKeyboardSkin.KEY;
        int width = icon.getWidth();
        int height = icon.getHeight();
        int xpos;
        int ypos;
        int pad = PADDING;

        Vector Keyboard = new Vector();

        Vector Lines = new Vector();
        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos = 0;
        Lines.addElement(new Key(icon, otherIcon, '1', '1', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '2', '2', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '3', '3', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '4', '4', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '5', '5', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '6', '6', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '7', '7', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '8', '8', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '9', '9', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '0', '0', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);

        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, '+', '+', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '-', '-', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '*', '*', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '/', '/', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '=', '=', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '.', '.', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ',', ',', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ';', ';', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '\'', '\'', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '\"', '\"', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);


        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 7 * width) / 2 - width * 3 / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.CAPS_META_KEY, '^', xpos, ypos, width, height));

        xpos = pad + (ScreenSkin.WIDTH - pad - 7 * width) / 2;

        Lines.addElement(new Key(icon, otherIcon, '~', '~', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '[', '[', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ']', ']', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '|', '|', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '&', '&', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '(', '(', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ')', ')', xpos, ypos, width, height));
        xpos += width * 3 / 2 + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.BACKSPACE_META_KEY, '<', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);

        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 5 * width) / 2;
        ypos += height + pad;


        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_ALPHA_META_KEY, 'a', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ' ', ' ', xpos, ypos, 3 * width, height));
        xpos += 3 * width + pad;
        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_SYMBOL_META_KEY, '$', xpos, ypos, width, height));
        xpos += width + pad;


        Keyboard.addElement(Lines);

        Maps.addElement(Keyboard);
        Keyboard = new Vector();
        Lines = new Vector();


        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos = 0;
        Lines.addElement(new Key(icon, otherIcon, 'q', 'q', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'w', 'w', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'e', 'e', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'r', 'r', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 't', 't', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'y', 'y', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'u', 'u', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'i', 'i', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'o', 'o', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'p', 'p', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);

        Lines = new Vector();
        xpos = pad + (ScreenSkin.WIDTH - pad - 9 * width) / 2;
        ypos += height + pad;
        Lines.addElement(new Key(icon, otherIcon, 'a', 'a', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 's', 's', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'd', 'd', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'f', 'f', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'g', 'g', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'h', 'h', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'j', 'j', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'k', 'k', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'l', 'l', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);

        Lines = new Vector();

        xpos = (ScreenSkin.WIDTH - pad - 7 * width) / 2 - width * 3 / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.CAPS_META_KEY, '^', xpos, ypos, width, height));

        xpos = pad + (ScreenSkin.WIDTH - pad - 7 * width) / 2;

        Lines.addElement(new Key(icon, otherIcon, 'z', 'z', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'x', 'x', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'c', 'c', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'v', 'v', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'b', 'b', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'n', 'n', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'm', 'm', xpos, ypos, width, height));
        xpos += 1.5 * width + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.BACKSPACE_META_KEY, '<', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);

        Lines = new Vector();
        xpos = pad + (ScreenSkin.WIDTH - pad - 5 * width) / 2;

        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_SYMBOL_META_KEY, '$', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ' ', ' ', xpos, ypos, 3 * width, height));
        xpos += 3 * width + pad;
        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_NUMERIC_META_KEY, '1', xpos, ypos, width, height));
        xpos += width + pad;
        

        Keyboard.addElement(Lines);
        Maps.addElement(Keyboard);
        Keyboard = new Vector();
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos = 0;
        Lines.addElement(new Key(icon, otherIcon, 'Q', 'Q', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'W', 'W', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'E', 'E', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'R', 'R', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'T', 'T', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'Y', 'Y', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'U', 'U', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'I', 'I', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'O', 'O', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'P', 'P', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 9 * width) / 2;
        ypos += height + pad;
        Lines.addElement(new Key(icon, otherIcon, 'A', 'A', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'S', 'S', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'D', 'D', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'F', 'F', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'G', 'G', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'H', 'H', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'J', 'J', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'K', 'K', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'L', 'L', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 7 * width) / 2 - width * 3 / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.CAPS_META_KEY, '^', xpos, ypos, width, height));

        xpos = pad + (ScreenSkin.WIDTH - pad - 7 * width) / 2;

        Lines.addElement(new Key(icon, otherIcon, 'Z', 'Z', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'X', 'X', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'C', 'C', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'V', 'V', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'B', 'B', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'N', 'N', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, 'M', 'M', xpos, ypos, width, height));
        xpos += width * 3 / 2 + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.BACKSPACE_META_KEY, '<', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 5 * width) / 2;
        ypos += height + pad;


        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_SYMBOL_META_KEY, '$', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ' ', ' ', xpos, ypos, 3 * width, height));
        xpos += 3 * width + pad;
        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_NUMERIC_META_KEY, 'X', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Maps.addElement(Keyboard);
        Keyboard = new Vector();
        Lines = new Vector();


        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos = 0;
        Lines.addElement(new Key(icon, otherIcon, '!', '!', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '@', '@', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '#', '#', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '$', '$', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '%', '%', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '^', '^', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '&', '&', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '*', '*', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '(', '(', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ')', ')', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 10 * width) / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, '+', '+', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '-', '-', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '*', '*', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '/', '/', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '.', '.', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ',', ',', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ';', ';', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '\'', '\'', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '\"', '\"', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '~', '~', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 8 * width) / 2 - width * 3 / 2;
        ypos += height + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.CAPS_META_KEY, '^', xpos, ypos, width, height));

        xpos = pad + (ScreenSkin.WIDTH - pad - 8 * width) / 2;


        Lines.addElement(new Key(icon, otherIcon, '_', '_', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ':', ':', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '<', '<', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '>', '>', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '{', '{', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '}', '}', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, '[', '[', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ']', ']', xpos, ypos, width, height));
        xpos += width * 3 / 2 + pad;

        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.BACKSPACE_META_KEY, '<', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Lines = new Vector();

        xpos = pad + (ScreenSkin.WIDTH - pad - 5 * width) / 2;
        ypos += height + pad;


        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_ALPHA_META_KEY, 'a', xpos, ypos, width, height));
        xpos += width + pad;
        Lines.addElement(new Key(icon, otherIcon, ' ', ' ', xpos, ypos, 3 * width, height));
        xpos += 3 * width + pad;
        Lines.addElement(new Key(icon, otherIcon, VirtualKeyboard.MODE_NUMERIC_META_KEY, '1', xpos, ypos, width, height));
        xpos += width + pad;

        Keyboard.addElement(Lines);
        Maps.addElement(Keyboard);

        return Maps;

    }

    /**
     * Prepare key map for Canvas keypad.
     */
    public static Vector prepareKeyMapCanvas() {

        Vector Maps = new Vector();

        Vector Keyboard = new Vector();

        Vector Lines = new Vector();
        Lines.addElement(new Key(VirtualKeyboardSkin.BTN_UP_UN, VirtualKeyboardSkin.BTN_UP_SEL,
                Constants.KEYCODE_UP,
                VirtualKeyboardSkin.BTN_LEFT_UN.getWidth() - 1,
                0));
        Lines.addElement(new Key(VirtualKeyboardSkin.BTN_LEFT_UN, VirtualKeyboardSkin.BTN_LEFT_SEL,
                Constants.KEYCODE_LEFT,
                0,
                VirtualKeyboardSkin.BTN_UP_UN.getHeight() - 1));
        /* Lines.addElement(new Key(VirtualKeyboardSkin.BTN_MID_UN,VirtualKeyboardSkin.BTN_MID_SEL,
       Constants.KEYCODE_SELECT,
        left_un.getWidth()-2,
        up_un.getHeight()-2));*/
        Lines.addElement(new Key(VirtualKeyboardSkin.BTN_DOWN_UN, VirtualKeyboardSkin.BTN_DOWN_SEL,
                Constants.KEYCODE_DOWN,
                VirtualKeyboardSkin.BTN_LEFT_UN.getWidth() - 1,
                VirtualKeyboardSkin.BTN_UP_UN.getHeight() + VirtualKeyboardSkin.BTN_MID_UN.getHeight() - 1));
        Lines.addElement(new Key(VirtualKeyboardSkin.BTN_RIGHT_UN, VirtualKeyboardSkin.BTN_RIGHT_SEL,
                Constants.KEYCODE_RIGHT,
                VirtualKeyboardSkin.BTN_LEFT_UN.getWidth() + VirtualKeyboardSkin.BTN_MID_UN.getWidth() - 1,
                VirtualKeyboardSkin.BTN_UP_UN.getHeight() - 1));


//        Lines.addElement(new Key(getImageFromInternalStorage("red_un_" + size),
//                getImageFromInternalStorage("red_sel_" + size),
//                '1', base, 10));
//        Lines.addElement(new Key(getImageFromInternalStorage("green_un_" + size),
//                getImageFromInternalStorage("green_sel_" + size)
//                , '3', base + size * 2, 10));
//        Lines.addElement(new Key(getImageFromInternalStorage("blue_un_" + size),
//                getImageFromInternalStorage("blue_sel_" + size),
//                '7', base + size / 2, 10 + size * 3 / 2));
//        Lines.addElement(new Key(getImageFromInternalStorage("yellow_un_" + size),
//                getImageFromInternalStorage("yellow_sel_" + size),
//                '9', base + size * 5 / 2, 10 + size * 3 / 2));

        Keyboard.addElement(Lines);

        Maps.addElement(Keyboard);

        return Maps;
    }

    // ********* attributes ********* //

    /**
     * padding between rows of buttons
     */
    private static int PADDING;

    // If you want to change the order of the buttons, just
    // change the serial numbers here:
    public final static int OK_META_KEY = -100;
    public final static int CANCEL_META_KEY = -101;
    public final static int MODE_NUMERIC_META_KEY = -102;
    public final static int MODE_SYMBOL_META_KEY = -103;
    public final static int MODE_ALPHA_META_KEY = -104;
    public final static int BACKSPACE_META_KEY = -105;
    public final static int SHIFT_META_KEY = -106;
    public final static int CAPS_META_KEY = -107;

    //When input method is changed, process this key to update UI 
    final static int IM_CHANGED_KEY = -99;

    public void changeKeyboad(int which) {
        currentKeyboardIndex = which;
        if (currentKeyboardIndex >= keyboardMaps.size()) {
            currentKeyboardIndex = keyboardMaps.size() - 1;
        }
        currentKeyboard = (Vector) keyboardMaps.elementAt(currentKeyboardIndex);
        currentLine = (Vector) currentKeyboard.elementAt(0);
        currentKey = (Key) currentLine.elementAt(0);
    }

    Vector keyboardMaps;
    Vector currentKeyboard;
    Vector currentLine;
    Key currentKey;
}

