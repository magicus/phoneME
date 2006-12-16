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

package com.sun.jump.command;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPOutgoingMessage;


/**
 * <code>JUMPCommand</code> is base class that encapsulates any command that
 * is sent from the executive to an isolate (and vice-versa). The 
 * <code>JUMPCommand</code> abstracts the contents of the data part (payload)
 * of the <code>JUMPMessage</code>. The following code samples shows sending
 * and receiving of <code>JUMPCommand</code> using the messaging
 * infrastructure.
 * <h3>Sending a JUMPCommand.</h3>
 * <pre>
 *   JUMPCommand command; // JUMPRequest or JUMPResponse
 *   JUMPMessagingService thisProcess;  // JUMPExecutive or JUMPIsolateProcess
 *   JUMPMessageSender target; // JUMPIsolateProcessProxy or JUMPProcessProxy
 *
 *   JUMPCommand command = new JUMPRequest(&lt;id&gt;, args); 
 *   // create a message of the command type
 *   JUMPOutgoingMessage message = command.toMessage(thisProcess);
 *
 *   // (a) synchronous send of a message
 *   JUMPMessage responseMessage = target.sendMessage(message, 0L);
 *   JUMPResponse response = JUMPResponse.fromMessage(responseMessage);
 *
 *   // (b) asynchronous send of a message
 *   JUMPMessageDispatcher disp = thisProcess.getMessageDispatcher();
 *   // Register a handler for a response to this message
 *   Object token = disp.registerHandler(message, ResponseHandler.getInstance());
 * 
 *   // send the message to the isolate
 *   target.sendMessage(message);
 * </pre>
 *
 * <h3>Receving a JUMPResponse</h3>
 * <pre>
 *   public class ResponseHandler implements JUMPMessageHandler {
 *       void 	handleMessage(JUMPMessage message) {
 *           JUMPResponse response = JUMPResponse.fromMessage(message);
 *           // "response" usage follows ...
 *           // Unregister registration for this transaction.
 *           disp.cancelRegistration(token);
 *       }
 *   }
 * </pre>
 * <h3>Receiving a JUMPCommand and sending a response. </h3>
 * <pre>
 *   public class RequestHandler implements JUMPMessageHandler {
 *       void 	handleMessage(JUMPMessage message) {
 *           JUMPRequest request = JUMPRequest.fromMessage(message);
 *           JUMPMessageResponseSender requestSender = message.getSender();
 *           // process request
 *           // ...
 *           JUMPResponse response = new JUMPResponse(&lt;responseId&gt;);
 *           // fill in response...
 *           // ...
 *           JUMPMessagingService myProcess; // JUMPExecutive or 
 *                                              JUMPIsolateProcess
 *
 *           // create a response message of the command type
 *           JUMPOutgoingMessage responseMessage = 
 *               response.toMessage(myProcess);
 *
 *           // The response goes back to the sender
 *           requestSender.sendResponseMessage(responseMessage);
 *        }
 *    }   
 * </pre>
 */
public abstract class JUMPCommand {
    protected String messageType;
    protected String id;
    protected String[] data;

    /**
     * Creates a new instance of JUMPCommand
     * @param messageType the type of the message to carry this command
     * @param id the type of the command
     * @param data the data carried in the command
     */
    JUMPCommand(String messageType,
		String id, String[] data) {
	this.messageType = messageType;
        this.id = id;
        this.data = data;
    }
    
    //
    // To be filled in when de-serializing
    //
    protected JUMPCommand() {
    }

    private void setMessageType(String mType) {
	this.messageType = mType;
    }

    public String[] getCommandData() {
        return this.data;
    }
    
    public String getCommandId() {
        return this.id;
    }
    
    public String getCommandMessageType() {
        return this.messageType;
    }
    
    /**
     * Convert this command into an outgoing message
     */
    public final JUMPOutgoingMessage toMessage(JUMPMessagingService s) {
	JUMPOutgoingMessage m = s.newOutgoingMessage(this.messageType);
	this.serializeInto(m);
	return m;
    }

    /**
     * Convert this command into an outgoing message
     */
    public final JUMPOutgoingMessage 
	toMessageInResponseTo(JUMPMessage r, JUMPMessagingService s) {
	JUMPOutgoingMessage m = s.newOutgoingMessage(r);
	this.serializeInto(m);
	return m;
    }

    /**
     * Creates a new instance of <code>JUMPCommand</code> by deserializing
     * the data from the <code>JUMPMessage</code>
     */
    public static JUMPCommand fromMessage(JUMPMessage message,
					  Class commandClass) {
	JUMPCommand c;
	try {
	    c = (JUMPCommand)commandClass.newInstance();
	} catch (InstantiationException e) {
	    e.printStackTrace();
	    return null;
	} catch (IllegalAccessException e) {
	    e.printStackTrace();
	    return null;
	}
	c.setMessageType(message.getType());
        c.deserializeFrom(message);
        return c;
    }

    protected final void deserializeFrom(JUMPMessage message) {
	this.messageType = message.getType();
	JUMPMessageReader r = new JUMPMessageReader(message);
	deserializeFrom(r);
    }
    
    /** 
     * For subclasses to use to initialize any fields
     * using <code>JUMPMessage.get*</code> methods.
     */
    protected void deserializeFrom(JUMPMessageReader message) {
	this.id = message.getUTF();
	this.data = message.getUTFArray();
    }
    
    /** 
     * For subclasses to use to put data in a message
     * using <code>JUMPOutgoingMessage.add*</code> methods.
     */
    protected void serializeInto(JUMPOutgoingMessage message) {
	message.addUTF(this.id);
	message.addUTFArray(this.data);
    }
}
