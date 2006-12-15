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
import com.sun.jump.command.JUMPRequest;
import com.sun.jump.command.JUMPResponse;


/**
 * Helper class to handle request/response exchange.
 */
public class RequestSenderHelper {
    private static final long DEFAULT_TIMEOUT = 0L;

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

    /**
     * Send outgoing request and get a response
     */
    public JUMPResponse
    sendRequest(JUMPMessageSender target, JUMPRequest request) {
        JUMPResponse response = null;
        try {
            JUMPOutgoingMessage m = request.toMessage(host);
            JUMPMessage         r = target.sendMessage(m, DEFAULT_TIMEOUT);

            response = JUMPResponse.fromMessage(r);
        } catch(JUMPTimedOutException e) {
            e.printStackTrace();
        } catch(IOException e) {
            e.printStackTrace();
        }
        return response;
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
    sendBooleanResponse(JUMPMessage incoming,
			boolean booleanResponse) {
        try {
            JUMPOutgoingMessage m = host.newOutgoingMessage(incoming);
	    JUMPMessageResponseSender mrs = incoming.getSender();
            mrs.sendResponseMessage(m);
        } catch(IOException e) {
            e.printStackTrace();
        }
    }

}
