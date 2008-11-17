/*
 * @(#)GFileDialogPeer.java	1.14 06/10/10
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

import java.awt.*;
import sun.awt.peer.*;
import java.io.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GFileDialogPeer extends GDialogPeer implements FileDialogPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GFileDialogPeer. */

    GFileDialogPeer (GToolkit toolkit, FileDialog target) {
        super(toolkit, target);
        String dir = target.getDirectory();
        String file = target.getFile();  // added for Bug #4700136
        // first "if" block added for Bug #4700136
        if ((dir != null) && (file != null)) {
            if (dir.endsWith("/")) {
                setFile(dir.concat(file));
            } else {
                String temp = dir.concat("/");
                setFile(temp.concat(file));
            }
        } else if (dir != null) {
            String temp = null;   // added for Bug #4700136
            // "if" block added for Bug #4700136
            if (!dir.endsWith("/"))
                temp = dir.concat("/");
            else
                temp = dir;
            setDirectory(temp);
        } else if ((dir = target.getFile()) != null) {
            setFile(dir);
        }
    }
	
    protected native void create();

    private native void setFileNative(byte[] file);
	
    public void setFile(String file) {
        if (file != null)
            setFileNative(stringToNulMultiByte(file));
    }

    public void setDirectory(String dir) {
        if (dir != null)
            setFileNative(stringToNulMultiByte(dir));
    }
	
    /** This feature cannot be supported as Gtk does not have calling back for its filter support.
     Instead one must provide a list of file name patterns. */

    public void setFilenameFilter(FilenameFilter filter) {}
}
