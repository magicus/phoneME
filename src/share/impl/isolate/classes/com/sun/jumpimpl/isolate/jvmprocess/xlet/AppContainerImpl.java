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

package com.sun.jumpimpl.isolate.jvmprocess.xlet;

import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;

import sun.mtask.xlet.PXletManager;
import java.io.IOException;
import java.io.File;
import java.util.StringTokenizer;

/*
 * Application Container for the xlet app model.
 *
 * FIXME: We should move this class into the cdc/fp/pbp repository
 * and compile it there, to be aligned with midp app container in the
 * midp repository.  The code needs to be compiled before jump api
 * and after jump impl.
 */

public class AppContainerImpl extends JUMPAppContainer {

   public static JUMPAppContainer
       getInstance(JUMPAppContainerContext context) {
	   return new AppContainerImpl(); 
   }

    PXletManager xletManager;	 

    public static final String CLASSPATH_KEY = "XLETApplication_classpath";
    public static final String INITIAL_CLASS_KEY = "XLETApplication_initialClass";

    private static JUMPApplication currentApp = null;

    /**
     * Creates a new instance of JUMPAppContainer
     * For xlets, there is only one per vm - ignore appId.
     */
    public AppContainerImpl() {
    }
    
    /**
     * Start the application specific by the JUMPApplication object.
     */
    public int startApp(JUMPApplication app, String[] args) {
    
       try {

          String className = app.getProperty(INITIAL_CLASS_KEY);
          String classPath = app.getProperty(CLASSPATH_KEY);

	  StringTokenizer st = new StringTokenizer(classPath, File.pathSeparator);
	  int count = st.countTokens();
          String[] pathArray = new String[count];
          count = 0;

          while (st.hasMoreTokens()) {
                 pathArray[count++] = st.nextToken();
          }

          xletManager = PXletManager.createXlet(className, 
		       null, null, pathArray, args);
	  xletManager.postInitXlet();
	  xletManager.postStartXlet();

          // FIXME: xlet manager calls are asynchronous, need to wait. 

	  currentApp = app;

       } catch (IOException e) {
	       e.printStackTrace();
	       return -1;
       } 

       return Integer.parseInt(app.getProperty(JUMPApplication.ID_KEY));
    }
    
    public void pauseApp(int appId) {
       xletManager.postPauseXlet();	     
    }
    
    public void resumeApp(int appId) {
       xletManager.postStartXlet();	     
    }
    
    public void destroyApp(int appId, boolean force) {
       xletManager.postDestroyXlet(force);	     
       currentApp = null;
    }
    
    public void handleMessage(JUMPMessage message) {
    }

}
