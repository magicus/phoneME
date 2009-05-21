/*
 * @(#)QtButtonPeer.java	1.14 06/10/10
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

import sun.awt.peer.*;
import java.awt.*;
import java.awt.event.*;

/**
 * QtButtonPeer.java
 *
 * @author Nicholas Allen
 */

class QtButtonPeer extends QtComponentPeer implements ButtonPeer
{
    private static native void initIDs();
	
    static
    {
	initIDs();
    }
	
    QtButtonPeer (QtToolkit toolkit, Button target)
    {
	super (toolkit, target);
	setLabel (target.getLabel());
    }
	
    public Dimension getPreferredSize ()
    {
	Font font = target.getFont();
	FontMetrics fm = toolkit.getFontMetrics(font);
		
	return new Dimension
	    (fm.stringWidth(((Button)target).getLabel()) + 14,
	     fm.getHeight() + 8);
    }


    public boolean isFocusTraversable() {return true;}
	
    protected native void create(QtComponentPeer parentPeer);
    protected native void setLabelNative (String label);
	
    public void setLabel(String label)
    {
	if(label!=null)
	    setLabelNative(label);
        target.invalidate();
    }

    private void postActionEvent()
    {
        QtToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED,
					    ((Button)target).getActionCommand()));
    }

    public boolean isFocusable() {
        return true;
    }
}

