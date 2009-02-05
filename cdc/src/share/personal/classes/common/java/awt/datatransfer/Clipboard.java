/*
 * @(#)Clipboard.java	1.15 06/10/10
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

package java.awt.datatransfer;

/**
 * A class which implements a mechanism to transfer data using 
 * cut/copy/paste operations.
 *
 * @version 	1.12, 02/02/00
 * @author	Amy Fowler
 */
public class Clipboard {
    String name;
    protected ClipboardOwner owner;
    protected Transferable contents;
    /**
     * Creates a clipboard object.
     */
    public Clipboard(String name) {
        this.name = name;
    }

    /**
     * Returns the name of this clipboard object.
     */
    public String getName() {
        return name;
    }

    /**
     * Sets the current contents of the clipboard to the specified
     * transferable object and registers the specified clipboard owner
     * as the owner of the new contents.  If there is an existing owner 
     * registered, that owner is notified that it no longer holds ownership
     * of the clipboard contents.
     * @param content the transferable object representing the clipboard content
     * @param owner the object which owns the clipboard content
     */
    public synchronized void setContents(Transferable contents, ClipboardOwner owner) {
        if (this.owner != null && this.owner != owner) {
            this.owner.lostOwnership(this, this.contents);
        }
        this.owner = owner;
        this.contents = contents;
    }

    /**
     * Returns a transferable object representing the current contents
     * of the clipboard.  If the clipboard currently has no contents,
     * it returns null. The parameter Object requestor is not currently used.
     * @param requestor the object requesting the clip data  (not used)
     * @return the current transferable object on the clipboard
     */
    public synchronized Transferable getContents(Object requestor) {
        return contents;
    }
}
