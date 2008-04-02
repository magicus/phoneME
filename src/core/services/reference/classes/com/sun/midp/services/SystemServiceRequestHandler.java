/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.services;

import com.sun.cldc.isolate.*;
import com.sun.midp.links.*;
import java.io.*;

final class SystemServiceRequestHandler {
    final static String SERVICE_TO_CLIENT_LINK_NAME = 
        "Service to client service request link";

    final static String CLIENT_TO_SERVICE_LINK_NAME = 
        "Client to service service request link";

    private SystemServiceManager serviceManager = null;

    class IsolateRequestHandlerThread extends Thread {
        private IsolateSystemServiceRequestHandler requestHandler = null;

        IsolateRequestHandlerThread(IsolateSystemServiceRequestHandler 
                requestHandler) {
            this.requestHandler = requestHandler;
        }

        public void run() {
            SystemServiceConnectionLinks requestLinks = 
                requestHandler.getSendReceiveLinks();

            Link receiveLink = requestLinks.getReceiveLink();

            try {
                while (true) {
                    LinkMessage msg = receiveLink.receive();
                    requestHandler.handleServiceRequest();
                }
            } catch (ClosedLinkException cle) {
                // do nothing
            } catch (InterruptedIOException iioe) {
                requestLinks.close();
            } catch (IOException ioe) {
                requestLinks.close();
            }
        }
    }

    SystemServiceRequestHandler(SystemServiceManager serviceManager) {
        this.serviceManager = serviceManager;
    }

    IsolateSystemServiceRequestHandler newIsolateRequestHandler(
            Isolate clientIsolate) {

        IsolateSystemServiceRequestHandler requestHandler = null;
        requestHandler = new IsolateSystemServiceRequestHandler(
                serviceManager, clientIsolate);
        SystemServiceConnectionLinks requestLinks = 
            requestHandler.getSendReceiveLinks();

        NamedLinkPortal.putLink(
                SystemServiceRequestHandler.SERVICE_TO_CLIENT_LINK_NAME,
                requestLinks.getSendLink());
        NamedLinkPortal.putLink(
                SystemServiceRequestHandler.CLIENT_TO_SERVICE_LINK_NAME,
                requestLinks.getReceiveLink());

        return requestHandler;
    }

    void handleIsolateRequests(IsolateSystemServiceRequestHandler 
            requestHandler) {
        new IsolateRequestHandlerThread(requestHandler).start();
    }

}
