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
import com.sun.jump.common.JUMPWindow;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.command.JUMPExecutiveLifecycleRequest;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import java.util.HashMap;
import java.util.HashSet;

public class JUMPIsolateProxyImpl extends JUMPProcessProxyImpl implements JUMPIsolateProxy {
    // FIXME: Timeout values should be centralized somewhere
    private static final long DEFAULT_TIMEOUT = 5000L;
    private int                     isolateId;
    private RequestSenderHelper     requestSender;
    private HashMap                 appIDHash = null;
    private HashSet                 windows;
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
		wait(timeout);
		if (state < targetState) {
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
        appIDHash = new HashMap();
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
    
    public JUMPApplicationProxy startApp(JUMPApplication app, String[] args) {
        if (isAlive()) {
           int appID = requestSender.sendRequestWithIntegerResponse(
                   this,
                   new JUMPExecutiveLifecycleRequest(
                       JUMPExecutiveLifecycleRequest.ID_START_APP,
		       app.toByteArray(),
		       args));

	   if (appID == -1) { // failure
	   	return null;
           } 
        
           JUMPApplicationProxy appProxy = new JUMPApplicationProxyImpl(app, appID, this);
           appIDHash.put(new Integer(appID), appProxy);
	   setIsolateState(JUMPIsolateLifecycleRequest.ISOLATE_STATE_RUNNING);

           return appProxy;
        }

	return null;  
    }

    public JUMPApplicationProxy[] getApps() {
        Object obj[];
        synchronized(this) {
            obj = appIDHash.values().toArray();
        }
        JUMPApplicationProxy appProxy[] = new JUMPApplicationProxy[obj.length];
        for (int i = 0; i < obj.length; i++) {
            appProxy[i] = (JUMPApplicationProxy)obj[i];
        }
        return appProxy;
    }
    
    public JUMPWindow[] getWindows() {
        synchronized(this) {
            if(windows == null || windows.size() == 0) {
                return null;
            }
            return (JUMPWindow[])windows.toArray(new JUMPWindow[]{});
        }
    }


    public int
    getIsolateId() {
        return isolateId;
    }

    public void
    kill(boolean force) {
        if (isAlive()) {
   	   setStateToDestroyed();
           JUMPResponse response =
               requestSender.sendRequest(
                   this,
                   new JUMPExecutiveLifecycleRequest(
                       JUMPExecutiveLifecycleRequest.ID_DESTROY_ISOLATE,
                       new String[] { Boolean.toString(force) }));
           requestSender.handleBooleanResponse(response);
	}   
    }

    RequestSenderHelper getRequestSender() {
        return requestSender;
    }

    /**
     * Sets this IsolateProxy to the destroyed state and 
     * perform all the data cleanup.
     */
    public void
    setStateToDestroyed() {
	setIsolateState(JUMPIsolateLifecycleRequest.ISOLATE_STATE_DESTROYED);
        appIDHash.clear();
    }

    /**
     * Return true if this IsolateProxy represents a created and
     * not yet destroyed Isolate.
     */
    public boolean 
    isAlive() { 
        int state = getIsolateState();
        switch(state) {
	    case JUMPIsolateLifecycleRequest.ISOLATE_STATE_CREATED:
	    case JUMPIsolateLifecycleRequest.ISOLATE_STATE_INITIALIZED:
	    case JUMPIsolateLifecycleRequest.ISOLATE_STATE_RUNNING:
		    return true;
 	}
	return false;
    }

    public void
    registerWindow(JUMPWindow w) {
        if(w == null || w.getIsolate() != this) {
            throw new IllegalStateException();
        }

        synchronized(this) {
            if(windows == null) {
                windows = new HashSet();
            }
            windows.add(w);
        }
    }
}
