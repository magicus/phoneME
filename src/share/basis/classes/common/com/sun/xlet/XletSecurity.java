/*
 * @(#)XletSecurity.java	1.6 06/10/10
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
 */

package com.sun.xlet;

import java.io.File;
import java.io.FilePermission;
import java.io.IOException;
import java.io.FileDescriptor;
import java.net.URL;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.net.SocketPermission;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.HashSet;
import java.util.StringTokenizer;
import java.security.*;
import sun.awt.AWTSecurityManager;
import sun.awt.AppContext;
import sun.security.provider.*;
import sun.security.util.SecurityConstants;


/**
 * This class defines an xlet security policy
 *
 * @version 	1.109, 01/23/03
 */
public
class XletSecurity extends AWTSecurityManager {
    private AppContext mainAppContext;

    /**
     * Construct and initialize.
     */
    public XletSecurity() {
	this(AppContext.getAppContext());
    }

    public XletSecurity(AppContext appContext) {
        // reset(); see reset().
	mainAppContext = appContext;
    }

    // Cache to store known restricted packages
    private HashSet restrictedPackages = new HashSet();

    /**
     * Reset from Properties
     *
     * this method exists to set additional package
     * access restrictions, which in Applet's case provided
     * in $USER/.appletviewer or $USER/.hotjava/properties file
     * as "package.restrict.access.<pkgname>, * <true | false>".
     * Xlets don't have this default property file, but I'm
     * leaving the method for now and commenting out the call
     * from the constructor.
     */
    public void reset()
    {
        // Clear cache
        restrictedPackages.clear();
                                                                                                       
        AccessController.doPrivileged(new PrivilegedAction() {
            public Object run()
            {
                // Enumerate system properties
                Enumeration e = System.getProperties().propertyNames();
                                                                                                       
                while (e.hasMoreElements())
                {
                    String name = (String) e.nextElement();
                                                                                                       
                    if (name != null && name.startsWith("package.restrict.access."))
                    {
                        String value = System.getProperty(name);
                                                                                                       
                        if (value != null && value.equalsIgnoreCase("true"))
                        {
                            String pkg = name.substring(24);
                                                                                                       
                            // Cache restricted packages
                            restrictedPackages.add(pkg);
                        }
                    }
                }
                return null;
            }
        });
    }

   
    /**
     * get the current (first) instance of an XletClassLoader on the stack.
     */
    private XletClassLoader currentXletClassLoader()
    {
/**
	// try currentClassLoader first
	ClassLoader loader = currentClassLoader();
	if ((loader == null) || (loader instanceof XletClassLoader))
	    return (XletClassLoader)loader;
**/
	ClassLoader loader;

	// if that fails, get all the classes on the stack and check them.
	Class[] context = getClassContext();
	for (int i = 0; i < context.length; i++) {
	    loader = context[i].getClassLoader();
	    if (loader instanceof XletClassLoader)
		return (XletClassLoader)loader;
	}

	// if that fails, try the context class loader
	loader = Thread.currentThread().getContextClassLoader();
	if (loader instanceof XletClassLoader)
	    return (XletClassLoader)loader;

	// no XletClassLoaders on the stack
	return (XletClassLoader)null;
    }

    /**
     * Returns true if this threadgroup is in the xlet's own thread
     * group. This will return false if there is no current class
     * loader.
     */
    protected boolean inThreadGroup(ThreadGroup g) {
        if ( currentXletClassLoader() != null)
	    return getThreadGroup().parentOf(g);

        return false;
    }

    /**
     * Returns true of the threadgroup of thread is in the xlet's
     * own threadgroup.
     */
    protected boolean inThreadGroup(Thread thread) {
	return inThreadGroup(thread.getThreadGroup());
    }

    /**
     * Xlets are not allowed to manipulate threads outside
     * xlet thread groups.
     */
    public synchronized void checkAccess(Thread t) {
	if (!inThreadGroup(t)) {
	    checkPermission(SecurityConstants.MODIFY_THREAD_PERMISSION);
	}
    }

    private boolean inThreadGroupCheck = false;

    /**
     * Xlets are not allowed to manipulate thread groups outside
     * xlet thread groups.
     */
    public synchronized void checkAccess(ThreadGroup g) {
	if (inThreadGroupCheck) {
	    // if we are in a recursive check, it is because
	    // inThreadGroup is calling xletLoader.getThreadGroup
	    // in that case, only do the super check, as xletLoader
	    // has a begin/endPrivileged
	    checkPermission(SecurityConstants.MODIFY_THREADGROUP_PERMISSION);
	} else {
	    try {
		inThreadGroupCheck = true;
		if (!inThreadGroup(g)) {
		    checkPermission(SecurityConstants.MODIFY_THREADGROUP_PERMISSION);
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

	// now check the list of restricted packages
	for (Iterator iter = restrictedPackages.iterator(); iter.hasNext();)
	{
	    String pkg = (String) iter.next();
	    
	    // Prevent matching "sun" and "sunir" even if they
	    // starts with similar beginning characters
	    //
	    if (pkgname.equals(pkg) || pkgname.startsWith(pkg + "."))
	    {
		checkPermission(new java.lang.RuntimePermission
			    ("accessClassInPackage." + pkgname));
	    }
	}
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
	XletClassLoader xletClassLoader = currentXletClassLoader();

	if ((appContext == mainAppContext) && (xletClassLoader != null)) {
	    // If we're about to allow access to the main EventQueue,
	    // and anything untrusted is on the class context stack,
	    // disallow access.
	    super.checkAwtEventQueueAccess();
	}
    } // checkAwtEventQueueAccess()

    /**
     * Returns the thread group of the xlet. We consult the classloader
     * if there is one.
     */
    public ThreadGroup getThreadGroup() {
        /* If any xlet code is on the execution stack, we return
           that xlet's ThreadGroup.  Otherwise, we use the default
           behavior. */
        XletClassLoader xletLoader = currentXletClassLoader();
        ThreadGroup loaderGroup = (xletLoader == null) ? null
                                          : xletLoader.getThreadGroup();
        if (loaderGroup != null) {
            return loaderGroup;
        } else {
            return super.getThreadGroup();
        }
    } // getThreadGroup()

    /**
      * Get the AppContext corresponding to the current context.
      * The default implementation returns null, but this method
      * may be overridden by various SecurityManagers
      * (e.g. XletSecurity) to index AppContext objects by the
      * calling context.
      * 
      * @return  the AppContext corresponding to the current context.
      * @see     sun.awt.AppContext
      * @see     java.lang.SecurityManager
      * @since   JDK1.2.1
      */
    public AppContext getAppContext() {
        XletClassLoader xletLoader = currentXletClassLoader();
        return (xletLoader == null) ? null : xletLoader.getAppContext();
    }

} // class XletSecurity
