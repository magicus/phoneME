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
package com.sun.jumpimpl.os;

import com.sun.jump.os.JUMPOSInterface;
import com.sun.jump.os.JUMPMessageQueueInterface;

/**
 * <code>JUMPOSInterface</code> needs to be implemented to fulfill
 * the OS dependencies in the JUMP system.
 */
public class JUMPOSInterfaceImpl extends JUMPOSInterface {
    public JUMPOSInterfaceImpl() {
	super();
    }

    /**
     * Set testing mode. A "TESTING_MODE" message is sent to the
     * server. The 'filePrefix' argument tells the server where
     * the stdout and stderr output files should go.
     */
    public native void setTestingMode(String filePrefix);
    
    /**
     * Shutdown the server
     */
    public native void shutdownServer();

    /**
     * Create a process and return a unique integer ID designating it.
     */
    public native int createProcess(String[] args);

    /**
     * Create a native process and return a unique integer ID designating it.
     */
    public native int createProcessNative(String[] args);

    /**
     * Return the current process ID.
     */
    public native int getProcessID();

    /**
     * Return the executive process ID.
     */
    public native int getExecutiveProcessID();

    /**
     * Get the message queue interface
     */
    public JUMPMessageQueueInterface getQueueInterface() {
	return new JUMPMessageQueueInterfaceImpl();
    }
}
