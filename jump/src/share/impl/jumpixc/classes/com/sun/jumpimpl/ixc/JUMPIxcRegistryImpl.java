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

package com.sun.jumpimpl.ixc;

import java.awt.Container;
import java.rmi.*;
import java.rmi.registry.Registry;
import java.lang.reflect.Method;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.util.HashMap;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.UnavailableContainerException;
import javax.microedition.xlet.ixc.*;

public class JUMPIxcRegistryImpl extends IxcRegistry {
  
    // <XletContext, JUMPIxcRegistryImpl>  
    static HashMap ixcRegistries = new HashMap();

    // This IxcRegistry's XletContext
    private XletContext context;

    // The 'name' Strings binded by this IxcRegistry
    private ArrayList exportedNames;
    
    // The stub for the Executive's IxcRegistry
    JUMPExecIxcRegistryStub amHandler; 

    // This Xlet's AccessControlContext snapshot
    private AccessControlContext acc;

    /**
     * Returns the Inter-Xlet Communication registry.
     */
    public static IxcRegistry getIxcRegistryImpl(XletContext context) {

       synchronized(ixcRegistries) {
           IxcRegistry regis = (IxcRegistry)ixcRegistries.get(context);
           if (regis != null) return regis;

           regis = new JUMPIxcRegistryImpl(context);

           if (regis != null)
              ixcRegistries.put(context, regis);

           return regis;
       }
    }

    public static boolean isExecutiveVM() {
       return (Utils.getMtaskServerID() == Utils.getMtaskClientID() &&
               Utils.getMtaskServerID() != -1) ; 

    }

    protected JUMPIxcRegistryImpl(XletContext context) {
        super();
        this.context = context;
        this.amHandler = new JUMPExecIxcRegistryStub(context);
        this.exportedNames = new ArrayList();

        this.acc = AccessController.getContext();
    }

    public Remote lookup(String name)
        throws StubException, NotBoundException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
           sm.checkPermission(new IxcPermission(name, "lookup"));

        try {
           return amHandler.lookupWithXletID(name, 
              Utils.getMtaskClientID());
        } catch (RemoteException re) {
           System.out.println("@@@Error with lookup()");

           if (re instanceof StubException) 
              throw (StubException) re;
   
           Throwable e = re.getCause(); 
           throw new StubException("lookup() problem", e);
        }
    } 

    public void bind(String name, Remote obj)
        throws StubException, AlreadyBoundException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
           sm.checkPermission(new IxcPermission(name, "bind"));

        if (name == null || obj == null) 
           throw new NullPointerException("name and obj can't be null");

        // Basic check, if this xlet already bound the given name.
        if (exportedNames.contains(name)) 
           throw new AlreadyBoundException(name);
       
        try {

           amHandler.bind(name, obj);

        } catch (RemoteException re) {

           if (re instanceof StubException) 
              throw (StubException) re;
   
           Throwable e = re.getCause(); 
           //if (e instanceof AlreadyBoundException) 
           //   throw (AlreadyBoundException) e;
            
           throw new StubException("Bind() problem", e);
        }

        // The object is successfully registered.
        // Record the name to the local list.
        synchronized(exportedNames) {
           exportedNames.add(name); 
        }
    }
    
    public void unbind(String name)
        throws NotBoundException, AccessException {

        if (!exportedNames.contains(name)) { 

           // First, a security check.
           SecurityManager sm = System.getSecurityManager();
           if (sm != null) {
              sm.checkPermission(new IxcPermission(name, "bind"));
           }

           try {
              // This is inefficient, but we need to check for
              // NotBoundException first.
              List list = Arrays.asList(amHandler.list());
              if (!list.contains(name))
                 throw new NotBoundException("Name " + name + " not bound");
           } catch (RemoteException re) {
           }

           // At this point, just throw AccessException
           throw new AccessException("Cannot unbind objects bound by other xlets");
        } 

        try {
           amHandler.unbind(name);
        } catch (RemoteException e) {
           throw new RuntimeException(e.getCause());
        }

        // Successfully unbinded, remove the name
        // from the local list as well.
        synchronized(exportedNames) {
           exportedNames.remove(name); 
        }
    }

    public void rebind(String name, Remote obj)
        throws StubException, AccessException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
           sm.checkPermission(new IxcPermission(name, "bind"));

        if (name == null || obj == null) 
           throw new NullPointerException("name and obj can't be null");

        if (!exportedNames.contains(name)) { 

           // If we know from the local list that this xlet 
           // never exported an obj under a given name before, 
           // then do bind(). If AlreadyBoundException is caught, 
           // then throw it back as AccessException, as another
           // xlet must have bound an object under the same name.
           try { 
              bind(name, obj);
           } catch (AlreadyBoundException e) {
              throw new AccessException("Name already bound");
           }  // let other exception fall through

        } else {
           try {
              amHandler.rebind(name, obj);
           } catch (RemoteException re) {

              // StubException is locally thrown if the obj 
              // implements malformed Remote interface
              if (re instanceof StubException) 
                 throw (StubException)re;
 
              // Else inspect what happened in the
              // Executive VM
              Throwable e = re.getCause(); 
            
              if (e instanceof StubException) 
                 throw (StubException)e;

              else throw new RuntimeException(e);
           }
        }

        // Successfully rebinded, record this name
        // just in case the name was never bound.
        synchronized(exportedNames) {
           if (!exportedNames.contains(name))
              exportedNames.add(name); 
        }
    }

    public String[] list() {
        String[] names;

        try {
           names = amHandler.list();
        } catch (RemoteException re) {
           System.err.println("Unexpected exception during list()");
           re.printStackTrace();
           // At least return the names that are known locally...
           names = (String[])exportedNames.toArray(new String[]{});
        }

        SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
           ArrayList list = new ArrayList();
           for (int i = 0; i < names.length; i++) {
              try {
                 sm.checkPermission(new IxcPermission(names[i], "lookup"));
                 list.add(names[i]);
              } catch (SecurityException e) {}
           }
           names = (String[])list.toArray(new String[]{});
        }

        return names;
    }

    public void unbindAll() {

       String[] names = (String[])exportedNames.toArray(new String[]{});

       for (int i = 0; i < names.length; i++) {
          try { 
             amHandler.unbind(names[i]);
          } catch (NotBoundException nbe) {
             // Just ignore it.
          } catch (RemoteException re) {
             System.err.println("Unexpected exception during unbindAll()");
             throw new RuntimeException(re.getCause());
          }
       }

       // Successfully cleared, remove all names from the local list.
       synchronized(exportedNames) {
          exportedNames.clear();
       }
    }

    AccessControlContext getACC() { return acc; }
}
