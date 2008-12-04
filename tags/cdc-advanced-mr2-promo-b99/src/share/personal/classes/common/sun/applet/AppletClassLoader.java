/*
 * @(#)AppletClassLoader.java	1.84 06/10/10
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

import java.net.URL;
import java.net.URLClassLoader;
import java.net.SocketPermission;
import java.net.URLConnection;
import java.net.MalformedURLException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.io.File;
import java.io.FilePermission;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.NoSuchElementException;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.security.PrivilegedActionException;
import java.security.CodeSource;
import java.security.PermissionCollection;
import sun.awt.AppContext;
import sun.awt.SunToolkit;

/**
 * This class defines the class loader for loading applet classes and
 * resources. It extends URLClassLoader to search the applet code base
 * for the class or resource after checking any loaded JAR files.
 */
public class AppletClassLoader extends URLClassLoader {
    private URL base;	/* applet code base URL */
    private CodeSource codesource; /* codesource for the base URL */
    private AccessControlContext acc;
    /*
     * Creates a new AppletClassLoader for the specified base URL.
     */
    protected AppletClassLoader(URL base) {
        super(new URL[0]);
        this.base = base;
        this.codesource = new CodeSource(base, null);
        acc = AccessController.getContext();
    }

    /*
     * Returns the applet code base URL.
     */
    URL getBaseURL() {
        return base;
    }

    /*
     * Returns the URLs used for loading classes and resources.
     */
    public URL[] getURLs() {
        URL[] jars = super.getURLs();
        URL[] urls = new URL[jars.length + 1];
        System.arraycopy(jars, 0, urls, 0, jars.length);
        urls[urls.length - 1] = base;
        return urls;
    }

    /*
     * Adds the specified JAR file to the search path of loaded JAR files.
     */
    void addJar(String name) {
        URL url;
        try {
            url = new URL(base, name);
        } catch (MalformedURLException e) {
            throw new IllegalArgumentException("name");
        }
        addURL(url);
        // DEBUG
        //URL[] urls = getURLs();
        //for (int i = 0; i < urls.length; i++) {
        //    System.out.println("url[" + i + "] = " + urls[i]);
        //}
    }

    /*
     * Override loadClass so that class loading errors can be caught in
     * order to print better error messages.
     */
    public synchronized Class loadClass(String name, boolean resolve)
        throws ClassNotFoundException {
        // First check if we have permission to access the package. This
        // should go away once we've added support for exported packages.
        int i = name.lastIndexOf('.');
        if (i != -1) {
            SecurityManager sm = System.getSecurityManager();
            if (sm != null)
                sm.checkPackageAccess(name.substring(0, i));
        }
        try {
            return super.loadClass(name, resolve);
        } catch (ClassNotFoundException e) {
            //printError(name, e.getException());
            throw e;
        } catch (RuntimeException e) {
            //printError(name, e);
            throw e;
        } catch (Error e) {
            //printError(name, e);
            throw e;
        }
    }

    /*
     * Finds the applet class with the specified name. First searches
     * loaded JAR files then the applet code base for the class.
     */
    protected Class findClass(String name) throws ClassNotFoundException {
        // check loaded JAR files
        try {
            return super.findClass(name);
        } catch (ClassNotFoundException e) {}
        // Otherwise, try loading the class from the code base URL
        final String path = name.replace('.', '/').concat(".class");
        try {
            byte[] b = (byte[]) AccessController.doPrivileged(
                    new PrivilegedExceptionAction() {
                        public Object run() throws IOException {
                            return getBytes(new URL(base, path));
                        }
                    }, acc);
            if (b != null) {
                return defineClass(name, b, 0, b.length, codesource);
            } else {
                throw new ClassNotFoundException(name);
            }
        } catch (PrivilegedActionException e) {
            throw new ClassNotFoundException(name, e.getException());
        }
    }

    /**
     * Returns the permissions for the given codesource object.
     * The implementation of this method first calls super.getPermissions,
     * to get the permissions
     * granted by the super class, and then adds additional permissions
     * based on the URL of the codesource.
     * <p>
     * If the protocol is "file"
     * and the path specifies a file, permission is granted to read all files 
     * and (recursively) all files and subdirectories contained in 
     * that directory. This is so applets with a codebase of
     * file:/blah/some.jar can read in file:/blah/, which is needed to
     * be backward compatible. We also add permission to connect back to
     * the "localhost".
     *
     * @param codesource the codesource
     * @return the permissions granted to the codesource
     */
    protected PermissionCollection getPermissions(CodeSource codesource) {
        final PermissionCollection perms = super.getPermissions(codesource);
        URL url = codesource.getLocation();
        if (url.getProtocol().equals("file")) {
            String path = url.getFile().replace('/', File.separatorChar);
            if (!path.endsWith(File.separator)) {
                int endIndex = path.lastIndexOf(File.separatorChar);
                if (endIndex != -1) {
                    path = path.substring(0, endIndex + 1) + "-";
                    perms.add(new FilePermission(path, "read"));
                }
            }
            perms.add(new SocketPermission("localhost", "connect,accept"));
            AccessController.doPrivileged(new PrivilegedAction() {
                    public Object run() {
                        try {
                            String host = InetAddress.getLocalHost().getHostName();
                            perms.add(new SocketPermission(host, "connect,accept"));
                        } catch (UnknownHostException uhe) {}
                        return null;
                    }
                }
            );
            if (base.getProtocol().equals("file")) {
                String bpath = base.getFile().replace('/', File.separatorChar);
                if (bpath.endsWith(File.separator)) {
                    bpath += "-";
                }
                perms.add(new FilePermission(bpath, "read"));
            }
        }
        return perms;
    }

