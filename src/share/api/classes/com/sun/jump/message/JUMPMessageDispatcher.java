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

/**
 * <code>JUMPMessageDispatcher</code> is a high-level construct used to
 * read messages from underlying message queues and distribute them to
 * listeners.
 */
public interface JUMPMessageDispatcher {

    /**
     * Registers the message type for direct receipt via
     * <code>waitForMessage()</code>. This is akin to a reservation of
     * the type before blocking on incoming messages.
     *
     * @return
     * opaque object that represents the registration. The token can be
     * used to cancel the registaration using
     * {@link #cancelRegistration(Object)}
     * @throws JUMPMessageDispatcherTypeException if there is already an
     * existing handler registration for the message type.
     * @throws IOException if there is a problem registering for the
     * message type.
     */
    public Object registerDirect(String messageType) 
	throws JUMPMessageDispatcherTypeException, IOException;
    

    /**
     * Blocks and waits for a message of type <code>messageType</code>.
     * 
     * @throws JUMPMessageDispatcherTypeException if there is already
     * an existing handler registration for the message type, or if it has not
     * been registered via <code>registerDirect()</code>.
     *
     * @throws JUMPTimedOutException
     * @throws JUMPUnblockedException
     * @throws IOException
     */
    public JUMPMessage waitForMessage(String messageType, long timeout)
	throws JUMPMessageDispatcherTypeException, JUMPTimedOutException, IOException;
    
    /**
     * Registers the message handler for the message type.  This
     * starts a daemon thread which will persist until all
     * handlers for the message type have been cancelled.  
     *
     * @return
     * opaque object that represents the registration. The token can be
     * used to cancel the registaration using
     * {@link #cancelRegistration(Object)}
     *
     * @throws JUMPMessageDispatcherTypeException if the message type was
     * registered via <code>registerDirect()</code>.
     * @throws IOException if there is a problem registering for the
     * message type.
     */
    public Object registerHandler(String messageType,
				  JUMPMessageHandler handler) 
	throws JUMPMessageDispatcherTypeException, IOException;
    

    /**
     * Removes the registration for the message type. This applies to
     * direct registrations as well as handler registrations.
     *
     * @throws IOException if there is a problem canceling the
     * registration.
     * @throws IllegalStateException if the registrationToken
     * has already been canceled.
     */
    public void cancelRegistration(Object registrationToken)
	throws IOException;
}
