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

package com.sun.tck.xlet;

/**
 * This interface defines methods that trigger state changes
 * and query for the state of an xlet. The state change trigger
 * methods should be called from the same thread to guarantee
 * the correct sequence of state changes.
 */

public interface ManageableXlet {
    /**
     * Request the Xlet to enter PAUSED state from LOADED state. 
     * It should be silently ignored if the Xlet is not in LOADED state at the
     * time of the request, otherwise the Xlet's initXlet() method should 
     * be invoked.
     *
     * Note that the state of the Xlet may not yet be changed when this method 
     * returns, i.e., the method may return to the caller before completing 
     * the work. 
     *
     */
    void initXlet();

    /**
     * Request the Xlet to enter ACTIVE state from PAUSED state. 
     * It should be silently ignored if the Xlet is not in PAUSED state at the
     * time of the request, otherwise the Xlet's startXlet() method should 
     * be invoked.
     * 
     * Note that the state of the xlet may not yet be changed when this method 
     * returns, i.e., the method may return to the caller before completing 
     * the work. 
     *
     */
    void startXlet();

    /**
     * Request the Xlet to enter PAUSED state from ACTIVE state. 
     * It should be silently ignored if the Xlet is not in ACTIVE state at the
     * time of the request, otherwise the Xlet's pauseXlet() method should 
     * be invoked.
     *
     * Note that the state of the xlet may not yet be changed when this method 
     * returns, i.e., the method may return to the caller before completing 
     * the work. 
     *
     */
    void pauseXlet();
    
    /**
     * Request the Xlet to enter DESTROYED state and invoke the Xlet's
     * destroyXlet(unconditional) method. 
     *
     * Note that the state of the xlet may not yet be changed when this method 
     * returns, i.e., the method may return to the caller before completing 
     * the work. 
     *
     */
    void destroyXlet(boolean unconditional);
    
    /**
     * Get the state of the xlet.
     *
     * @return state of the Xlet, which should be one of 
     *          LOADED, ACTIVE, PAUSED, DESTROYED or UNKNOWN
     */
    int getState();
    
    /**
     * Xlet is not loaded, or its state information
     * can not be found by the Xlet manager. This may happen
     * if the Xlet has been destroyed and the Xlet
     * manager does not keep its reference.
     */
    int UNKNOWN   = 0;
    
    /**
     * Xlet is loaded by calling its
     * no-argument constructor.
     */
    int LOADED    = 1;
    
    /**
     * Xlet is paused 
     */
    int PAUSED    = 2;
    
    /**
     * Xlet is active
     */
    int ACTIVE    = 3;
    
    /**
     * Xlet is destroyed
     */
    int DESTROYED = 4;

}
