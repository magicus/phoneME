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

/**
 * This interface specifies methods of the object passed to the static
 * factory method of <code>JUMPAppContainer</code>. The methods enable
 * a container to discover information about its environment, as well
 * as a way to signal <code>JUMPApplication</code> state changes.
 *
 * The implementation of the <code>JUMPAppContainerContext</code> should
 * start off a non-daemon thread to ensure the process will not naturally die
 * before the application's startApp() method is invoked.
 *
 * @see <code>com.sun.jump.isolate.jvmprocess.JUMPAppContainer</code>
 */
public interface JUMPAppContainerContext {
    /**
     * Called by an <code>JUMPAppContainer</code> to notify central
     * AMS that a <code>JUMPApplication</code> has entered into the
     * <em>Destroyed</em> state.
     * <p>
     * If this method was called because the the last application in
     * the container was destroyed, then <em>after</em> this returns the
     * container should call <code>terminateIsolate</code>
     * and not <code>System.exit</code>.
     * <p>
     * This method may be called in any state, assuming the central AMS is
     * keeping track of state changes and will ignore extra notifications.
     *
     * @param appId runtime ID of the application assigned by
     *              <code>JUMPAppContainer.startApp</code>
     */
    void notifyDestroyed(int appId);

    /**
     * Called by an <code>JUMPAppContainer</code> to notify central
     * AMS that a <code>JUMPApplication</code> has entered into the
     * <em>PausedDestroyed</em> state.
     * <p>
     * This method may be called in any state, assuming the central AMS is
     * keeping track of state changes and will ignore extra notifications.
     *
     * @param appId runtime ID of the application assigned by
     *              <code>JUMPAppContainer.startApp</code>
     */
    void notifyPaused(int appId);

    /**
     * Called by an <code>JUMPAppContainer</code> to foreward a request for
     * a <code>JUMPApplication</code> to the central AMS to be allowed to
     * enter <em>Active</em> state.
     * <p>
     * This method may be called in any state, assuming the central AMS is
     * keeping track of state changes and will ignore extra requests.
     *
     * @param appId runtime ID of the application assigned by
     *              <code>JUMPAppContainer.startApp</code>
     */
    void resumeRequest(int appId);

    /**
     * Provides an <code>JUMPAppContainer</code> with a mechanism to retrieve
     * JUMP configuration properties.
     *
     * @param key name of the property
     *
     * @return value of the property or <code>null</code>,
     *         if no value is available for property
     */
    String getConfigProperty(String key);

    /**
     * Called to terminate the isolate gracefully.
     */
    void terminateIsolate();

    /**
     * Called to terminate the non-daemon thread running in this context.
     */
    void terminateKeepAliveThread();
}
