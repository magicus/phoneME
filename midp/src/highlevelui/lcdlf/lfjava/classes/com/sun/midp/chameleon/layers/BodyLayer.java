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

import com.sun.midp.chameleon.*;
import javax.microedition.lcdui.*;

/**
 * Basic layer containing the application area of the display. This layer
 * contains the current Displayable contents, such as a Form or Canvas.
 */
public class BodyLayer extends CLayer {

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
        this.visible = true;
        this.layerID = "BodyLayer";
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
        this.visible = true;
        this.layerID = "BodyLayer";
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
}

