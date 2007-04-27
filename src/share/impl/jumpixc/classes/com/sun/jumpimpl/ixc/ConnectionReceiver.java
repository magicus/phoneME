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

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.ServerSocket;
import java.net.Socket;
import java.rmi.Remote;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.security.PrivilegedActionException;
import java.util.HashMap;
import sun.security.action.GetIntegerAction;

import javax.microedition.xlet.*;
import javax.microedition.xlet.ixc.*;

/* 
 * This is VM-wide singleton class for accepting incoming
 * connections from other clients, with it's ServerSocket.
 * It's used for receiving IXC method invocation requests.
 * This ServerSocket's port number is included in the RemoteRef
 * of the objects exported by this VM's xlet(s).
 */
   
public class ConnectionReceiver implements Runnable {

   private static int execVMServicePort;
   private static boolean initialized = false;
   private static ServerSocket serverSocket;
   private static boolean debug = false; // Enable/disable debug output

   private ConnectionReceiver(final int port) {
      serverSocket = (ServerSocket) AccessController.doPrivileged(
         new PrivilegedAction() {
            public Object run() {
               ServerSocket socket = null;
               try {
                  if (port == 0) {
                     socket = new ServerSocket();
                     socket.bind(null);
                  } else {
                     socket = new ServerSocket(port);
                  }
               } catch (IOException e) { 
                  System.err.println("Error opening ServerSocket");
                  e.printStackTrace();
               }
               return socket;
            }
         });

       if (serverSocket != null)
          new Thread(this).start();
    }

    public synchronized static void setExecVMServicePort(int portNumber) {
       if (initialized) { 
          throw new InternalError("ExecutiveVM port already set: " + getExecVMServicePort());
       }
       execVMServicePort = portNumber;
       initialized = true;
    }

    public synchronized static int getExecVMServicePort() {
       if (!initialized) { 
          throw new InternalError("ExecutiveVM port not yet set");
       }
       return execVMServicePort;
    }

    /*
     * Starts the executive VM ServerSocket.
     * Should only be called in the ExecutiveVM.
     */
    public static void startExecVMService() {
       setExecVMServicePort(getLocalServicePort());
    }

    /*
     * Starts the ConnectionReceiver if necessary, and returns the local
     * VM's ServerSocket. 
     */
    static int getLocalServicePort() {
       if (serverSocket == null) {
          new ConnectionReceiver(0);
       }

       return serverSocket.getLocalPort();
    }

    private static void debugOut(String s) {
       System.out.println("ConnectionReceiver: " + s);
    }


   // Waits for the connection request to come to this ServerSocket.
   public void run() {
      Socket clientSocket = null;

      while (true) {
        if (debug) debugOut("ConnectionReceiver waiting for new connection");
           
        final ServerSocket finalSocket = serverSocket;
        clientSocket = (Socket) AccessController.doPrivileged(
           new PrivilegedAction() {
              public Object run() {
                 try {
                    Socket s = serverSocket.accept(); 
                    s.setReuseAddress(true);
                    return s;
                 } catch (IOException e) {
                    e.printStackTrace();
                 }
                 return null;
              }
           });

        if (clientSocket != null) {
           // Connected - execute the method invocation in this vm.
           if (debug) debugOut("Connected with " +  clientSocket);
           Runnable executor = new RemoteMethodExecutor(clientSocket);
           WorkerThreadPool.execute(Thread.NORM_PRIORITY, executor);
        }
      }
    }

    // Handles each incoming connection request.
    class RemoteMethodExecutor implements Runnable {
       Socket clientSocket;

       public RemoteMethodExecutor(Socket s) {
          clientSocket = s;
       }
  
