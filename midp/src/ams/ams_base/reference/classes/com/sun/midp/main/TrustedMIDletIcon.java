/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.main;

import javax.microedition.lcdui.Image;

import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.security.Permissions;

/**
 * This holds the trusted MIDlet icon.
 */
public class TrustedMIDletIcon {
    /** The trusted icon. */
    private static Image trustedIcon;

    /**
     * Initialize trusted icon class.
     *
     * @param icon the trusted icon
     */
    static void initClass(Image icon) {
        trustedIcon = icon;
    }
    
    /**
     * Get the Image of the trusted icon for this Display.
     * Only callers with the internal AMS permission can use this method.
     *
     * @return an Image of the trusted icon.
     *
     * @exception SecurityException if the suite calling does not have the
     * the AMS permission
     */
    public static Image getIcon() {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();

        MIDletSuite suite = midletStateHandler.getMIDletSuite();

        suite.checkIfPermissionAllowed(Permissions.AMS);

        return trustedIcon;
    }
}






