/*
 * @(#)QtTextAreaPeer.java	1.14 06/10/10
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
package sun.awt.qt;

import java.awt.*;
import sun.awt.peer.*;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;

/**
 *
 *
 * @author Nicholas Allen
 */

class QtTextAreaPeer extends QtTextComponentPeer implements TextAreaPeer
{
    /** Creates a new QtTextAreaPeer. */

    QtTextAreaPeer (QtToolkit toolkit, TextArea target)
    {
	super (toolkit, target);
    }

     protected void create(QtComponentPeer parentPeer) {
         // currently, this following property is an undocumented property to determine
         // whether or not wordwrap on word boundaries should be used.  If true, it will
         // be used.  Otherwise, lines will break on any character.

         String prop = (String) AccessController.doPrivileged(
             new GetPropertyAction("sun.textarea.wordwrap")); 
         if (prop == null) {
             create(parentPeer, false);
         } else if (prop.toLowerCase().equals("true")) {
             create(parentPeer, true);
         } else {
             create(parentPeer, false);
         }
    }   
	
    protected native void create(QtComponentPeer parentPeer, boolean wordwrap);

    protected native void insertNative(String text, int pos);

    native int getExtraWidth();

    native int getExtraHeight();

    public void insert(String text, int pos)
    {
	if(text!=null)
	    insertNative(text, pos);
    }

    protected native void replaceRangeNative(String text, int start, int end);

    public void replaceRange(String text, int start, int end)
    {
        if(text!=null)
	    replaceRangeNative(text, start, end);
    }

    public Dimension getPreferredSize(int rows, int columns) {
	FontMetrics fm = getFontMetrics(target.getFont());

	/* Calculate proper size for text area
	 */
	int colWidth = fm.charWidth('0');
	int rowHeight = fm.getHeight();
        //int rowHeight = fm.getMaxAscent() + fm.getMaxDescent();
 
        // The getExtraWidth() and getExtraHeight() calls, inspired from J2SE,
        // calculate the extra space for scrollbars and other margins.
	return new Dimension(columns * colWidth + getExtraWidth(),
                             rows * rowHeight + getExtraHeight());
    }

    public Dimension getMinimumSize(int rows, int columns) {return getMinimumSize();}

    /**
     * DEPRECATED:  Replaced by insert(String, int).
     */
    public void insertText(String txt, int pos) {insert (txt, pos);}

    /**
     * DEPRECATED:  Replaced by ReplaceRange(String, int, int).
     */
    public void replaceText(String txt, int start, int end) {replaceRange(txt, start, end);}

    /**
     * DEPRECATED:  Replaced by getPreferredSize(int, int).
     */
    public Dimension preferredSize(int rows, int cols) {return getPreferredSize(rows, cols);}

    /**
     * DEPRECATED:  Replaced by getMinimumSize(int, int).
     */
    public Dimension minimumSize(int rows, int cols) {return getMinimumSize(rows, cols);}

    public boolean isFocusable() {
       return true;
    }
}

