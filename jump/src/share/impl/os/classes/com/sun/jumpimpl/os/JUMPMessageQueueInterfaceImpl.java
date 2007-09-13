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

package com.sun.jumpimpl.os;

import java.io.IOException;

import com.sun.jump.message.JUMPMessagable;
import com.sun.jump.message.JUMPTimedOutException;
import com.sun.jump.message.JUMPUnblockedException;
import com.sun.jump.os.JUMPMessageQueueInterface;

/**
 * This subclass of <code>JUMPMessageQueueInterface</code> implements
 * JUMPMessageQueue for the linux OS.
 */
public class JUMPMessageQueueInterfaceImpl extends JUMPMessageQueueInterface {
    
    protected JUMPMessageQueueInterfaceImpl() {
    }
    
    /**
     * Get offset of non-OS data in message buffer 
     */
    public native int getDataOffset();

    /**
     * Send an asynchronous message to process pid
     *
     * @throws JUMPTargetNonexistentException
     * @throws JUMPWouldBlockException
     * @throws IOException
     */
    public native void sendMessageAsync(int pid,
					byte[] message,
					boolean isResponse)
	throws IOException;
    

    /**
     * Send a response to a process (return information in the message).
     *
     * @throws JUMPTargetNonexistentException
     * @throws JUMPWouldBlockException
     * @throws IOException
     */
    public native void sendMessageResponse(byte[] message,
					   boolean isResponse)
	throws IOException;
    
    /**
     * Send a synchronous message and get byte[] back from which
     * we can construct a JUMPMessage.
     *
     * @throws JUMPTimedOutException
     * @throws JUMPTargetNonexistentException
     * @throws JUMPWouldBlockException
     * @throws IOException
     */
    public native byte[] sendMessageSync(int pid,
					 byte[] message,
					 boolean isResponse,
					 long timeout)
	throws JUMPTimedOutException, IOException;

    /**
     * @throws JUMPTimedOutException
     * @throws JUMPUnblockedException
     * @throws IOException
     */
    public native byte[] receiveMessage(String messageType,
					long timeout) 
	throws JUMPTimedOutException, IOException;

    /*
     * Get return type for caller thread
     */
    public native String getReturnType();

    /**
     * To make sure that the OS structures exist for the type we
     * are listening to, before we start listening.
     */
    public native void reserve(String messageType)
	throws IOException;

    /**
     * Free low-level structures when we're done listening.
     */
    public native void unreserve(String messageType);

    /**
     * Causes a thread blocking in or about to block in
     * receiveMessage(messageType) to throw JUMPUnblockedException.
     */
    public native void unblock(String messageType)
	throws IOException;
}
