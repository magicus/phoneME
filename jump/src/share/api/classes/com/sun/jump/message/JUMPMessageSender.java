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
package com.sun.jump.message;

import java.io.IOException;
import com.sun.jump.message.JUMPTimedOutException;

/**
 * <code>JUMPMessageSender</code> defines the interface for an object
 * that can forward messages to a target it represents.
 *
 * A <code>JUMPMessageSender</code> can handle both synchronous and
 * asynchronous calls.
 */
public interface JUMPMessageSender extends JUMPMessageResponseSender {
    /**
     * Send a message and wait for a response message.
     *
     * @param message the message to be sent
     * @param timeout the time in milliseconds to wait, if there was no
     *        response.
     */
    public JUMPMessage sendMessage(JUMPOutgoingMessage message, long timeout)
        throws JUMPTimedOutException, IOException;


    /**
     * Send an asynchronous message.
     *
     * @param message the message to be sent
     */
    public void sendMessage(JUMPOutgoingMessage message) throws IOException;
}
