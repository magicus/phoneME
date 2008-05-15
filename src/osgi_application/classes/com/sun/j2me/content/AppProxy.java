/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import java.lang.reflect.Method;
import java.util.Hashtable;

import javax.microedition.content.ContentHandlerException;

import sun.misc.MIDPConfig;
import sun.misc.MIDPImplementationClassLoader;

import com.sun.j2me.security.Token;

/**
 * Each AppProxy instance provides access to the AMS information
 * and functions for a running or installed application.
 * This class must be replaced for each platform/profile combination and
 * be integrated with the appropriate Application Management Software.
 * <p>
 * The AppProxy class is *only* available within
 * this package for security purposes.
 * An instance exists for the current application and can
 * be retrieved for any installed application.
 * The following methods provide functions needed by CHAPI:
 * <ul>
 * <li>{@link #getCurrent} - the current application and the main
 *     class within it.
 * <li>{@link #forApp} - the AppProxy for a named  Jar ID and
 *     classname.  For MIDP, they are the suiteID of the storage and
 *     MIDlet within the JAR.
 * <li>{@link #getStorageID} - the storage ID for this AppProxy.
 * <li>{@link #getClassname} - the classname for this AppProxy.
 * <li>{@link #getApplicationID} - the CHAPI defined unique identifier
 *     for the application
 * <li>{@link #getApplicationName} = a user friendly name for this application.
 * <li>{@link #getAuthority} - the authority under which the application
 *     is granted permissions. For MIDP, the subject field of the signing
 *     certificate.
 * <li>{@link #getProperty} - access to the properties in the manifest or
 *     application descriptor.
 * <li>{@link #getVersion} - the version number of this application.
 * <li>{@link #getDefaultID} - the default applicationID if none is provided
 *     in the manifest or application descriptor.
 * <li>{@link #checkRegisterPermission} - method to check if the caller
 *     is allowed to register or unregister handlers.
 * <li>{@link #checkAPIPermission} - method to check if the caller has
 *     permission to use internal APIs. Caller must have an appropriate
 *     security token (depending on the platform)
 * <li>{@link #isRegistered} - used to check if the application is
 *     registered as appropriate for the platform. For MIDP, there
 *     is a MIDlet-n attribute in manifest or JAD.
 * <li>{@link #verifyApplication} - Verify that a named class is the
 *     correct type and access to be launched as an application in the
 *     profile.  For MIDP, the class must extend MIDlet.
 * <li>{@link #launch} - Request this application be launched if it is
 *     not already running.  Return a boolean indicating if the current
 *     application MUST voluntarily exit before the launched app can run.
 * <li>{@link #requestForeground} - ask the "window manager" to give the
 *     foreground to the requested application.
 * <
 * </ul>
 */
public class AppProxy {

    /** The log flag to enable informational messages. */
    static final Logger LOGGER = new Logger();

    /** MIDlet property for the suite version. */
    static final String VERSION_PROP       = "MIDlet-Version";

    /** MIDlet property for the suite vendor. */
    static final String VENDOR_PROP        = "MIDlet-Vendor";
    
    static final int INVALID_SUITE_ID = 0;

    /** The mutex used to avoid corruption between threads. */
    protected static final Object mutex = new Object();

    /** The current AppProxy. */
    private static AppProxy currentApp;
    static RegistryGate registry;
    static AMSGate ams;

    /** The known AppProxy instances. Key is classname. */
    protected Hashtable appmap;

    /** The storageId (suiteId) for this application. */
    protected final int storageId;

    /** The classname of the application. */
    protected String classname;
    
    protected String version, authority;

    /** The application name. */
    private String applicationName;

    /** The ApplicationID, (same a suiteId). */
    private String applicationID;

    /** The application is registered. */
    private boolean isRegistered;

    private final static MIDPImplementationClassLoader midpClassLoader = 
    						MIDPConfig.getMIDPImplementationClassLoader();
    private final static Class classMIDletStateHandler;
    private final static Class classMIDletSuite;
    static {
    	try {
			classMIDletStateHandler = 
					midpClassLoader.loadClass("com.sun.midp.midlet.MIDletStateHandler");
			classMIDletSuite = midpClassLoader.loadClass("com.sun.midp.midlet.MIDletSuite");
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
			throw new RuntimeException(e); 
		}
    }

    static Object getMIDletSuite(){
    	Object midletSuite = null;
		try {
			Method method_getMIDletStateHandler = 
				classMIDletStateHandler.getMethod("getMidletStateHandler", null);
			Method method_getMIDletSuite = 
				classMIDletStateHandler.getMethod("getMIDletSuite", null);
			
			Object midletStateHandler = method_getMIDletStateHandler.invoke(null, null);
			midletSuite = method_getMIDletSuite.invoke(midletStateHandler, null); 
		} catch( Exception x ){
			x.printStackTrace();
			throw new RuntimeException(x); 
		} 
		return midletSuite;
    }

