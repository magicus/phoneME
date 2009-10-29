/*
 * @(#)QtFileDialogPeer.java	1.17 06/10/10
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
import java.io.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class QtFileDialogPeer extends QtDialogPeer implements FileDialogPeer
{
    private static native void initIDs();
	
    static
    {
	initIDs();
    }
	
    /** Creates a new QtFileDialogPeer. */

    QtFileDialogPeer (QtToolkit toolkit, FileDialog target)
    {
	super (toolkit, target);
		
	String dir = target.getDirectory();
	String file = target.getFile();
	
	if (dir != null)
	    setDirectory(dir);
			
	if (file != null)
	    setFile(file);
    }
	
    protected native void create(QtComponentPeer parentPeer,
                                 boolean isUndecorated);

    protected void create(QtComponentPeer parentPeer) {
          create(parentPeer, ((FileDialog)target).isUndecorated());
    } 

    private native void setFileNative (String file);
    private native void setDirectoryNative (String dir);
    public native void showNative();

    public void show() {
        // 6224133.
        // No longer needed.
        // toolkit.setRun(true);
        // 6224133.
        Rectangle bounds = target.getBounds();
        setBounds(bounds.x, bounds.y, bounds.width, bounds.height);
        showNative();
    }

    public void setFile(String file)
    {
	if(file!=null)
	    setFileNative(file);
    }

    public void setDirectory(String dir)
    {
        if(dir!=null)
	    setDirectoryNative(dir);
    }
	
    /** This feature cannot be supported as Qt does not have calling back for its filter support.
	Instead one must provide a list of file name patterns. */

    public void setFilenameFilter(FilenameFilter filter) { }
}

