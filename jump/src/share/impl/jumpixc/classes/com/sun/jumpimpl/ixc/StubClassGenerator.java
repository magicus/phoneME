/*
 * @(#)StubClassGenerator.java	1.5 06/08/10
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

import javax.microedition.xlet.ixc.StubException;

import java.lang.reflect.Method;
import java.lang.reflect.AccessibleObject;
import java.security.AccessController;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.rmi.RemoteException;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;

/** 
 * Instances of this class generate stub classes.  They do this
 * by creating a byte[], and then passing it to the xlet's
 * ClassLoader.  The method on ClassLoader is protected, so
 * we use reflection wrapped in a doPrivileged block to call it.
 * <p>
 * To give an example, suppose that a user class implemens a remote 
 * interface UserIF,
 * and that interface contains two methods, void frob(Something), and
 * int glorp(float).  This class will automatically generate 
 * a remote stub.  The stub will be equivalent to the following class:
 * <pre>
 *
 *  package com.sun.jumpimpl.ixc;
 *
 *  import java.rmi.RemoteException;
 *
 *  public final class StubClass_stub42 
 *              extends com.sun.jumpimpl.ixc.StubObject
 *		implements UserIF {
 *
 *      public StubClass_stub42(Object registry,
 *			        Object target) {
 *          // Arguments are of type ImportedObjectRegistry and RemoteHandle
 *	    super(registry, target);
 *	}
 *
 *      public void frob(Something arg1) throws RemoteException {
 *          com_sun_xlet_execute("frob", new Object[] { arg1 });
 *      }
 *
 *      public int glorp(float arg1) throws RemoteException {
 *	    Object r = com_sun_xlet_execute("glorb",new Object[] { new Float(arg1) });
 *          return ((Integer) r).intValue();
 *      }
 *  }
 *
 * </pre>
 **/

 // @@ Add the synthetic attribute
 // @@ Do this with a security manager installed
 // @@ Make exception handling consistent with 1.2 behavior.  Specifically,
 //    RemoteExceptions should be cloned and wrapped, RuntimeExceptions
 //    should be cloned and re-thrown, checked exception that appear in the
 //    interface method's signature should be cloned and re-thrown (probably
 //    with our deprecated friend, Thread.stop(Throwable)), and unexpected
 //    checked exceptions should be cloned and wrapped (there's some exception
 //    under java.rmi specifically for this).
public class StubClassGenerator {

    private ClassLoader xletClassLoader;

    private static int nextStubNumber = 1;
	// This could be non-static, because xlets aren't permitted
	// to share classloaders.  However, if for some reason two
	// xlets ever to share a classloader (due to a bug or a
	// spec change), then this being static will prevent
	// a collision.

    public StubClassGenerator(ClassLoader xletClassLoader) {
	this.xletClassLoader = xletClassLoader;
    }

