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

package com.sun.jumpimpl.executive;

import java.io.IOException;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;
import com.sun.jump.command.JUMPCommand;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.executive.*;
import com.sun.jump.message.*;
import com.sun.jump.module.JUMPModule;
import com.sun.jump.os.JUMPOSInterface;

import com.sun.jumpimpl.process.JUMPIsolateProxyImpl;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;
import com.sun.jumpimpl.process.RequestSenderHelper;
import com.sun.jumpimpl.process.JUMPModulesConfig;

public class IsolateFactoryImpl
        implements JUMPIsolateFactory, JUMPMessageHandler {
    
    private Vector processes;
    private Vector isolates;
    private JUMPMessageDispatcher dispatcher;
    private JUMPExecutive exec;
    private RequestSenderHelper rsh;
    private Object messageRegistration;
    private String isolateExtraArg;
    private String defaultVMArgs;
    
    IsolateFactoryImpl() {
	exec = JUMPExecutive.getInstance();
        dispatcher = exec.getMessageDispatcher();
	rsh = new RequestSenderHelper(exec);
	processes = new Vector();
	isolates = new Vector();

        Map config = JUMPModulesConfig.getProperties();

        //
        // Get the default VM arguments from config
        //
        defaultVMArgs = (String)config.get("vm.args");

	//
	// Get all isolatemanager command messages here.
	//
	try {
	    String type = JUMPIsolateLifecycleRequest.MESSAGE_TYPE;
	    messageRegistration = dispatcher.registerHandler(type, this);
	} catch (Throwable e) {
	    e.printStackTrace();
	    throw new RuntimeException("Lifecycle module initialization failed");
	}

        isolateExtraArg = (String)config.get("runtime-properties-file");
    }
    
    /**
     * Create new isolate conforming to <code>model</code>,
     * and with additional VM arugments.
     */
    public JUMPIsolateProxy newIsolate(JUMPAppModel model, String vmArgs) {
        String vmArgs0 = "";       
        if (defaultVMArgs != null && !defaultVMArgs.equals("")) {
            vmArgs0 = defaultVMArgs.trim() + " ";
        }
        if (vmArgs != null) {
            vmArgs0 = vmArgs0 + vmArgs.trim();
        }

        String args[];
        if(isolateExtraArg == null) {
            args = new String[] { vmArgs0, model.getName() };
        } else {
            args = new String[] { vmArgs0, model.getName(), isolateExtraArg };
        }

	// The following is inherently racy, but it is handled properly.
	// 
	// 1) The process is created
	// 2) The representative JUMPIsolateProxyImpl is created.
	//
	// Between #1 and #2, the "initialized" message might come in
	// from the newly created process, and will be looking for the isolate
	// object JUMPIsolateProxyImpl that matches the pid. If the instance
	// is not registered yet, we have a problem.
	//
	// The solution is to have the message handler and newIsolate()
	// race to register the isolate object for the pid.
	// One of them will always win out, and there will still be
	// only one isolate object for each pid.
	//
        int pid = JUMPOSInterface.getInstance().createProcess(args);
        if (pid == -1) {
	    /* failed to create process */
            throw new RuntimeException("Failed to the isolate process");
        }

        JUMPIsolateProxyImpl isolate =
	    JUMPIsolateProxyImpl.registerIsolate(pid);
        isolates.add(isolate);

	//
	// Wait until isolate is initialized (it will send us a message
	// when it is ready)
	//
	// FIXME: what's the right timeout value and where is that stored?
	isolate.waitForState(JUMPIsolateLifecycleRequest.ISOLATE_STATE_INITIALIZED, 5000L);

        // Tell the isolate about the ixc port.
        // FIXME: should go away once ixc is on messaging.
        rsh.sendRequest(isolate,
               new com.sun.jumpimpl.ixc.IxcMessage(
               com.sun.jumpimpl.ixc.ConnectionReceiver.getExecVMServicePort()));

	//
	// FIXME!!!! What happens if we time out? We can kill
	// the isolate and return null? 
	// Or we can make newIsolate() return some sort of error code
	// and allow a re-try? Must figure this out
	//
        return isolate;
    }


    /**
     * Create new isolate conforming to <code>model</code>
     */
    public JUMPIsolateProxy newIsolate(JUMPAppModel model) {
        return newIsolate(model, null);
    }

    /**
     * Returns the <code>JUMPIsolate</code> associated with the isolate id.
     * It returns <Code>null</code> if no such isolate is found.
     */
    public JUMPIsolateProxy getIsolate(int isolateId) {
	return JUMPIsolateProxyImpl.getRegisteredIsolate(isolateId);
    }
    
    /**
     * Returns all the active and running isolates.
     */
    public JUMPIsolateProxy[] getActiveIsolates() {
        Vector activeIsolates = new Vector();
        Iterator i = isolates.iterator();
	//
	// FIXME: what state should this call be looking at?
	// All created isolates? All initialized isolates? All running?
	//
	// FIXME: Need synchronization here on "isolates".
	//
        while (i.hasNext()) {
            JUMPIsolateProxyImpl isolate = ((JUMPIsolateProxyImpl)i.next());
	    if (isolate.isAlive()) {
                activeIsolates.add(isolate);
	    }
        }
        
        return (JUMPIsolateProxy[]) activeIsolates.toArray(new JUMPIsolateProxy[]{});        
    }
    
    public void handleMessage(JUMPMessage m) {
	JUMPCommand raw = JUMPIsolateLifecycleRequest.fromMessage(m);
        JUMPIsolateLifecycleRequest request = (JUMPIsolateLifecycleRequest)raw;
	 
	String requestId = request.getCommandId();
        
	//
	// FIXME: the isolate object should created automatically by the
	// JUMPIsolateLifecycleRequest class. To ensure that, however,
	// requires that getIsolate() be available both on the client
	// and the executive side.
	//
	String initId = JUMPIsolateLifecycleRequest.ID_ISOLATE_INITIALIZED;
	String destroyedId = JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED;

	int initState = JUMPIsolateLifecycleRequest.ISOLATE_STATE_INITIALIZED;
	
        if (requestId.equals(initId)) {
	    // Construction 
	    JUMPIsolateProxyImpl ipi = 
		JUMPIsolateProxyImpl.registerIsolate(request.getIsolateId());

	    //
	    // Make sure proxy reflects state reached by the isolate. 
	    // Caller is waiting on this.
	    //
	    ipi.setIsolateState(initState);
	    // No response required on this request.
        } else if (requestId.equals(destroyedId)) {
            int pid = request.getIsolateId();

	    JUMPIsolateProxyImpl ipi = 
		JUMPIsolateProxyImpl.getRegisteredIsolate(pid);
	    if (ipi != null) {
                ipi.setStateToDestroyed();
	        isolates.remove(ipi);
            }
        }
    }
}
