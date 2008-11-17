/*
 * @(#)GraphicsSystem.java	1.31 06/10/10
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

package sun.porting.graphicssystem;

import sun.porting.windowsystem.EventHandler;
import java.awt.AWTError;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Image;
import java.awt.Point;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;

/**
 * This is the top-level entry point for a graphics system implementation.
 * It manages creation of resources (images, fonts, cursors), access to the
 * screen, and drawing to offscreen memory regions.
 *
 * @version 1.26, 08/19/02
 */
public abstract class GraphicsSystem {
    private static GraphicsSystem graphicsSystem = null;
    /**
     * Get a <code>GraphicsSystem</code> object corresponding to the default graphics system.
     * The default graphics system is determined by the value of the system property
     * <code>sun.graphicssystem</code>.
     * @return A <code>GraphicsSystem</code> object for the default graphics system.
     */
    public static GraphicsSystem getDefaultGraphicsSystem() {
        if (graphicsSystem == null) {
            String nm = (String) AccessController.doPrivileged(
                    new GetPropertyAction("sun.graphicssystem", 
                        "sun.awt.aw.GraphicsSystem"));
            try {
                graphicsSystem = (GraphicsSystem) Class.forName(nm).newInstance();
            } catch (ClassNotFoundException e) {
                throw new AWTError("GraphicsSystem not found: " + nm);
            } catch (InstantiationException e) {
                throw new AWTError("Could not instantiate GraphicsSystem: " + nm);
            } catch (IllegalAccessException e) {
                throw new AWTError("Could not access GraphicsSystem: " + nm);
            }
        }
        return graphicsSystem;
    }

    /**
     * Register a callback handler for receiving events.
     * @param h The callback handler, an <code>EventHandler</code> object.
     * @throws IllegalStateException if there is already a handler
     * registered.
     */
    public abstract void registerEventHandler(EventHandler h)
        throws IllegalStateException;

    /**
     * Get an implementation of a typeface with the given name and
     * style.  Style values are as in <code>java.awt.Font</code>.  If an exact match
     * is not possible, returns the closest available match according to
     * a system-dependent algorithm.
     * @param name The name of the typeface
     * @param style The style (e.g. <code>PLAIN</code>, <code>BOLD</code>, <code>ITALIC</code>) of the font.
     * @return A <code>FontPeer</code> corresponding to the nearest matching font.
     * @see java.awt.Font
     */
    public abstract sun.awt.peer.FontPeer getFont(String name, int style);

    /**
     * Get a valid <code>FontMetrics</code> object for the given font.
     * @param font The java font descriptor for the font
     * @return The corresponding <code>FontMetrics</code> object.
     * @see java.awt.Font
     */
    public abstract java.awt.FontMetrics getFontMetrics(java.awt.Font font);

    /**
     * Create an <code>Image</code> object with the given <code>ImageProducer</code>.
     * The width, height and color model will all be determined later, when
     * the image data is decoded.
     * @param producer The <code>ImageProducer</code> object which will supply the image data
     * @return An <code>Image</code>, initialized from the given <code>ImageProducer</code> object.
     */
    public abstract java.awt.Image getImage(java.awt.image.ImageProducer producer);

    /**
     * Get an <code>Image</code> object for use as an offscreen
     * drawing area related to the given component.
     * The desired width and height must be supplied.
     * @param component The <code>Component</code> with which this image is associated
     * @param w the width of the image
     * @param h the height of the image
     * @return An offscreen <code>Image</code> into which graphics can be drawn.
     * @see java.awt.Image
     * @see sun.awt.Image
     * @see sun.awt.ImageRepresentation
     */
    public abstract Image makeDrawableImage(java.awt.Component c, int w, int h);

    /**
     * Get <code>Drawable</code> objects corresponding to the available screens.
     * @return An array of <code>Drawable</code> objects.
     */
    public abstract Drawable[] getScreens();

    /**
     * Get a <code>Drawable</code> object corresponding to the default screen.
     * @return A <code>Drawable</code> object for the default screen.
     */
    public Drawable getScreen() {
        Drawable scr[] = getScreens();
        return (scr == null) ? null : scr[0];
    }

    /**
     * Get a new rectangular <code>Region</code>.
     * @param x The upper left x coordinate of the region
     * @param y The upper left y coordinate of the region
     * @param w The width of the region
     * @param h The height of the region
     * @return The newly-created <code>Region</code> object.
     */
    public abstract Region makeRegion(int x, int y, int w, int h);

    /**
     * Set the visibility of the cursor.
     * @param visible true to make the cursor visible, false otherwise.
     */
    public abstract void setCursorVisibility(boolean visible);

    /**
     * Query the visibility of the cursor.
     * @return true if the cursor is currently visible.
     */
    public abstract boolean isCursorVisible();

    /**
     * Get the cursor hotspot.
     */
    public abstract Point getCursorLocation();

    /**
     * Create a new <code>CursorImage</code> object.
     */
    public abstract CursorImage makeCursorImage(Image img, int hotX, int hotY);

    /**
     * Get a <code>CursorImage</code> object that corresponds to a standard 
     * "system" cursor.
     * @param c A Cursor object which specifies a standard system cursor.
     * @return The corresponding CursorImage.
     */
    public abstract CursorImage getCursorImage(Cursor c);

    /**
     * Set the cursor image to match the supplied <code>CursorImage</code> object.
     * @param image The contents of the cursor image.
     */
    public abstract void setCursorImage(CursorImage image);

    /**
     * Get the maximum supported size for a cursor.
     * @return a <code>Dimension</code> object containing the maximum cursor size, or
     * null if there is no maximum.  While a return value of null implies that
     * there is no maximum, it does not guarantee that all sizes are
     * supported, because aspect ratio has not been taken into account.
     */
    public abstract Dimension getMaximumCursorSize();

    /**
     * Find the nearest supported cursor size.
     * @return true if the size is supported, false otherwise.
     */
    public abstract Dimension getBestCursorSize(int width, int height);

    /**
     * Returns the maximum number of colors allowed in a cursor.  "Transparent"
     * is not to be counted as a color.
     * @return the maximum number of colors supported in a cursor image.
     */
    public abstract int getMaximumCursorColors();

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
     */
    public abstract boolean prepareScrImage(java.awt.Image image, int width, int height,
        java.awt.image.ImageObserver observer);

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
     */
    public abstract int checkScrImage(java.awt.Image image, int width, int height,
        java.awt.image.ImageObserver observer);

    /**
     * Return the resolution of the screen, in pixels per inch.
     */
    public abstract int getScreenResolution();

    /**
     * Synchronizes the graphics state.
     * Some systems may do buffering of graphics events;
     * this method ensures that the display is up-to-date. It is useful
     * for animation.
     */
    public abstract void sync();

    /**
     * Emits an audio beep.
     */
    public abstract void beep();
}
