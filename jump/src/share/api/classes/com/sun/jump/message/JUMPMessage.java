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

package com.sun.jump.message;

import java.io.IOException;
import com.sun.jump.os.JUMPOSInterface;

/**
 * <code>JUMPMessage</code> encapsulates the message header (envelope 
 * information) and the message payload. The message payload is an array
 * of bytes.
 *
 * The message is composed at a data offset determined by the OS. (obtained 
 * from <code>JUMPMessageQueueInterface.getDataOffset()</code>)
 * <p>
 * The message header consists of the following information
 * <ul>
 *   <li>Message ID - A unique identifier for the message</li>
 *   <li>Response Message ID - The message ID for which the message is a
 *       response. {@link #isResponseMessage()} indicates if a message is
 *       a response or not</li>
 *   <li>Sender - The sender of the message. If there are any responses to
 *       be sent, the sender's outhoing queue is used to send it.
 *       The sender is {@link com.sun.jump.message.JUMPMessagable}</li>
 *   <li>Message Type - An arbitrary string that identifies the message. This
 *       is typically used to tag or classify the message</li>
 * </ul>
 * <p>
 *
 * Instances of <Code>JUMPMessage</code> are manufactured from factories 
 * conforming to {@link com.sun.jump.message.JUMPMessagingService} 
 *
 */
public abstract class JUMPMessage {
    protected int messageDataOffset;
    protected int messageMarkOffset;
    protected int messageUserDataOffset;
    protected byte[] messageDataBytes;
    protected int MESSAGE_DATA_OFFSET =
	JUMPOSInterface.getInstance().getQueueInterface().getDataOffset();
    
    protected JUMPMessagable sender;
    protected String type;
    protected String returnType;
    protected int id;
    protected int responseId = -1;

    /**
     * Creates a new instance of JUMPMessage when
     * deserializing an incoming message
     */
    protected JUMPMessage(byte[] data) {
	this.messageDataBytes = data;
	this.messageDataOffset = MESSAGE_DATA_OFFSET;
	readHeader();
    }
    
    protected JUMPMessage() {
    }

    /**
     * Get sender of this message in a form that allows the caller
     * to send a response
     */
    public JUMPMessageResponseSender getSender() {
        return this.getMessageSender();
    }

    public String getReturnType() 
    {
	return this.returnType;
    }
    
    /**
     * Returns the message type.
     */
    public String getType() {
        return this.type;
    }
    
    /**
     * Returns the message id
     */
    public int getId() {
        return this.id;
    }
    
    /**
     * Returns the response message id. The value makes sense only if
     * {@link #isResponseMessage()} return true.
     */
    public int getResponseId() {
        return this.responseId;
    }
    
    /**
     * Return payload data offset
     */
    public int getPayloadDataOffset() {
	return messageDataOffset;
    }

    /**
     * Return payload 
     * (to be read at offset <code>getPayloadDataOffset()</code>)
     */
    public byte[] getPayload() {
	return messageDataBytes;
    }

    /**
     * Return payload (to be read at offset
     * <code>JUMPMessageQueueInterface.getDataOffset()</code>)
     */
    public byte[] getMessageData() {
	return messageDataBytes;
    }

    /**
     * Indicates if this message is a response to a <code>JUMPMessage</code>
     * 
     * @see #getResponseId
     */
    public boolean isResponseMessage() {
        return this.responseId >= 0;
    }
    
    private void readHeader() {
	JUMPMessageReader r = new JUMPMessageReader(this, MESSAGE_DATA_OFFSET);
	this.id = r.getInt();
	this.responseId = r.getInt();
	readMessageSender(r.getInt());
	this.returnType = r.getUTF();	
	this.type = r.getUTF();	
	// Update the offsets to point past the header
	this.messageDataOffset = r.messageDataOffset;
	this.messageUserDataOffset = messageDataOffset;
	this.messageMarkOffset = messageDataOffset;
	// User data follows
    }

    public void reset() {
	messageDataOffset = messageMarkOffset;
    }

    protected abstract void readMessageSender(int id);
    protected abstract JUMPMessageResponseSender getMessageSender();
}