    static int getSuiteID(){
		try {
			Method method_getID = classMIDletSuite.getMethod("getID", null);
			return ((Integer)method_getID.invoke(getMIDletSuite(), null)).intValue();
		} catch( Exception x ){
			if( x instanceof RuntimeException ) throw (RuntimeException)x;
			x.printStackTrace();
			throw new RuntimeException(x); 
		} 
    }

    static boolean midletIsRegistered( String midletClassName ){
		try {
			Method method_isRegistered = classMIDletSuite.getMethod("isRegistered", new Class[]{String.class});
			return ((Boolean)method_isRegistered.invoke(getMIDletSuite(), 
								new Object[]{midletClassName})).booleanValue();
		} catch( Exception x ){
			if( x instanceof RuntimeException ) throw (RuntimeException)x;
			x.printStackTrace();
			throw new RuntimeException(x); 
		} 
    }

    /**
     * Gets the AppProxy for the currently running application.
     * @return the current application.
     */
    static AppProxy getCurrent() {
        synchronized (mutex) {
            if (currentApp == null) {
                currentApp = new AppProxy( getSuiteID(), "Invalid.Class.Name", null );
            }
        }
        return currentApp;
    }

    /**
     * Construct an AppProxy with the specified suiteId, classname.
     * This is just a placeholder to make launch work.
     *
     * @param storageId the suiteId
     * @param classname the classname
     */
    protected AppProxy(int storageId, String classname, Hashtable appmap) {
        this.storageId = storageId;
        this.classname = classname;
        isRegistered = midletIsRegistered(classname);
        
        if( appmap == null ) appmap = new Hashtable();
        this.appmap = appmap;
        this.appmap.put(classname, this);
        if (LOGGER != null) {
            LOGGER.println("AppProxy created: (" + storageId + ", '" + classname + "')");
            LOGGER.println("\tRegistry(" + registry + ")");
            LOGGER.println("\tAMS(" + ams + ")");
        }
    }

    /**
     * Gets the AppProxy for an application class in the current bundle.
     * @param classname the name of the application class
     * @return the AppProxy for classname; <code>null</code> if not
     * a valid application (MIDlet)
     * @exception ClassNotFoundException if the <code>classname</code>
     *   is not present
     * @exception IllegalArgumentException if classname is
     *  not a valid application
     */
    AppProxy forClass(String classname) throws ClassNotFoundException {
        AppProxy curr = null;
        synchronized (mutex) {
            // Check if class already has a AppProxy
            curr = (AppProxy)appmap.get(classname);
            if (curr == null) {
                // Create a new instance
                // throws ClassNotFoundException and IllegalArgumentException
                curr = new AppProxy( storageId, classname, appmap );
            }
        }
        return curr;
    }

    /**
     * Gets the AppProxy for an storageID and classname.
     * @param storageId the storageId (suiteId)
     * @param classname the name of the application class
     * @return the AppProxy for suiteId, classname;
     *   <code>null</code> if not a valid application (MIDlet)
     * @exception ClassNotFoundException if the <code>classname</code>
     * is not present
     * @exception IllegalArgumentException if classname is not
     *   a valid application
     */
    AppProxy forApp(int storageId, String classname)
        throws ClassNotFoundException
    {
        // Check in the current suite
        if (storageId == this.storageId) 
            return forClass(classname);

        // Create a new instance
        return new AppProxy(storageId, classname, null);
    }

    /**
     * Gets the storage ID of this application.
     * The ID uniquely identifies the package/application bundle.
     * @return the application ID.
     */
    int getStorageId() {
        return storageId;
    }

    /**
     * Gets the classname of this application.
     * @return the classname
     */
    String getClassname() {
        return classname;
    }

    /**
     * Gets the user friendly application name.
     * @return the user friendly application name
     */
    String getApplicationName() {
        return applicationName;
    }

    /**
     * Gets the CHAPI application ID for this application.
     * @return the CHAPI application ID.
     */
    String getApplicationID() {
        return applicationID;
    }

    /**
     * Gets the version string for the application.
     * @return the version
     */
    String getVersion() {
        return version;
    }

    /**
     * Gets the Trusted authority that authenticated this application.
     * <p>
     * For MIDP, this is the CA of the signer.
     * If exception is thrown during getting authorization
     * this methods ignores it and returns null.
     * @return the authority.
     */
    String getAuthority() {
        return authority;
    }

    /**
     * Gets true if the application is a registered application.
     * <p>
     * for MIDP, this means there was a MIDlet-n attribute.
     * @return true if this application is registered
     */
    boolean isRegistered() {
        return isRegistered;
    }

    /**
     * Gets a property from the manifest or application descriptor.
     * @param key the name of the property to retrieve
     * @return the value of the property or <code>null</code>
     */
    String getProperty(final String key) {
        return null;
    }

    /**
     * Check the permission to register or unregister.
     * @param reason the reason for the permission check
     * @exception SecurityException if not allowed
     */
    final void checkRegisterPermission(final String reason) {}

