/*
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

package com.sun.midp.jump.installer;

import com.sun.midp.installer.Verifier;
import com.sun.midp.installer.InstallListener;
import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.installer.JadProperties;
import com.sun.midp.installer.ManifestProperties;
import com.sun.midp.installer.OtaNotifier;
import com.sun.midp.installer.InstallState;
import com.sun.midp.installer.VerifierImpl;

import java.util.Vector;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayInputStream;

import javax.microedition.io.ConnectionNotFoundException;

import com.sun.j2me.security.AccessController;

import com.sun.midp.security.SecurityHandler;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import com.sun.midp.main.MIDletSuiteVerifier;
import com.sun.midp.main.MIDletAppImageGenerator;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteInfo;
import com.sun.midp.midletsuite.InstallInfo;
import com.sun.midp.midletsuite.SuiteSettings;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;

import com.sun.midp.jarutil.JarReader;

import com.sun.midp.io.HttpUrl;
import com.sun.midp.io.Util;

import com.sun.midp.io.j2me.push.PushRegistryInternal;
import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;

import com.sun.midp.rms.RecordStoreFactory;

// IMPL_NOTE: sync with cldc installer for CHManager dependency
// import com.sun.midp.content.CHManager;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.configurator.Constants;

/**
 * An JUMPInstaller manages MIDlet suites and libraries
 * present in a Java application environment.  An MIDlet suite
 * distributed as a descriptor and JAR pair.
 * The descriptor identifies the configuration and contains security
 * information and the manifest of the JAR describes the contents.
 * The implementation of an JUMPInstaller is
 * specific to the platform and provides access to
 * procedures that make an MIDlet suite visible to users.
 * <P>
 * Each installed package is uniquely identified by a storage name
 * constructed from the combination
 * of the values of the <code>MIDlet-Name</code> and
 * <code>MIDlet-Vendor</code> attributes.
 * The syntax and content of the strings used to identify
 * installed packages are implementation dependent.
 * Only packages installed or upgraded using this API appear
 * in the list of known packages.
 *
 */

public abstract class JUMPInstaller {

    /** Status code to signal connection to the JAD server was successful. */
    public static final int DOWNLOADING_JAD = 1;

    /** Status code to signal that another 1K of the JAD has been download. */
    public static final int DOWNLOADED_1K_OF_JAD = 2;

    /** Status code to signal connection to the JAR server was successful. */
    public static final int DOWNLOADING_JAR = 3;

    /** Status code to signal that another 1K of the JAR has been download. */
    public static final int DOWNLOADED_1K_OF_JAR = 4;

    /**
     * Status code to signal that download is done and the suite is being
     * verified.
     */
    public static final int VERIFYING_SUITE = 5;

    /**
     * Status code to signal that application image is being generating.
     */
    public static final int GENERATING_APP_IMAGE = 6;

    /**
     * Status code to signal that suite classes are being verified.
     */
    public static final int VERIFYING_SUITE_CLASSES = 7;

    /**
     * Status code for local writing of the verified MIDlet suite.
     * Stopping the install at this point has no effect, so there user
     * should not be given a chance to stop the install.
     */
    public static final int STORING_SUITE = 8;

    /** Status code for corrupted suite */
    public static final int CORRUPTED_SUITE = 9;

    /** System property containing the supported microedition profiles */
    protected static final String MICROEDITION_PROFILES =
        "microedition.profiles";

    /** System property containing the microedition configuration */
    protected static final String MICROEDITION_CONFIG =
        "microedition.configuration";

    /** System property containing the microedition locale */
    protected static final String MICROEDITION_LOCALE = "microedition.locale";

    /** Media-Type for valid application descriptor files. */
    public static final String JAD_MT = "text/vnd.sun.j2me.app-descriptor";

    /** Media-Type for valid Jar file. */
    public static final String JAR_MT_1 = "application/java";

    /** Media-Type for valid Jar file. */
    public static final String JAR_MT_2 = "application/java-archive";

    /**
     * Filename to save the JAR of the suite temporarily. This is used
     * to avoid overwriting an existing JAR prior to verification.
     */
    protected static final String TMP_FILENAME = "installer.tmp";

    /** Midlet suite signature verifier. */
    protected Verifier verifier;

    /** Holds the install state. */
    protected InstallStateImpl state;

    /** An alias for more state.installInfo to get more compact record. */
    protected InstallInfo info;

    /** An alias for more state.suiteSettings to get more compact record. */
    protected SuiteSettings settings;

    /** Holds the CLDC configuration string. */
    protected String cldcConfig;

    /** Holds the device's Runtime Execution Environment string. */
    protected final String cldcRuntimeEnv = "MIDP.CLDC";

    /** Holds the MIDP supported profiles. */
    private Vector supportedProfiles;

    /** Use this to be the security domain for unsigned suites. */
    protected String unsignedSecurityDomain =
        Permissions.getUnsignedDomain();

    /* SecurityToken provided by the user of the JUMPInstaller */
    protected SecurityToken token = null;

    /**
     * Constructor of the JUMPInstaller.
     */
    public JUMPInstaller() {
        state = getInstallState();
        verifier = new VerifierImpl(state);

        /* Aliases for more compact record. */
        info = state.installInfo;
        settings = state.suiteSettings;
    }
    
    /**
     * Creates an instance of InstallState of the appropriate type
     * depending on the installer type. Should be overloaded in the
     * inherited classes.
     *
     * @return an instance of class containing the installation state
     */
    protected InstallStateImpl getInstallState() {
        if (state == null) {
            state = new InstallStateImpl();
        }

        return state;
    }

    /**
     * Installs a software package from the given URL. The URL is assumed
     * refer to an application descriptor.
     * <p>
     * If the component to be installed is the same as an existing
     * component (by comparing the <code>MIDlet-Name</code>,
     * <code>MIDlet-Vendor</code> attributes)
     * then this install is an upgrade if the version number is greater
     * than the current version.  If so, the new version replaces in its
     * entirety the current version.
     * <p>
     * It is implementation dependent when the upgraded component is
     * made available for use.
     * <p>
     * The implementation of install must be robust in the presence
     * of failures such as running out of memory.  If this method
     * throws an exception then the package must <em>not</em> be installed
     * and any previous version of the component must be left intact
     * and operational.
     * <p>
     * To receive status updates and installer warnings, provide an install
     * listener. If no listener is provided all warnings will be thrown
     * as exceptions.
     *
     * @param location the URL from which the application descriptor can be
     *        updated
     * @param storageId ID of the storage where the suite should be saved
     * @param force if <code>true</code> the MIDlet suite components to be
     *              installed will overwrite any existing components without
     *              any version comparison
     * @param removeRMS if <code>true</code> and existing RMS data will be
     *              removed when overwriting an existing suite
     * @param installListener object to receive status updates and install
     *     warnings, can be null
     *
     * @return the unique ID of the installed package.
     *
     * @exception ConnectionNotFoundException if JAD URL is invalid
     * @exception IOException is thrown if any error prevents the installation
     *   of the MIDlet suite, including being unable to access the application
     *   descriptor or JAR
     * @exception InvalidJadException if the downloaded application descriptor
     *   is invalid
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception SecurityException if the caller does not have permission
     *   to install software
     * @exception IllegalArgumentException is thrown, if the location of the
     * descriptor file is not specified
     */
    public int installJad(String location, int storageId, boolean force,
        boolean removeRMS, InstallListener installListener)
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException, SecurityException {

        info.jadUrl = location;
        state.force = force;
        state.removeRMS = removeRMS;
        state.nextStep = 1;
        state.listener = installListener;
        // IMPL_NOTE: sync with cldc installer for CHManager dependency
        // state.chmanager = CHManager.getManager(null);
        state.storageId = storageId;

        return performInstall();
    }

