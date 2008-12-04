/*
 * @(#)Registry.java	1.8 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package java.rmi.registry;

import java.rmi.*;

/**
 * For obtaining references to remote objects, RMI provides a simple
 * remote object registry interface, implemented by RMI's
 * <code>rmiregistry</code>, that provides methods for storing and retrieving
 * remote object references.  The <code>java.rmi.Naming</code> class
 * provides methods to access a remote object registry using URL-formatted
 * names to specify in a compact format both the remote registry along
 * with the name for the remote object.
 *
 * <p>Typically a "registry" exists on every node that allows RMI connections
 * to servers on that node.  A registry on a particular node contains a
 * transient database that maps names to remote objects.  When a registry
 * starts up, the registry database is empty.  The names stored in the
 * registry are pure and are not parsed.  A service storing itself in the
 * registry may want to prefix its name of the service by a package name
 * (although not required), to reduce name collisions in the registry.
 *
 * <p>To create a registry that runs in an application, use one of the
 * <code>LocateRegistry.createRegistry</code> methods.  To obtain a reference
 * to a remote object registry, use one of the
 * <code>LocateRegistry.getRegistry</code> methods.
 *
 * @version 1.13, 02/02/00
 * @author  Ann Wollrath
 * @since   JDK1.1
 * @see     java.rmi.Naming
 * @see     java.rmi.registry.LocateRegistry
 */
public interface Registry extends Remote {
    /** Well known port for registry. */
    public static final int REGISTRY_PORT = 1099;
    /**
     * Returns a reference, a stub, for the remote object associated
     * with the specified <code>name</code>.
     *
     * @param name a URL-formatted name for the remote object
     * @return a reference for a remote object
     * @exception NotBoundException if name is not currently bound
     * @exception RemoteException if registry could not be contacted
     * @exception AccessException if this operation is not permitted (if
     * originating from a non-local host, for example)
     * @since JDK1.1
     */
    public Remote lookup(String name)
        throws RemoteException, NotBoundException, AccessException;
    /**
     * Binds the specified <code>name</code> to a remote object.
     *
     * @param name a URL-formatted name for the remote object
     * @param obj a reference for the remote object (usually a stub)
     * @exception AlreadyBoundException if name is already bound
     * @exception MalformedURLException if the name is not an appropriately
     *  formatted URL
     * @exception RemoteException if registry could not be contacted
     * @exception AccessException if this operation is not permitted (if
     * originating from a non-local host, for example)
     * @since JDK1.1
     */
    public void bind(String name, Remote obj)
        throws RemoteException, AlreadyBoundException, AccessException;
    /**
     * Destroys the binding for the specified name that is associated
     * with a remote object.
     *
     * @param name a URL-formatted name associated with a remote object
     * @exception NotBoundException if name is not currently bound
     * @exception MalformedURLException if the name is not an appropriately
     *  formatted URL
     * @exception RemoteException if registry could not be contacted
     * @exception AccessException if this operation is not permitted (if
     * originating from a non-local host, for example)
     * @since JDK1.1
     */
    public void unbind(String name)
        throws RemoteException, NotBoundException, AccessException;
    /** 
     * Rebinds the specified name to a new remote object. Any existing
     * binding for the name is replaced.
     *
     * @param name a URL-formatted name associated with the remote object
     * @param obj new remote object to associate with the name
     * @exception MalformedURLException if the name is not an appropriately
     *  formatted URL
     * @exception RemoteException if registry could not be contacted
     * @exception AccessException if this operation is not permitted (if
     * originating from a non-local host, for example)
     * @since JDK1.1
     */
    public void rebind(String name, Remote obj)
        throws RemoteException, AccessException;
    /**
     * Returns an array of the names bound in the registry.  The names are
     * URL-formatted strings. The array contains a snapshot of the names
     * present in the registry at the time of the call.
     *
     * @return an array of names (in the appropriate URL format) bound
     *  in the registry
     * @exception RemoteException if registry could not be contacted
     * @exception AccessException if this operation is not permitted (if
     * originating from a non-local host, for example)
     * @since JDK1.1
     */
    public String[] list()
        throws RemoteException, AccessException;
}
