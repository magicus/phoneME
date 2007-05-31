/*
 * %W% %E%
 *
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

package com.sun.jump.isolate.jvmprocess;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcess;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.message.JUMPMessagingService;
import java.util.Map;
import java.rmi.Remote;

/**
 * <code>JVMIsolateVM</code> encapsulates an isolate that is implemented
 * using a single JVM Process. There is only a single instance of this
 * class, so all the methods are static. This class also hosts all the 
 * common infrastucture services (like messaging, locking etc) for both
 * the system libraries as well as the application running within the
 * container.
 */
public abstract class JUMPIsolateProcess
    implements JUMPProcess, JUMPIsolate, JUMPMessagingService {
    
    private static JUMPIsolateProcess INSTANCE = null;
    
    public synchronized static JUMPIsolateProcess getInstance() {
        return INSTANCE;
    }
    
    /** Creates a new instance of JUMPIsolateProcess */
    protected JUMPIsolateProcess() {
        synchronized (JUMPIsolateProcess.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns a module's remote object.  The isolate can access
     * JUMP modules in the executive through the remote object.
     *
     * <p>
     * This method should be used to access implementation interfaces only.
     * </p>
     *
     * @param name name of service to retrieve
     * @return remote object which implements the remoteInterface.
     */
    public abstract Remote getRemoteService(String name);

    /**
     * Returns the executive process interface so that the Isolate
     * can send messages to the executive.
     */
    public abstract JUMPProcessProxy getExecutiveProcess();

    public int getIsolateId() {
        return this.getProcessId();
    }

    /**
     * Returns configuration parameters associated with the isolate.
     */
    public abstract Map getConfig();

    /**
     * Get app model running in this isolate process
     */
    public abstract JUMPAppModel getAppModel();

    public void initialize(JUMPAppModel appModel) {
    }
}
