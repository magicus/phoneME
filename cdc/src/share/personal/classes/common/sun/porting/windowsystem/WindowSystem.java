/*
 * @(#)WindowSystem.java	1.33 06/10/10
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
 *
 */

package sun.porting.windowsystem;

import sun.porting.graphicssystem.CursorImage;
import sun.porting.graphicssystem.GraphicsSystem;
import sun.porting.toolkit.ToolkitEventHandler;
import java.awt.AWTError;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Image;
import java.awt.image.ColorModel;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;

/**
 * This is the top-level entry point for a window system implementation.
 * It manages the creation of resources (windows, images, fonts, cursors)
 * and the delivery of input events.
 *
 * @version 1.28, 08/19/02
 */
public abstract class WindowSystem {
    private static WindowSystem windowSystem = null;
    private static final int DRAWABLE_IMAGE_STATUS =
        java.awt.image.ImageObserver.WIDTH |
        java.awt.image.ImageObserver.HEIGHT |
        java.awt.image.ImageObserver.PROPERTIES |
        java.awt.image.ImageObserver.ALLBITS;
    /**
     * Obtain an object corresponding to the default window system.
     */
    public static WindowSystem getDefaultWindowSystem() {
        if (windowSystem == null) {
            String nm = (String) AccessController.doPrivileged(
                    new GetPropertyAction("sun.windowsystem", 
                        "sun.awt.aw.WindowSystem"));
            try {
                windowSystem = (WindowSystem) Class.forName(nm).newInstance();
            } catch (ClassNotFoundException e) {
                throw new AWTError("WindowSystem not found: " + nm);
            } catch (InstantiationException e) {
                throw new AWTError("Could not instantiate WindowSystem: " + nm);
            } catch (IllegalAccessException e) {
                throw new AWTError("Could not access WindowSystem: " + nm);
            }
        }
        return windowSystem;
    }

    /**
     * Make a new <code>WindowSystem</code> object
     */
    public WindowSystem() {
        this(GraphicsSystem.getDefaultGraphicsSystem());
    }

    /**
     * Make a new <code>WindowSystem</code> object running on top of the specified <code>GraphicsSystem</code>.
     */
    public WindowSystem(GraphicsSystem gfx) {
        gfxSys = gfx;
    }
    protected GraphicsSystem gfxSys;
    /**
     * Register a callback handler for receiving events.
     * @param h The callback handler, a <code>ToolkitEventHandler</code> object.
     * @throws IllegalStateException if there is already a handler
     * registered.
     */
    public abstract void registerToolkitEventHandler(ToolkitEventHandler h)
        throws IllegalStateException;

    /**
     * Change the factory object used to create windows.  (The factory
     * mechanism is used in case a subclass of <code>Window</code> is needed.) 
     * If factory is null, use the window system's default factory.
     */
    public abstract void setWindowFactory(WindowFactory factory);

    /**
     * Start the window system running.  This will create the root
     * window, etc.  (A separate start mechanism is necessary in
     * order to break the mutual dependency between WindowSystem and
     * WindowFactory.)
     */
    public abstract void start();

    /**
     * Return the root window.  Use with caution!
     */
    public abstract Window getRootWindow();

    /**
     * Create a new window as a child of the given parent window,
     * and having the specified dimensions.  Uses the current 
     * <code>WindowFactory</code>.
     * @param parent The parent window.
     * @param x the x coordinate for the window's upper left hand corner,
     * in the coordinate system of the parent window.
     * @param y the y coordinate for the window's upper left hand corner,
     * in the coordinate system of the parent window.
     * @param w the width of the window
     * @param h the height of the window
     * @return The new <code>Window</code> object.
     */
    public abstract Window makeWindow(Window parent, int x, int y, int w, int h);

    /**
     * Create a new top-level window of the given type
     * and having the specified dimensions.  Uses the current 
     * <code>WindowFactory</code>.
     * @param windowType The type of the window
     * @param x the x coordinate for the window's upper left hand corner,
     * in the global coordinate system
     * @param y the y coordinate for the window's upper left hand corner,
     * in the global coordinate system
     * @param w the width of the window
     * @param h the height of the window
     * @return The new <code>Window</code> object.
     */
    public abstract Window makeTopLevelWindow(int windowType,
        int x, int y, int w, int h);

    /**
     * Get the window that has the keyboard input focus, or null if the
     * focus is not assigned to any window.
     * @return A <code>Window</code> object corresponding to the window that will receive
     * any keyboard events which occur.
     */
    public abstract Window getFocusWindow();

