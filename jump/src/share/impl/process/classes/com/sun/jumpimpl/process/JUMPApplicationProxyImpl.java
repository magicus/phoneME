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

package com.sun.jumpimpl.process;

import com.sun.jump.command.JUMPExecutiveLifecycleRequest;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.executive.JUMPIsolateProxy;

public class JUMPApplicationProxyImpl implements JUMPApplicationProxy {

    /**
     * The application this Proxy is associated with.
     */
    private JUMPApplication application;

    /**
     * This Proxy's application ID.
     */
    private int applicationId;

    /**
     * The Isolate which this application is running within.
     */
    private JUMPIsolateProxyImpl isolateProxy;

    /**
     * The (executive) RequestSender from the JUMPIsolateProxyImpl.
    **/
    private RequestSenderHelper     requestSender;

    /**
     * Creates an instance of JUMPApplicationProxy.  This method is expected to be
     * called by the JUMPIsolateProxy implementation.
     */
    JUMPApplicationProxyImpl(JUMPApplication application,
                         int applicationId, JUMPIsolateProxyImpl isolateProxy) {
        this.application = application;
        this.applicationId = applicationId;
        this.isolateProxy = isolateProxy;

        this.requestSender = isolateProxy.getRequestSender();
    }

    /**
     * Returns the JUMPApplication this proxy is accociated with.
     */
    public JUMPApplication getApplication() {
        return application;
    }

    /**
     * Returns the isolateProxy proxy this application is running in.
     */
    public JUMPIsolateProxy getIsolateProxy() {
        return isolateProxy;
    }

    /**
     * Pauses the application associated with this
     * <code>JUMPApplicationProxy</code>.
     **/
    public void pauseApp() {
        if (isolateProxy.isAlive()) {
           JUMPResponse response =
               requestSender.sendRequest(
                   isolateProxy,
                   new JUMPExecutiveLifecycleRequest(
                       JUMPExecutiveLifecycleRequest.ID_PAUSE_APP,
                       new String[] { Integer.toString(applicationId) }));
           requestSender.handleBooleanResponse(response);
        }
    }

    /**
     * Resumes the application associated with this
     * <code>JUMPApplicationProxy</code>.
     **/
    public void resumeApp() {
        if (isolateProxy.isAlive()) {
           JUMPResponse response =
               requestSender.sendRequest(
                   isolateProxy,
                   new JUMPExecutiveLifecycleRequest(
                       JUMPExecutiveLifecycleRequest.ID_RESUME_APP,
                       new String[] { Integer.toString(applicationId) }));
           requestSender.handleBooleanResponse(response);
        }
    }

    /**
     * Destroys the application associated with this
     * <code>JUMPApplicationProxy</code>.
     **/
    public void destroyApp() {

	// There is only one app in the isolate. 
	// Let the app state be reflected eagerly to the Isolate Proxy.
	isolateProxy.setStateToDestroyed();

        JUMPResponse response =
            requestSender.sendRequest(
                isolateProxy,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_DESTROY_APP,
                    new String[] { Integer.toString(applicationId), "true" }));
        requestSender.handleBooleanResponse(response);
    }

    /**
     * Returns the state of the application associated with this
     * <code>JUMPApplicationProxy</code>.
     **/
    public int getAppState() {
        if (!isolateProxy.isAlive()) {
		return DESTROYED;
        }

	return RUNNING;
    }

    public String toString() {
       return Integer.toString(applicationId);
    }
}
