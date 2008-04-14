/*
 * $LastChangedDate: 2005-11-21 02:11:20 +0900 (월, 21 11 2005) $  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package javax.microedition.lcdui;

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.layers.PopupLayer;


/**
 * This is a popup layer that handles a sub-popup within the text tfContext
 */

class Key {

    final static int RELEASED = 0;
    final static int PRESSED = 1;

    Key(Image regular, Image pressed, int actionkey, int displaykey, int x, int y, int w, int h) {
        init( regular, pressed,actionkey, displaykey, x, y, w, h);
    }

    Key(Image regular, Image pressed, int keycode, int x, int y) {
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
    private Image pressedKeyImage;

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

        if ((keyImages!=null) && (keyImages[status]!=null))
            g.drawImage(keyImages[status],x,y,Graphics.TOP|Graphics.LEFT);
        else 
            if (status==RELEASED)
                drawButton(g,x,y,width,height);
            else
                drawBeveledButton(g,x,y,width,height);

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
     * draw a border
     * 
     * @param g The graphics context to paint to
     * @param x1 x-coordinate of the button's location
     * @param y1 y-coordinate of the button's location
     * @param x2 the x-coordinate at the width of the border
     * @param y2 the y-coordinate at the height of the border
     */
    private void drawBorder(Graphics g, int x1, int y1, int x2, int y2) {

        g.setColor(GRAY);
        g.drawLine(x1+2,y1+2,x1+2,y2-3);    // left
        g.drawLine(x1+2,y1+2,x2-2,y1+2);    // top
        g.drawLine(x1+2,y2-1,x2-1,y2-1);    // bottom
        g.drawLine(x2-1,y1+2,x2-1,y2-1);    // right
        g.setColor(WHITE);
        g.drawRect(x1+1,y1+1,x2-x1-3,y2-y1-3);
    }

    /**
     * draw a sunken border
     * 
     * @param g The graphics context to paint to
     * @param x1 x-coordinate of the button's location
     * @param y1 y-coordinate of the button's location
     * @param x2 the x-coordinate at the width of the border
     * @param y2 the y-coordinate at the height of the border
     */
    private void drawSunkedBorder(Graphics g, int x1, int y1, int x2, int y2) {

        g.setColor(WHITE);
        g.fillRect(x1+2,y1+2,x2-x1-2,y2-y1-2);

        g.setColor(GRAY);
        g.drawLine(x1+2,y1+2,x1+2,y2-2);    //left
        g.drawLine(x1+2,y1+2,x2-2,y1+2);    //top
        g.setColor(DARK_GRAY);
        g.drawLine(x1+3,y1+3,x1+3,y2-3);    //left
        g.drawLine(x1+3,y1+3,x2-3,y1+3);    //top

        g.setColor(LIGHT_GRAY);
        g.drawLine(x1+3,y2-2,x2-2,y2-2);    //bottom
        g.drawLine(x2-2,y1+3,x2-2,y2-2);    //right
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
     * draw a beveled button
     * 
     * @param g The graphics context to paint to
     * @param x x-coordinate of the button's location
     * @param y y-coordinate of the button's location
     * @param w the width of the button
     * @param h the height of the button
     */
    private void drawBeveledButton(Graphics g, int x, int y, int w, int h) {
        g.setColor(GRAY);
        g.drawLine(x+1,y+h-1,x+w,y+h-1);    //bottom
        g.drawLine(x+w-1,y+1,x+w-1,y+h);    //right

        g.setColor(WHITE);
        g.drawLine(x,y+h,x+w,y+h);    //bottom
        g.drawLine(x+w,y,x+w,y+h);    //right

        g.setColor(GRAY);
        g.drawLine(x,y,x+w-1,y);
        g.drawLine(x,y,x,y+h-1);

        g.setColor(WHITE);
        g.drawLine(x+1,y+1,x+w-2,y+1);
        g.drawLine(x+1,y+1,x+1,y+h-2);

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
    private final static int LIGHT_GRAY = 0x6699ff;//0xccccff
    private final static int TEXT_COLOR = 0xCCFFFF;//0xccccff

}