    /**
     * Get the window that has grabbed input, or null if none has.
     * @return A <code>Window</code> object corresponding to the window that has grabbed
     * input events.  Note that the grabbing window will not have the keyboard
     * focus unless it has been specifically assigned to it.
     */
    public abstract Window getGrabbingWindow();

    // these can be straight pass-throughs from graphics system, or not...

    /**
     * Get a new rectangular region.
     * @param x The upper left x coordinate of the region
     * @param y The upper left y coordinate of the region
     * @param w The width of the region
     * @param h The height of the region
     * @return The newly-created <code>Region</code> object.
     */
    public sun.porting.graphicssystem.Region makeRegion(int x, int y,
        int w, int h) {
        return gfxSys.makeRegion(x, y, w, h);
    }

    /**
     * Get an implementation of a typeface with the given name and
     * style.  Style values are as in <code>java.awt.Font</code>.
     * @param name The name of the typeface
     * @param style The style (<code>PLAIN</code>, <code>BOLD</code>, <code>ITALIC</code>) of the font.
     * @return A <code>FontPeer</code> for the nearest matching font.
     * @see java.awt.Font
     */
    public sun.awt.peer.FontPeer getFont(String name, int style) {
        return gfxSys.getFont(name, style);
    }

    /**
     * Get a valid <code>FontMetrics</code> object for the given font.
     * @param font The java font descriptor for the font
     * @return The corresponding <code>FontMetrics</code> object.
     * @see java.awt.Font
     */
    public java.awt.FontMetrics getFontMetrics(java.awt.Font font) {
        return gfxSys.getFontMetrics(font);
    }

    /**
     * Get an implementation of an image that corresponds to the
     * given <code>ImageRepresentation</code> object.
     * @param image The <code>ImageRepresentation</code> object describing this image
     * @see java.awt.Image
     * @see sun.awt.ImageRepresentation
     * @return An image, initialized from the given <code>ImageRepresentation</code>.
     */
    public Image getImage(java.awt.image.ImageProducer producer) {
        // NOTE: If the system has to support multiple screens,
        // and the screens can have different underlying ColorModels,
        // we might have to carry around a "meta" ImageRepresentation
        // and program the system accordingly.  Some aspects of the
        // image implementation in sun.awt.image may have to change
        // to accomodate this.
        return gfxSys.getImage(producer);
    }

    /**
     * Get an <code>Image</code> object for use as an offscreen
     * drawing area.  The object should have the specified size
     * and color model.
     * @param w The width of the offscreen image
     * @param h The height of the offscreen image
     * @return An offscreen image into which graphics can be drawn.
     */
    public Image makeDrawableImage(java.awt.Component c, int w, int h) {
        // NOTE: If the system has to support multiple screens,
        // and the screens can have different underlying ColorModels,
        // we might have to carry around a "meta" ImageRepresentation
        // and program the system accordingly.  Some aspects of the
        // image implementation in sun.awt.image may have to change
        // to accomodate this.
        return gfxSys.makeDrawableImage(c, w, h);
    }

    /**
     * Set the visibility of the cursor.
     * @param visible Whether to make the cursor visible or hidden.
     */
    public void setCursorVisibility(boolean visible) {
        gfxSys.setCursorVisibility(visible);
    }

    /**
     * Query the visibility of the cursor.
     * @return true if the cursor is currently visible.
     */
    public boolean isCursorVisible() {
        return gfxSys.isCursorVisible();
    }

    /**
     * Create a new <code>CursorImage</code> object.
     */
    public CursorImage makeCursorImage(java.awt.Image img, int hotX, int hotY) {
        return gfxSys.makeCursorImage(img, hotX, hotY);
    }

    /**
     * Get a <code>CursorImage</code> object that corresponds to a standard 
     * "system" cursor.
     * @param c A Cursor object which specifies a standard system cursor.
     * @return The corresponding CursorImage.
     */
    public CursorImage getCursorImage(Cursor c) {
        return gfxSys.getCursorImage(c);
    }

    /**
     * Set the cursor image to match the supplied <code>CursorImage</code>.
     * @param image The cursor image
     */
    public void setCursorImage(CursorImage image) {
        gfxSys.setCursorImage(image);
    }

