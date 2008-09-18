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

import java.rmi.registry.Registry;
import java.rmi.*;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Iterator;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.StubException;

/* 
 * A pre-generated Stub class for JUMPExecIxcRegistry.
 * Clinet VMs use this class to talk with master IxcRegistry.
 */

public final class JUMPExecIxcRegistryStub extends StubObject 
                                  implements JUMPExecIxcRegistryRemote {

    static RemoteRef thisRef;
    static HashMap methodIDsByName = new HashMap();

    static private boolean isInitialized = false;
    private static RemoteRef initialize() {
        if (!isInitialized) {
           isInitialized = true;
           try {
              RemoteObjectType type = 
                 new RemoteObjectType(JUMPExecIxcRegistryRemote.class);

              thisRef = new RemoteRef(ExportedObject.newObjectID(),
                              ConnectionReceiver.getExecVMServicePort(),
                              Utils.getMtaskServerID(),
                              type.getRemoteInterfaceNames(),
                              type.getMethodIDsAslongs());


              Iterator iterator = type.methodsByID.keySet().iterator();
              while (iterator.hasNext()) {
                 Long id = (Long) iterator.next(); 
                 Method m = (Method) type.methodsByID.get(id);
                 methodIDsByName.put(m.getName(), id);
              }


           } catch (RemoteException e) {
              System.err.println("Fatal error in starting IXC");
              throw new RuntimeException(e.getCause());
           }
        }

        return thisRef;
    }

    public JUMPExecIxcRegistryStub(XletContext context) {
        // Arguments are of RemoteRef and XletContext, for it's ClassLoader.
        super(initialize(), context);
    }

    public long getMethodID(String s) {
        Long l = (Long) methodIDsByName.get(s);
        if (l != null) 
           return l.longValue();
        else 
           return 0x0;
    }

    public Remote lookup(String name)
        throws RemoteException, NotBoundException, AccessException {

        try {
           Object r = com_sun_xlet_execute(getMethodID("lookup"),new Object[] { name });
           return (Remote) r;
        } catch (RemoteException re) { 
           throw re; 
        } catch (NotBoundException nbe) {
           throw nbe;
        } catch (Exception e) {
           new StubException("", e); 
        }
        return null;
    }

    public Remote lookupWithXletID(String name, int xletID)
        throws RemoteException, NotBoundException, AccessException {

        try {
           Object r = com_sun_xlet_execute(
               getMethodID("lookupWithXletID"), new Object[] { name, new Integer(xletID) });
           return (Remote) r;
        } catch (RemoteException re) { 
           throw re; 
        } catch (NotBoundException nbe) {
           throw nbe;
        } catch (Exception e) {
           new StubException("", e); 
        }
        return null;
    }

    public void bind(String name, Remote obj)
        throws RemoteException, AlreadyBoundException, AccessException {

        try {
           com_sun_xlet_execute(getMethodID("bind"),new Object[] { name, obj });
        } catch (RemoteException re) { 
           throw re; 
        } catch (AlreadyBoundException nbe) {
           throw nbe;
        } catch (Exception e) {
           new StubException("", e); 
        }
    }

    public void unbind(String name)
        throws RemoteException, NotBoundException, AccessException {
        try {
           com_sun_xlet_execute(getMethodID("unbind"),new Object[] { name });
        } catch (RemoteException re) { 
           throw re; 
        } catch (NotBoundException nbe) {
           throw nbe;
        } catch (Exception e) {
           new StubException("", e); 
        }
    }

    public void rebind(String name, Remote obj)
        throws RemoteException, AccessException {
        try {
           com_sun_xlet_execute(getMethodID("rebind"),new Object[] { name, obj });
        } catch (RemoteException re) { 
           throw re; 
        } catch (Exception e) {
           new StubException("", e); 
        }
    }

    public String[] list()
        throws RemoteException, AccessException {
        try {
           Object r = com_sun_xlet_execute(getMethodID("list"),new Object[] {});
           return (String[]) r;
        } catch (RemoteException re) { 
           throw re; 
        } catch (Exception e) {
           new StubException("", e); 
        }
        return null;
    }

    public void notifyObjectImport(int importingXletID, Remote obj) {
        try {
           com_sun_xlet_execute(getMethodID("notifyObjectImport"),
                                new Object[] { new Integer(importingXletID), obj });
        } catch (Exception e) {
           e.printStackTrace();
        }
    }
}