    /**
     * Check if the internal API use is allowed.
     * @param securityToken a generic security token
     * @exception SecurityException thrown if internal API use not allowed
     */
    final static void checkAPIPermission(Token securityToken) {}

    /**
     * Request the transition of the foreground to this application
     * from the invoking application.
     * @param invokingSuiteId the invoking suiteId
     * @param invokingClassname the invoking classname
     * @param targetSuiteId the target suiteId
     * @param targetClassname the target classname
     */
    static void requestForeground(int invokingSuiteId,
                                  String invokingClassname,
                                  int targetSuiteId,
                                  String targetClassname)
    {
    }

    /**
     * The stronger variant for request the transition of
     * the foreground to this application.
     * @param targetSuiteId the target suiteId
     * @param targetClassname the target classname
     */
    static void requestForeground(int targetSuiteId,
                                  String targetClassname)
    {
    }
    
    static native void midletIsAdded( int suiteId, String className );
    static native boolean isMidletRunning( int suiteId, String className );
    static native void midletIsRemoved( int suiteId, String className );

    /**
     * Launch this application.
     * Don't launch another application unless
     * the execute allows this application to continue after
     * the launch.
     * <p>
     * In SVM, (sequential applications) only the first
     * execute matters; later ones should not override the
     * first.  All pending Invocations are queued in InvocationStore
     * so they will not be lost.  When MIDlets exit, another
     * application will be selected from those pending.
     *
     * @param displayName name to show to the user of what to launch
     * @return <code>true</code> if the application is started.
     */
    boolean launch(String displayName) {
        if( isMidletRunning(storageId, classname) )
            return true;
        return false;
    }


    /**
     * Verify that the classname is a valid application.
     * It must extend MIDlet.
     * @param classname the application class
     *
     * @exception ClassNotFoundException is thrown if the class cannot be found
     * @exception IllegalArgumentException if the classname is null or empty
     *  or does not implement the lifecycle of a MIDlet.
     */
    protected void verifyApplication(String classname)
        throws ClassNotFoundException
    {
        /* check the classname for null and get the class */
        Class appClass = Class.forName(classname);
        Class midletClass = Class.forName("javax.microedition.midlet.MIDlet");
        if ((!midletClass.isAssignableFrom(appClass)) ||
                appClass == midletClass) {
            throw new IllegalArgumentException("Class '" + classname + 
                                        "' is not a MIDlet");
        }
    }


    /**
     * Gets the content handler ID for the current application.
     * The ID uniquely identifies the application which contains the
     * content handler.
     * The application ID is assigned when the application is installed.
     * <p>
     * All installed applications have vendor and name;
     *
     * @return the ID; MUST NOT be <code>null</code>
     */
    String getDefaultID() {
        StringBuffer sb = new StringBuffer(80);
        String s = getProperty(VENDOR_PROP);
        sb.append((s != null) ? s : "internal");
        sb.append('-');
        s = getProperty(".SUITE_NAME_PROP");
        sb.append((s != null) ? s : "system");
        sb.append('-');
        sb.append(classname);
        return sb.toString().replace(' ', '_');
    }

    /**
     * Get the MIDletInfo for the named MIDlet.
     * @param suite the MIDlet suite to look in for the midlet
     * @param classname the class name to look for
     * @return an array of Strings, name, icon, ID
     *  null if there is no matching MIDlet-n.
     *
    private static String[] getMIDletInfo(MIDletSuite suite, String classname)
    {
        for (int i = 1; ; i++) {
            String midletn = "MIDlet-".concat(Integer.toString(i));
            String attr = suite.getProperty(midletn);
            if (attr == null) {
                break; // break out of loop, not found
            }

            Vector args = Util.getCommaSeparatedValues(attr);
            if (args.size() < 3) {
                // Not enough args to be legit
                continue;
            }

            if (!classname.equals(args.elementAt(2))) {
                continue;
            }
            String[] values = new String[args.size()];
            args.copyInto(values);

            String ID = suite.getProperty(midletn.concat("-ID"));
            values[2] = ID;

            return values;
        }
        return null;
    }*/

    static boolean launchNativeHandler(String id) throws ContentHandlerException {
        return false;
    }
    
    static boolean platformFinish(int tid) {
        return false;
    }

    public static void setRegistry( RegistryGate registry ) {
    	AppProxy.registry = registry;
        System.out.println("AppProxy.setRegistry(" + registry + ")");
    }

    public static void setAMS( AMSGate ams ) {
    	AppProxy.ams = ams;
        System.out.println("AppProxy.setAMS(" + ams + ")");
    }

    /**
     * Create a printable representation of this AppProxy.
     * @return a printable string
     */
    public String toString() {
        if (LOGGER != null) {
            return"class: " + classname +
                ", suite: " + storageId +
                ", registered: " + isRegistered +
                ", name: " + applicationName +
                ", ID: " + applicationID;
        } else {
            return super.toString();
        }
    }
}