    /**
     * Get the maximum supported size for a cursor.
     * @return a Dimension object containing the maximum cursor size, or
     * null if there is no maximum.  A return value of null implies that
     * there is no maximum; it does not guarantee that all sizes are
     * supported because aspect ratio has not been taken into account.
     */
    public Dimension getMaximumCursorSize() {
        return gfxSys.getMaximumCursorSize();
    }

    /**
     * Find the nearest supported cursor size.
     * @return true if the size is supported, false otherwise.
     */
    public Dimension getBestCursorSize(int width, int height) {
        return gfxSys.getBestCursorSize(width, height);
    }

    /**
     * Returns the maximum number of colors allowed in a cursor.  "Transparent"
     * is not to be counted as a color.
     * @return the maximum number of colors supported in a cursor image.
     */
    public int getMaximumCursorColors() {
        return gfxSys.getMaximumCursorColors();
    }

    /**
     * Get the color model of the screen.
     * @return The color model, as a java.awt.image.ColorModel object
     */
    public java.awt.image.ColorModel getScreenColorModel() {
        return gfxSys.getScreen().getColorModel();
    }

    /**
     * Prepares an image for rendering. 
     * <p>
     * If the values of the width and height arguments are both 
     * <code>-1</code>, this method prepares the image for rendering 
     * on the default screen; otherwise, this method prepares an image 
     * for rendering on the default screen at the specified width and height. 
     * <p>
     * The image data is downloaded asynchronously in another thread, 
     * and an appropriately scaled screen representation of the image is 
     * generated. 
     * <p>
     * This method is called by components <code>prepareImage</code> 
     * methods. 
     * <p>
     * Information on the flags returned by this method can be found 
     * with the definition of the <code>ImageObserver</code> interface. 

     * @param     image      the image for which to prepare a  
     *                           screen representation.
     * @param     width      the width of the desired screen 
     *                           representation, or <code>-1</code>.
     * @param     height     the height of the desired screen 
     *                           representation, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code> 
     *                           object to be notified as the 
     *                           image is being prepared.
     * @return    <code>true</code> if the image has already been 
     *                 fully prepared; <code>false</code> otherwise.
     * @see       java.awt.Component#prepareImage(java.awt.Image, 
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#prepareImage(java.awt.Image, 
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public boolean prepareScrImage(java.awt.Image image, int width, int height,
        java.awt.image.ImageObserver observer) {
        if (image instanceof sun.porting.graphicssystem.Drawable) {
            return true;
        } else {
            return gfxSys.prepareScrImage(image, width, height, observer);
        } 
    }

    /**
     * Indicates the construction status of a specified image that is
     * being prepared for display.
     * <p>
     * If the values of the width and height arguments are both 
     * <code>-1</code>, this method returns the construction status of 
     * a screen representation of the specified image in this toolkit. 
     * Otherwise, this method returns the construction status of a
     * scaled representation of the image at the specified width 
     * and height.
     * <p>
     * This method does not cause the image to begin loading. 
     * An application must call <code>prepareImage</code> to force 
     * the loading of an image.
     * <p>
     * This method is called by the component's <code>checkImage</code>
     * methods.
     * <p>
     * Information on the flags returned by this method can be found
     * with the definition of the <code>ImageObserver</code> interface.
     * @param     image   the image whose status is being checked.
     * @param     width   the width of the scaled version whose status is
     *                 being checked, or <code>-1</code>.
     * @param     height  the height of the scaled version whose status
     *                 is being checked, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code> object to be
     *                 notified as the image is being prepared.
     * @return    the bitwise inclusive <strong>OR</strong> of the
     *                 <code>ImageObserver</code> flags for the 
     *                 image data that is currently available.
     * @see       java.awt.Toolkit#prepareImage(java.awt.Image, 
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image, 
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image, 
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public int checkScrImage(java.awt.Image image, int width, int height,
        java.awt.image.ImageObserver observer) {
        if (image instanceof sun.porting.graphicssystem.Drawable) {
            return DRAWABLE_IMAGE_STATUS;
        } else {
            return gfxSys.checkScrImage(image, width, height, observer);
        }
    }

    /**
     * Return the resolution of the screen, in pixels per inch.
     */
    public int getScreenResolution() {
        return gfxSys.getScreenResolution();
    }

    /**
     * Synchronizes the graphics state. Some systems may do buffering of graphics events;
     * this method ensures that the display is up-to-date. It is useful
     * for animation.
     */
    public void sync() {
        gfxSys.sync();
    }

    /**
     * Emits an audio beep.
     */
    public void beep() {
        gfxSys.beep();
    }
}