    public int installJad(String location, String jadLocal, String jarLocal, 
        String encoding, int storageId, boolean force,
        boolean removeRMS, InstallListener installListener)
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException, SecurityException {

        state.localJarUrl = jarLocal;
        state.localJadUrl = jadLocal;
        state.jadEncoding = encoding;

        return installJad(location, storageId, force, removeRMS,
			  installListener);
    }


    /**
     * Installs a software package from the given URL. The URL is assumed
     * refer to a JAR.
     * <p>
     * If the component to be installed is the same as an existing
     * component (by comparing the <code>MIDlet-Name</code>,
     * <code>MIDlet-Vendor</code> attributes)
     * then this install is an upgrade if the version number is greater
     * than the current version.  If so, the new version replaces in its
     * entirety the current version.
     * <p>
     * It is implementation dependent when the upgraded component is
     * made available for use.
     * <p>
     * The implementation of install must be robust in the presence
     * of failures such as running out of memory.  If this method
     * throws an exception then the package must <em>not</em> be installed
     * and any previous version of the component must be left intact
     * and operational.
     * <p>
     * To receive status updates and installer warnings, provide an install
     * listener. If no listener is provided all warnings will be thrown
     * as exceptions.
     *
     * @param location the URL which the JAR originally came from
     * @param name the name of the suite to be updated
     * @param storageId ID of the storage where the suite should be saved
     * @param force if <code>true</code> the MIDlet suite components to be
     *              installed will overwrite any existing components without
     *              any version comparison
     * @param removeRMS if <code>true</code> and existing RMS data will be
     *              removed when overwriting an existing suite
     * @param installListener object to receive status updates and install
     *     warnings, can be null
     *
     * @return the unique ID of the installed package.
     *
     * @exception IOException is thrown if any error prevents the installation
     *   of the MIDlet suite, including being unable to access the JAR
     * @exception InvalidJadException if the downloaded JAR is invalid
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception SecurityException if the caller does not have permission
     *   to install software
     * @exception IllegalArgumentException is thrown, if the location of the
     * JAR specified
     */
    public int installJar(String location, String name, int storageId,
            boolean force, boolean removeRMS, InstallListener installListener)
                throws IOException, InvalidJadException,
                    MIDletSuiteLockedException {

        if (location == null || location.length() == 0) {
            throw
                new IllegalArgumentException("Must specify URL of .jar file");
        }

        info.jarUrl = location;
        info.suiteName = name;
        state.force = force;
        state.removeRMS = removeRMS;
        state.file = new File();
        state.nextStep = 5;
        state.listener = installListener;
        // state.chmanager = CHManager.getManager(null);
        state.storageId = storageId;

        return performInstall();
    }

    /**
     * @param local    the location in local filesystem where the JAR
     * can be updated
     */
    public int installJar(String location, String local, String name,
			  int storageId, 
			  boolean force, boolean removeRMS,
			  InstallListener installListener)
	throws IOException, InvalidJadException,
	       MIDletSuiteLockedException {
        state.localJarUrl = local;
        return installJar(location, name, storageId, force,
			  removeRMS, installListener);
    }
    
    /**
     * Performs an install.
     *
     * @return the unique name of the installed package
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     * information
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception IllegalArgumentException is thrown, if the
     * descriptor file is not specified
     */
    protected int performInstall()
	throws IOException, InvalidJadException,
                   MIDletSuiteLockedException {
	
        state.midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
	
        /* Disable push interruptions during install. */
        PushRegistryInternal.enablePushLaunch(false);
	
        try {
            state.startTime = System.currentTimeMillis();
	    
            while (state.nextStep < 9) {
		
		
                /*
                 * clear the previous warning, so we can tell if another has
                 * happened
                 */
                state.exception = null;

                if (state.stopInstallation) {
                    postInstallMsgBackToProvider(
		        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }
		
                switch (state.nextStep) {
                case 1:
                    installStep1();
                    break;

                case 2:
                    installStep2();
                    break;

                case 3:
                    installStep3();
                    break;

                case 4:
                    installStep4();
                    break;

                case 5:
                    installStep5();
                    break;

                case 6:
                    installStep6();
                    break;

                case 7:
                    installStep7();
                    break;

                case 8:
                    installStep8();
                    break;

                default:
                    // for safety/completeness.
                    Logging.report(Logging.CRITICAL, LogChannels.LC_AMS,
                        "JUMPInstaller: Unknown step: " + state.nextStep);
                    break;
                }

                if (state.exception != null) {
                    if (state.listener == null) {
                        throw state.exception;
                    }

                    if (!state.listener.warnUser(state)) {
                        state.stopInstallation = true;
                        postInstallMsgBackToProvider(
                            OtaNotifier.USER_CANCELLED_MSG);
                        throw state.exception;
                    }
                }
            }
        } finally {
            if (state.previousSuite != null) {
                state.previousSuite.close();
            }
            if (info.jarFilename != null) {
                if (state.file.exists(info.jarFilename)) {
                    try {
                        state.file.delete(info.jarFilename);
                    } catch (Exception e) {
                        if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                            Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                            "delete file  threw an Exception");
                        }
                    }
                }
            }

            PushRegistryInternal.enablePushLaunch(true);
        }
        
