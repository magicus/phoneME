/*
 * %W% %E%
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

package com.sun.jump.isolate.jvmprocess;

import com.sun.jump.common.JUMPWindow;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;

import com.sun.jump.common.JUMPApplication;

/**
 * <code>JUMPAppContainer</code> defines the application container. 
 * The container is responsible for launching the the application class
 * and hosts the application and handles all the JUMP Isolate related 
 * functionality transparently to the application. It also receives 
 * and processes application model agnostic messages.
 * <p>
 * Unless specified for a method, it is assumed that methods catch all
 * exceptions (but not fatal errors).
 * <p>
 * This class is extended by AppModel specific containers.
 */
public abstract class JUMPAppContainer implements JUMPMessageHandler {
    
    /**
     * Creates a new instance of JUMPAppContainer.
     * An uncheck exception can be thrown from this method to cause
     * the isolate to exit.
     */
    protected  JUMPAppContainer() {
    }
    
    /**
     * Start the application specific by the JUMPApplication object.
     * The method should not return until the application is considered
     * completely initilized and in a running state according to the profile
     * of the container.
     *
     * @param app properties of the application to start
     * @param args arguments to pass to the application
     *
     * @return non-negative container unique runtime ID to identify the
     * application in future method calls or -1 if the application can't be
     * started
     */
    public abstract int startApp(JUMPApplication app, String[] args);

    /**
     * Pause an application.
     * The method should not return until the application is has responded
     * to the action according to the profile of the container.
     *
     * @param appId runtime ID of the application assigned by startApp    
     */
    public abstract void pauseApp(int appId);
    
    /**
     * Resume a paused an application.
     * The method should not return until the application is has responded
     * to the action according to the profile of the container.
     *
     * @param appId runtime ID of the application assigned by startApp    
     */
    public abstract void resumeApp(int appId);
    
    /**
     * Destroy an application.
     * The method should not return until the application is has responded
     * to the action according to the profile of the container.
     * <p>
     * The method can throw an exception to indicate that the MIDlet
     * was not destroyed.
     *
     * @param appId runtime ID of the application assigned by startApp    
     */
    public abstract void destroyApp(int appId, boolean force);

    public void handleMessage(JUMPMessage message) {
        // call the methods by unpacking the message contents
    }
}
