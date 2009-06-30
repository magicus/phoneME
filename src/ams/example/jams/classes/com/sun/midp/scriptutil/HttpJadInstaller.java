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

package com.sun.midp.scriptutil;

import java.io.*;

import javax.microedition.midlet.MIDlet;

import com.sun.midp.configurator.Constants;

import com.sun.midp.installer.*;

import com.sun.midp.midlet.*;

import com.sun.midp.midletsuite.*;

/**
 * Installs/Updates a suite, prints out the result and parameters to
 * run the first MIDlet of the suite using runMidlet. Upon success
 * write to standard output the suite ID and class name of the first
 * MIDlet enclosed by brackets. ["suiteID" "class name"]
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for the suite
 *   <li>arg-1: Used to override the default domain used when installing
 *    an unsigned suite. The default is maximum to allow the runtime API tests
 *    be performed automatically without tester interaction. The domain name
 *    may be followed by a colon and a list of permissions that must be allowed
 *    even if they are not listed in the MIDlet-Permissions attribute in the
 *    application descriptor file. Instead of the list a keyword "all" can be
 *    specified indicating that all permissions must be allowed, for example:
 *    operator:all.
 * </ol>
 */
public class HttpJadInstaller extends MIDlet implements Runnable {
    /** URL of the test suite. */
    String url;
    /** The installer. */
    HttpInstaller installer;

    /**
     * Create and initialize the MIDlet.
     */
    public HttpJadInstaller() {
        // The arg-<n> properties are generic command arguments
        url = getAppProperty("arg-0");
        if (url == null) {
            System.err.println("URL argument missing.");
            System.err.println("HTTP JAD installer arguments: " +
                "<HTTP URL> [<domain for unsigned suites>]");

            notifyDestroyed();
            return;
        }

        installer = new HttpInstaller();

        // URL given as a argument, look for a domain arg and then start
        String domain = getAppProperty("arg-1");
        if (domain != null) {
            String additionalPermissions = null;
            int index = domain.indexOf(":");
            int len = domain.length();

            if (index > 0 && index + 1 < len) {
                additionalPermissions = domain.substring(index + 1, len);
                domain = domain.substring(0, index);
            }

            installer.setUnsignedSecurityDomain(domain);
            installer.setExtraPermissions(additionalPermissions);
        }

        new Thread(this).start();
    }

    /**
     * Start.
     */
    public void startApp() {
    }

    /**
     * Pause; there are no resources that need to be released.
     */
    public void pauseApp() {
    }

    /**
     * Destroy cleans up.
     *
     * @param unconditional is ignored; this object always
     * destroys itself when requested.
     */
    public void destroyApp(boolean unconditional) {
    }


    /** Install a MIDlet suite. */
    public void run() {
        int suiteId;
        MIDletInfo midletInfo;

        try {
            // force an overwrite and remove the RMS data
            suiteId = installer.installJad(url, Constants.INTERNAL_STORAGE_ID,
                                           true, true, null);
            midletInfo = getFirstMIDletOfSuite(suiteId);

            System.out.println("*Installed suite " + midletInfo.name);
            System.out.println("[" + suiteId + " " +
                               midletInfo.classname + "]");
        } catch (Throwable t) {
            handleInstallerException(t);
        }

        notifyDestroyed();
    }

    /**
     * Returns the information of the first MIDlet of the newly installed
     * suite.
     *
     * @param suiteId ID of the MIDlet Suite
     *
     * @return an object with the class name and display name of
     * the suite's MIDlet-1 property
     */
    static MIDletInfo getFirstMIDletOfSuite(int suiteId) {
        MIDletSuiteStorage midletSuiteStorage =
            MIDletSuiteStorage.getMIDletSuiteStorage();
        MIDletSuite ms = null;
        String name = null;

        try {
            ms = midletSuiteStorage.getMIDletSuite(suiteId, false);
            name = ms.getProperty("MIDlet-1");
        } catch (Exception e) {
            throw new RuntimeException("midlet properties corrupted");
        } finally {
            if (ms != null) {
                ms.close();
            }
        }

        if (name == null) {
            throw new RuntimeException("MIDlet-1 missing");
        }

        // found the entry now parse out the class name, and display name
        return new MIDletInfo(name);
    }


