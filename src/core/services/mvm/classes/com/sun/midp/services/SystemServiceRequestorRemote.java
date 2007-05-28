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

import com.sun.midp.links.*;
import java.io.*;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;


/**
 * Implements SystemServiceRequestor interface for the case
 * when client is not in AMS Isolate and thereofre cross 
 * Isolates communication is reqyuired.
 */
final class SystemServiceRequestorRemote extends SystemServiceRequestor {

    /** Pair of Links between AMS and this Isolate for request negotiation */  
    private SystemServiceConnectionLinks sendReceiveLinks = null;

    /** Protocol which does request negotiation */
    private SystemServiceRequestProtocolClient requestProtocol = null;

    
    /**
     * Constructor.
     *
     * @param sendReceiveLinks pair of Links between AMS and this 
     * Isolate for request negotiation
     */
    SystemServiceRequestorRemote(
            SystemServiceConnectionLinks sendReceiveLinks) {

        this.sendReceiveLinks = sendReceiveLinks;
        if (sendReceiveLinks != null) {
            this.requestProtocol = 
                new SystemServiceRequestProtocolClient(sendReceiveLinks);
        }
    }

    /**
     * Gets new instance.
     *
     * @param token security token
     * @return new instance
     */
    static SystemServiceRequestorRemote newInstance(SecurityToken token) {
        token.checkIfPermissionAllowed(Permissions.MIDP);

        Link receiveLink = NamedLinkPortal.getLink(
                SystemServiceRequestHandler.AMS_TO_CLIENT_LINK_NAME);
        Link sendLink = NamedLinkPortal.getLink(
                SystemServiceRequestHandler.CLIENT_TO_AMS_LINK_NAME);
        SystemServiceConnectionLinks requestLinks = null;
        requestLinks = new SystemServiceConnectionLinks(sendLink, receiveLink);

        return new SystemServiceRequestorRemote(requestLinks);
    }

    /**
     * Establishes connection to service.
     *
     * @param serviceID unique service ID
     * @return connection to service, 
     * or null if some reasons service request has failed (for example, 
     * there is no such service registered)
     */
    public SystemServiceConnection requestService(String serviceID) {
        synchronized (this) {
            return doRequestService(serviceID);
        }
    }

    /**
     * Really establishes connection to service.
     *
     * @param serviceID unique service ID
     * @return connection to service, 
     * or null if some reasons service request has failed (for example, 
     * there is no such service registered)
     */    
    private SystemServiceConnection doRequestService(String serviceID) {
        if (sendReceiveLinks == null) {
            return null;
        }

        /* 
         * In case of exception we are going to panic and close 
         * negotiation Links because something is wrong with them
         */
        try {
            // send empty message to kick a session 
            Link sendLink = sendReceiveLinks.getSendLink();
            LinkMessage emptyMsg = LinkMessage.newStringMessage("");
            sendLink.send(emptyMsg);

            SystemServiceConnectionLinks connectionLinks = 
                requestProtocol.requestService(serviceID);

            if (connectionLinks != null) {
                return new SystemServiceConnectionImpl(connectionLinks);
            } else {
                return null;
            }
        } catch (ClosedLinkException e) {
            sendReceiveLinks.close();
            sendReceiveLinks = null;

            return null;
        } catch (InterruptedIOException e) {
            sendReceiveLinks.close();
            sendReceiveLinks = null;

            return null;
        } catch (IOException e) {
            sendReceiveLinks.close();
            sendReceiveLinks = null;

            return null;            
        } catch (IllegalStateException e) {
            sendReceiveLinks.close();
            sendReceiveLinks = null;

            return null;            
        }
    }
}
