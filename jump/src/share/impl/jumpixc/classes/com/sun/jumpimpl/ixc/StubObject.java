/*
 * @(#)StubObject.java	1.5 06/08/10
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
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.EOFException;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.rmi.Remote;
import java.rmi.RemoteException;
import java.security.PrivilegedAction;
import java.security.AccessController;

import javax.microedition.xlet.XletContext;

/* 
 * The superclass of an generated stub.
 * com_sun_xlet_execute(long, Object[]) is the method
 * which handles the remote method invocation.
 */

public class StubObject {

    /** 
     * Importing Xlet's XletContext.
     * (Contains info for the ClassLoader used to define this stub)
     */
    XletContext context;

    /** 
     * RemoteRef used to generate this StubObject. 
     * Package private, to support the library code to 
     * map the stub back into RemoteRef. 
     */
    final RemoteRef remoteRef;

    /* Set to true for debugging info */
    private static boolean debug = false;

    protected StubObject(Object obj1, Object obj2) {
       remoteRef = (RemoteRef) obj1;
       context = (XletContext) obj2;
    }
  
    //    
    // Execute a remote method.  
    //
    protected final Object
    com_sun_xlet_execute(long methodHash, Object[] args) 
       throws Exception {

       Object returnValue = null;
       Exception exceptionValue = null;

       InputStream in = null;
       OutputStream out = null;
       Socket clientSocket = null;

       try {
  
         Object obj = AccessController.doPrivileged(
            new PrivilegedAction() {
               public Object run() { 
                  try {
                     Socket s= new Socket("localhost", remoteRef.getPortID());
                     s.setReuseAddress(true);
                     return s;
                  } catch (UnknownHostException uhe) {
                     return uhe; 
                  } catch (IOException ioe) { 
                     return ioe;
                  }
               }
            });

         if (obj instanceof Exception) 
            throw (Exception) obj;

         clientSocket = (Socket)obj;

         // connnected, start communicating!

         out = clientSocket.getOutputStream();
         in  = clientSocket.getInputStream();

         if (debug) debugOut("Client writing out: " + remoteRef.getObjectID() + "," + methodHash);

         DataOutputStream dout = new DataOutputStream(out);
         dout.writeLong(remoteRef.getObjectID());
         dout.writeLong(methodHash);

         IxcOutputStream oout = new IxcOutputStream(out, context, false);

         for (int i = 0; i < args.length; i++) {
            oout.writeObject(args[i]);
         }
         if (debug) debugOut("Stub done sending data, waiting for reply");

         IxcInputStream oin = new IxcInputStream(in, context, false);

         boolean didExceptionHappen = oin.readBoolean();
         if (!didExceptionHappen) {
            returnValue = oin.readObject();
         } else {
            exceptionValue = (Exception) oin.readObject();
         }

         if (debug) debugOut("Done with reading result, closing, returning " + returnValue);

         if (debug && exceptionValue != null)
            exceptionValue.printStackTrace();

         oin.close();
         oout.flush();
         oout.close();
      } catch (RemoteException re) { 
         throw re;
      } catch (EOFException eofe) { 
         if (debug) debugOut("Problem in communicating with the other xlet: " + eofe);
         throw new RemoteException("Error in remote method invocation", eofe);
      } catch (java.net.ConnectException ce) { 
         if (debug) debugOut("Cannot connect to the other xlet (Xlet died?)" + ce);
         throw new RemoteException("Cannot connect to the exported xlet (xlet died?)", ce);
      } catch (Exception e) { 
         if (debug) debugOut("General Exception in stub_execute(): " + e);
         throw new RemoteException("Error in remote method invocation", e);
      }

      try {
         clientSocket.shutdownInput();
         clientSocket.shutdownOutput();
         clientSocket.close();
      } catch (IOException e) {}

      if (exceptionValue != null) {
         throw exceptionValue;
      }

      return returnValue;
   }

   private void debugOut(String s) {
      System.out.println(s);
   }

   public String toString() {
      return (this.getClass().getName() + "[" + remoteRef +"]");
   }
}
