/*
 * @(#)IxcClassLoader.java	1.29 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package com.sun.xlet.ixc;

import java.rmi.Remote;
import java.rmi.RemoteException;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.StubException;
import java.net.URL;
import java.lang.reflect.Method;
import java.lang.reflect.Array;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.HashSet;
import java.util.Iterator;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.security.AccessController;
import java.security.PrivilegedAction;

/**
 * Class loader for Xlets.  This is needed to automatically generate stub
 * classes for IXC.
 * <p>
 * Suppose, for example, that a user class implemens a remote interface UserIF,
 * and that interface contains two methods, void frob(Something), and
 * int glorp(float).  This classloader will automatically generate 
 * a remote stub.  The stub will be equivalent to the following class:
 * <pre>
 *
 *  package com.sun.xlet;
 *
 *  import java.lang.reflect.Method;
 *
 *  public final class StubClass$$42 extends com.sun.xlet.WrappedRemote 
 *		implements UserIF {
 *
 *      private static Method com_sun_xlet_method0;
 *      private static Method com_sun_xlet_method1;
 *
 *
 *      public static void com_sun_xlet_init(Method findMethodMethod) 
 *              throws Exception {
 *       	// findMethodMethod is Utils.findMethod for the ClassLoader
 *		// where the *target* Remote object lives.
 *       
 *          if (com_sun_xlet_method0 != null) {
 *	        return;
 *          }
 *          com_sun_xlet_method0 = (Method) findMethodMethod.invoke(null,
 *		new Object[] { "UserIF", "frob", new Object[] { "Something" }});
 *          com_sun_xlet_method1 = (Method) findMethodMethod.invoke(null,
 *		new Object[] { "UserIF", "glorp", 
 *                             new Object[] { java.lang.Float.TYPE }});
 *      }
 *
 *      public static void com_sun_xlet_destroy() {
 *          com_sun_xlet_method0 = null;
 *          com_sun_xlet_method1 = null;
 *      }
 *
 *      public StubClass$$42(Remote target, ImportRegistryImpl registry,
 *			     RegistryKey key) {
 *	    super(target, registry, key);
 *	}
 *
 *      public void frob(Something arg1) throws org.dvb.ixc.RemoteException {
 *          com_sun_xlet_execute(com_sun_xlet_method0, 
 *				   new Object[] { arg1 });
 *      }
 *
 *      public int glorp(float arg1) throws org.dvb.ixc.RemoteException {
 *	    Object r = com_sun_xlet_execute(com_sun_xlet_method1,
 *					      new Object[] { new Float(arg1) });
 *          return ((Integer) r).intValue();
 *      }
 *  }
 *
 * </pre>
 * <p>
 * @@ Add the synthetic attribute
 * @@ Do this with a security manager installed
 * @@ Make exception handling consistent with 1.2 behavior.  Specifically,
 *    RemoteExceptions should be cloned and wrapped, RuntimeExceptions
 *    should be cloned and re-thrown, checked exception that appear in the
 *    interface method's signature should be cloned and re-thrown (probably
 *    with our deprecated friend, Thread.stop(Throwable)), and unexpected
 *    checked exceptions should be cloned and wrapped (there's some exception
 *    under java.rmi specifically for this).
 */

/*
 * This classloader is deligated from an Xlet's class loader and generates 
 * a stub class to be used for inter-xlet communication
 */

