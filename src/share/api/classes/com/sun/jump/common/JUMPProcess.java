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

package com.sun.jump.common;

import com.sun.jump.messagequeue.JUMPIncomingQueue;
import com.sun.jump.messagequeue.JUMPMessagable;
import com.sun.jump.messagequeue.JUMPMessage;
import com.sun.jump.messagequeue.JUMPOutgoingQueue;

/**
 * <code>JUMPProcess</code> abstracts an OS process which runs the JUMP 
 * components.
 */
public interface JUMPProcess extends JUMPMessagable {
    public int getProcessId();
    
    /**
     * A messagable component has a queue for incoming messages
     */
    public JUMPIncomingQueue getIncomingQueue();

    /**
     * A messagable component has a queue for outgoing messages
     */
    public JUMPOutgoingQueue getOutgoingQueue();
    
    /**
     * Creates a new <Code>JUMPMessage</code> for the message type and the
     * data (payload) specified. The sender of the message is automatically
     * filled in by the <Code>JUMPProcess</code> implementation.
     * 
     * @throws java.lang.IllegalArgumentException if the data is not a
     *         byte[], String or java.io.Serializable.
     */
    public JUMPMessage newMessage(String mesgType, Object data);
    
    /**
     * Create a new <code>JUMPMessage</code> as a response to the request
     * message passed. The sender of the message is automatically
     * filled in by the <Code>JUMPProcess</code> implementation.
     */
    public JUMPMessage newMessage(JUMPMessage requestMessage,
        String mesgType, Object data);
}
