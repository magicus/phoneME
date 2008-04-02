/*
 *   
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

package com.sun.midp.lcdui;

import com.sun.j2me.security.AccessController;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;
import javax.microedition.lcdui.Display;


/** System non-MIDlet classes use this class to get Display. */ 
public class DisplayFactory {
    private static DisplayStaticAccess displayTunnel;

    /**
     * Sets up the reference to the DisplayStaticAccess implementation.
     * This must be called exactly once during system initialization.
     *
     * @param token security token for authorizing the caller
     * @param tunnel the DisplayStaticAccess implementation
     */
    public static void setStaticDisplayAccess(SecurityToken token,
                                              DisplayStaticAccess tunnel) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        displayTunnel = tunnel;
    }

    /**
     * Gets the <code>Display</code> object by owner name.
     *
     * @param token security token for authorizing the caller for the
     *    com.sun.midp permission.
     * @param owner name of the owner of the display, the owner can be
     *
     * @return the display object that application can use for its user
     * interface
     *
     * @throws NullPointerException if <code>owner</code> is <code>null</code>
     */
    public static Display getDisplay(SecurityToken token, String owner) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        return displayTunnel.getDisplay(owner);
    }

    /**
     * Gets the <code>Display</code> object by owner name.
     * The caller must be granted the com.sun.midp permission.
     *
     * @param owner name of the owner of the display, the owner can be
     *
     * @return the display object that application can use for its user
     * interface
     *
     * @throws NullPointerException if <code>owner</code> is <code>null</code>
     */
    public static Display getDisplay(String owner) {
        AccessController.checkPermission(Permissions.MIDP_PERMISSION_NAME);

        return displayTunnel.getDisplay(owner);
    }
} 
