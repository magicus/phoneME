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

import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.rmi.Remote;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.lang.reflect.Method;

/*
 * This is the 'marker' class that gets passed around
 * between VMs for the method invocation.
 * Each instance contains information for the original Remote object's
 * ID (only valid within the exporting VM), the port number
 * which the exporting VM is listening to with it's 
 * ConnectionReceiver, the exporting xlet's ID, and
 * information about the original remote object's
 * remote interfaces and remote methods, which are used
 * by the importer VM to generate a stub.
 */

public class RemoteRef implements Remote, Serializable{

   private long objectID;
   private int portID;
   private int processID;

   String[] interfaceNames;
   long[] methodIDs;

   public RemoteRef(long objectID, int portID, int processID,
                    String[] remoteInterfaces,
                    long[] methodIDAsLongs) {
      this.objectID = objectID;
      this.portID = portID;
      this.processID = processID;

      interfaceNames = remoteInterfaces;
      methodIDs = methodIDAsLongs;

      //Arrays.sort(methodIDs);
      //dump();
   }

   public long   getObjectID()  { return objectID; } 
   public int    getPortID()    { return portID; }
   public int    getXletID()    { return processID; }

   public void dump() {
      System.out.println("ObjectID = " + objectID);
      System.out.println("portID   = " + portID);
      System.out.println("ProcessID= " + processID);
      for (int i = 0; i < interfaceNames.length; i++) {
         System.out.println("   " + i + ":" + interfaceNames[i]);
      }
      System.out.println("   " + methodIDs.length + " remote methods");
   }

   public String toString() {
      StringBuffer buf = new StringBuffer("RemoteRef[ObjectID=" 
              + objectID + ",portID=" + portID + ",ProcessID=" + 
              processID+",RemoteInterfaces=<");
      int i = 0;
      while (i < interfaceNames.length) {
         buf.append(interfaceNames[i]);
         i++;
         if (i < interfaceNames.length) 
            buf.append(",");
      }  
      buf.append(">]");
      return buf.toString();
   }

   public boolean equals(Object obj)  {
       if (!(obj instanceof RemoteRef)) return false;
       RemoteRef otherRef = (RemoteRef)obj;

       if ((objectID != otherRef.objectID) || 
           (portID   != otherRef.portID) ||
           (processID != (otherRef.processID)))
          return false;
  
       return true;
       
       //List thisClasses = Arrays.asList(interfaceNames); 
       //List otherClasses = Arrays.asList(otherRef.interfaceNames);

       //return (
       //   (thisClasses.containsAll(otherClasses)) &&
       //   (otherClasses.containsAll(thisClasses)) &&
       //   (Arrays.equals(methodIDs, otherRef.methodIDs))
            
          //(objectID == otherRef.objectID) && 
          //(portID   == otherRef.portID) &&
          //(Arrays.equals(interfaceNames, otherRef.interfaceNames)) &&
          //(Arrays.equals(methodIDs, otherRef.methodIDs))
       //);
   }
}
