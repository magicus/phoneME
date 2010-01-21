/*
 * @(#)Launcher.java	1.52 06/11/07
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

package sun.misc;

import java.io.File;
import java.io.IOException;
import java.io.FilePermission;
import java.io.InputStream;
import java.io.FileInputStream;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.MalformedURLException;
import java.net.URLStreamHandler;
import java.net.URLStreamHandlerFactory;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.jar.Attributes;
import java.util.jar.Attributes.Name;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.Manifest;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.security.AccessControlContext;
import java.security.PermissionCollection;
import java.security.Permissions;
import java.security.Permission;
import java.security.ProtectionDomain;
import java.security.CodeSource;
import sun.security.action.GetPropertyAction;
import sun.net.www.ParseUtil;

/**
 * This class is used by the system to launch the main application.
Launcher */
public class Launcher {
    private static Launcher launcher;
    private static URLStreamHandlerFactory factory;

    static {
	factory = new Factory();
	launcher = (Launcher)
	    AccessController.doPrivileged(new PrivilegedAction() {
		public Object run() {
		    return new Launcher();
		}
        });
    }

    public static Launcher getLauncher() {
	return launcher;
    }

    //
    // Private static to get to the application classloader instance
    // of the singleton launcher
    //
    private static ClassLoader getAppClassLoader() {
	return launcher.loader;
    }

    // Utility to be shared between app class loader and extension class
    // loader
    static boolean inList(URL url, URL[] list) {
	for (int i = 0; i < list.length; i++) {
	    if (url.equals(list[i])) {
		return true;
	    }
	}
	return false;
    }
	
    /*
     * Update app class loader with a new java.class.path
     * value.
     */
    public static void updateLauncher() throws IOException
    {
	//
	// Invalidate cached boot class path, so it can be recomputed
	// the next time it is needed
	//
	bootstrapClassPath = null;

	// Handle new classpath for the app loader
	AppClassLoader appLoader = (AppClassLoader)launcher.loader;
	appLoader.updateWithNewClasspath();

	// Handle the ext loader if java.ext.dirs is specified.
	ExtClassLoader extLoader = (ExtClassLoader)appLoader.getParent();
	extLoader.updateWithNewExtdirs();

	// Finally, handle any indication of a security manager request.
	installSecurityManagerIfRequested(appLoader);
    }
    
    private ClassLoader loader;

    public Launcher() {
	// Create the extension class loader unconditionally.
	ClassLoader extcl;
	try {
	    extcl = ExtClassLoader.getExtClassLoader();
	} catch (IOException e) {
	    throw new InternalError(
		"Could not create extension class loader");
	}

	// Now create the class loader to use to launch the application
	try {
	    loader = AppClassLoader.getAppClassLoader(extcl);
	} catch (IOException e) {
	    throw new InternalError(
		"Could not create application class loader");
	}

        // Set the loader as the systemClassLoader
        CVM.setSystemClassLoader(loader);

	// Register in case application classes were ROMized
	CVM.Preloader.registerClassLoader("sys", loader);

	// Also set the context class loader for the primordial thread.
	Thread.currentThread().setContextClassLoader(loader);

	// Finally, create security manager if necessary.
	installSecurityManagerIfRequested(loader);
    }

    private static void installSecurityManagerIfRequested(ClassLoader loader) {
	// Finally, install a security manager if requested
	String s = System.getProperty("java.security.manager");
	if (s != null) {
	    SecurityManager sm = null;
	    if ("".equals(s) || "default".equals(s)) {
		sm = new java.lang.SecurityManager();
	    } else {
		try {
		    sm = (SecurityManager)loader.loadClass(s).newInstance();
		} catch (IllegalAccessException e) {
		} catch (InstantiationException e) {
		} catch (ClassNotFoundException e) {
		} catch (ClassCastException e) {
		}
	    }
	    if (sm != null) {
		System.setSecurityManager(sm);
	    } else {
		throw new InternalError(
		    "Could not create SecurityManager: " + s);
	    }
	}
    }

    /*
     * Returns the class loader used to launch the main application.
     */
    public ClassLoader getClassLoader() {
	return loader;
    }

    /*
     * The class loader used for loading installed extensions.
     */
    static class ExtClassLoader extends URLClassLoader {
	private File[] dirs;

