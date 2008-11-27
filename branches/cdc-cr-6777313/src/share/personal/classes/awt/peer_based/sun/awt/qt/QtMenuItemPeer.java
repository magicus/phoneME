/*
 * @(#)QtMenuItemPeer.java	1.15 06/10/10
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
 *
 *
 * @author Nicholas Allen
 */

class QtMenuItemPeer implements MenuItemPeer
{
	private static native void initIDs();
	
	static
	{
		initIDs();
	}
	
	/** Creates a new QtMenuItemPeer. */

	QtMenuItemPeer (MenuItem target)
	{
		this.target = target;
		
		create();
		
		try
		{
			QtMenuContainer container = (QtMenuContainer)QtToolkit.getMenuComponentPeer((MenuComponent)target.getParent());
		
			menuBar = container.getMenuBar();
			/* Add the item to its container. */
		
			container.add(this);
		}
		
		catch (ClassCastException e) { }
		
		setLabel (target.getLabel());
		setEnabled (target.isEnabled());
                setFont(target.getFont());
	}
	
	protected void create()
	{
		boolean isSeparator = target.getLabel().equals("-");
		create (isSeparator);		
	}
	
	protected native void create(boolean isSeparator);
	
	public native void dispose ();
	
	public void setLabel(String label)
        {
	    if(label!=null)
	    {
	        setLabelNative(label);
	    }
	}

        protected native void setLabelNative(String label);

    public native void setEnabled(boolean b);

    public native void setFont(Font f);

    public QtMenuBarPeer getMenuBar () {
       return menuBar;
    }

    // Fix for 4803980.  See also 4024569.
    // Invoke the parent class action method so that
    // action events can be generated.
    protected void postActionEvent () {
        QtToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED,
                                  target.getActionCommand()));
    }
	
    private int data;
    protected MenuItem target;
	
	/** The menu bar that this menu item is attatched to if it is attatcyed to a menu bar.
	    This is needed for the accelerator group that is used to implement shortcuts in Qt.
	 The menu bar holds the accelerator group. */
	
    private QtMenuBarPeer menuBar;
}

 
