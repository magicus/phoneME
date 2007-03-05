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
package com.sun.midp.lcdui;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.game.GameCanvas;

import com.sun.midp.configurator.Constants;


/**
* This is the look &amp; feel implementation for GameCanvas.
*/
public class GameCanvasLFImpl {

    /**
     * The owner of this view.
     */
    GameCanvas owner;

    /**
     * currently every GameCanvas has one offscreen buffer
     * can be optimized so that we put a limit on no of offscreen buffers
     * an application can have
     */
    private Image offscreen_buffer;


    GameCanvasLFImpl(GameCanvas c) {
        owner = c;
        offscreen_buffer = Image.createImage(
            owner.getWidth(),owner.getHeight());
    }

    /**
     * return offscreen buffer
     * @return offscreen buffer
     * @param w    width of displayable area available to the application
     * @param h  - height of the displayable area available to the application
     */
    public Image getBuffer(int w, int h) {
        return offscreen_buffer;
    }

    /**
     * Handle screen size change event to update internal
     * state of the GameCanvas accordingly
     *
     * @param w new screen width
     * @param h new screen height
     */
    public void lCallSizeChanged(int w, int h) {
        // OutOfMemoryError can be thrown
        resizeImage(offscreen_buffer, w, h, true);
    }

    /**
     * Obtains the Graphics object for rendering a GameCanvas.  The returned
     * Graphics object renders to the off-screen buffer belonging to this
     * GameCanvas.
     * @return  the Graphics object that renders to current GameCanvas
     */
    public Graphics getGraphics() {
        if (offscreen_buffer != null) {
            return offscreen_buffer.getGraphics();
        }
        
        return null;
    }

    /**
     * Render the off-screen buffer content to the Graphics object
     * @param g the Graphics object to render off-screen buffer content
     */
    public void drawBuffer(Graphics g) {
        // NullPointerException will be thrown in drawImage if g == null
       if (offscreen_buffer != null) {
            g.drawImage(offscreen_buffer, 0, 0, Graphics.TOP|Graphics.LEFT);
       }
    }

    /**
     *  Flushes the off-screen buffer to the display.
     */
    public void flushGraphics() {
        DisplayAccess displayAccess = GameMap.getDisplayAccess(owner);
        if (displayAccess != null && offscreen_buffer != null) {
	        displayAccess.flush(owner, offscreen_buffer,
			      0, 0, owner.getWidth(), owner.getHeight());
        }
    }

    /**
     * Flushes the specified region of the off-screen buffer to the display.
     * @param x the left edge of the region to be flushed
     * @param y the top edge of the region to be flushed
     * @param width the width of the region to be flushed
     * @param height the height of the region to be flushed
     */
    public void flushGraphics(int x, int y, int width, int height) {
        if (width < 1 || height < 1) {
	        return;
	    }

        DisplayAccess displayAccess = GameMap.getDisplayAccess(owner);
        if (displayAccess != null && offscreen_buffer != null) {
            displayAccess.flush(owner, offscreen_buffer,
    			  x, y,	width, height);
        }
    }

    /**
     * Gets the states of the physical game keys.
      * @return An integer containing the key state information (one bit per
     * key), or 0 if the GameCanvas is not currently shown.
     */
    public int getKeyStates() {
        DisplayAccess displayAccess = GameMap.getDisplayAccess(owner);
        if (displayAccess != null) {
            return displayAccess.getKeyMask();
        }
        return 0;
    }

    /**
     * Resize image to new dimension
     *
     * @param image Image instance to resize
     * @param w new width of the image
     * @param h new height of the image
     * @param keepContent if true the original image content will
     *  be preserved, though it will be clipped according to the
     *  new image geometry
     * throws OutOfMemoryError in the case there is not enough
     *  memory for resizing
     * @throws IllegalArgumentException
     *  in the case the image is not muttable or invalid
     */
    native void resizeImage(Image image, int w, int h, boolean keepContent)
        throws OutOfMemoryError, IllegalArgumentException;

}
