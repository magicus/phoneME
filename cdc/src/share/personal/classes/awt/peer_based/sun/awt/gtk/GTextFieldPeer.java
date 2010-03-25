/*
 * @(#)GTextFieldPeer.java	1.14 06/10/10
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
import java.awt.event.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class GTextFieldPeer extends GTextComponentPeer implements TextFieldPeer {
    private static native void initIDs();
    static {
        initIDs();
    }
    /** Creates a new GTextFieldPeer. */

    GTextFieldPeer (GToolkit toolkit, TextField target) {
        super(toolkit, target);
        setEchoChar(target.getEchoChar());
    }
	
    protected native void create();
	
    public native void setEchoChar(char echoChar);

    public Dimension getPreferredSize(int columns) {
        return getPreferredSize(1, columns);
    }

    public Dimension getMinimumSize(int columns) {
        return getMinimumSize(1, columns);
    }
	
    private void postActionEvent() {
        GToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED, getText()));
    }
}
