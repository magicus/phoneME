/*
 * @(#)PPCTextAreaPeer.java	1.10 06/10/10
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

class PPCTextAreaPeer extends PPCTextComponentPeer implements TextAreaPeer {

    private static native void initIDs();
    static {
        initIDs();
    }

    /** Creates a new PPCTextAreaPeer. */
    PPCTextAreaPeer (TextArea target) {
        super(target);
    }

    public Dimension getMinimumSize() {
	return getMinimumSize(10, 60);
    }

    // TextAreaPeer implementation

    /* This should eventually be a direct native method. */
    public void insert(String txt, int pos) {
	insertText(txt, pos);
    }

    /* This should eventually be a direct native method. */
    public void replaceRange(String txt, int start, int end) {
	replaceText(txt, start, end);
    }

    public Dimension getPreferredSize(int rows, int cols) {
	return getMinimumSize(rows, cols);
    }
    public Dimension getMinimumSize(int rows, int cols) {
	FontMetrics fm = getFontMetrics(((TextArea)target).getFont());
	PPCToolkit toolkit = (PPCToolkit)Toolkit.getDefaultToolkit();

	int extraWidth = 2 * toolkit.frameWidth;
	int extraHeight= 2 * toolkit.frameHeight;
	int hsbHeight = toolkit.hsbHeight;
	int hsbMinWidth = toolkit.hsbMinWidth;
        int vsbMinHeight = toolkit.vsbMinHeight;
        int vsbWidth = toolkit.vsbWidth;

	int sbv = ((TextArea)target).getScrollbarVisibility();
	Dimension dim =
	    new Dimension(fm.charWidth('0') * cols, fm.getHeight() * rows);
	switch (sbv) {
	    case TextArea.SCROLLBARS_HORIZONTAL_ONLY:
		dim.height += hsbHeight;
		if (dim.width < hsbMinWidth) {
		    dim.width = hsbMinWidth;
		}
		break;
	    case TextArea.SCROLLBARS_VERTICAL_ONLY:
		dim.width += vsbWidth;
		if (dim.height < vsbMinHeight) {
		    dim.height = vsbMinHeight;
		}
		break;
             case TextArea.SCROLLBARS_BOTH:
		 if (dim.height < vsbMinHeight) {
		     dim.height = vsbMinHeight;
		 }
                 if (dim.width < hsbMinWidth) {
		     dim.width = hsbMinWidth;
		 }
		 dim.height += hsbHeight;
		 dim.width += vsbWidth;
		 break;

	    case TextArea.SCROLLBARS_NONE:
		break;
	}
	dim.width += extraWidth;
	dim.height += extraHeight;
	return dim;
    }

    native void create(PPCComponentPeer parent);

    // native callbacks


    // deprecated methods

    /**
     * DEPRECATED but, for now, still called by insert(String, int).
     */
    public void insertText(String txt, int pos) {
        replaceText(txt, pos, pos);
    }

    /**
     * DEPRECATED but, for now, still called by replaceRange(String, int, int).
     */
    public native void replaceText(String txt, int start, int end);

    /**
     * DEPRECATED
     */
    public Dimension minimumSize() {
	return getMinimumSize();
    }

    /**
     * DEPRECATED
     */
    public Dimension minimumSize(int rows, int cols) {
	return getMinimumSize(rows, cols);
    }

    /**
     * DEPRECATED
     */
    public Dimension preferredSize(int rows, int cols) {
	return getPreferredSize(rows, cols);
    }
}
