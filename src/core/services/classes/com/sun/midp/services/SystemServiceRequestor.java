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

package com.sun.midp.services;

import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

/**
 * Used by client to obtain connection to service.
 */
public abstract class SystemServiceRequestor {
    private static SystemServiceRequestor instance = null;

    /**
     * Establishes connection to service.
     *
     * @param serviceID unique service ID
     * @return connection to service, 
     * or null if some reasons service request has failed (for example, 
     * there is no such service registered)
     */
    abstract public SystemServiceConnection requestService(String serviceID);

    /**
     * Gets new class instance.
     *
     * @return SystemServiceRequestor class instance
     */
    synchronized public static SystemServiceRequestor getInstance(
            SecurityToken token) {

        if (instance == null) {
            if (MIDletSuiteUtils.isAmsIsolate()) {
                instance = SystemServiceRequestorLocal.newInstance(token);
            } else {
                instance = SystemServiceRequestorRemote.newInstance(token);
            }
        }

        return instance;
    }
}