    /*
     * Returns the contents of the specified URL as an array of bytes.
     */
    private static byte[] getBytes(URL url) throws IOException {
        URLConnection uc = url.openConnection();
        if (uc instanceof java.net.HttpURLConnection) {
            java.net.HttpURLConnection huc = (java.net.HttpURLConnection) uc;
            int code = huc.getResponseCode();
            if (code >= java.net.HttpURLConnection.HTTP_BAD_REQUEST) {
                throw new IOException("open HTTP connection failed.");
            }
        }
        int len = uc.getContentLength();
        InputStream in = uc.getInputStream();
        byte[] b;
        try {
            if (len != -1) {
                // Read exactly len bytes from the input stream
                b = new byte[len];
                while (len > 0) {
                    int n = in.read(b, b.length - len, len);
                    if (n == -1) {
                        throw new IOException("unexpected EOF");
                    }
                    len -= n;
                }
            } else {
                // Read until end of stream is reached
                b = new byte[1024];
                int total = 0;
                while ((len = in.read(b, total, b.length - total)) != -1) {
                    total += len;
                    if (total >= b.length) {
                        byte[] tmp = new byte[total * 2];
                        System.arraycopy(b, 0, tmp, 0, total);
                        b = tmp;
                    }
                }
                // Trim array to correct size, if necessary
                if (total != b.length) {
                    byte[] tmp = new byte[total];
                    System.arraycopy(b, 0, tmp, 0, total);
                    b = tmp;
                }
            }
        } finally {
            in.close();
        }
        return b;
    }

    /*
     * Finds the applet resource with the specified name. First checks
     * loaded JAR files then the applet code base for the resource.
     */
    public URL findResource(String name) {
        // check loaded JAR files
        URL url = super.findResource(name);
        if (url == null) {
            // otherwise, try the code base
            try {
                url = new URL(base, name);
                // check if resource exists
                if (!resourceExists(url))
                    url = null;
            } catch (Exception e) {
                // all exceptions, including security exceptions, are caught
                url = null;
            }
        }
        return url;
    }

    private boolean resourceExists(URL url) {
        // Check if the resource exists.
        // It almost works to just try to do an openConnection() but
        // HttpURLConnection will return true on HTTP_BAD_REQUEST
        // when the requested name ends in ".html", ".htm", and ".txt"
        // and we want to be able to handle these
        //
        // Also, cannot just open a connection for things like FileURLConnection,
        // because they suceed when connecting to a non-existant file.
        // So, in those cases we open and close an input stream.
        boolean ok = true;
        try {
            URLConnection conn = url.openConnection();
            if (conn instanceof java.net.HttpURLConnection) {
                java.net.HttpURLConnection hconn = 
                    (java.net.HttpURLConnection) conn;
                int code = hconn.getResponseCode();
                if (code == java.net.HttpURLConnection.HTTP_OK) {
                    return true;
                }
                if (code >= java.net.HttpURLConnection.HTTP_BAD_REQUEST) {
                    return false;
                }
            } else {
                // our best guess for the other cases
                InputStream is = url.openStream();
                is.close();
            }
        } catch (Exception ex) {
            ok = false;
        }
        return ok;
    }

    /*
     * Returns an enumeration of all the applet resources with the specified
     * name. First checks loaded JAR files then the applet code base for all
     * available resources.
     */
    public Enumeration findResources(String name) throws IOException {
        URL u = new URL(base, name);
        if (!resourceExists(u)) {
            u = null;
        }
        final Enumeration e = super.findResources(name);
        final URL url = u;
        return new Enumeration() {
                private boolean done;
                public Object nextElement() {
                    if (!done) {
                        if (e.hasMoreElements()) {
                            return e.nextElement();
                        }
                        done = true;
                        if (url != null) {
                            return url;
                        }
                    }
                    throw new NoSuchElementException();
                }

                public boolean hasMoreElements() {
                    return !done && (e.hasMoreElements() || url != null);
                }
            };
    }

