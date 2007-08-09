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

package com.sun.jumpimpl.isolate.jvmprocess;

import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;
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
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jumpimpl.client.module.windowing.WindowingIsolateClient;
import com.sun.jumpimpl.client.module.serviceregistry.ServiceRegistryClient;

import java.rmi.Remote;
import java.rmi.NotBoundException;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import com.sun.jumpimpl.ixc.XletContextFactory;

import sun.misc.MIDPConfig;
import sun.misc.ThreadRegistry;

import java.util.Map;
import java.util.StringTokenizer;


public class JUMPIsolateProcessImpl 
    extends JUMPIsolateProcess
    implements JUMPMessageHandler, JUMPAppContainerContext
{
    private JUMPProcessProxyImpl    pp;
    private JUMPOSInterface         os;
    private int                     isolateId;
    private JUMPProcessProxy        execProcess;
    private JUMPMessageDispatcher   disp;
    private JUMPAppModel            appModel;
    private JUMPAppContainer        appContainer;
    private WindowingIsolateClient  windowing;
    private ServiceRegistryClient   serviceRegistry;
    private Object stateChangeMutex = new Object();
    private boolean dispatchingStateChange;
    private boolean exitAfterStateChange;
    private final KeepAliveMonitor keepAliveMonitor = new KeepAliveMonitor();

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
    public static void main(String[] args) {
	try {
            if(args.length > 1 && args[1] != null) {
                JUMPModulesConfig.overrideDefaultConfig(args[1]);
            }

            // Initialize os interface
            new com.sun.jumpimpl.os.JUMPOSInterfaceImpl();

            // Create and register the singleton isolate process
            JUMPIsolateProcessImpl ipi = new JUMPIsolateProcessImpl();

	    // Register the executive before starting the messaging thread
            ipi.getExecutiveProcess();

            JUMPMessageDispatcher d = ipi.getMessageDispatcher();

            d.registerHandler("mvm/client", ipi);
            // FIXME: should go away once Ixc is on messaging
            d.registerHandler("mvm/ixc", ipi);

            JUMPAppModel appModel = JUMPAppModel.fromName(args[0]);
            if (appModel == null) {
                // Unknown app model
                throw new RuntimeException("Unknown app model "+args[0]);
            }

            ipi.initialize(appModel);

            //
            // Once registerDirect() completes with success,
            // we know we can receive messages. Report.
            //
            ipi.reportIsolateInitialized();

            // Keep around the main thread  
            ipi.keepAliveMonitor.startWaiting();

	} catch (Throwable e) {
	    e.printStackTrace();
	    System.exit(-1);
	}
    }

    public void initialize(JUMPAppModel appModel) {
	System.err.println("Setting app model to "+appModel);
	this.appModel = appModel;

        AppContainerFactoryImpl factory = new AppContainerFactoryImpl();

        this.appContainer = factory.getAppContainer(appModel, this);

        this.windowing = new WindowingIsolateClient();

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

    //
    // Messages to this VM processed here
    // For now, all we do is report receipt, send back a success code
    // Eventually, we should handle generic messages here, and pass on
    // anything we don't know about to the container to process.
    //
    public void handleMessage(JUMPMessage in) {
	JUMPOutgoingMessage responseMessage;
	JUMPCommand raw = JUMPRequest.fromMessage(in);
	String id = raw.getCommandId();

	System.err.println("RECEIVED MESSAGE TYPE "+id);

        synchronized (stateChangeMutex) {
            dispatchingStateChange = true;
        }

        try {
            // Now let's figure out the type
            if (id.equals(JUMPExecutiveLifecycleRequest.ID_START_APP)) {
                responseMessage = handleStartAppMessage(in);
            } else if (id.equals(JUMPExecutiveLifecycleRequest.ID_PAUSE_APP)) {
                responseMessage = handlePauseAppMessage(in);
            } else if (
                    id.equals(JUMPExecutiveLifecycleRequest.ID_RESUME_APP)) {
                responseMessage = handleResumeAppMessage(in);
            } else if (
                    id.equals(JUMPExecutiveLifecycleRequest.ID_DESTROY_APP)) {
                responseMessage = handleDestroyAppMessage(in);
            } else if (
                id.equals(com.sun.jumpimpl.ixc.IxcMessage.ID_PORT)) {
                // Tell the isolate about the ixc port.
                // FIXME: should go away once ixc is on messaging.
                responseMessage = handleIxcMessage(in);
            } else {
                responseMessage = handleUnknownMessage(in);
            }

            in.getSender().sendResponseMessage(responseMessage);

        } catch (Throwable e) {
            e.printStackTrace();
        } finally {
            synchronized (stateChangeMutex) {
                dispatchingStateChange = false;

                if (exitAfterStateChange) {
                    System.exit(0);
                }
            }
        }
    }

    public void notifyDestroyed(int appId) {
        //TODO: send message back to executive
    }

    public void notifyPaused(int appId) {
        //TODO: send message back to executive
    }

    public void resumeRequest(int appId) {
        //TODO: send message back to executive
    }

    public String getConfigProperty(String key) {
        return (String)JUMPModulesConfig.getProperties().get(key);
    }

    public void terminateIsolate() {
        synchronized (stateChangeMutex) {
            if (dispatchingStateChange) {
                exitAfterStateChange = true;
            } else {
                System.exit(0);
            }
        }
    }

    /**
     * Report to the executive that we have initialized ourselves
     */
    private void reportIsolateInitialized() {
	JUMPProcessProxy e = getExecutiveProcess();
	RequestSenderHelper rsh = new RequestSenderHelper(this);
	String reqId = JUMPIsolateLifecycleRequest.ID_ISOLATE_INITIALIZED;
	String[] reqArgs = new String[] { Integer.toString(isolateId), "" };
	JUMPRequest req = new JUMPIsolateLifecycleRequest(reqId, this);
					    
	rsh.sendRequestAsync(e, req);
    }

    /** {@inheritDoc} */
    public Remote getRemoteService(String name) {
        return serviceRegistry.getRemoteService(name);
    }

    private JUMPOutgoingMessage handleStartAppMessage(JUMPMessage in) {
        JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
            JUMPExecutiveLifecycleRequest.fromMessage(in);
        byte[] barr = elr.getAppBytes();
        JUMPApplication app = JUMPApplication.fromByteArray(barr);
        String[] args = elr.getArgs();
        System.err.println("START_APP("+app+")");
        int appId = -1;

        // Notify windowing that application is going to be started before
        // app container is initialized to set bounds 
        // for the app screen area.
        windowing.onBeforeApplicationStarted(app);

        if(appContainer != null) {
            // The message is telling us to start an application
            appId = appContainer.startApp(app, args);
        }

        // Now wrap this appid in a message and return it
        JUMPResponseInteger resp;
        if (appId >= 0) {
            resp = new JUMPResponseInteger(in.getType(), 
                                           JUMPResponseInteger.ID_SUCCESS,
                                           appId);
        } else {
            resp = new JUMPResponseInteger(in.getType(), 
                                           JUMPResponseInteger.ID_FAILURE,
                                           -1);
        }

        /*
         * Now convert JUMPResponse to a message in response
	 * to the incoming message
	 */
        return resp.toMessageInResponseTo(in, this);
    }

    private JUMPOutgoingMessage handlePauseAppMessage(JUMPMessage in) {
        JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
            JUMPExecutiveLifecycleRequest.fromMessage(in);
        String[] args = elr.getArgs();
        int appID = Integer.parseInt(args[0]);
        String responseId;

        System.err.println("PAUSE_APP("+appID+")");

        try {
            appContainer.pauseApp(appID);
            responseId = JUMPResponseInteger.ID_SUCCESS;
        } catch (Throwable t) {
            responseId = JUMPResponseInteger.ID_FAILURE;
        }

        JUMPResponse resp = new JUMPResponse(in.getType(), responseId);
        
        return resp.toMessageInResponseTo(in, this);
    }

    private JUMPOutgoingMessage handleResumeAppMessage(JUMPMessage in) {
        JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
            JUMPExecutiveLifecycleRequest.fromMessage(in);
        String[] args = elr.getArgs();
        int appID = Integer.parseInt(args[0]);
        String responseId;

        System.err.println("RESUME_APP("+appID+")");

        try {
            appContainer.resumeApp(appID);
            responseId = JUMPResponseInteger.ID_SUCCESS;
        } catch (Throwable t) {
            responseId = JUMPResponseInteger.ID_FAILURE;
        }

        JUMPResponse resp = new JUMPResponse(in.getType(), responseId);

        return resp.toMessageInResponseTo(in, this);
    }

    private JUMPOutgoingMessage handleDestroyAppMessage(JUMPMessage in) {
        JUMPExecutiveLifecycleRequest elr = (JUMPExecutiveLifecycleRequest)
            JUMPExecutiveLifecycleRequest.fromMessage(in);
        String[] args = elr.getArgs();
        int appID = Integer.parseInt(args[0]);
        boolean unconditional = Boolean.getBoolean(args[1]);
        System.err.println("DESTROY_APP("+appID+")");
        String responseCode = JUMPResponseInteger.ID_SUCCESS;

        try {
            appContainer.destroyApp(appID, unconditional);
        } catch (Throwable t) {
            responseCode = JUMPResponseInteger.ID_FAILURE;
        }

        JUMPResponse resp = new JUMPResponse(in.getType(), responseCode);
        return resp.toMessageInResponseTo(in, this);
    }

    private JUMPOutgoingMessage handleUnknownMessage(JUMPMessage in) {
        // Assumption of default message
        // A utf array, expecting a generic JUMPResponse
        JUMPMessageReader reader = new JUMPMessageReader(in);
        System.err.println("Incoming client message:");
        String[] responseStrings = reader.getUTFArray();

        for (int j = 0; j < responseStrings.length; j++) {
            System.err.println("    \""+responseStrings[j]+"\"");
        }

        JUMPOutgoingMessage out = newOutgoingMessage(in);
        out.addUTFArray(new String[] {"SUCCESS"});
        return out;
    }

    /**
     * Fetches class loader to use for implementation classes in IXC.
     *
     * @return class loader to use
     */
    private ClassLoader getImplClassLoader() {
        return appContainer.getClass().getClassLoader();
    }

    /**
     * Extract port number from the message and tell it to the 
     * service registry client.
     * FIXME: should be removed once ixc is on messaging.
     */
    private JUMPOutgoingMessage handleIxcMessage(JUMPMessage in) {
        JUMPCommand message = JUMPCommand.fromMessage(in, 
                                  com.sun.jumpimpl.ixc.IxcMessage.class);

        int port = Integer.parseInt(message.getCommandData()[0]);
 
        serviceRegistry = new ServiceRegistryClient(getImplClassLoader(), port);

        JUMPResponse resp = new JUMPResponse(in.getType(), JUMPResponseInteger.ID_SUCCESS);
        return resp.toMessageInResponseTo(in, this);
    }

    public void terminateKeepAliveThread() {
       keepAliveMonitor.terminate();
    }

    static class KeepAliveMonitor {
        private boolean keepAlive = true;
        public synchronized void startWaiting() {
           while (keepAlive) {
              try {
                  this.wait();
              } catch (InterruptedException e){
                  /*
                   * Ignore InterruptedException and continue with the loop.
                   * This thread should exit only if terminate() is invoked.
                   */
              }
           }
        }
          
        public synchronized void terminate() {
           keepAlive = false; 
           this.notifyAll();
        }
     }
}
