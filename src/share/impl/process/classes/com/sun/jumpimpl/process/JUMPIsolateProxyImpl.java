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

import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.command.JUMPExecutiveLifecycleRequest;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;

public class JUMPIsolateProxyImpl extends JUMPProcessProxyImpl implements JUMPIsolateProxy {
    private static final long DEFAULT_TIMEOUT = 0L;
    private int                     isolateId;
    private RequestSenderHelper     requestSender;
    //
    // Isolate state
    //
    private int state = 0;

    /**
     * Wait for the isolate to reach a target state
     * FIXME: This should probably return a boolean indicating
     * whether target state was reached.
     */
    public synchronized void waitForState(int targetState, long timeout) 
    {
	long time = System.currentTimeMillis();
	
	while (state < targetState) {
	    try {
		// FIXME: This code is unfortunate. There is no
		// timeout exception in wait(), so we must figure out
		// if timeout happens or state is reached first.
		wait(timeout);
		long time2 = System.currentTimeMillis();
		long elapsed = time2 - time;
		if (elapsed > timeout) {
		    System.err.println("Timed out waiting for "+
				       "target state="+targetState);
		    return;
		}
	    } catch (Exception e) {
		e.printStackTrace();
		return;
	    }
	}
    }
    
    //
    // A constructor. This instance is to be constructed after the isolate
    // process is created. This proxy represents that isolate.
    //
    public JUMPIsolateProxyImpl(int pid) {
	super(pid);
        isolateId = pid;
        requestSender = new RequestSenderHelper(JUMPExecutive.getInstance()); 
	setIsolateState(JUMPIsolateLifecycleRequest.ISOLATE_STATE_CREATED);
    }

    public static JUMPIsolateProxyImpl registerIsolate(int pid) 
    {
	//
	// Synchronize on the JUMPProcessProxyImpl class which does
	// process instance registration.
	//
	synchronized(JUMPProcessProxyImpl.class) {
	    JUMPIsolateProxyImpl ipi = getRegisteredIsolate(pid);
	    if (ipi == null) {
		// The constructor registers the instance as well.
		return new JUMPIsolateProxyImpl(pid);
	    } else {
		return ipi;
	    }
	}
    }
    
    public static JUMPIsolateProxyImpl getRegisteredIsolate(int pid) 
    {
	//
	// Synchronize on the JUMPProcessProxyImpl class which does
	// process instance registration.
	//
	synchronized(JUMPProcessProxyImpl.class) {
	    JUMPProcessProxyImpl ppi = 
		JUMPProcessProxyImpl.getProcessProxyImpl(pid);
	    if ((ppi != null) && (ppi instanceof JUMPIsolateProxyImpl)) {
		return (JUMPIsolateProxyImpl)ppi;
	    } else {
		return null;
	    }
	}
    }
    
    public int getState(int appId) {
        throw new UnsupportedOperationException();
    }

    /**
     * Set isolate state to a new state, and notify all listeners
     */
    public synchronized void setIsolateState(int state) 
    {
	this.state = state;
	notifyAll();
    }
    
    /**
     * Return last known state in isolate.
     */
    public synchronized int getIsolateState() 
    {
	return this.state;
    }
    
    public int startApp(JUMPApplication app, String[] args) {
        return
            requestSender.sendRequestWithIntegerResponse(
                this,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_START_APP,
		    app.toByteArray(),
		    args));
    }

    public void pauseApp(int appId) {
        JUMPResponse response =
            requestSender.sendRequest(
                this,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_PAUSE_APP,
                    new String[] { Integer.toString(appId) }));
        requestSender.handleBooleanResponse(response);
    }

    public void resumeApp(int appId) {
        JUMPResponse response =
            requestSender.sendRequest(
                this,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_RESUME_APP,
                    new String[] { Integer.toString(appId) }));
        requestSender.handleBooleanResponse(response);
    }

    public void destroyApp(int appId) {
        JUMPResponse response =
            requestSender.sendRequest(
                this,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_DESTROY_APP,
                    new String[] { Integer.toString(appId), "true" }));
        requestSender.handleBooleanResponse(response);
    }

    public int
    getIsolateId() {
        return isolateId;
    }

    public void
    kill(boolean force) {
        JUMPResponse response =
            requestSender.sendRequest(
                this,
                new JUMPExecutiveLifecycleRequest(
                    JUMPExecutiveLifecycleRequest.ID_DESTROY_ISOLATE,
                    new String[] { Boolean.toString(force) }));
        requestSender.handleBooleanResponse(response);
    }
}
