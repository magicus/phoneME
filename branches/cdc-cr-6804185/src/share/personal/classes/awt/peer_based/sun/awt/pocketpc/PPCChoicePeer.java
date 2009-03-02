/*
 * @(#)PPCChoicePeer.java	1.8 06/10/10
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
import sun.awt.peer.*;
import java.awt.event.ItemEvent;

class PPCChoicePeer extends PPCComponentPeer implements ChoicePeer
{

    private static native void initIDs();

    static {
        initIDs();
    }

    // PPCComponentPeer overrides
  
    public Dimension getMinimumSize()
    {
	FontMetrics fm = getFontMetrics( ( (Choice)target ).getFont() );
	Choice c = (Choice)target;
	int w = 0;
	for ( int i = c.getItemCount() ; i-- > 0 ; ) {
	    w = Math.max(fm.stringWidth(c.getItem(i)), w);
	}
	int offset = PPCToolkit.getComboHeightOffset();
	return new Dimension( 28 + w, 
                              Math.max( fm.getHeight() + offset + 6,
				      21 + offset ) );
    }

    public boolean isFocusTraversable()
    {
	return true;
    }

    // ChoicePeer implementation

    public native void select( int index );

    public void add( String item, int index )
    {
      	addItem( item, index );
        return;
    }

    public native void remove( int index );

    void clearRectBeforePaint( Graphics g, Rectangle r )
    {
        // Overload to do nothing for native components
        return;
    }

    /**
     * DEPRECATED, but for now, called by add(String, int).
     */
    public native void addItem( String item, int index );

    public native void reshape( int x, int y, int width, int height );

    // Toolkit & peer internals

    PPCChoicePeer( Choice target )
    {
	super( target );
    }

    native void create( PPCComponentPeer parent );

    void initialize()
    {
	Choice opt = (Choice)target;
	int itemCount = opt.getItemCount();
	for ( int i=0; i < itemCount; i++ ) {
	    add(opt.getItem(i), i);
	}
	if ( itemCount > 0 && opt.getSelectedIndex() >= 0 ) {
	    select(opt.getSelectedIndex());
	}
	super.initialize();
        return;
    }

    // native callbacks

    void handleAction( int index )
    {
	Choice c = (Choice)target;
	c.select( index );
        PPCToolkit.postEvent( new ItemEvent( c, ItemEvent.ITEM_STATE_CHANGED,
                                c.getItem( index ), ItemEvent.SELECTED ) );
        return;
    }

    int getDropDownHeight()
    {
	Choice c = (Choice)target;
	FontMetrics fm = getFontMetrics( c.getFont() );
        int maxItems = Math.min( c.getItemCount(), 8 );
	return fm.getHeight() * maxItems;
    }

    /**
     * DEPRECATED
     */
    public Dimension minimumSize()
    {
        return getMinimumSize();
    }

}
