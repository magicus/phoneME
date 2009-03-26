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

/**
 * <code>JUMPResponse</code> encapsulates the response command
 */
public class JUMPResponse extends JUMPCommand {
   
    public static final String ID_SUCCESS      = "Success";
    public static final String ID_FAILURE      = "Failure";
    /**
     * Response that contains some data, which can be retrieved using
     * <code>JUMPCommand.getCommandData</code>
     */
    public static final String ID_DATA         = "Data";
    
    /**
     * Creates a new instance of <code>JUMPResponse</code> by deserializing
     * the data from the <code>JUMPMessage</code>
     */
    public static JUMPResponse fromMessage(JUMPMessage message) {
	return (JUMPResponse)JUMPCommand.fromMessage(message,
						     JUMPResponse.class);
    }

    //
    // To be filled in when de-serializing
    //
    protected JUMPResponse() {
	super();
    }

    //
    // A private constructor when the request is to be filled in
    // by deserialization from a message
    //
    private JUMPResponse(String messageType){
        super(messageType, "", null);
    }
    
    public JUMPResponse(String messageType, String id){
        super(messageType, id, null);
    }
    
    public JUMPResponse(String messageType, String id, String[] args){
        super(messageType, id, args);
    }
}
