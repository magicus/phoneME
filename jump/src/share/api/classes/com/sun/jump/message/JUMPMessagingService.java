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

/**
 * <code>JUMPMessagingService</code> provides the methods for creating and
 * receiving messages. The interface is exposed by the appropriate 
 * singleton classes 
 * {@link com.sun.jump.executive.JUMPExecutive} and 
 * {@link com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess}
 * 
 */
public interface JUMPMessagingService {
    /**
     * Gets the message dispacther for the process
     */
    JUMPMessageDispatcher getMessageDispatcher ();

    /**
     * Creates a new, <i>blank</i> <Code>JUMPOutgoingMessage</code> for the
     * message type. The sender of the message is automatically filled
     * in by the factory implementation.
     */
    JUMPOutgoingMessage newOutgoingMessage (String mesgType);
    
    /**
     * Create a new, <i>blank</i> <code>JUMPOutgoingMessage</code> as a
     * response to the request message passed. The sender of the
     * message is automatically filled in by the factory
     * implementation.
     */
    JUMPOutgoingMessage newOutgoingMessage (JUMPMessage requestMessage);

    /**
     * Create a new, <i>immutable</i> <code>JUMPMessage</code>
     * out of a raw array of bytes received on the message queue.
     */
    JUMPMessage newMessage (byte[] rawData);

}
