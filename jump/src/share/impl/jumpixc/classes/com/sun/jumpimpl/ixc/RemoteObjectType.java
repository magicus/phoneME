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

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.lang.reflect.Method;

import java.rmi.RemoteException;
import javax.microedition.xlet.ixc.StubException;

/*
 * A type of remote object, identified by Remote interfaces 
 * and their methods implemented by a particular remote object.
 */

class RemoteObjectType {

   Class[] remoteInterfaces; // All remote interfaces of "remoteObject" 
                             // Classes
   String[] classNames; // Names of the Classes in the above Class[] array.
   HashMap methodsByID; // All remote methods, by <MethodID, Method>

   public RemoteObjectType(Class remoteObjectClass) throws RemoteException { 
      // List is an ArrayList of interface classes
      this.remoteInterfaces = getRemoteInterfacesFor(remoteObjectClass);

      classNames = new String[remoteInterfaces.length];
      for (int i = 0; i < remoteInterfaces.length; i++) {
         classNames[i] = remoteInterfaces[i].getName();
      }

      fillMethodsHash(this.remoteInterfaces);
   }


   public RemoteObjectType(String[] interfaceNames, ClassLoader loader) 
      throws RemoteException {

      classNames = interfaceNames;
      this.remoteInterfaces = getRemoteInterfacesFor(interfaceNames, loader);

      fillMethodsHash(this.remoteInterfaces);
   }

   private void fillMethodsHash(Class[] clazzlist) 
      throws RemoteException {
      Method[] methods  = getRemoteMethods(clazzlist);
                                                                                
      // Check the validity of all Remote Methods
      verifyRemoteMethods(methods);
                                                                                
      // Remote methods are conformant, generate method hash.
      this.methodsByID = new HashMap();
      for (int i = 0; i < methods.length; i++) {
         Method m = methods[i];
         this.methodsByID.put(new Long(Utils.computeMethodHash(m)), m);
      }
   }
  
   public String[] getRemoteInterfaceNames() {
      return classNames;
   }

   public long[] getMethodIDsAslongs() {
      Long[] longArray = (Long[])methodsByID.keySet().toArray(new Long[]{});

      long[] methodIDs = new long[longArray.length];
      for (int i = 0; i < longArray.length; i++){
         methodIDs[i] = longArray[i].longValue();
      }
      return methodIDs;
   }

   private final static Class theRemoteIF = Remote.class;

   private Class[] getRemoteInterfacesFor(Class cl) {
        HashSet interfaces = new HashSet();
        if (cl.isInterface() && theRemoteIF.isAssignableFrom(cl)) {
           interfaces.add(cl);
           getInterfacesFor(cl, interfaces, false);
        } else {
           getInterfacesFor(cl, interfaces, true);
        }

        return (Class[])(new ArrayList(interfaces)).toArray(new Class[]{});
   }

   private Class[] getRemoteInterfacesFor(String[] interfaceNames, 
                                    ClassLoader loader)
        throws StubException {
        HashSet interfaces = new HashSet();
                                                                                
        try {
           for (int i = 0; i < interfaceNames.length; i++) {
              Class cl = Class.forName(interfaceNames[i], true, loader);
              interfaces.add(cl);
              getInterfacesFor(cl, interfaces, false);
           }
        } catch (ClassNotFoundException e) {
           throw new StubException(e.toString());
        }
                                                                                
        return (Class[])new ArrayList(interfaces).toArray(new Class[]{});
   }

   private void getInterfacesFor(Class cl, HashSet interfaces, boolean isOnlyRemote) {
        if (cl != null) {
            Class[] ifs = cl.getInterfaces();
            for (int i = 0; i < ifs.length; i++) {
               if (isOnlyRemote) {
                  if (theRemoteIF.isAssignableFrom(ifs[i])) {
                     interfaces.add(ifs[i]);
                     getInterfacesFor(ifs[i], interfaces, false);
                     continue;
                  } 
               } else {
                  interfaces.add(ifs[i]);
               }
               getInterfacesFor(ifs[i], interfaces, isOnlyRemote);
            }
       }
    }

 

   private Method[] getRemoteMethods(Class[] clazzes) {
        HashSet methods  = new HashSet();
        for (int i = 0; i < clazzes.length; i++) {
            methods.addAll(Arrays.asList(clazzes[i].getMethods()));
        }
        return (Method[])methods.toArray(new Method[]{}); 
   }

   private void verifyRemoteMethods(Method[] methods)
        throws RemoteException {
        // To check if the methods declared in Remote Interfaces are valid.
        // From the PBP spec:
        // ---
        // Methods declared in a remote interface must be defined as follows:
        // 1. Each method must declare java.rmi.RemoteException in its throws
        //    clause, in addition to any application-specific exceptions.
        // 2. A remote object passed by remote reference as an argument or
        //    return value must be declared as an interface that extends
        //    java.rmi.Remote , and not as an application class that
        //    implements this remote interface.
        // 3. The type of each method argument must either be a remote
        //    interface, a class or interface that implements
        //    java.io.Serializable, or a primitive type.
        // 4. Each return value must either be a remote interface, a class or
        //    interface that implements java.io.Serializable, a primitive type,
        //    or void.
                                                                                       
        boolean doesExceptionThrown;
        int count;
        String errorMsg = "";
        int i = 0;
        next:
        for (; i < methods.length; i++) {
            doesExceptionThrown = false;
            Class[] exceptions = methods[i].getExceptionTypes();
            for (count = 0; count < exceptions.length; count++) {
                if (exceptions[count].equals(RemoteException.class)) {
                    doesExceptionThrown = true;
                    break;
                }
            }
            if (doesExceptionThrown == false) {
                errorMsg +=
                        "Method does not declare java.rmi.RemoteException " +
                        "in it's throws clause : " + methods[i].toString() + "\n";
                continue next;
            }
            Class[] parameters = methods[i].getParameterTypes();
            for (count = 0; count < parameters.length; count++) {
                Class param = parameters[count];
                if (
                    (param.isPrimitive()) ||
                    ((java.io.Serializable.class).isAssignableFrom(param)) ||
                    (param.isInterface() &&
                        theRemoteIF.isAssignableFrom(param))) {;
                } else {
                    errorMsg +=
                            "Method parameter type is not primitive, " +
                            "remote interface, or Serializable : \n" +
                            methods[i].toString() + "\n";
                    continue next;
                }
            }
            Class rt = methods[i].getReturnType();
            if ((rt.isPrimitive()) ||
                (Void.TYPE.equals(rt)) ||
                ((java.io.Serializable.class).isAssignableFrom(rt)) ||
                (rt.isInterface() &&
                    theRemoteIF.isAssignableFrom(rt))) {;
            } else {
                errorMsg +=
                        "Method return type is not primitive, " +
                        "remote interface, Serializable, or void : \n" +
                        methods[i].toString() + "\n";
            }
        }
        if (!errorMsg.equals("")) {
            throw new StubException(errorMsg);
        }
   }
}
