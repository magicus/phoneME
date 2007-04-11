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

package com.sun.jumpimpl.process;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.message.JUMPTimedOutException;

import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPProcess;

import com.sun.jump.os.JUMPOSInterface;
import com.sun.jumpimpl.os.JUMPMessageQueueInterfaceImpl;

import java.io.IOException;
import java.util.Vector;
import java.util.HashMap;

public class JUMPMessageDispatcherImpl implements JUMPMessageDispatcher {
    private HashMap msgTypeToDispatcherThread = new HashMap();
    private static JUMPMessageDispatcherImpl INSTANCE = null;
    private static final JUMPMessageQueueInterfaceImpl queue =
        (JUMPMessageQueueInterfaceImpl)JUMPOSInterface.getInstance().getQueueInterface();

    public static JUMPMessageDispatcherImpl getInstance() 
    {
	synchronized(JUMPMessageDispatcherImpl.class) {
	    if (INSTANCE == null) {
		INSTANCE = new JUMPMessageDispatcherImpl();
	    }
	}
	return INSTANCE;
    }
    
    /*
     * A token that represents a registration
     * In the case of a handler based registration:
     * A handle to the dispatcher thread, and a pointer to the handler
     * constitutes a token.
     *
     * In the case of a direct registration:
     * Null for dispatcher and handler, type for the direct-registered
     * message type.
     */
    private static class Token {
	DispatcherThread dt;
        JUMPMessageHandler handler;
	String type;

	Token(DispatcherThread dt, JUMPMessageHandler handler, String type) 
	{
	    this.dt = dt;
	    this.handler = handler;
	    this.type = type;
	}
    }

    /*
     * A DispatcherThread is created for every new unique message type
     * for which handlers are created. Each such thread keeps track
     * of any handlers to invoke for that message type.
     */
    private static class DispatcherThread extends Thread {
	String mesgType;
	JUMPMessageDispatcherImpl mdi;
	Vector handlers = new Vector();
	Object lock = new Object();
	
	DispatcherThread(JUMPMessageDispatcherImpl mdi, String mesgType) 
	{
	    super();
	    this.mdi = mdi;
	    this.mesgType = mesgType;
	    //
	    // To make sure that the OS structures exist for the type we
	    // are listening to, before we start listening.
	    //
	    queue.reserve(mesgType);
	}
	
	private void dispatchMessage(JUMPMessage msg) {
	    Object[] harr;

	    synchronized(lock) {
		// snapshot under lock, in case anyone tries to add
		// or remove elements from this handler.
		harr = handlers.toArray();
	    }
	    
	    for (int i = 0; i < harr.length; i++) {
		JUMPMessageHandler handler = (JUMPMessageHandler)harr[i];
		try {
		    handler.handleMessage(msg);
		} catch(RuntimeException e) {
		    e.printStackTrace();
		}
	    }
	}
	
	void addHandler(JUMPMessageHandler h) {
	    synchronized(lock) {
		handlers.addElement(h);
	    }
	}
	
	void removeHandler(JUMPMessageHandler h) {
	    synchronized(lock) {
		handlers.removeElement(h);
	    }
	}
	
	public void run() {
	    while(true) {
		// FIXME: should we have "timeout" set from some system
		// property, say
		// "com.sun.jump.messagequeue.JUMPMessageDispatcherImpl.timeout"
		// FIXME: What is the exit condition for this thread?

		try {
		    JUMPMessage msg = mdi.doWaitForMessage(mesgType, 0L);
		    dispatchMessage(msg);
		} catch(JUMPTimedOutException e) {
		    e.printStackTrace();
		} catch(IOException e) {
		    e.printStackTrace();
		}
	    }
	}
    }

    private Vector directRegistrations = new Vector();
    
    public Object registerDirect(String messageType) 
	throws JUMPMessageDispatcherTypeException {
	synchronized(msgTypeToDispatcherThread) {
	    if (msgTypeToDispatcherThread.get(messageType) != null) {
		// Already registered for handler-based registration
		throw new JUMPMessageDispatcherTypeException("Type "+messageType+" already registered with handlers");
	    }
	}
	
	synchronized(directRegistrations) {
	    directRegistrations.addElement(messageType);
	}
	//
	// To make sure that the OS structures exist for the type we
	// are listening to, before we start listening.
	//
	queue.reserve(messageType);
	
	return new Token(null, null, messageType);
    }

    public JUMPMessage waitForMessage(String messageType, long timeout)
        throws JUMPMessageDispatcherTypeException, JUMPTimedOutException, IOException
    {
	// First check for type consistency. Has this been "direct-registered"?
	synchronized(directRegistrations) {
	    if (!directRegistrations.contains(messageType)) {
		throw new JUMPMessageDispatcherTypeException("Type "+messageType+" not registered for direct listening");
	    }
	}
	//
	// At this point, we know it's been direct-registered, which
	// means it can't be callback based. No need to check that table
	// as well.
	//
	
	// Now do the work
	return doWaitForMessage(messageType, timeout);
    }

    private JUMPMessage doWaitForMessage(String messageType, long timeout)
        throws JUMPTimedOutException, IOException 
    {
	byte[] raw = queue.receiveMessage(messageType, timeout);
	return new MessageImpl.Message(raw);
    }
    
    public Object
    registerHandler(String messageType, JUMPMessageHandler handler)
	throws JUMPMessageDispatcherTypeException {
	DispatcherThread dt;
	
        if(handler == null) {
            throw new IllegalArgumentException("handler can't be null");
        }
        if(messageType == null) {
            throw new IllegalArgumentException("messageType can't be null");
        }
	synchronized(directRegistrations) {
	    if (directRegistrations.contains(messageType)) {
		throw new JUMPMessageDispatcherTypeException("Type "+messageType+" already registered for direct listening");
	    }
	}
	
	synchronized(msgTypeToDispatcherThread) {
	    dt = (DispatcherThread)msgTypeToDispatcherThread.get(messageType);
	    
	    if (dt == null) {
		dt = new DispatcherThread(this, messageType);
		msgTypeToDispatcherThread.put(messageType, dt);
		dt.start();
	    }
	}
	dt.addHandler(handler);
	return new Token(dt, handler, messageType);
    }

    public void
    cancelRegistration(Object registrationToken) {
        Token token = (Token)registrationToken;
	if (token.dt == null) {
	    synchronized(directRegistrations) {
		directRegistrations.removeElement(token.type);
	    }
	} else {
	    token.dt.removeHandler(token.handler);
	}
	
	// FIXME: Some GC of dispatcher threads would be cool here.
	// For example, any dispatcher thread without remaining handles
	// can simply exit.
    }
}
