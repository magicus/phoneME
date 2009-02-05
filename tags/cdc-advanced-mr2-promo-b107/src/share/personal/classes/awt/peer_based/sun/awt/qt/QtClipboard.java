/*
 * @(#)QtClipboard.java	1.11 06/10/10
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
/*
 * @(#)QtClipboard.java	1.2 02/08/09
 *
 */

package sun.awt.qt;

import java.awt.datatransfer.*;

// 6185915.
// QtClipboard should not implement ClipboardOwner.
// Plus cleanup of clipboard contents management code.
public class QtClipboard extends Clipboard {
    
    static {
        initIDs();
    }
    
    
    QtClipboard(String name) {
        super(name);
	if (data == 0) {
	    data = initClipboard();
	}
    }
        
    private static native void initIDs();
    
    private static int data = 0;
//  private Object pasteRequester;
    
    private native int initClipboard();
    private native void destroyClipboard(int data);
    
    private native void setNativeClipboard(String contentsString);
    private native String getNativeClipboardContents();
    
    public synchronized void setContents(Transferable t, ClipboardOwner owner) {

        super.setContents(t, owner);
                
        /* Only strings are copied to native clipboard */
                            
        if (contents != null &&
            contents.isDataFlavorSupported(DataFlavor.stringFlavor))
        {
            try {
                String contentsString = (String)
                    contents.getTransferData(DataFlavor.stringFlavor);
                setNativeClipboard(contentsString);
            } catch (Exception e) {
                
            }
        }
    }
    
    // 6185915
    // This function is no longer called from the native layer
    // because we always call getNativeClipboardContents() from
    // within getContents().
    /* Called from native when the requestee returns the data to be
       pasted. Currently only strings are supported.
     */ 
    private void updateContents(String contentsString) {
            
        // This could be the place to add functionality to notify
        // the clipboard listener(s) that the contents have been
        // updated.

        if (contentsString == null) {
            contents = null;
        } else {
            contents = new StringSelection(contentsString);
        }
    }

    // 6185915
    // The parameter Object requester is not currently used.
    public synchronized Transferable getContents(Object requester) {

        // Get the native contents of the clipboard and return them.

        String contentsString = null;
        
        try {
            contentsString = getNativeClipboardContents();
        } catch (Exception e) {
            
        }

        updateContents(contentsString);
        //contents = new StringSelection(contentsString);

        return contents;
    }
    
    // 6185915
    // This is called from the native layer.
    // Use invokeLater() to avoid deadlock.
    protected void lostOwnership() {
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                lostOwnership0();
            }
        });
    }

    // 6185915
    /** Owner ship of the Qt clipboard has been lost, some other
	application will be providing data to be pasted
     */
    protected synchronized void lostOwnership0() {
        if (this.owner != null) {
	    this.owner.lostOwnership(this, this.contents);
            this.owner = null;
	}

        this.contents = null;    // Do not know what is in the clipboard yet.
    }
    
    protected void finalize() throws Throwable {
        destroyClipboard(data);
        data = 0;

        super.finalize();
    }
    
}
