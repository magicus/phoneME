/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.client;

import java.rmi.registry.Registry;

/**
 * 
 * <tt>Client</tt> is the class that contains handles to each of the
 * AppManager client modules. Only one <tt>Client</tt> instance is
 * created per cdcams client. The static methods of <tt>Client</tt>
 * may be used to get access to client components.
 *
 */
abstract public class Client {

    /**
     * The id of this client instance, as identified by mtask
     */
    protected String clientId;

    private static Client instance = null;

    /**
     * Create a singleton instance.
     *
     * <p>If more than one instance is created, a
     * <tt>RuntimeException</tt> is thrown.
     */
    public Client() {
        if (instance == null) {
            instance = this;
        } else {
            throw new RuntimeException();
        }
    }

    /**
     * Return the mtask client id of this client
     */
    public static String getClientId() {
	return instance.clientId;
    }
}
