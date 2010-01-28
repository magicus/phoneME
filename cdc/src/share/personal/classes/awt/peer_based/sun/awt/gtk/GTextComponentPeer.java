/*
 * @(#)GTextComponentPeer.java	1.18 06/10/10
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
 *
 */

package sun.awt.gtk;

import java.security.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import sun.awt.peer.*;

/**
 *
 *
 * @author Nicholas Allen
 */



abstract class GTextComponentPeer extends GComponentPeer implements TextComponentPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GTextComponentPeer. */

    GTextComponentPeer (GToolkit toolkit, TextComponent target) {
        super(toolkit, target);
        setText(target.getText());
        setEditable(target.isEditable());
    }
	
    public boolean isFocusTraversable() {
        return true;
    }

    public native void setEditable(boolean editable);

    protected native byte[] getTextNative();

    public String getText() {
        byte[] text = getTextNative();
        return new String(text);
    }

    protected native void setTextNative(byte[] text);

    public void setText(String text) {
        if (text != null) 
            if ((text.length() > 0) || (getText().length() > 0))
                setTextNative(stringToNulMultiByte(text));
    }

    public Dimension getPreferredSize(int rows, int columns) {
        return getMinimumSize(rows, columns);
    }

    public Dimension getMinimumSize(int rows, int columns) {
        FontMetrics fm = getFontMetrics(target.getFont());
        /* Calculate proper size for text area
         */
        int colWidth = fm.charWidth('0');
        int rowHeight = fm.getHeight();
        /* adding 1 to both columns and row to extend visual space
         * of the minimumSize - fixes bug 4707759
         */
        return new Dimension((columns + 1) * colWidth, (rows + 1) * rowHeight);
    }

    public native int getSelectionStart();

    public native int getSelectionEnd();

    public native void select(int selStart, int selEnd);

    public native void setCaretPosition(int pos);

    public native int getCaretPosition();
	
    /*
     * Post a new TextEvent when the value of a text component changes.
     */
	
    private void postTextEvent() {
        GToolkit.postEvent(new TextEvent(target, TextEvent.TEXT_VALUE_CHANGED));
    }
}
