/*
 * @(#)IxcRegistryImpl.java	1.25 05/05/10
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

package com.sun.xlet.ixc;

import javax.microedition.xlet.ixc.*;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.AccessException;
import java.rmi.NotBoundException;
import java.rmi.AlreadyBoundException;
import java.rmi.registry.Registry;
import javax.microedition.xlet.XletContext;
import java.util.Hashtable;
import java.util.Enumeration;
import java.util.Vector;

import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.security.PrivilegedActionException;


/**
 * <code>IXCRegistry</code> is the bootstrap mechanism for obtaining
 * references to remote objects residing in other Xlets executing on
 * the same machine, but in separate classloaders.
 *  * <p>
 * Instances of <code>IXCRegistry</code> are never accessible via
 * <code>java.rmi.Naming</code> or
 * <code>java.rmi.registry.LocateRegistry</code> if RMI functionality
 * is implemented.
 *
 * @see java.rmi.Registry
 */

public class IxcRegistryImpl extends IxcRegistry {
    // Hashtable<XletContext, IxcRegistry>
    static Hashtable registries = new Hashtable(11);

    // Hashtable<Name, XletContext>, who exported what names.
    static Hashtable xletContextMap = new Hashtable(11);
    // Hashtable<Name, RemoteObject>, who exported what Remote objects.
    static Hashtable remoteObjectMap = new Hashtable(11);

    // Lock object to synchronize access to
    // remoteObjectMap and xletContextMap.
    // We don't want Name to be appearing in one but not in another.
    static private Object lock = new Object();

    /* A thread for Remote method invocation */
    private static Worker worker;

    /* Xlet which this IxcRegistry belongs to*/
    private XletContext context;
    /* IxcClassLoader for this xlet */
    IxcClassLoader ixcClassLoader;
    /* ImportRegistries for this xlet.
       <target xlet's XletContext, ImportRegis> */
    Hashtable importRegistries = new Hashtable(11);

    AccessControlContext acc;

    /**
     * Creates a IxcRegistry instance.
     */
    protected IxcRegistryImpl(XletContext ctxt) {
        context = ctxt;

        final ClassLoader finalLoader = ctxt.getClassLoader();
        ixcClassLoader = (IxcClassLoader) AccessController.doPrivileged(
           new PrivilegedAction() {
              public Object run() {
                  return new IxcClassLoader(finalLoader);
              }
           }
        );

        acc = AccessController.getContext();

        synchronized(registries) {
           if (worker == null)  // first time caller!
               worker = new Worker("Remote invocation thread for IxcRegistry");
        }
    }

    /**
     * Gets the IxcRegistry
     */
    public static IxcRegistryImpl getIxcRegistryImpl(XletContext ctxt) {

       IxcRegistryImpl regis = (IxcRegistryImpl) registries.get(ctxt);

       if (regis == null) {
           IxcRegistryImpl newRegis = new IxcRegistryImpl(ctxt);

           registries.put(ctxt, newRegis);
           regis = newRegis;
       }
 
       return regis;
    }

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
     */
    public Remote lookup(String name)
        throws StubException, NotBoundException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
            sm.checkPermission(new IxcPermission(name, "lookup"));

        /* if not in the global hash, forget it */
        if (!xletContextMap.containsKey(name)) {
            throw new NotBoundException();
        }
        /* find out who exported this name */
        XletContext targetXlet = (XletContext) xletContextMap.get(name);

