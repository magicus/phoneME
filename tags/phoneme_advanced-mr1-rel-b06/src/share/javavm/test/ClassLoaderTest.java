/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
import sun.misc.*;
import java.lang.reflect.*;

public class ClassLoaderTest {
    public static void main(String argv[]) {
	int i;
	for (i = 0; i < Integer.parseInt(argv[0]); i++) {
	    try {
		/* Get an application ClassLoader. It will load classes
		 * off of the classpath.
		 */ 
		sun.misc.Launcher launcher = new Launcher();
		ClassLoader cl = launcher.getClassLoader();
		
		/* Load the class whose name was passed in. */
		Class clazz = cl.loadClass(argv[1]);
		
		/* Call the main() method of the class. */
		Class[] argTypes = new Class[1];
		argTypes[0] = Class.forName("[Ljava.lang.String;");
		Method main = clazz.getMethod("main", argTypes);
		Object[] args = new Object[1];
		String[] arg0 = new String[argv.length - 2];
		args[0] = arg0;
		for (int j = 0; j < argv.length - 2; j++) {
		    arg0[j] = argv[j+2];
		}
		main.invoke(null, args);
		
		/* Null out any references we have to the Test class and
		 * the application ClassLoader.
		 */
		launcher = null;
		cl = null;
		clazz = null;
		main = null;
		
		/* The current thread can end up with a reference to the
		 * application ClassLoader. Null it out too.
		 */
		Thread.currentThread().setContextClassLoader(null);
		
		/* Run the gc. If you set a breakpoint in CVMclassFree(), it
		 * should get hit for Test and any other application class
		 * loaded by Test.
		 */
		java.lang.Runtime.getRuntime().gc();
	    } catch (Throwable e) {
		e.printStackTrace();
	    }
	}
    }
}
