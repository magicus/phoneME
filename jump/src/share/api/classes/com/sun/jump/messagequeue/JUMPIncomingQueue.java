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

import java.io.IOException;

/**
 * <code>JUMPIncomingQueue</code> defines the interface for receiving message.
 */
public interface JUMPIncomingQueue {
    /**
     * Returns the next message from the inbox if one is available or 
     * waits (till the timeout) for the arrival of one.
     * 
     * @param timeout timeout value in milli seconds. A value of 0L indicates
     *        to wait forever.
     * @throws com.sun.jump.mailbox.JUMPJUMPTimedOutException there was no
     *         message in the received within the timeout specified.
     * @throws java.io.IOException if there was any I/O error reading from
     *         the inbox.
     */
    public JUMPMessage receiveMessage(long timeout) 
        throws JUMPTimedOutException, IOException ;
}
