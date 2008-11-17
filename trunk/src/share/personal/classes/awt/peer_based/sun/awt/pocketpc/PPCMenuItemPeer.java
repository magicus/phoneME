/*
 * @(#)PPCMenuItemPeer.java	1.9 06/10/10
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

import java.util.ResourceBundle;
import java.util.MissingResourceException;
import java.awt.*;
import sun.awt.peer.*;
import java.awt.event.ActionEvent;

/**
 *
 *
 * @author Nicholas Allen
 */

class PPCMenuItemPeer extends PPCObjectPeer implements MenuItemPeer {

    private static native void initIDs();
    static {
        initIDs();
    }

    private boolean disposed = false;

    String shortcutLabel;

    // MenuItemPeer implementation

    /*
     * Subclasses should override disposeImpl() instead of dispose(). Client
     * code should always invoke dispose(), never disposeImpl().
     */
    private native void disposeNative();
    protected void disposeImpl() {
//        PPCToolkit.targetDisposedPeer(target, this);
        disposeNative();
    }
    public final void dispose() {
        boolean call_disposeImpl = false;

        if (!disposed) {
            synchronized (this) {
                if (!disposed) {
                    disposed = call_disposeImpl = true;
                }
            }
        }

        if (call_disposeImpl) {
            disposeImpl();
        }
    }

    public void setEnabled(boolean b) {
	enable(b);
    }

    /**
     * DEPRECATED:  Replaced by setEnabled(boolean).
     */
    public void enable() {
        enable(true);
    }

    /**
     * DEPRECATED:  Replaced by setEnabled(boolean).
     */
    public void disable() {
        enable(false);
    }

    public void setLabel(String label) {
	//Netscape : Windows popup menus dont have labels, so trying to do a 
	//setLabel on one will cause a segFault.  Ideally, we'd just
	//override setLabel in PPCPopupMenuPeer, but due to a JIT bug we cant.
	if (this instanceof PPCPopupMenuPeer)
		return;
	
        MenuShortcut sc = ((MenuItem)target).getShortcut();
        shortcutLabel = (sc != null) ? sc.toString() : null;
        _setLabel(label);
    }
    public native void _setLabel(String label);

    // Toolkit & peer internals

    boolean	isCheckbox = false;

    protected PPCMenuItemPeer() {
    }

    PPCMenuItemPeer(MenuItem target) {
	this.target = target;
	PPCMenuPeer parent = (PPCMenuPeer) PPCToolkit.getMenuComponentPeer
	                                    ((MenuComponent)target.getParent());
	create(parent);
        MenuShortcut sc = ((MenuItem)target).getShortcut();
        if (sc != null) {
            shortcutLabel = sc.toString();
        }
    }
    native void create(PPCMenuPeer parent);

    native void enable(boolean e);
    
    protected void finalize() throws Throwable {
        // Calling dispose() here is essentially a NOP since the current
        // implementation prohibts gc before an explicit call to dispose().
        dispose();
        super.finalize();
    }

    // native callbacks

    void handleAction(int modifiers) {
        PPCToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED, 
                                  ((MenuItem)target).getActionCommand(), modifiers));

    }

    private static Font defaultMenuFont;

    static {
        try {
            ResourceBundle rb = ResourceBundle.getBundle("sun.awt.windows.awtLocalization");
            defaultMenuFont = Font.decode(rb.getString("menuFont"));
        } catch (MissingResourceException e) {
            //System.out.println(e.getMessage());
           // System.out.println("Using default MenuItem font\n");
            defaultMenuFont = new Font("SanSerif", Font.PLAIN, 11);
        }

    }

    Font getDefaultFont() {
        return defaultMenuFont;
    }
}
