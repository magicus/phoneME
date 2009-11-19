/*
 * @(#)ClassLoaderTest.java	1.10 06/10/10
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
import sun.misc.*;
import java.lang.reflect.*;
import java.net.*;
import java.io.*;
import sun.misc.GC;

public class ClassLoaderTest {
    /*
     * The stream handler factory for loading system protocol handlers.
     * NOTE: This is a clone of the Factor class in Launch.java.
     */
    private static class Factory implements URLStreamHandlerFactory {
	private static String PREFIX = "sun.net.www.protocol";

	public URLStreamHandler createURLStreamHandler(String protocol) {
	    String name = PREFIX + "." + protocol + ".Handler";
	    try {
		Class c = Class.forName(name);
		return (URLStreamHandler)c.newInstance();
	    } catch (ClassNotFoundException e) {
		e.printStackTrace();
	    } catch (InstantiationException e) {
		e.printStackTrace();
	    } catch (IllegalAccessException e) {
		e.printStackTrace();
	    }
	    throw new InternalError("could not load " + protocol +
				    "system protocol handler");
	}
    }

    public static void main(String argv[]) {
	/* Make sure that calling ClassLoader.getResourceAsStream doesn't
	 * result in opening jarfiles that never close.
	 */
	sun.misc.CVM.setURLConnectionDefaultUseCaches(false);

	/*
	 * Setup the urls[] array for the URLClassLoader. This is basically
	 * copying what is done in Launcher.java, although we take
	 * some liberties like assuming that java.class.path only
	 * contains one item.
	 */
	final String s = System.getProperty("java.class.path");
	System.out.println("java.class.path: " + s);
	URL[] urls = new URL[1];
	URLStreamHandlerFactory factory = new Factory();
	URLStreamHandler fileHandler = factory.createURLStreamHandler("file");
	try {
	    urls[0] = new URL("file", "", -1, s, fileHandler);
	} catch (MalformedURLException e) {
	    // Should never happen since we specify the protocol...
	    throw new InternalError();
	}

	int i;
	for (i = 0; i < Integer.parseInt(argv[0]); i++) {
	    try {
		/* Get a URLClassLoader. It will load classes
		 * off of the java.class.path.
		 */ 
		URLClassLoader cl = new URLClassLoader(urls, null);
		
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

		/* Verify that jarfiles opened by getResourceAsStream
		   also get closed.
		*/
		InputStream stream = cl.getResourceAsStream("Test.class");
		System.out.println("InputStream: " + stream);
		
		/* Null out any references we have to the Test class and
		 * the application ClassLoader.
		 */
		cl = null;
		clazz = null;
		main = null;
		stream = null;
		
		/* Run the gc. If you set a breakpoint in CVMclassFree(), it
		 * should get hit for Test and any other application class
		 * loaded by Test.
		 */
		System.out.println("Running GC #1");
		java.lang.Runtime.getRuntime().gc();
		System.out.println("Running Finalizers #1");
		java.lang.Runtime.getRuntime().runFinalization();
		System.out.println("Running GC #2");
		java.lang.Runtime.getRuntime().gc();
		System.out.println("Running Finalizers #2");
		java.lang.Runtime.getRuntime().runFinalization();
		System.out.println("Done");
	    } catch (Throwable e) {
		e.printStackTrace();
	    }
	}
    }
}
