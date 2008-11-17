/*
 * @(#)PPCCanvasPeer.java	1.3 02/12/09
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
 */

package sun.awt.pocketpc;

import java.awt.*;
import sun.awt.peer.*;
//import java.awt.peer.*;
//import sun.awt.DrawingSurface;
//import sun.awt.DrawingSurfaceInfo;

class PPCCanvasPeer extends PPCComponentPeer 
                            implements CanvasPeer { //, DrawingSurface {

    PPCCanvasPeer (Component target) {
        super(target);
    }
	
    native void create(PPCComponentPeer parent);
	
    void initialize() {
        super.initialize();
	Color bg = ((Component)target).getBackground();
	if (bg != null) {
	    setBackground(bg);
	}
    }

    protected boolean canHavePixmapBackground() {
        return false;
    }
	
    public Dimension getPreferredSize() {
        return ((Component)target).getSize();
    }

    public void print(Graphics g) {
	Dimension d = ((Component)target).getSize();
	g.setColor(((Component)target).getBackground());
	g.fillRect(0, 0, d.width, d.height);
	g.setColor(((Component)target).getForeground());
	g.setFont(((Component)target).getFont());
	super.print(g);
    }

    /* TODO */
    // commenting out for a quick compilation
    //public DrawingSurfaceInfo getDrawingSurfaceInfo() {
//	return new WDrawingSurfaceInfo(this);
//    }
}
