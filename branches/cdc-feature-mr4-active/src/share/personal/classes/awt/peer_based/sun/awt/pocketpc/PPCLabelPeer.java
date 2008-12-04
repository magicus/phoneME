/*
 * @(#)PPCLabelPeer.java	1.8 06/10/10
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

class PPCLabelPeer extends PPCComponentPeer implements LabelPeer
{

    private static native void initIDs();

    static
    {
        initIDs();
    }

    // ComponentPeer overrides
    
    public Dimension getMinimumSize()
    {
	FontMetrics fm = getFontMetrics( ( (Label)target ).getFont( ) );
	String label = ( (Label)target ).getText();
	if ( label == null ) {
            label = "";
        }
	return new Dimension( fm.stringWidth( label ) + 14,
                              fm.getHeight() + 8 );
    }

    // LabelPeer implementation

    public native void setText( String label );
    public native void setAlignment( int alignment );

    // Toolkit & peer internals
    
    PPCLabelPeer( Label target )
    {
	super( target );
    }

    native void create( PPCComponentPeer parent );

    void initialize()
    {
	Label l = (Label)target;

        String  txt = l.getText();
	if ( txt != null ) {
	    setText(txt);
	}

        int align = l.getAlignment();
	if ( align != Label.LEFT ) {
	    setAlignment(align);
	}

	Color bg = ( (Component)target ).getBackground();
	if ( bg != null ) {
	    setBackground( bg );
	}

	super.initialize();
        return;
    }

    void clearRectBeforePaint( Graphics g, Rectangle r )
    {
        // Overload to do nothing for native components
        return;
    }

    /**
     * DEPRECATED
     */
    public Dimension minimumSize()
    {
        return getMinimumSize();
    }

}
