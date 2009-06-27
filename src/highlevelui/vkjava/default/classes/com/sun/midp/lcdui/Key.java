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
    /* key sets virtual keyboard into numeric mode */
    public final static int GAME_MODE_KEY = 6;
    /* Left arrow canvas key */
    public final static int LEFT_ARROW_KEY = 7;
    /* Right arrow canvas key */
    public final static int RIGHT_ARROW_KEY = 8;
    /* Up arrow canvas key */
    public final static int UP_ARROW_KEY = 9;
    /* Down arrow canvas key */
    public final static int DOWN_ARROW_KEY = 10;
    /* Fire canvas key */
    public final static int FIRE_KEY = 11;
    /* Enter key */
    public final static int ENTER_KEY = 12;
    /* Soft button 1 (typicaly left) key */
    public final static int SB1_KEY = 13;
    /* Soft button 2 (typicly right) key */
    public final static int SB2_KEY = 14;
    /* Game A key */
    public final static int GAMEA_KEY = 15;
    /* Game B key */
    public final static int GAMEB_KEY = 16;
    /* Game C key */
    public final static int GAMEC_KEY = 17;
    /* Game D key */
    public final static int GAMED_KEY = 18;
    /* Game Soft Buton 1 key */
    public final static int GAME_SB1_KEY = 19;
    /* Game Soft Buton 2 key */
    public final static int GAME_SB2_KEY = 20;
   /* Numeric key, type 1 */
    public final static int NUM1_KEY = 21;
   /* Numeric key, type 2 */
    public final static int NUM2_KEY = 22;



    
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
                keyImage = VirtualKeyboardSkin.BTN_NUMERIC_MODE_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_NUMERIC_MODE_SEL;
                break;
            case GAME_MODE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAME_MODE_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAME_MODE_SEL;
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
           case FIRE_KEY:
                keyImage = VirtualKeyboardSkin.BTN_MID_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_MID_SEL;
                break;
            case ENTER_KEY:
                keyImage = VirtualKeyboardSkin.BTN_ENTER;
                break;
            case SB1_KEY:
                keyImage = VirtualKeyboardSkin.BTN_SB_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_SB_SEL;
                break;
           case SB2_KEY:
                keyImage = VirtualKeyboardSkin.BTN_SB_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_SB_SEL;
                break;
           case GAME_SB1_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAME_SB_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAME_SB_SEL;
                break;
           case GAME_SB2_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAME_SB_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAME_SB_SEL;
                break;
           case GAMEA_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAMEA_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAMEA_SEL;
                break;
           case GAMEB_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAMEB_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAMEB_SEL;
                break;
           case GAMEC_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAMEC_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAMEC_SEL;
                break;
           case GAMED_KEY:
                keyImage = VirtualKeyboardSkin.BTN_GAMED_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_GAMED_SEL;
                break;
           case NUM1_KEY:
                keyImage = VirtualKeyboardSkin.BTN_NUM1_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_NUM1_SEL;
                break;
           case NUM2_KEY:
                keyImage = VirtualKeyboardSkin.BTN_NUM2_UN;
                keyImageSelected = VirtualKeyboardSkin.BTN_NUM2_SEL;
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
        int color = g.getColor();
        if (selected) {
            g.setColor(VirtualKeyboardSkin.COLOR_SEL);
            if (keyImageSelected != null) {
                g.drawImage(keyImageSelected, x, y, Graphics.TOP | Graphics.LEFT);
            } else {
                if (keyImage != null) {
                    g.drawImage(keyImage, x, y, Graphics.TOP | Graphics.LEFT);
                }
                g.drawRect(x,y,width,height);
            }
        } else {
            g.setColor(VirtualKeyboardSkin.COLOR_UN);
            if (keyImage != null) {
                g.drawImage(keyImage, x, y, Graphics.TOP | Graphics.LEFT);
            }
        }
        
        if (key > 0) {
                        // Draw text version
            int startY = (height - VirtualKeyboardSkin.FONT.getHeight()) >> 1;
            int startX = (width - VirtualKeyboardSkin.FONT.charWidth((char)key)) >> 1;
            if (startY < 0) {
                startY = 0;
            }
            if (startX < 0) {
                startX = 0;
            }
            
            g.drawChar((char) key, x + startX, y + startY, Graphics.TOP | Graphics.LEFT);
        }
        g.setColor(color);
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
        if (keyType == GENERAL_KEY ||
            keyType == NUM1_KEY ||
            keyType == NUM2_KEY) {
            return key;
        } else {
            return keyType;
        }
    }
    
    public void resize(double dX, double dY) {
        this.x = startX + (int)dX;
    	this.y = startY + (int)dY;       
    }

}