    //Class generate(RemoteRef remoteRef) throws StubException {
    Class generate(RemoteObjectType remoteRef) throws StubException {
	final String stubName = "StubClass_stub"
				+ (nextStubNumber++); 
	byte[] tmp = null;
	try {
	    tmp = generateClassBytes(stubName, remoteRef);
	} catch (IOException ex) {
	    throw new StubException("error generating stub", ex);
	}
	final byte[] classBytes = tmp;

	java.lang.reflect.Method tmp2 = null;
	try {
	    tmp2 = ClassLoader.class.getDeclaredMethod("defineClass",
		       new Class[] { String.class, classBytes.getClass(), 
				     int.class, int.class });
	} catch (NoSuchMethodException ex) {
	    throw new StubException("internal error", ex);
	}
	final java.lang.reflect.Method m = tmp2;

	// We need a privileged block, so that we can call the
	// protected defineClass method.  Since we're doing that
	// anyway, we define the stub class in our package namespace,
	// to guarantee there won't be any name collisions.
	Object result  = AccessController.doPrivileged(new PrivilegedAction() {
	    public Object run() {
		m.setAccessible(new AccessibleObject[]{m}, true);
                // Tempolary disable the security manager here, as it can 
                // error in loadClass for RuntimePermission 
                // accessClassInPackage.sun.mtask.xlet.ixc
                // (Yes, even in the privileged block)
                //SecurityManager sm = System.getSecurityManager();
                //if (sm != null) 
                //   System.setSecurityManager(null);

                Object result = null;
		try {
		   Object[] args = new Object[] {
		      stubName.replace('/', '.'),
		      classBytes,
		      new Integer(0),
		      new Integer(classBytes.length)
		   };

		   result = m.invoke(xletClassLoader, args);
		} catch (Throwable t) {
		   result = t;
		}

                //if (sm != null) // Set the SecurityManager back
                //   System.setSecurityManager(sm);

                return result;
	   }
	});
	if (result instanceof Class) {
	    return (Class) result;
	} else if (result instanceof StubException) {
	    throw (StubException) result;
	} else if (result instanceof NoClassDefFoundError) {
	    Throwable err = (NoClassDefFoundError) result;
            throw new StubException("Cannot find a class definition",  err);
	} else if (result instanceof Exception) {
            throw new StubException("getStub() failed", (Exception) result);
	} else {
            throw new StubException("getStub() failed:  " + result);
	}
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

    //
    // The stub includes methods defined in 'remote interface',
    // which is an interface that directly or indirectly extends
    // java.rmi.Remote.  In other words,
    //    interface BaseInterface extends java.rmi.Remote
    //    interface ExtendedInterface extends java.rmi.Remote, Xlet
    //    class TestXlet implements ExtendedInterface
    // then, ExtendedInterface is a 'remote interface' for TestXlet 
    // and methods declared in BaseInterface, ExtendedInterface 
    // and Xlet interface are treated as remote methods.
    //
    private byte[] generateClassBytes(String stubName, RemoteObjectType type) 
	    throws IOException, StubException
    {
        ConstantPool cp = new ConstantPool();
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(bos);
        cp.addString("Code");		// For the code attribute
        cp.addString("Exceptions");	// For the exceptoins attribute
        String importedObject = "com/sun/jumpimpl/ixc/StubObject";
        //String importedObject = "sun/mtask/xlet/ixc/StubObject";
        String constructorDescriptor =
		"(Ljava/lang/Object;Ljava/lang/Object;)V";
        String executeDescriptor = "(J[Ljava/lang/Object;)Ljava/lang/Object;";
        // Add constant pool entries for names derived from the stuff
        // we're implementing.
        cp.addClass(importedObject);
        cp.addClass(stubName);
        // @@ Need this:  cp.addClass("com/sun/xlet/mvmixc/Utils");
        cp.addClass("java/lang/Object");
        cp.addClass("java/lang/Exception");
        cp.addClass("java/rmi/RemoteException");
        cp.addClass("java/lang/RuntimeException");
        cp.addClass("java/rmi/UnexpectedException");

        String[] remoteInterfaces = type.getRemoteInterfaceNames();
        Long[] methodIDs = (Long[])type.methodsByID.keySet().toArray(new Long[]{});
        Method[] remoteMethods = new Method[methodIDs.length];
        for (int i = 0; i < remoteInterfaces.length; i++) {
            cp.addClass(remoteInterfaces[i].replace('.', '/'));
        }

        HashMap primArgsDone = new HashMap();
        HashMap primRetsDone = new HashMap();

        for (int i = 0; i < methodIDs.length; i++) {
	    remoteMethods[i] = (Method)type.methodsByID.get(methodIDs[i]);

            Method m = remoteMethods[i];
            cp.addStringConstant(m.getName());
            cp.addLong(methodIDs[i]);
            cp.addIfMethodReference(m.getDeclaringClass().getName().replace('.', '/'),
				    m.getName(), 
				    descriptorFor(m));
            cp.addStringConstant(m.getDeclaringClass().getName());
            Class rt = m.getReturnType();
            if (Void.TYPE.equals(rt)) {
		// do nothing
	    } else if (rt.isPrimitive()) {
                TypeInfo info = TypeInfo.get(rt);
                String rtNm = info.primitiveWrapper.getName().replace('.', '/');
                cp.addClass(rtNm);
                cp.addMethodReference(rtNm, info.valueMethod,
                    "()" + info.typeDescriptor);
            } else {
                cp.addClass(rt.getName().replace('.', '/'));
            }
            Class[] params = m.getParameterTypes();
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
            // Adding exception types
            Class[] exceptions = m.getExceptionTypes();
            for (int j = 0; j < exceptions.length; j++) {
               cp.addClass(exceptions[j].getName().replace('.', '/'));
            }
        }
        cp.addStringConstant("");
        // Add constructor constants...
        cp.addString("<init>");
        cp.addMethodReference(importedObject, "<init>", constructorDescriptor);
        cp.addMethodReference(importedObject, "com_sun_xlet_execute",
			      executeDescriptor);
        // For creating UnexpectedException
        String unexpectedExceptionDescriptor = 
                "(Ljava/lang/String;Ljava/lang/Exception;)V";
        cp.addMethodReference("java/rmi/UnexpectedException", "<init>", unexpectedExceptionDescriptor);

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
        dos.writeShort(cp.lookupClass(importedObject));
        // Interfaces:
        dos.writeShort(remoteInterfaces.length);
        for (int i = 0; i < remoteInterfaces.length; i++) {
	    String nm = remoteInterfaces[i];
            dos.writeShort(cp.lookupClass(nm.replace('.', '/')));
        }
        // Fields:
        dos.writeShort(0);
        // Methods:
        dos.writeShort(remoteMethods.length + 1); // +1 for constructor
	
	// First, the constructor:
        {
            dos.writeShort(0x1);		// PUBLIC
            dos.writeShort(cp.lookupString("<init>"));
            dos.writeShort(cp.lookupString(constructorDescriptor));
            dos.writeShort(1);	// 1 attribute, the Code attribute
            dos.writeShort(cp.lookupString("Code"));
            int codeLen = 7;
            dos.writeInt(12 + codeLen);	// attribute_length
            dos.writeShort(10);		// max_stack; be conservative
            dos.writeShort(10);		// max_locals; be conservative
            dos.writeInt(codeLen);
            // The code:
            dos.write(0x2a);	// aload_0, loading this
            dos.write(0x2b);	// aload_1, loading StubObject
            dos.write(0x2c);	// aload_2, loading RemoteHandle
            dos.write(0xb7);	// invokespecial, e.g. super(xxx) of ...
            dos.writeShort( cp.lookupMethod(importedObject, 
	    				    "<init>", constructorDescriptor));
            dos.write(0xb1);	// return
            // The rest of the code attribute:
            dos.writeShort(0);		// exception_table_length
            dos.writeShort(0);		// attribute_count
        }

        int executeMethod = cp.lookupMethod(importedObject, 
					    "com_sun_xlet_execute",
					    executeDescriptor);
        // Now the stub methods:
        for (int i = 0; i < remoteMethods.length; i++) {
            Method m = remoteMethods[i];
            Class[] args = m.getParameterTypes();
            //int maxLocals = 1;	// 1 for "this" parameter
            int maxLocals = 2;	// 1 for "this" parameter
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
            Class ret = m.getReturnType();
            if (Void.TYPE.equals(ret)) {
                //codeLen += 2;
                codeLen += 5;
            } else if (ret.isPrimitive()) {
                codeLen += 7;
            } else {
                codeLen += 4;
            }

            int pc_end = codeLen;
            if (Void.TYPE.equals(ret)) {
               pc_end -= 4;  
            } else {
               pc_end -= 1;  
            }
  
            // For exception handling.
            // Each exception catch/throw takes 3 bytes.
            // For RuntimeException and Exception.
            codeLen += 6;
            // For all other catched Exceptions.
            Class[] exceptions = m.getExceptionTypes();
            codeLen += (exceptions.length*3);
            codeLen += 9; // For creating UnexpectedException

            dos.writeShort(0x1 | 0x10);		// PUBLIC | FINAL
            dos.writeShort(cp.lookupString(m.getName()));
            dos.writeShort(cp.lookupString(descriptorFor(remoteMethods[i])));
            dos.writeShort(2);	// attributes_count
            dos.writeShort(cp.lookupString("Code"));

            // 8 for max_stock + max_locals + code_length fields;
            // 4 for attribute_count and exception_length fields;
            // Each exception table takes 8 bytes,
            // There are two additional exceptions: Runtime and Exception.
            int codeAttributeLen = (8  + codeLen + 4 + (exceptions.length*8) + 16); 
            dos.writeInt(codeAttributeLen);
            dos.writeShort(10);			// max_stack; be conservative
            dos.writeShort(maxLocals);
            dos.writeInt(codeLen);
            // Now the code
            dos.write(0x2a);	// aload_0
            dos.write(0x14);	// ldc2_w
            dos.writeShort(cp.lookupLongConstant(methodIDs[i]));
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

            if (Void.TYPE.equals(ret)) {
                dos.write(0x57);	// pop
                //dos.write(0xb1);	// return
                dos.write(0xa7);	// goto
                dos.writeShort(codeLen - pc_end - 1); // target of goto
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

            // csaito: for catched exceptions
            for (int j = 0; j < exceptions.length; j++) {
               dos.write(0x4c); // astore_1
               dos.write(0x2b); // aload_1
               dos.write(0xbf); // athrow
            }

            {// For RuntimeException
               dos.write(0x4c); // astore_1
               dos.write(0x2b); // aload_1
               dos.write(0xbf); // athrow
            }

            {// For General Exception
               dos.write(0x4c); // astore_1
               dos.write(0xbb);	// new
               dos.writeShort(cp.lookupClass("java/rmi/UnexpectedException"));
               dos.write(0x59);	// dup 
               dos.write(0x12);	// lcd
               dos.write(cp.lookupStringConstant(""));	// lcd
               dos.write(0x2b); // aload_1
               dos.write(0xb7);	// invokespecial
               dos.writeShort(cp.lookupMethod("java/rmi/UnexpectedException", "<init>", unexpectedExceptionDescriptor));
               dos.write(0xbf); // athrow
            }

            // finally, a return.
            if (Void.TYPE.equals(ret)) {
                dos.write(0xb1);	// return
            } 

            // The rest of the code attribute:
            // First, the exception table.
            dos.writeShort(exceptions.length + 2);// exception_table_length

            int astore_start;
            if (Void.TYPE.equals(ret)) {
               astore_start = pc_end + 3; // after goto
            } else {
               astore_start = pc_end + 1; // after return
            }

            for (int j = exceptions.length-1; j >= 0; j--) {
               dos.writeShort(0); // pc_start
               dos.writeShort(pc_end); // pc_end
               dos.writeShort(astore_start);
               dos.writeShort(cp.lookupClass(exceptions[j].getName().replace('.', '/')));
               astore_start += 3;
            }

            { // for RuntimeException
               dos.writeShort(0); // pc_start
               dos.writeShort(pc_end); // pc_end
               dos.writeShort(astore_start);
               dos.writeShort(cp.lookupClass("java/lang/RuntimeException"));
               astore_start += 3;
            }

            { // for general Exception
               dos.writeShort(0); // pc_start
               dos.writeShort(pc_end); // pc_end
               dos.writeShort(astore_start);
               dos.writeShort(cp.lookupClass("java/lang/Exception"));
            }

            dos.writeShort(0);		// attribute_count
            // Now the second attribute of the method
            dos.writeShort(cp.lookupString("Exceptions"));
            dos.writeInt(4);
            dos.writeShort(1);
            dos.writeShort(cp.lookupClass("java/rmi/RemoteException"));
        }
        // Attributes (of ClassFile):
        dos.writeShort(0);
        // And we're done!
        dos.close();
        // @@: uncomment below to write out the generated stub class 
/**
**        System.out.println("@@ dumping class " + stubName
**             + " to /tmp/foo.class file for debug");
**          try {
**              dos = new DataOutputStream(new java.io.FileOutputStream(
**					   "/tmp/foo.class"));
**              dos.write(bos.toByteArray());
**              dos.close();
**          } catch (java.io.FileNotFoundException e) {
**	     System.out.println("@@ cannot write " + stubName
**                   + " to .class file to this file system");
**          } catch (SecurityException se) {
**	     System.out.println("@@ cannot write " + stubName
**                   + " to .class file to this file system");
**          }
**/
        return bos.toByteArray();
    }
}
