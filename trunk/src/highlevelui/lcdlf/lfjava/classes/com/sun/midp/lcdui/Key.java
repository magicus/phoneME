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

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.layers.PopupLayer;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Graphics;


/**
 * This is a popup layer that handles a sub-popup within the text tfContext
 */

public class Key {

    final static int RELEASED = 0;
    final static int PRESSED = 1;

    public Key(Image regular, Image pressed, int actionkey, int displaykey, int x, int y, int w, int h) {
        init( regular, pressed,actionkey, displaykey, x, y, w, h);
    }

    public Key(Image regular, Image pressed, int keycode, int x, int y) {
        init(regular,pressed,keycode,0,x,y,regular.getWidth(),regular.getHeight());
    }

    void init(Image regular, Image pressed, int actionkey, int displaykey, int x, int y, int w, int h) {
        keyImages = new Image[2];
        keyImages[0]=regular;
        keyImages[1]=pressed;
        this.x = x;
        this.y = y;
        this.width = w;
        this.height = h;

        this.actionKey = actionkey;
        this.displayKey = displaykey;

    }

    private Image keyImages[];

    private int actionKey;
    private int displayKey;

    int x;
    int y;
    int width;
    int height;

    int status;

    /**
     * paint the virtual keyboard on the screen
     * 
     * @param g The graphics context to paint to
     */
     void paint(Graphics g) {

        if ((keyImages!=null) && (keyImages[status]!=null)) {
            g.drawImage(keyImages[status],x,y,Graphics.TOP|Graphics.LEFT);
        }

        if (displayKey >0) {
            // Draw text version
            g.setColor(BLACK);
            g.drawChar((char)displayKey,x+4,y+4,Graphics.TOP|Graphics.LEFT);
        }
    }

     void paintSelection(Graphics g) {
         drawButton(g,x,y,width,height);

     }


    /**
     * draw a button
     * 
     * @param g The graphics context to paint to
     * @param x x-coordinate of the button's location
     * @param y y-coordinate of the button's location
     * @param w the width of the button
     * @param h the height of the button
     */
    private void drawButton(Graphics g, int x, int y, int w, int h) {
        g.setColor(GRAY);
        g.drawLine(x+1,y+h-1,x+w,y+h-1);    //bottom
        g.drawLine(x+w-1,y+1,x+w-1,y+h);    //right

        g.setColor(DARK_GRAY);
        g.drawLine(x,y+h,x+w,y+h);    //bottom
        g.drawLine(x+w,y,x+w,y+h);    //right

        g.setColor(WHITE);
        g.drawLine(x,y,x+w-1,y);
        g.drawLine(x,y,x,y+h-1);

    }

    
   
    /**
     * Helper function to determine the itemIndex at the x,y position
     *
     * @param x,y   pointer coordinates in menuLayer's space (0,0 means left-top
     *      corner) both value can be negative as menuLayer handles the pointer
     *      event outside its bounds
     * @return menuItem's index since 0, or PRESS_OUT_OF_BOUNDS, PRESS_ON_TITLE
     *
     */
    private boolean isKeyAtPointerPosition(int x, int y) {
        if ((x>=this.x)&&
            (y>=this.y) && 
            (x < this.x + this.width)  &&
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
     * @param x the x coordinate of the event
     * @param y the y coordinate of the event
     */
    public boolean pointerInput(int type, int x, int y) {
        switch (type) {
        case EventConstants.PRESSED:
            if ( isKeyAtPointerPosition(x, y)) {
                return true;
            }
            break;
        case EventConstants.RELEASED:
                if ( isKeyAtPointerPosition(x, y)) {
                    return true;
            }

            break;
        }
        return false;  
    }

    int getKey() { 
        return actionKey;
    }

    int getX() {
        return x;
    }

    int getY() {
        return y;
    }

    int getWidth() {
        return width;
    }

    int getHeight() {
        return height;
    }

    void setStatus(int status) {
        this.status = status;
    }


    private final static int WHITE = 0xffffff;
    private final static int BLACK = 0x000000;
    // blue scheme
    private final static int DARK_GRAY = 0x666699; // dark blue
    private final static int GRAY = 0x3366cc;//0x8DB0D9;

}

