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

/**
 * <code>JUMPMessageDispatcher</code> is a high-level construct used to
 * read messages from the underlying <code>JUMPIncomingQueue</code> and dispatches
 * it to the list of handlers registered for the message type. 
 * This must be the only class that performs
 * <code>JUMPIncomingQueue.readMessage()</code> in a single JVM.
 */
public class JUMPMessageDispatcher {
    private static JUMPMessageDispatcher instance = null;
    public static synchronized JUMPMessageDispatcher getInstance() {
        if ( instance == null )
            instance = new JUMPMessageDispatcher();
        return instance;
    }
    
    /**
     * Creates a new instance of JUMPMessageDispatcher
     */
    protected JUMPMessageDispatcher() {
    }
    
    /**
     * Registers the message handler for the message type. Returns 
     * 
     * @return 
     * opaque object that represents the registration. The token can be 
     * used to cancel the registaration using 
     * {@link #cancelRegistration(Object)}
     */
    public Object registerHandler(String messageType, 
        JUMPMessageHandler handler){
        return null;
    }
    
    /**
     * Registers the handler for a reponse for the <i>message</i> passed.
     * The implementation would examine all received messages and 
     * examine if the response message id matches the requestMessage's
     * id and call the handler if there is a match.
     * 
     * @return 
     * opaque object that represents the registration. The token can be 
     * used to cancel the registaration using 
     * {@link #cancelRegistration(Object)}
     */
    public Object registerHandler(JUMPMessage requestMessage, 
        JUMPMessageHandler handler) {
        return null;
    }
    
    /**
     * Removes the registration of the message handler for the message 
     * type.
     */
    public void cancelRegistration(Object registrationToken) {
        
    }
}
