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

import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessagable;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPTimedOutException;

import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.os.JUMPMessageQueueInterfaceImpl;

import java.util.HashMap;
import java.io.IOException;

/**
 * Base class for any proxy that refers to another process
 */
public class JUMPProcessProxyImpl implements JUMPProcessProxy, JUMPMessagingService, JUMPMessageSender, JUMPMessagable {
    int processId;

    private static final JUMPMessageQueueInterfaceImpl queue =
        (JUMPMessageQueueInterfaceImpl)JUMPOSInterface.getInstance().getQueueInterface();
    private static final HashMap proxyMap = new HashMap();
    
    /** 
     * Given a process id, create a proxy that can forward messages 
     */
    protected JUMPProcessProxyImpl(int processId) {
	this.processId = processId;
	// Register this instance
	Integer pidObj = new Integer(processId);
	synchronized(JUMPProcessProxyImpl.class) {
	    proxyMap.put(pidObj, this);
	}
    }
    
    /*
     * Make sure to track process proxies here. 
     * Keep one process proxy object per pid.
     */
    public static synchronized JUMPProcessProxyImpl 
	createProcessProxyImpl(int processId) 
    {
	if (processId == -1) {
	    throw new Error("Uninitialized process!");
	}
	JUMPProcessProxyImpl ppi = getProcessProxyImpl(processId);
	if (ppi == null) {
	    ppi = new JUMPProcessProxyImpl(processId);
	}
	return ppi;
    }
    
    /*
     * Make sure to track process proxies here. 
     * Keep one process proxy object per pid.
     */
    public static synchronized JUMPProcessProxyImpl 
	getProcessProxyImpl(int processId) 
    {
	Integer pidObj = new Integer(processId);
	Object ppiObj = proxyMap.get(pidObj);
	
	return (JUMPProcessProxyImpl)ppiObj;
    }
    
    /*
     * Delete process proxy for 'processId'
     */
    public static synchronized void
	deleteProcessProxyImpl(int processId) 
    {
	Integer pidObj = new Integer(processId);
	Object ppiObj = proxyMap.get(pidObj);
	if (ppiObj != null) {
	    proxyMap.remove(pidObj);
	}
    }
    
    public int getProcessId() {
	return processId;
    }

    /**
     * Sends a response message
     * 
     * @throws java.io.IOException is the message cannot be sent due to
     *         I/O error.
     */
    public void
    sendResponseMessage(JUMPOutgoingMessage message) throws IOException {
        queue.sendMessageResponse(message.serialize(),
				  message.isResponseMessage());
    }

    /**
     * Sends a message to the <code>process</code>.
     * 
     * @throws java.io.IOException is the message cannot be sent due to
     *         I/O error.
     */
    public void
    sendMessage(JUMPOutgoingMessage message) throws IOException {
        queue.sendMessageAsync(this.processId, 
			       message.serialize(),
			       message.isResponseMessage());
    }

    public JUMPMessage
    sendMessage(JUMPOutgoingMessage message, long timeout)
        throws JUMPTimedOutException, IOException {
	byte[] raw = queue.sendMessageSync(this.processId, 
					   message.serialize(),
					   message.isResponseMessage(),
					   timeout);
	return new MessageImpl.Message(raw);
    }


    /**
     * Gets the message dispacther for the process
     */
    public JUMPMessageDispatcher getMessageDispatcher () 
    {
	return JUMPMessageDispatcherImpl.getInstance();
    }
    

    /**
     * Creates a new, <i>blank</i> <Code>JUMPOutgoingMessage</code> for the
     * message type. The sender of the message is automatically filled
     * in by the factory implementation.
     */
    public JUMPOutgoingMessage newOutgoingMessage (String mesgType)
    {
	JUMPMessagable s = this;
	return new MessageImpl.OutgoingMessage(s, 
					       mesgType,
					       queue.getReturnType(), 
					       -1);
    }
    
    /**
     * Create a new, <i>blank</i> <code>JUMPOutgoingMessage</code> as a
     * response to the request message passed. The sender of the
     * message is automatically filled in by the factory
     * implementation.
     * Make sure to pass the return type of the request message.
     */
    public JUMPOutgoingMessage newOutgoingMessage (JUMPMessage requestMessage)
    {
	JUMPMessagable s = (JUMPMessagable)requestMessage.getSender();
	return new MessageImpl.OutgoingMessage(s, 
					       requestMessage.getType(), 
					       requestMessage.getReturnType(), 
					       requestMessage.getResponseId());
    }
    

    /**
     * Create a new, <i>immutable</i> <code>JUMPMessage</code>
     * out of a raw array of bytes received on the message queue.
     */
    public JUMPMessage newMessage (byte[] rawData) 
    {
	return new MessageImpl.Message(rawData);
    }


}

    
