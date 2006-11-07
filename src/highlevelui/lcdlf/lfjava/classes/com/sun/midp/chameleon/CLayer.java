/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.chameleon;

import javax.microedition.lcdui.*;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.util.ResourceHandler;

/**
 * This class represents a "layer". A layer is an element used to comprise
 * a higher-level "window" (see CWindow.java). A layer has several properties
 * such as its visibility, whether it accepts input, its size, etc. A layer
 * also has content, such as its background and its foreground.
 */
public class CLayer {
    
    /** A String identifier used by subclasses (for debug purposes) */
    protected String layerID;
    
    /** Flag indicating this layer is in need of repainting */
    protected boolean dirty;
    
    /** Array holding a bounding rectangle of an area needing repainting */
    protected int[] dirtyBounds;
    
    /** Flag indicating if this layer has a transparent background or not */
    protected boolean transparent;
    
    /** Flag indicating the current visibility state of this layer */
    protected boolean visible;
    
    /** Flag indicating the ability of this layer to support key/pen input */
    protected boolean supportsInput;

    /** 
     * Flag indicating this layer is either completely opaque, or not.
     * By default, a layer is not opaque, and thus requires the background
     * and any layers below it to be painted in addition to itself. However,
     * if a layer is opaque, it does not require anything it is obscuring to
     * be painted, just itself. All layers are opaque by default.
     */    
    protected boolean opaque = true;
    
    /** 
     * If this layer has a filled color background, 
     * the 0xrrggbbaa value of that color 
     */
    protected int   bgColor;
    
    /**
     * The image to use as a background for this layer if this layer
     * is not transparent and does not use a fill color.
     */
    protected Image[] bgImage;
    
    /**
     * If this layer is not transparent and uses a background image,
     * a flag indicating if that image should be tiled or otherwise centered.
     */
    protected boolean tileBG;
    
    /**
     * An array holding the bounds of this layer. The indices are
     * as follows:
     * 0 = layer's 'x' coordinate
     * 1 = layer's 'y' coordinate
     * 2 = layer's width
     * 3 = layer's height
     * The 'x' and 'y' coordinates are in the coordinate space of the
     * window which contains this layer.
     */
    protected int[]   bounds;

    /**
     * The window which owns this layer. If this layer has not been added
     * to a window or it has been removed from a window, the owner will be
     * null.
     */
    protected CWindow owner;

    /** Constant used to reference the '0' index of the bounds array */
    public static final int X = 0;
    
    /** Constant used to reference the '1' index of the bounds array */
    public static final int Y = 1;
    
    /** Constant used to reference the '2' index of the bounds array */
    public static final int W = 2;
    
    /** Constant used to reference the '3' index of the bounds array */
    public static final int H = 3;
    
    /** 
     * When in the paint() routine, graphicsColor will be set to the
     * default graphics color. This is a convenience variable for
     * subclasses to easily modify and then reset the graphics color.
     */
    int graphicsColor;
    
    /**
     * When in the paint() routine, graphicsFont will be set to the
     * default graphics Font. This is a convenience variable for
     * subclasses to easily modify and then reset the graphics font.
     */
    Font graphicsFont;
    
    /**
     * Construct a default, transparent layer. As a result, the
     * 'transparent' value will be set to true.
     */
    public CLayer() {
        this((Image)null, -1);
    }
    
    /**
     * Construct a layer with the given background image if it is not null 
     * or with a background fill color.
     * The color should be in the 0xaarrggbb format. If the bgColor is 
     * invalid and bgImage is null, this layer will be transparent and 
     * equal to the no-argument constructor.
     *
     * @param bgImage The background image to use for this layer.
     * @param bgColor The color (0xaarrggbb) to use as the background fill
     */
    public CLayer(Image bgImage, int bgColor) 
    {
        if (bgImage != null) {
            this.bgImage = new Image[] { bgImage };
            this.tileBG = true;
            transparent = false;
        } else {
            transparent = (bgColor < 0);
        }
        this.bgColor = bgColor;
        initialize();
    }


