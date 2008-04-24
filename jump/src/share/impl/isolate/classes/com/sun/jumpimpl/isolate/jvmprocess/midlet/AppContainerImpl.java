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

package com.sun.jumpimpl.isolate.jvmprocess.midlet;

import java.io.File;

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;

import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;

import com.sun.jumpimpl.process.JUMPModulesConfig;

import sun.misc.MIDPConfig;

import java.lang.reflect.Constructor;

/**
 * Application Container for the MIDlet app model.
 * <p>
 * This class is only a factory. The actual implementation is the 
 * com.sun.midp.jump.isolate.MIDletContainer class in the MIDP JAR.
 * <p>
 * The factory uses the system property sun.midp.home.path as the
 * directory of the MIDP JAR (classes.zip).
 */
/*
 * Impl note: We should change the build so that event this class
 * can be in the midp repository. This code needs to be compiled
 * before jump api and after jump impl.
 */
public class AppContainerImpl extends JUMPAppContainer {
    /**
     * Creates an app container for a MIDlet.
     * This method is called using reflection.
     */
    public static JUMPAppContainer
        getInstance(JUMPAppContainerContext context) {
        try {

            /*
             * The MIDlet container uses classes of the MIDP library, which
             * is only accessible through the MIDPImplementationClassLoader.
             * Hence the MIDlet container needs to be loaded by the MIDP
             * implementation classloader.
             */

            /* First, see if anyone created the classloader already */		 
            ClassLoader midpImplementationClassLoader =
	       MIDPConfig.getMIDPImplementationClassLoader();

	    if (midpImplementationClassLoader == null) {
	       //String midpJar = (String)
               //    context.getConfigProperty("jump.midp.classes.zip");

               midpImplementationClassLoader =
                   MIDPConfig.newMIDPImplementationClassLoader(null);
            }
             
            Class clazz =
                Class.forName("com.sun.midp.jump.isolate.MIDletContainer",
                              true, midpImplementationClassLoader);

	    Constructor constructor  = clazz.getDeclaredConstructor(
			    new Class[] {JUMPAppContainerContext.class});

            JUMPAppContainer midletContainer =
                (JUMPAppContainer)constructor.newInstance(
                    new Object[] {context});

            return midletContainer;
        } catch (ClassNotFoundException e) {
            // MIDPConfig not found, not a dual stack platform.
        } catch (Throwable e) { 
            // Other unexpected error? Let's report it.
            e.printStackTrace();
        }

        return null;
    }

    /**
     * Prevents this class from being created directly.
     */
    private AppContainerImpl() {
    }

    /**
     * Create a MIDlet and call its startApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     *
     * @param app application properties
     * @param args arguments for the app
     *
     * @return runtime application ID
     */
    public int startApp(JUMPApplication app, String[] args) {
        throw new RuntimeException("should not get called");
    }

    /**
     * Call a MIDlet's pauseApp method.
     * This method will not return until after the the MIDlet's pauseApp
     * method has returned.
     *
     * @param the application ID returned from startApp
     */    
    public void pauseApp(int appId) {
        throw new RuntimeException("should not get called");
    }
    
    /**
     * Call a MIDlet's startApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     *
     * @param the application ID returned from startApp
     */    
    public void resumeApp(int appId) {
        throw new RuntimeException("should not get called");
    }
    
    /**
     * Call a MIDlet's destroyApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     *
     * @param appId the application ID returned from startApp
     * @param force if false, give the app the option of not being destroyed
     */    
    public void destroyApp(int appId, boolean force) {
        throw new RuntimeException("should not get called");
    }
}
