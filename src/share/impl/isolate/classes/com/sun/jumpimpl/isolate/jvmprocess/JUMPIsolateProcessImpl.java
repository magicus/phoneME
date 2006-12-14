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
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcess;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageResponseSender;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;
import com.sun.jumpimpl.process.RequestSenderHelper;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.command.JUMPRequest;
import com.sun.jump.command.JUMPResponse;

import sun.misc.ThreadRegistry;

public class JUMPIsolateProcessImpl extends JUMPIsolateProcess {
    private JUMPProcessProxyImpl    pp;
    private JUMPOSInterface         os;
    private int                     isolateId;
    private JUMPProcessProxy        execProcess;
    private JUMPMessageDispatcher   disp;
    private JUMPAppModel            appModel;

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

    /*
     * Kick start this isolate.
     * This is the entry point for the isolate process, creating the message
     * listener.
     */
    public static void start() {
	// Initialize os interface
	new com.sun.jumpimpl.os.JUMPOSInterfaceImpl();

	// Create and register the singleton isolate process
	JUMPIsolateProcessImpl ipi = new JUMPIsolateProcessImpl();

	// Create message processor thread.
	ipi.createListenerThread();
    }

    //
    // Main program sleeps forever. The message handlers do the job.
    // FIXME: This can initialize the system more.
    // We can also potentially combine main() and start() into one.
    // Finally, if we can find a way the dispatcher thread for mvm/client 
    // can keep the vm alive, we might not need a main() that sleeps forever.
    //
    public static void main(String[] args) 
    {
	JUMPAppModel appModel = JUMPAppModel.fromName(args[0]);
	if (appModel == null) {
	    // Unknown app model
	    throw new RuntimeException("Unknown app model "+args[0]);
	}
	JUMPIsolateProcessImpl ipi = 
	    (JUMPIsolateProcessImpl)JUMPIsolateProcess.getInstance();
	ipi.initialize(appModel);
	
	do {
	    try {
		Thread.sleep(0L);
	    } catch (Throwable e) {
	    }
	} while(true);
    }
    
    public void initialize(JUMPAppModel appModel) {
	System.err.println("Setting app model to "+appModel);
	this.appModel = appModel;
	// FIXME: Create container for 'appModel'
    }

    private void createListenerThread()
    {
	ThreadGroup tg = Thread.currentThread().getThreadGroup();
	for (ThreadGroup tgn = tg;
	     tgn != null;
	     tg = tgn, tgn = tg.getParent());
	Thread lthread = new ListenerThread(tg, "mvm client listener");
	/* If there were a special system-only priority greater than
	 * MAX_PRIORITY, it would be used here
	 */
	lthread.setPriority(Thread.MAX_PRIORITY);
	lthread.setDaemon(true);
	lthread.start();
    }
    
    //
    // Messages to this VM processed here
    // For now, all we do is report receipt, send back a success code
    // Eventually, we should handle generic messages here, and pass on
    // anything we don't know about to the container to process.
    //
    void processMessage(JUMPMessage in) 
    {
	JUMPMessageReader reader = new JUMPMessageReader(in);
	System.err.println("Incoming client message:");
	String[] responseStrings = reader.getUTFArray();
	for (int j = 0; j < responseStrings.length; j++) {
	    System.err.println("    \""+responseStrings[j]+"\"");
	}
	JUMPMessageResponseSender returnTo = in.getSender();
	JUMPOutgoingMessage m = newOutgoingMessage(in);
	m.addUTFArray(new String[] {"SUCCESS"});
	try {
	    returnTo.sendResponseMessage(m);
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
    
    class ListenerThread extends Thread {
	
	ListenerThread(ThreadGroup g, String name) {
	    super(g, name);
	}

	public void run() {
	    JUMPMessageDispatcher d = getMessageDispatcher();
	    try {
		d.registerDirect("mvm/client");
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
		    JUMPMessage m = d.waitForMessage("mvm/client", 0L);
		    processMessage(m);
		} catch (Throwable e) {
		    e.printStackTrace();
		}
	    }
	}
    }
}
