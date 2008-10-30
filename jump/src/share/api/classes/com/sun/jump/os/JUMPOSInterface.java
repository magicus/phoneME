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
package com.sun.jump.os;

/**
 * <code>JUMPOSInterface</code> needs to be implemented to fulfill
 * the OS dependencies in the JUMP system.
 */
public abstract class JUMPOSInterface {
    private static JUMPOSInterface INSTANCE = null;
    
    public static synchronized JUMPOSInterface getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPOSInterface
     */
    protected JUMPOSInterface() {
        synchronized (JUMPOSInterface.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }

    /**
     * Set testing mode. A "TESTING_MODE" message is sent to the
     * server.
     * @param filePrefix tells the server where the stdout and 
     * stderr output files should go.
     */
    public abstract void setTestingMode(String filePrefix);
    
    /**
     * Create a process and return a unique integer ID designating it.
     */
    public abstract int createProcess(String[] args);

    /**
     * Create a native process and return a unique integer ID designating it.
     */
    public abstract int createProcessNative(String[] args);
    
    /**
     * Return the current process ID.
     */
    public abstract int getProcessID();

    /**
     * Return the executive's process ID.
     */
    public abstract int getExecutiveProcessID();

    /**
     * Get the message queue interface
     */
    public abstract JUMPMessageQueueInterface getQueueInterface();
    
}
