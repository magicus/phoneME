/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.ams.service;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.services.*;

/**
 * Implements application management service
 */
class TaskManagerService implements SystemService  {

    /** Our service ID */
    final static String SERVICE_ID = "com.sun.midp.ams.service.task_manager";

    /** Need token to access MIDP privileged service */
    private SecurityToken token;

    /** Constructor */
    TaskManagerService(SecurityToken t) {
        token = t;
    }

    /**
     * Gets unique service identifier.
     *
     * @return unique String service identifier
     */
    public String getServiceID() {
        return SERVICE_ID;
    }

    /**
     * Starts service. Called when service is about to be
     * requested for the first time.
     */
    public void start() {
    }

    /**
     * Shutdowns service.
     */
    public void stop() {
    }

    /**
     * Accepts connection. When client requests a service, first,
     * a connection between client and service is created, and then
     * it is passed to service via this method to accept it and
     * start doing its thing. Note: you shouldn't block in this
     * method.
     *
     * @param theConnection connection between client and service
     */
    public void acceptConnection(SystemServiceConnection theConnection) {
        /* Process data at separate thread. */
        TaskManagerServiceProtocol.process(token, theConnection);
    }

}