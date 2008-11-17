/*
 * @(#)QtMenuPeer.java	1.8 06/10/10
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

/**
 *
 *
 * @author Nicholas Allen
 */

class QtMenuPeer extends QtMenuItemPeer implements MenuPeer, QtMenuContainer
{
	/** Creates a new QtMenuPeer. */

	QtMenuPeer (Menu target)
	{
		super (target);
	}
	
	protected native void create();
	
	public native void add (QtMenuItemPeer peer);
	
	/** This method does not need to do anything because the menuitem peer will add itself to a
	 QtMenuContainer when it is created. */
	
	public void addItem(MenuItem item) { }
	
	/** This method doesn't need to do anything because the menu item's peer will have been disposed
	    of by this time and disposing of a peer which is in a container in Qt automatically removes it
	 from the parent container. */
	
    public void delItem (int index) { }
}