    /**
     * Construct a layer with the given background images (if not null) 
     * or with a background fill color and border. The background images
     * should be a 9 element array, creating a 9-piece image background.
     * The color should be in the 0xaarrggbb format.  If the bgColor is 
     * invalid and bgImages are null, this layer will be transparent and 
     * equal to the no-argument constructor.
     *
     * @param bgImages The background image to use for this layer.
     * @param bgColor The color (0xaarrggbb) to use as the background fill
     */
    public CLayer(Image[] bgImages, int bgColor) {
        if (bgImages != null) {
            this.bgImage = new Image[bgImages.length];
            System.arraycopy(bgImages, 0, this.bgImage, 0, bgImages.length);
            this.tileBG = false;
            transparent = false;
        } else {
            transparent = (bgColor < 0);
        }
        this.bgColor = bgColor;
        initialize();
    }
    
    /**
     * Finish initialization of this CLayer. This can
     * be extended by subclasses. The dimensions of the
     * CLayer are stored in its bounds[]. The 'x' and 'y'
     * coordinates are in the coordinate space of the
     * window which contains this layer. By default, a layer is
     * located at the origin and is as large as the screen size.
     *
     * The X and Y coordinates represent the upper left position
     * of this CLayer in the containing CWindow's coordinate space.
     *
     */
    protected void initialize() {
        bounds = new int[4];
        bounds[X] = 0;
        bounds[Y] = 0;
        bounds[W] = ScreenSkin.WIDTH;
        bounds[H] = ScreenSkin.HEIGHT;
        
        dirtyBounds = new int[4];
        dirtyBounds[X] = dirtyBounds[Y] = dirtyBounds[W] = dirtyBounds[H] = -1;
        
        // IMPL_NOTE : center the background image by default
    }
    
    /**
     * Establish a background. This method will evaluate the parameters
     * and create a background which is appropriate. If the image is non-null,
     * the image will be used to create the background. If the image is null,
     * the values for the colors will be used and the background will be
     * painted in fill color instead. If the image is null, and the background
     * color is a negative value, this layer will become transparent and no
     * background will be painted.
     *
     * @param bgImage the image to use for the background tile (or null)
     * @param tileBG If true, then tile the background image as necessary
     *               to fill a larger screen. If false, treat the image
     *               as fullsize.
     * @param bgColor if the image is null, use this color as a background
     *                fill color
     */
    public void setBackground(Image bgImage, boolean tileBG, int bgColor) {
        if (bgImage != null) {
            this.bgImage = new Image[] { bgImage };
            this.tileBG = tileBG;
            transparent = false;
        } else {
            this.bgImage = null;
            transparent = (bgColor < 0);
        }
        this.bgColor = bgColor;
        setDirty();
    }
    
    /**
     * Establish a background. This method will evaluate the parameters
     * and create a background which is appropriate. If the images are 
     * non-null, the images will be used to create a 9-piece background.
     * If the images are null, the value for the color will be used and
     * the background will be painted in fill color instead. If the images
     * are null, and the background color is a negative value, this layer
     * will become transparent and no background will be painted.
     *
     * @param bgImages an array containing a 9-piece image set to be used
     *                 as the background for this layer
     * @param bgColor if the images are null, use this color as a background
     *                fill color
     */
    public void setBackground(Image[] bgImages, int bgColor) {
        if (bgImages != null) {
            this.bgImage = new Image[bgImages.length];
            System.arraycopy(bgImages, 0, this.bgImage, 0, bgImages.length);
            this.tileBG = false;
            transparent = false;
        } else {
            this.bgImage = null;
            transparent = (bgColor < 0);
        }
        this.bgColor = bgColor;
        setDirty();            
    }

    /**
     * Establish the bounds of this layer. The coordinate space for
     * the 'x' and 'y' anchor will be interpreted in that of the window
     * which contains this layer.
     *
     * @param x The 'x' coordinate of this layer's origin
     * @param y The 'y' coordinate of this layer's origin
     * @param w The width of this layer
     * @param h The height of this layer
     */
    public void setBounds(int x, int y, int w, int h) {
        if (bounds == null) {
            bounds = new int[4];
        }
        bounds[X] = x;
        bounds[Y] = y;
        bounds[W] = w;
        bounds[H] = h;
    }
    
