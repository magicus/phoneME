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

package com.sun.jump.module.serviceregistry;

import com.sun.jump.module.JUMPModule;

import java.rmi.Remote;
import java.rmi.AlreadyBoundException;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.rmi.AccessException;

/**
 * <code>JUMPServiceRepository</code> represents a registry of services
 * in which processes in JUMP system can register and let others to use
 * in the form of lightweight remote method invocation (RMI).
 */

public interface JUMPServiceRegistryModule extends JUMPModule {

    /**
     * Registers a service under a given name to the registry, and make it
     * available for other JUMP processes.
     * A wrapper for IxcRegistry.bind(String, Remote)
     *
     * @throws AlreadyBoundException if the name is already bound to another service.
     * @throws RemoteException if the service object is not well-formed.
     **/
     public void registerService(String name, Remote service) 
        throws RemoteException, AlreadyBoundException; 

    /**
     * Removes a service with a given name from the registry.
     * A wrapper for IxcRegistry.unbind(String)
     *
     * @throws NotBoundException if the name is not found in this registry.
     * @throws AccessException if the name is bound not by the caller.
     **/
     public void unregisterService(String name) throws NotBoundException, AccessException; 

    /**
     * Gets a service from the registry under a given name.
     * A wrapper for IxcRegistry.lookup(name)
     *
     * @throws NotBoundException if the name is not found in this registry.
     * @throws RemoteException if the service retrieval fails.
     **/
     public Remote getService(String name) throws NotBoundException, RemoteException;

    /**
     * Provides a list of service names currently available in this registry.
     * A wrapper for IxcRegistry.list()
     **/
     public String[] listRegisteredServices(); 

}