        return info.id;
    }

    /**
     * Downloads the JAD, save it in the install state.
     * Parse the JAD, make sure it has
     * the required properties, and save them in the install state.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required attributes
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception IllegalArgumentException is thrown, if the
     * descriptor file is not specified
     */
    private void installStep1()
        throws IOException, InvalidJadException, MIDletSuiteLockedException {

        if (state.localJarUrl == null || state.localJadUrl.length() == 0) {
            throw
                new IllegalArgumentException("Must specify URL of .jad file");
        }

        try {
            state.jad = downloadJAD();
        } catch (OutOfMemoryError e) {
            try {
                postInstallMsgBackToProvider(
                    OtaNotifier.INSUFFICIENT_MEM_MSG);
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "Throwable during posting install message");
                }
            }

            throw new
                InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
        }

        if (state.exception != null) {
            return;
        }

        state.jadProps = new JadProperties();

        try {
            state.jadProps.load(new ByteArrayInputStream(state.jad),
                                state.jadEncoding);
        } catch (OutOfMemoryError e) {
            state.jad = null;
            try {
                postInstallMsgBackToProvider(
                    OtaNotifier.INSUFFICIENT_MEM_MSG);
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "Throwable during posting install message");
                }
            }

            throw new
                InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
        } catch (InvalidJadException ije) {
            state.jad = null;
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw ije;
        } catch (java.io.UnsupportedEncodingException uee) {
            state.jad = null;
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new InvalidJadException(
                InvalidJadException.UNSUPPORTED_CHAR_ENCODING,
                    state.jadEncoding);
        }

        info.suiteName = state.jadProps.getProperty(
            MIDletSuite.SUITE_NAME_PROP);
        if (info.suiteName == null || info.suiteName.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_SUITE_NAME);
        }

        info.suiteVendor = state.jadProps.getProperty(MIDletSuite.VENDOR_PROP);
        if (info.suiteVendor == null || info.suiteVendor.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_VENDOR);
        }

        info.suiteVersion = state.jadProps.getProperty(
            MIDletSuite.VERSION_PROP);
        if (info.suiteVersion == null || info.suiteVersion.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_VERSION);
        }

        try {
            checkVersionFormat(info.suiteVersion);
        } catch (NumberFormatException nfe) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new InvalidJadException(
                  InvalidJadException.INVALID_VERSION);
        }

        info.id = state.midletSuiteStorage.createSuiteID();

        checkPreviousVersion();
        state.nextStep++;
    }

    /**
     * If the JAD belongs to an installed suite, check the URL against the
     * installed one.
     */
    private void installStep2() {
        state.nextStep++;

        if (state.isPreviousVersion) {
            checkForDifferentDomains(info.jadUrl);
        }
    }

    /**
     * Makes sure the suite can fit in storage.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     */
    private void installStep3()
            throws IOException, InvalidJadException {
        String sizeString;
        int dataSize;
        int suiteSize;

        sizeString = state.jadProps.getProperty(MIDletSuite.JAR_SIZE_PROP);
        if (sizeString == null) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_JAR_SIZE);
        }

        try {
            info.expectedJarSize = Integer.parseInt(sizeString);
        } catch (NumberFormatException e) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        sizeString = state.jadProps.getProperty(MIDletSuite.DATA_SIZE_PROP);
        if (sizeString == null) {
            dataSize = 0;
        } else {
            try {
                dataSize = Integer.parseInt(sizeString);
            } catch (NumberFormatException e) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAD_MSG);
                throw new
                    InvalidJadException(InvalidJadException.INVALID_VALUE);
            }
        }

        /*
         * A suite is a jad + jar + manifest + url + data size.
         * lets say the manifest is the same size as the jad
         * since we do know at this point. the size is in bytes,
         * UTF-8 chars can be upto 3 bytes
         */
        suiteSize = info.expectedJarSize + (state.jad.length * 2) +
                    (info.jadUrl.length() * 3) + dataSize;
        state.jad = null;

        state.file = new File();
        
        /*
        if (suiteSize > state.file.getBytesAvailableForFiles(state.storageId)) {
            postInstallMsgBackToProvider(
                OtaNotifier.INSUFFICIENT_MEM_MSG);

            // the size reported to the user should be in K and rounded up
            throw new
                InvalidJadException(InvalidJadException.INSUFFICIENT_STORAGE,
                    Integer.toString((suiteSize + 1023)/ 1024));
        }
        */

        info.jarUrl = state.jadProps.getProperty(MIDletSuite.JAR_URL_PROP);
        if (info.jarUrl == null || info.jarUrl.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_JAR_URL);
        }

        state.nextStep++;
    }

    /**
     * Confirm installation with the user.
     *
     * @exception IOException is thrown, if the user cancels installation
     */
    private void installStep4()
            throws IOException {

        synchronized (state) {
            /* One more check to see if user has already canceled */
            if (state.stopInstallation) {
                postInstallMsgBackToProvider(
                    OtaNotifier.USER_CANCELLED_MSG);
                throw new IOException("stopped");
            }
            /*
             * Not canceled, so ignore cancel requests for now because below we
             * are going to ask anyway if user wants to install suite
             */
            state.ignoreCancel = true;
        }

        if (state.listener != null &&
            !state.listener.confirmJarDownload(state)) {
            state.stopInstallation = true;
            postInstallMsgBackToProvider(
                OtaNotifier.USER_CANCELLED_MSG);
            throw new IOException("stopped");
        }

        synchronized (state) {
            /* Allow cancel requests again */
            state.ignoreCancel = false;
        }
        state.nextStep++;
    }

    /**
     * Downloads the JAR, make sure it is the correct size, make sure
     * the required attributes match the JAD's. Then store the
     * application.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     */
    private void installStep5()
            throws IOException, InvalidJadException {
        int bytesDownloaded;
        MIDletInfo midletInfo;
        String midlet;

        // Send out delete notifications that have been queued, first
        OtaNotifier.postQueuedDeleteMsgsBackToProvider(state.proxyUsername,
            state.proxyPassword);

        // Save jar file to temp name; we need to do this to read the
        // manifest entry, but, don't want to overwrite an existing
        // application in case there are problems with the manifest
        state.storageRoot = File.getStorageRoot(state.storageId);
        info.jarFilename = state.storageRoot + TMP_FILENAME;

        bytesDownloaded = downloadJAR(info.jarFilename);

        if (state.exception != null) {
            return;
        }

        try {
            state.storage = new RandomAccessStream();

            state.installInfo.authPath =
                verifier.verifyJar(state.storage, info.jarFilename);

            if (state.listener != null) {
                state.listener.updateStatus(VERIFYING_SUITE, state);
            }

            // Create JAR Properties (From .jar file's MANIFEST)

            try {
                state.manifest = JarReader.readJarEntry(info.jarFilename,
                    MIDletSuite.JAR_MANIFEST);
                if (state.manifest == null) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new
                        InvalidJadException(InvalidJadException.CORRUPT_JAR,
                                            MIDletSuite.JAR_MANIFEST);
                }
            } catch (OutOfMemoryError e) {
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable during posting the install message");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            } catch (IOException ioe) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAR_MSG);
                throw new
                    InvalidJadException(InvalidJadException.CORRUPT_JAR,
                                        MIDletSuite.JAR_MANIFEST);
            }


            state.jarProps = new ManifestProperties();

            try {
		//  JarFile jarFile = 
		//    new JarFile(new java.io.File(info.jarFilename));
		//  Manifest manifest = jarFile.getManifest();

		//  state.jarProps.readFromAttributes(
		//    manifest.getMainAttributes());

                state.jarProps.load(new ByteArrayInputStream(state.manifest));
                state.manifest = null;
            } catch (OutOfMemoryError e) {
                state.manifest = null;
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable while posting install message ");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            } catch (InvalidJadException ije) {
                state.manifest = null;

                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                } catch (Throwable t) {
                    // ignore
                }

                throw ije;
            }

            for (int i = 1; ; i++) {
                midlet = state.getAppProperty("MIDlet-" + i);
                if (midlet == null) {
                    break;
                }

                /*
                 * Verify the MIDlet class is present in the JAR
                 * An exception thrown if not.
                 * Do the proper install notify on an exception
                 */
                try {
                    midletInfo = new MIDletInfo(midlet);

                    verifyMIDlet(midletInfo.classname);
                } catch (InvalidJadException ije) {
                    if (ije.getReason() == InvalidJadException.INVALID_VALUE) {
                        postInstallMsgBackToProvider(
                            OtaNotifier.INVALID_JAD_MSG);
                    } else {
                        postInstallMsgBackToProvider(
                            OtaNotifier.INVALID_JAR_MSG);
                    }
                    throw ije;
                }
            }

            // move on to the next step after a warning
            state.nextStep++;

            // Check Manifest entries against .jad file
            if (info.jadUrl != null) {
                if (bytesDownloaded != info.expectedJarSize) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.JAR_SIZE_MISMATCH_MSG);
                    throw new  InvalidJadException(
                        InvalidJadException.JAR_SIZE_MISMATCH);
                }

                if (!info.suiteName.equals(state.jarProps.getProperty(
                            MIDletSuite.SUITE_NAME_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                        InvalidJadException.SUITE_NAME_MISMATCH);
                }

                if (!info.suiteVersion.equals(
                        state.jarProps.getProperty(MIDletSuite.VERSION_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.VERSION_MISMATCH, 
                             new String(info.suiteVersion + ","
					+ state.jarProps.getProperty(
					    MIDletSuite.VERSION_PROP)));
                }

                if (!info.suiteVendor.equals(
                        state.jarProps.getProperty(MIDletSuite.VENDOR_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.VENDOR_MISMATCH);
                }
            } else {
                info.expectedJarSize = bytesDownloaded;
                info.suiteName = state.jarProps.getProperty(
                    MIDletSuite.SUITE_NAME_PROP);
                if (info.suiteName == null || info.suiteName.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_SUITE_NAME);
                }

                info.suiteVendor = state.jarProps.getProperty(
                    MIDletSuite.VENDOR_PROP);
                if (info.suiteVendor == null ||
                        info.suiteVendor.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_VENDOR);
                }

                info.suiteVersion = state.jarProps.getProperty(
                    MIDletSuite.VERSION_PROP);
                if (info.suiteVersion == null ||
                        info.suiteVersion.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_VERSION);
                }

                try {
                    checkVersionFormat(info.suiteVersion);
                } catch (NumberFormatException nfe) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.INVALID_VERSION);
                }

                // if already installed, check the domain of the JAR URL

                info.id = state.midletSuiteStorage.createSuiteID();

                checkPreviousVersion();
            }
        } catch (Exception e) {
            state.file.delete(info.jarFilename);

            if (e instanceof IOException) {
                throw (IOException)e;
            }

            throw (RuntimeException)e;
        }
    }

    /**
     * If the JAR belongs to an installed suite if there was
     * no JAD, check the URL against the installed one.
     */
    private void installStep6() {
        state.nextStep++;

        if (info.jadUrl == null && state.isPreviousVersion) {
            checkForDifferentDomains(info.jarUrl);
        }
    }

    /**
     * Checks the permissions and store the suite.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * storing the suite
     * @exception InvalidJadException is thrown, if the there is
     * permission problem
     */
    private void installStep7() throws IOException, InvalidJadException {

        try {
            if (info.authPath != null) {
                // suite was signed
                info.domain = verifier.getSecurityDomainName(info.authPath[0]);
                if (state.listener != null &&
                    !state.listener.confirmAuthPath(state)) {
                    state.stopInstallation = true;
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }
            } else {
                info.domain = unsignedSecurityDomain;
            }

            info.trusted = Permissions.isTrusted(info.domain);
                       
            // Do not overwrite trusted suites with untrusted ones
            if (!info.trusted && state.isPreviousVersion &&
                    state.previousSuite.isTrusted()) {

                postInstallMsgBackToProvider(
                    OtaNotifier.AUTHORIZATION_FAILURE_MSG);

                throw new InvalidJadException(
                    InvalidJadException.TRUSTED_OVERWRITE_FAILURE,
                        state.previousInstallInfo.authPath[0]);
            }

            /*
             * The unidentified suites do not get checked for requested
             * permissions.
             */
            if (Permissions.getUnsignedDomain().equals(info.domain)) {

                settings.setPermissions((Permissions.forDomain(
                    info.domain)) [Permissions.CUR_LEVELS]);

                /*
                 * To keep public key management simple, there is only one
                 * trusted keystore. So it is possible that the CA for
                 * the suite is untrusted. This may be done on purpose for
                 * testing. This is OK, but do not confuse the user by saying
                 * the untrusted suite is authorized, so set the CA name to
                 * null.
                 */
                info.authPath = null;
            } else {
                /*
                 * For identified suites, make sure an properties duplicated in
                 * both the manifest and JAD are the same.
                 */
                if (info.jadUrl != null) {
                    checkForJadManifestMismatches();
                }

                settings.setPermissions(getInitialPermissions(info.domain));
            }

            if (state.isPreviousVersion) {
                applyCurrentUserLevelPermissions(
                    state.previousSuite.getPermissions(),
                    (Permissions.forDomain(info.domain))
                        [Permissions.MAX_LEVELS],
                    settings.getPermissions());

                if (state.removeRMS) {
                    // override suite comparisons, just remove RMS
                    RecordStoreFactory.removeRecordStoresForSuite(null,
                        info.id);
                } else {
                    processPreviousRMS();
                }
            }

            state.securityHandler = new SecurityHandler(
                settings.getPermissions(), info.domain);

            checkRuntimeEnv();
            checkConfiguration();
            matchProfile();

	    // IMPL_NOTE: sync with cldc installer for CHManager dependency
	    /*
	     *    try {
	     *         state.chmanager.preInstall(this,
	     *                (InstallState)state,
	     *                (MIDletSuite)state,
	     *                (info.authPath == null ?
	     *                    null : info.authPath[0]));
	     *     } catch (InvalidJadException jex) {
	     *         // Post the correct install notify msg back to the server
	     *         String msg = OtaNotifier.INVALID_CONTENT_HANDLER;
	     *         if (jex.getReason() ==
	     *             InvalidJadException.CONTENT_HANDLER_CONFLICT) {
	     *             msg = OtaNotifier.CONTENT_HANDLER_CONFLICT;
	     *         }
	     *
	     *         postInstallMsgBackToProvider(msg);
	     *         throw jex;
	     *     } catch (SecurityException se) {
	     *         postInstallMsgBackToProvider(
	     *             OtaNotifier.AUTHORIZATION_FAILURE_MSG);
	     *
	     *         // since our state object put the permission in message
	     *         throw new InvalidJadException(
	     *             InvalidJadException.AUTHORIZATION_FAILURE,
	     *             se.getMessage());
	     *     }
	     */
	    
            // make sure at least 1 second has passed
            try {
                long waitTime = 1000 -
                    (System.currentTimeMillis() - state.startTime);

                if (waitTime > 0) {
                    Thread.sleep(waitTime);
                }
            } catch (InterruptedException ie) {
                // ignore
            }

            synchronized (state) {
                // this is the point of no return, one last check
                if (state.stopInstallation) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }

                state.ignoreCancel = true;
            }

            if (state.listener != null) {
                state.listener.updateStatus(STORING_SUITE, state);
            }

            registerPushConnections();

            /** Do the Content Handler registration updates now */
            // IMPL_NOTE: sync with cldc installer for CHManager dependency
            // state.chmanager.install();

            /*
             * Store suite will remove the suite including push connections,
             * if there an error, but may not remove the temp jar file.
             */
            MIDletInfo midletInfo = state.getMidletInfo();
            String midletClassNameToRun = null, iconName = null;
            MIDletSuiteInfo msi;

            if (midletInfo != null) {
                midletClassNameToRun = midletInfo.classname;
                iconName = midletInfo.icon;
            }

            msi = new MIDletSuiteInfo(info.id);
            msi.displayName = state.getDisplayName();
            msi.midletToRun = midletClassNameToRun;
            msi.numberOfMidlets = state.getNumberOfMIDlets();
            /* default is to enable a newly installed suite */
            msi.enabled = true;
            msi.trusted = info.trusted;
            msi.preinstalled = false;
            msi.iconName = iconName;
            msi.storageId = state.storageId;

            state.midletSuiteStorage.storeSuite(info, settings, msi,
                                                state.jadProps, state.jarProps);
        } catch (Throwable e) {
            state.file.delete(info.jarFilename);
            if (e instanceof IOException) {
                throw (IOException)e;
            }

            if (e instanceof OutOfMemoryError) {
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable during posting install message");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            }

            throw (RuntimeException)e;
        }

        state.nextStep++;

        try {
            postInstallMsgBackToProvider(OtaNotifier.SUCCESS_MSG);
        } catch (Throwable t) {
            /*
             * The suite is successfully installed, but the post of the
             * status message failed. Do not let this failure prevent
             * the suite from being used.
             */
        }
    }

    /**
     * Installation time optimizations are to be done in this step.
     * MONET optimization, VERIFY ONCE optimization, etc.
     * This step is done after obligatory installation part,
     * so the suite is downloaded, checked and stored by this moment.
     */
    private void installStep8() throws IOException {

        // In case of installation from JAR the suite id as well as other
        // properties are read from .jar file's MANIFEST rather then from
        // JAD file. Only after that application image can be generated.
        if (Constants.MONET_ENABLED) {
            if (state.listener != null) {
                state.listener.updateStatus(GENERATING_APP_IMAGE, state);
            }
            MIDletAppImageGenerator.createAppImage(
                info.id, state.midletSuiteStorage);
        }
        // Verification caching should be done if MONET disabled only
        else if (Constants.VERIFY_ONCE) {
            if (state.listener != null) {
                state.listener.updateStatus(VERIFYING_SUITE_CLASSES, state);
            }
            // Preverify all suite classes
            // in the case of success store hash value of the suite
            try {
                info.verifyHash =
                    MIDletSuiteVerifier.verifySuiteClasses(info.id,
                        state.midletSuiteStorage);
                if (info.verifyHash != null) {
                    state.midletSuiteStorage.storeSuiteVerifyHash(
                        info.id, info.verifyHash);
                }
            } catch (Throwable t) {
                // Notify installation listener of verifcation error
                state.exception = new InvalidJadException(
                    InvalidJadException.JAR_CLASSES_VERIFICATION_FAILED);
                if (state.listener != null) {
                    state.listener.updateStatus(
                        VERIFYING_SUITE_CLASSES, state);
                }

                // Clean exception since this step is optional and its
                // problems shouldn't cause whole installation failure
                state.exception = null;
            }
        }

        state.nextStep++;
    }

    /**
     * Verify that a class is present in the JAR file.
     * If the classname is invalid or is not found an
     * InvalidJadException is thrown.
     * @param classname the name of the class to verify
     * @exception InvalidJadException is thrown if the name is null or empty
     * or if the file is not found
     */
    public void verifyMIDlet(String classname) throws InvalidJadException {
        if (classname == null ||
            classname.length() == 0) {
            throw new
                InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        String file = classname.replace('.', '/').concat(".class");

        try {
            /* Attempt to read the MIDlet from the JAR file. */
            if (JarReader.readJarEntry(info.jarFilename, file) != null) {
                return;                // File found, normal return
            }
            // Fall into throwing the exception
        } catch (IOException ioe) {
            // Fall into throwing the exception
        }
        // Throw the InvalidJadException
        throw new InvalidJadException(InvalidJadException.CORRUPT_JAR, file);
    }

    /**
     * Downloads an application descriptor file from the given URL.
     *
     * @return a byte array representation of the file or null if not found
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the JAD
     */
    protected abstract byte[] downloadJAD() throws IOException;

    /**
     * Downloads an application archive file from the given URL into the
     * given file. Automatically handle re-tries.
     *
     * @param filename name of the file to write. This file resides
     *          in the storage area of the given application
     *
     * @return size of the JAR
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the JAR
     */
    protected abstract int downloadJAR(String filename) throws IOException;
   
    /**
     * If the JAD belongs to an installed suite, check the URL against the
     * installed one. Set the state.exception if the user needs to be warned.
     *
     * @param url JAD or JAR URL of the suite being installed
     */
    protected void checkForDifferentDomains(String url) {
        String previousUrl = state.previousInstallInfo.getDownloadUrl();
        // perform a domain check not a straight compare
        if (info.authPath == null && previousUrl != null) {
            HttpUrl old = new HttpUrl(previousUrl);
            HttpUrl current = new HttpUrl(url);

            if ((current.domain != null && old.domain == null) ||
                (current.domain == null && old.domain != null) ||
                (current.domain != null && old.domain != null &&
                 !current.domain.regionMatches(true, 0, old.domain, 0,
                                           old.domain.length()))) {
                /*
                 * The jad is at new location, could be bad,
                 * let the user decide
                 */
                state.exception = new InvalidJadException(
                    InvalidJadException.JAD_MOVED, previousUrl);
                return;
            }
        }
    }

    /**
     * See if there is an installed version of the suite being installed and
     * if so, make an necessary checks. Will set state fields, including
     * the exception field for warning the user.
     *
     * @exception InvalidJadException if the new version is formated
     * incorrectly
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     */
    protected void checkPreviousVersion()
        throws InvalidJadException, MIDletSuiteLockedException {

        int id;
        MIDletSuiteImpl midletSuite;
        String installedVersion;
        int cmpResult;

        // Check if app already exists
        id = MIDletSuiteStorage.getSuiteID(info.suiteVendor, info.suiteName);
        if (id == MIDletSuite.UNUSED_SUITE_ID) {
            // there is no previous version
            return;
        }
        
        try {
            midletSuite =
              state.midletSuiteStorage.getMIDletSuite(id, true);

            if (midletSuite == null) {
                // there is no previous version
                state.isPreviousVersion = false;
                return;
            }             
            checkVersionFormat(info.suiteVersion);

            state.isPreviousVersion = true;

            // This is now an update, use the old ID
            info.id = id;

            state.previousSuite = midletSuite;
            state.previousInstallInfo = midletSuite.getInstallInfo();

            if (state.force) {
                // do not ask questions, force an overwrite
                return;
            }

            // If it does, check version information
            installedVersion = midletSuite.getProperty(
                MIDletSuite.VERSION_PROP);
            cmpResult = vercmp(info.suiteVersion,
                               installedVersion);
            if (cmpResult < 0) {
                // older version, warn user
                state.exception = new InvalidJadException(
                                  InvalidJadException.OLD_VERSION,
                                  installedVersion);
                return;
            }

            if (cmpResult == 0) {
                // already installed, warn user
                state.exception = new InvalidJadException(
                                  InvalidJadException.ALREADY_INSTALLED,
                                  installedVersion);
                return;
            }

            // new version, warn user
            state.exception = new InvalidJadException(
                                  InvalidJadException.NEW_VERSION,
                                  installedVersion);
            return;
        } catch (MIDletSuiteCorruptedException mce) {
            if (state.listener != null) {
                state.listener.updateStatus(CORRUPTED_SUITE, state);
            }
        } catch (NumberFormatException nfe) {
            postInstallMsgBackToProvider(
                OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.INVALID_VERSION);
        }
    }

    /**
     * Posts a status message back to the provider's URL in JAD.
     *
     * @param message status message to post
     */
    protected void postInstallMsgBackToProvider(String message) {
        OtaNotifier.postInstallMsgBackToProvider(message, state,
            state.proxyUsername, state.proxyPassword);
    }

    /**
     * Function that actually does the work of transferring file data.
     * <p>
     * Updates the listener every 1 K bytes.
     * <p>
     * If the amount of data to be read is larger than <code>maxDLSize</code>
     * we will break the input into chunks no larger than
     * <code>chunkSize</code>. This prevents the VM from running out of
     * memory when processing large files.
     *
     * @param in the input stream to read from
     * @param out the output stream to write to
     * @param chunkSize size of piece to read from the input buffer
     *
     * @return number of bytes written to the output stream
     *
     * @exception IOException if any exceptions occur during transfer
     * of data
     */
    protected int transferData(InputStream in, OutputStream out, int chunkSize)
            throws IOException {
        byte[] buffer = new byte[chunkSize];
        int bytesRead;
        int totalBytesWritten = 0;

        if (state.listener != null) {
            state.listener.updateStatus(state.beginTransferDataStatus, state);
        }

        try {
            for (int nextUpdate = totalBytesWritten + 1024; ; ) {
                bytesRead = in.read(buffer);

                if (state.listener != null && (bytesRead == -1 ||
                        totalBytesWritten + bytesRead >= nextUpdate)) {

                    synchronized (state) {
                        if (state.stopInstallation) {
                            throw new IOException("stopped");
                        }

                        state.listener.updateStatus(state.transferStatus,
                                                    state);
                    }

                    nextUpdate = totalBytesWritten + 1024;
                }

                if (bytesRead == -1) {
                    return totalBytesWritten;
                }

                out.write(buffer, 0, bytesRead);
                totalBytesWritten += bytesRead;
            }
        } catch (IOException ioe) {
            if (state.stopInstallation) {
                postInstallMsgBackToProvider(
                    OtaNotifier.USER_CANCELLED_MSG);
                throw new IOException("stopped");
            } else {
                throw ioe;
            }
        }
    }

    /**
     * Retrieves a scheme component of the given URL.
     *
     * @param url url to parse
     * @param defaultScheme if the url has no scheme component, this one
     *                      will be returned; may be null
     *
     * @return scheme component of the given URL
     */
    public static String getUrlScheme(String url, String defaultScheme) {
        if (url == null) {
            return null;
        }

        /* this will parse any kind of URL, not only Http */
        HttpUrl parsedUrl = new HttpUrl(url);

        if (parsedUrl.scheme == null) {
            return defaultScheme;
        }

        return parsedUrl.scheme;
    }

    /**
     * Retrieves a path component of the given URL.
     *
     * @param url url to parse
     *
     * @return path component of the given URL
     */
    public static String getUrlPath(String url) {
        if (url == null) {
            return null;
        }

        /* this will parse any kind of URL, not only Http */
        HttpUrl parsedUrl = new HttpUrl(url);
        return parsedUrl.path;
    }

    /**
     * Compares two URLs for equality in sense that they have the same
     * scheme, host and path.
     *
     * @param url1 the first URL for comparision
     * @param url1 the second URL for comparision
     *
     * @return true if the scheme, host and path of the first given url
     *              is identical to the scheme, host and path of the second
     *              given url; false otherwise
     */
    protected abstract boolean isSameUrl(String url1, String url2);

    /**
     * If this is an update, make sure the RMS data is handle correctly
     * according to the OTA spec.
     * <p>
     * From the OTA spec:
     * <blockquote>
     * The RMS record stores of a MIDlet suite being updated MUST be
     * managed as follows:</p>
     * <ul>
     * <li>
     *   If the cryptographic signer of the new MIDlet suite and the
     *   original MIDlet suite are identical, then the RMS record
     *   stores MUST be retained and made available to the new MIDlet
     *   suite.</li>
     * <li>
     *   If the scheme, host, and path of the URL that the new
     *   Application Descriptor is downloaded from is identical to the
     *   scheme, host, and path of the URL the original Application
     *   Descriptor was downloaded from, then the RMS MUST be retained
     *   and made available to the new MIDlet suite.</li>
     * <li>
     *   If the scheme, host, and path of the URL that the new MIDlet
     *   suite is downloaded from is identical to the scheme, host, and
     *   path of the URL the original MIDlet suite was downloaded from,
     *   then the RMS MUST be retained and made available to the new
     *   MIDlet suite.</li>
     * <li>
     *   If the above statements are false, then the device MUST ask
     *   the user whether the data from the original MIDlet suite
     *   should be retained and made available to the new MIDlet
     *   suite.</li>
     * </ul>
     * </blockquote>
     *
     * @exception IOException if the install is stopped
     */
    protected void processPreviousRMS() throws IOException {
        if (!RecordStoreFactory.suiteHasRmsData(info.id)) {
            return;
        }

        if (state.previousInstallInfo.authPath != null &&
            info.authPath != null &&
            info.authPath[0].equals(
                state.previousInstallInfo.authPath[0])) {
            // signers the same
            return;
        }

        if (isSameUrl(info.jadUrl, state.previousInstallInfo.getJadUrl()) ||
            isSameUrl(info.jarUrl, state.previousInstallInfo.getJarUrl())) {
            return;
        }

        // ask the user, if no listener assume no for user's answer
        if (state.listener != null) {
            if (state.listener.keepRMS(state)) {
                // user wants to keep the data
                return;
            }
        }

        // this is a good place to check for a stop installing call
        if (state.stopInstallation) {
            postInstallMsgBackToProvider(
                OtaNotifier.USER_CANCELLED_MSG);
            throw new IOException("stopped");
        }

        RecordStoreFactory.removeRecordStoresForSuite(null, info.id);
    }

    /**
     * Stops the installation. If installer is not installing then this
     * method has no effect. This will cause the install method to
     * throw an IOException if the install is not writing the suite
     * to storage which is the point of no return.
     *
     * @return true if the install will stop, false if it is too late
     */
    public boolean stopInstalling() {
        if (state == null) {
            return false;
        }

        synchronized (state) {
            if (state.ignoreCancel) {
                return false;
            }

            state.stopInstallation = true;
        }

        return true;
    }

    /**
     * Tells if the installation was stopped by another thread.
     * @return true if the installation was stopped by another thread
     */
    public boolean wasStopped() {
        if (state == null) {
            return false;
        }

        return state.stopInstallation;
    }

    /**
     * Builds the initial API permission for suite currently being installed.
     *
     * @param domain security domain name for the CA of the suite
     *
     * @return current level of permissions
     *
     * @exception InvalidJadException if a permission attribute is not
     *     formatted properly or a required permission is denied
     */
    protected byte[] getInitialPermissions(String domain)
            throws InvalidJadException {
        byte[][] domainPermissions = Permissions.forDomain(domain);
        byte[] permissions = Permissions.getEmptySet();

        // only the current level of each permission has to be adjusted
        getRequestedPermissions(MIDletSuite.PERMISSIONS_PROP,
                                domainPermissions[Permissions.CUR_LEVELS],
                                permissions, true);

        getRequestedPermissions(MIDletSuite.PERMISSIONS_OPT_PROP,
                                domainPermissions[Permissions.CUR_LEVELS],
                                permissions, false);

        return permissions;
    }

    /**
     * Gets the permissions for a domain that are requested the manifest.
     *
     * @param propName name of the property in the manifest
     * @param domainPermissions array of the starting levels for permissions
     *        of a domain
     * @param permissions array to put the permissions from the domain in
     *        when found in the manifest property
     * @param required if set to true the manifest permissions are required
     *
     * @exception InvalidJadException if a permission attribute is not
     *     formatted properly or a required permission is denied
     */
    private void getRequestedPermissions(String propName,
            byte[] domainPermissions, byte[] permissions, boolean required)
            throws InvalidJadException {

        String jadPermissionLine;
        String reqPermissionLine;
        Vector reqPermissions;
        String permission;
        boolean found;
        int i;

        reqPermissionLine = state.getAppProperty(propName);
        if (reqPermissionLine == null || reqPermissionLine.length() == 0) {
            // Zero properties are allowed.
            return;
        }

        reqPermissions = Util.getCommaSeparatedValues(reqPermissionLine);
        if (reqPermissions.size() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        for (int j = 0; j < reqPermissions.size(); j++) {
            permission = (String)reqPermissions.elementAt(j);

            if (permission.length() == 0) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAD_MSG);
                throw new
                    InvalidJadException(InvalidJadException.INVALID_VALUE);
            }

            found = false;
            for (i = 0; i < Permissions.NUMBER_OF_PERMISSIONS; i++) {
                if (Permissions.getName(i).equals(permission)) {
                    if (domainPermissions[i] != Permissions.NEVER) {
                        found = true;
                    }

                    break;
                }
            }

            if (!found) {
                if (required) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.AUTHORIZATION_FAILURE_MSG);
                    throw new InvalidJadException(
                        InvalidJadException.AUTHORIZATION_FAILURE, permission);
                }

                continue;
            }

            permissions[i] = domainPermissions[i];
        }
    }

    /**
     * Apply the previous user level permission of the currently installed
     * version of a suite to the next version of the suite in a secure way.
     *
     * @param current array permissions for the current version
     * @param domainPermissions array of the starting levels for permissions
     *        of the new domain
     * @param next array permissions for the next version
     */
    private void applyCurrentUserLevelPermissions(byte[] current,
            byte[] domainPermissions, byte[] next) {

        for (int i = 0; i < current.length && i < next.length; i++) {
            switch (current[i]) {
            case Permissions.ALLOW:
            case Permissions.NEVER:
                // not a user level permission
                continue;
            }

            switch (domainPermissions[i]) {
            case Permissions.ALLOW:
            case Permissions.NEVER:
                // not a user level permission
                continue;

            case Permissions.ONESHOT:
                if (current[i] == Permissions.SESSION) {
                    // do not apply
                    continue;
                }
                // fall through; per-session permissions may be permitted.

            case Permissions.SESSION:
                if (current[i] == Permissions.BLANKET ||
                    current[i] == Permissions.BLANKET_GRANTED) {
                    // do not apply
                    continue;
                }
                // fall through to store the permission for the next version.

            default:
                next[i] = current[i];
                continue;
            }
        }
    }

    /**
     * Checks to see if the JAD has a signature, but does not verify the
     * signature. This is a place holder the the Secure JUMPInstaller and
     * just returns false.
     *
     * @return true if the JAD has a signature
     */
    public boolean isJadSigned() {
        return verifier.isJadSigned();
    }

    /**
     * Sets security domain for unsigned suites. The default is untrusted.
     * Can only be called by JAM for testing.
     *
     * @param domain name of a security domain
     */
    public void setUnsignedSecurityDomain(String domain) {
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);

        unsignedSecurityDomain = domain;
    }

    /**
     * Checks to see that if any properties that are both in the JAD and
     * JAR manifest are not equal and throw a exception and notify the
     * server when a mismatch is found. Only used for trusted suites.
     * @exception InvalidJadException  if the properties do not match
     */
    protected void checkForJadManifestMismatches()
            throws InvalidJadException {

        for (int i = 0; i < state.jarProps.size(); i++) {
            String key = state.jarProps.getKeyAt(i);
            String value = state.jarProps.getValueAt(i);
            String dup = state.jadProps.getProperty(key);

            if (dup == null) {
                continue;
            }

            if (!dup.equals(value)) {
                postInstallMsgBackToProvider(
                    OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                throw new InvalidJadException(
                    InvalidJadException.ATTRIBUTE_MISMATCH, key);
            }
        }
    }

    /**
     * Compares two version strings. The return values are very similar to
     * that of strcmp() in 'C'. If the first version is less than the second
     * version, a negative number will be returned. If the first version is
     * greater than the second version, a positive number will be returned.
     * If the two versions are equal, zero is returned.
     * <p>
     * Versions must be in the form <em>xxx.yyy.zzz</em>, where:
     * <pre>
     *     <em>xxx</em> is the major version
     *     <em>yyy</em> is the minor version
     *     <em>zzz</em> is the micro version
     * </pre>
     * It is acceptable to omit the micro and possibly the minor versions.
     * If these are not included in the version string, the period immediately
     * preceding the number must also be removed. So, the versions
     * <em>xxx.yyy</em> or <em>xxx</em> are also valid.
     * <p>
     * Version numbers do not have to be three digits wide. However, you may
     * pad versions with leading zeros if desired.
     * <p>
     * If a version number is omitted, its value is assumed to be zero. All
     * tests will be based on this assumption.
     * <p>
     * For example:
     * <pre>
     *    1.04  >  1.
     *    1.04  <  1.4.1
     *    1.04  =  1.4.0
     * </pre>
     * <p>
     *
     * @param ver1 the first version to compare
     * @param ver2 the second version to compare
     *
     * @return  1 if <code>ver1</code> is greater than <code>ver2</code>
     *          0 if <code>ver1</code> is equal to <code>ver2</code>
     *         -1 if <code>ver1</code> is less than <code>ver2</code>
     *
     * @exception NumberFormatException if either <code>ver1</code> or
     * <code>ver2</code> contain characters that are not numbers or periods
     */
    private static int vercmp(String ver1, String ver2)
            throws NumberFormatException {
        String strVal1;
        String strVal2;
        int    intVal1;
        int    intVal2;
        int    idx1 = 0;
        int    idx2 = 0;
        int    newidx;

        if ((ver1 == null) && (ver2 == null)) {
            return 0;
        }

        if (ver1 == null) {
            return -1;
        }

        if (ver2 == null) {
            return 1;
        }

        for (int i = 0; i < 3; i++) {
            strVal1 = "0"; // Default value
            strVal2 = "0"; // Default value
            if (idx1 >= 0) {
                newidx = ver1.indexOf('.', idx1);
                if (newidx < 0) {
                    strVal1 = ver1.substring(idx1);
                } else {
                    strVal1 = ver1.substring(idx1, newidx);
                    newidx++; // Idx of '.'; need to go to next char
                }

                idx1 = newidx;
            }

            if (idx2 >= 0) {
                newidx = ver2.indexOf('.', idx2);
                if (newidx < 0) {
                    strVal2 = ver2.substring(idx2);
                } else {
                    strVal2 = ver2.substring(idx2, newidx);
                    newidx++;
                }

                idx2 = newidx;
            }

            intVal1 = Integer.parseInt(strVal1); // May throw NFE
            intVal2 = Integer.parseInt(strVal2); // May throw NFE

            if (intVal1 > intVal2) {
                return 1;
            }

            if (intVal1 < intVal2) {
                return -1;
            }
        }

        return 0;
    }

    /**
     * Checks the format of a version string.
     * <p>
     * Versions must be in the form <em>xxx.yyy.zzz</em>, where:
     * <pre>
     *     <em>xxx</em> is the major version
     *     <em>yyy</em> is the minor version
     *     <em>zzz</em> is the micro version
     * </pre>
     * It is acceptable to omit the micro and possibly the minor versions.
     * If these are not included in the version string, the period immediately
     * preceding the number must also be removed. So, the versions
     * <em>xxx.yyy</em> or <em>xxx</em> are also valid.
     * <p>
     * Version numbers do not have to be three digits wide. However, you may
     * pad versions with leading zeros if desired.
     *
     * @param ver the version to check
     *
     * @exception NumberFormatException if <code>ver</code>
     *     contains any characters that are not numbers or periods
     */
    private static void checkVersionFormat(String ver)
            throws NumberFormatException {
        int length;
        int start = 0;
        int end;

        length = ver.length();
        for (int i = 0; ; i++) {
            // check for more than 3 parts or a trailing '.'
            if (i == 3 || start == length) {
                throw new NumberFormatException();
            }

            end = ver.indexOf('.', start);
            if (end == -1) {
                end = length;
            }

            // throws NFE if the substring is not all digits
            Integer.parseInt(ver.substring(start, end));

            if (end == length) {
                // we are done
                return;
            }

            // next time around start after the index of '.'
            start = end + 1;
        }
    }

    /**
     * Checks to make sure the runtime environment required by
     * the application is supported.
     * Send a message back to the server if the check fails and
     * throw an exception.
     *
     * @exception InvalidJadException if the check fails
     */
    private void checkRuntimeEnv() throws InvalidJadException {
        String execEnv;

        execEnv = state.getAppProperty(MIDletSuite.RUNTIME_EXEC_ENV_PROP);
        if (execEnv == null || execEnv.length() == 0) {
            execEnv = MIDletSuite.RUNTIME_EXEC_ENV_DEFAULT;
        }

        // need to call trim to remove trailing spaces
        execEnv = execEnv.trim();

        if (execEnv.equals(cldcRuntimeEnv)) {
            // success, done
            return;
        }

        postInstallMsgBackToProvider(OtaNotifier.INCOMPATIBLE_MSG);
        throw new InvalidJadException(InvalidJadException.DEVICE_INCOMPATIBLE);
    }

    /**
     * Match the name of the configuration or profile, and return
     * true if the first name has a greater or equal version than the
     * second. The names of the format "XXX-Y.Y" (e.g. CLDC-1.0, MIDP-2.0)
     * as used in the system properties (microedition.configuration &
     * microedition.profiles).
     *
     * This is used for checking both configuration and profiles.
     *
     * @param name1 name of configuration or profile
     * @param name2 name of configuration or profile
     * @return  true is name1 matches name2 and is greater or equal in
     *          version number. false otherwise
     */
    private static boolean matchVersion(String name1, String name2) {
        int dash1 = name1.indexOf('-');

        if (dash1 < 0) {
            return false;
        }

        int dash2 = name2.indexOf('-');

        if (dash2 < 0) {
            return false;
        }

        String base1 = name1.substring(0, dash1);
        String base2 = name2.substring(0, dash2);

        if (!base1.equals(base2)) {
            return false;
        }

        String ver1 = name1.substring(dash1 + 1, name1.length());
        String ver2 = name2.substring(dash2 + 1, name2.length());

        return (vercmp(ver1, ver2) >= 0);
    }

    /**
     * Checks to make sure the configration need by the application
     * is supported.
     * Send a message back to the server if the check fails and
     * throw an exception.
     *
     * @exception InvalidJadException if the check fails
     */
    private void checkConfiguration() throws InvalidJadException {
        String config;

        config = state.getAppProperty(MIDletSuite.CONFIGURATION_PROP);
        if (config == null || config.length() == 0) {
            postInstallMsgBackToProvider(
                OtaNotifier.INVALID_JAR_MSG);
            throw new InvalidJadException(
                InvalidJadException.MISSING_CONFIGURATION);
        }

        if (cldcConfig == null) {
            // need to call trim to remove trailing spaces
            cldcConfig =
                System.getProperty(MICROEDITION_CONFIG).trim();
        }

        if (matchVersion(cldcConfig, config)) {
            // success, done
            return;
        }

        postInstallMsgBackToProvider(OtaNotifier.INCOMPATIBLE_MSG);
        throw new InvalidJadException(InvalidJadException.DEVICE_INCOMPATIBLE);
    }

    /**
     * Tries to match one of the supported profiles with a profile
     * listed in string of profiles separated by a space.
     * Send a message back to the server if a match is not found and
     * throw an exception.
     *
     * @exception InvalidJadException if there is no match
     */
    private void matchProfile() throws InvalidJadException {
        String profiles = state.getAppProperty(MIDletSuite.PROFILE_PROP);

        if (profiles == null || profiles.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAR_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_PROFILE);
        }

        // build the list of supported profiles if needed
        if (supportedProfiles == null) {
            int start;
            int nextSpace = -1;
            String meProfiles =
                System.getProperty(MICROEDITION_PROFILES);
            if (meProfiles == null || meProfiles.length() == 0) {
                throw new RuntimeException(
                    "system property microedition.profiles not set");
            }
            supportedProfiles = new Vector();
            // need to call trim to remove trailing spaces
            meProfiles = meProfiles.trim();

            for (; ; ) {
                start = nextSpace + 1;
                nextSpace = meProfiles.indexOf(' ', start);

                // consecutive spaces, keep searching
                if (nextSpace == start) {
                    continue;
                }

                if ((nextSpace < 0)) {
                    supportedProfiles.addElement(
                        meProfiles.substring(start, meProfiles.length()));
                    break;
                }

                supportedProfiles.addElement(
                    meProfiles.substring(start, nextSpace));

            }
        }

        /*
         * for each profiles listed in MicroEdition-Profile, we need to
         * find a matching profile in microedition.profiles.
         */
        int current = 0;
        int nextSeparatorIndex = 0;
        String requestedProfile;
        boolean supported = false;

        // convert tab to space so that the parsing later is simplified
        StringBuffer tmp = new StringBuffer(profiles);
        boolean modified = false;
        while ((nextSeparatorIndex = profiles.indexOf('\t', current)) != -1) {
            tmp.setCharAt(nextSeparatorIndex, ' ');
            current++;
            modified = true;
        }

        if (modified) {
            profiles = tmp.toString();
        }

        // reset the indices
        current = nextSeparatorIndex = 0;
        do {
            // get the next requested profiles
            nextSeparatorIndex = profiles.indexOf(' ', current);

            if (nextSeparatorIndex == current) {
                // consecutive spaces, keep searching
                current++;
                continue;
            }

            if (nextSeparatorIndex == -1) {
                // last (or the only one) value in the list
                requestedProfile =
                   profiles.substring(current, profiles.length());
            } else {
                requestedProfile =
                    profiles.substring(current, nextSeparatorIndex);
                current = nextSeparatorIndex + 1;
            }

            /*
             * try to match each requested profiles against the supported
             * ones.
             */
            supported = false;
            for (int i = 0; i < supportedProfiles.size(); i++) {
                String supportedProfile =
                    (String)supportedProfiles.elementAt(i);
                if (matchVersion(supportedProfile, requestedProfile)) {
                     supported = true;
                     break;
                }
            }

            // short circuit the test if there is one mismatch
            if (!supported) {
                break;
            }
        } while (nextSeparatorIndex != -1);

        // matched all requested profiles against supported ones
        if (supported) {
            return;
        }

        postInstallMsgBackToProvider(OtaNotifier.INCOMPATIBLE_MSG);
        throw new InvalidJadException(InvalidJadException.DEVICE_INCOMPATIBLE);
    }

    /**
     * Registers the push connections for the application.
     * Send a message back to the server if a connection cannot be
     * registered and throw an exception.
     *
     * @exception InvalidJadException if a connection cannot be registered
     */
    private void registerPushConnections()
            throws InvalidJadException {
        byte[] curLevels = settings.getPermissions();

        if (state.isPreviousVersion) {
            PushRegistryInternal.unregisterConnections(info.id);
        }

        for (int i = 1; ; i++) {
            String pushProp;

            pushProp = state.getAppProperty("MIDlet-Push-" + i);
            if (pushProp == null) {
                break;
            }

            /*
             * Parse the comma separated values  -
             *  " connection, midlet, role, filter"
             */
            int comma1 = pushProp.indexOf(',', 0);
            int comma2 = pushProp.indexOf(',', comma1 + 1);

            String conn = pushProp.substring(0, comma1).trim();
            String midlet = pushProp.substring(comma1+1, comma2).trim();
            String filter = pushProp.substring(comma2+1).trim();

            /* Register the new push connection string. */
            try {
                PushRegistryInternal.registerConnectionInternal(state,
                    conn, midlet, filter, false);
            } catch (Exception e) {
                /* If already registered, abort the installation. */
                PushRegistryInternal.unregisterConnections(info.id);

                if (state.isPreviousVersion) {
                    // put back the old ones, removed above
                    redoPreviousPushConnections();
                }

                if (e instanceof SecurityException) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.AUTHORIZATION_FAILURE_MSG);

                    // since our state object put the permission in message
                    throw new InvalidJadException(
                        InvalidJadException.AUTHORIZATION_FAILURE,
                        e.getMessage());
                }

                postInstallMsgBackToProvider(
                    OtaNotifier.PUSH_REG_FAILURE_MSG);

                if (e instanceof IllegalArgumentException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_FORMAT_FAILURE, pushProp);
                }

                if (e instanceof ConnectionNotFoundException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_PROTO_FAILURE, pushProp);
                }

                if (e instanceof IOException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_DUP_FAILURE, pushProp);
                }

                if (e instanceof ClassNotFoundException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_CLASS_FAILURE, pushProp);
                }

                // error in the implementation code
                throw (RuntimeException)e;
            }
        }

        if (state.isPreviousVersion) {
            // preserve the push options when updating
            settings.setPushOptions(state.previousSuite.getPushOptions());

            // use the old setting
            settings.setPushInterruptSetting(
                (byte)state.previousSuite.getPushInterruptSetting());

            // The old suite may have not had push connections
            if (settings.getPushInterruptSetting() != Permissions.NEVER) {
                return;
            }
        }

		int PUSH_ID = Permissions.getId("javax.microedition.io.PushRegistry");
        if (curLevels[PUSH_ID] == Permissions.NEVER) {
            settings.setPushInterruptSetting(Permissions.NEVER);
        } else if (curLevels[PUSH_ID] == Permissions.ALLOW) {
            // Start the default at session for usability when denying.
            settings.setPushInterruptSetting(Permissions.SESSION);
        } else {
            settings.setPushInterruptSetting(curLevels[PUSH_ID]);
        }
    }

    /**
     * Registers the push connections for previous version after
     * and aborted update.
     */
    private void redoPreviousPushConnections() {
        for (int i = 1; ; i++) {
            String pushProp;

            pushProp = state.previousSuite.getProperty("MIDlet-Push-" + i);
            if (pushProp == null) {
                break;
            }

            /*
             * Parse the comma separated values  -
             *  " connection, midlet, role, filter"
             */
            int comma1 = pushProp.indexOf(',', 0);
            int comma2 = pushProp.indexOf(',', comma1 + 1);

            String conn = pushProp.substring(0, comma1).trim();
            String midlet = pushProp.substring(comma1+1, comma2).trim();
            String filter = pushProp.substring(comma2+1).trim();

            /* Register the new push connection string. */
            try {
                PushRegistryInternal.registerConnectionInternal(
                    state, conn, midlet, filter, true);
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "registerConnectionInternal  threw an IOException");
                }
            } catch (ClassNotFoundException e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "registerConnectionInternal threw a " +
                        "ClassNotFoundException");
                }
            }
        }
    }
}
