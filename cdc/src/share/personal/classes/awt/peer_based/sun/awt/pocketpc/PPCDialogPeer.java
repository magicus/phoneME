/*
 * @(#)PPCDialogPeer.java	1.10 06/10/10
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

class PPCDialogPeer extends PPCWindowPeer implements DialogPeer
{
    static private native void initIDs();
    static {
        initIDs();
    }

    class ModalLock extends Object
    {
	synchronized void waitModal()
        {
	    try {
		wait();
	    } catch(InterruptedException ie) {
	    }
            return;
	}

	synchronized void endModal()
        {
	    notifyAll();
            return;
	}
    }

    ModalLock	modalLock = new ModalLock();
    // DialogPeer implementation

    // Toolkit & peer internals

    PPCDialogPeer( Dialog target )
    {
	super( target );
    }

    native void create( PPCComponentPeer parent );
    native void showModal();
    native void endModal();

    void initialize()
    {
        super.initialize();

        Dialog target = (Dialog)this.target;

	if ( target.getTitle() != null ) {
	    setTitle( target.getTitle() );
	}
	setResizable( target.isResizable() );
        return;
    }

    public void show()
    {
	if ( ( (Dialog)target ).isModal() ) {
	    showModal();
            // Short-term fix. Comment out to solve
            // modality problems. 
            // modalLock.waitModal();
        } else {
            super.show();
        }
        return;
    }

    public void hide()
    {
	if ( ( (Dialog)target ).isModal() ) {
            endModal();
            // Short-term fix. Comment out to solve
            // modality problems. 
	    // modalLock.endModal();
        } else {
            super.hide();
        }
        return;
    }

    /* Set the appropriate default dialog color, depending on whether
     * Win95/NT 4.0 is running or not.  Since these values are class
     * constants, it's easier (and safer) to do this in Java than C++.
     */
    private void setDefaultColor( boolean version4 )
    {
        Color c = version4 ? SystemColor.control : SystemColor.window;
        ((Dialog)target).setBackground(c);
        return;
    }

    //not originally in Pjava version
    //protected native void setTitleNative( byte[] title );

    //not originally in Pjava version
    public void setTitle( String title )
    {
        //        if (title != null)
        //            setTitleNative(stringToNulMultiByte(title));
        return;
    }

}
