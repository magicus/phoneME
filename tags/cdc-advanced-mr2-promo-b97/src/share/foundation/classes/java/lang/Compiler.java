/*
 * @(#)Compiler.java	1.24 06/10/13
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

package java.lang;

/**
 * The <code>Compiler</code> class is provided to support
 * Java-to-native-code compilers and related services. By design, the
 * <code>Compiler</code> class does nothing; it serves as a
 * placeholder for a JIT compiler implementation.
 * <p>
 * When the Java Virtual Machine first starts, it determines if the
 * system property <code>java.compiler</code> exists. (System
 * properties are accessible through <code>getProperty</code> and, 
 * a method defined by the <code>System</code> class.) If so, and the
 * name isn't <b>NONE</b> or <b>none</b>, the internal JIT is enabled.
 * <p>
 * If no compiler is available, these methods do nothing.
 *
 * @version 1.17 10/17/00
 * @see     java.lang.System#getProperty(java.lang.String)
 * @see     java.lang.System#getProperty(java.lang.String, java.lang.String)
 * @since   JDK1.0
 */
public final class Compiler  {
    private Compiler() {}		// don't make instances

    private static native void registerNatives();

    static {
        registerNatives();
	java.security.AccessController.doPrivileged
	    (new java.security.PrivilegedAction() {
		public Object run() {
		    boolean loaded = false;
		    String jit = System.getProperty("java.compiler");
		    if (jit == null ||
			jit.equalsIgnoreCase("none") ||
			jit.equals("")) {
			// -- NO JIT --
			loaded = false;
		    }
		    else {
			if (!jit.equals("sunwjit")) {
			  System.err.println("Warning: invalid JIT compiler. " +
					     "Choices are sunwjit or NONE.\n" +
					     "Will use sunwjit.");
			}
			loaded = true;
		    }
		    String info = System.getProperty("java.vm.info");
		    if (loaded) {
			System.setProperty("java.vm.info", info + ", " + jit);
		    } else {
			System.setProperty("java.vm.info", info + ", nojit");
		    }
		    return null;
		}
	    });
    }

    /**
     * Compiles the specified class.
     *
     * @param   clazz   a class.
     * @return  <code>true</code> if the compilation succeeded;
     *          <code>false</code> if the compilation failed or no compiler
     *          is available.
     * @exception NullPointerException if <code>clazz</code> is 
     *          <code>null</code>.
     */
    public static native boolean compileClass(Class clazz);

    /**
     * Compiles all classes whose name matches the specified string.
     *
     * @param   string   the name of the classes to compile.
     * @return  <code>true</code> if the compilation succeeded;
     *          <code>false</code> if the compilation failed or no compiler
     *          is available.
     * @exception NullPointerException if <code>string</code> is 
     *          <code>null</code>.
     */
    public static native boolean compileClasses(String string);

    /**
     * Examines the argument type and its fields and perform some documented
     * operation. No specific operations are required.
     *
     * @param   any   an argument.
     * @return  a compiler-specific value, or <code>null</code> if no compiler
     *          is available.
     * @exception NullPointerException if <code>any</code> is 
     *          <code>null</code>.
     */
    public static native Object command(Object any);

    /**
     * Cause the Compiler to resume operation. (This a noop on Solaris).
     */
    public static native void enable();

    /**
     * Cause the Compiler to cease operation.  (This a noop on Solaris).
     */
    public static native void disable();
}
