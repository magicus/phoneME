/*
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

package com.sun.jump.executive;

import com.sun.jump.common.JUMPApplication;

/**
 * <code>JUMPApplicationProxy</code> encapsulates the information of the running 
 * application on the exectutive side, and associates it with the containing isolateProxy and 
 * running JUMPApplication, and the application ID.
 *
 * The <Code>JUMPIsolateProxy</code> is responsible for creating and returning
 * <Code>JUMPApplicationProxy</code> objects.
 */
public interface JUMPApplicationProxy {

    /**
     * Returns the JUMPApplication this proxy is accociated with.
     */
    public JUMPApplication getApplication();

    /**
     * Returns the isolateProxy proxy this application is running in.
     */
    public JUMPIsolateProxy getIsolateProxy();

    /** 
     * Pauses the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void pauseApp();

    /** 
     * Resumes the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void resumeApp();

    /** 
     * Destroys the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void destoryApp();

    /** 
     * Returns the state of the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public int getAppState();
   
}
