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

package com.sun.midp.chameleon.layers;

import com.sun.midp.chameleon.skins.SoftButtonSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;

import com.sun.midp.chameleon.*;
import javax.microedition.lcdui.*;
import com.sun.midp.chameleon.skins.ScrollIndSkin;
import com.sun.midp.util.ResourceHandler;

/**
 * A ScrollIndLayer is a region of the display used for showing scroll indicator
 * status arrows.  
 * IMPL NOTE: implement a scroll bar mode, so that scroll feedback can be provided
 *        via scroll indicators or a scroll bar
 */
public class ScrollIndLayer extends CLayer {

    /**
     * IMPL NOTE:  Add parameters to support a scroll bar appearance mode, 
     * where a scroll bar is drawn on the right side of the screen.
     *
     * The ScrollBar mode and ButtonBar-Indicator modes will be toggled
     * with a mode flag set in our skin class.
     */

    /**
     * True if up arrow is visible
     */
    protected boolean upViz;
    
    /**
     * True if down arrow is visible
     */
    protected boolean downViz;
    
    /**
     * True if special alert arrow indicators should be drawn instead of 
     * the regular ones
     */
    protected boolean alertMode;

    /**
     * Construct a new ScrollIndLayer, visible, but transparent :)
     */
    public ScrollIndLayer() {
        super((Image)null, -1);
        super.setVisible(true);
        this.layerID = "ScrollIndLayer";
        super.setOpaque(false);
    }

    /**
     * Called by MIDPWindow to initialize this layer
     */
    protected void initialize() {
        super.initialize();
        setAnchor();
    }
	
    public void setAnchor() {
        switch (ScrollIndSkin.MODE) {
            case ScrollIndSkin.MODE_BAR:
                // IMPL NOTE: Implement scroll bars
                break;
            case ScrollIndSkin.MODE_ARROWS:
            default:
                bounds[H] = SoftButtonSkin.HEIGHT;
                if (ScrollIndSkin.IMAGE_UP != null) {
                    bounds[W] = ScrollIndSkin.IMAGE_UP.getWidth();
                    bounds[H] = (2 * ScrollIndSkin.IMAGE_UP.getHeight());
                    bounds[Y] = SoftButtonSkin.HEIGHT - bounds[H];
                    bounds[Y] = bounds[Y] / 3;
                    bounds[H] += bounds[Y];
                    bounds[Y] = ScreenSkin.HEIGHT - SoftButtonSkin.HEIGHT +
                        bounds[Y];
                } else {
                    bounds[W] = ScrollIndSkin.WIDTH;
                    bounds[Y] = 3;
                }
                bounds[X] = (ScreenSkin.WIDTH - bounds[W]) / 2;
                break;
        }
    }

    /**
     * Set the current vertical scroll position and proportion.
     *
     * @param alt_arrows true if alert indicators should be drawn
     * @param scrollPosition vertical scroll position.
     * @param scrollProportion vertical scroll proportion.
     */
    public void setVerticalScroll(boolean alt_arrows, int scrollPosition, 
                                  int scrollProportion) 
    {	
        alertMode = alt_arrows;
        boolean up = upViz;
        boolean dn = downViz;
        if (scrollProportion < 100) {
            upViz = (scrollPosition > 0);
            downViz = (scrollPosition < 100);
        } else {
            upViz = false;
            downViz = false;
        }
        
        if (up != upViz || dn != downViz) {
            this.dirty = true;
            requestRepaint();
        }
    }

    /**
     * Paint the scroll indicator.  The indicator arrows may be appear 
     * individually
     * or together, and may vary in appearance based on whether they appear
     * in the normalsoft button region or an alert's softbutton region.
     * The visible state is based on the state of the <code>alertMode</code>, 
     * <code>upViz</code>, and <code>downViz</code> variables set by the
     * <code>setVerticalScroll</code> method.
     * @param g the graphics context to paint in
     */
    protected void paintBody(Graphics g) {
        if (upViz) {
            Image i = ScrollIndSkin.IMAGE_UP;
            if (alertMode && ScrollIndSkin.IMAGE_AU_UP != null) {
                i = ScrollIndSkin.IMAGE_AU_UP;
            }
            if (i != null) {
                g.drawImage(i, 0, 0, Graphics.LEFT| Graphics.TOP);
            }
        }
        
        if (downViz) {
            Image i = ScrollIndSkin.IMAGE_DN;
            if (alertMode && ScrollIndSkin.IMAGE_AU_DN != null) {
                i = ScrollIndSkin.IMAGE_AU_DN;
            }
            if (i != null) {
                g.drawImage(i, 0, 
                    bounds[H] - ScrollIndSkin.IMAGE_DN.getHeight(),
                    Graphics.LEFT | Graphics.TOP);
            }
        }
    }
}