    /**
     * Return the bounds of this layer (in the coordinate space of
     * its parent Window). Returns a 4 element array, containing the
     * x, y, width, and height representing this layer's bounding
     * rectangle
     *
     * @return this layer's bounding rectangle in the coordinate space
     *         of its parent window
     */
    public int[] getBounds() {
        return bounds;
    }

    /**
     * Determine the current visibility of this layer. Note that this
     * state only pertains to the layer's visibility status within its
     * containing window. The window itself may or may not be actually
     * visible on the physical display.
     *
     * @return true if this layer is currently visible in a window
     */
    public boolean isVisible() {
        return visible;
    }

    /**
     * Toggle the visibility state of this layer within its containing
     * window.
     *
     * @param visible If true, this layer will be painted as part of its
     *                containing window, as well as receive events if it
     *                supports input.
     */
    public void setVisible(boolean visible) {
        this.visible = visible;
        addDirtyRegion();
    }

    /**
     * Determine if this layer is opaque
     *
     * @return true if this layer does not have any transparent
     *         or translucent areas
     */
    public boolean isOpaque() {
        return opaque;
    }
    
    /**
     * Set the opacity flag for this layer. True means that this
     * layer does not have any transparent or translucent areas
     *
     * @param opaque a flag indicating the layer's opacity
     */
    public void setOpaque(boolean opaque) {
        this.opaque = opaque;
    }
    
    /**
     * Returns true if this layer is in need of repainting.
     *
     * @return true if this layer is marked as 'dirty' and needs repainting.
     */
    public boolean isDirty() {
        return this.dirty;
    }
    
    /**
     * Mark this layer as being dirty. By default, this will also mark the
     * containing window (if there is one) as being dirty as well.
     */    
    public void setDirty() {
        this.dirty = true;
        if (owner != null) {
            owner.setDirty();
        }
    }
     
    /**
     * Determine if this layer supports input, such as key and pen events.
     *
     * @return true if this layer supports handling input events
     */
    public boolean supportsInput() {
        return supportsInput;
    }

    /**
     * Toggle the ability of this layer to receive input.
     *
     * @param support If true, this layer will receive user input events
     *                (as long as the layer is also visible)
     */
    public void setSupportsInput(boolean support) {
        this.supportsInput = support;
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
        return false;
    }

    /**
     * Handle key input from a keypad. Parameters describe
     * the type of key event and the platform-specific
     * code for the key. (Codes are translated using the
     * lcdui.Canvas)
     *
     * @param type the type of key event
     * @param code the numeric code assigned to the key
     */
    public boolean keyInput(int type, int code) {
        return false;
    }

    /**
     * Handle input from some type of device-dependent
     * input method. This could be input from something
     * such as T9, or a phonebook lookup, etc.
     *
     * @param str the text to handle as direct input
     */
    public boolean methodInput(String str) {
        return false;
    }

    /**
     * Utility method to determine if the given point lies within
     * the bounds of this layer. The point should be in the coordinate
     * space of this layer's containing CWindow.
     *
     * @param x the "x" coordinate of the point
     * @param y the "y" coordinate of the point
     * @return true if the coordinate lies in the bounds of this layer
     */
    public boolean containsPoint(int x, int y) {
        if (x >= bounds[X] && x <= (bounds[X] + bounds[W])) {
            if (y >= bounds[Y] && y <= (bounds[Y] + bounds[H])) {
                return true;
            }
        }
        return false;
    }

    /**
     * Utility method to determine if the given region lies at
     * least in part within the bounds of this layer. The region
     * should be in the coordinate space of this layer's containing
     * CWindow.
     *
     * @param region A four element array containing a region defined
     *               by the X, Y, width, and height respectively
     * @return true if the given region lies in any part within the
     *              bounds of this layer
     */
    public boolean intersectsRegion(int[] region) {
        return intersectsRegion(region[X], region[Y], region[W], region[H]);
    }
    
