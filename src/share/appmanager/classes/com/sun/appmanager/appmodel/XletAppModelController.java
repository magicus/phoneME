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
 * A class that encapsulates the lifecycle of an xlet app model
 */
public class XletAppModelController extends AppModelController
{
    /**
     * Construct a new xlet controller based on an underlying
     * mtask client.
     */
    public XletAppModelController(Client mtaskClient) {
	super(mtaskClient);
    }

    /**
     * Start target xlet <tt>xletId</tt> (cause its
     * <tt>initXlet()</tt> method to be called).  */
    public boolean xletInitialize(String xletId) {
	return mtaskClient.message(xletId, "XLET_INITIALIZE");
    }

    /**
     * Start target xlet <tt>xletId</tt> (cause its
     * <tt>startXlet()</tt> method to be called).  */
    public boolean xletStart(String xletId) {
	return mtaskClient.message(xletId, "XLET_START");
    }

    /**
     * Pause target xlet <tt>xletId</tt> (cause its
     * <tt>pauseXlet()</tt> method to be called).  
     */
    public boolean xletPause(String xletId) {
	return mtaskClient.message(xletId, "XLET_PAUSE");
    }

    /**
     * Destroy target xlet <tt>xletId</tt> (cause its
     * <tt>destroyXlet()</tt> method to be called).  
     */
    public boolean xletDestroy(String xletId) {
       return xletDestroy(xletId, true);
    } 
 
    public boolean xletDestroy(String xletId, boolean conditional) {
        if (conditional) {
	   return mtaskClient.message(xletId, "XLET_DESTROY");
        } else {
	   return mtaskClient.message(xletId, "XLET_DESTROY_COND");
        }
    }

    /**
     * Activate a target xlet: make it visible and cause
     * its <tt>startXlet()</tt> method to be called.
     */
    public boolean xletActivate(String xletId) {
	return mtaskClient.message(xletId, "XLET_ACTIVATE");
    }

    /**
     * De-activate a target xlet: make it invisible and cause
     * its <tt>pauseXlet()</tt> method to be called.
     */
    public boolean xletDeactivate(String xletId) {
	return mtaskClient.message(xletId, "XLET_DEACTIVATE");
    }

    /**
     * Get state of target xlet
     */
    public int xletGetState(String xletId) {
	String r = mtaskClient.messageWithResponse(xletId, "XLET_GET_STATE");
	if (r == null) {
	    return XLET_UNKNOWN;
	} else if (r.equals("loaded")) {
	    return XLET_LOADED;
	} else if (r.equals("paused")) {
	    return XLET_PAUSED;
	} else if (r.equals("active")) {
	    return XLET_ACTIVE;
	} else if (r.equals("destroyed")) {
	    return XLET_DESTROYED;
	} else {
	    return XLET_UNKNOWN;
	}
    }

    public String xletStateToString(int state) {
	switch(state) {
	    case XLET_LOADED: return "XLET_LOADED";
	    case XLET_PAUSED: return "XLET_PAUSED";
	    case XLET_ACTIVE: return "XLET_ACTIVE";
	    case XLET_DESTROYED: return "XLET_DESTROYED";
	    case XLET_UNKNOWN: return "UNKNOWN";
	    default: return "UNKNOWN";
	}
    }

    /*
     * Xlet lifecycle settings
     */
    public static final int XLET_UNKNOWN = 0;
    public static final int XLET_LOADED = 1;
    public static final int XLET_PAUSED = 2;
    public static final int XLET_ACTIVE = 3;
    public static final int XLET_DESTROYED = 4;
}