    /*
     * Load and resolve the file specified by the applet tag CODE
     * attribute. The argument can either be the relative path
     * of the class file itself or just the name of the class.
     */
    Class loadCode(String name) throws ClassNotFoundException {
        // first convert any '/' or native file separator to .
        name = name.replace('/', '.');
        name = name.replace(File.separatorChar, '.');
        // save that name for later
        String fullName = name;
        // then strip off any suffixes
        if (name.endsWith(".class") || name.endsWith(".java")) {
            name = name.substring(0, name.lastIndexOf('.'));
        }
        try {
            return loadClass(name);
        } catch (ClassNotFoundException e) {}
        // then if it didn't end with .java or .class, or in the 
        // really pathological case of a class named class or java
        return loadClass(fullName);
    }
    /*
     * The threadgroup that the applets loaded by this classloader live
     * in. In the sun.* implementation of applets, the security manager's
     * (AppletSecurity) getThreadGroup returns the thread group of the 
     * first applet on the stack, which is the applet's thread group.
     */
    private AppletThreadGroup threadGroup;
    private AppContext appContext;
    synchronized ThreadGroup getThreadGroup() {
        if (threadGroup == null || threadGroup.isDestroyed()) {
            AccessController.doPrivileged(new PrivilegedAction() {
                    public Object run() {
                        threadGroup = new AppletThreadGroup(base + "-threadGroup");
                        // threadGroup.setDaemon(true);
                        // threadGroup is now destroyed by AppContext.dispose()

                        // Create the new AppContext from within a Thread belonging
                        // to the newly created ThreadGroup, and wait for the
                        // creation to complete before returning from this method.
                        AppContextCreator creatorThread = new AppContextCreator(threadGroup);
                        // Since this thread will later be used to launch the
                        // applet's AWT-event dispatch thread and we want the applet 
                        // code executing the AWT callbacks to use their own class
                        // loader rather than the system class loader, explicitly
                        // set the context class loader to the AppletClassLoader.
                        creatorThread.setContextClassLoader(AppletClassLoader.this);
                        synchronized (creatorThread.syncObject) {
                            creatorThread.start();
                            try {
                                creatorThread.syncObject.wait();
                            } catch (InterruptedException e) {}
                            appContext = creatorThread.appContext;
                        }
                        return null;
                    }
                }
            );
        }	    
        return threadGroup;
    }

    /*
     * Get the AppContext, if any, corresponding to this AppletClassLoader.
     */
    AppContext getAppContext() {
        return appContext;
    }
    int usageCount = 0;
    /**
     * Grab this AppletClassLoader and its ThreadGroup/AppContext, so they
     * won't be destroyed.
     */
    synchronized void grab() {
        usageCount++;
        getThreadGroup(); // Make sure ThreadGroup/AppContext exist
    }

    /**
     * Release this AppletClassLoader and its ThreadGroup/AppContext.
     * If nothing else has grabbed this AppletClassLoader, its ThreadGroup
     * and AppContext will be destroyed.
     * 
     * Because this method may destroy the AppletClassLoader's ThreadGroup,
     * this method should NOT be called from within the AppletClassLoader's
     * ThreadGroup.
     */
    synchronized void release() {
        if (usageCount > 1) {
            --usageCount;
        } else {
            if (appContext != null) {
                try {
                    appContext.dispose(); // nuke the world!
                } catch (IllegalThreadStateException e) {}
            }
            usageCount = 0;
            appContext = null;
            threadGroup = null;
        }
    }
    private static AppletMessageHandler mh =
        new AppletMessageHandler("appletclassloader");
    /*
     * Prints a class loading error message.
     */
    private static void printError(String name, Throwable e) {
        String s = null;
        if (e == null) {
            s = mh.getMessage("filenotfound", name);
        } else if (e instanceof IOException) {
            s = mh.getMessage("fileioexception", name);
        } else if (e instanceof ClassFormatError) {
            s = mh.getMessage("fileformat", name);
        } else if (e instanceof ThreadDeath) {
            s = mh.getMessage("filedeath", name);
        } else if (e instanceof Error) {
            s = mh.getMessage("fileerror", e.toString(), name);
        }
        if (s != null) {
            System.err.println(s);
        }
    }
}

/*
 * The AppContextCreator class is used to create an AppContext from within
 * a Thread belonging to the new AppContext's ThreadGroup.  To wait for
 * this operation to complete before continuing, wait for the notifyAll()
 * operation on the syncObject to occur.
 */
class AppContextCreator extends Thread {
    Object syncObject = new Object();
    AppContext appContext = null;
    AppContextCreator(ThreadGroup group) {
        super(group, "AppContextCreator");
    }

    public void run() {
        synchronized (syncObject) {
            appContext = SunToolkit.createNewAppContext();
            syncObject.notifyAll();
        }
    } // run()
} // class AppContextCreator
