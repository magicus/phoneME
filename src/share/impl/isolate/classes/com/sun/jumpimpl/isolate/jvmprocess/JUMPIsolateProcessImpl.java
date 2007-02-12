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

package com.sun.jumpimpl.isolate.jvmprocess;

import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcess;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageResponseSender;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.message.JUMPMessageReceiveQueue;
import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.process.JUMPModulesConfig;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;
import com.sun.jumpimpl.process.RequestSenderHelper;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.command.JUMPExecutiveLifecycleRequest;
import com.sun.jump.command.JUMPCommand;
import com.sun.jump.command.JUMPRequest;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.command.JUMPResponseInteger;

import sun.misc.ThreadRegistry;

import java.util.Map;
import java.util.StringTokenizer;


public class JUMPIsolateProcessImpl extends JUMPIsolateProcess {
    private JUMPProcessProxyImpl    pp;
    private JUMPOSInterface         os;
    private int                     isolateId;
    private JUMPProcessProxy        execProcess;
    private JUMPMessageDispatcher   disp;
    private JUMPAppModel            appModel;
    private JUMPAppContainer        appContainer;

    protected JUMPIsolateProcessImpl() {
	super();
	os = JUMPOSInterface.getInstance();
	isolateId = os.getProcessID();
	pp = JUMPProcessProxyImpl.createProcessProxyImpl(isolateId);
    }

    public JUMPProcessProxy
    getExecutiveProcess() {
        if(execProcess == null) {
	    int epid = os.getExecutiveProcessID();
            execProcess = JUMPProcessProxyImpl.createProcessProxyImpl(epid);
        }
        return execProcess;
    }

    public int
    getProcessId() {
        return os.getProcessID();
    }

    public Map
    getConfig() {
        return JUMPModulesConfig.getProperties();
    }

    /**
     * Get app model running in this isolate process
     */
    public JUMPAppModel getAppModel() {
	return appModel;
    }
    

    public synchronized JUMPMessageDispatcher
    getMessageDispatcher() {
	if (disp == null) {
	    disp = pp.getMessageDispatcher();
	}
	return disp;
    }

    public JUMPOutgoingMessage
    newOutgoingMessage(String mesgType) {
        return pp.newOutgoingMessage(mesgType);
    }

    public JUMPOutgoingMessage
    newOutgoingMessage(JUMPMessage requestMessage) {
        return pp.newOutgoingMessage(requestMessage);
    }

    public JUMPMessage
    newMessage(byte[] rawData) {
        return pp.newMessage(rawData);
    }

    public void
    kill(boolean force) {
        throw new UnsupportedOperationException();
    }

    //
    // The message handlers do the job.
    // The message processor thread keeps the JVM alive.
    //
    public static void main(String[] args) 
    {
        if(args.length > 1 && args[1] != null) {
            JUMPModulesConfig.overrideDefaultConfig(args[1]);
        }

	// Initialize os interface
	new com.sun.jumpimpl.os.JUMPOSInterfaceImpl();

	// Create and register the singleton isolate process
	JUMPIsolateProcessImpl ipi = new JUMPIsolateProcessImpl();

	// Create message processor thread.
	ipi.createListenerThread();
	
	JUMPAppModel appModel = JUMPAppModel.fromName(args[0]);
	if (appModel == null) {
	    // Unknown app model
	    throw new RuntimeException("Unknown app model "+args[0]);
	}

	ipi.initialize(appModel);

	// Now we are ready. Drop off and let the message listener thread
	// keep this JVM alive.
    }
    
    public void initialize(JUMPAppModel appModel) {
	System.err.println("Setting app model to "+appModel);
	this.appModel = appModel;

        AppContainerFactoryImpl factory = new AppContainerFactoryImpl();
	this.appContainer = factory.getAppContainer(appModel);

        System.err.println(
            this + " config: " + JUMPModulesConfig.getProperties());

        String classes = (String)getConfig().get("isolate-init-classes");
        if(classes != null) {
            StringTokenizer st = new StringTokenizer(classes, ",");
            while(st.hasMoreTokens()) {
                try {
                    Class.forName(st.nextToken()).newInstance();
                } catch(Exception e) {
                    e.printStackTrace();
                    throw new RuntimeException("Initialization failed");
                }
            }
        }
    }

    private void createListenerThread()
    {
	Thread lthread = new ListenerThread("mvm client listener");
	/* If there were a special system-only priority greater than
	 * MAX_PRIORITY, it would be used here
	 */
	lthread.setPriority(Thread.MAX_PRIORITY);
	lthread.start();
    }
    
