/*
 * @(#)IxcInputStream.java	1.6 06/08/10
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

import java.io.ObjectInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.ObjectStreamClass;
import java.rmi.Remote;
import java.security.AccessController;
import java.security.PrivilegedAction;

import java.lang.reflect.Array;

import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import javax.microedition.xlet.ixc.StubException;

/*
 *
 */

public class IxcInputStream extends ObjectInputStream {

   XletContext context;

   IxcInputStream(InputStream in, XletContext context, boolean isExecutiveVM) 
      throws IOException {
      super(in);
      this.context = context;

      /*** 
       * isExecutiveVM value indicates that this IxcInputStream is used
       * for the central JUMPExecIxcRegistry's input stream.
       * In thie JUMPExecIxcRegistry, we don't want to be converting
       * Remote object to a stub or vice versa, but just record incoming
       * RemoteRef objects.
      **/
      if (!isExecutiveVM) {
         AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
               enableResolveObject(true);
               return null;
            }
         });
      }
   }

   protected Class resolveClass(ObjectStreamClass desc)
      throws IOException, StubException {
      String name = desc.getName();
      try {
          // First, try to find the class in the xlet's classloader
          return Class.forName(name, false, context.getClassLoader());
      } catch (ClassNotFoundException ex) {
          try {
             // If not found, fall back into the default classloader implementation
             // in the superclass
             return super.resolveClass(desc);
          } catch (ClassNotFoundException cnfe) {
             throw new StubException("Class " + name + " not found", cnfe);
          }
      }
   }

   // If the incoming object is a RemoteRef, inspect it's origin. 
   // If this VM is the exporter, return the original remote object,
   // else record it as an import and return a generated stub instead.
   protected Object resolveObject(Object obj) 
      throws IOException { 

      Object nextObject = obj;
    
      if (nextObject instanceof NullObject) {
         nextObject = null;
      }

      if (nextObject instanceof Remote) { 
         /* Remote object over the wire should be a RemoteRef instance */
         RemoteRef remoteRef = (RemoteRef) nextObject;

         ExportedObject eo =
            ExportedObject.findExportedObject(remoteRef.getObjectID());
         
         if (eo != null && eo.getRemoteRef().equals(remoteRef)) {
            nextObject = eo.remoteObject; // get the original
         } else {
            nextObject = ImportedObject.registerImportedObject(
                            remoteRef, context); // Implicit import
            reportImportToJumpExecIxcRegistry((StubObject)nextObject, context);
         }
      }

      return nextObject;
    }

    void reportImportToJumpExecIxcRegistry(StubObject r,
                                   XletContext importer) {
                                                                                
       // Report to the JUMPExecIxcRegistry about this new import if
       // the connection is made not with
       // JumpExecIxcRegistry itself but with some other client VM.

       IxcRegistry registry = IxcRegistry.getRegistry(importer);
                                                                                
       if (registry instanceof JUMPIxcRegistryImpl) {
          ((JUMPIxcRegistryImpl)registry).amHandler.notifyObjectImport(
                         Utils.getMtaskClientID(), (Remote)r);
       }
    }


}