        /* When the xlet who exported the object is found,
           we can consult appropriate ImportRegistry to make a stub*/
        if (targetXlet != null) {
            try {
                Remote origRemoteObj =
                    (Remote)remoteObjectMap.get(name);

                /* Returns the object wrapped as a 'stub' in the context
                   of this method's caller */
                return getImportRegistry(targetXlet).lookup(origRemoteObj);
            } catch (StubException se) {
                throw se;
            } catch (RemoteException re) {
                // shouldn't happen, but just in case...
                throw new StubException("lookup failed", re);
            } catch (InterruptedException ie) {
                throw new StubException("Can't lookup", ie);
            }
        }
        return null;
    }

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
     */
    public void bind(String name, Remote obj)
        throws StubException, AlreadyBoundException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
            sm.checkPermission(new IxcPermission(name, "bind"));

        synchronized (lock) {
            if (remoteObjectMap.get(name) != null) {
                throw new AlreadyBoundException();
            }
            try {
                rebind(name, obj);
            } catch (AccessException e) {
                // can't happen, but just in case,
                throw new AlreadyBoundException();
            }
        }
    }

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
     */
    public void unbind(String name)
        throws NotBoundException, AccessException {

        /* 6254044, Exception checking - there are three cases.
         * 1) The name was never bound  (binderCtxt == null)
         * 2) The name was binded by the same xlet as the caller
         *    of this method. (binderCtxt == this.context)
         * 3) The name was binded by a different xlet.
         *    (binderCtxt != this.context && binderCtxt != null)
         * Permission check is needed for (1) and (3) but not for (2).
        */
        XletContext binderCtxt = (XletContext) xletContextMap.get(name);
        if (binderCtxt != context) {
           // Either case (1) or (3), Security check first.
           SecurityManager sm = System.getSecurityManager();
           if (sm != null) {
               sm.checkPermission(new IxcPermission(name, "bind"));
           }

           // The caller has the right permission, so just throw
           // exceptions based on the condition.

           if (binderCtxt == null) {  // case (1)
              throw new NotBoundException("Object not bound");
           } else { // case (3)
              throw new AccessException("Cannot unbind objects bound by other xlets");
           }
        }

        // Case (2), proceed.
        synchronized (lock) {
            remoteObjectMap.remove(name);
            xletContextMap.remove(name);
        }
    }

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
     */
    public void rebind(String name, Remote obj)
        throws StubException, AccessException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
            sm.checkPermission(new IxcPermission(name, "bind"));

        /* first check if the remote interface is valid */
        try {
            ixcClassLoader.getStubClass(ixcClassLoader, obj.getClass());
        } catch (RemoteException e) {
            if (e instanceof StubException) throw (StubException) e;
            else throw new StubException("Cannot bind", e);
        }
        /* validity check is done here */
        synchronized (lock) {
            Object ctxt = xletContextMap.get(name);
            if (ctxt == null) {
                xletContextMap.put(name, context);
            } else if (ctxt != context) {
                throw new AccessException("Cannot rebind an object of another xlet");
            }
            remoteObjectMap.put(name, obj);
        }
    }

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
     */
    public String[] list() {

        SecurityManager sm = System.getSecurityManager();
        if (sm == null) {
           synchronized (lock) {
              return (String[]) remoteObjectMap.keySet().toArray(
                      new String[remoteObjectMap.size()]);
           }
        }

        // SecurityManager is installed, return the names
        // which the caller has the permission to lookup.
        Vector v = new Vector();
        synchronized (lock) {
           String name;
           for (Enumeration enumeration = remoteObjectMap.keys();
                enumeration.hasMoreElements(); ) {
              name = (String)enumeration.nextElement();
              try {
                 sm.checkPermission(new IxcPermission(name, "lookup"));
                 v.add(name);
              } catch (SecurityException e) {}
           }
        }

        return (String[]) v.toArray(new String[v.size()]);
    }

    /**
     * Removes the bindings for all remote objects currently exported by
     * the calling Xlet.
     */
    public void unbindAll() {
        for (Enumeration e = xletContextMap.keys(); e.hasMoreElements();) {
            String key = (String) e.nextElement();
            XletContext other = (XletContext) xletContextMap.get(key);
            if (other == context) {
                // Cleanup xletContextMap and remoteObjectMap
                synchronized(lock) {
                   xletContextMap.remove(key);
                   remoteObjectMap.remove(key);
                }
            } else {
                // Clean up ImportRegistry for the other (alive) xlet
                // which imported objects from this destroyed xlet

                IxcRegistryImpl regis =
                    (IxcRegistryImpl) registries.get(other);
                ImportRegistry ir = null;
                synchronized (regis.importRegistries) {
                    ir = (ImportRegistry) regis.importRegistries.get(context);
                    if (ir != null) {
                        regis.importRegistries.remove(context);
                    }
                }
                if (ir != null) {
                    // This means that we have imported objects from other,
                    // so we need to unhook them.
                    ir.targetDestroyed();
                }
            }
        }
        // finally remove entry from IxcRegistry Hash itself
        registries.remove(context);
        return;
    }

    /**
     *  Returns an ImportRegistry of the target xlet which the xlet
     *  belonging to this IxcRegistry is trying to import an object
     *  from.
     */
    ImportRegistry getImportRegistry(XletContext targetXlet) {
        if (targetXlet == null)
            return null;
        ImportRegistry result;
        synchronized (importRegistries) {
            result = (ImportRegistry) importRegistries.get(targetXlet);
            if (result == null) {
                result = new ImportRegistry(context, targetXlet);
                importRegistries.put(targetXlet, result);
            }
        }
        return (ImportRegistry) result;
    }

    void executeForClient(Runnable r) throws RemoteException {
        worker.execute(r);
    }

    // Convenience method to access IxcClassLoader based on XletContext
    static IxcClassLoader getIxcClassLoader(XletContext ctxt) {
        return getIxcRegistryImpl(ctxt).ixcClassLoader;
    }
} 
