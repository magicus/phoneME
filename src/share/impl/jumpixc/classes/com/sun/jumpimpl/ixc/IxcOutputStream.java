/*
 * @(#)IxcOutputStream.java	1.6 06/08/10
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

import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;
import java.io.OutputStream;
import java.lang.reflect.Array;
import java.rmi.Remote;
import java.security.AccessController;
import java.security.PrivilegedAction;

import javax.microedition.xlet.XletContext;

public class IxcOutputStream extends ObjectOutputStream {

   private XletContext context;

   static private NullObject nullObject = new NullObject();

   IxcOutputStream(OutputStream out, XletContext context, boolean isExecutiveVM)
      throws IOException {
      super(out);
      this.context = context;

      /***
       * isExecutiveVM value indicates that this IxcOutputStream is used
       * for the central JUMPExecIxcRegistry's output stream.
       * In the JUMPExecIxcRegistry, we don't want to be converting
       * Remote object to a RemoteRef, but just write out outgoing
       * RemoteRef objects.
      **/
      if (!isExecutiveVM) {
         AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
               enableReplaceObject(true);
               return null;
            }
         });
      }
   }

   protected Object replaceObject(Object obj) 
      throws IOException { 

      Object nextObject = obj;

      if (nextObject instanceof Remote) { 
         /* 
         * If this is a Remote object, need to replace it
         * with a corresponding RemoteRef instance to be sent
	 * over the pipe. IxcInputStream will convert the 
	 * incoming RemoteRef instance to the stub.
	 *
         * If this object is already a stub, just send 
         * the RemoteRef used to create that stub.
         * Else, record this Remote instance in 
         * the ExportedObject table, and send out the
         * RemoteRef given from the ExportedObject.
         **/
         if (nextObject instanceof StubObject) { // is it a stub?
            nextObject = ((StubObject)nextObject).remoteRef;
         } else {
            //System.out.println("@@implicit export at marshallObject");
            ExportedObject eo =
               ExportedObject.registerExportedObject((Remote)nextObject, context);

            nextObject = eo.getRemoteRef();
         }
      } else if (nextObject == null) {
         nextObject = nullObject;
      }
   
      return nextObject;
   }
}
