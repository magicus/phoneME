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

package com.sun.jump.messagequeue;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

/**
 * <code>JUMPMessage</code> encapsulates the message header (envelope 
 * information) and the message payload. The message payload can be one
 * of the following
 * <ul>
 *   <li>java.lang.String</li>
 *   <li>byte[]</li>
 *   <li>any serilaizable object (implementing java.io.Serializable)</li>
 * </ul>
 * <p>
 * The message header consists of the following information
 * <ul>
 *   <li>Message ID - A unique identifier for the message</li>
 *   <li>Response Message ID - The message ID for which the message is a
 *       response. {@link #isResponseMessage()} indicates if a message is
 *       a response or not</li>
 *   <li>Sender - The sender of the message. If there are any responses to
 *       be sent, the sender's outhoing queue is used to send it.
 *       The sender is {@link com.sun.jump.messagequeue.JUMPMessagable}</li>
 *   <li>Message Type - An arbitrary string that identifies the message. This
 *       is typically used to tag or classify the message</li>
 * </ul>
 */
public class JUMPMessage {
    private byte[] dataBytes;
    private JUMPMessagable sender;
    private String type;
    private int id;
    private int responseId = -1;
    private int dataType;
    
    private static final int DATATYPE_BYTE_ARRAY   = 0x01;
    private static final int DATATYPE_STRING       = 0x02;
    private static final int DATATYPE_SERIALIZABLE = 0x04;
    
    /**
     * 
     * Creates a new instance of JUMPMessage 
     * 
     * @throws java.lang.IllegalArgumentException if the data is not a
     *         byte[], String or java.io.Serializable.
     */
    protected JUMPMessage(JUMPMessagable sender, 
        String type,
        Object data,
        int responseId) {
        this.sender = sender;
        this.type = type;
        this.responseId = responseId;
        if ( data instanceof byte[] ){
            this.dataType = DATATYPE_BYTE_ARRAY;
            this.dataBytes = (byte[])data;
        }
        else
        if ( data instanceof String ){
            this.dataType = DATATYPE_STRING;
            this.dataBytes = ((String)data).getBytes();
        }
        else
        if ( data instanceof Serializable ){
            this.dataType = DATATYPE_SERIALIZABLE;
            this.dataBytes = getBytes((Serializable)data);
        }
        else {
            throw new IllegalArgumentException("Invalid data");
        }
    }
    
    protected JUMPMessage(JUMPMessagable sender, 
        String type,
        Object data) {
        this(sender, type, data, -1);
    }
    public boolean containsString() {
        return false;
    }
    
    public boolean containsBytes() {
        return false;
    }
    
    public boolean containsSerializable() {
        return false;
    }
    
    public JUMPMessagable getSender() {
        return this.sender;
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
     * Indicates if this message is a response to a <code>JUMPMessage</code>
     * 
     * @see #getResponseId
     */
    public boolean isResponseMessage() {
        return this.responseId >= 0;
    }
    
    public String extractString() {
        return new String(this.dataBytes);
    }
    
    public byte[] extractBytes() {
        return this.dataBytes;
    }
    
    public Serializable extractSerializable() {
        ByteArrayInputStream bain = 
            new ByteArrayInputStream(this.dataBytes);
        ObjectInputStream ois = null;
        try {
            ois = new ObjectInputStream(bain);
            return (Serializable) ois.readObject();
        } catch (IOException ex) {
            ex.printStackTrace();
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        }
        finally {
            if ( ois != null ){
                try {
                    ois.close();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        }
        return null;
    }
    
    private byte[] getBytes(Serializable object) {
        ByteArrayOutputStream baout = 
            new ByteArrayOutputStream();
        ObjectOutputStream oos = null;
        try {
            oos = new ObjectOutputStream(baout);
            oos.writeObject(object);
            return baout.toByteArray();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        finally {
            if ( oos != null ){
                try {
                    oos.close();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        }
        return null;
    }
}
