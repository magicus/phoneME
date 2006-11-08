/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.xlet.mvmixc;

import java.rmi.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import javax.microedition.xlet.ixc.StubException;

import com.sun.appmanager.mtask.TaskListener;

public class CDCAmsIxcRegistry extends IxcRegistry 
            implements CDCAmsRemoteInterface, TaskListener {
 
    String CDCAmsMasterAppID = "0";

    // <name, RemoteRef>
    private HashMap registeredObjects  = new HashMap(); 

    // <RemoteRef, HashSet of imported xlet IDs>
    private HashMap importedObjects    = new HashMap(); 

    static CDCAmsIxcRegistry registry;

    private RegistryListUpdater updater = new RegistryListUpdater();

    private CDCAmsIxcRegistry(XletContext context) {
       try {
          //Automatically export this to be available for the children
          ExportedObject.registerExportedObject(this, context);

          //Start ServerSocket
          ConnectionReceiver.startAppManagerSocket();

          new Thread(updater).start(); 
         
       } catch (Exception e) {
          System.out.println("Cannot export CDCAmsIxcRegistry!!" + e);
          return;   
       }
    }

    public static IxcRegistry getRegistry(XletContext context) {
       if (registry == null) {
          registry = new CDCAmsIxcRegistry(context);
       }
 
       return registry;
    }

    public Remote lookup(String name)  
        throws StubException, NotBoundException {
       synchronized(registeredObjects) {
          if (!registeredObjects.containsKey(name)) {  
             throw new NotBoundException("Name not bound: " + name);
          }

          return (Remote) registeredObjects.get(name);
       }
    } 

    public void bind(String name, Remote obj) 
       throws StubException, AlreadyBoundException {
       synchronized(registeredObjects) {
          if (registeredObjects.containsKey(name)) {  
             throw new AlreadyBoundException("Name already bound: " + name);
          }
          registeredObjects.put(name, obj);
       }

       notifyObjectImport(CDCAmsMasterAppID, obj);
    }
    
    public void unbind(String name)
        throws NotBoundException, AccessException {
 
       Remote r;
       synchronized(registeredObjects) {
          if (!registeredObjects.containsKey(name)) {  
             throw new NotBoundException("Name not bound: " + name);
          }
          r = (Remote) registeredObjects.remove(name);
       }

       // If noone else is binding this remote object to the IxcRegistry  
       // using another name, then remove the MasterApp's ID from the 
       // importedObjects list also.
       if (!registeredObjects.containsValue(r)) {
          synchronized(importedObjects) {
             Set s = (Set)importedObjects.get(r);
             if (s != null)
                s.remove(CDCAmsMasterAppID);
          }
       }
    }

    public void rebind(String name, Remote obj)
        throws StubException, AccessException {
        // Unbind, bind.
        try {
           unbind(name);
           bind(name,obj);
        } catch (NotBoundException e) {// forget it
        } catch (AlreadyBoundException e) { // can't happen
           throw new StubException("Error in rebind", e);
        }
    }

    public String[] list() {
       String[] list;
       synchronized(registeredObjects) {
          list = (String[]) registeredObjects.keySet().toArray(new String[]{});
       }
       return list;
    }

    public void unbindAll() {
        // Unbind all the objects exported by this guy.
        System.out.println("@@@CDCAmsIxcRegistry.unbindAll() not implemented");
    }

   public void notifyObjectImport(String importingXletID, Remote ref) {
 
      synchronized(importedObjects) {
         Set s = (Set)importedObjects.get(ref);
         if (s == null) {
            s = new HashSet();
            importedObjects.put(ref, s);
         }
         s.add(importingXletID);
      }
   }

   public Remote lookupWithXletID(String name, String importingXletID) 
      throws RemoteException, NotBoundException, AccessException {

      Remote r = lookup(name);

      if (!((RemoteRef)r).getXletID().equals(importingXletID)) {
         notifyObjectImport(importingXletID, r);
      }
 
      return r;
   }


   // CDCAmsAppControllerListener's handler for 
   // AppManager's task_killed events.
   public void taskEvent(String appID, int what) {
      if (what == TaskListener.CDCAMS_TASK_KILLED) {
         //System.out.println("@@@ Got killed event: " + appID);
         //Worker.execute(Thread.NORM_PRIORITY, new RegistryListUpdater(appID));
         updater.addAppID(appID);
      }
   }

/* 
 * A thread that updates the CDCAmsAppRegistry's registered/
 * imported remote object data structure in response to the
 * client died notification from the master mvm VM.
*/
class RegistryListUpdater implements Runnable {

   private ArrayList appIDs = new ArrayList();

   public synchronized void addAppID(String id) {
      appIDs.add(id);
   }

   public synchronized String getNextAppID() {
      while (appIDs.isEmpty()) {
         try {
            wait(1000);
         } catch (InterruptedException e) {}
      }

      return (String)appIDs.remove(0);
   }

   public void run() { 
      for (;;) {
         iterateLists(getNextAppID());
      } 
   }

   public void iterateLists(String appID) {

      String name;
      RemoteRef ref;
      ArrayList nameList = new ArrayList();

      // Remove objects bound by this dying xlet from this 
      // IxcRegistry.
      synchronized(registeredObjects) {
         Iterator iterator = registeredObjects.keySet().iterator();
         while (iterator.hasNext()) {
            name = (String)iterator.next();
            ref = (RemoteRef)registeredObjects.get(name);
            if (ref.getXletID().equals(appID)) {
               nameList.add(name);
            }
         }
         for (int i = 0; i < nameList.size(); i++) {
            registeredObjects.remove(nameList.get(i));
         }
      }

      Set set;
      ArrayList refList  = new ArrayList();
      synchronized(importedObjects) {

         // First, need to remove all RemoteRefs exported by this dying
         // xlet from the importedObjects hash.
         // Second, need to iterate other RemoteRefs, and remove
         // the ID recorded for this dying xlet.
         // Finally, if this dying xlet's ID is the last item of
         // the other RemoteRefs' set, then remove that RemoteRef
         // from the hash too, and notify the RemoteRef owner
         // that nobody holds an reference to that RemoteObject so
         // it can safely be discarded for GC.

         Iterator iterator = importedObjects.keySet().iterator();
         while (iterator.hasNext()) {
            ref = (RemoteRef)iterator.next();

            if (ref.getXletID().equals(appID)) {
               // Need to remove this RemoteRef.
               refList.add(ref);
            } else {
               // Need to update this RemoteRef's Set.
               set = (Set)importedObjects.get(ref);
               set.remove(appID);

               if (set.isEmpty()) {
                  refList.add(ref);
                  notifyReferenceRemoval(ref);
               } 
            }
         }

         for (int i = 0; i < refList.size(); i++) {
            importedObjects.remove(refList.get(i));
         }
      }
   }

   private void notifyReferenceRemoval(RemoteRef ref) {
      // ToDo: Need to implement
      System.out.println("@@@notifyReferenceRemoval: " + ref);
   }
}

}
