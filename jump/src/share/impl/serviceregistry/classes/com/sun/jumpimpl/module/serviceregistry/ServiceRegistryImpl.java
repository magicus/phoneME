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

package com.sun.jumpimpl.module.serviceregistry;

import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModule;

import java.rmi.Remote;
import java.rmi.AlreadyBoundException;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.rmi.AccessException;

import javax.microedition.xlet.ixc.IxcRegistry;
import javax.microedition.xlet.ixc.StubException;

import java.util.Map;

public class ServiceRegistryImpl implements JUMPServiceRegistryModule {
    
    public void load(Map map) {}
    public void unload() {}

    private IxcRegistry registry;

    public ServiceRegistryImpl(IxcRegistry registry) {
       this.registry = registry;
    }

    /**
     * Registers a service under a given name to the registry, and make it
     * available for other JUMP processes.
     * A wrapper for IxcRegistry.bind(String, Remote)
     *
     * @throws AlreadyBoundException if the name is already bound to another service.
     * @throws RemoteException if the service object is not well-formed.
     **/
     public void registerService(String name, Remote service) 
        throws RemoteException, AlreadyBoundException {
           registry.bind(name, service);
     }

    /**
     * Removes a service with a given name from the registry.
     * A wrapper for IxcRegistry.unbind(String)
     *
     * @throws NotBoundException if the name is not found in this registry.
     **/
     public void unregisterService(String name) throws NotBoundException, AccessException {
          registry.unbind(name);
     }

    /**
     * Gets a service from the registry under a given name.
     * A wrapper for IxcRegistry.lookup(name)
     *
     * @throws NotBoundException if the name is not found in this registry.
     **/
     public Remote getService(String name) throws NotBoundException, RemoteException {
          return registry.lookup(name);
     }

    /**
     * Provides a list of service names currently available in this registry.
     * A wrapper for IxcRegistry.list()
     **/
     public String[] listRegisteredServices() {
          return registry.list();
     }

}
