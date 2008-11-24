/*
 * @(#)GtkClipboard.java	1.12 06/10/10
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

import java.awt.datatransfer.*;

public class GtkClipboard extends Clipboard implements ClipboardOwner {
    static {
        initIDs();
    }
    GtkClipboard(String name) {
        super(name);
        data = createWidget();
    }
        
    private static native void initIDs();
    private int data;
    private native int createWidget();

    private native void destroyWidget(int data);
    
    private native void setNativeClipboard(int data);

    private native void getNativeClipboard(int data);
        
    /* Called from native */
    private byte[] getStringContents() {
        String str = null;
        if (contents != null && contents.isDataFlavorSupported(DataFlavor.stringFlavor)) {
            try {
                str = (String) contents.getTransferData(DataFlavor.stringFlavor);
            } catch (Exception e) {}
        }
        return str != null ? str.getBytes() : null;
    }
    
    public synchronized void setContents(Transferable t, ClipboardOwner owner) {
        super.setContents(t, owner);
        
        /* We own the clipboard, this is set so we know we have it when returning
         * the contents */
        
        if(owner == null)
            this.owner = this;
        
        /* Only strings are copied to native clipboard */
                            
        if (contents != null && contents.isDataFlavorSupported(DataFlavor.stringFlavor)) 
            setNativeClipboard(data);  
    }

    /* Called from native when the requestee returns the data to be pasted.
     * Currently only strings are supported...
     */ 
    
    private synchronized void updateContents(byte[] nativeString) {
        if (nativeString != null) {
            contents = new StringSelection(new String(nativeString));

            notifyAll();
        }
    }

    public synchronized Transferable getContents(Object requester) {
        /* Return the local contents, if there isn't any then check the native
         * clipboard and try to return this...This returns immediately,
         * updateContents will be called when the data is ready...
         */
        
        if(owner == null) {
            getNativeClipboard(data);
            try { wait(500); } catch (InterruptedException e) { }
        }

        return contents;
    }

    /** Owner ship of the GTk+ clipboard has been lost, someother applicaiton
     *  will be providing data to be pasted
     */
    
    public synchronized void lostOwnership(Clipboard c, Transferable t) {
        owner = null;       // Do not know who owns the clipboard
        contents = null;    // Do not know what is in the clipboard yet.
    }
    
    protected void finalize() throws Throwable {
        destroyWidget(data);
        data = 0;
        super.finalize();
    }
}
