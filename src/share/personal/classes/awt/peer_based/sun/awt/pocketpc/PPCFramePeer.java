/*
 * @(#)PPCFramePeer.java	1.16 06/10/10
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

import java.util.Vector;
import java.awt.*;
import sun.awt.peer.*;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageRepresentation;

class PPCFramePeer extends PPCWindowPeer implements FramePeer
{

    // FramePeer implementation

    // System property to indicate whether we can set Frame state
    private static boolean setStateRestricted;

    private static native void initIDs();

    static
    {
        initIDs();
        setStateRestricted = Boolean.getBoolean( 
                                     "java.awt.Frame.setState.isRestricted" );
    }

    public Dimension minimumSize()
    {
        int height = ( ( (Frame)target ).getMenuBar() != null) ? 45 : 30;
	return ( new Dimension( 30, height ) );
    }

    public void setIconImage( Image im )
    {
        ImageRepresentation ir = ( (PPCImage)im ).getImageRep();
        ir.reconstruct( ImageObserver.ALLBITS );
        _setIconImage( ir );
        return;
    }

    // Fix for 1251439: like PPCComponentPeer.reshape, but checks SM_C[XY]MIN
    public synchronized native void reshape( int x, int y,
                                             int width, int height );

    public void setMenuBar( MenuBar mb )
    {
	PPCMenuBarPeer mbPeer = (PPCMenuBarPeer) PPCObjectPeer.getPeerForTarget(mb);
        //PPCMenuBarPeer mbPeer = null;
        setMenuBarNative( mbPeer );
        updateInsets( insets_ );
        return;
    }
    private native void setMenuBarNative( PPCMenuBarPeer mbPeer );

    // Toolkit & peer internals

    PPCFramePeer( Frame target )
    {
	super( target );
    }

    native void create( PPCComponentPeer parent );

    public void setState( int state )
    {
        if ( ! setStateRestricted ) {
            this.state = state;
        }
        return;
    }
	
    public int getState()
    {
        return this.state;
    }

    void initialize()
    {
        super.initialize();

        Frame target = (Frame)this.target;

	if (target.getTitle() != null) {
	    setTitle(target.getTitle());
	}

	setResizable(target.isResizable());

	Image icon = target.getIconImage();
	if (icon != null) {
	    setIconImage(icon);
	}

	if (!target.getCursor().equals(Cursor.getDefaultCursor())) {
	    setCursor(target.getCursor());
	}

        return;
    }

    native void _setIconImage(ImageRepresentation ir);

    // setState( int ) and getState() were added to the
    // FramePeer interface after the pjava port. Put the
    // state variable here for the time being.
    int state = 0;
}
