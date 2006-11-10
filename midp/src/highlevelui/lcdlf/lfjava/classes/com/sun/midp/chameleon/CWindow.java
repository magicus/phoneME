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

import com.sun.midp.chameleon.skins.ScreenSkin;
import com.sun.midp.chameleon.skins.SoftButtonSkin;

import com.sun.midp.chameleon.layers.MenuLayer;
import com.sun.midp.chameleon.layers.AlertLayer;

import javax.microedition.lcdui.*;
import java.util.Vector;

/**
 * This class is a top-level "window" in Chameleon. A window is
 * a collection of other layers and serves to maintain a z-ordering
 * of those layers. The window also contains the complex repaint logic
 * to support semi-transparent windows, their dirty regions, and the
 * rectangle logic to repaint other layers of the window when necessary.
 */
public abstract class CWindow {
    
    /**
     * An array holding the bounds of this window. The indices are
     * as follows:
     * 0 = window's 'x' coordinate
     * 1 = window's 'y' coordinate
     * 2 = window's width
     * 3 = window's height
     * 
     * Note: The window's x and y coordinate can only be interpreted
     * by some outside entity. For example, if some sort of manager
     * was in charge of overseeing the placement of windows on the
     * screen, it could do so by using the x and y coordinate values
     * of this window's bounds.
     */
    protected int[]   bounds;
    
    /**
     * Flag indicating that at least one layer belonging to this
     * window is in need of repainting 
     */
    protected boolean dirty;

    /**
     * A Vector containing all the layers of this window.
     */
    protected Vector layers;

    /** 
     * If this layer has a filled color background, 
     * the 0xrrggbbaa value of that color 
     */
    protected int   bgColor;
    
    /**
     * The image to use as a background for this layer if this layer
     * is not transparent and does not use a fill color.
     */
    protected Image bgImage;
    
    /** Cache values for the clip rectangle */
    protected int cX, cY, cW, cH;
    
    /** Cache values for the graphics translation */
    protected int tranX, tranY;
    
    /** Cache value for the graphics font */
    protected Font font;
    
    /** Cache value for the graphics foreground color */
    protected int color;

    /**
     * Construct a new CWindow given the background image and color.
     * If the background image is null, the fill color will be used
     * instead.
     *
     * @param bgImage the background image to use for the window background
     * @param bgColor the background fill color to use for the window
     *        background if the background image is null
     */
    public CWindow(Image bgImage, int bgColor) {
        this.bgImage = bgImage;
        this.bgColor = bgColor;
        
        initialize();
    }

    /**
     * Add a new CLayer to the "deck" of layers associated
     * with this CWindow. This method will sequentially add
     * layers to the window, placing subsequently added layers
     * on top of previously added layers.
     *
     * @param layer the new layer to add to this window
     * @return the z-index value of the added layer, or -1 if the
     *         layer could not be added or was null
     */
    public int addLayer(CLayer layer) {
        if (layer != null) {
            if (CGraphicsQ.DEBUG) {
                System.err.println("Layer located at: " + 
                    layer.bounds[X] + ", " + layer.bounds[Y]);
            }
            synchronized (layers) {
                if (!layers.contains(layer)) {
                    layer.owner = this;
                    layers.addElement(layer);
                    requestRepaint();
                    return layers.size() - 1;
                }
            }
        }
        return -1;
    }

    /**
     * Remove a layer from this CWindow. This method will remove
     * the given layer from the "deck" of layers associated with
     * this CWindow. If successfull, this method will return true,
     * false otherwise (for example, if the layer does not belong
     * to this window).
     *
     * @param layer the layer to remove from this window
     * @return true if successful, false otherwise
     */
    public boolean removeLayer(CLayer layer) {
        if (layer == null) {
            return false;
        }
        
        synchronized (layers) {
            layer.owner = null;
            requestRepaint();
            return layers.removeElement(layer);
        }
        
        // NOTE There is a bug here.
        // IMPL NOTE: when a layer gets removed (or has its setVisible(false))
        // called, the parent window must loop through all the other
        // layers and mark them as dirty if they intersect with the
        // layer being removed (or having its visibility changed).
    }

