/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * Handles service requests in AMS Isolate from client Isolates.
 */
final class SystemServiceRequestHandler {

    /** Named Link name: Link to send service requests data to client */
    final static String AMS_TO_CLIENT_LINK_NAME = 
        "AMS to client service request link";

    /** Named Link name: Link to receive service requests data from client */
    final static String CLIENT_TO_AMS_LINK_NAME = 
        "Client to AMS service request link";

    /** Service manager */
    private SystemServiceManager serviceManager = null;


    /**
     * Thread to handle service requests from single client Isolate.
     */
    class IsolateRequestHandlerThread extends Thread {

        /** Requests handler instance: handles requests from client Isolate */
        private IsolateSystemServiceRequestHandler requestHandler = null;

        /**
         * Constructor.
         *
         * @param requestHandler IsolateSystemServiceRequestHandler instance
         * for handling service requests from client Isolate
         */
        IsolateRequestHandlerThread(IsolateSystemServiceRequestHandler 
                requestHandler) {
            this.requestHandler = requestHandler;
        }

        /**
         * Thread body.
         */
        public void run() {
            // get a Link to listen for incoming service requests on
            SystemServiceConnectionLinks requestLinks = 
                requestHandler.getSendReceiveLinks();

            Link receiveLink = requestLinks.getReceiveLink();

            try {
                while (true) {
                    // listen on Link
                    LinkMessage msg = receiveLink.receive();

                    // handle service request
                    requestHandler.handleServiceRequest();
                }
            } catch (ClosedLinkException cle) {
                // do nothing and let the thread exit
            } catch (InterruptedIOException iioe) {
                requestLinks.close();
            } catch (IOException ioe) {
                requestLinks.close();
            }
        }
    }


    /**
     * Constructor.
     *
     * @param serviceManager service manager
     */
    SystemServiceRequestHandler(SystemServiceManager serviceManager) {
        this.serviceManager = serviceManager;
    }

    /**
     * Creates IsolateSystemServiceRequestHandler instance for
     * handling service requests from client Isolate.
     *
     * @param clientIsolate client Isolate
     * @return IsolateSystemServiceRequestHandler instance
     */
    IsolateSystemServiceRequestHandler newIsolateRequestHandler(
            Isolate clientIsolate) {

        // create IsolateSystemServiceRequestHandler instance
        IsolateSystemServiceRequestHandler requestHandler = null;
        requestHandler = new IsolateSystemServiceRequestHandler(
                serviceManager, clientIsolate);

        // ask it for Links used for requests negotiation
        SystemServiceConnectionLinks requestLinks = 
            requestHandler.getSendReceiveLinks();

        // make those Links available to client Isolate via NamedLinkPortal
        NamedLinkPortal.putLink(
                SystemServiceRequestHandler.AMS_TO_CLIENT_LINK_NAME,
                requestLinks.getSendLink());
        NamedLinkPortal.putLink(
                SystemServiceRequestHandler.CLIENT_TO_AMS_LINK_NAME,
                requestLinks.getReceiveLink());

        return requestHandler;
    }

    /**
     * Start handling service requests using specified requests handler.
     * It is assumed here that client Isolate has received Links from 
     * NamedLinkPortal.
     *
     * @param requestHandler requests handler to use
     */
    void startHandlingIsolateRequests(IsolateSystemServiceRequestHandler 
            requestHandler) {
        new IsolateRequestHandlerThread(requestHandler).start();
    }

}
