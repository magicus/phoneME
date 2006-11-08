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

package com.sun.appmanager.appmodel;

import com.sun.appmanager.mtask.Client;

/**
 * A class that encapsulates the lifecycle of a managed app model
 */
public abstract class AppModelController
{
    /**
     * Xlet application model, to be passed in to
     * {@link getAppModelController()}.
     */
    public static final int XLET_APP_MODEL = 1;

    /**
     * The basic mtask controller for this app model
     */
    protected Client mtaskClient;

    /**
     * A new AppModelController with an underlying mtask client
     * <tt>mtaskClient</tt> */
    public AppModelController(Client mtaskClient) {
	this.mtaskClient = mtaskClient;
    }

    /**
     * Create a new app model controller for a given app model id.
     */
    public static AppModelController 
    getAppModelController(int appModelId, Client client) {
	if (appModelId == XLET_APP_MODEL) {
	    return new XletAppModelController(client);
	} else {
	    // Not a recognized app model
	    return null;
	}
    }
}
