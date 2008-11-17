/*
 * @(#)QtCheckboxMenuItemPeer.java	1.9 06/10/10
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

class QtCheckboxMenuItemPeer extends QtMenuItemPeer implements CheckboxMenuItemPeer
{
	private static native void initIDs();
	
	static
	{
		initIDs();
	}
	
	/** Creates a new QtCheckboxMenuItemPeer. */

	QtCheckboxMenuItemPeer (CheckboxMenuItem item)
	{
		super (item);
		setState (item.getState());
	}
	
	protected native void create();
	
	public native void setState (boolean b);
	
	public void postItemEvent (boolean state)
	{
		CheckboxMenuItem cb = (CheckboxMenuItem)target;
		
                QtToolkit.postEvent(new ItemEvent(cb, ItemEvent.ITEM_STATE_CHANGED,
                                                  target.getLabel(),
                                                  state? ItemEvent.SELECTED : ItemEvent.DESELECTED));

                // Fix for 4803980.  See also 4024569.
                // Invoke the parent class action method so that
                // action events can be generated.
                super.postActionEvent();
        }
}

