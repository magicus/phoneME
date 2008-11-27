/*
 * @(#)QtMenuBarPeer.java	1.11 06/10/10
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

class QtMenuBarPeer implements MenuBarPeer, QtMenuContainer
{
	private static native void initIDs();
	
	static
	{
		initIDs();
	}
	
	/** Creates a new QtMenuBarPeer. */

	public QtMenuBarPeer (MenuBar target)
	{
		this.target = target;
		create();
        Font f = target.getFont();
        if ( f != null ) {
            setFont(f);
        }
	}
	
	public QtMenuBarPeer getMenuBar () {return this;}
	
	protected native void create ();
	public native void dispose ();

    // 4678754
    public void add(QtMenuItemPeer item) {
        // This method is called as a side-effect of creating the 
        // peer (See QtMenuItemPeer). The code that computes if the item 
        // passed is a helpmenu will only be correct if this was called 
        // in the context of QtMenuBarPeer creation.
        //
        // When this is called after this peer creation, we cannot 
        // determine correctly if the "item" is a help menu, since the
        // marking of the menu is done after this is called. The good
        // news is that we will get notified in "addHelpMenu()" if the 
        // item was really a helpmenu. 
        //
        // The bottom line is, if isHelpMenu is true it is always correct,
        // and we let the native layer know that information. If it is
        // false, then AWT code would call "addHelpMenu()", if the item
        // passed was supposed to be a helpmenu item. In a nutshell we
        // always do the following actions in either case
        // - addNative(menuPeer) ;
        // - if ( menuPeer is an helpMenu )
        //     setAsHelpMenu(menuPeer)
        boolean isHelpMenu = 
            (((MenuBar)this.target).getHelpMenu() == item.target) ;

        addNative(item) ;
        if ( isHelpMenu ) {
            // set the menupeer as the help menu
            setAsHelpMenu((QtMenuPeer)item);
        }
    }

	native void addNative(QtMenuItemPeer item);

    native void setAsHelpMenu(QtMenuPeer menuPeer);
    // 4678754
	
	/** This method does not need to do anything because the menuitem peer will add itself to a
	 QtMenuContainer when it is created. */
	
	public void addMenu(Menu m) { }

    public void delMenu(int index) { }

    // 4678754
    public void addHelpMenu(Menu m) {
        // note : this method is called after addNative() has been called
        // on "m" (See MenuBar.setHelpMenu() for details)
        setAsHelpMenu((QtMenuPeer)QtToolkit.getMenuComponentPeer(m));
    }
    // 4678754

    public native void setFont(Font f);
	
	private MenuBar target;
	private int data;
}