    //
    // Messages to this VM processed here
    // For now, all we do is report receipt, send back a success code
    // Eventually, we should handle generic messages here, and pass on
    // anything we don't know about to the container to process.
    //
    private void processMessage(JUMPMessage in) 
    {
	JUMPOutgoingMessage responseMessage;
	JUMPMessageResponseSender returnTo = in.getSender();
	
	JUMPCommand raw = JUMPRequest.fromMessage(in);
	String id = raw.getCommandId();
	// Now let's figure out the type
	if (id.equals(JUMPExecutiveLifecycleRequest.ID_START_APP)) {
	    JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
		JUMPExecutiveLifecycleRequest.fromMessage(in);
	    byte[] barr = elr.getAppBytes();
	    JUMPApplication app = JUMPApplication.fromByteArray(barr);
	    String[] args = elr.getArgs();
	    System.err.println("START_APP("+app+")");
	    // The message is telling us to start an application
	    int appId = appContainer.startApp(app, args);
	    // Now wrap this appid in a message and return it
	    JUMPResponseInteger resp;
	    if (appId != -1) {
		resp = new JUMPResponseInteger(in.getType(), 
					       JUMPResponseInteger.ID_SUCCESS,
					       appId);
	    } else {
		resp = new JUMPResponseInteger(in.getType(), 
					       JUMPResponseInteger.ID_FAILURE,
					       -1);
	    }
	    //
	    // Now convert JUMPResponse to a message in response
	    // to the incoming message
	    //
	    responseMessage = resp.toMessageInResponseTo(in, this);
	} else if (id.equals(JUMPExecutiveLifecycleRequest.ID_PAUSE_APP)) {
            JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
                    JUMPExecutiveLifecycleRequest.fromMessage(in);
	    String[] args = elr.getArgs();
	    int appID = Integer.parseInt(args[0]);
	    System.err.println("PAUSE_APP("+appID+")");
	    appContainer.pauseApp(appID);

	    JUMPResponse resp = new JUMPResponse(in.getType(), 
			                         JUMPResponseInteger.ID_SUCCESS);
	    responseMessage = resp.toMessageInResponseTo(in, this);
	} else if (id.equals(JUMPExecutiveLifecycleRequest.ID_RESUME_APP)) {
            JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
                    JUMPExecutiveLifecycleRequest.fromMessage(in);
	    String[] args = elr.getArgs();
	    int appID = Integer.parseInt(args[0]);
	    System.err.println("RESUME_APP("+appID+")");
	    appContainer.resumeApp(appID);

	    JUMPResponse resp = new JUMPResponse(in.getType(), 
			                         JUMPResponseInteger.ID_SUCCESS);
	    responseMessage = resp.toMessageInResponseTo(in, this);
	} else if (id.equals(JUMPExecutiveLifecycleRequest.ID_DESTROY_APP)) {
		
            JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
                    JUMPExecutiveLifecycleRequest.fromMessage(in);
	    String[] args = elr.getArgs();
	    int appID = Integer.parseInt(args[0]);
	    boolean unconditional = Boolean.getBoolean(args[1]);
	    System.err.println("DESTROY_APP("+appID+")");
	    appContainer.destroyApp(appID, unconditional);

	    JUMPResponse resp = new JUMPResponse(in.getType(), 
			                         JUMPResponseInteger.ID_SUCCESS);
	    responseMessage = resp.toMessageInResponseTo(in, this);
	} else {
	    // Assumption of default message
	    // A utf array, expecting a generic JUMPResponse
	    JUMPMessageReader reader = new JUMPMessageReader(in);
	    System.err.println("Incoming client message:");
	    String[] responseStrings = reader.getUTFArray();
	    for (int j = 0; j < responseStrings.length; j++) {
		System.err.println("    \""+responseStrings[j]+"\"");
	    }
	    responseMessage = newOutgoingMessage(in);
	    responseMessage.addUTFArray(new String[] {"SUCCESS"});
	}

	try {
	    returnTo.sendResponseMessage(responseMessage);
	} catch (Throwable e) {
	    e.printStackTrace();
	}
    }
    
    /**
     * Report to the executive that we have initialized ourselves
     */
    private void reportIsolateInitialized() 
    {
	JUMPProcessProxy e = getExecutiveProcess();
	RequestSenderHelper rsh = new RequestSenderHelper(this);
	String reqId = JUMPIsolateLifecycleRequest.ID_ISOLATE_INITIALIZED;
	String[] reqArgs = new String[] { Integer.toString(isolateId), "" };
	JUMPRequest req = new JUMPIsolateLifecycleRequest(reqId, this);
					    
	rsh.sendRequestAsync(e, req);
    }
    
    // XXX Why not just use JUMPMessageDispatcher.addHandler("mvm/client", ...)?
    private class ListenerThread extends Thread {
	
	ListenerThread(String name) {
	    super(name);
	}

	public void run() {
	    JUMPMessageReceiveQueue queue;
	    try {
		JUMPMessageDispatcher dispatcher =
		    getMessageDispatcher();
		queue = dispatcher.createJUMPMessageReceiveQueue("mvm/client");
	    } catch (Throwable e) {
		e.printStackTrace();
		return;
	    }
	    //
	    // Once registerDirect() completes with success,
	    // we know we can receive messages. Report.
	    //
	    reportIsolateInitialized();
	    while (!ThreadRegistry.exitRequested()) {
		try {
		    JUMPMessage m = queue.receiveMessage(0L);
		    processMessage(m);
		} catch (Throwable e) {
		    e.printStackTrace();
		}
	    }
	}
    }
}