    /**
     * Allow this window to process key input. The type of key input
     * will be press, release, repeat, etc. The key code will identify
     * which key generated the event. This method will return true if
     * the event was processed by this window or one of its layers,
     * false otherwise.
     *
     * @param type the type of key event (press, release, repeat)
     * @param keyCode the identifier of the key which generated the event
     * @return true if this window or one of its layers processed the event
     */
    public boolean keyInput(int type, int keyCode) {
        CLayer layer;
        synchronized (layers) {
            for (int i = layers.size() - 1; i >= 0; i--) {
                layer = (CLayer)layers.elementAt(i);
                if (layer.supportsInput &&
                    layer.keyInput(type, keyCode))
                {
                    return true;
                }
            }
        } // sync
        return false;
    }

    /**
     * Allow this window to process pointer input. The type of pointer input
     * will be press, release, drag, etc. The x and y coordinates will 
     * identify the point at which the pointer event occurred in the coordinate
     * system of this window. This window will translate the coordinates
     * appropriately for each layer contained in this window. This method will
     * return true if the event was processed by this window or one of its 
     * layers, false otherwise.
     *
     * @param type the type of pointer event (press, release, drag)
     * @param x the x coordinate of the location of the event
     * @param y the y coordinate of the location of the event
     * @return true if this window or one of its layers processed the event
     */
    public boolean pointerInput(int type, int x, int y) {
        CLayer layer;
        synchronized (layers) {
            for (int i = layers.size() - 1; i >= 0; i--) {
                layer = (CLayer)layers.elementAt(i);
                if (layer.visible && layer.supportsInput &&
                    layer.containsPoint(x, y))
                {
                    // If the layer is visible, supports input, and
                    // contains the point of the pointer press, we translate
                    // the point into the layer's coordinate space and
                    // pass on the input
                    if (layer.pointerInput(type, x - layer.bounds[X],
                                           y - layer.bounds[Y]))
                    {
                        return true;
                    }
                }
            }
        } // sync
        return false;
    }

    /**
     * Handle input from some type of device-dependent
     * input method. This could be input from something
     * such as T9, or a phonebook lookup, etc.
     *
     * @param str the text to handle as direct input
     * @return true if this window or one of its layers processed the event
     */
    public boolean methodInput(String str) {
        CLayer layer;
        synchronized (layers) {
            for (int i = layers.size() - 1; i >= 0; i--) {
                layer = (CLayer)layers.elementAt(i);
                if (layer.visible && layer.supportsInput &&
                    layer.methodInput(str))
                {
                    return true;
                }
            }
        } // sync
        return false;
    }

    /**
     * Request a repaint. This method MUST be overridden 
     * by subclasses to provide the implementation.
     */
    public abstract void requestRepaint();

