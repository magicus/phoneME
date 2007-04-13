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

package com.sun.jump.isolate.jvmprocess;

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
     * Loads, initializes, and signals the <code>JUMPApplication</code> to
     * enter the <em>Active</em> state.
     * <p>
     * In the <em>Active</EM> state the <code>JUMPApplication</code> may
     * hold resources.
     * <p>
     * The method will only be called once per application launch.
     * <p>
     * If the <code>JUMPApplication</code> interfaces with the user, it
     * expected that it will display a user interface before this method
     * returns.
     * <p>
     * If successful, the method must not return until the application is
     * considered completely initilized and active according to the
     * application model of the container.
     *
     * @param app properties of the application to start
     * @param args arguments to pass to the application
     *
     * @return non-negative container unique runtime ID to identify the
     * application in future method calls or negative int if the application
     * can't be started
     */
    public abstract int startApp(JUMPApplication app, String[] args);

    /**
     * Signals the <code>JUMPApplication</code> to enter
     * the <em>Paused</em> state.
     * <p>
     * In the <em>Paused</em> state the <code>JUMPApplication</code> should
     * release shared resources and become quiescent. If the application
     * interfaces with the user it should still be prepared to display its
     * interface.
     * <p>
     * Some application models will not have the concept of <em>Paused</em>
     * state, in this case the method can be empty.
     * <p>
     * This method will only be called called when the
     * <code>JUMPApplication</code> is in the <em>Active</em> state.
     * <p>
     * The method must not return until the application is has responded
     * to state change according the application model of the container.
     * <p>
     * This method should not catch unchecked exceptions, so that caller
     * can report a failure back to the executive and then the executive may
     * issue a destroyApp request.
     *
     * @param appId runtime ID of the application assigned by startApp    
     */
    public abstract void pauseApp(int appId);
    
    /**
     * Signals the <code>JUMPApplication</code> that to enter the
     * <em>Active</em> state.
     * In the <em>Active</EM> state the <code>JUMPApplication</code> may
     * hold resources.
     * <p>
     * Some application models will not have the concept of <em>Paused</em>
     * state, in this case the method can be empty.
     * <p>
     * The method will only be called when the <code>JUMPApplication</code>
     * is in the <em>Paused</em> state.
     * <p>
     * The method must not return until the application is has responded
     * to state change according the application model of the container.
     * <p>
     * This method should not catch unchecked exceptions, so that caller
     * can report a failure back to the executive and then the executive may
     * issue a destroyApp request.
     *
     * @param appId runtime ID of the application assigned by startApp    
     */
    public abstract void resumeApp(int appId);
    
    /**
     * Signals the <code>JUMPApplication</code> to prepare to be terminated
     * by entering the <em>Destroyed</em> state.
     * <p>
     * When entering the <Destroyed</em> state the
     * <code>JUMPApplication</code> should release all resources gracefully
     * and saved any persistent state, in preparation for isolate
     * termination.
     * <p>
     * This method may be called from the <em>Paused</em> or <em>Active</em>
     * states.
     * <p>
     * The method must not return until the application is has responded
     * to state change according the application model of the container.
     * <p>
     * If method results in destroying the last application, then this
     * method should call <code>JUMPAppContainerContext.terminateIsolate</code>
     * and not <code>System.exit</code>.
     *
     * @param appId runtime ID of the application assigned by startApp
     * @param force If true when this method is called, the
     * <code>JUMPApplication</code> must cleanup and release all resources.
     * If false the <code>JUMPApplication</code> may choose to ignore the
     * request and this method will throw an exception to indicate the
     * applcation was not destroyed.
     */
    public abstract void destroyApp(int appId, boolean force);

    /**
     * Handle messages specific to an application model, if needed. This
     * message would be from a module implemented for the same application
     * model with a format agreed upon outside JUMP API.
     *
     * @param message message from a peer
     */
    public void handleMessage(JUMPMessage message) {
        // call the methods by unpacking the message contents
    }
}
