/*
 * @(#)QtLabelPeer.java	1.14 06/10/10
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

import java.awt.*;
import sun.awt.peer.*;

/**
 * The peer for label.
 *
 */

public class QtLabelPeer extends QtComponentPeer implements LabelPeer
{
    /** Creates a new QtLabelPeer. */

    public QtLabelPeer (QtToolkit toolkit, Label target)
    {
	super( toolkit, target );
        // Fix for bug 4756049. Once we're sure a peer has
        // been created, set alignment based on the target's
        // value.
        this.setAlignment( target.getAlignment() );
    }
	
    protected native void create( QtComponentPeer parentPeer );

    public Dimension getPreferredSize ()
    {
	Font font = target.getFont();
	FontMetrics fm = toolkit.getFontMetrics( font );
	String label = ((Label)target).getText();
	if ( label == null ) {
            label = "";
        }
	return new Dimension (fm.stringWidth(label) + 14,
			      fm.getHeight() + 8);
    }
	
    public Dimension getMinimumSize ()
    {
        return getPreferredSize();
    }

    public void setText (String text) 
    {
	setTextNative(text);
	target.repaint();
    }

    public void setAlignment (int alignment) 
    {
	setAlignmentNative(alignment);
	target.repaint();
    }

    private native void setTextNative (String text);
    private native void setAlignmentNative (int alignment);
}

