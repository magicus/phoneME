/*
 * @(#)IxcRegistry.java	1.19 06/10/10
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

package javax.microedition.xlet.ixc;

import java.lang.reflect.Method;
import java.rmi.*;
import java.rmi.registry.Registry;
import java.util.Hashtable;
import javax.microedition.xlet.XletContext;

/**
 * <code>IXCRegistry</code> is the bootstrap mechanism for obtaining
 * references to remote objects residing in other Xlets executing on
 * the same machine, but in separate classloaders.
 * 
 * <p>
 * Instances of <code>IXCRegistry</code> are never accessible via
 * <code>java.rmi.Naming</code> or
 * <code>java.rmi.registry.LocateRegistry</code> if RMI functionality
 * is implemented.
 *
 * @see java.rmi.Registry
 */

public abstract class IxcRegistry implements Registry {
    /**
     * Creates the IxcRegistry instance.
     */
    protected IxcRegistry() {}

    static boolean isPlatformChecked = false;
    static Method  getRegistryImplMethod = null;

    static String svmIxcRegistryName = "com.sun.xlet.ixc.IxcRegistryImpl"; 
    static String mvmIxcRegistryName = "com.sun.jumpimpl.ixc.JUMPIxcRegistryImpl";

    /**
     * Returns the Inter-Xlet Communication registry.
     */
    public static IxcRegistry getRegistry(XletContext context) {

        if (context == null) 
           throw new NullPointerException("XletContext is null");
   
        if (getRegistryImplMethod == null) {
           Class ixcRegistryImplClass = null ;
           try {
              ixcRegistryImplClass = Class.forName(svmIxcRegistryName);
           } catch (Exception e) { // Not found, let's try MVM.
           }

           if (ixcRegistryImplClass == null) {
              try {
                 ixcRegistryImplClass = Class.forName(mvmIxcRegistryName);
              } catch (Exception e) { // Problem.  ixcRegistryImplClass remains null.
              }
           }
           
           if (ixcRegistryImplClass == null) {
              System.out.println("Fatal error in starting IXC: ");
              System.out.println("Neither " + svmIxcRegistryName + " or " 
                                 + mvmIxcRegistryName + " is found.");
           
              return null;
           }

           try {
              getRegistryImplMethod = ixcRegistryImplClass.getMethod("getIxcRegistryImpl",
                 new Class[] { javax.microedition.xlet.XletContext.class });
           } catch (NoSuchMethodException nsme) {
              nsme.printStackTrace();
           } catch (SecurityException se) {
              se.printStackTrace();
           }
        }

        try {
           if (getRegistryImplMethod != null) {
              return (IxcRegistry) 
                 getRegistryImplMethod.invoke(null, new Object[] { context } );
           } 
        } catch (Exception e) { e.printStackTrace(); }
   
        return null;
    }

    /**
     * Returns a reference, a stub, for the remote object associated
     * with the specified <code>name</code>.
     *
     * @param name a URL-formatted name for the remote object
     * @return a reference for a remote object
     * @exception NotBoundException if name is not currently bound
     * @exception StubException If a stub could not be generated
     */

    public abstract Remote lookup(String name)
        throws StubException, NotBoundException;

    /**
     * Binds the specified <code>name</code> to a remote object.
     *
     * @param name a URL-formatted name for the remote object
     * @param obj a reference for the remote object (usually a stub)
     * @exception AlreadyBoundException if name is already bound
     * @exception MalformedURLException if the name is not an appropriately
     *  formatted URL
     * @exception RemoteException if registry could not be contacted
     */
    public abstract void bind(String name, Remote obj)
        throws StubException, AlreadyBoundException;
    
    /**
     * Destroys the binding for the specified name that is associated
     * with a remote object.
     *
     * @param name a URL-formatted name associated with a remote object
     * @exception NotBoundException if name is not currently bound
     */
    public abstract void unbind(String name)
        throws NotBoundException, AccessException;

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
    public abstract void rebind(String name, Remote obj)
        throws StubException, AccessException;

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
    public abstract String[] list();


    /**
     * Removes the bindings for all remote objects currently exported by
     * the calling Xlet.
     */
    public abstract void unbindAll();

}
