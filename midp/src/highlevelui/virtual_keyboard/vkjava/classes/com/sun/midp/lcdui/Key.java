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
import com.sun.midp.chameleon.skins.*;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Graphics;


/**
 * Key of Virtual keyboard
 */
public class Key {

    final static int RELEASED = 0;
    final static int PRESSED = 1;

    /*Key types*/

    /* letter key*/
    public final static int GENERAL_KEY = 0;
    /* caps key */
    public final static int CAPS_KEY = 1;
    /* backspase key */
    public final static int BACKSPACE_KEY = 2;
    /* Key sets virtual keyboard into alphabetic mode */
    public final static int ALPHA_MODE_KEY = 3;
    /* Key sets virtual keyboard into symbolic mode */
    public final static int SYMBOL_MODE_KEY = 4;
    /* key sets virtual keyboard into numeric mode */
    public final static int NUMERIC_MODE_KEY = 5;
    /* Left arrow canvas key */
    public final static int LEFT_ARROW_KEY = 6;
    /* Right arrow canvas key */
    public final static int RIGHT_ARROW_KEY = 7;
    /* Up arrow canvas key */
    public final static int UP_ARROW_KEY = 8;
    /* Down arrow canvas key */
    public final static int DOWN_ARROW_KEY = 9;
    /* Enter key */
    public final static int ENTER_KEY = 10;

    /*Key code*/
    private int key;
    /*Image for key background*/
    private Image keyImage;
    /*Image for selected key background*/
    private Image keyImageSelected;
    /* Key type */
    private int keyType;
    /* x coordinate of key */
    private int x;
    /* y coordinate of type */
    private int y;
    /* width of key */
    private int width;
    /* height of key */
    private int height;

    private int startX;
    private int startY;
    /**
     * Constructor
     * @param key - key code
     * @param x coordinate
     * @param y coordinate
     * @param keyType key type
     */
    public Key(int key, int x, int y, int keyType) {
        this.key = key;
        this.x = x;
        this.y = y;
        this.startX = x;
        this.startY = y;
        this.keyType = keyType;
        
        switch (keyType) {
            case BACKSPACE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_BACKSPACE;
                break;
            case CAPS_KEY:
                keyImage = VirtualKeyboardSkin.BTN_CAPS;
                break;
            case ALPHA_MODE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_ALPHA_MODE;
                break;
            case SYMBOL_MODE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_SYMBOL_MODE;
                break;
            case NUMERIC_MODE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_NUMERIC_MODE;
                break;
            case LEFT_ARROW_KEY:
                keyImage = VirtualKeyboardSkin.BTN_LEFT_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_LEFT_SEL;
                break;
            case RIGHT_ARROW_KEY:
                keyImage = VirtualKeyboardSkin.BTN_RIGHT_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_RIGHT_SEL;
                break;
            case UP_ARROW_KEY:
                keyImage = VirtualKeyboardSkin.BTN_UP_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_UP_SEL;
                break;
            case DOWN_ARROW_KEY:
                keyImage = VirtualKeyboardSkin.BTN_DOWN_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_DOWN_SEL;
                break;
            case ENTER_KEY:
                keyImage = VirtualKeyboardSkin.BTN_ENTER;
                break;
            default:
                keyImage = VirtualKeyboardSkin.KEY;
                break;
        }
        width = keyImage.getWidth();
        height = keyImage.getHeight();
    }

    /**
     * Paint key
     * @param g graphics
     */
    void paint(Graphics g, boolean selected) {

        g.setFont(VirtualKeyboardSkin.FONT);

        if (selected) {

          if (keyImageSelected != null) {
                g.drawImage(keyImageSelected, x, y, Graphics.TOP | Graphics.LEFT);
          } else {
              if (keyImage != null) {
                 g.drawImage(keyImage, x, y, Graphics.TOP | Graphics.LEFT);
              }
              g.drawRect(x,y,width,height);
          }
        } else {
            if (keyImage != null) {
                g.drawImage(keyImage, x, y, Graphics.TOP | Graphics.LEFT);
            }
        }

        if (key > 0) {
            // Draw text version
            g.drawChar((char) key, x + 4, y + 4, Graphics.TOP | Graphics.LEFT);
        }

    }

    /**
     * Helper function to determine the itemIndex at the x,y position
     *
     * @param x,y pointer coordinates in menuLayer's space (0,0 means left-top
     *            corner) both value can be negative as menuLayer handles the pointer
     *            event outside its bounds
     * @return menuItem's index since 0, or PRESS_OUT_OF_BOUNDS, PRESS_ON_TITLE
     */
    private boolean isKeyAtPointerPosition(int x, int y) {

        if ((x >= this.x) &&
                (y >= this.y) &&
                (x < this.x + this.width) &&
                (y < this.y + this.height))
            return true;

        return false;
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
        switch (type) {
            case EventConstants.PRESSED:
            case EventConstants.RELEASED:
                if (isKeyAtPointerPosition(x, y)) {
                    return true;
                }

                break;
        }
        return false;
    }

    /**
     * Return key code
     * @return key code
     */
    int getKey() {
        if (keyType == 0) {
            return key;
        } else {
            return keyType;
        }
    }
    
    public void resize(double newX, double newY) {
        this.x = (int)(startX*newX);
    	this.y = (int)(startY*newY);       
    }

}