	/**
	 * create an ExtClassLoader. The ExtClassLoader is created
	 * within a context that limits which files it can read
	 */
	public static ExtClassLoader getExtClassLoader() throws IOException
	{
	    final File[] dirs = getExtDirs();

	    try {
		// Prior implementations of this doPrivileged() block supplied 
		// aa synthesized ACC via a call to the private method
		// ExtClassLoader.getContext().

		return (ExtClassLoader) AccessController.doPrivileged(
		     new PrivilegedExceptionAction() {
			public Object run() throws IOException {
			    return new ExtClassLoader(dirs);
			}
		    });
	    } catch (java.security.PrivilegedActionException e) {
		    throw (IOException) e.getException();
	    }
	}
	
	void addExtURL(URL url) {
		super.addURL(url);
	}

	/*
	 * Creates a new ExtClassLoader for the specified directories.
	 */
	public ExtClassLoader(File[] dirs) throws IOException {
	    super(getExtURLs(dirs), null, factory);
	    this.dirs = dirs;
	    // Register in case ext classes were ROMized
	    CVM.Preloader.registerClassLoader("ext", this);
	}

	/*
	 * Adds to current java.ext.dirs 
	 */
	void updateWithNewExtdirs() throws IOException
	{
	    // This reflects any past and current installed extensions
	    URL[] origUrls = this.getURLs();

	    // Now add those URL's we don't already know about.
	    URL[] urls = getExtURLs(getExtDirs());
	    for (int i = 0; i < urls.length; i++) {
		URL url = urls[i];
		if (!inList(url, origUrls)) {
		    this.addURL(url);
		}
	    }
	}

	private static File[] getExtDirs() {
	    String s = System.getProperty("java.ext.dirs");
	    File[] dirs;
	    if (s != null) {
		StringTokenizer st = 
		    new StringTokenizer(s, File.pathSeparator);
		int count = st.countTokens();
		dirs = new File[count];
		for (int i = 0; i < count; i++) {
		    dirs[i] = new File(st.nextToken());
		}
	    } else {
		dirs = new File[0];
	    }
	    return dirs;
	}

	private static URL[] getExtURLs(File[] dirs) throws IOException {
	    Vector urls = new Vector();
	    for (int i = 0; i < dirs.length; i++) {
		String[] files = dirs[i].list();
		if (files != null) {
		    for (int j = 0; j < files.length; j++) {
			File f = new File(dirs[i], files[j]);
			urls.add(getFileURL(f));
		    }
		}
	    }
	    URL[] ua = new URL[urls.size()];
	    urls.copyInto(ua);
	    return ua;
	}

	/*
	 * Searches the installed extension directories for the specified
	 * library name. For each extension directory, we first look for
	 * the native library in the subdirectory whose name is the value
	 * of the system property <code>os.arch</code>. Failing that, we
	 * look in the extension directory itself.
	 */
	public String findLibrary(String name) {
	    name = System.mapLibraryName(name);
	    for (int i = 0; i < dirs.length; i++) {
		// Look in architecture-specific subdirectory first
		String arch = System.getProperty("os.arch");
		if (arch != null) {
		    File file = new File(new File(dirs[i], arch), name);
		    if (file.exists()) {
			return file.getAbsolutePath();
		    }
		}
		// Then check the extension directory
		File file = new File(dirs[i], name);
		if (file.exists()) {
		    return file.getAbsolutePath();
		}
	    }
	    return null;
	}
    }

    /**
     * This is the container object returned from the native side.
     */
    static class ClassContainer {
	URL url; /* The URL of this path component */
	
	String entryname; /* The name of the class entry (.class name) */
	
	JarFile jfile;
	Class clazz; /* The class that has been defined */

	ClassContainer(URL url, String entryname,
		       JarFile jfile) {
	    this.url = url;
	    this.entryname = entryname;
	    this.jfile = jfile;
	    this.clazz = null;
	}
	
	ClassContainer(URL url, Class clazz,
		       String entryname, JarFile jfile) {
	    this.url = url;
	    this.entryname = entryname;
	    this.jfile = jfile;
	    this.clazz = clazz;
	}
    }


    /**
     * The class loader used for loading from java.class.path.
     * runs in a restricted security context.
     */
    static class AppClassLoader extends URLClassLoader {
        // Flag for extension information of JAR files
        // from java.class.path.
        private static boolean hasExtension = false;

