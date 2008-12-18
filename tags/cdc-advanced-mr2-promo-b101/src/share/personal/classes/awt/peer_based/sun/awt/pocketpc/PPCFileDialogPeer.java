/*
 * @(#)PPCFileDialogPeer.java	1.8 06/10/10
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
import sun.awt.ScreenUpdater;
import java.io.FilenameFilter;
import sun.awt.peer.*;

public class PPCFileDialogPeer extends PPCWindowPeer implements FileDialogPeer {
    private static native void initIDs();
    static {
        initIDs();
    }

    private PPCComponentPeer parent;

    public void setDirectory(String dir) {}
    public void setFile(String file) {}
    public void setTitle(String title) {}

    public void	setFilenameFilter(FilenameFilter filter) {
        // We can set the filter, but can't do anything with it yet.
        System.err.println("setFilenameFilter not implemented\n");
    }

    // Toolkit & peer internals
    PPCFileDialogPeer(FileDialog target) {
	super(target);
    }

    void create(PPCComponentPeer parent) {
        this.parent = parent;
    }

    void initialize() {
        FileDialog target = (FileDialog)this.target;
    }

    /*
    protected void disposeImpl() {
        ScreenUpdater.updater.removeClient(this);
        PPCToolkit.targetDisposedPeer(target, this);
    }
    */

    public native void show();

    // native callbacks
    void handleSelected(String file) {
	int index = file.lastIndexOf('\\');
        String dir;

	if (index == -1) {
	    dir = ".\\";
	    ((FileDialog)target).setFile(file);
	}
        else {
	    dir = file.substring(0, index+1);
	    ((FileDialog)target).setFile(file.substring(index+1));
	}
	((FileDialog)target).setDirectory(dir);
	((FileDialog)target).setVisible(false);
    }

    void handleCancel() {
	((FileDialog)target).setFile(null);
	((FileDialog)target).setVisible(false);
    }

    // unused methods.
    public void beginValidate() {}
    public void endValidate() {}
    public void toFront() {}
    public void toBack() {}
    public void setResizable(boolean resizable) {}
    public void hide() {}
    public void enable() {}
    public void disable() {}
    public void reshape(int x, int y, int width, int height) {}
    public boolean handleEvent(Event e) { return false;}
    public void setForeground(Color c) {}
    public void setBackground(Color c) {}
    public void setFont(Font f) {}
    public void requestFocus() {}
    public void nextFocus() {}
    public void setCursor(int cursorType) {}
    void start() {}
    void invalidate(int x, int y, int width, int height) {}
}
