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
package com.sun.jumpimpl.process;

import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageResponseSender;
import com.sun.jump.message.JUMPMessagable;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPTimedOutException;

/**
 * Process oriented messages, assuming JUMPProcessProxyImpl message peers.
 */
public class MessageImpl {
    public static class Message extends JUMPMessage {
	private int senderPid;
	
	public Message(byte[] rawBytes) {
	    super(rawBytes);
	}
	
	protected void readMessageSender(int id) {
	    this.senderPid = id;
	}
	
	protected JUMPMessageResponseSender getMessageSender() {
	    return JUMPProcessProxyImpl.getProcessProxyImpl(this.senderPid);
	}
    }

    public static class OutgoingMessage extends JUMPOutgoingMessage {
	protected JUMPMessageResponseSender getMessageSender() {
	    return (JUMPMessageResponseSender)sender;
	}
	
        public OutgoingMessage(JUMPMessagable sender,
			       String type,
			       String returnType,
			       int responseId) {
            super(sender, type, returnType, responseId);
        }

	protected int serializeMessagable(JUMPMessagable messagable) {
	    JUMPProcessProxyImpl ppi = (JUMPProcessProxyImpl)messagable;
	    return ppi.getProcessId();
	}
    }
}
