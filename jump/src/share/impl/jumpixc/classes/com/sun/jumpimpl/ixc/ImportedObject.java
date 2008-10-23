/*
 * @(#)ImportedObject.java	1.5 06/09/06
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

import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.rmi.Remote;
import java.rmi.RemoteException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.*;
 

/*
 * An record of an imported Remote objects for this client VM.
 */

public class ImportedObject {

   static HashMap importedObjects = new HashMap();  // <XletContext, HashMap of RemoteRef, ImportedObj>
   static HashMap typeByName = new HashMap(); // <Array of interface Names, stub>

   private StubObject  stub;

   /**
    * If the list of interface names in the RemoteRef parameter
    * is something that we've seen in the past, then simply instanciate
    * the stub class that we generated earlier.  Else, generate the stub 
    * and save it for the future use. 
    * TODO: In the future, may need to identify stub based on XletContext
    * as well (necessary when we have multiple xlets in a single VM).
    **/
   private ImportedObject(RemoteRef ref, XletContext context)
          throws StubException {
  
      List interfaceNames = Arrays.asList(ref.interfaceNames);
      Class stubClass;

      synchronized(typeByName) {
    
         stubClass = (Class)typeByName.get(interfaceNames);

         if (stubClass == null) {
            ClassLoader xletLoader = context.getClassLoader(); 
  
            if (xletLoader == null)
                throw new StubException("XletContext.getClassLoader() returns null");
  
            RemoteObjectType roType;
            try {
               roType = new RemoteObjectType(ref.interfaceNames, xletLoader);
            } catch (RemoteException re) {
               if (re instanceof StubException) 
                  throw (StubException) re;
               else
                  throw new StubException("Can't find needed interfaces for stub generation", re.getCause());
            }

            checkForTheMethodsMatch(ref.methodIDs, roType.methodsByID.keySet()); 
            StubClassGenerator generator = 
               new StubClassGenerator(xletLoader);
            stubClass = generator.generate(roType);

            typeByName.put(interfaceNames, stubClass);
         }
      }

      // Stub class to be instanciated.
      final Class stubClassF = stubClass;

      //System.out.println("@@ Got remote class " + stubClass);
      final Class[] types = { Object.class, Object.class };
      final Object[] args = { ref, context };

      // Instanciate a stub.
      Object returnValue = AccessController.doPrivileged(
         new PrivilegedAction() {
            public Object run() {

               Object obj;
               try {
                  Constructor m = stubClassF.getConstructor(types);
                  obj = m.newInstance(args);
               } catch (Exception ex) {
                  obj = new StubException("Cannot create stub object", ex);
               }
               
               return obj;
            }
         }
      );

      if (returnValue instanceof StubException) {
         throw (StubException) returnValue;
      } 

      stub = (StubObject) returnValue;
      //System.out.println("@@ Created object " + stub);
   }

   // Return a stub object for the RemoteRef.
   // If the RemoteRef object has been registered in the past,
   // then just return the ImportedObject for it.
   public static Remote registerImportedObject(RemoteRef ref, XletContext context) 
      throws StubException {

      ImportedObject importedObject = null;

      synchronized(importedObjects) {
         //importedObject = (ImportedObject) importedObjects.get(ref);
         HashMap map = (HashMap) importedObjects.get(context);
         if (map == null) {
            map = new HashMap();
            importedObjects.put(context, map);
         } else {
            importedObject = (ImportedObject) map.get(ref);
         }

         if (importedObject == null) {
            importedObject = new ImportedObject(ref, context);
            map.put(ref, importedObject);
         }
      }
      
      return (Remote) importedObject.stub;
   }

   private void checkForTheMethodsMatch(long[] originalIDs, Collection otherIDs)                                                                                
      throws StubException {
                                                                                
      // Checks whether the array of method hash passed in as a RemoteRef
      // contains exactly the same set of long values that this importer
      // computed based on the set of Remote Methods found in this classloader
      // context.
                                                                               
      if (originalIDs.length != otherIDs.size()) 
         throw new StubException("Mismatching Remote Interface");

      for (int i = 0; i < originalIDs.length; i++) {
         if (!otherIDs.contains(new Long(originalIDs[i])))
           throw new StubException("Mismatching Remote Interface");
      }
   }
}
