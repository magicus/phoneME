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

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageResponseSender;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPTimedOutException;
import java.io.IOException;
import com.sun.jump.command.JUMPCommand;
import com.sun.jump.command.JUMPRequest;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.command.JUMPResponseInteger;

/**
 * Helper class to handle request/response exchange.
 */
public class RequestSenderHelper {
    // FIXME: Timeout values should be centralized somewhere
    private static final long DEFAULT_TIMEOUT = 5000L;

    private JUMPMessagingService host; // either isolate or executive

    public RequestSenderHelper(JUMPMessagingService host) {
        this.host = host;
    }

    public boolean
    handleBooleanResponse(JUMPResponse response) {
        String id = null;
        if(response != null) {
            id = response.getCommandId();
	    if (id.equals(JUMPResponse.ID_SUCCESS)) {
		return true;
	    } else if (id.equals(JUMPResponse.ID_FAILURE)) {
		return false;
	    }
        }

        throw new IllegalArgumentException(); // FIXME: throw exception or System.exit(1)?
    }

    private int
    handleIntegerResponse(JUMPResponseInteger rint) {
	String id = rint.getCommandId();
	if (id.equals(JUMPResponseInteger.ID_SUCCESS)) {
	    // Return the data in the message
	    return rint.getInt();
	} else if (id.equals(JUMPResponse.ID_FAILURE)) {
	    return -1;
	}

        throw new IllegalArgumentException(); // FIXME: throw exception or System.exit(1)?
    }

    /**
     * Send outgoing request and get a response message
     */
    private JUMPMessage
    sendRequestWork(JUMPMessageSender target, JUMPRequest request) {
        try {
            JUMPOutgoingMessage m = request.toMessage(host);
            JUMPMessage         r = target.sendMessage(m, DEFAULT_TIMEOUT);

	    return r;
        } catch(JUMPTimedOutException e) {
            e.printStackTrace();
        } catch(IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Send outgoing request and get a response
     */
    public JUMPCommand
    sendRequest(
        JUMPMessageSender target, JUMPRequest request, Class responseClass) {
        
	JUMPMessage r = sendRequestWork(target, request);
	if (r == null) {
	    return null;
	}
	return JUMPCommand.fromMessage(r, responseClass);
    }
    
     /**
     * Send outgoing request and get a response
     */
    public JUMPResponse
    sendRequest(JUMPMessageSender target, JUMPRequest request) {
	return (JUMPResponse)sendRequest(target, request, JUMPResponse.class);
    }


    /**
     * Send outgoing request and get a response
     */
    public int
    sendRequestWithIntegerResponse(JUMPMessageSender target, 
				   JUMPRequest request) {
	JUMPMessage r = sendRequestWork(target, request);
	if (r == null) {
	    return -1;
	}
	JUMPResponseInteger rint = 
	    (JUMPResponseInteger)JUMPResponseInteger.fromMessage(r);
	return handleIntegerResponse(rint);
    }

    /**
     * Send async request
     */
    public void
    sendRequestAsync(JUMPMessageSender target, JUMPRequest request) {
        try {
            JUMPOutgoingMessage m = request.toMessage(host);
            target.sendMessage(m);
        } catch(IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Send boolean response to incoming request
     */
    public void
    sendBooleanResponse(JUMPMessage incoming, boolean value) {
        try {
            JUMPOutgoingMessage m = host.newOutgoingMessage(incoming);
	    JUMPMessageResponseSender mrs = incoming.getSender();

            m.addUTF(
                value ? JUMPResponse.ID_SUCCESS : JUMPResponse.ID_FAILURE);
            mrs.sendResponseMessage(m);
        } catch(IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Send response to incoming request
     */
    public void
    sendResponse(JUMPMessage incoming, JUMPResponse value) {
        try {
            JUMPOutgoingMessage m = value.toMessageInResponseTo(incoming, host);
	    JUMPMessageResponseSender mrs = incoming.getSender();

            mrs.sendResponseMessage(m);
        } catch(IOException e) {
            e.printStackTrace();
        }
    }
}
