/*
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

package com.sun.midp.push.persistence;

import java.io.IOException;
import java.util.Map;

import com.sun.midp.push.controller.ConnectionInfo;


public class ProxyStore implements Store {
    
    private final Store impl;

    protected Store
    getImpl() {
        return impl;
    }

    public ProxyStore(Store impl) {
        this.impl = impl;
    }

    public void listConnections(ConnectionsConsumer connectionsConsumer) {
        impl.listConnections(connectionsConsumer);
    }

    public void addConnection(int midletSuiteID,
            ConnectionInfo connection) throws IOException {
        impl.addConnection(midletSuiteID, connection);
    }

    public void addConnections(int midletSuiteID,
            ConnectionInfo [] connections) throws IOException {
        impl.addConnections(midletSuiteID, connections);
    }

    public void removeConnection(int midletSuiteID,
            ConnectionInfo connection) throws IOException {
        impl.removeConnection(midletSuiteID, connection);
    }

    public void removeConnections(int midletSuiteID) throws IOException {
        impl.removeConnections(midletSuiteID);
    }

    public void listAlarms(AlarmsConsumer alarmsConsumer) {
        impl.listAlarms(alarmsConsumer);
    }

    public void addAlarm(int midletSuiteID, String midlet, long time) 
            throws IOException {
        impl.addAlarm(midletSuiteID, midlet, time);
    }

    public void removeAlarm(int midletSuiteID, String midlet) 
            throws IOException {
        impl.removeAlarm(midletSuiteID, midlet);
    }
}
