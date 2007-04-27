/*
 * @(#)Utils.java	1.4 06/08/10
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
import java.util.*;
import java.security.*;
import java.io.*;
import java.rmi.*;
import javax.microedition.xlet.ixc.StubException;

import com.sun.jump.os.JUMPOSInterface;

/* 
 * This is a class with various (mostly static) utility methods.
 */

public class Utils {

    /**
     * Compute the "method hash" of a remote method.  The method hash
     * is a long containing the first 64 bits of the SHA digest from
     * the UTF encoded string of the method name and descriptor.
     */
    static long computeMethodHash(Method m) {
        long hash = 0;
        ByteArrayOutputStream sink = new ByteArrayOutputStream(127);
        try {
            MessageDigest md = MessageDigest.getInstance("SHA");
            DataOutputStream out = new DataOutputStream(
                new DigestOutputStream(sink, md));
                                                                             
            String s = getMethodNameAndDescriptor(m);
                                                                             
            out.writeUTF(s);
                                                                             
            // use only the first 64 bits of the digest for the hash
            out.flush();
            byte hasharray[] = md.digest();
            for (int i = 0; i < Math.min(8, hasharray.length); i++) {
                hash += ((long) (hasharray[i] & 0xFF)) << (i * 8);
            }
        } catch (IOException ignore) {
            /* can't happen, but be deterministic anyway. */
            hash = -1;
        } catch (NoSuchAlgorithmException complain) {
            throw new SecurityException(complain.getMessage());
        }
        return hash;
    }

    /**
     * Return a string consisting of the given method's name followed by
     * its "method descriptor", as appropriate for use in the computation
     * of the "method hash".
     *
     * See section 4.3.3 of The Java(TM) Virtual Machine Specification
     * for the definition of a "method descriptor".
     */
    private static String getMethodNameAndDescriptor(Method m) {
        StringBuffer desc = new StringBuffer(m.getName());
        desc.append('(');
        Class[] paramTypes = m.getParameterTypes();
        for (int i = 0; i < paramTypes.length; i++) {
            desc.append(getTypeDescriptor(paramTypes[i]));
        }
        desc.append(')');
        Class returnType = m.getReturnType();
        if (returnType == void.class) { // optimization: handle void here
            desc.append('V');
        } else {
            desc.append(getTypeDescriptor(returnType));
        }
        return desc.toString();
    }

    /**
     * Get the descriptor of a particular type, as appropriate for either
     * a parameter or return type in a method descriptor.
     */
    private static String getTypeDescriptor(Class type) {
        if (type.isPrimitive()) {
            if (type == int.class) {
                return "I";
            } else if (type == boolean.class) {
                return "Z";
            } else if (type == byte.class) {
                return "B";
            } else if (type == char.class) {
                return "C";
            } else if (type == short.class) {
                return "S";
            } else if (type == long.class) {
                return "J";
            } else if (type == float.class) {
                return "F";
            } else if (type == double.class) {
                return "D";
            } else if (type == void.class) {
                return "V";
            } else {
                throw new Error("unrecognized primitive type: " + type);
            }
        } else if (type.isArray()) {
            /*
             * According to JLS 20.3.2, the getName() method on Class does
             * return the VM type descriptor format for array classes (only);             
             * using that should be quicker than the otherwise obvious code:
             *
             *     return "[" + getTypeDescriptor(type.getComponentType());
             */
            return type.getName().replace('.', '/');
        } else {
            return "L" + type.getName().replace('.', '/') + ";";
        }
    }

    private static int mtaskServerID = -1;
    private static int mtaskClientID = -1;

    public static int getMtaskServerID() {

       if (mtaskServerID == -1) {
          try {

             JUMPOSInterface os = JUMPOSInterface.getInstance();
             mtaskServerID =  os.getExecutiveProcessID();
  
             //new Exception("serverID="+mtaskServerID).printStackTrace();

          } catch (Exception e) {
             e.printStackTrace();
             mtaskServerID = 0;
          }
       }

       return mtaskServerID;
    }

    public static int getMtaskClientID() {

       if (mtaskClientID == -1) {
          try {

             JUMPOSInterface os = JUMPOSInterface.getInstance();
             mtaskClientID = os.getProcessID();

             //new Exception("clientID="+mtaskClientID).printStackTrace();
          } catch (Exception e) {
             e.printStackTrace();
             mtaskClientID = 0;
          }
       }

       return mtaskClientID;
    }
}
