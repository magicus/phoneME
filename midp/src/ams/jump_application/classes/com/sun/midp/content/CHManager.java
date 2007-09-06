/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.content;

import com.sun.midp.security.SecurityToken;

/**
 * Stub interface to handle ContentHandlers functions.
 * This version has no dependentcie to the CLDC AMS classes. It is needed by
 * com.sun.midp.midletsuite.MIDletStorage.
 * When MIDletStorage no longer reference this class, it can be removed.
 */
public class CHManager {
    /**
     * Creates a new instance of CHManager.
     */
    private CHManager() {
    }

    /**
     * Get the Content Handler manager if the system is configured
     * to implement content handlers.  A dummy handler is returned
     * if content handlers are not supported.
     * @param token the SecurityToken to get the CHManager
     * @return the CHInstaller instance to be used to handle
     *  management and installation of content handlers.
     * @see com.sun.midp.content.CHInstaller
     * @exception SecurityException if the token or suite is not allowed
     */
    public static CHManager getManager(SecurityToken token) {
        return new CHManager();
    }

    /**
     * Uninstall the Content handler specific information for
     * the specified suiteId.
     * @param suiteId the suite ID
     */
    public void uninstall(int suiteId) {
    }
}
