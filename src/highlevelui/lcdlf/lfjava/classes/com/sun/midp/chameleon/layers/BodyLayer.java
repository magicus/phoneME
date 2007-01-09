/*
 *  
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

package com.sun.midp.chameleon.layers;

import com.sun.midp.chameleon.*;
import javax.microedition.lcdui.*;
import com.sun.midp.chameleon.skins.resources.ScrollIndResourcesConstants;
import com.sun.midp.chameleon.skins.ScrollIndSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;

/**
 * Basic layer containing the application area of the display. This layer
 * contains the current Displayable contents, such as a Form or Canvas.
 */
public class BodyLayer extends CLayer
    implements ScrollListener {

    /**
     * The scroll indicator layer to notify of scroll settings
     * in case not all content can fit on the menu.
     */
    protected ScrollIndLayer scrollInd;
    
    
    ChamDisplayTunnel tunnel;

    /**
     * Create a new BodyLayer.
     *
     * @param tunnel BodyLayer needs a "tunnel" class to cross the package
     *        protection boundary and access methods inside the 
     *        javax.microedition.lcdui package
     */
    public BodyLayer(ChamDisplayTunnel tunnel) {
        this((Image)null, -1, tunnel);
    }

    /**
     * Create a new BodyLayer with the given background image or color.
     * If the image is null, the color will be used.
     *
     * @param bgImage a background image array to use to render the
     *        background of this layer
     * @param bgColor a solid background fill color to use if the image
     *        background is null
     * @param tunnel BodyLayer needs a "tunnel" class to cross the package
     *        protection boundary and access methods inside the 
     *        javax.microedition.lcdui package
     */
    public BodyLayer(Image bgImage[], int bgColor, ChamDisplayTunnel tunnel)
    {
        super(bgImage, bgColor);
        this.tunnel = tunnel;
        this.visible = false;

        setScrollInd(ScrollIndLayer.getInstance(ScrollIndSkin.MODE));
    }
    
    /**
     * Create a new BodyLayer with the given background image or color.
     * If the image is null, the color will be used.
     *
     * @param bgImage a single background image to use to render the
     *        background of this layer
     * @param bgColor a solid background fill color to use if the image
     *        background is null
     * @param tunnel BodyLayer needs a "tunnel" class to cross the package
     *        protection boundary and access methods inside the 
     *        javax.microedition.lcdui package
     */
    public BodyLayer(Image bgImage, int bgColor, ChamDisplayTunnel tunnel) 
    {
        super(bgImage, bgColor);
        this.tunnel = tunnel;
        this.visible = false;
        setScrollInd(ScrollIndLayer.getInstance(ScrollIndSkin.MODE));
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
        boolean oldVis = this.visible;
        super.setVisible(visible);
        if (oldVis != visible) {
            if (scrollInd != null && !visible) {
                scrollInd.setVisible(visible);
            } else {
                updateScrollIndicator();
            }
        }
    }

    /**
     * Add this layer's entire area to be marked for repaint. Any pending
     * dirty regions will be cleared and the entire layer will be painted
     * on the next repaint.
     * TODO: need to be removed as soon as removeLayer algorithm
     * takes into account layers interaction
     */
    public void addDirtyRegion() {
        super.addDirtyRegion();
        if (scrollInd != null) {
            scrollInd.addDirtyRegion();
        }
    }
    
    /**
     * Mark this layer as being dirty. By default, this would also mark the
     * containing window (if there is one) as being dirty as well. However,
     * this parent class behavior is overridden in BodyLayer so as to not 
     * mark the containing window and therefor not require a full
     * Chameleon repaint when only the application area needs updating.
     */    
    public void setDirty() {
        this.dirty = true;
    }
    
    /**
     * Scrolling the contents according to the scrolling parameters.
     * @param scrollType  can be SCROLL_LINEUP, SCROLL_LINEDOWN, SCROLL_PAGEUP,
     *                SCROLL_PAGEDOWN or SCROLL_THUMBTRACK
     * @thumbPosition only valid when scrollType is SCROLL_THUMBTRACK
     * 
     */
    public void scrollContent(int scrollType, int thumbPosition) {
        tunnel.callScrollContent(scrollType, thumbPosition);
    }

    public void setScrollInd(ScrollIndLayer newScrollInd) {
        if (scrollInd != newScrollInd ||
            scrollInd != null && scrollInd.scrollable != this ||
            scrollInd != null && scrollInd.listener != this) {
            if (scrollInd != null) {
                scrollInd.setScrollable(null);
                scrollInd.setListener(null);
            }
            if (owner != null) {
                owner.removeLayer(scrollInd);
            }
            
            scrollInd = newScrollInd;
            if (scrollInd != null) {
                scrollInd.setScrollable(this);
                scrollInd.setListener(this);
            }
        }
        updateScrollIndicator();        
    }

    /**
     * Updates the scroll indicator.
     */
    public void updateScrollIndicator() {
        tunnel.updateScrollIndicator();
    }

    /**
     * Set the current vertical scroll position and proportion.
     *
     * @param scrollPosition vertical scroll position.
     * @param scrollProportion vertical scroll proportion.
     * @return true if set vertical scroll occues
     */
    public boolean setVerticalScroll(int scrollPosition, int scrollProportion) {
        if (scrollInd != null && owner != null)  {
            scrollInd.setVerticalScroll(scrollPosition, scrollProportion);
            if (scrollInd.isVisible()) {
                owner.addLayer(scrollInd);
            } else {
                owner.removeLayer(scrollInd);
            }
            owner.resize();
            return true;
        }
        return false;
    }


    /**
     * Paint the contents of this layer. This method is overridden from
     * the parent class to use the package tunnel to call back into the
     * javax.microedition.lcdui package and cause the current Displayable
     * to paint its contents into the body of this layer.
     *
     * @param g the Graphics to paint to
     */
    protected void paintBody(Graphics g) {
        if (tunnel != null) {
            tunnel.callPaint(g);
        }
    }

    /**
     * Update bounds of layer
     *
     * @param layers - current layer can be dependant on this parameter
     */
    public void update(CLayer[] layers) {
        super.update(layers);

        bounds[W] = ScreenSkin.WIDTH;
        bounds[H] = ScreenSkin.HEIGHT;

        if (layers[MIDPWindow.PTI_LAYER] != null && layers[MIDPWindow.PTI_LAYER].isVisible()) {
            bounds[H] -= layers[MIDPWindow.PTI_LAYER].bounds[H];
        }
        if (layers[MIDPWindow.TITLE_LAYER].isVisible()) {
            bounds[Y] = layers[MIDPWindow.TITLE_LAYER].bounds[H];
            bounds[H] -= layers[MIDPWindow.TITLE_LAYER].bounds[H];
        } else {
            bounds[Y] = 0;
        }
        if (layers[MIDPWindow.TICKER_LAYER].isVisible()) {
            bounds[H] -= layers[MIDPWindow.TICKER_LAYER].bounds[H];
        }
        if (layers[MIDPWindow.BTN_LAYER].isVisible()) {
            bounds[H] -= layers[MIDPWindow.BTN_LAYER].bounds[H];
        }

        if (scrollInd != null) {
            scrollInd.update(layers);
            if (scrollInd.isVisible()) {
                bounds[W] -= scrollInd.bounds[W];
            }
        }
    }
}

