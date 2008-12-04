/*
 * @(#)PPCTextFieldPeer.java	1.8 06/10/10
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
import java.awt.event.*;

class PPCTextFieldPeer extends PPCTextComponentPeer implements TextFieldPeer {
    private static native void initIDs();
    static {
        initIDs();
    }

    /** Creates a new PPCTextFieldPeer. */
    PPCTextFieldPeer (TextField target) {
        super( target);
    }
	
     public Dimension getMinimumSize() {
	FontMetrics fm = getFontMetrics(((TextField)target).getFont());
	return new Dimension(fm.stringWidth(getText()) + 24, 
                             fm.getHeight() + 8);
    }

    // TextFieldPeer implementation


    /* This should eventually be a direct native method. */
    public void setEchoChar(char c) {
      	setEchoCharacter(c);
    }

    public Dimension getPreferredSize(int cols) {
	return getMinimumSize(cols);
    }

    public Dimension getMinimumSize(int cols) {
	FontMetrics fm = getFontMetrics(((TextField)target).getFont());
	return new Dimension(fm.charWidth('0') * cols + 24, fm.getHeight() + 8);
    }

    
    native void create(PPCComponentPeer parent);

    void initialize() {
	TextField tf = (TextField)target;
	if (tf.echoCharIsSet()) {
	    setEchoChar(tf.getEchoChar());
	}
	super.initialize();
    }

    // native callbacks

    void handleAction() {
        PPCToolkit.postEvent(
            new ActionEvent(target, ActionEvent.ACTION_PERFORMED, getText()));
    }

    // deprecated methods

    /**
     * DEPRECATED but, for now, called by setEchoChar(char).
     */
    public native void setEchoCharacter(char c);

    /**
     * DEPRECATED
     */
    public Dimension minimumSize() {
	return getMinimumSize();
    }

    /**
     * DEPRECATED
     */
    public Dimension minimumSize(int cols) {
	return getMinimumSize(cols);
    }

    /**
     * DEPRECATED
     */
    public Dimension preferredSize(int cols) {
	return getPreferredSize(cols);
    }
}
