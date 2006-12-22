/*
 *
 *
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

package com.sun.midp.io.j2me.push;

import java.io.InterruptedIOException;
import java.io.IOException;

import java.lang.ClassNotFoundException;

import java.util.Date;
import java.util.Enumeration;
import java.util.Timer;
import java.util.TimerTask;

import javax.microedition.io.ConnectionNotFoundException;

import com.sun.midp.io.Util;
import com.sun.midp.io.HttpUrl;

import com.sun.midp.main.*;

import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import com.sun.midp.log.Logging;

/**
 * Inbound push connection watcher.
 */
public class PushRegistryImpl
    implements Runnable, MIDletProxyListListener {

    /**
     * Push option to only launch this suite when not other applications
     * are running.
     */
    public static final int PUSH_OPT_WHEN_ONLY_APP = 1;

    /** This class has a different security domain than the MIDlet suite */
    private static SecurityToken classSecurityToken;

    /**
     * Flag to control when push launching is permitted.
     * This flag is set to false by the AMS when installing or removing
     * MIDlets, when an interruption could compromise the integrity of
     * the operation.
     */
    private static boolean pushEnabled = true;

    /**
     * This flag is set to true by the AMS when running in MVM singal MIDlet
     * mode. In this mode the current MIDlet that is not the application
     * manager should be destroyed before the next MIDlet is started.
     */
    private static boolean mvmSingleMidletMode;

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /**
     * Start listening for push notifications. Will throw a security
     * exception if called by any thing other than the MIDletSuiteLoader.
     */
    public static void startListening() {
        (new Thread(new PushRegistryImpl())).start();
    }

    /**
     * Keeps an object of this class from being created out of
     * the startListening method.
     * Will throw a security exception if called by any thing other
     * than the MIDletSuiteLoader.
     */
    private PushRegistryImpl() {
        /*
         * Will throw a security exception if called by any thing other
         * than the MIDletSuiteLoader.
         */
        midletProxyList =
            MIDletProxyList.getMIDletProxyList(classSecurityToken);
        midletProxyList.addListener(this);
    }

    /**
     * Run the polling loop to check for inbound connections.
     */
    public void run() {
        int fd = -1;
        int ret = 0;
        while (true) {
            try {
                fd = poll0(System.currentTimeMillis());
                if (fd != -1) {
                    if (pushEnabled) {
                        byte[] registryEntry = new byte[512];
                        if ((ret = getMIDlet0(fd, registryEntry, 512)) == 0) {
                            String name = Util.toJavaString(registryEntry);
                            launchEntry(name);
                        }
                    } else {
                        checkInByHandle0(fd);
                    }
                }
            } catch (Exception e) {
                if (Logging.TRACE_ENABLED) {
                    Logging.trace(e, null);
                }
            }
        }
    }

    /**
     * Parse the registration entry and launch the associated
     * <code>MIDlet</code>.
     * @param name registration string for connection and
     * <code>MIDlet</code> to be launched
     */
    protected void launchEntry(String name) {
        String conn;
        String midlet;
        String filter;
        String strSuiteId;
        int id;
        MIDletSuite next = null;

        /*
         * Parse the comma separated values  -
         *  " connection, midlet,  filter, id"
         *  "  midlet,  wakeup, midlet suite ID"
         */
        int comma1 = name.indexOf(',', 0);
        int comma2 = name.indexOf(',', comma1 + 1);
        int comma3 = name.indexOf(',', comma2 + 1);

        if (comma3 == -1) {
            /* Alarm was triggered */
            conn = null;
            midlet = name.substring(0, comma1).trim();
            strSuiteId = name.substring(comma2+1).trim();
        } else {
            conn = name.substring(0, comma1).trim();
            midlet = name.substring(comma1+1, comma2).trim();
            filter = name.substring(comma2+1, comma3).trim();
            strSuiteId = name.substring(comma3+1).trim();
        }

        try {
            id = Integer.parseInt(strSuiteId);
        } catch (NumberFormatException nfe) {
            id = MIDletSuite.UNUSED_SUITE_ID;
        }

        try {
            MIDletSuiteStorage storage;

            /*
             * Check to see if the MIDlet is already started.
             */
            if (midletProxyList.isMidletInList(id, midlet)) {
                if (conn != null) {
                    checkInConnectionInternal(classSecurityToken, conn);
                }

                return;
            }

            storage =
                MIDletSuiteStorage.getMIDletSuiteStorage(classSecurityToken);
            next = storage.getMIDletSuite(id, false);
            if (next == null) {
                if (conn != null) {
                    checkInConnectionInternal(classSecurityToken, conn);
                }

                return;
            }

            if ((next.getPushOptions() & PUSH_OPT_WHEN_ONLY_APP) != 0 &&
                    !onlyAppManagerRunning()) {
                if (conn != null) {
                    checkInConnectionInternal(classSecurityToken, conn);
                }

                return;
            }

            if (!next.permissionToInterrupt(conn)) {
                // user does not want the interruption
                if (conn != null) {
                    checkInConnectionInternal(classSecurityToken, conn);
                }

                return;
            }

            if (MIDletSuiteUtils.execute(classSecurityToken, id, midlet,
                                          null)) {
                /* We are in SVM mode, destroy all running MIDlets. */
                MIDletStateHandler.getMidletStateHandler().destroySuite();
            } else if (mvmSingleMidletMode) {
                destroyAppMidlets();
            }
        } catch (Throwable e) {
            // Could not launch requested push entry
            if (conn != null) {
                checkInConnectionInternal(classSecurityToken, conn);
            }

            if (Logging.TRACE_ENABLED) {
                Logging.trace(e, null);
            }
        } finally {
            if (next != null) {
                next.close();
            }
        }
    }

    /**
     * Check to see if only the application manager MIDlet is running.
     *
     * @return true if only the application manager is running
     */
    private boolean onlyAppManagerRunning() {
        Enumeration midlets = midletProxyList.getMIDlets();

        while (midlets.hasMoreElements()) {
            MIDletProxy midlet = (MIDletProxy)midlets.nextElement();

            if (midlet.getSuiteId() != MIDletSuite.INTERNAL_SUITE_ID ||
                    midlet.getClassName().indexOf("Manager") == -1) {
                return false;
            }
        }

        return true;
    }

    /**
     * Destroy every MIDlet except the application manager midlet.
     * This should only be used in MVM Signal MIDlet Mode.
     */
    private void destroyAppMidlets() {
        Enumeration midlets = midletProxyList.getMIDlets();

        while (midlets.hasMoreElements()) {
            MIDletProxy midlet = (MIDletProxy)midlets.nextElement();

            if (midlet.getSuiteId() == MIDletSuite.INTERNAL_SUITE_ID &&
                    midlet.getClassName().indexOf("Manager") != -1) {
                continue;
            }

            midlet.destroyMidlet();
        }
    }

    /**
     * Called when a MIDlet is added to the list, not used by this class.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {};

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {};

    /**
     * Called when a MIDlet is removed from the list, the connections
     * in "launch pending" state for this MIDlet will be checked in.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        byte[] asciiClassName = Util.toCString(midlet.getClassName());

        checkInByMidlet0(midlet.getSuiteId(), asciiClassName);
    }

    /**
     * Called when error occurred while starting a MIDlet object. The
     * connections in "launch pending" state for this MIDlet will be checked
     * in.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param error start error code
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int error) {
        byte[] asciiClassName = Util.toCString(className);

        checkInByMidlet0(suiteId, asciiClassName);
    }

    /**
     * Register a dynamic connection with the
     * application management software. Once registered,
     * the dynamic connection acts just like a
     * connection preallocated from the descriptor file.
     * The internal implementation includes the storage name
     * that uniquely identifies the <code>MIDlet</code>.
     *
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *               and <em>port number</em>
     *               (optional parameters may be included
     *               separated with semi-colons (;))
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *               when new external data is available
     * @param filter a connection URL string indicating which senders
     *               are allowed to cause the MIDlet to be launched
     *
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception ConnectionNotFoundException if the runtime system does not
     *              support push delivery for the requested
     *              connection protocol
     * @exception IOException if the connection is already
     *              registered or if there are insufficient resources
     *              to handle the registration request
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     * @exception SecurityException if the <code>MIDlet</code> does not
     *              have permission to register a connection
     * @exception IllegalStateException if the MIDlet suite context is
     *              <code>null</code>.
     * @see #unregisterConnection
     */
    public static void registerConnection(String connection, String midlet,
                                          String filter)
        throws ClassNotFoundException, IOException {

        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        /* This method should only be called by running MIDlets. */
        if (midletSuite == null) {
            throw new IllegalStateException("Not in a MIDlet context");
        }

        /* Verify the MIDlet is in the current classpath. */
        if (midlet == null || midlet.length() == 0) {
            throw new ClassNotFoundException("MIDlet missing");
        }

        Class cl = Class.forName(midlet);
        Class m = Class.forName("javax.microedition.midlet.MIDlet");

        if (!m.isAssignableFrom(cl)) {
            throw new ClassNotFoundException("Not a MIDlet");
        }

        /* Perform the rest of the checks in the internal registry. */
        registerConnectionInternal(classSecurityToken, midletSuite,
                                   connection, midlet, filter, false);
    }

    /**
     * Check the registration arguments.
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *               and <em>port number</em>
     *               (optional parameters may be included
     *               separated with semi-colons (;))
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *               when new external data is available
     * @param filter a connection URL string indicating which senders
     *               are allowed to cause the MIDlet to be launched
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     */
    static void checkRegistration(String connection, String midlet,
                                  String filter)
                                  throws ClassNotFoundException,
                                      ConnectionNotFoundException {

        /* Verify the MIDlet is in the current classpath. */
        if (midlet == null || midlet.length() == 0) {
            throw new ClassNotFoundException("MIDlet missing");
        }

        /* Verify that the filter requested is valid. */
        if (filter == null || filter.length() == 0) {
            throw new IllegalArgumentException("Filter missing");
        }

        ProtocolPush.getInstance(connection).checkRegistration(connection, midlet, filter);

    }

    /**
     * Register a dynamic connection with the
     * application management software. Once registered,
     * the dynamic connection acts just like a
     * connection preallocated from the descriptor file.
     * The internal implementation includes the storage name
     * that uniquely identifies the <code>MIDlet</code>.
     * This method bypasses the class loader specific checks
     * needed by the <code>Installer</code>.
     *
     * @param token security token of the calling class, can be null
     * @param midletSuite MIDlet suite for the suite registering,
     *                   the suite only has to implement isRegistered,
     *                   checkForPermission, and getID.
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *               and <em>port number</em>
     *               (optional parameters may be included
     *               separated with semi-colons (;))
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *               when new external data is available
     * @param filter a connection URL string indicating which senders
     *               are allowed to cause the MIDlet to be launched
     * @param bypassChecks if true, bypass the permission checks,
     *         used by the installer when redo old connections during an
     *         aborted update
     *
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception ConnectionNotFoundException if the runtime system does not
     *              support push delivery for the requested
     *              connection protocol
     * @exception IOException if the connection is already
     *              registered or if there are insufficient resources
     *              to handle the registration request
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     * @exception SecurityException if the <code>MIDlet</code> does not
     *              have permission to register a connection
     *
     * @see #unregisterConnection
     */
    public static void registerConnectionInternal(SecurityToken token,
                                                  MIDletSuite midletSuite,
                                                  String connection,
                                                  String midlet,
                                                  String filter,
                                                  boolean bypassChecks)
        throws ClassNotFoundException, IOException {

        HttpUrl url;
        int id;

        if (token == null) {
            MIDletSuite current =
                MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

            if (current != null) {
                current.checkIfPermissionAllowed(Permissions.AMS);
            }
        } else {
            token.checkIfPermissionAllowed(Permissions.AMS);
        }

        if (!bypassChecks) {
            try {
                midletSuite.checkForPermission(Permissions.PUSH, null);
                /* Check the registration arguments. */
                checkRegistration(connection, midlet, filter);

                /* Check if an appropriate MIDlet-<n> record exists. */
                if (!midletSuite.isRegistered(midlet)) {
                    throw new
                        ClassNotFoundException("No MIDlet-<n> registration");
                }

                ProtocolPush.getInstance(connection).registerConnection
                    (midletSuite, connection, midlet, filter);

            } catch (InterruptedException ie) {
                throw new InterruptedIOException(
                    "Interrupted while trying to ask the user permission");
            }
        }

        id = midletSuite.getID();

        byte[] asciiRegistration = Util.toCString(connection
                  + "," + midlet
                  + "," + filter
                  + "," + String.valueOf(id));

        if (add0(asciiRegistration) == -1) {
            // in case of Bluetooth URL, unregistration within Bluetooth
            // PushRegistry was already performed by add0()
            throw new IOException("Connection already registered");
        }

    }

    /**
     * Remove a dynamic connection registration.
     *
     * @param connection generic connection <em>protocol</em>,
     *             <em>host</em> and <em>port number</em>
     * @exception SecurityException if the connection was
     *            not registered by the current <code>MIDlet</code>
     *            suite
     * @return <code>true</code> if the unregistration was successful,
     *         <code>false</code> the  connection was not registered.
     * @see #registerConnection
     */
    public static boolean unregisterConnection(String connection) {

        /* Verify the connection string before using it. */
        if (connection == null || connection.length() == 0) {
            return false;
        }

        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite current = midletStateHandler.getMIDletSuite();

        byte[] asciiRegistration = Util.toCString(connection);
        byte[] asciiStorage = Util.toCString(String.valueOf(current.getID()));
        int ret =  del0(asciiRegistration, asciiStorage);
        if (ret == -2) {
            throw new SecurityException("wrong suite");
        }
        return ret != -1;
    }
    /**
     * Check in a push connection into AMS so the owning MIDlet can get
     * launched next time data is pushed. This method is used when a MIDlet
     * will not be able to get the connection and close (check in) the
     * connection for some reason. (normally because the user denied a
     * permission)
     * <p>
     * For datagram connections this function will discard the cached message.
     * <p>
     * For server socket connections this function will close the
     * accepted connection.
     *
     * @param token security token of the calling class
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *              and <em>port number</em>
     *              (optional parameters may be included
     *              separated with semi-colons (;))
     * @exception IllegalArgumentException if the connection string is not
     *              valid
     * @exception SecurityException if the <code>MIDlet</code> does not
     *              have permission to clear a connection
     * @return <code>true</code> if the check in was successful,
     *         <code>false</code> the connection was not registered.
     * @see #unregisterConnection
     */
    public static boolean checkInConnectionInternal(SecurityToken token,
                                                    String connection) {
        int ret;

        token.checkIfPermissionAllowed(Permissions.AMS);
        /* Verify that the connection requested is valid. */
        if (connection == null || connection.length() == 0) {
            throw new IllegalArgumentException("Connection missing");
        }

        byte[] asciiRegistration = Util.toCString(connection);

        return checkInByName0(asciiRegistration) != -1;
    }

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public static void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }

    /**
     * Return a list of registered connections for the current
     * <code>MIDlet</code> suite.
     *
     * @param available if <code>true</code>, only return the list of
     *      connections with input available
     * @return array of connection strings, where each connection is
     *       represented by the generic connection <em>protocol</em>,
     *       <em>host</em> and <em>port number</em> identification
     */
    public static String listConnections(boolean available) {
        MIDletSuite current =
            MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

        if (current == null) {
            return null;
        }

        return listConnections(classSecurityToken, current.getID(),
                               available);
    }

    /**
     * Return a list of registered connections for given
     * <code>MIDlet</code> suite.
     *
     * @param id identifies the specific <code>MIDlet</code>
     *               suite to be launched
     * @param available if <code>true</code>, only return the list of
     *      connections with input available
     *
     * @return array of connection strings, where each connection is
     *       represented by the generic connection <em>protocol</em>,
     *       <em>host</em> and <em>port number</em> identification
     */
    public static String listConnections(int id, boolean available) {

        return listConnections(null, id, available);
    }

    /**
     * Return a list of registered connections for given
     * <code>MIDlet</code> suite. AMS permission is required.
     *
     * @param token security token of the calling class, or <code>null</code>
     *        to check the suite
     * @param id identifies the specific <code>MIDlet</code>
     *               suite to be launched
     * @param available if <code>true</code>, only return the list of
     *      connections with input available
     *
     * @return array of connection strings, where each connection is
     *       represented by the generic connection <em>protocol</em>,
     *       <em>host</em> and <em>port number</em> identification
     */
    public static String listConnections(SecurityToken token,
            int id, boolean available) {
        byte[] nativeID;
        String connections = null;
        byte[] connlist;
        if (token == null) {
            MIDletSuite current =
                MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

            if (current != null) {
                current.checkIfPermissionAllowed(Permissions.AMS);
            }
        } else {
            token.checkIfPermissionAllowed(Permissions.AMS);
        }

        nativeID = Util.toCString(new Integer(id).toString());
        connlist = new byte[512];

        if (list0(nativeID, available, connlist, 512) == 0) {
            connections = Util.toJavaString(connlist);
        }

        return connections;
    }

    /**
     * Unregister all the connections for a <code>MIDlet</code> suite.
     *
     * @param id identifies the specific <code>MIDlet</code>
     *               suite
     */
    public static void unregisterConnections(int id) {
        MIDletSuite current =
            MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

        if (current != null) {
            current.checkIfPermissionAllowed(Permissions.AMS);
        }

        delAllForSuite0(id);
    }


    /**
     * Retrieve the registered <code>MIDlet</code> for a requested connection.
     *
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *              and <em>port number</em>
     *              (optional parameters may be included
     *              separated with semi-colons (;))
     * @return  class name of the <code>MIDlet</code> to be launched,
     *              when new external data is available, or
     *              <code>null</code> if the connection was not
     *              registered
     * @see #registerConnection
     */
    public static String getMIDlet(String connection) {

        /* Verify that the connection requested is valid. */
        if (connection == null || connection.length() == 0) {
            return null;
        }

        String midlet = null;
        byte[] asciiConn = Util.toCString(connection);
        byte[] registryEntry = new byte[512];

        if (getEntry0(asciiConn, registryEntry, 512) == 0) {
            String name = Util.toJavaString(registryEntry);
            try {
                int comma1 = name.indexOf(',', 0);
                int comma2 = name.indexOf(',', comma1 + 1);

                midlet = name.substring(comma1+1, comma2).trim();
            } catch (Exception e) {
                if (Logging.TRACE_ENABLED) {
                    Logging.trace(e, null);
                }
            }
        }
        return  midlet;
    }

    /**
     * Retrieve the registered filter for a requested connection.
     *
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *              and <em>port number</em>
     *              (optional parameters may be included
     *              separated with semi-colons (;))
     * @return a filter string indicating which senders
     *              are allowed to cause the MIDlet to be launched or
     *              <code>null</code> if the connection was not
     *              registered
     * @see #registerConnection
     */
    public static String getFilter(String connection) {

        /* Verify that the connection requested is valid. */
        if (connection == null || connection.length() == 0) {
            return null;
        }

        String filter = null;
        byte[] asciiConn = Util.toCString(connection);
        byte[] registryEntry = new byte[512];

        if (getEntry0(asciiConn, registryEntry, 512) == 0) {
            String name = Util.toJavaString(registryEntry);
            try {
                int comma1 = name.indexOf(',', 0);
                int comma2 = name.indexOf(',', comma1 + 1);
                int comma3 = name.indexOf(',', comma2 + 1);

                filter = name.substring(comma2+1, comma3).trim();
            } catch (Exception e) {
                if (Logging.TRACE_ENABLED) {
                    Logging.trace(e, null);
                }
            }
        }
        return  filter;
    }

    /**
     * Register a time to launch the specified application. The
     * <code>PushRegistry</code> supports one outstanding wake up
     * time per <code>MIDlet</code> in the current suite. An application
     * is expected to use a <code>TimerTask</code> for notification
     * of time based events while the application is running.
     * <P>If a wakeup time is already registered, the previous value will
     * be returned, otherwise a zero is returned the first time the
     * alarm is registered. </P>
     *
     * @param midlet  class name of the <code>MIDlet</code> within the
     *                current running <code>MIDlet</code> suite
     *                to be launched,
     *                when the alarm time has been reached
     * @param time time at which the <code>MIDlet</code> is to be executed
     *        in the format returned by <code>Date.getTime()</code>
     * @return the time at which the most recent execution of this
     *        <code>MIDlet</code> was scheduled to occur,
     *        in the format returned by <code>Date.getTime()</code>
     * @exception ConnectionNotFoundException if the runtime system does not
     *              support alarm based application launch
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *              name can not be found in the current
     *              <code>MIDlet</code> suite
     * @see Date#getTime()
     * @see Timer
     * @see TimerTask
     */
    public static long registerAlarm(String midlet, long time)
        throws ClassNotFoundException, ConnectionNotFoundException {

        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        /* There is no suite running when installing from the command line. */
        if (midletSuite != null) {
            try {
                midletSuite.checkForPermission(Permissions.PUSH, null);
            } catch (InterruptedException ie) {
                throw new RuntimeException(
                    "Interrupted while trying to ask the user permission");
            }
        }

        /* Verify the MIDlet is in the current classpath. */
        if (midlet == null || midlet.length() == 0) {
            throw new ClassNotFoundException("MIDlet missing");
        }

        /* Check if an appropriate MIDlet-<n> record exists. */
        if (!midletSuite.isRegistered(midlet)) {
            throw new ClassNotFoundException("No MIDlet-<n> registration");
        }

        Class c = Class.forName(midlet);
        Class m = Class.forName("javax.microedition.midlet.MIDlet");

        if (!m.isAssignableFrom(c)) {
            throw new ClassNotFoundException("Not a MIDlet");
        }

        /*
         * Add the alarm for the specified MIDlet int the current
         * MIDlet suite.
         */
        MIDletSuite current =
              MIDletStateHandler.getMidletStateHandler().getMIDletSuite();
        if (current != null) {
            byte[] asciiName = Util.toCString(midlet + ","
                      + time + ","
                      + current.getID());
            return addAlarm0(asciiName, time);
        }

        return 0;
    }

    /**
     * Sets the flag which enables push launches to take place.
     *
     * @param token security token of the calling class
     * @param enable set to <code>true</code> to enable launching
     *  of MIDlets based on alarms and connection notification
     *  events, otherwise set to <code>false</code> to disable
     *  launches
     */
    public static void enablePushLaunch(SecurityToken token,
          boolean enable) {
        token.checkIfPermissionAllowed(Permissions.AMS);

        pushEnabled = enable;
    }

    /**
     * Sets the flag which enables push launches to take place.
     *
     * @param enable set to <code>true</code> to enable launching
     *  of MIDlets based on alarms and connection notification
     *  events, otherwise set to <code>false</code> to disable
     *  launches
     */
    public static void enablePushLaunch(boolean enable) {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        /* There is no suite running when installing from the command line. */
        if (midletSuite != null) {
            midletSuite.checkIfPermissionAllowed(Permissions.AMS);
        }

        pushEnabled = enable;
    }

    /**
     * Sets the flag which indicates that the AMS is operating in MVM
     * single MIDlet mode.
     */
    public static void setMvmSingleMidletMode() {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();

        /* There is no suite running when installing from the command line. */
        if (midletSuite != null) {
            midletSuite.checkIfPermissionAllowed(Permissions.AMS);
        }

        mvmSingleMidletMode = true;
    }

    /**
     * Native connection registry add connection function.
     * @param connection string to register
     * @return 0 if successful, -1 if failed
     */
    private static native int add0(byte[] connection);

    /**
     * Native function to test registered inbound connections
     * for new connection notification.
     * @param time current time to use for alarm checks
     * @return handle for the connection with inbound connection
     *         pending.
     */
    private native int poll0(long time);

    /**
     * Native connection registry lookup for MIDlet name from file
     * descriptor.
     * @param handle file descriptor of registered active connection
     * @param regentry registered entry
     * @param entrysz maximum string that will be accepted
     * @return 0 if successful, -1 if failed
     */
    private static native int getMIDlet0(int handle, byte[] regentry,
           int entrysz);

    /**
     * Native connection registry lookup registry entry from a
     * specific connection.
     * @param connection registered connection string
     * @param regentry registered entry
     * @param entrysz maximum string that will be accepted
     * @return 0 if successful, -1 if failed
     */
    private static native int getEntry0(byte[]connection, byte[] regentry,
           int entrysz);

    /**
     * Native connection registry add alarm function.
     * @param midlet string to register
     * @param time
     * @return 0 if unregistered, otherwise the time of the previous
     *         registered alarm
     */
    private static native long addAlarm0(byte[] midlet, long time);

    /**
     * Native connection registry del connection function.
     * @param connection string to register
     * @param storage current suite storage name
     * @return 0 if successful, -1 if failed
     */
    private static native int del0(byte[] connection, byte[] storage);

    /**
     * Native connection registry check in connection function.
     * @param connection string to register
     * @return 0 if successful, -1 if failed
     */
    private static native int checkInByName0(byte[] connection);

    /**
     * Native connection registry check in connection function.
     * @param handle native handle of the connection
     */
    private static native void checkInByHandle0(int handle);

    /**
     * Native connection registry method to check in connections that are in
     * launch pending state for a specific MIDlet.
     *
     * @param suiteId Suite ID of the MIDlet
     *        array
     * @param className Class name of the MIDlet as zero terminated ASCII
     *        byte array
     */
    private static native void checkInByMidlet0(int suiteId,
                                                byte[] className);

    /**
     * Native connection registry list connection function.
     * @param midlet string to register
     * @param available if <code>true</code>, only return the list of
     *      connections with input available
     * @param connectionlist comma separated string of connections
     * @param listsz maximum string that will be accepted in connectionlist
     * @return 0 if successful, -1 if failed
     */
    private static native int list0(byte[] midlet, boolean available,
            byte[] connectionlist, int listsz);

    /**
     * Native connection registry delete a suite's connections function.
     * @param id suite's ID
     */
    private static native void delAllForSuite0(int id);
}
