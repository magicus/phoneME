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

import com.sun.midp.services.*;
import java.io.*;
import com.sun.midp.security.*;

/**
 * Client side of AutoTester service data exchange protocol.
 */
public final class TaskManagerServiceClient {

    /** Connection between service and client */
    private SystemServiceConnection con;

    /**
     * Establishes connection to AutoTester service.
     *
     * @param token security token
     * @return AutoTesterServiceProtocolAMS instance if connection
     * has been established, null otherwise
     */
    public static TaskManagerServiceClient connectToService(SecurityToken token) {
        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = serviceRequestor.requestService(
                TaskManagerService.SERVICE_ID);
        if (con == null) {
            return null;
        }

        return new TaskManagerServiceClient(con);
    }

    public TaskInfo[] getTaskList() {
        /* TODO: need security check */
        return TaskManagerServiceProtocol.getTaskList(con);
    }

    /**
     * Constructor.
     *
     * @param theConnection AutoTester service connection
     */
    private TaskManagerServiceClient(SystemServiceConnection 
            theConnection) {
        con = theConnection;
    }    
}