    /**
     * Handles an installer exceptions.
     *
     * @param ex exception to handle
     */
    static void handleInstallerException(Throwable ex) {
        String message = null;

        if (ex instanceof InvalidJadException) {
            InvalidJadException ije = (InvalidJadException)ex;
            int reason = ije.getReason();

            message = "** Error installing suite (" + reason + "): " +
                messageForInvalidJadException(ije);
        } else if (ex instanceof IOException) {
            message = "** I/O Error installing suite: " + ex.getMessage();
        } else {
            message = "** Error installing suite: " + ex.toString();
        }

        if (message != null) {
            System.err.println(message);
        }
    }

    /**
     * Returns the associated message for the given exception.
     * This function is here instead of in the exception its self because
     * it not need on devices, it needed only on development platforms that
     * have command line interface.
     *
     * @param ije reason reason code for the exception
     *
     * @return associated message for the given reason
     */
    static String messageForInvalidJadException(InvalidJadException ije) {
        switch (ije.getReason()) {
        case InvalidJadException.MISSING_PROVIDER_CERT:
        case InvalidJadException.MISSING_SUITE_NAME:
        case InvalidJadException.MISSING_VENDOR:
        case InvalidJadException.MISSING_VERSION:
        case InvalidJadException.MISSING_JAR_URL:
        case InvalidJadException.MISSING_JAR_SIZE:
        case InvalidJadException.MISSING_CONFIGURATION:
        case InvalidJadException.MISSING_PROFILE:
            return "A required attribute is missing";

        case InvalidJadException.SUITE_NAME_MISMATCH:
        case InvalidJadException.VERSION_MISMATCH:
        case InvalidJadException.VENDOR_MISMATCH:
            return "A required suite ID attribute in the JAR manifest " +
                "do not match the one in the JAD";

        case InvalidJadException.ATTRIBUTE_MISMATCH:
            return "The value for " + ije.getExtraData() + " in the " +
                "trusted JAR manifest did not match the one in the JAD";

        case InvalidJadException.CORRUPT_PROVIDER_CERT:
            return "The content provider certificate cannot be decoded.";

        case InvalidJadException.UNKNOWN_CA:
            return "The content provider certificate issuer " +
                ije.getExtraData() + " is unknown.";

        case InvalidJadException.INVALID_PROVIDER_CERT:
            return "The signature of the content provider certificate " +
                "is invalid.";

        case InvalidJadException.CORRUPT_SIGNATURE:
            return "The JAR signature cannot be decoded.";

        case InvalidJadException.INVALID_SIGNATURE:
            return "The signature of the JAR is invalid.";

        case InvalidJadException.UNSUPPORTED_CERT:
            return "The content provider certificate is not a supported " +
                "version.";

        case InvalidJadException.EXPIRED_PROVIDER_CERT:
            return "The content provider certificate is expired.";

        case InvalidJadException.EXPIRED_CA_KEY:
            return "The public key of " + ije.getExtraData() +
                " has expired.";

        case InvalidJadException.JAR_SIZE_MISMATCH:
            return "The Jar downloaded was not the size in the JAD";

        case InvalidJadException.OLD_VERSION:
            return "The application is an older version of one that is " +
                "already installed";

        case InvalidJadException.NEW_VERSION:
            return "The application is an newer version of one that is " +
                "already installed";

        case InvalidJadException.INVALID_JAD_URL:
            return "The JAD URL is invalid";

        case InvalidJadException.JAD_SERVER_NOT_FOUND:
            return "JAD server not found";

        case InvalidJadException.JAD_NOT_FOUND:
            return "JAD not found";

        case InvalidJadException.INVALID_JAR_URL:
            return "The JAR URL in the JAD is invalid: " +
                ije.getExtraData();

        case InvalidJadException.JAR_SERVER_NOT_FOUND:
            return "JAR server not found: " + ije.getExtraData();

        case InvalidJadException.JAR_NOT_FOUND:
            return "JAR not found: " + ije.getExtraData();

        case InvalidJadException.CORRUPT_JAR:
            return "Corrupt JAR, error while reading: " +
                ije.getExtraData();

        case InvalidJadException.INVALID_JAR_TYPE:
            if (ije.getExtraData() != null) {
                return "JAR did not have the correct media type, it had " +
                    ije.getExtraData();
            }

            return "The server did not have a resource with an " +
                "acceptable media type for the JAR URL. (code 406)";

        case InvalidJadException.INVALID_JAD_TYPE:
            if (ije.getExtraData() != null) {
                String temp = ije.getExtraData();

                if (temp.length() == 0) {
                    return "JAD did not have a media type";
                }

                return "JAD did not have the correct media type, it had " +
                    temp;
            }

            /*
             * Should not happen, the accept field is not send
             * when getting the JAD.
             */
            return "The server did not have a resource with an " +
                "acceptable media type for the JAD URL. (code 406)";

        case InvalidJadException.INVALID_KEY:
            return "The attribute key [" + ije.getExtraData() +
                "] is not in the proper format";

        case InvalidJadException.INVALID_VALUE:
            return "The value for attribute " + ije.getExtraData() +
                " is not in the proper format";

        case InvalidJadException.INSUFFICIENT_STORAGE:
            return "There is insufficient storage to install this suite";

        case InvalidJadException.UNAUTHORIZED:
            return "Authentication required or failed";

        case InvalidJadException.JAD_MOVED:
            return "The JAD to be installed is for an existing suite, " +
                "but not from the same domain as the existing one: " +
                ije.getExtraData();

        case InvalidJadException.CANNOT_AUTH:
            return
                "Cannot authenticate with the server, unsupported scheme";

        case InvalidJadException.DEVICE_INCOMPATIBLE:
            return "Either the configuration or profile is not supported.";

        case InvalidJadException.ALREADY_INSTALLED:
            return
                "The JAD matches a version of a suite already installed.";

        case InvalidJadException.AUTHORIZATION_FAILURE:
            return "The suite is not authorized for " + ije.getExtraData();

        case InvalidJadException.PUSH_DUP_FAILURE:
            return "The suite is in conflict with another application " +
                "listening for network data on " + ije.getExtraData();

        case InvalidJadException.PUSH_FORMAT_FAILURE:
            return "Push attribute in incorrectly formated: " +
                ije.getExtraData();

        case InvalidJadException.PUSH_PROTO_FAILURE:
            return "Connection in push attribute is not supported: " +
                ije.getExtraData();

        case InvalidJadException.PUSH_CLASS_FAILURE:
            return "The class in push attribute not in a MIDlet-<n> " +
                "attribute: " + ije.getExtraData();

        case InvalidJadException.TRUSTED_OVERWRITE_FAILURE:
            return "Cannot update a trusted suite with an untrusted " +
                "version";

        case InvalidJadException.INVALID_CONTENT_HANDLER:
            return "Content handler attribute(s) incorrectly formatted: " +
                ije.getExtraData();

        case InvalidJadException.CONTENT_HANDLER_CONFLICT:
            return "Content handler would conflict with another handler: " +
                ije.getExtraData();

        case InvalidJadException.CA_DISABLED:
            return "The application can't be authorized because " +
                ije.getExtraData() + " is disabled.";

        case InvalidJadException.UNSUPPORTED_CHAR_ENCODING:
            return "Unsupported character encoding: " + ije.getExtraData();

        case InvalidJadException.REVOKED_CERT:
            return "The content provider certificate has been revoked.";

        case InvalidJadException.UNKNOWN_CERT_STATUS:
            return "The content provider certificate status is unknown.";
        }

        return ije.getMessage();
    }
}
