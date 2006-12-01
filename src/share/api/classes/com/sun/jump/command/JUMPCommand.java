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

import com.sun.jump.messagequeue.JUMPMessage;


/**
 * <code>JUMPCommand</code> is base class that encapsulates any command that
 * is sent from the executive to an isolate (and vice-versa). The methods 
 * <code>JUMPCommand.serialize()</code> and 
 * <code>JUMPCommand.deserialize()</code> are used to create 
 * <code>JUMPMessage</code> which can then be transported using the
 * messaging infrastructure.
 */
public abstract class JUMPCommand {
    private String[] data;
    private String id;
    
    /**
     * Creates a new instance of JUMPCommand
     */
    JUMPCommand(String id, String[] args) {
        this.id = id;
        this.data = args;
    }
    
    /**
     * Deserializes and Initializes the command with contents of the message.
     */
    public void deserialize(JUMPMessage message){
    }
    
    public String[] getCommandData() {
        return this.data;
    }
    
    public String getCommandId() {
        return this.id;
    }
    
    /**
     * Returns a link message encapsulating the contents of the command, so
     * that it can be transported using the messaging infrastructure.
     */
    public JUMPMessage serialize() {
        return null;
    }
    
}
