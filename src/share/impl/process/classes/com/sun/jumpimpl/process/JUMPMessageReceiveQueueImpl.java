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

package com.sun.jumpimpl.process;

import java.io.IOException;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageReceiveQueue;
import com.sun.jump.message.JUMPTimedOutException;

// XXX This will be a layer above the native code, and the native code
// should return some kind of handle for read queues.  I.e., this
// should replace JUMPMessageQueueInterfaceImpl, or something like
// that.  This is not thread-safe because close() has a race, but it
// doesn't have to be.  close() will either be called explicitly, or
// at finalize time.  As used by JUMPMessageDispatcherImpl, it will
// always be called explicitly.

/* package */ class JUMPMessageReceiveQueueImpl
    implements JUMPMessageReceiveQueue
{
    private volatile boolean closed = false;

    public static JUMPMessageReceiveQueueFactory createFactory ()
    {
	return new JUMPMessageReceiveQueueFactory () {
	    public JUMPMessageReceiveQueue createJUMPMessageReceiveQueue(
		String messageType)
	    {
		return new JUMPMessageReceiveQueueImpl(messageType);
	    }
	};
    }

    // Instantiable only via the factory.
    private JUMPMessageReceiveQueueImpl (String messageType)
    {
	// XXX native
    }

    // JUMPMessageReceiveQueue implementation.

    public JUMPMessage receiveMessage (long timeout)
	throws JUMPTimedOutException, IOException 
    {
	byte[] raw = receiveMessage0(timeout);
	return new MessageImpl.Message(raw);
    }

    public void close ()
    {
	if (!closed) {
	    closed = true;
	    // XXX Call native code to close.
	}
    }

    public void finalize ()
    {
	// Free our resources if they were not freed explicitly as
	// they should have been.
	close();
    }

    // XXX native
    private byte[] receiveMessage0 (long timeout)
    {
	return new byte[0];
    }
}