class IxcClassLoader extends ClassLoader {
    private static Class theRemoteIF = Remote.class;
    private Hashtable generated = new Hashtable(11);
    // Key is remote class, value is stub we generate.
    private 
    static	// for debugging
    int nextStubNumber = 1;
    // ImportRegistryImpl.copyObject needs to
    // deserialize objects in the context of this classloader.  To do this,
    // it defines a class called com.sun.xlet.DeserializeUtility, with
    // a single static method, deserialize(byte[]).  The source for this
    // class, and a little utility for converting the .class to a
    // byte[], can be found in ~/misc/com/sun/xlet.
    private Method deserializeMethod = null;
    private Method findMethodMethod = null;
    private Class utilsClass = null;
    private static byte[] utilsClassBody = {  //	    The .class file
            (byte) 0xca, (byte) 0xfe, (byte) 0xba, (byte) 0xbe, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x2e, 
            (byte) 0x00, (byte) 0x3d, (byte) 0x0a, (byte) 0x00, 
            (byte) 0x11, (byte) 0x00, (byte) 0x1f, (byte) 0x07, 
            (byte) 0x00, (byte) 0x20, (byte) 0x07, (byte) 0x00, 
            (byte) 0x21, (byte) 0x0a, (byte) 0x00, (byte) 0x03, 
            (byte) 0x00, (byte) 0x22, (byte) 0x0a, (byte) 0x00, 
            (byte) 0x02, (byte) 0x00, (byte) 0x23, (byte) 0x0a, 
            (byte) 0x00, (byte) 0x02, (byte) 0x00, (byte) 0x24, 
            (byte) 0x07, (byte) 0x00, (byte) 0x25, (byte) 0x07, 
            (byte) 0x00, (byte) 0x26, (byte) 0x07, (byte) 0x00, 
            (byte) 0x27, (byte) 0x0a, (byte) 0x00, (byte) 0x08, 
            (byte) 0x00, (byte) 0x28, (byte) 0x0a, (byte) 0x00, 
            (byte) 0x08, (byte) 0x00, (byte) 0x29, (byte) 0x07, 
            (byte) 0x00, (byte) 0x2a, (byte) 0x07, (byte) 0x00, 
            (byte) 0x2b, (byte) 0x0a, (byte) 0x00, (byte) 0x0c, 
            (byte) 0x00, (byte) 0x2c, (byte) 0x0a, (byte) 0x00, 
            (byte) 0x0d, (byte) 0x00, (byte) 0x2d, (byte) 0x07, 
            (byte) 0x00, (byte) 0x2e, (byte) 0x07, (byte) 0x00, 
            (byte) 0x2f, (byte) 0x01, (byte) 0x00, (byte) 0x06, 
            (byte) 0x3c, (byte) 0x69, (byte) 0x6e, (byte) 0x69, 
            (byte) 0x74, (byte) 0x3e, (byte) 0x01, (byte) 0x00, 
            (byte) 0x03, (byte) 0x28, (byte) 0x29, (byte) 0x56, 
            (byte) 0x01, (byte) 0x00, (byte) 0x04, (byte) 0x43, 
            (byte) 0x6f, (byte) 0x64, (byte) 0x65, (byte) 0x01, 
            (byte) 0x00, (byte) 0x0f, (byte) 0x4c, (byte) 0x69, 
            (byte) 0x6e, (byte) 0x65, (byte) 0x4e, (byte) 0x75, 
            (byte) 0x6d, (byte) 0x62, (byte) 0x65, (byte) 0x72, 
            (byte) 0x54, (byte) 0x61, (byte) 0x62, (byte) 0x6c, 
            (byte) 0x65, (byte) 0x01, (byte) 0x00, (byte) 0x0b, 
            (byte) 0x64, (byte) 0x65, (byte) 0x73, (byte) 0x65, 
            (byte) 0x72, (byte) 0x69, (byte) 0x61, (byte) 0x6c, 
            (byte) 0x69, (byte) 0x7a, (byte) 0x65, (byte) 0x01, 
            (byte) 0x00, (byte) 0x1a, (byte) 0x28, (byte) 0x5b, 
            (byte) 0x42, (byte) 0x29, (byte) 0x4c, (byte) 0x6a, 
            (byte) 0x61, (byte) 0x76, (byte) 0x61, (byte) 0x2f, 
            (byte) 0x69, (byte) 0x6f, (byte) 0x2f, (byte) 0x53, 
            (byte) 0x65, (byte) 0x72, (byte) 0x69, (byte) 0x61, 
            (byte) 0x6c, (byte) 0x69, (byte) 0x7a, (byte) 0x61, 
            (byte) 0x62, (byte) 0x6c, (byte) 0x65, (byte) 0x3b, 
            (byte) 0x01, (byte) 0x00, (byte) 0x0a, (byte) 0x45, 
            (byte) 0x78, (byte) 0x63, (byte) 0x65, (byte) 0x70, 
            (byte) 0x74, (byte) 0x69, (byte) 0x6f, (byte) 0x6e, 
            (byte) 0x73, (byte) 0x07, (byte) 0x00, (byte) 0x30, 
            (byte) 0x07, (byte) 0x00, (byte) 0x31, (byte) 0x01, 
            (byte) 0x00, (byte) 0x0a, (byte) 0x66, (byte) 0x69, 
            (byte) 0x6e, (byte) 0x64, (byte) 0x4d, (byte) 0x65, 
            (byte) 0x74, (byte) 0x68, (byte) 0x6f, (byte) 0x64, 
            (byte) 0x01, (byte) 0x00, (byte) 0x4b, (byte) 0x28, 
            (byte) 0x4c, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x53, 
            (byte) 0x74, (byte) 0x72, (byte) 0x69, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x3b, (byte) 0x4c, (byte) 0x6a, 
            (byte) 0x61, (byte) 0x76, (byte) 0x61, (byte) 0x2f, 
            (byte) 0x6c, (byte) 0x61, (byte) 0x6e, (byte) 0x67, 
            (byte) 0x2f, (byte) 0x53, (byte) 0x74, (byte) 0x72, 
            (byte) 0x69, (byte) 0x6e, (byte) 0x67, (byte) 0x3b, 
            (byte) 0x5b, (byte) 0x4c, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x6e, (byte) 0x67, (byte) 0x2f, 
            (byte) 0x4f, (byte) 0x62, (byte) 0x6a, (byte) 0x65, 
            (byte) 0x63, (byte) 0x74, (byte) 0x3b, (byte) 0x29, 
            (byte) 0x4c, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x4f, 
            (byte) 0x62, (byte) 0x6a, (byte) 0x65, (byte) 0x63, 
            (byte) 0x74, (byte) 0x3b, (byte) 0x01, (byte) 0x00, 
            (byte) 0x0a, (byte) 0x53, (byte) 0x6f, (byte) 0x75, 
            (byte) 0x72, (byte) 0x63, (byte) 0x65, (byte) 0x46, 
            (byte) 0x69, (byte) 0x6c, (byte) 0x65, (byte) 0x01, 
            (byte) 0x00, (byte) 0x0a, (byte) 0x55, (byte) 0x74, 
            (byte) 0x69, (byte) 0x6c, (byte) 0x73, (byte) 0x2e, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x0c, (byte) 0x00, (byte) 0x12, (byte) 0x00, 
            (byte) 0x13, (byte) 0x01, (byte) 0x00, (byte) 0x19, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x69, (byte) 0x6f, (byte) 0x2f, 
            (byte) 0x4f, (byte) 0x62, (byte) 0x6a, (byte) 0x65, 
            (byte) 0x63, (byte) 0x74, (byte) 0x49, (byte) 0x6e, 
            (byte) 0x70, (byte) 0x75, (byte) 0x74, (byte) 0x53, 
            (byte) 0x74, (byte) 0x72, (byte) 0x65, (byte) 0x61, 
            (byte) 0x6d, (byte) 0x01, (byte) 0x00, (byte) 0x1c, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x69, (byte) 0x6f, (byte) 0x2f, 
            (byte) 0x42, (byte) 0x79, (byte) 0x74, (byte) 0x65, 
            (byte) 0x41, (byte) 0x72, (byte) 0x72, (byte) 0x61, 
            (byte) 0x79, (byte) 0x49, (byte) 0x6e, (byte) 0x70, 
            (byte) 0x75, (byte) 0x74, (byte) 0x53, (byte) 0x74, 
            (byte) 0x72, (byte) 0x65, (byte) 0x61, (byte) 0x6d, 
            (byte) 0x0c, (byte) 0x00, (byte) 0x12, (byte) 0x00, 
            (byte) 0x32, (byte) 0x0c, (byte) 0x00, (byte) 0x12, 
            (byte) 0x00, (byte) 0x33, (byte) 0x0c, (byte) 0x00, 
            (byte) 0x34, (byte) 0x00, (byte) 0x35, (byte) 0x01, 
            (byte) 0x00, (byte) 0x14, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x69, 
            (byte) 0x6f, (byte) 0x2f, (byte) 0x53, (byte) 0x65, 
            (byte) 0x72, (byte) 0x69, (byte) 0x61, (byte) 0x6c, 
            (byte) 0x69, (byte) 0x7a, (byte) 0x61, (byte) 0x62, 
            (byte) 0x6c, (byte) 0x65, (byte) 0x01, (byte) 0x00, 
            (byte) 0x0f, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x43, 
            (byte) 0x6c, (byte) 0x61, (byte) 0x73, (byte) 0x73, 
            (byte) 0x01, (byte) 0x00, (byte) 0x10, (byte) 0x6a, 
            (byte) 0x61, (byte) 0x76, (byte) 0x61, (byte) 0x2f, 
            (byte) 0x6c, (byte) 0x61, (byte) 0x6e, (byte) 0x67, 
            (byte) 0x2f, (byte) 0x53, (byte) 0x74, (byte) 0x72, 
            (byte) 0x69, (byte) 0x6e, (byte) 0x67, (byte) 0x0c, 
            (byte) 0x00, (byte) 0x36, (byte) 0x00, (byte) 0x37, 
            (byte) 0x0c, (byte) 0x00, (byte) 0x38, (byte) 0x00, 
            (byte) 0x39, (byte) 0x01, (byte) 0x00, (byte) 0x13, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x6c, (byte) 0x61, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x2f, (byte) 0x45, (byte) 0x78, 
            (byte) 0x63, (byte) 0x65, (byte) 0x70, (byte) 0x74, 
            (byte) 0x69, (byte) 0x6f, (byte) 0x6e, (byte) 0x01, 
            (byte) 0x00, (byte) 0x1a, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x6e, (byte) 0x67, (byte) 0x2f, 
            (byte) 0x52, (byte) 0x75, (byte) 0x6e, (byte) 0x74, 
            (byte) 0x69, (byte) 0x6d, (byte) 0x65, (byte) 0x45, 
            (byte) 0x78, (byte) 0x63, (byte) 0x65, (byte) 0x70, 
            (byte) 0x74, (byte) 0x69, (byte) 0x6f, (byte) 0x6e, 
            (byte) 0x0c, (byte) 0x00, (byte) 0x3a, (byte) 0x00, 
            (byte) 0x3b, (byte) 0x0c, (byte) 0x00, (byte) 0x12, 
            (byte) 0x00, (byte) 0x3c, (byte) 0x01, (byte) 0x00, 
            (byte) 0x12, (byte) 0x63, (byte) 0x6f, (byte) 0x6d, 
            (byte) 0x2f, (byte) 0x73, (byte) 0x75, (byte) 0x6e, 
            (byte) 0x2f, (byte) 0x78, (byte) 0x6c, (byte) 0x65, 
            (byte) 0x74, (byte) 0x2f, (byte) 0x55, (byte) 0x74, 
            (byte) 0x69, (byte) 0x6c, (byte) 0x73, (byte) 0x01, 
            (byte) 0x00, (byte) 0x10, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x6e, (byte) 0x67, (byte) 0x2f, 
            (byte) 0x4f, (byte) 0x62, (byte) 0x6a, (byte) 0x65, 
            (byte) 0x63, (byte) 0x74, (byte) 0x01, (byte) 0x00, 
            (byte) 0x13, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x69, (byte) 0x6f, 
            (byte) 0x2f, (byte) 0x49, (byte) 0x4f, (byte) 0x45, 
            (byte) 0x78, (byte) 0x63, (byte) 0x65, (byte) 0x70, 
            (byte) 0x74, (byte) 0x69, (byte) 0x6f, (byte) 0x6e, 
            (byte) 0x01, (byte) 0x00, (byte) 0x20, (byte) 0x6a, 
            (byte) 0x61, (byte) 0x76, (byte) 0x61, (byte) 0x2f, 
            (byte) 0x6c, (byte) 0x61, (byte) 0x6e, (byte) 0x67, 
            (byte) 0x2f, (byte) 0x43, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x73, (byte) 0x73, (byte) 0x4e, (byte) 0x6f, 
            (byte) 0x74, (byte) 0x46, (byte) 0x6f, (byte) 0x75, 
            (byte) 0x6e, (byte) 0x64, (byte) 0x45, (byte) 0x78, 
            (byte) 0x63, (byte) 0x65, (byte) 0x70, (byte) 0x74, 
            (byte) 0x69, (byte) 0x6f, (byte) 0x6e, (byte) 0x01, 
            (byte) 0x00, (byte) 0x05, (byte) 0x28, (byte) 0x5b, 
            (byte) 0x42, (byte) 0x29, (byte) 0x56, (byte) 0x01, 
            (byte) 0x00, (byte) 0x18, (byte) 0x28, (byte) 0x4c, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x69, (byte) 0x6f, (byte) 0x2f, 
            (byte) 0x49, (byte) 0x6e, (byte) 0x70, (byte) 0x75, 
            (byte) 0x74, (byte) 0x53, (byte) 0x74, (byte) 0x72, 
            (byte) 0x65, (byte) 0x61, (byte) 0x6d, (byte) 0x3b, 
            (byte) 0x29, (byte) 0x56, (byte) 0x01, (byte) 0x00, 
            (byte) 0x0a, (byte) 0x72, (byte) 0x65, (byte) 0x61, 
            (byte) 0x64, (byte) 0x4f, (byte) 0x62, (byte) 0x6a, 
            (byte) 0x65, (byte) 0x63, (byte) 0x74, (byte) 0x01, 
            (byte) 0x00, (byte) 0x14, (byte) 0x28, (byte) 0x29, 
            (byte) 0x4c, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x4f, 
            (byte) 0x62, (byte) 0x6a, (byte) 0x65, (byte) 0x63, 
            (byte) 0x74, (byte) 0x3b, (byte) 0x01, (byte) 0x00, 
            (byte) 0x07, (byte) 0x66, (byte) 0x6f, (byte) 0x72, 
            (byte) 0x4e, (byte) 0x61, (byte) 0x6d, (byte) 0x65, 
            (byte) 0x01, (byte) 0x00, (byte) 0x25, (byte) 0x28, 
            (byte) 0x4c, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x53, 
            (byte) 0x74, (byte) 0x72, (byte) 0x69, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x3b, (byte) 0x29, (byte) 0x4c, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x6c, (byte) 0x61, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x2f, (byte) 0x43, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x73, (byte) 0x73, (byte) 0x3b, 
            (byte) 0x01, (byte) 0x00, (byte) 0x09, (byte) 0x67, 
            (byte) 0x65, (byte) 0x74, (byte) 0x4d, (byte) 0x65, 
            (byte) 0x74, (byte) 0x68, (byte) 0x6f, (byte) 0x64, 
            (byte) 0x01, (byte) 0x00, (byte) 0x40, (byte) 0x28, 
            (byte) 0x4c, (byte) 0x6a, (byte) 0x61, (byte) 0x76, 
            (byte) 0x61, (byte) 0x2f, (byte) 0x6c, (byte) 0x61, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x2f, (byte) 0x53, 
            (byte) 0x74, (byte) 0x72, (byte) 0x69, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x3b, (byte) 0x5b, (byte) 0x4c, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x6c, (byte) 0x61, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x2f, (byte) 0x43, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x73, (byte) 0x73, (byte) 0x3b, 
            (byte) 0x29, (byte) 0x4c, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x6e, (byte) 0x67, (byte) 0x2f, 
            (byte) 0x72, (byte) 0x65, (byte) 0x66, (byte) 0x6c, 
            (byte) 0x65, (byte) 0x63, (byte) 0x74, (byte) 0x2f, 
            (byte) 0x4d, (byte) 0x65, (byte) 0x74, (byte) 0x68, 
            (byte) 0x6f, (byte) 0x64, (byte) 0x3b, (byte) 0x01, 
            (byte) 0x00, (byte) 0x08, (byte) 0x74, (byte) 0x6f, 
            (byte) 0x53, (byte) 0x74, (byte) 0x72, (byte) 0x69, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x01, (byte) 0x00, 
            (byte) 0x14, (byte) 0x28, (byte) 0x29, (byte) 0x4c, 
            (byte) 0x6a, (byte) 0x61, (byte) 0x76, (byte) 0x61, 
            (byte) 0x2f, (byte) 0x6c, (byte) 0x61, (byte) 0x6e, 
            (byte) 0x67, (byte) 0x2f, (byte) 0x53, (byte) 0x74, 
            (byte) 0x72, (byte) 0x69, (byte) 0x6e, (byte) 0x67, 
            (byte) 0x3b, (byte) 0x01, (byte) 0x00, (byte) 0x15, 
            (byte) 0x28, (byte) 0x4c, (byte) 0x6a, (byte) 0x61, 
            (byte) 0x76, (byte) 0x61, (byte) 0x2f, (byte) 0x6c, 
            (byte) 0x61, (byte) 0x6e, (byte) 0x67, (byte) 0x2f, 
            (byte) 0x53, (byte) 0x74, (byte) 0x72, (byte) 0x69, 
            (byte) 0x6e, (byte) 0x67, (byte) 0x3b, (byte) 0x29, 
            (byte) 0x56, (byte) 0x00, (byte) 0x21, (byte) 0x00, 
            (byte) 0x10, (byte) 0x00, (byte) 0x11, (byte) 0x00, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, 
            (byte) 0x03, (byte) 0x00, (byte) 0x01, (byte) 0x00, 
            (byte) 0x12, (byte) 0x00, (byte) 0x13, (byte) 0x00, 
            (byte) 0x01, (byte) 0x00, (byte) 0x14, (byte) 0x00, 
            (byte) 0x00, (byte) 0x00, (byte) 0x1d, (byte) 0x00, 
            (byte) 0x01, (byte) 0x00, (byte) 0x01, (byte) 0x00, 
            (byte) 0x00, (byte) 0x00, (byte) 0x05, (byte) 0x2a, 
            (byte) 0xb7, (byte) 0x00, (byte) 0x01, (byte) 0xb1, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x01, 
            (byte) 0x00, (byte) 0x15, (byte) 0x00, (byte) 0x00, 
            (byte) 0x00, (byte) 0x06, (byte) 0x00, (byte) 0x01, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x08, 
            (byte) 0x00, (byte) 0x09, (byte) 0x00, (byte) 0x16, 
            (byte) 0x00, (byte) 0x17, (byte) 0x00, (byte) 0x02, 
            (byte) 0x00, (byte) 0x14, (byte) 0x00, (byte) 0x00, 
            (byte) 0x00, (byte) 0x34, (byte) 0x00, (byte) 0x05, 
            (byte) 0x00, (byte) 0x02, (byte) 0x00, (byte) 0x00, 
            (byte) 0x00, (byte) 0x18, (byte) 0xbb, (byte) 0x00, 
            (byte) 0x02, (byte) 0x59, (byte) 0xbb, (byte) 0x00, 
            (byte) 0x03, (byte) 0x59, (byte) 0x2a, (byte) 0xb7, 
            (byte) 0x00, (byte) 0x04, (byte) 0xb7, (byte) 0x00, 
            (byte) 0x05, (byte) 0x4c, (byte) 0x2b, (byte) 0xb6, 
            (byte) 0x00, (byte) 0x06, (byte) 0xc0, (byte) 0x00, 
            (byte) 0x07, (byte) 0xb0, (byte) 0x00, (byte) 0x00, 
            (byte) 0x00, (byte) 0x01, (byte) 0x00, (byte) 0x15, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x0a, 
            (byte) 0x00, (byte) 0x02, (byte) 0x00, (byte) 0x00, 
            (byte) 0x00, (byte) 0x0b, (byte) 0x00, (byte) 0x10, 
            (byte) 0x00, (byte) 0x0d, (byte) 0x00, (byte) 0x18, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x06, 
            (byte) 0x00, (byte) 0x02, (byte) 0x00, (byte) 0x19, 
            (byte) 0x00, (byte) 0x1a, (byte) 0x00, (byte) 0x09, 
            (byte) 0x00, (byte) 0x1b, (byte) 0x00, (byte) 0x1c, 
            (byte) 0x00, (byte) 0x01, (byte) 0x00, (byte) 0x14, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x93, 
            (byte) 0x00, (byte) 0x04, (byte) 0x00, (byte) 0x05, 
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x53, 
            (byte) 0x2c, (byte) 0xbe, (byte) 0xbd, (byte) 0x00, 
            (byte) 0x08, (byte) 0x4e, (byte) 0x03, (byte) 0x36, 
            (byte) 0x04, (byte) 0x15, (byte) 0x04, (byte) 0x2c, 
            (byte) 0xbe, (byte) 0xa2, (byte) 0x00, (byte) 0x2f, 
            (byte) 0x2c, (byte) 0x15, (byte) 0x04, (byte) 0x32, 
            (byte) 0xc1, (byte) 0x00, (byte) 0x08, (byte) 0x99, 
            (byte) 0x00, (byte) 0x11, (byte) 0x2d, (byte) 0x15, 
            (byte) 0x04, (byte) 0x2c, (byte) 0x15, (byte) 0x04, 
            (byte) 0x32, (byte) 0xc0, (byte) 0x00, (byte) 0x08, 
            (byte) 0x53, (byte) 0xa7, (byte) 0x00, (byte) 0x11, 
            (byte) 0x2d, (byte) 0x15, (byte) 0x04, (byte) 0x2c, 
            (byte) 0x15, (byte) 0x04, (byte) 0x32, (byte) 0xc0, 
            (byte) 0x00, (byte) 0x09, (byte) 0xb8, (byte) 0x00, 
            (byte) 0x0a, (byte) 0x53, (byte) 0x84, (byte) 0x04, 
            (byte) 0x01, (byte) 0xa7, (byte) 0xff, (byte) 0xd0, 
            (byte) 0x2a, (byte) 0xb8, (byte) 0x00, (byte) 0x0a, 
            (byte) 0x2b, (byte) 0x2d, (byte) 0xb6, (byte) 0x00, 
            (byte) 0x0b, (byte) 0xb0, (byte) 0x4e, (byte) 0xbb, 
            (byte) 0x00, (byte) 0x0d, (byte) 0x59, (byte) 0x2d, 
            (byte) 0xb6, (byte) 0x00, (byte) 0x0e, (byte) 0xb7, 
            (byte) 0x00, (byte) 0x0f, (byte) 0xbf, (byte) 0x00, 
            (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00, 
            (byte) 0x45, (byte) 0x00, (byte) 0x46, (byte) 0x00, 
            (byte) 0x0c, (byte) 0x00, (byte) 0x01, (byte) 0x00, 
            (byte) 0x15, (byte) 0x00, (byte) 0x00, (byte) 0x00, 
            (byte) 0x26, (byte) 0x00, (byte) 0x09, (byte) 0x00, 
            (byte) 0x00, (byte) 0x00, (byte) 0x13, (byte) 0x00, 
            (byte) 0x06, (byte) 0x00, (byte) 0x14, (byte) 0x00, 
            (byte) 0x10, (byte) 0x00, (byte) 0x15, (byte) 0x00, 
            (byte) 0x1a, (byte) 0x00, (byte) 0x16, (byte) 0x00, 
            (byte) 0x28, (byte) 0x00, (byte) 0x18, (byte) 0x00, 
            (byte) 0x36, (byte) 0x00, (byte) 0x14, (byte) 0x00, 
            (byte) 0x3c, (byte) 0x00, (byte) 0x1b, (byte) 0x00, 
            (byte) 0x46, (byte) 0x00, (byte) 0x1c, (byte) 0x00, 
            (byte) 0x47, (byte) 0x00, (byte) 0x1d, (byte) 0x00, 
            (byte) 0x01, (byte) 0x00, (byte) 0x1d, (byte) 0x00, 
            (byte) 0x00, (byte) 0x00, (byte) 0x02, (byte) 0x00, 
            (byte) 0x1e
    };
    IxcClassLoader(ClassLoader l) {
        super(l);
    }

    private void addMethodsFor(Class clazz, Hashtable methods) {
        if (theRemoteIF.isAssignableFrom(clazz)) {
            if (clazz.isInterface()) {
                Method[] m = clazz.getMethods();
                for (int i = 0; i < m.length; i++) {
                    methods.put(m[i], m[i]);
                }
            } else {
                Class[] ifs = clazz.getInterfaces();
                for (int i = 0; i < ifs.length; i++) {
                    addMethodsFor(ifs[i], methods);
                }
                Class sup = clazz.getSuperclass();
                if (sup != null) {
                    addMethodsFor(sup, methods);
                }
            }
        }
    }

    private Method[] getMethodsFor(Class remote) {
        Hashtable methods = new Hashtable(11);
        // Set of methods we have to generate.  Both key and value are
        // (the same instance of) Method.  This eliminates duplicates.
        addMethodsFor(remote, methods);
        Method[] result = new Method[methods.size()];
        int i = 0;
        for (Enumeration e = methods.elements(); e.hasMoreElements();) {
            result[i++] = (Method) e.nextElement();
        }
        return result;
    }

    private void addInterfacesFor(Class clazz, Hashtable interfaces,
        boolean addAll) {
        // If addAll = true, then add any interface to the list.
        // Else add interfaces that extend java.rmi.Remote to the list.

        if (clazz.isInterface()) {
            if (addAll) {
                interfaces.put(clazz, clazz);
            } else {
                if (theRemoteIF.isAssignableFrom(clazz)) {
                    interfaces.put(clazz, clazz);
                    addAll = true;
                }
            }
        }
        Class[] ifs = clazz.getInterfaces();
        for (int i = 0; i < ifs.length; i++) {
            addInterfacesFor(ifs[i], interfaces, addAll);
        }
        Class sup = clazz.getSuperclass();
        if (sup != null) {
            addInterfacesFor(sup, interfaces, addAll);
        }
    }
  
    private Class[] getInterfacesFor(Class remote) {
        // Returns a set of interfaces we have to add to a ConstantPool.

        Hashtable interfaces = new Hashtable(11);
        addInterfacesFor(remote, interfaces, false);
        Class[] result = new Class[interfaces.size()];
        int i = 0;
        for (Enumeration e = interfaces.elements(); e.hasMoreElements();) {
            result[i++] = (Class) e.nextElement();
        }
        return result;
    }

    //
    // Get the set of remote interfaces that are
    // implemented by the class cl.  This includes
    // remote interfaces implemented by
    // the superclasses of cl.
    //
    private Class[] getRemoteInterfaces(Class cl) {
        HashSet interfaces = new HashSet();
        getRemoteInterfacesFor(cl, interfaces);
        Class[] result = new Class[interfaces.size()];
        Iterator it = interfaces.iterator();
        for (int i = 0; it.hasNext(); i++){
            result[i] = (Class) it.next();
        }
        return result;
    }
                                                                          
    private void getRemoteInterfacesFor(Class cl, HashSet interfaces) {
        while (cl != null) {
            Class[] ifs = cl.getInterfaces();
            for (int i = 0; i < ifs.length; i++) {
                if (theRemoteIF.isAssignableFrom(ifs[i])) {
                    interfaces.add(ifs[i]);
                }
            }
            cl = cl.getSuperclass();
        }
    }

    private String nextStubName(String className) {
        // We expect to be called with this's lock held

        // fix for 4977190
        return new String(className + "_stub" + (nextStubNumber++)); 
    }

    private String descriptorFor(Method m) {
        String descriptor = "(";
        Class[] params = m.getParameterTypes();
        for (int j = 0; j < params.length; j++) {
            descriptor += TypeInfo.descriptorFor(params[j]);
        }
        descriptor += ")";
        descriptor += TypeInfo.descriptorFor(m.getReturnType());
        return descriptor;
    }

    synchronized Class getStubClass(IxcClassLoader target, Class remote) 
        throws RemoteException {
        loadUtilsClass();
        Class result = (Class) generated.get(remote);
        if (result != null) {
            return result;
        }
        if (!theRemoteIF.isAssignableFrom(remote)) {
            throw new StubException("Remote class must implement Remote");
        }
        try {
            String stubName = nextStubName(remote.getName());
            // fix for 4977190
            byte[] classBytes = generateStubClass(stubName.replace('.','/'), 
                                                  remote);

            // workaround to invoke defineClass() of a ClassLoader 
            //           that loaded an xlet..
              
            ClassLoader loader = getParent();
            final java.lang.reflect.Method m
                = ClassLoader.class.getDeclaredMethod("defineClass",
                   new Class[] { String.class, classBytes.getClass(), int.class, int.class });

            AccessController.doPrivileged(new PrivilegedAction() {
               public Object run() {
                   m.setAccessible(new java.lang.reflect.AccessibleObject[]{m}, true);
                   return null;
               }
            });

            result = (Class) m.invoke(loader, new Object[] { stubName, classBytes, new Integer(0), new Integer(classBytes.length) } );
   
            Method initialize = result.getMethod("com_sun_xlet_init",
                    new Class[] { Method.class }
                );
            Method findMethod = target.getFindMethodMethod();
            initialize.invoke(null, new Object[] { findMethod }
            );
            generated.put(remote, result);
            return result;
        } catch (StubException r) {
            throw r;
        } catch (Exception ex) {
            throw new StubException("getStub() failed", ex);
        } catch (NoClassDefFoundError err) {
            throw new StubException("Cannot find a class definition for " + err.getMessage(), err);
        }
    }

    synchronized void forgetStubsFor(ClassLoader other) {
        Class[] keys = new Class[generated.size()];
        int i = 0;
        for (Enumeration e = generated.keys(); e.hasMoreElements();) {
            keys[i++] = (Class) e.nextElement();
        }
        for (i = 0; i < keys.length; i++) {
            Class stub = (Class) generated.get(keys[i]);
            // Now, we invoke com_sun_xlet_destroy on the stub.  This
            // is important, because our stub maintains references to
            // methods of the ClassLoader that is being destroyed.  If
            // we maintained them, that ClassLoader would still be reachable
            // through our stub class, so the victim ClassLoader would never
            // become collectible.
            try {
                Method destroy = stub.getMethod("com_sun_xlet_destroy",
                        new Class[0]);
                destroy.invoke(null, new Object[0]);
            } catch (Exception ex) {
                ex.printStackTrace();	// Should never happen
            }
            generated.remove(keys[i]);
        }
    }

    private synchronized byte[] generateStubClass(String stubName, Class remote)
        throws IOException, RemoteException {
        Method[] methods = getMethodsFor(remote);
        // The stub would include methods defined in 'remote interface',
        // which is an interface that directly or indirectly extends
        // java.rmi.Remote.  In other words,
        //    interface BaseInterface extends java.rmi.Remote
        //    interface ExtendedInterface extends java.rmi.Remote, Xlet
        //    class TestXlet implements ExtendedInterface
        // then, ExtendedInterface is a 'remote interface' for TestXlet 
        // and methods declared in BaseInterface, ExtendedInterface 
        // and Xlet interface are treated as remote methods.
        //
        Class[] allInterfaces = getInterfacesFor(remote);
        Class[] toplevelInterfaces = getRemoteInterfaces(remote);
        // time to check if all method declarations are valid..
        verifyRemoteMethods(methods);
        ConstantPool cp = new ConstantPool();
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(bos);
        cp.addString("Code");		// For the code attribute
        cp.addString("Exceptions");	// For the exceptoins attribute
        String wrappedRemote = "com/sun/xlet/ixc/WrappedRemote";
        String constructorDescriptor =
            "(Ljava/rmi/Remote;Lcom/sun/xlet/ixc/ImportRegistry;Lcom/sun/xlet/ixc/RegistryKey;)V";
        String executeDescriptor = 
            "(Ljava/lang/reflect/Method;[Ljava/lang/Object;)Ljava/lang/Object;";
        String invokeDescriptor =
            "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;";
        // Add constant pool entries for names derived from the stuff
        // we're implementing.
        cp.addClass(wrappedRemote);
        cp.addClass(stubName);
        cp.addClass("com/sun/xlet/Utils");
        cp.addClass("java/lang/Object");
        cp.addClass("java/lang/Exception");
        cp.addClass("java/lang/reflect/Method");
        cp.addClass("java/rmi/RemoteException");
        for (int i = 0; i < allInterfaces.length; i++) {
            cp.addClass(allInterfaces[i].getName().replace('.', '/'));
        }
        Hashtable primArgsDone = new Hashtable(11);
        Hashtable primRetsDone = new Hashtable(11);
        for (int i = 0; i < methods.length; i++) {
            cp.addField(stubName,
                "com_sun_xlet_method" + i,
                "Ljava/lang/reflect/Method;");	// For class init method
            cp.addStringConstant(methods[i].getName());
            cp.addIfMethodReference(methods[i].getDeclaringClass().getName().replace('.', '/'),
                methods[i].getName(), 
                descriptorFor(methods[i]));
            cp.addStringConstant(methods[i].getDeclaringClass().getName());
            Class rt = methods[i].getReturnType();
            if (Void.TYPE.equals(rt)) {} else if (rt.isPrimitive()) {
                TypeInfo info = TypeInfo.get(rt);
                String rtNm = info.primitiveWrapper.getName().replace('.', '/');
                cp.addClass(rtNm);
                cp.addMethodReference(rtNm, info.valueMethod,
                    "()" + info.typeDescriptor);
            } else {
                cp.addClass(rt.getName().replace('.', '/'));
            }
            Class[] params = methods[i].getParameterTypes();
            for (int j = 0; j < params.length; j++) {
                if (params[j].isPrimitive()) {
                    // Don't need to worry about void here
                    TypeInfo info = TypeInfo.get(params[j]);
                    Class p = info.primitiveWrapper;
                    String nm = p.getName().replace('.', '/');
                    if (primArgsDone.get(nm) == null) {
                        primArgsDone.put(nm, nm);
                        cp.addClass(nm);
                        // The constructor for the wrapper class:
                        cp.addMethodReference(nm, "<init>", 
                            "(" + TypeInfo.descriptorFor(params[j]) + ")V");
                        // The TYPE field
                        cp.addField(nm, "TYPE", "Ljava/lang/Class;");
                    }
                } else {
                    cp.addStringConstant(params[j].getName());
                }
            }
            //for exceptions
            //Class[] exceptions = methods[i].getExceptionTypes();
            //for (int j = 0; j < exceptions.length; j++) {
            //  cp.addClass(exceptions[j].getName().replace('.', '/'));
            //}
            cp.addClass("java/rmi/RemoteException");
        }
        cp.addString("com_sun_xlet_init");
        cp.addString("(Ljava/lang/reflect/Method;)V");
        cp.addString("com_sun_xlet_destroy");
        cp.addString("()V");
        // Add constructor constants...
        cp.addString("<init>");
        cp.addMethodReference(wrappedRemote, "<init>", constructorDescriptor);
        cp.addMethodReference(wrappedRemote, "com_sun_xlet_execute",
            executeDescriptor);
        cp.addMethodReference("java/lang/reflect/Method", "invoke",
            invokeDescriptor);
        // Now write out the .class!
        dos.writeInt(0xcafebabe);
        dos.writeShort(0x3a);	// Minor version, JDK 1.1.3
        dos.writeShort(0x2d);	// Major version, JDK 1.1.3
        // It's what I observed in 1.1.3, which was handy.  Nothing magic
        // about 1.1.3.

        cp.write(dos);		// Constant pool
        dos.writeShort(0x31);	// ACC_SUPER | ACC_PUBLIC | ACC_FINAL
        // this_class:
        dos.writeShort(cp.lookupClass(stubName));
        // super_class:
        dos.writeShort(cp.lookupClass(wrappedRemote));
        // Interfaces:
        dos.writeShort(toplevelInterfaces.length);
        for (int i = 0; i < toplevelInterfaces.length; i++) {
            dos.writeShort(cp.lookupClass(toplevelInterfaces[i].getName().replace('.', '/')));
        }
        // Fields:
        dos.writeShort(methods.length);
        int methodClassType = cp.lookupString("Ljava/lang/reflect/Method;");
        for (int i = 0; i < methods.length; i++) {
            dos.writeShort(0x8 | 0x2);  // STATIC, PRIVATE
            dos.writeShort(cp.lookupString("com_sun_xlet_method" + i));
            dos.writeShort(methodClassType);
            dos.writeShort(0);		// attributes_count
        }
        // Methods:
        dos.writeShort(methods.length + 3); // First, the constructor:
        {
            dos.writeShort(0x1);		// PUBLIC
            dos.writeShort(cp.lookupString("<init>"));
            dos.writeShort(cp.lookupString(constructorDescriptor));
            dos.writeShort(1);	// 1 attribute, the Code attribute
            dos.writeShort(cp.lookupString("Code"));
            int codeLen = 8;
            dos.writeInt(12 + codeLen);	// attribute_length
            dos.writeShort(10);		// max_stack; be conservative
            dos.writeShort(10);		// max_locals; be conservative
            dos.writeInt(codeLen);
            // The code:
            dos.write(0x2a);	// aload_0
            dos.write(0x2b);	// aload_1
            dos.write(0x2c);	// aload_2
            dos.write(0x2d);	// aload_3
            dos.write(0xb7);	// invokespecial
            dos.writeShort(
                cp.lookupMethod(wrappedRemote, "<init>", constructorDescriptor));
            dos.write(0xb1);	// return
            // The rest of the code attribute:
            dos.writeShort(0);		// exception_table_length
            dos.writeShort(0);		// attribute_count
        }
        // Next, the com_sun_xlet_init static initialization method:
        int invokeMethod = cp.lookupMethod("java/lang/reflect/Method",
                "invoke", invokeDescriptor); {
            dos.writeShort(0x1 | 0x8);		// PUBLIC | STATIC
            dos.writeShort(cp.lookupString("com_sun_xlet_init"));
            dos.writeShort(cp.lookupString("(Ljava/lang/reflect/Method;)V"));
            dos.writeShort(2);	// 2 attributes, Code and Exceptions
            dos.writeShort(cp.lookupString("Code"));
            int codeLen = 7;
            for (int i = 0; i < methods.length; i++) {
                Class[] params = methods[i].getParameterTypes();
                codeLen += 25 + (7 * params.length) + 10;
            }
            codeLen += 1;
            dos.writeInt(12 + codeLen);	// attribute_length
            dos.writeShort(20);		// max_stack; be conservative
            dos.writeShort(20);		// max_locals; be conservative
            dos.writeInt(codeLen);
            // The code:
	    
            dos.write(0xb2);	// getstatic
            dos.writeShort(cp.lookupField(stubName, "com_sun_xlet_method0",
                    "Ljava/lang/reflect/Method;"));
            dos.write(0xc6);	// ifnull
            dos.writeShort(4);
            dos.write(0xb1);	// return
            for (int i = 0; i < methods.length; i++) {
                Method m = methods[i];
                Class[] params = m.getParameterTypes();
                String from = m.getDeclaringClass().getName();
                dos.write(0x2a);	// aload_0
                dos.write(0x1);		// aconst_null
                dos.write(0x6);		// iconst_3
                dos.write(0xbd);	// anewarray
                dos.writeShort(cp.lookupClass("java/lang/Object"));
                dos.write(0x59);	// dup
                dos.write(0x3);		// iconst_0
                dos.write(0x13);	// ldc_2
                dos.writeShort(cp.lookupStringConstant(from));
                dos.write(0x53);	// aastore
                dos.write(0x59);	// dup
                dos.write(0x4);		// iconst_1
                dos.write(0x13);	// ldc_w
                dos.writeShort(cp.lookupStringConstant(m.getName()));
                dos.write(0x53);	// aastore
                dos.write(0x59);	// dup
                dos.write(0x5);		// iconst_2
                dos.write(0x10);	// bipush
                dos.write(params.length);
                dos.write(0xbd);	// anewarray
                dos.writeShort(cp.lookupClass("java/lang/Object"));
                for (int j = 0; j < params.length; j++) {
                    Class p = params[j];
                    dos.write(0x59);	// dup
                    dos.write(0x10);	// bipush
                    dos.write(j);
                    if (p.isPrimitive()) {
                        p = TypeInfo.get(p).primitiveWrapper;
                        String nm = p.getName().replace('.', '/');
                        dos.write(0xb2);	// getstatic
                        dos.writeShort(cp.lookupField(nm, "TYPE", 
                                "Ljava/lang/Class;"));
                    } else {
                        dos.write(0x13);	// ldc_w
                        dos.writeShort(cp.lookupStringConstant(p.getName()));
                    }
                    dos.write(0x53);	// aastore
                }
                dos.write(0x53);	// aastore
                dos.write(0xb6);		// invokevirtual
                dos.writeShort(invokeMethod);
                dos.write(0xc0);	// checkcast
                dos.writeShort(cp.lookupClass("java/lang/reflect/Method"));
                dos.write(0xb3);	// putstatic
                dos.writeShort(cp.lookupField(stubName, "com_sun_xlet_method" + i, "Ljava/lang/reflect/Method;"));
            }
            dos.write(0xb1);	// return
            // The rest of the code attribute:
            dos.writeShort(0);		// exception_table_length
            dos.writeShort(0);		// attribute_count
            // Now the second attribute of the method
            dos.writeShort(cp.lookupString("Exceptions"));
            dos.writeInt(4);
            dos.writeShort(1);
            dos.writeShort(cp.lookupClass("java/lang/Exception"));
        } // Next, the com_sun_xlet_destroy static method.  cf. 
        // forgetStubsFor(XletClassLoader l).
        {
            dos.writeShort(0x1 | 0x8);		// PUBLIC | STATIC
            dos.writeShort(cp.lookupString("com_sun_xlet_destroy"));
            dos.writeShort(cp.lookupString("()V"));
            dos.writeShort(1);	// 1 attribute, Code
            dos.writeShort(cp.lookupString("Code"));
            int codeLen = (4 * methods.length) + 1;
            dos.writeInt(12 + codeLen);	// attribute_length
            dos.writeShort(5);		// max_stack; be conservative
            dos.writeShort(5);		// max_locals; be conservative
            dos.writeInt(codeLen);
            // The code:
	    
            for (int i = 0; i < methods.length; i++) {
                Method m = methods[i];
                dos.write(0x1);		// aconst_null
                dos.write(0xb3);	// putstatic
                dos.writeShort(cp.lookupField(stubName, "com_sun_xlet_method" + i, "Ljava/lang/reflect/Method;"));
            }
            dos.write(0xb1);	// return
            // The rest of the code attribute:
            dos.writeShort(0);		// exception_table_length
            dos.writeShort(0);		// attribute_count
        }
        int executeMethod = cp.lookupMethod(wrappedRemote, 
                "com_sun_xlet_execute",
                executeDescriptor);
        // Now the stub methods:
        for (int i = 0; i < methods.length; i++) {
            Method m = methods[i];
            Class[] args = m.getParameterTypes();
            int maxLocals = 1;	// 1 for "this" parameter
            for (int j = 0; j < args.length; j++) {
                maxLocals += TypeInfo.localSlotsFor(args[j]);
            }
            int codeLen = 9;
            for (int j = 0; j < args.length; j++) {
                if (args[j].isPrimitive()) {
                    codeLen += 9 + 4;
                } else {
                    codeLen += 2 + 4;
                }
            }
            codeLen += 3;
            if (Void.TYPE.equals(m.getReturnType())) {
                codeLen += 2;
            } else if (m.getReturnType().isPrimitive()) {
                codeLen += 7;
            } else {
                codeLen += 4;
            }
            dos.writeShort(0x1 | 0x10);		// PUBLIC | FINAL
            dos.writeShort(cp.lookupString(m.getName()));
            dos.writeShort(cp.lookupString(descriptorFor(methods[i])));
            dos.writeShort(2);	// attributes_count
            dos.writeShort(cp.lookupString("Code"));
            dos.writeInt(12 + codeLen);
            dos.writeShort(10);			// max_stack; be conservative
            dos.writeShort(maxLocals);
            dos.writeInt(codeLen);
            // Now the code
            dos.write(0x2a);	// aload_0
            dos.write(0xb2);	// getstatic
            dos.writeShort(cp.lookupField(stubName, "com_sun_xlet_method" + i,
                    "Ljava/lang/reflect/Method;"));
            dos.write(0x10);		// bipush
            dos.write(args.length);
            dos.write(0xbd);	// anewarray  java.lang.Object
            dos.writeShort(cp.lookupClass("java/lang/Object"));
            int slot = 1;
            for (int j = 0; j < args.length; j++) {
                dos.write(0x59);	// dup
                dos.write(0x10);	// bipush
                dos.write(j);
                if (args[j].isPrimitive()) {
                    TypeInfo info = TypeInfo.get(args[j]);
                    Class p = info.primitiveWrapper;
                    String pName = p.getName().replace('.', '/');
                    dos.write(0xbb);	// new
                    dos.writeShort(cp.lookupClass(pName));
                    dos.write(0x59);	// dup
                    dos.write(info.loadInstruction);
                    dos.write(slot);
                    // Invoke constructor of primitive wrapper type:
                    dos.write(0xb7);	// invokespecial
                    String d = "(" + info.typeDescriptor + ")V";
                    dos.writeShort(cp.lookupMethod(pName, "<init>", d));
                    slot += info.localSlots;
                } else {
                    dos.write(0x19);
                    dos.write(slot);
                    slot++;
                }
                dos.write(0x53);	// aastore
            }
            dos.write(0xb6);		// invokevirtual
            dos.writeShort(executeMethod);
            Class ret = m.getReturnType();
            if (Void.TYPE.equals(ret)) {
                dos.write(0x57);	// pop
                dos.write(0xb1);	// return
            } else if (ret.isPrimitive()) {
                TypeInfo info = TypeInfo.get(ret);
                Class wr = info.primitiveWrapper;
                String wrNm = wr.getName().replace('.', '/');
                dos.write(0xc0);	// checkcast
                dos.writeShort(cp.lookupClass(wrNm));
                dos.write(0xb6);		// invokevirtual
                dos.writeShort(cp.lookupMethod(wrNm, info.valueMethod,
                        "()" + info.typeDescriptor));
                dos.write(info.returnInstruction);
            } else {
                dos.write(0xc0);	// checkcast
                dos.writeShort(cp.lookupClass(ret.getName().replace('.', '/')));
                dos.write(0xb0);	// areturn
            }
            // The rest of the code attribute:
            dos.writeShort(0);		// exception_table_length
            dos.writeShort(0);		// attribute_count
            // Now the second attribute of the method
            dos.writeShort(cp.lookupString("Exceptions"));
            dos.writeInt(4);
            dos.writeShort(1);
            dos.writeShort(cp.lookupClass("java/rmi/RemoteException"));
            // Class[] exceptions = m.getExceptionTypes();
            // number of description (2) + exception index table
            //dos.writeInt(2 + exceptions.length*2);
            //dos.writeShort(exceptions.length);
            // assume RemoteException is already declared in the method 
            //dos.writeShort(cp.lookupClass("java/rmi/RemoteException"));
            //for (int j = 0; j < exceptions.length; j++) {
            //   dos.writeShort(cp.lookupClass(exceptions[j].getName().replace('.', '/')));
            //}
        }
        // Attributes (of ClassFile):
        dos.writeShort(0);
        // And we're done!
        dos.close();
        // debug
        //System.out.println("@@ dumping class " + stubName
        //    + " to .class file for debug");

/**
** uncomment this part to dump the stub object in the current dir
**        try {
**            // dumping the stub class to the current dir. 
**            dos = new DataOutputStream(new java.io.FileOutputStream(stubName + ".class"));
**            dos.write(bos.toByteArray());
**            dos.close();
**        } catch (Exception e) {
**            // The output is for debugging - if it can't be done,
**            // just notify the user and move on.
**            System.out.println("@@ cannot write " + stubName
**                    + " to .class file to this file system: " + e);
**        }
**/
        return bos.toByteArray();
    }

    private synchronized void loadUtilsClass() throws RemoteException {
        if (utilsClass == null) {
            try {
                // fix for 4977190
                utilsClass = defineClass("com.sun.xlet.Utils",
                            utilsClassBody, 0,
                            utilsClassBody.length);
            } catch (Exception ex) {
                //throw new RemoteException("Failed to load a util class", ex);
                throw new StubException("Failed to load a util class", ex);
            }
        }
    }

    synchronized Method getDeserializeMethod() throws RemoteException {
        if (deserializeMethod == null) {
            loadUtilsClass();
            try {
                Class byteA = (Array.newInstance(Byte.TYPE, 0)).getClass();
                deserializeMethod 
                        = utilsClass.getMethod("deserialize", 
                            new Class[] { byteA }
                        );
            } catch (Exception ex) {
                //throw new RemoteException("Can't deserialize", ex);
                throw new StubException("Can't deserialize", ex);
            }
        }
        return deserializeMethod;
    }

    synchronized Method getFindMethodMethod() throws RemoteException {
        if (findMethodMethod == null) {
            loadUtilsClass();
            try {
                Class sc = String.class;
                Class oa = (Array.newInstance(Object.class, 0)).getClass();
                findMethodMethod
                        = utilsClass.getMethod("findMethod", 
                            new Class[] { sc, sc, oa }
                        );
            } catch (Exception ex) {
                //throw new RemoteException("Can't find the method", ex);
                throw new StubException("Can't find the method", ex);
            }
        }
        return findMethodMethod;
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

    private void log(String s) {
        System.err.println("IxcClassLoader: " + s);
    }
}