    /**
     * Utility method to determine if the given region lies at
     * least in part within the bounds of this layer. The region
     * should be in the coordinate space of this layer's containing
     * CWindow.
     *
     * @param x the x coordinate of the region
     * @param y the y coordinate of the region
     * @param w the width of the region
     * @param h the height of the region
     * @return true if the given region lies in any part within the
     *              bounds of this layer
     */
    public boolean intersectsRegion(int x, int y, int w, int h) {
        if (CGraphicsQ.DEBUG) {
            System.err.println("comparing regions (" + layerID + "):");
            System.err.println("\t" + x + ", " + y 
                + ", " + w + ", " + h);
                System.err.println("\t" + bounds[X] + ", " + bounds[Y] 
                    + ", " + bounds[W] + ", " + bounds[H]);
                    
            if (x < bounds[X] + bounds[W] && 
                x + w > bounds[X] && 
                    y < bounds[Y] + bounds[H] && 
                        y + h > bounds[Y])
            {
                System.err.println("\t...returning true");
                return true;
            }
            System.err.println("\t...returning false");
            return false;
        }
        
        return (x < bounds[X] + bounds[W] && 
               x + w > bounds[X] && 
               y < bounds[Y] + bounds[H] && 
               y + h > bounds[Y]);
    }
    
    /**
     * Add this layer's entire area to be marked for repaint. Any pending
     * dirty regions will be cleared and the entire layer will be painted
     * on the next repaint.
     */
    public void addDirtyRegion() {
        if (CGraphicsQ.DEBUG) {
            System.err.println("Layer " + layerID + ":");
            System.err.println("\tMarking entire layer dirty");
        }
        dirtyBounds[X] = dirtyBounds[Y] = dirtyBounds[W] = dirtyBounds[H] = -1;
        setDirty();
    }
    
    /**
     * Add an area to be marked for repaint to this layer. This could
     * be needed for a variety of reasons, such as this layer being
     * obscured by another layer or window element. The new region should
     * be in the coordinate space of this layer.
     *
     * @param x the x coordinate of the region
     * @param y the y coordinate of the region
     * @param w the width of the region
     * @param h the height of the region
     */
    public void addDirtyRegion(int x, int y, int w, int h) {
        if (CGraphicsQ.DEBUG) {
            System.err.println("Layer " + layerID + ":");
            System.err.println("\tAdd Dirty: " + x + ", " 
                + y + ", " + w + ", " + h);
        }
        
        // If the layer is dirty and the bounds isn't yet set,
        // then the bounds is actually the entire layer, so just
        // return
        if (dirty && dirtyBounds[X] == -1) {
            return;
        }
        
        if (dirtyBounds[X] == -1) {
            dirtyBounds[X] = x;
            dirtyBounds[Y] = y;
            dirtyBounds[W] = w;
            dirtyBounds[H] = h;
        } else {
            int x2 = x + w;
            if (x2 < (dirtyBounds[X] + dirtyBounds[W])) {
                x2 = dirtyBounds[X] + dirtyBounds[W];
            }
            int y2 = y + h;
            if (y2 < (dirtyBounds[Y] + dirtyBounds[H])) {
                y2 = dirtyBounds[Y] + dirtyBounds[H];
            }
            if (x < dirtyBounds[X]) {
                dirtyBounds[X] = x;
            }
            if (y < dirtyBounds[Y]) {
                dirtyBounds[Y] = y;
            }
            dirtyBounds[W] = x2 - dirtyBounds[X];
            dirtyBounds[H] = y2 - dirtyBounds[Y];
        }        
        setDirty();
        
        // Lastly, we carefully restrict the dirty region
        // to be within the bounds of this layer
        if (dirtyBounds[X] < 0) {
            dirtyBounds[X] = 0;
        }
        if (dirtyBounds[Y] < 0) {
            dirtyBounds[Y] = 0;
        }
        if ((dirtyBounds[X] + dirtyBounds[W]) > bounds[W]) {
            dirtyBounds[W] = bounds[W] - dirtyBounds[X];
        }
        if ((dirtyBounds[Y] + dirtyBounds[H]) > bounds[H]) {
            dirtyBounds[H] = bounds[H] - dirtyBounds[Y];
        }
        
        if (CGraphicsQ.DEBUG) {
            System.err.println("\tCurrent Dirty: " + dirtyBounds[X] + ", " 
                + dirtyBounds[Y] + ", " + dirtyBounds[W] + ", " 
                + dirtyBounds[H]);
        }
    }
    