        // Called by classPathInit() from classload.c if
        // extension information is found in JAR manifests.
        public static void setExtInfo() {
            hasExtension = true; 
        }

	public static ClassLoader getAppClassLoader(final ClassLoader extcl)
	    throws IOException
	{
	    final String s = System.getProperty("java.class.path");
	    final File[] path = (s == null) ? new File[0] : getClassPath(s);

	    // Note: on bugid 4256530
	    // Prior implementations of this doPrivileged() block supplied 
	    // a rather restrictive ACC via a call to the private method
	    // AppClassLoader.getContext(). This proved overly restrictive
	    // when loading  classes. Specifically it prevent
	    // accessClassInPackage.sun.* grants from being honored.
	    //
	    return (AppClassLoader) 
		AccessController.doPrivileged(new PrivilegedAction() {
		public Object run() {
		    URL[] urls =
			(s == null) ? new URL[0] : pathToURLs(path);
		    return new AppClassLoader(urls, extcl);
		}
	    });
	}

	/*
	 * Creates a new AppClassLoader
	 */
	AppClassLoader(URL[] urls, ClassLoader parent) {
	    super(urls, parent, factory);
	}

	/*
	 * In case java.class.path has been updated, update the set of
	 * URL's that this class loader has been initialized with.
	 * 
	 * Use URLClassLoader.{getURLs(), addURL()}
	 */
	void updateWithNewClasspath() throws IOException
	{
	    final String s = System.getProperty("java.class.path");
	    final File[] path = (s == null) ? new File[0] : getClassPath(s);
	    URL[] urls = pathToURLs(path);
	    URL[] origUrls = this.getURLs();
	    
	    /* Now add any new URL's not found in the original list */
	    for (int i = 0; i < urls.length; i++) {
		URL url = urls[i];
		if (!inList(url, origUrls)) {
		    this.addURL(url);
		}
	    }
	}

	/**
	 * Override loadClass so we can checkPackageAccess.
	 */
	public synchronized Class loadClass(String name, boolean resolve)
	    throws ClassNotFoundException
	{
	    int i = name.lastIndexOf('.');
	    if (i != -1) {
		SecurityManager sm = System.getSecurityManager();
		if (sm != null) {
		    sm.checkPackageAccess(name.substring(0, i));
		}
	    }
	    return (super.loadClass(name, resolve));
	}

	/*
	 * Returns true if the specified package name is sealed according to the
	 * given manifest.
	 */
	private boolean isSealedPrivate(String name, Manifest man) {
	    String path = name.replace('.', '/').concat("/");
	    Attributes attr = man.getAttributes(path);
	    String sealed = null;
	    if (attr != null) {
		sealed = attr.getValue(Name.SEALED);
	    }
	    if (sealed == null) {
		if ((attr = man.getMainAttributes()) != null) {
		    sealed = attr.getValue(Name.SEALED);
		}
	    }
	    return "true".equalsIgnoreCase(sealed);
	}

