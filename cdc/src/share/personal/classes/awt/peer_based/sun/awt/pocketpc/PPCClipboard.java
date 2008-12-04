/*
 * @(#)PPCClipboard.java	1.8 06/10/10
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

import java.awt.datatransfer.*;

import java.awt.AWTException;
import java.awt.datatransfer.*;

/**
 * A class which interfaces with the Windows clipboard in order
 * to support data transfer via clipboard operations.
 */
public class PPCClipboard extends Clipboard {

    static {
	initIDs();
        init();
    }

    private static native void initIDs();

    public PPCClipboard() {
        super("System");
    }

    public synchronized void setContents(Transferable contents, 
                                         ClipboardOwner owner) {
	super.setContents(contents, owner);

        if (contents instanceof StringSelection) {
            setClipboardText((StringSelection)contents);
        }
    }

    public synchronized Transferable getContents(Object requestor) {
        String s = getClipboardText();
        return (s == null) ? null : new StringSelection(s);
    }

    public synchronized void lostSelectionOwnership() {
        if (this.owner != null) {
	    this.owner.lostOwnership(this, this.contents);
	    this.owner = null;
	}
	
        // fix 4177171 : always clear cached contents when losing ownership
	this.contents = null;
    }

    // Register Java String clipboard format.
    static private native void init();

    // Replace the current clipboard contents with ss.
    private native void setClipboardText(StringSelection ss);

    // Get the current clipboard contents.
    private native String getClipboardText();
}