    /**
     * Request a repaint for the entire contents of this layer.
     */
    public void requestRepaint() {
        requestRepaint(0, 0, bounds[W], bounds[H]);
    }
    
    /**
     * Request a repaint for a specific region of this layer.
     *
     * @param x The 'x' coordinate of the upper left corner of the
     *          repaint region
     * @param y The 'y' coordinate of the upper right corner of the
     *          repaint region
     * @param w The width of the repaint region
     * @param h The height of the repaint region
     */
    public void requestRepaint(int x, int y, int w, int h) {
        addDirtyRegion(x, y, w, h);
        
        if (owner != null && visible) {
            // We request a repaint of our parent window after translating
            // the origin into the coordinate space of the window.
            owner.requestRepaint();
        }
    }
    
    /**
     * Paint this layer. This method should not generally be overridden by
     * subclasses. This method carefully stores the clip, translation, and
     * color before calling into subclasses. The graphics region will be
     * translated such that it is in this layer's coordinate space (0,0 is
     * the top left corner of this layer). The paintBackground() method will
     * be called first, then the paintBody() method. The graphics will then
     * carefully be put back into its original state before returning.
     * Subclasses should override the paintBody() method and do not generally 
     * need to do any translates or bother to undo any changes made to
     * the Graphics object.
     *
     * @param g The graphics object to use to paint this layer.
     */
    public void paint(Graphics g) {
        try {
            // We first reset our dirty flag
            this.dirty = false;
            
            graphicsColor = g.getColor();
            graphicsFont = g.getFont();
            
            // Paint the background
            if (!transparent) {
                paintBackground(g);
            }
            
            // Just in case subclasses modified these values,
            // return them to standard
            g.setColor(graphicsColor);
            g.setFont(graphicsFont);
            
            // Paint the body
            paintBody(g);
            
            // Just in case subclasses modified these values,
            // return them to standard
            g.setColor(graphicsColor);
            g.setFont(graphicsFont);
            
            // We reset our dirty bounds region
            dirtyBounds[X] = dirtyBounds[Y] = 
                dirtyBounds[W] = dirtyBounds[H] = -1;
            
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /** 
     * Paint the background of this layer. This method will automatically
     * handle a transparent layer, a layer with a background color (and/or
     * border color), and a layer with a background image (either centered
     * or tiled). Subclasses may override this method if they require a
     * more advanced background (note that the transparent value should be
     * set to false in order for the paintBackground() method to be called),
     * but subclasses must be careful to reset any modifications made to
     * the Graphics object before returning.
     *
     * @param g The Graphics object to use to paint the background.
     */
    protected void paintBackground(Graphics g) {
        if (bgImage == null) {
            // background is null, just fill using the fill color
            g.setColor(bgColor);
            g.fillRect(g.getClipX(), g.getClipY(), 
                       g.getClipWidth(), g.getClipHeight());
        } else {
            if (bgImage.length == 1) {
                CGraphicsUtil.paintBackground(g, bgImage[0], tileBG, bgColor, 
                                              bounds[W], bounds[H]);
            } else if (bgImage.length == 9) {
                CGraphicsUtil.draw9pcsBackground(g, 0, 0, 
                                             bounds[W], bounds[H],
                                             bgImage);
            
            } else if (bgImage.length == 3) {
                CGraphicsUtil.draw3pcsBackground(g, 0, 0, bounds[W], bgImage);
            }
        }
    }

    /**
     * Paint the body or content of this layer. This method should be
     * overridden by subclasses. Note that the Graphics object will
     * already be translated into this layer's coordinate space. Subclasses
     * do not need to reset any changes made to the Graphics object as the
     * CLayer paint() routine will do that automatically.
     *
     * @param g The Graphics object to use to paint the content of this layer
     */
    protected void paintBody(Graphics g) {
    }

}

