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
import com.sun.midp.chameleon.skins.*;

/**
 * The InputModeLayer is a very simple overlay that displays to
 * the user the notion of the "current" input mode. The layer is
 * not interactive and is only visible when a text component is
 * currently selected.
 */
public class InputModeLayer extends PopupLayer {

    /** A variable holding the current display mode label */
    protected String mode;
    
    /** The width in pixels (using the default font) of the mode name */
    protected int stringWidth;
    
    protected int stringHeight;
    
    protected int[] anchor;   
    
    public InputModeLayer() {
        super();
        setBackground(null, 0xA3E2F8);
        this.supportsInput = false;
        anchor = new int[2];
        stringHeight = Font.getDefaultFont().getHeight();        
    }
    
    /**
     * Set the text to be displayed which represents the display name of
     * the currently selected input mode
     *
     * @param mode the display name of the currently selected input mode
     */
    public void setDisplayMode(String mode) {
        if (this.mode != mode) {
            this.mode = mode;
            stringWidth = Font.getDefaultFont().stringWidth(mode);
            updateLocation();
        }
    }
    
    
    /**
     * Get the text to be displayed which represents the display name of
     * the currently selected input mode
     *
     * @return mode the display name of the currently selected input mode
     */
    public String getDisplayMode() {
        return mode;
    }

    public void setAnchor(
        int anchorX, 
        int anchorY,
        int itemH,
        int spaceBelow) 
    {       
        anchor[X] = anchorX;
        anchor[Y] = anchorY;
        if (spaceBelow < stringHeight) {
            anchor[Y] -= stringHeight;
        } else {
            anchor[Y] += itemH;
        }
        updateLocation();
    }
    
    public void paintBody(Graphics g) {
        if (mode != null) {
            g.setFont(Font.getDefaultFont());
            g.setColor(0);
            g.drawString(mode, 3, 0, Graphics.LEFT | Graphics.TOP);
        }
        g.setColor(0x336699);
        g.drawRect(0, 0, bounds[W] - 1, bounds[H] - 1);
        g.setColor(0);
    }
    
    protected void updateLocation() {
        super.setBounds(
            anchor[X] - stringWidth - 6,
            anchor[Y], 
            stringWidth + 6, 
            stringHeight);
        requestRepaint();
    }
}

