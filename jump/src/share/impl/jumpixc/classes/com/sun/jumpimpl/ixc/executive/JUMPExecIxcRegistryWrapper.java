/*
 * @(#)JumpExecIxcRegistryWrapper.java	1.3 06/09/06
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
 *
 */

package com.sun.jumpimpl.ixc.executive;

import java.rmi.*;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcPermission;
import javax.microedition.xlet.ixc.IxcRegistry;
import javax.microedition.xlet.ixc.StubException;

import com.sun.jumpimpl.ixc.ExportedObject;
import com.sun.jumpimpl.ixc.ImportedObject;
import com.sun.jumpimpl.ixc.RemoteRef;

/*
 * Special purpose IxcRegistry for the Executive VM only.
 * Talks with JUMPExecIxcRegistry directly without serialization 
 * or Socket connection.
 */

public class JUMPExecIxcRegistryWrapper extends IxcRegistry {
 
    private static JUMPExecIxcRegistryWrapper serviceRegistry;
    private JUMPExecIxcRegistry mainRegistry;
    private XletContext context;

    // The 'name' Strings binded by this IxcRegistry
    private ArrayList exportedNames;

    protected JUMPExecIxcRegistryWrapper(XletContext context) {
       this.context = context;
       exportedNames = new ArrayList();
       mainRegistry = JUMPExecIxcRegistry.getJUMPExecIxcRegistry();
    }

    public static IxcRegistry getRegistry(XletContext context) {
       if (serviceRegistry == null) 
          serviceRegistry = new JUMPExecIxcRegistryWrapper(context);

       return serviceRegistry;
    }

    public Remote lookup(String name)  
        throws StubException, NotBoundException {

        SecurityManager sm = System.getSecurityManager();
        if (sm != null)
           sm.checkPermission(new IxcPermission(name, "lookup"));

        RemoteRef remoteRef = (RemoteRef) mainRegistry.lookup(name);

        Remote nextObject;

        ExportedObject eo =
            ExportedObject.findExportedObject(remoteRef.getObjectID());
                                                                                
        if (eo != null && eo.getRemoteRef().equals(remoteRef)) {
            nextObject = eo.getRemoteObject(); // get the original
        } else {
            nextObject = ImportedObject.registerImportedObject(
                         remoteRef, context); // Implicit import
        }
        
        return nextObject;
    } 

    public void bind(String name, Remote obj) 
       throws StubException, AlreadyBoundException {

       SecurityManager sm = System.getSecurityManager();
       if (sm != null)
          sm.checkPermission(new IxcPermission(name, "bind"));

       try {
          ExportedObject eo =
            ExportedObject.registerExportedObject((Remote)obj, context);

          RemoteRef remoteRef = eo.getRemoteRef();

          mainRegistry.bind(name, remoteRef);

       } catch (RemoteException e) {
          if (e instanceof StubException) 
             throw (StubException) e;
          throw new RuntimeException(e.getCause());
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
                                                                                
           // This is inefficient, but we need to check for
           // NotBoundException first.
           List list = Arrays.asList(mainRegistry.list());
           if (!list.contains(name))
              throw new NotBoundException("Name " + name + " not bound");

           // At this point, just throw AccessException
           throw new AccessException("Cannot unbind objects bound by other xlets");
        }

        mainRegistry.unbind(name); 

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

        try {
           mainRegistry.unbind(name); 
           bind(name, obj);
        } catch (NotBoundException nbe) { // just ignore
        } catch (AlreadyBoundException abe) { // can't happen
        }
    }

    public String[] list() {
        String[] names = mainRegistry.list();

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
             mainRegistry.unbind(names[i]);
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
}
