/*
 * @(#)ExportedObject.java	1.7 06/09/06
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

package com.sun.jumpimpl.ixc;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Arrays;
import java.util.List;
import java.lang.reflect.Method;

import javax.microedition.xlet.XletContext;

/*
 * A record of Remote objects exported by this VM.
 * ExportObject is also responsible for creating a corresponding
 * RemoteRef.
 *
 * When ConnectionReceiver receives a request for a remote method
 * invocation, it queries for the ExportedObject corresponding to
 * the incoming RemoteRef, and find out the original Remote object
 * which the method can be invoked on.
 */

public class ExportedObject {

   static HashMap objectsByID = new HashMap(); // <object ID, ExportedObject>
   static HashMap typeByName = new HashMap(); // <RemoteObject Class, RemoteObjectType>

   long objectID;
   Remote remoteObject; // The original "Remote" object exported
   XletContext context; // The xlet that owns the original Remote object
   RemoteObjectType type; // This remote object's Object type.
   RemoteRef remoteRef; // This ExportedObject's RemoteRef.

   static final long startingObjectID = 1000L; // the first ID, 

   /*
    * Note: this initial value is assigned for Executive VM's
    * IxcRegistry object as well as the first object exported by
    * any child xlet.
    */
   private static long nextObjectID = startingObjectID;
   
   private ExportedObject(long objectID, Remote remote, XletContext context) 
      throws RemoteException {

      this.context      = context;
      this.objectID     = objectID;
      this.remoteObject = remote;

      Class remoteObjectClass  = remote.getClass();

      synchronized(typeByName) {
          this.type = (RemoteObjectType) typeByName.get(remoteObjectClass);
          if (this.type == null) {
             this.type = new RemoteObjectType(remoteObjectClass);
             typeByName.put(remoteObjectClass, this.type);
          }
      }
   }

   static long newObjectID() {
      return nextObjectID++;
   }

   /* Returns the "original" remote object this ExportedObject encapselates. */
   public Remote getRemoteObject() {
      return remoteObject;
   }

   public static ExportedObject registerExportedObject(Remote r, XletContext context) 
      throws RemoteException {
      long id = newObjectID();
      ExportedObject obj = new ExportedObject(id, r, context);
      objectsByID.put(new Long(id), obj);
      return obj;
   }

   public static ExportedObject findExportedObject(long ObjID) {
      return (ExportedObject) objectsByID.get(new Long(ObjID));
   }

   public Method findExportedMethod(long methodID) {
      return (Method) type.methodsByID.get(new Long(methodID));
   }

   public RemoteRef getRemoteRef() {

      if (remoteRef == null) {
         remoteRef = new RemoteRef(objectID, 
                           ConnectionReceiver.getLocalServicePort(),
                           Utils.getMtaskClientID(),
                           type.getRemoteInterfaceNames(),                          
                           type.getMethodIDsAslongs());
      }
      return remoteRef;
   }
}  