	private byte[] getBytesPrivate(InputStream in, int len) 
            throws IOException {
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

	private void handlePackage(String name, Manifest man, URL url)
	    throws IOException
				   
	{
	    int i = name.lastIndexOf('.');
	    if (i == -1) {
		return;
	    }
	    String pkgname = name.substring(0, i);
            if (name.startsWith("java.")) {
                throw new SecurityException("Prohibited package name: " +
                                        pkgname);
            }

	    
	    // Check if package already loaded.
	    Package pkg = getPackage(pkgname);
	    
	    if (pkg != null) {
		// Package found, so check package sealing.
		boolean ok;
		if (pkg.isSealed()) {
		    // Verify that code source URL is the same.
		    ok = pkg.isSealed(url);
		} else {
		    // Make sure we are not attempting to seal the package
		    // at this code source URL.
		    ok = (man == null) || !isSealedPrivate(pkgname, man);
		}
		if (!ok) {
		    throw new SecurityException("sealing violation");
		}
	    } else {
		if (man != null) {
		    definePackage(pkgname, man, url);
		} else {
		    definePackage(pkgname, null, null, null, null, null, null, null);
		}
	    }
	}
	
	private Class defineClassPrivate(String name, ClassContainer cc) 
            throws IOException {
	    URL url = cc.url;
            JarFile jfile = cc.jfile;
	    Manifest man;

	    if (jfile != null) { // Must be a ZIP/JAR
		man = jfile.getManifest();
	    } else {
		man = null;
	    }
		
	    // See if there are any package operations to do
	    handlePackage(name, man, url);

	    if (cc.clazz != null) {
		/* load superclasses in a way that avoid C recursion */
		sun.misc.CVM.executeLoadSuperClasses(cc.clazz);
		return cc.clazz;
	    } else {
                String path = cc.entryname;
                JarEntry entry = jfile.getJarEntry(path);
                InputStream in = jfile.getInputStream(entry);
                int len = (int)entry.getSize();

		// Now get the class bytes and define the class
		byte[] b = getBytesPrivate(in, len);
                java.security.cert.Certificate[] certs = entry.getCertificates();
		CodeSource cs = new CodeSource(url, certs);
		return defineClass(name, b, 0, b.length, cs);
	    }
	}
	
	private Class doClassFind(final String name)
	    throws ClassNotFoundException, 
		   java.security.PrivilegedActionException
	{
	    return (Class)
		AccessController.doPrivileged(new PrivilegedExceptionAction() {
		    public Object run() throws ClassNotFoundException {
			ClassContainer cc = findContainer(name);
			if (cc != null) {
			    try {
				return defineClassPrivate(name, cc);
			    } catch (IOException e) {
				throw new ClassNotFoundException(name, e);
			    }
			} else {
			    throw new ClassNotFoundException(name);
			}
		    }
               });
	}

        /* 
         * Check syntax of name passed to defineClass() or findClass methods
         * array syntax clasnames with /'s are not allowed
         */
        private boolean checkName(String name) {
            if(name == null || name.length() == 0)
                return true;
            if (name.indexOf('/') != -1)
                return false;
            if (name.charAt(0) == '[')
                return false;
            return true;
        }
	

        /**
         * Override findClass
         */
        protected Class findClass(final String name)
            throws ClassNotFoundException
        {
            if (!checkName(name))
                throw new ClassNotFoundException("Illegal name: " + name);

            // If Extension Mechanism is supported
            // we use URLClassLoader's implementation.
            // Otherwise we use our own.
            if (hasExtension) {
                return super.findClass(name);
            } 
	    try {
		return doClassFind(name);
	    } catch (java.security.PrivilegedActionException pae) {
		throw (ClassNotFoundException) pae.getException();
	    }
	}

        private native ClassContainer findContainer(String name)
                       throws ClassNotFoundException;


	/**
	 * allow any classes loaded from classpath to exit the VM.
	 */
	protected PermissionCollection getPermissions(CodeSource codesource)
	{
	    PermissionCollection perms = super.getPermissions(codesource);
	    perms.add(new RuntimePermission("exitVM"));
	    return perms;
	}
    }

    //
    // Cache the bootstrap class path so it does not have to be recreated
    // from the sun.boot.class.path every time.
    //
    private static URLClassPath bootstrapClassPath = null;

    // Returns the URLClassPath that is used for finding system resources.
    public static URLClassPath getBootstrapClassPath() {
	if (bootstrapClassPath == null) {
	    bootstrapClassPath = getBootstrapClassPath0();
	}
	return bootstrapClassPath;
    }
  
    // Compute a URLClassPath representing the boot classpath
    private static URLClassPath getBootstrapClassPath0() {
	String prop = (String)AccessController.doPrivileged(
	    new GetPropertyAction("sun.boot.class.path"));
	URL[] urls;
	if (prop != null) {
	    final String path = prop;
	    urls = (URL[])AccessController.doPrivileged(
		new PrivilegedAction() {
		    public Object run() {
			return pathToURLs(getClassPath(path));
		    }
		}
	    );
	} else {
	    urls = new URL[0];
	}
	return new URLClassPath(urls, factory);
    }

    private static URL[] pathToURLs(File[] path) {
	URL[] urls = new URL[path.length];
	for (int i = 0; i < path.length; i++) {
	    urls[i] = getFileURL(path[i]);
	}
	// DEBUG
	//for (int i = 0; i < urls.length; i++) {
	//  System.out.println("urls[" + i + "] = " + '"' + urls[i] + '"');
	//}
	return urls;
    }

