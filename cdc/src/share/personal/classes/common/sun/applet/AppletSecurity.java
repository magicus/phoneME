/*
 * @(#)AppletSecurity.java	1.89 06/10/10
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

package sun.applet;

import java.io.File;
import java.io.FilePermission;
import java.io.IOException;
import java.io.FileDescriptor;
import java.net.URL;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.net.SocketPermission;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.Hashtable;
import java.security.*;
import sun.awt.AWTSecurityManager;
import sun.awt.AppContext;
import sun.security.provider.*;

/**
 * This class defines an applet security policy
 *
 * @version 	1.85, 08/19/02
 */
public
class AppletSecurity extends AWTSecurityManager {
    private AppContext mainAppContext;
    /**
     * Construct and initialize.
     */
    public AppletSecurity() {
        reset();
        mainAppContext = AppContext.getAppContext();
    }

    /**
     * Reset from Properties
     */
    public void reset() {}
    /**
     * get the current (first) instance of an AppletClassLoader on the
     * execution stack. Returns null if a call to checkPermission with
     * java.security.AllPermission does not result in a SecurityException,
     * or if no AppletClassLoader is found.
     */

    private static AllPermission allPermission;
    private AppletClassLoader currentAppletClassLoader() {
        try {
            if (allPermission == null) {
                allPermission = new AllPermission();
            }
            checkPermission(allPermission);
        } catch (SecurityException se) {
            ClassLoader loader;
            Class[] context = getClassContext();
            for (int i = 0; i < context.length; i++) {
                loader = context[i].getClassLoader();
                if (loader instanceof AppletClassLoader) {
                    return (AppletClassLoader) loader;
                }
            }
            // if that fails, try the context class loader
            loader = Thread.currentThread().getContextClassLoader();
            if (loader instanceof AppletClassLoader) {
                return (AppletClassLoader) loader;
            }
        }
        // no AppletClassLoader found, or we have AllPermission
        return (AppletClassLoader) null;
    }

    /**
     * Returns true if this threadgroup is in the applet's own thread
     * group. This will return false if there is no current applet class
     * loader.
     */
    protected boolean inThreadGroup(ThreadGroup g) {
        if (currentAppletClassLoader() == null)
            return false;
        else
            return getThreadGroup().parentOf(g);
    }

    /**
     * Returns true of the threadgroup of thread is in the applet's
     * own threadgroup.
     */
    protected boolean inThreadGroup(Thread thread) {
        return inThreadGroup(thread.getThreadGroup());
    }

    /**
     * Applets are not allowed to manipulate threads outside
     * applet thread groups.
     */
    public synchronized void checkAccess(Thread t) {
        if (!inThreadGroup(t)) {
            if (threadPermission == null)
                threadPermission = new RuntimePermission("modifyThread");
            checkPermission(threadPermission);
        }
    }
    private static RuntimePermission threadPermission;
    private static RuntimePermission threadGroupPermission;
    private boolean inThreadGroupCheck = false;
    /**
     * Applets are not allowed to manipulate thread groups outside
     * applet thread groups.
     */
    public synchronized void checkAccess(ThreadGroup g) {
        if (inThreadGroupCheck) {
            // if we are in a recursive check, it is because
            // inThreadGroup is calling appletLoader.getThreadGroup
            // in that case, only do the super check, as appletLoader
            // has a begin/endPrivileged
            if (threadGroupPermission == null)
                threadGroupPermission = 
                        new RuntimePermission("modifyThreadGroup");
            checkPermission(threadGroupPermission);
        } else {
            try {
                inThreadGroupCheck = true;
                if (!inThreadGroup(g)) {
                    if (threadGroupPermission == null)
                        threadGroupPermission = 
                                new RuntimePermission("modifyThreadGroup");
                    checkPermission(threadGroupPermission);
                }
            } finally {
                inThreadGroupCheck = false;
            }
        }
    }
    
    /**
     * Throws a <code>SecurityException</code> if the 
     * calling thread is not allowed to access the package specified by 
     * the argument. 
     * <p>
     * This method is used by the <code>loadClass</code> method of class 
     * loaders. 
     * <p>
     * The <code>checkPackageAccess</code> method for class 
     * <code>SecurityManager</code>  calls
     * <code>checkPermission</code> with the
     * <code>RuntimePermission("accessClassInPackage."+pkg)</code>
     * permission.
     *
     * @param      pkg   the package name.
     * @exception  SecurityException  if the caller does not have
     *             permission to access the specified package.
     * @see        java.lang.ClassLoader#loadClass(java.lang.String, boolean)
     */
    public void checkPackageAccess(final String pkgname) {
        // first see if the VM-wide policy allows access to this package
        super.checkPackageAccess(pkgname);
        final boolean[] check = { false };
        AccessController.doPrivileged(new PrivilegedAction() {
                public Object run() {
                    int i;
                    String pkg = pkgname;
                    do {
                        String prop = "package.restrict.access." + pkg;
                        if (Boolean.getBoolean(prop)) {
                            check[0] = true;
                            break;
                        }
                        if ((i = pkg.lastIndexOf('.')) != -1) {
                            pkg = pkg.substring(0, i);
                        }
                    }
                    while (i != -1);
                    return null;
                }
            }
        );
        if (check[0])
            checkPermission(new java.lang.RuntimePermission
                ("accessClassInPackage." + pkgname));
    }

    /**
     * Tests if a client can get access to the AWT event queue.
     * <p>
     * This method calls <code>checkPermission</code> with the
     * <code>AWTPermission("accessEventQueue")</code> permission.
     *
     * @since   JDK1.1
     * @exception  SecurityException  if the caller does not have 
     *             permission to accesss the AWT event queue.
     */
    public void checkAwtEventQueueAccess() {
        AppContext appContext = AppContext.getAppContext();
        if ((appContext == mainAppContext) &&
            (currentAppletClassLoader() != null)) {
            // If we're about to allow access to the main EventQueue,
            // and anything untrusted is on the class context stack,
            // disallow access.
            super.checkAwtEventQueueAccess();
        }
    } // checkAwtEventQueueAccess()

    /**
     * Returns the thread group of the applet. We consult the classloader
     * if there is one.
     */
    public ThreadGroup getThreadGroup() {
        /* If any applet code is on the execution stack, we return
         that applet's ThreadGroup.  Otherwise, we use the default
         behavior. */
        AppletClassLoader appletLoader = currentAppletClassLoader();
        return (appletLoader == null) ? super.getThreadGroup() :
            appletLoader.getThreadGroup();
    }

    /**
     * Get the AppContext corresponding to the current context.
     * The default implementation returns null, but this method
     * may be overridden by various SecurityManagers
     * (e.g. AppletSecurity) to index AppContext objects by the
     * calling context.
     * 
     * @return  the AppContext corresponding to the current context.
     * @see     sun.awt.AppContext
     * @see     java.lang.SecurityManager
     * @since   JDK1.2.1
     */
    public AppContext getAppContext() {
        AppletClassLoader appletLoader = currentAppletClassLoader();
        return (appletLoader == null) ? null : appletLoader.getAppContext();
    }
} // class AppletSecurity
