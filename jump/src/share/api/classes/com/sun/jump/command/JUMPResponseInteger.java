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
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.command.JUMPResponse;

/**
 * <code>JUMPResponse</code> encapsulates the response command
 */
public class JUMPResponseInteger extends JUMPResponse {
    private int theInt = 0;
    
    public int getInt() {
	return theInt;
    }
    
    public static final String ID_SUCCESS      = JUMPResponse.ID_SUCCESS;
    public static final String ID_FAILURE      = JUMPResponse.ID_FAILURE;

    /**
     * Creates a new instance of <code>JUMPResponseInteger</code> by
     * deserializing the data from the <code>JUMPMessage</code>
     */
    public static JUMPResponse fromMessage(JUMPMessage message) {
	return (JUMPResponse)
	    JUMPCommand.fromMessage(message, JUMPResponseInteger.class);
    }

    //
    // To be filled in when de-serializing
    //
    protected JUMPResponseInteger() {
	super();
    }

    //
    // A private constructor when the request is to be filled in
    // by deserialization from a message
    //
    private JUMPResponseInteger(String messageType){
        super(messageType, "", null);
    }
    
    public JUMPResponseInteger(String messageType, String id){
        super(messageType, id, null);
    }
    
    public JUMPResponseInteger(String messageType, String id, int theInt) {
        super(messageType, id, null);
	this.theInt = theInt;
    }

    /**
     * For subclasses to use to initialize any fields
     * using <code>JUMPMessage.get*</code> methods.
     */
    protected void deserializeFrom(JUMPMessageReader message) {
	// First deserialize any shared fields
	super.deserializeFrom(message);
	// And now lifecycle request specific fields
	this.theInt = message.getInt();
    }

    /**
     * For subclasses to use to put data in a message
     * using <code>JUMPOutgoingMessage.add*</code> methods.
     */
    protected void serializeInto(JUMPOutgoingMessage message) {
	// First deserialize any shared fields
	super.serializeInto(message);
	// And now lifecycle request specific fields
	message.addInt(this.theInt);
    }

}