    private static File[] getClassPath(String cp) {
	File[] path;
	if (cp != null) {
	    int count = 0, maxCount = 1;
	    int pos = 0, lastPos = 0;
	    // Count the number of separators first
	    while ((pos = cp.indexOf(File.pathSeparator, lastPos)) != -1) {
		maxCount++;
		lastPos = pos + 1;
	    }
	    path = new File[maxCount];
	    lastPos = pos = 0;
	    // Now scan for each path component
	    while ((pos = cp.indexOf(File.pathSeparator, lastPos)) != -1) {
		if (pos - lastPos > 0) {
		    path[count++] = new File(cp.substring(lastPos, pos));
		} else {
		    // empty path component translates to "."
		    path[count++] = new File(".");
		}
		lastPos = pos + 1;
	    }
	    // Make sure we include the last path component
	    if (lastPos < cp.length()) {
		path[count++] = new File(cp.substring(lastPos));
	    } else {
		path[count++] = new File(".");
	    }
	    // Trim array to correct size
	    if (count != maxCount) {
		File[] tmp = new File[count];
		System.arraycopy(path, 0, tmp, 0, count);
		path = tmp;
	    }
	} else {
	    path = new File[0];
	}
	// DEBUG
	//for (int i = 0; i < path.length; i++) {
	//  System.out.println("path[" + i + "] = " + '"' + path[i] + '"');
	//}
	return path;
    }

    private static URLStreamHandler fileHandler;

    static URL getFileURL(File file) {
	try {
	    file = file.getCanonicalFile();
	} catch (IOException e) {
	}
	String path = file.getAbsolutePath();
        path = ParseUtil.encodePath(path);
	if (File.separatorChar != '/') {
	    path = path.replace(File.separatorChar, '/');
	}
	if (!path.startsWith("/")) {
	    path = "/" + path;
	}
	if (!path.endsWith("/") && file.isDirectory()) {
	    path = path + "/";
	}
	if (fileHandler == null) {
	    fileHandler = factory.createURLStreamHandler("file");
	}
	try {
	    return new URL("file", "", -1, path, fileHandler);
	} catch (MalformedURLException e) {
	    // Should never happen since we specify the protocol...
	    throw new InternalError();
	}
    }

    /*
     * The stream handler factory for loading system protocol handlers.
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
}

class PathPermissions extends PermissionCollection {
    // use serialVersionUID from JDK 1.2.2 for interoperability
    private static final long serialVersionUID = 8133287259134945693L;

    private File path[];
    private Permissions perms;

    URL codeBase;

    PathPermissions(File path[])
    {
	this.path = path;
	this.perms = null;
	this.codeBase = null;
    }

    URL getCodeBase()
    {
	return codeBase;
    }

    public void add(java.security.Permission permission) {
	throw new SecurityException("attempt to add a permission");
    }

    private synchronized void init()
    {
	if (perms != null)
	    return;

	perms = new Permissions();

	// this is needed to be able to create the classloader itself!
	perms.add(new RuntimePermission("createClassLoader"));

	// add permission to read any "java.*" property
	perms.add(new java.util.PropertyPermission("java.*","read"));

	AccessController.doPrivileged(new PrivilegedAction() {
	    public Object run() {
		for (int i=0; i < path.length; i++) {
		    File f = path[i];
		    String path;
		    try {
			path = f.getCanonicalPath();
		    } catch (IOException ioe) {
			path = f.getAbsolutePath();
		    }
		    if (i == 0) {
			codeBase = Launcher.getFileURL(new File(path));
		    }
		    if (f.isDirectory()) {
			if (path.endsWith(File.separator)) {
			    perms.add(new FilePermission(path+"-", "read"));
			} else {
			    perms.add(new FilePermission(path +
						File.separator+"-", "read"));
			}
		    } else {
			int endIndex = path.lastIndexOf(File.separatorChar);
			if (endIndex != -1) {
			    path = path.substring(0, endIndex+1) + "-";
			    perms.add(new FilePermission(path, "read"));
			} else {
			    // ?
			}
		    }
		}
		return null;
	    }
	});
    }

    public boolean implies(java.security.Permission permission) {
	if (perms == null)
	    init();
	return perms.implies(permission);
    }

    public java.util.Enumeration elements() {
	if (perms == null)
	    init();
	return perms.elements();
    }

    public String toString() {
	if (perms == null)
	    init();
	return perms.toString();
    }
}
