/*
 * @(#)PPCPanelPeer.java	1.9 06/10/10
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

class PPCPanelPeer extends PPCCanvasPeer implements PanelPeer {

    // ComponentPeer overrides

    public void print(Graphics g) {
	super.print(g);
	((Container)target).printComponents(g);
    }

    // ContainerPeer (via PanelPeer) implementation

    public Insets getInsets() {
	return insets_;
    }

    // Toolkit & peer internals

    Insets insets_;

    PPCPanelPeer(Component target) {
	super(target);
    }

    void initialize() {
        super.initialize();
    	insets_ = new Insets(0,0,0,0);

	Color c = ((Component)target).getBackground();
	if (c == null) {
            c = PPCColor.getDefaultColor(PPCColor.WINDOW_BKGND);
	    ((Component)target).setBackground(c);
	    setBackground(c);
	}
	c = ((Component)target).getForeground();
	if (c == null) {
            c = PPCColor.getDefaultColor(PPCColor.WINDOW_TEXT);
	    ((Component)target).setForeground(c);
	    setForeground(c);
	}
    }

    /**
     * DEPRECATED:  Replaced by getInsets().
     */
    public Insets insets() {
	return getInsets();
    }

}
