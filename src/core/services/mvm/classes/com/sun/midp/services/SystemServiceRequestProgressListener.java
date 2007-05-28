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

/**
 * Internal interface for communication between AMS part of service 
 * request protocol and whatever entity is communicating with it. 
 * It allows us to i3test service request protocol independently.
 */
interface SystemServiceRequestProgressListener {
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
    public SystemServiceConnectionLinks onServiceRequest(String serviceID);    

    /**
     * Callback method. Called by protocol after pair of Links created on 
     * previous step have been passed to client. 
     *
     * @param connectionLinks pair of Links for data exhange between 
     * service and its client
     */
    public void onLinksPassedToClient(SystemServiceConnectionLinks 
            connectionLinks);
}
