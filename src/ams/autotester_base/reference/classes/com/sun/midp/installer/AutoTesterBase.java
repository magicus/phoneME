/*
 *
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

package com.sun.midp.installer;

import java.io.IOException;

import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.MIDlet;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.security.Permissions;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Installs/Updates a test suite, runs the first MIDlet in the suite in a loop
 * until the new version of the suite is not found, then removes the suite.
 * <p>
 * The MIDlet uses these application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: URL for the test suite
 *   <li>arg-1: Used to override the default domain used when installing
 *    an unsigned suite. The default is maximum to allow the runtime API tests
 *    be performed automatically without tester interaction. The domain name
 *    may be followed by a colon and a list of permissions that must be allowed
 *    even if they are not listed in the MIDlet-Permissions attribute in the
 *    application descriptor file. Instead of the list a keyword "all" can be
 *    specified indicating that all permissions must be allowed, for example:
 *    operator:all.
 * </ol>
 * <p>
 * If arg-0 is not given then a form will be used to query the tester for
 * the arguments.</p>
 */
class AutoTesterBase extends MIDlet implements CommandListener,
    Runnable {

    /** Standard timeout for alerts. */
    static final int ALERT_TIMEOUT = 5000;
    /** Contains the default URL. */
    static final String defaultUrl = "http://";

    /** Display for this MIDlet. */
    Display display;
    /** Parameter form if there is not URL parameter given. */
    Form parameterForm;
    /** Contains the URL the user typed in. */
    TextField urlTextField;
    /** Contains the domain the user typed in. */
    TextField domainTextField;
    /** Command object for "Exit" command in the URL screen. */
    Command endCmd = new Command(Resource.getString
                                         (ResourceConstants.EXIT),
                                         Command.EXIT, 1);
    /** Command object for URL screen start testing. */
    Command testCmd =
        new Command(Resource.getString(ResourceConstants.GO),
                    Command.SCREEN, 1);
    /** URL of the test suite. */
    String url;
    /** Security domain to assign to unsigned suites. */
    String domain = Permissions.UNIDENTIFIED_DOMAIN_BINDING;
    /** MIDlet suite storage object. */
    MIDletSuiteStorage midletSuiteStorage;
    /** The installer. */
    Installer installer;
    /** How many iterations to run the suite */
    int loopCount = -1;

    /** The InstallListener to use when creating the Installer. */
    protected InstallListener installListener;

    /**
     * Create and initialize a new auto tester MIDlet.
     */
    AutoTesterBase() {
        display = Display.getDisplay(this);

        // The arg-<n> properties are generic command arguments
        url = getAppProperty("arg-0");
        if (url != null) {
            // URL given as a argument, look for a domain arg and then start
            String arg1 = getAppProperty("arg-1");

            boolean hasLoopCount = false;
            if (arg1 != null) {
                // this can be domain or loop count
                try {
                    loopCount = Integer.parseInt(arg1);
                    hasLoopCount = true;
                } catch (NumberFormatException e) {
                    // then its domain
                    domain = arg1;
                }

                if (!hasLoopCount) {
                    String arg2 = getAppProperty("arg-2");
                    if (arg2 != null) {
                        try {
                            loopCount = Integer.parseInt(arg2);
                        } catch (NumberFormatException e) {
                            // just ignore
                        }
                    }
                }
            }
        }
    }

    /**
     * Start.
     */
    public void startApp() {
        // Avoid competing for foreground with Test MIDlet
        display.setCurrent(null);
        notifyPaused();
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

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == testCmd) {
            getURLTextAndTest();
        } else if (c == endCmd || c == Alert.DISMISS_COMMAND) {
            // goto back to the manager midlet
            notifyDestroyed();
        }
    }

    /**
     * Ask the user for the URL.
     */
    void getUrl() {
        try {
            parameterForm = new
                Form(Resource.getString
                     (ResourceConstants.AMS_AUTO_TESTER_TESTSUITE_PARAM));

            urlTextField = new TextField
                (Resource.getString(ResourceConstants.AMS_AUTO_TESTER_URL),
                              defaultUrl, 1024, TextField.ANY);
            urlTextField.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
            parameterForm.append(urlTextField);

            domainTextField =
                new TextField(Resource.getString(ResourceConstants.
                              AMS_AUTO_TESTER_UNSIGN_SECURITY_DOMAIN),
                              domain, 1024, TextField.ANY);
            domainTextField.setLayout(Item.LAYOUT_NEWLINE_AFTER |
                                      Item.LAYOUT_2);
            parameterForm.append(domainTextField);

            parameterForm.addCommand(endCmd);
            parameterForm.addCommand(testCmd);
            parameterForm.setCommandListener(this);

            display.setCurrent(parameterForm);
        } catch (Exception ex) {
            displayException(Resource.getString(ResourceConstants.EXCEPTION),
                             ex.toString());
        }
    }

    /**
     * Save the URL setting the user entered in to the urlTextBox.
     */
    void getURLTextAndTest() {
        url = urlTextField.getString();

        if (url == null || url.length() == 0) {
            Alert a = new Alert(Resource.getString(ResourceConstants.ERROR),
                                Resource.getString(ResourceConstants.
                                       AMS_AUTO_TESTER_ERROR_URL_MSG),
                                   null, AlertType.ERROR);
            a.setTimeout(ALERT_TIMEOUT);
            display.setCurrent(a, parameterForm);
            return;
        }

        domain = domainTextField.getString();

        if (domain == null || domain.length() == 0) {
            Alert a = new Alert(Resource.getString(ResourceConstants.ERROR),
                                Resource.getString(ResourceConstants.
                                    AMS_AUTO_TESTER_ERROR_SECURITY_DOMAIN_MSG),
                                null, AlertType.ERROR);
            a.setTimeout(ALERT_TIMEOUT);
            display.setCurrent(a, parameterForm);
            return;
        }

        startBackgroundTester();
    }

    /**
     * Start the background tester.
     */
    void startBackgroundTester() {
        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        installer = new HttpInstaller();
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

    /** Run the installer. */
    public void run() {
    }

    /**
     * Handles an installer exceptions.
     *
     * @param suiteId ID of the suite being installed, can be null
     * @param ex exception to handle
     */
    void handleInstallerException(int suiteId, Throwable ex) {
        String message = null;

        if (ex instanceof InvalidJadException) {
            InvalidJadException ije = (InvalidJadException)ex;

            /*
             * The server will signal the end of testing with not found
             * status. However print out the JAD not found error if this
             * is the first download. (suiteId == null)
             */
            int reason = ije.getReason();
            if ((reason != InvalidJadException.JAD_NOT_FOUND &&
                reason != InvalidJadException.JAD_SERVER_NOT_FOUND) ||
                    suiteId == MIDletSuite.UNUSED_SUITE_ID) {
                message = "** Error installing suite (" + reason + "): " +
                          messageForInvalidJadException(ije);
            }
        } else if (ex instanceof IOException) {
            message = "** I/O Error installing suite: " + ex.getMessage();
        } else {
            message = "** Error installing suite: " + ex.toString();
        }

        if (message != null) {
            displayException(Resource.getString(ResourceConstants.ERROR),
                             message);

            long start = System.currentTimeMillis();
            long time_left = ALERT_TIMEOUT;

            while (time_left > 0) {
                try {
                    Thread.sleep(time_left);
                    time_left = 0;
                } catch (InterruptedException ie) {
                    long tmp = System.currentTimeMillis();
                    time_left -= (tmp - start);
                    start = tmp;
                }
            }
        }
    }

    /**
     * Display an exception to the user, with a done command.
     *
     * @param title exception form's title
     * @param message exception message
     */
    void displayException(String title, String message) {
        Alert a = new Alert(title, message, null, AlertType.ERROR);

        // This application must log always.
        Logging.report(Logging.CRITICAL, LogChannels.LC_CORE, message);
        a.setTimeout(ALERT_TIMEOUT);
        a.setCommandListener(this);

        display.setCurrent(a);
    }

    /**
     * Returns the class name of the first MIDlet of the newly installed
     * suite.
     *
     * @param suiteId ID of the MIDlet Suite
     * @param midletSuiteStorage MIDlet suite storage to look up properties
     *
     * @return an object with the class name and display name of
     * the suite's MIDlet-1 property
     */
    static MIDletInfo getFirstMIDletOfSuite(int suiteId,
            MIDletSuiteStorage midletSuiteStorage) {
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
        }

        return ije.getMessage();
    }
}
