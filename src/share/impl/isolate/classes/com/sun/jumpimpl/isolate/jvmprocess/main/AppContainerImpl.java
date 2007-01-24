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

package com.sun.jumpimpl.isolate.jvmprocess.main;

import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;

import java.io.IOException;
import java.io.File;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.net.URLClassLoader;

/*
 * Application Container for the main(String[]) app model.
 * 
 * FIXME: We should move this class into the cdc/fp/pbp repository
 * and compile it there, to be aligned with midp app container in the
 * midp repository.  The code needs to be compiled before jump api
 * and after jump impl.
 */

public class AppContainerImpl extends JUMPAppContainer {

   public static JUMPAppContainer getInstance() {
	   return new AppContainerImpl(); 
   }

   public static final String CLASSPATH_KEY = "MAINApplication_classpath";
   public static final String INITIAL_CLASS_KEY = "MAINApplication_initialClass";

   private static JUMPApplication currentApp = null;

    /**
     * Creates a new instance of JUMPAppContainer
     * For main app, there is only one per vm - ignore appId.
     */
    public AppContainerImpl() {
    }
    
    /**
     * Start the application specific by the JUMPApplication object.
     */
    public int startApp(JUMPApplication app, String[] mainArgs) {

       try {

          String className = app.getProperty(INITIAL_CLASS_KEY);
          File classPath = new File(app.getProperty(CLASSPATH_KEY)).getCanonicalFile();

	  URLClassLoader loader = new URLClassLoader(
			  new URL[] {classPath.toURL()}, 
			  ClassLoader.getSystemClassLoader());

	  try {
	     Class [] args1 = {new String[0].getClass()};
	     Object [] args2 = {mainArgs};

	     Class mainClass = loader.loadClass(className);
	     Method mainMethod = mainClass.getMethod("main", args1);
	     mainMethod.invoke(null, args2);
	  } catch (InvocationTargetException i) {
             throw i.getTargetException();
          }

	  currentApp = app;

       } catch (Throwable e) {
	       if (e instanceof Error)
		       throw (Error) e;

	       e.printStackTrace();
	       return -1;
       }

       return 1; // only one app per isolate
    }
    
    public void pauseApp(int appId) {
       System.out.println("Main AppContainer pausing " + currentApp);
    }
    
    public void resumeApp(int appId) {
       System.out.println("Main AppContainer resuming " + currentApp);
    }
    
    public void destroyApp(int appId, boolean force) {
       System.out.println("Main AppContainer destroying " + currentApp);

       // FIXME: How do I kill myself?
       currentApp = null;
    }
    
    public void handleMessage(JUMPMessage message) {
    }

    public static JUMPApplication getCurrentApplication() {
       return currentApp;
    }
}
