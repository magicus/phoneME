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
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;

import java.io.IOException;
import java.io.File;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.net.MalformedURLException;
import java.util.StringTokenizer;
import sun.misc.CDCAppClassLoader;

/*
 * Application Container for the main(String[]) app model.
 * 
 * FIXME: We should move this class into the cdc/fp/pbp repository
 * and compile it there, to be aligned with midp app container in the
 * midp repository.  The code needs to be compiled before jump api
 * and after jump impl.
 */

public class AppContainerImpl extends JUMPAppContainer implements Runnable {

   private JUMPAppContainerContext context;

   public static JUMPAppContainer
       getInstance(JUMPAppContainerContext context) {
	   return new AppContainerImpl(context); 
   }

   public static final String CLASSPATH_KEY = "MAINApplication_classpath";
   public static final String INITIAL_CLASS_KEY = "MAINApplication_initialClass";

   private static JUMPApplication currentApp = null;
   private String[] args;

    /**
     * Creates a new instance of JUMPAppContainer
     * For main app, there is only one per vm - ignore appId.
     */
    public AppContainerImpl(JUMPAppContainerContext context) {
         this.context = context;
    }
    
    /**
     * Start the application specific by the JUMPApplication object.
     */
    public int startApp(JUMPApplication app, String[] mainArgs) {

         currentApp = app;
         args = mainArgs;
       
         Thread t = new Thread(this);
         t.setDaemon(false);
         t.start();

         context.terminateKeepAliveThread();

         return Integer.parseInt(app.getProperty(JUMPApplication.ID_KEY));
    }

    /**
     * Invokes the application's main() method.
     */
    public void run() {

       try {
          String className = currentApp.getProperty(INITIAL_CLASS_KEY);
          String classPath = currentApp.getProperty(CLASSPATH_KEY);

	  StringTokenizer st = new StringTokenizer(classPath, File.pathSeparator); 
	  int count = st.countTokens();
	  URL[] pathArray = new URL[count];

	  count = 0;

	  while (st.hasMoreTokens()) {
             try {		   
	        pathArray[count++] = new File(st.nextToken()).toURL();
	     } catch (MalformedURLException e) {	 
		System.err.println("Caught: " + e);
	        pathArray[count] = null;
             }
	  }

	  CDCAppClassLoader loader = new CDCAppClassLoader(
			  pathArray, null);

	  try {

	     Class [] args1 = {new String[0].getClass()};

	     // Main app typically expect zero length array for the main(Str[]) 
	     // parameter, instead of null. 
	     String [] args2 = (args == null)? new String[0] : args;

	     Class mainClass = loader.loadClass(className);
	     Method mainMethod = mainClass.getMethod("main", args1);
             mainMethod.setAccessible(true);
	     mainMethod.invoke(null, new Object[]{args2});

	  } catch (InvocationTargetException i) {
             throw i.getTargetException();
          }

       } catch (Throwable e) {
	       if (e instanceof Error)
		       throw (Error) e;

	       e.printStackTrace();
       }

    }
    
    public void pauseApp(int appId) {
       System.out.println("Main AppContainer pausing " + currentApp);
    }
    
    public void resumeApp(int appId) {
       System.out.println("Main AppContainer resuming " + currentApp);
    }
    
    public void destroyApp(int appId, boolean force) {
       System.out.println("Main AppContainer destroying " + currentApp);

       currentApp = null;

       context.terminateIsolate();
    }
    
    public void handleMessage(JUMPMessage message) {
    }

}