       // Executed by the worker thread. 
       public void run() {

          InputStream in = null;
          OutputStream out = null;

          boolean isExecutiveIxcRegistry = false;

          try {

             /**
              * Invocation request comes in this order.
              * 1.The object ID (long).
              * 2.The method ID (long).
              * 3.Parameters marshalled out (if needed)
              */
             in = clientSocket.getInputStream();

             DataInputStream din = new DataInputStream(in);
             long objectID = din.readLong();
             long methodID = din.readLong();

             Object    remoteObject = null;
             Method    method = null;
             Object    returnValue = null;
             boolean   hasExceptionThrown = false; 

             if (debug) debugOut("Obtained ObjectID and methodID");

             // First, find the object.
             ExportedObject exportedObject = ExportedObject.findExportedObject(objectID);

             if (exportedObject != null) {
                remoteObject = exportedObject.remoteObject;
                isExecutiveIxcRegistry = 
                   (remoteObject instanceof JUMPExecIxcRegistryRemote); 
             }

             if (debug) debugOut("Found corresponding Object " + remoteObject + ", isExecutiveIxcRegistry=" + isExecutiveIxcRegistry);

             if (remoteObject == null) {
                /* problem here */
                hasExceptionThrown = true; 
                returnValue = 
                   new StubException("Cannot find corresponding ExportedObject:" + objectID);
             } 

             // Next, find the method 
             if (remoteObject != null) {
                method = exportedObject.findExportedMethod(methodID);
                if (method == null) {
                   /* problem here */
                   hasExceptionThrown = true; 
                   returnValue = 
                      new StubException("Cannot find corresponding method:" + methodID);
                } 
             }
 
             if (debug) debugOut("Found corresponding method " + method);

             // If both object and method are found, read params and invoke
             if (method != null) {

                Class[]  paramTypes = method.getParameterTypes();
                Object[] paramObjects = new Object[paramTypes.length];
           
                try {
                   if (paramTypes.length > 0) {
           
                      IxcInputStream oin = new IxcInputStream(in, 
                               exportedObject.context, isExecutiveIxcRegistry);

                      for (int i = 0; i < paramObjects.length; i++) {
                         paramObjects[i] = oin.readObject();
                         if (debug) debugOut("Read param " + i + " : " + paramObjects[i]);
                      }
                   }

                   // Need to find out the client's AccessControlContext,
                   // so that the method invocation can happen in the proper
                   // security context.
                   AccessControlContext acc = AccessController.getContext();
                   returnValue = invokeMethod(method, 
                                              remoteObject, 
                                              paramObjects, 
                                              acc); 

                } catch (StubException se) { // Error in importing parameters
                   hasExceptionThrown = true;
                   returnValue = se;
                } catch (ClassCastException cce) {  // Error in parameter deserialization
                   hasExceptionThrown = true;
                   returnValue = cce;
                } catch (InvocationTargetException ite) { // Method invocation error
                   hasExceptionThrown = true;
                   returnValue = ite.getCause();
                } catch (IllegalAccessException iae) {    // Method invocation error
                   hasExceptionThrown = true;
                   returnValue = iae;
                } catch (IllegalArgumentException iage) { // Method invocation error
                   hasExceptionThrown = true;
                   returnValue = iage;
                } catch (Throwable e) {
                   hasExceptionThrown = true;
                   returnValue = e;
                }
             }
    

             if (debug) debugOut("Done invoking, marshalling out results");
             if (debug) debugOut("hasExceptionThrown = " + hasExceptionThrown);
             if (debug && hasExceptionThrown) debugOut("Exception: " + returnValue);

             out = clientSocket.getOutputStream();

             /**
              * Invocation results are sent out in this order.
              * 1.Whether the method invocation ended abnormally (bool).
              * 2.Return value (In normal ending, object or NullObject
              *   for null return, else the corresponding Throwable)
              */
             
             IxcOutputStream oout = new IxcOutputStream(out, 
                                exportedObject.context, isExecutiveIxcRegistry);

             oout.writeBoolean(hasExceptionThrown); 
             oout.writeObject(returnValue);
           
             if (debug) debugOut("Done with this RMI, closing the output");
             Thread.sleep(200);

             oout.flush();
             in.close();
             oout.close();

          } catch (Exception e) {
             System.out.println("Caught exception while processing method invocation"); 
             e.printStackTrace();
          }
  
          try {
             clientSocket.shutdownInput();
             clientSocket.shutdownOutput();
             clientSocket.close();
          } catch (Exception e) {
             //e.printStackTrace();
          }
       }
    }

    private Object invokeMethod(final Method method, 
                                final Object remoteObject, 
                                final Object[] paramObjects, 
                                final AccessControlContext xletACC) 
          throws InvocationTargetException, 
                 IllegalAccessException, 
                 IllegalArgumentException {
  
       try {
          return AccessController.doPrivileged(
             new PrivilegedExceptionAction() {
                public Object run() throws InvocationTargetException, IllegalAccessException {
                   return method.invoke(remoteObject, paramObjects);
                }
             }
          , xletACC);
       } catch (PrivilegedActionException e) {
          Exception ex = e.getException();
          if (ex instanceof InvocationTargetException)
            throw (InvocationTargetException) ex;
          if (ex instanceof IllegalAccessException)
            throw (IllegalAccessException) ex;
          if (ex instanceof IllegalArgumentException)
            throw (IllegalArgumentException) ex;
          
          throw new RuntimeException("Error in invoking: " + ex);
       }
    }
}

// Placeholder for sending NULL across the socket connection.
class NullObject implements java.io.Serializable {}
