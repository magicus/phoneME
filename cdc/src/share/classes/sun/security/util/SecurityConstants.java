/*
 * @(#)SecurityConstants.java	1.12 06/10/10
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

package sun.security.util;

import java.io.FilePermission;
import java.lang.RuntimePermission;
import java.net.SocketPermission;
import java.net.NetPermission;
import java.security.SecurityPermission;
import java.security.AllPermission;
import java.security.BasicPermission;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
/* javax.security.auth.AuthPermission subsetted out
 * of CDC
import javax.security.auth.AuthPermission;
 */

/*
 * Note that there are two versions of this file, this subsetted
 * version for CDC and another for the security optional package.
 * Be sure you're editting the right one!
 */

/**
 * Permission constants and string constants used to create permissions
 * used throughout the JDK.
 */
public final class SecurityConstants {
    // Cannot create one of these
    private SecurityConstants () {
    }

    // Commonly used string constants for permission actions used by
    // SecurityManager. Declare here for shortcut when checking permissions
    // in FilePermssion, SocketPermission, and PropertyPermission.
    
    public static final String FILE_DELETE_ACTION = "delete";
    public static final String FILE_EXECUTE_ACTION = "execute";
    public static final String FILE_READ_ACTION = "read";
    public static final String FILE_WRITE_ACTION = "write";

    public static final String SOCKET_RESOLVE_ACTION = "resolve";
    public static final String SOCKET_CONNECT_ACTION = "connect";
    public static final String SOCKET_LISTEN_ACTION = "listen";
    public static final String SOCKET_ACCEPT_ACTION = "accept";
    public static final String SOCKET_CONNECT_ACCEPT_ACTION = "connect,accept";

    public static final String PROPERTY_RW_ACTION = "read,write";
    public static final String PROPERTY_READ_ACTION = "read";
    public static final String PROPERTY_WRITE_ACTION = "write";

    // Permission constants used in the various checkPermission() calls in JDK.

    // java.lang.Class, java.lang.SecurityManager, java.lang.System,
    // java.net.URLConnection, java.security.AllPermission, java.security.Policy,
    // sun.security.provider.PolicyFile
    public static final AllPermission ALL_PERMISSION = new AllPermission();

    
    // java.net.URL
    public static final NetPermission SPECIFY_HANDLER_PERMISSION =
       new NetPermission("specifyStreamHandler");

    // java.lang.SecurityManager, sun.applet.AppletPanel, sun.misc.Launcher
    public static final RuntimePermission CREATE_CLASSLOADER_PERMISSION =
        new RuntimePermission("createClassLoader");

    // java.lang.SecurityManager
    public static final RuntimePermission CHECK_MEMBER_ACCESS_PERMISSION =
	new RuntimePermission("accessDeclaredMembers");

    // java.lang.SecurityManager, sun.applet.AppletSecurity
    public static final RuntimePermission MODIFY_THREAD_PERMISSION = 
        new RuntimePermission("modifyThread");

    // java.lang.SecurityManager, sun.applet.AppletSecurity
    public static final RuntimePermission MODIFY_THREADGROUP_PERMISSION =
        new RuntimePermission("modifyThreadGroup");

    // java.lang.Class
    public static final RuntimePermission GET_PD_PERMISSION =
        new RuntimePermission("getProtectionDomain");

    // java.lang.Class, java.lang.ClassLoader, java.lang.Thread
    public static final RuntimePermission GET_CLASSLOADER_PERMISSION =
        new RuntimePermission("getClassLoader");

    // java.lang.Thread
    public static final RuntimePermission STOP_THREAD_PERMISSION =
       new RuntimePermission("stopThread");

    // java.security.AccessControlContext
    public static final SecurityPermission CREATE_ACC_PERMISSION =
       new SecurityPermission("createAccessControlContext");

    // java.security.AccessControlContext
    public static final SecurityPermission GET_COMBINER_PERMISSION =
       new SecurityPermission("getDomainCombiner");

    // java.security.Policy, java.security.ProtectionDomain
    public static final SecurityPermission GET_POLICY_PERMISSION =
        new SecurityPermission ("getPolicy");

    // java.lang.SecurityManager
    public static final SocketPermission LOCAL_LISTEN_PERMISSION =
	new SocketPermission("localhost:1024-", SOCKET_LISTEN_ACTION);

    /* javax.security.auth.AuthPermission subsetted out of CDC.
    // javax.security.auth.Subject
    public static final AuthPermission DO_AS_PERMISSION =
        new AuthPermission("doAs");
    
    // javax.security.auth.Subject
    public static final AuthPermission DO_AS_PRIVILEGED_PERMISSION =
        new AuthPermission("doAsPrivileged");
    */

    // Make this class more friendly for mTASK:
    // To eagerly initialize topLevelWindowPermission,
    // accessClipboardPermission and checkAwtEventQueuePermission,
    // the private static method initAwtPerms() is now changed to
    // be a static initializer. 
    //
    // The static variable, isAwtPermInitialized is no longer in use.
    //
    //private static boolean isAwtPermInitialized = false;

    private static BasicPermission topLevelWindowPermission;

    private static BasicPermission accessClipboardPermission;

    private static BasicPermission checkAwtEventQueuePermission;

    public static BasicPermission getTopLevelWindowPermission() {
       //if (!isAwtPermInitialized)
       //    initAwtPerms();
       return topLevelWindowPermission;
    }

    public static BasicPermission getAccessClipboardPermission() {
       //if (!isAwtPermInitialized)
       //    initAwtPerms();
       return accessClipboardPermission;
    }

    public static BasicPermission getCheckAwtEventQueuePermission() {
       //if (!isAwtPermInitialized)
       //    initAwtPerms();
       return checkAwtEventQueuePermission;
    }

    // use reflection to find out whether AWT classes are available
    static {
       Constructor AwtPermissionCtor = null;
       try {
          AwtPermissionCtor = Class.forName("java.awt.AWTPermission").
                         getConstructor(new Class[] { String.class });
       } catch (ClassNotFoundException ce) {
          // No AWT, so what are you playing with windows for?
	  //isAwtPermInitialized = true; // no longer need
	  //return; 
       } catch (NoSuchMethodException ne) {
          throw new SecurityException("AWTPermission constructor changed");
       }

       if (AwtPermissionCtor != null) {
           try { 
              topLevelWindowPermission = (BasicPermission)
                    AwtPermissionCtor.newInstance(new Object[] {
                             "showWindowWithoutWarningBanner" });
           } catch (InstantiationException ie) {
           } catch (IllegalAccessException iae) { 
           } catch (InvocationTargetException ite) { 
           }
           try { 
              checkAwtEventQueuePermission = (BasicPermission)
                    AwtPermissionCtor.newInstance(new Object[] {
                             "accessEventQueue" });
           } catch (InstantiationException ie) {
           } catch (IllegalAccessException iae) { 
           } catch (InvocationTargetException ite) { 
           }
           try { 
              accessClipboardPermission = (BasicPermission)
                    AwtPermissionCtor.newInstance(new Object[] {
                             "accessClipboard" });
           } catch (InstantiationException ie) {
           } catch (IllegalAccessException iae) { 
           } catch (InvocationTargetException ite) { 
           }

           //isAwtPermInitialized = true; // no longer need
           //return; 
       }
    }

}
