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
 * Handles service requests coming from single Isolate.
 * It also implements internal SystemServiceRequestProgressListener
 * interface.
 */
final class IsolateSystemServiceRequestHandler 
    implements SystemServiceRequestProgressListener {

    /** Service provider Isolate */
    private Isolate serviceIsolate = null;

    /** Isolate to handle service requests from */
    private Isolate clientIsolate = null;

    /** Service manager */
    private SystemServiceManager serviceManager = null;

    /** When request is in progress, holds requested service */
    private SystemService requestedService = null;

    /** Pair of Links for service request negotiation */
    private SystemServiceConnectionLinks sendReceiveLinks = null;

    /** Protocol which does service request negotiation */
    private SystemServiceRequestProtocolAMS serviceRequestProtocol = null;


    /**
     * Constructor.
     *
     * @param serviceManager service manager
     * @clientIsolate Isolate to handle service requests from
     */
    IsolateSystemServiceRequestHandler(SystemServiceManager serviceManager, 
            Isolate clientIsolate) {

        this.serviceIsolate = Isolate.currentIsolate();
        this.clientIsolate = clientIsolate;
        this.serviceManager = serviceManager;

        // create pair of Links between Isolates for requests negotiation
        Link sendLink = Link.newLink(serviceIsolate, clientIsolate);
        Link receiveLink = Link.newLink(clientIsolate, serviceIsolate);      
        this.sendReceiveLinks = new SystemServiceConnectionLinks(
                sendLink, receiveLink);

        // create protocol
        serviceRequestProtocol = new SystemServiceRequestProtocolAMS(this);
    }

    /**
     * Gets pair of Links for requests negotiation.
     *
     * @return SystemServiceConnectionLinks object incapsulating pair of Links
     * for requests negotiation
     */
    SystemServiceConnectionLinks getSendReceiveLinks() {
        return sendReceiveLinks;
    }

    /**
     * Handles service request.
     */
    void handleServiceRequest()
        throws ClosedLinkException, 
               InterruptedIOException, 
               IOException {

        // ask protocol to do the negotiation
        serviceRequestProtocol.handleServiceRequest(sendReceiveLinks);
    }

    /***
     * Callback method. Called by protocol after it recieves service ID
     * from client. Protocol expects us to request service from Service 
     * Manager and create pair of Links for data exchange between service
     * and its client.
     *
     * @param serviceID service ID
     * @return SystemServiceConnectionLinks object incapsulating pair of Links 
     * for data exchange between service and its client. If Service Manager is
     * unable to fulfill service request, null is returned
     */
    public SystemServiceConnectionLinks onServiceRequest(String serviceID) {
        // request service from service manager
        requestedService = serviceManager.getService(serviceID);
        if (requestedService == null) {
            return null;
        }

        // create pair of links for data exhange between service and its client
        Link serviceToClient = Link.newLink(serviceIsolate, clientIsolate);
        Link clientToService = Link.newLink(clientIsolate, serviceIsolate);
        SystemServiceConnectionLinks connectionLinks = 
            new SystemServiceConnectionLinks(serviceToClient, clientToService);

        return connectionLinks;
    }

    /**
     * Callback method. Called by protocol after pair of Links created on 
     * previous step have been passed to client. 
     *
     * @param connectionLinks pair of Links for data exhange between 
     * service and its client
     */
    public void onLinksPassedToClient(SystemServiceConnectionLinks 
            connectionLinks) {

        if (connectionLinks == null || requestedService == null) {
            throw new IllegalStateException();
        }

        // create SystemServiceConnection instance from this pair of Links
        SystemServiceConnection serviceConnection = 
            new SystemServiceConnectionImpl(connectionLinks);
            
        // tell service to accept connection
        synchronized (requestedService) {
            requestedService.acceptConnection(serviceConnection);
        }

        requestedService = null;
    }
}
