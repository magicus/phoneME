/*
 * @(#)PPCTextComponentPeer.java	1.9 06/10/10 
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

import java.security.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import sun.awt.peer.*;


abstract class PPCTextComponentPeer extends PPCComponentPeer implements TextComponentPeer {
    private static native void initIDs();
    static {
        initIDs();
    }

    /** Creates a new PPCTextComponentPeer. */
    PPCTextComponentPeer (TextComponent target) {
        super(target);
    }

    // PPCComponentPeer overrides
  
    public void setBackground(Color c) {
	TextComponent tc = (TextComponent)target;
	if (tc.isEditable()) {
	    c = c.brighter();
	}
	super.setBackground(c);
    }

    // TextComponentPeer implementation

    public void setEditable(boolean editable) {
	enableEditing(editable);
	setBackground(((TextComponent)target).getBackground());
    }
    public native String getText();
    public native void setText(String txt);
    public native int getSelectionStart();
    public native int getSelectionEnd();
    public native void select(int selStart, int selEnd);
    
    void initialize() {
	TextComponent tc = (TextComponent)target;
	String text = tc.getText();

	if (text != null) {
	    setText(text);
	}
	select(tc.getSelectionStart(), tc.getSelectionEnd());
	setEditable(tc.isEditable());

	super.initialize();
    }

    void clearRectBeforePaint(Graphics g, Rectangle r) {
        // Overload to do nothing for native components
    }

    native void enableEditing(boolean e);

    public boolean isFocusTraversable() {
	return true;
    }

    /*
     * Set the caret position by doing an empty selection. This
     * unfortunately resets the selection, but seems to be the
     * only way to get this to work.
     */
    public void setCaretPosition(int pos) {
        select(pos,pos);
    }

    /*
     * Get the caret position by looking up the end of the current
     * selection.
     */
    public int getCaretPosition() {
        return getSelectionStart();
    };

    /*
     * Post a new TextEvent when the value of a text component changes.
     */
    public void valueChanged() {
        postEvent(new TextEvent(target, TextEvent.TEXT_VALUE_CHANGED));
    }

}