    /**
     * Paint this window. This method should not generally be overridden by
     * subclasses. This method carefully stores the clip, translation, and
     * color before calling into subclasses. The graphics context should be
     * translated such that it is in this window's coordinate space (0,0 is
     * the top left corner of this window). 
     *
     * @param g The graphics object to use to paint this window.
     * @param refreshQ The custom queue which holds the set of refresh
     *        regions needing to be blitted to the screen
     */
    public void paint(Graphics g, CGraphicsQ refreshQ) {
        // We reset our dirty flag first. Any layers that become
        // dirty in the duration of this method will then cause it
        // to toggle back to true for the subsequent pass.
        // IMPL NOTE: when layers start to do complex animation, there will
        // likely need to be better atomic handling of the dirty state,
        // and layers becoming dirty and getting painted
        this.dirty = false;
        
        // Store the clip, translate, font, color
        cX = g.getClipX();
        cY = g.getClipY();
        cW = g.getClipWidth();
        cH = g.getClipHeight();

        tranX = g.getTranslateX();
        tranY = g.getTranslateY();

        font = g.getFont();
        color = g.getColor();
    
        // We set the basic clip to the size of this window
        g.setClip(bounds[X], bounds[Y], bounds[W], bounds[H]);
        
        // Heuristic Explanation: Any layer that needs painting also
        // requires all layers below that region to be painted. This is
        // required because layers may be transparent or even partially
        // translucent - thus they require that all layers beneath them
        // be repainted as well.
        
        // To accomplish this we
        // loop through the stack of layers from the top most to the 
        // bottom most. If a layer is "dirty" (has its dirty bit set),
        // we find all layers that intersect with the dirty region and we
        // mark that layer to be painted as well. If that layer is already
        // marked to be painted and has its own dirty region, we union
        // the existing region with the new region. If that layer does
        // not have a dirty region, we simply set a new one.
        
        // After doing this initial iteration, all layers will now be
        // marked dirty where appropriate and have their individual dirty
        // regions set. We then make another iteration from the bottom
        // most layer to the top, painting the dirty region of each layer.
        
        CLayer l, l2;
        synchronized (layers) {

            // First Pass : We do a sweep and mark of all layers requiring
            // a repaint
                      
            for (int i = layers.size() - 1; i >= 0; i--) {
                l = (CLayer)layers.elementAt(i);
                if (l.visible && l.dirty) {
                    // We find every layer in the stack that intersects
                    // (if there is one) and make sure it is marked 
                    // dirty as well.
                    for (int j = layers.size() - 1; j >= 0; j--) {
                        // We obviously skip the layer we're comparing to
                        if (j == i) {
                            continue;
                        }
                        
                        l2 = (CLayer)layers.elementAt(j);
                        
                        // We only need to share dirty regions if the 
                        // lower layer is in fact visible and intersects 
                        // at all with the dirty layer
                        if (l2.visible && l2.intersectsRegion(l.bounds)) {
                            
                            // We create a new dirty region for the background
                            // layer by translating the upper layer's region
                            // into the coordinate space of the lower layer.
                            if (l.dirtyBounds[X] != -1) {
                                
                                if (!l2.intersectsRegion(l.dirtyBounds[X] + 
                                                            l.bounds[X],
                                                         l.dirtyBounds[Y] +
                                                            l.bounds[Y],
                                                         l.dirtyBounds[W],
                                                         l.dirtyBounds[H])) 
                                {
                                    // Its possible that the two layers 
                                    // intersect, but the dirty region in
                                    // layer (l) does not intersect an area
                                    // of layer (l2). If thats the
                                    // case, just continue
                                    continue;
                                }
                                
                                l2.addDirtyRegion(l.dirtyBounds[X] + 
                                                    l.bounds[X] - l2.bounds[X],
                                                  l.dirtyBounds[Y] + 
                                                    l.bounds[Y] - l2.bounds[Y],
                                                  l.dirtyBounds[W], 
                                                  l.dirtyBounds[H]);
                            } else {
                                l2.addDirtyRegion(l.bounds[X] - l2.bounds[X], 
                                                  l.bounds[Y] - l2.bounds[Y], 
                                                  l.bounds[W], l.bounds[H]);
                            }
                        } // if visible and intersects
                    } // for
                } // if (l.visible && l.dirty)
            } // for
            
            // Second Pass : We sweep through the layers from the bottom to
            // the top and paint the background behind every dirty region.
            for (int i = 0; i < layers.size(); i++) {
                l = (CLayer)layers.elementAt(i);
                if (l.visible && l.dirty && !l.opaque) {
                    // First, clip the graphics to only contain the dirty
                    // region of the layer (if the dirty region isn't set,
                    // clip to the whole layer contents).
                    if (CGraphicsQ.DEBUG) {
                        System.err.println("Painting background behind: " 
                            + l.layerID);
                    }
                    if (l.dirtyBounds[X] == -1) {
                        if (CGraphicsQ.DEBUG) {
                            System.err.println("\tClip: " + l.bounds[X] +
                                               ", " + l.bounds[Y] +
                                               ", " + l.bounds[W] +
                                               ", " + l.bounds[H]);
                        }
                        g.clipRect(l.bounds[X], l.bounds[Y], 
                                   l.bounds[W], l.bounds[H]);
                    } else {
                        // The layer's dirty region is in its own coordinate
                        // space. We've translated the graphics above so
                        // we just set the clip.
                        if (CGraphicsQ.DEBUG) {
                            System.err.println("\tClip: " + 
                                (l.dirtyBounds[X] + l.bounds[X]) +
                                ", " + (l.dirtyBounds[Y] + l.bounds[Y]) +
                                ", " + l.dirtyBounds[W] +
                                ", " + l.dirtyBounds[H]);
                        }
                        g.clipRect(l.dirtyBounds[X] + l.bounds[X],
                                   l.dirtyBounds[Y] + l.bounds[Y],
                                   l.dirtyBounds[W], l.dirtyBounds[H]);
                    }
                    
                    // We fill each dirty region we find with the background
                    paintBackground(g);
                    
                    // We restore the clip and color
                    g.setClip(bounds[X], bounds[Y], bounds[W], bounds[H]);
                    g.setColor(color);                                      
                } // if
            } // for
                    
            // Third Pass : We sweep through the layers from the bottom to
            // the top and paint each one that is marked as dirty
            for (int i = 0; i < layers.size(); i++) {
                l = (CLayer)layers.elementAt(i);
                               
                if (l.visible && l.dirty) {
                    // Before we call into the layer to paint, we
                    // translate the graphics context into the layer's
                    // coordinate space
                    g.translate(l.bounds[X], l.bounds[Y]);

                    // First, clip the graphics to only contain the dirty
                    // region of the layer (if the dirty region isn't set,
                    // clip to the whole layer contents).
                    if (CGraphicsQ.DEBUG) {
                        System.err.println("Painting Layer: " + l.layerID);
                    }
                    if (l.dirtyBounds[X] == -1) {
                        if (CGraphicsQ.DEBUG) {
                            System.err.println("\tClip: 0, 0, " + l.bounds[W] 
                                + ", " + l.bounds[H]);
                        }
                        g.clipRect(0, 0, l.bounds[W], l.bounds[H]);
                        refreshQ.queueRefresh(l.bounds[X], l.bounds[Y], 
                                               l.bounds[W], l.bounds[H]);
                    } else {
                        // The layer's dirty region is in its own coordinate
                        // space, so we carefully offset by the layer's origin
                        // when setting the clip
                        if (CGraphicsQ.DEBUG) {
                            System.err.println("\tClip: " + l.dirtyBounds[X] +
                                               ", " + l.dirtyBounds[Y] +
                                               ", " + l.dirtyBounds[W] +
                                               ", " + l.dirtyBounds[H]);
                        }
                        g.clipRect(l.dirtyBounds[X], l.dirtyBounds[Y],
                                   l.dirtyBounds[W], l.dirtyBounds[H]);
                        refreshQ.queueRefresh(l.dirtyBounds[X] + l.bounds[X],
                                               l.dirtyBounds[Y] + l.bounds[Y],
                                               l.dirtyBounds[W],
                                               l.dirtyBounds[H]);
                    }
                    
                    l.paint(g);
                        
                    // We restore our graphics context to prepare
                    // for the next layer
                    g.translate(-g.getTranslateX(), -g.getTranslateY());
                    g.translate(tranX, tranY);
                    
                    // We reset our clip to this window's bounds again.
                    g.setClip(bounds[X], bounds[Y], bounds[W], bounds[H]);
                   
                    g.setFont(font);
                    g.setColor(color);                                      
                } // if
            } // for
            
            // We restore the original clip. The original font, color, etc.
            // have already been restored
            g.setClip(cX, cY, cW, cH);   
                 
        } // sync
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
     * @param bgColor if the image is null, use this color as a background
     *                fill color
     */
    public void setBackground(Image bgImage, int bgColor) {
        if (bgImage != null) {
            this.bgImage = bgImage;
        } else {
            this.bgImage = null;
            if (bgColor >= 0) {
                this.bgColor = bgColor;
            }
        }
    }
    
    /**
     * Returns true if any layer of this window is in need of repainting.
     *
     * @return true if any layer of this window is marked as 'dirty' 
     *         and needs repainting.
     */
    public boolean isDirty() {
        return this.dirty;
    }
    
    /**
     * Mark this window as being dirty and requiring a repaint.
     */    
    public void setDirty() {
        this.dirty = true;
    }

    /**
     * Initialize this window by allocating its bounds array and
     * layers Vector.
     */
    protected void initialize() {
        bounds = new int[4];
        bounds[X] = 0;
        bounds[Y] = 0;
        bounds[W] = ScreenSkin.WIDTH;
        bounds[H] = ScreenSkin.HEIGHT;

        layers = new Vector();
    }

    /**
     * Paint the background of this window
     *
     * @param g the graphics context to paint to
     */
    protected void paintBackground(Graphics g) {
        CGraphicsUtil.paintBackground(g, bgImage, true, 
                                      bgColor, bounds[W], bounds[H]);
    }
    
    /** Constant used to reference the '0' index of the bounds array */
    public static final int X = 0;
    
    /** Constant used to reference the '1' index of the bounds array */
    public static final int Y = 1;
    
    /** Constant used to reference the '2' index of the bounds array */
    public static final int W = 2;
    
    /** Constant used to reference the '3' index of the bounds array */
    public static final int H = 3;
    
}

