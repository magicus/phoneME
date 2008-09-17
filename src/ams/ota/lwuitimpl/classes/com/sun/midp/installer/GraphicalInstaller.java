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

import java.io.*;

import javax.microedition.io.*;
import javax.microedition.midlet.*;
import javax.microedition.rms.*;

import com.sun.j2me.security.AccessController;
import com.sun.lwuit.*;
import com.sun.lwuit.TextArea;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.geom.Dimension;
import com.sun.lwuit.layouts.*;
import com.sun.lwuit.layouts.FlowLayout;
import com.sun.lwuit.plaf.Style;
import com.sun.lwuit.plaf.UIManager;
import com.sun.lwuit.util.Resources;
import com.sun.midp.appmanager.AppManagerUIImpl;	/* for convertImage */
import com.sun.midp.configurator.Constants;
import com.sun.midp.content.CHManager;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.io.j2me.storage.*;
import com.sun.midp.io.j2me.storage.File;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;
import com.sun.midp.main.TrustedMIDletIcon;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midletsuite.*;
import com.sun.midp.security.*;
import com.sun.midp.util.ResourceHandler;

/**
 * The Graphical MIDlet suite installer.
 * <p>
 * The graphical installer is implements the installer requirements of the
 * MIDP OTA specification.</p>
 * <p>
 * If the Content Handler API (CHAPI) is present the GraphicalInstaller will
 * dequeue a single Invocation and install from the URL contained
 * in the request. If there is no Invocation present then the arguments below
 * will be used.
 * <p>
 * The MIDlet uses certain application properties as arguments: </p>
 * <ol>
 *   <li>arg-0: "U" for update "I" or anything else for install</li>
 *   <li>arg-1: Suite ID for updating, URL for installing
 *   <li>arg-2: For installing a name to put in the title bar when installing
 * </ol>
 * @see CHManagerImpl
 * @see CHManager
 */
public class GraphicalInstaller extends MIDlet implements ActionListener {

    /** Standard timeout for alerts. */
    public static final int ALERT_TIMEOUT = 1250;
    /** settings database */
    public static final String SETTINGS_STORE = "settings";
    /** record id of selected midlet */
    public static final int URL_RECORD_ID = 1;
    /** record is of the last installed midlet */
    public static final int SELECTED_MIDLET_RECORD_ID = 2;

    /** The installer that is being used to install or update a suite. */
    private Installer installer;
    /** Display for this MIDlet. */
    private Display display;
    /** Form obtain a password and a username. */
    private Form passwordForm;
    /** Contains the username for installing. */
    private TextField usernameField;
    /** Contains the password for installing. */
    private TextField passwordField;
    /** Background installer that holds state for the current install. */
    private BackgroundInstaller backgroundInstaller;
    /** Keeps track of when the display last changed, in milliseconds. */
    private long lastDisplayChange;
    /** What to display to the user when the current action is cancelled. */
    private String cancelledMessage;
    /** Displays a list of storages to install to. */
    private List storageListBox;
    /** Contains storageListBox */
    private Form storageListForm;
    /** ID of the storage where the new midlet suite will be installed. */
    private int storageId = Constants.INTERNAL_STORAGE_ID;

    /* label at the progress form */
    private Label urlLabel;

    /** Content handler specific install functions. */
    CHManager chmanager;

    /** Command object for "Stop" command for progress form. */
    private Command stopCmd = new Command(Resource.getString
                                          (ResourceConstants.STOP),
                                          ResourceConstants.STOP);
    /** Command object for "Cancel" command for the confirm form. */
    private Command cancelCmd =
        new Command(Resource.getString(ResourceConstants.CANCEL),
                    ResourceConstants.CANCEL);
    /** Command object for "Exit" command*/
    private Command backCmd =
	new Command(Resource.getString(ResourceConstants.BACK),
		    ResourceConstants.EXIT);
    /** Command object for "Install" command for the confirm download form. */
    private Command continueCmd =
        new Command(Resource.getString(ResourceConstants.INSTALL),
                    ResourceConstants.INSTALL);
    /** Command object for "Next" command for storage select list. */
    private Command storeSelectCmd =
            new Command(Resource.getString(ResourceConstants.SELECT),
                        ResourceConstants.SELECT);
    /** Command object for "Next" command for password form. */
    private Command nextCmd =
        new Command(Resource.getString(ResourceConstants.NEXT),
                    ResourceConstants.NEXT);
    /** Command object for "continue" command for warning form. */
    private Command okCmd =
        new Command(Resource.getString(ResourceConstants.OK),
                    ResourceConstants.OK);
    /** Command object for "Yes" command for keep RMS form. */
    private Command keepRMSCmd =
        new Command(Resource.getString(ResourceConstants.YES),
                    ResourceConstants.YES);
    /** Command object for "No" command for keep RMS form. */
    private Command removeRMSCmd =
        new Command(Resource.getString(ResourceConstants.NO),
                    ResourceConstants.NO);

    /* Suite name */
    private String label;
    /* Url to install from */
    private String url;
    /* true if update should be forced without user confirmation */
    private boolean forceUpdate = false;
    /* true if user confirmation should be presented */
    private boolean noConfirmation = false;

    private Transition dialogTransition;

    /* transition speed */
    private final int runSpeed = 1000;

    /**
     * Gets an image from the internal storage.
     * <p>
     * Method requires com.sun.midp.ams permission.
     *
     * IMPL_NOTE: this method should be moved somewhere.
     *
     * @param imageName image file name without a path and extension
     * @return Image loaded from storage, or null if not found
     */
    public static Image
	getLwuitImageFromInternalStorage(String imageName) {
        byte[] imageBytes =
                ResourceHandler.getSystemImageResource(null, imageName);

        if (imageBytes != null) {
            return Image.createImage(imageBytes, 0, imageBytes.length);
        }

        return null;
    }

    public static javax.microedition.lcdui.Image
	getImageFromInternalStorage(String imageName) {
	System.out.println("GraphicalInstaller.java.getImageFromInternalStorage():  enter");
	byte[] imageBytes =
		ResourceHandler.getSystemImageResource(null, imageName);

	if (imageBytes != null) {
	    System.out.println("GraphicalInstaller.java.getImageFromInternalStorage():  calling createImage().  "+
			       "imageBytes.length is " + imageBytes.length);
	    return javax.microedition.lcdui.Image.createImage(
		imageBytes, 0, imageBytes.length);
	}

	System.out.println("GraphicalInstaller.java.getImageFromInternalStorage():  returning null");
	return null;
    }


    /**
     * Translate an InvalidJadException into a message for the user.
     *
     * @param exception exception to translate
     * @param name name of the MIDlet suite to insert into the message
     * @param vendor vendor of the MIDlet suite to insert into the message,
     *        can be null
     * @param version version of the MIDlet suite to insert into the message,
     *        can be null
     * @param jadUrl URL of a JAD, can be null
     *
     * @return message to display to the user
     */
    private static String translateJadException(
            InvalidJadException exception, String name, String vendor,
            String version, String jadUrl) {
	System.out.println("GraphicalInstaller.translateJadException():  enter.  exception.getReason() is " + exception.getReason());
        String[] values = {name, vendor, version, jadUrl,
                           exception.getExtraData()};
        int key;

        switch (exception.getReason()) {
        case InvalidJadException.OLD_VERSION:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_OLD_VERSION;
            break;

        case InvalidJadException.ALREADY_INSTALLED:
            key =
                ResourceConstants.
                  AMS_GRA_INTLR_INVALIDJADEXCEPTION_ALREADY_INSTALLED;
            break;

        case InvalidJadException.NEW_VERSION:
            key = ResourceConstants.
                      AMS_GRA_INTLR_INVALIDJADEXCEPTION_NEW_VERSION;
            break;

        case InvalidJadException.JAD_SERVER_NOT_FOUND:
        case InvalidJadException.JAD_NOT_FOUND:
        case InvalidJadException.INVALID_JAD_URL:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_JAD_URL;
            break;

        case InvalidJadException.INVALID_JAD_TYPE:
            key = ResourceConstants.
                 AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_JAD_TYPE;
            break;

        case InvalidJadException.MISSING_PROVIDER_CERT:
        case InvalidJadException.MISSING_SUITE_NAME:
        case InvalidJadException.MISSING_VENDOR:
        case InvalidJadException.MISSING_VERSION:
        case InvalidJadException.MISSING_JAR_URL:
        case InvalidJadException.MISSING_JAR_SIZE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_MISSING_JAD_INFO;
            break;

        case InvalidJadException.MISSING_CONFIGURATION:
        case InvalidJadException.MISSING_PROFILE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_MISSING_JAR_INFO;
            break;

        case InvalidJadException.INVALID_KEY:
        case InvalidJadException.INVALID_VALUE:
        case InvalidJadException.INVALID_VERSION:
        case InvalidJadException.PUSH_FORMAT_FAILURE:
        case InvalidJadException.PUSH_CLASS_FAILURE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_FORMAT;
            break;

        case InvalidJadException.DEVICE_INCOMPATIBLE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_DEVICE_INCOMPATIBLE;
            break;

        case InvalidJadException.JAD_MOVED:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_JAD_MOVED;
            break;

        case InvalidJadException.INSUFFICIENT_STORAGE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INSUFFICIENT_STORAGE;
            break;

        case InvalidJadException.JAR_SERVER_NOT_FOUND:
        case InvalidJadException.JAR_NOT_FOUND:
        case InvalidJadException.INVALID_JAR_URL:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_JAR_NOT_FOUND;
            break;

        case InvalidJadException.INVALID_JAR_TYPE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_JAR_TYPE;
            break;

        case InvalidJadException.SUITE_NAME_MISMATCH:
        case InvalidJadException.VERSION_MISMATCH:
        case InvalidJadException.VENDOR_MISMATCH:
        case InvalidJadException.JAR_SIZE_MISMATCH:
        case InvalidJadException.ATTRIBUTE_MISMATCH:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_ATTRIBUTE_MISMATCH;
            break;

        case InvalidJadException.CORRUPT_JAR:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_CORRUPT_JAR;
            break;

        case InvalidJadException.CANNOT_AUTH:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_CANNOT_AUTH;
            break;

        case InvalidJadException.CORRUPT_PROVIDER_CERT:
        case InvalidJadException.INVALID_PROVIDER_CERT:
        case InvalidJadException.CORRUPT_SIGNATURE:
        case InvalidJadException.INVALID_SIGNATURE:
        case InvalidJadException.UNSUPPORTED_CERT:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_SIGNATURE;
            break;

        case InvalidJadException.UNKNOWN_CA:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_UNKNOWN_CA;
            break;

        case InvalidJadException.EXPIRED_PROVIDER_CERT:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_EXPIRED_PROVIDER_CERT;
            break;

        case InvalidJadException.EXPIRED_CA_KEY:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_EXPIRED_CA_KEY;
            break;

        case InvalidJadException.AUTHORIZATION_FAILURE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_AUTHORIZATION_FAILURE;
            break;

        case InvalidJadException.CA_DISABLED:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_CA_DISABLED;
            break;

        case InvalidJadException.PUSH_DUP_FAILURE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_PUSH_DUP_FAILURE;
            break;

        case InvalidJadException.PUSH_PROTO_FAILURE:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_PUSH_PROTO_FAILURE;
            break;

        case InvalidJadException.TRUSTED_OVERWRITE_FAILURE:
            if (exception.getExtraData() != null) {
                key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_TRUSTED_OVERWRITE_FAILURE;
            } else {
                key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_TRUSTED_OVERWRITE_FAILURE_2;
            }

            break;

        case InvalidJadException.TOO_MANY_PROPS:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_APP_TOO_BIG;
            break;

        case InvalidJadException.INVALID_CONTENT_HANDLER:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_INVALID_CONTENT_HANDLER;
            break;

        case InvalidJadException.CONTENT_HANDLER_CONFLICT:
            key = ResourceConstants.
            AMS_GRA_INTLR_INVALIDJADEXCEPTION_CONTENT_HANDLER_CONFLICT;
            break;

        case InvalidJadException.JAR_CLASSES_VERIFICATION_FAILED:
            // This constant is shared between graphical installer
            // and standalone class verifier MIDlet used for SVM mode
            key = ResourceConstants.AMS_CLASS_VERIFIER_FAILURE;
            break;

        case InvalidJadException.UNSUPPORTED_CHAR_ENCODING:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_UNSUPPORTED_CHAR_ENCODING;
            break;

        case InvalidJadException.REVOKED_CERT:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_REVOKED_PROVIDER_CERT;
            break;

        case InvalidJadException.UNKNOWN_CERT_STATUS:
            key = ResourceConstants.
                AMS_GRA_INTLR_INVALIDJADEXCEPTION_UNKNOWN_PROVIDER_CERT_STATUS;
            break;

        default:
            return exception.getMessage();
        }

        return Resource.getString(key, values);
    }

    /**
     * Create and initialize a new graphical installer MIDlet.
     * <p>
     * If a ContentHandler request to install a suite is found,
     * then that URL will be installed.  In this case the command
     * arguments are ignored.
     * <p>
     * The Display is retrieved and the list of MIDlet will be retrieved or
     * update a currently installed suite.
     */
    public GraphicalInstaller() {
	System.out.println(">>>GraphicalInstaller.GraphicalInstaller()");
	String arg0;

        installer = new HttpInstaller();
        initSettings();

	dialogTransition = CommonTransitions.createSlide(
	    CommonTransitions.SLIDE_VERTICAL, true, runSpeed);

        // Establish Content handler installer context
        chmanager = CHManager.getManager(null);

        // Get the URL, if any, provided from the invocation mechanism.
        url = chmanager.getInstallURL(this);
        if (url != null) {
            label = Resource.getString(ResourceConstants.APPLICATION);
            forceUpdate = false;
            noConfirmation = false;
        } else {
            arg0 = getAppProperty("arg-0");
            if (arg0 == null) {
                // goto back to the discovery midlet
                exit(false);
                return;
            }

            if ("U".equals(arg0)) {
                String strSuiteID = getAppProperty("arg-1");
                int suiteId = MIDletSuite.UNUSED_SUITE_ID;

                if (strSuiteID != null) {
                  try {
                      suiteId = Integer.parseInt(strSuiteID);
                  } catch (NumberFormatException nfe) {
                      // Intentionally ignored
                  }
                }

                if (suiteId == MIDletSuite.UNUSED_SUITE_ID) {
                    // goto back to the discovery midlet
                    exit(false);
                    return;
                }

                updateSuite(suiteId);
                return;
            } else if("FI".equals(arg0)) {
                /* force installation without user confirmation */
                noConfirmation = true;
                /* force installation without user confirmation and force update */
                forceUpdate = false;
            } else if("FU".equals(arg0)) {
                /* force installation without user confirmation */
                noConfirmation = true;
                /* force installation without user confirmation and force update */
                forceUpdate = true;
            }

            url = getAppProperty("arg-1");
            if (url == null) {
                // goto back to the discovery midlet
                exit(false);
                return;
            }

            label = getAppProperty("arg-2");
            if (label == null || label.length() == 0) {
                label = Resource.getString(ResourceConstants.APPLICATION);
            }
        }

        cancelledMessage =
            Resource.getString(ResourceConstants.AMS_GRA_INTLR_INST_CAN);
	storageListForm = new Form(Resource.getString(
                ResourceConstants.AMS_GRA_INTLR_SELECT_STORAGE));
	storageListForm.setLayout(new FlowLayout());
	storageListForm.addComponent(new Label(Resource.getString(
                ResourceConstants.AMS_INTERNAL_STORAGE_NAME)));
	storageListBox = new List();

        String storagePrefix = Resource.getString(ResourceConstants.AMS_EXTRENAL_STORAGE_NAME);

        int validStorageCnt = Constants.MAX_STORAGE_NUM;
        for (int i = 1; i < Constants.MAX_STORAGE_NUM; i++) {
            /* IMPL_NOTE: here we should check if storage is accessible and
               update validStorageCnt accordingly */
	    storageListBox.addItem(new Label(storagePrefix + i));
        }

        /*
         * if VERIFY_ONCE is enabled then MONET is disabled
         * so we don't have to check MONET_ENABLED.
         */
        if (Constants.VERIFY_ONCE && (validStorageCnt > 1)) {
            // select storage
            storageListForm.addCommand(cancelCmd);
            storageListForm.addCommand(storeSelectCmd);
            //storageListForm.setSelectCommand(storeSelectCmd);
            storageListForm.setCommandListener(this);
            storageListForm.show();
        } else {
            // use default storage
            installSuite(label, url, storageId, forceUpdate, noConfirmation);
        }
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
        if (installer != null) {
            installer.stopInstalling();
        }

        /* The backgroundInstaller could be waiting for the user. */
        cancelBackgroundInstall();
    }

    /**
     * Exit the GraphicalInstaller with the status supplied.
     * It will perform any remaining cleanup and call notifyDestroyed.
     * @param success <code>true</code> if the install was a success,
     *  <code>false</code> otherwise.
     */
    void exit(boolean success) {
	System.out.println("GraphicalInstaller.exit(): enter. success is " + success);
        chmanager.installDone(success);
        notifyDestroyed();
    }


    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();
	System.out.println("GraphicalInstaller.actionPerformed():  enter.  cmd.getId() is " + cmd.getId());

	switch (cmd.getId()) {
	case ResourceConstants.NEXT:
	    // the user has entered a username and password
	    System.out.println("GraphicalInstaller.actionPerformed():  enter.  cmd.getId() is " + cmd.getId());
	    resumeInstallWithPassword();
	    break;
	case ResourceConstants.SELECT:
	    storageId = this.storageListBox.getSelectedIndex();
	    installSuite(label, url, storageId, forceUpdate, noConfirmation);
	    break;
	case ResourceConstants.OK:

	    break;
	case ResourceConstants.INSTALL:
//	    startJarDownload();
	    break;
	case ResourceConstants.STOP:
	    if (installer != null) {
		/*
		 * BackgroundInstaller may be displaying
		 * the "Finishing" message
		 *
		 * also we need to prevent the BackgroundInstaller from
		 * re-displaying the list before the cancelled message is
		 * displayed
		 */
		synchronized (this) {
		    if (installer.stopInstalling()) {
			Dialog.show(Resource.getString(     	//title
					ResourceConstants.INFO),
					cancelledMessage,	//text
					new Command[]{okCmd},	//commands
					Dialog.TYPE_INFO,	//type
					null,			//icon
					0,			//infinite timeout,
					dialogTransition);	//transition
			exit(false);
		    }
		}
	    } else {
		// goto back to the manager midlet
		exit(false);
	    }
	    break;
	case ResourceConstants.CANCEL:

	    break;
	case ResourceConstants.BACK:
	    // goto back to the manager midlet
	    exit(false);
	    break;
	}
    }


    /**
     * Initialize the settings database if it doesn't exist. This may create
     * two entries. The first will be for the download url, the second will
     * be for storing the storagename of the currently selected midlet
     * <p>
     * Method requires com.sun.midp.ams permission.
     */
    public static void initSettings() {
	System.out.println("GraphicalInstaller.initSettings():  enter");
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);

        try {
            RecordStore settings = RecordStore.
                                   openRecordStore(SETTINGS_STORE, true);

            try {
                if (settings.getNumRecords() == 0) {
                    // space for a URL
                    settings.addRecord(null, 0, 0);

                    // space for current MIDlet Suite name
                    settings.addRecord(null, 0, 0);
                }
            } finally {
                settings.closeRecordStore();
            }

        } catch (Exception e) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                               "initSettings  throw an Exception");
            }
        }
    }

    /**
     * Save the settings the user entered.
     * <p>
     * Method requires com.sun.midp.ams permission.
     *
     * @param url the url to save
     * @param curMidlet suiteId of the currently selected midlet
     * @return the Exception that may have been thrown, or null
     */
    public static void saveSettings(String url, int curMidlet) {
	System.out.println("GraphicalInstaller.saveSettings():  enter");
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);

        try {
            String temp;
            ByteArrayOutputStream bas;
            DataOutputStream dos;
            byte[] data;
            RecordStore settings;

            bas = new ByteArrayOutputStream();
            dos = new DataOutputStream(bas);
            settings = RecordStore.openRecordStore(SETTINGS_STORE, false);

            if (url != null) {
                dos.writeUTF(url);
                data = bas.toByteArray();
                settings.setRecord(URL_RECORD_ID, data, 0, data.length);
            }

            // Save the current midlet even if its id is
            // MIDletSuite.UNUSED_SUITE_ID. Otherwise in SVM mode
            // the last installed midlet will be always highlighted
            // because its id is recorded in this RMS record.
            bas.reset();

            dos.writeInt(curMidlet);
            data = bas.toByteArray();
            settings.setRecord(SELECTED_MIDLET_RECORD_ID,
                               data, 0, data.length);

            settings.closeRecordStore();
            dos.close();
        } catch (Exception e) {
	    e.printStackTrace();
	    //throw e;
        }
    }

    /**
     * Update a suite.
     *
     * @param id ID of the suite to update
     */
    private void updateSuite(int id) {
	System.out.println("GraphicalInstaller.updateSuite():  enter");
        MIDletSuiteImpl midletSuite = null;
        try {
            // Any runtime error will get caught by the installer
            midletSuite =
                MIDletSuiteStorage.getMIDletSuiteStorage()
                    .getMIDletSuite(id, false);
            InstallInfo installInfo = midletSuite.getInstallInfo();
            MIDletInfo midletInfo;
            String name;

            if (midletSuite.getNumberOfMIDlets() == 1) {
                midletInfo =
                    new MIDletInfo(midletSuite.getProperty("MIDlet-1"));
                name = midletInfo.name;
            } else {
                name = midletSuite.getProperty(MIDletSuite.SUITE_NAME_PROP);
            }

            cancelledMessage =
                Resource.getString(ResourceConstants.AMS_GRA_INTLR_UPD_CAN);

            installSuiteCommon(Resource.getString
                               (ResourceConstants.AMS_GRA_INTLR_UPDATING),
                               name,
                               installInfo.getDownloadUrl(),
                               MIDletSuiteStorage.getMidletSuiteStorageId(id),
                               name + Resource.getString
                               (ResourceConstants.AMS_GRA_INTLR_SUCC_UPDATED),
                               true, false);
        } catch (MIDletSuiteLockedException e) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                               "updateSuite threw MIDletSuiteLockedException");
            }
        } catch (MIDletSuiteCorruptedException e) {
            String msg = Resource.getString
                         (ResourceConstants.AMS_MIDLETSUITE_ID_CORRUPT_MSG)
                         + id;
	    Dialog.show(Resource.getString(     //title
		    ResourceConstants.ERROR),
		    msg,			//message
		    new Command[]{okCmd},//commands
		    Dialog.TYPE_ERROR,//type
		    null,//icon
		    0,	//timeout,
		    dialogTransition);//transition
        } finally {
            if (midletSuite != null) {
                midletSuite.close();
            }
        }
    }

    /**
     * Install a suite from URL.
     *
     * @param label label of the URL link
     * @param url HTTP/S URL of the suite to update
     * @param storageId id of the storage
     * @param forceUpdate no user confirmation for update
     * @param noConfirmation no user confirmation
     */
    private void installSuite(String label, String url, int storageId,
                              boolean forceUpdate, boolean noConfirmation) {
	System.out.println("GraphicalInstaller.installSuite():  enter");
        cancelledMessage =
            Resource.getString(ResourceConstants.AMS_GRA_INTLR_INST_CAN);

        installSuiteCommon(Resource.getString
                           (ResourceConstants.AMS_GRA_INTLR_INSTALLING),
                           label, url, storageId,
                           label + Resource.getString
                           (ResourceConstants.AMS_GRA_INTLR_SUCC_INSTALLED),
                           forceUpdate, noConfirmation);
    }

    /**
     * Common helper method to install or update a suite.
     *
     * @param action action to put in the form's title
     * @param name name to in the form's title
     * @param url URL of a JAD
     * @param storageId id of the storage
     * @param successMessage message to display to user upon success
     * @param updateFlag if true the current suite is being updated
     * @param noConfirmation no user confirmation
     */
    private void installSuiteCommon(String action, String name, String url, int storageId,
            String successMessage, boolean updateFlag, boolean noConfirmation) {
	System.out.println("GraphicalInstaller.installSuiteCommon():  enter");
        try {
            backgroundInstaller = new BackgroundInstaller(this, url, name, storageId,
                                      successMessage, updateFlag, noConfirmation);
            new Thread(backgroundInstaller).start();
        } catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(name);
            sb.append("\n");
            sb.append(Resource.getString(ResourceConstants.ERROR));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString
                             (ResourceConstants.AMS_CANT_ACCESS),
                             sb.toString());
        }
    }



    /** Cancel an install (if there is one) waiting for user input. */
    private void cancelBackgroundInstall() {
	System.out.println("GraphicalInstaller.cancelBackgroundInstall():  enter");
        if (backgroundInstaller != null) {
            backgroundInstaller.continueInstall = false;

            synchronized (backgroundInstaller) {
                backgroundInstaller.notify();
            }
        }
    }

    /**
     * Update the status form.
     *
     * @param status current status of the install.
     * @param state current state of the install.
     */
    private void updateStatus(int status, InstallState state) {
	System.out.println("GraphicalInstaller.updateStatus():  enter");

        if (status == Installer.DOWNLOADING_JAR) {

	    return;
        }

        if (status == Installer.DOWNLOADED_1K_OF_JAR &&
                state.getJarSize() > 0) {
            //progressGauge.setProgress(progressGauge.getProgress() + 0x01);
            return;
        }

        if (Constants.MONET_ENABLED) {
            if (status == Installer.GENERATING_APP_IMAGE) {

		return;
            }
        }

        if (Constants.VERIFY_ONCE) {
            if (status == Installer.VERIFYING_SUITE_CLASSES) {
                if (state.getLastException() != null) {

		    Dialog.show(Resource.getString(	//title
				ResourceConstants.AMS_GRA_INTLR_INSTALL_WARNING),
				Resource.getString(	//message
				ResourceConstants.AMS_CLASS_VERIFIER_FAILURE),
				new Command[]{okCmd},//commands
				Dialog.TYPE_ERROR,//type
				null,//icon
				0,   //infinite timeout,
				dialogTransition);//transition
                } else {
		    return;
                }
            }
        }

        if (status == Installer.VERIFYING_SUITE) {

	    return;
        }

        if (status == Installer.STORING_SUITE) {

	    return;
        }

        if (status == Installer.CORRUPTED_SUITE) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                               "Suite is corrupted");
            }
            return;
        }
    }



    /**
     * Give the user a chance to act on warning during an installation.
     *
     * @param name name of the MIDlet suite to insert into the message
     * @param vendor vendor of the MIDlet suite to insert into the message,
     *        can be null
     * @param version version of the MIDlet suite to insert into the message,
     *        can be null
     * @param jadUrl URL of a JAD, can be null
     * @param e last exception from the installer
     */
    private boolean warnUser(String name, String vendor, String version,
                          String jadUrl, InvalidJadException e) {
	System.out.println("GraphicalInstaller.warnUser():  enter");

	Command cmd = Dialog.show(Resource.getString(     //title
		ResourceConstants.WARNING),
		translateJadException(e, name, vendor, version, jadUrl),//message
		new Command[]{cancelCmd, okCmd},//commands
		Dialog.TYPE_WARNING,//type
		null,//icon
		0,//timeout,
		dialogTransition);//transition

	if (cmd == okCmd) {
	    return true;
	}
	return false;
    }


    /**
     * Ask for a username and password.
     */
    private void getUsernameAndPassword() {
	System.out.println("GraphicalInstaller.getUsernameAndPassword():  enter");
        getUsernameAndPasswordCommon("");
    }

    /**
     * Ask for proxy username and password.
     */
    private void getProxyUsernameAndPassword() {
	System.out.println("GraphicalInstaller.getProxyUsernameAndPassword():  enter");
        getUsernameAndPasswordCommon(
              Resource.getString(
		  ResourceConstants.AMS_GRA_INTLR_PASSWORD_FORM_FIREWALL_TITLE));
    }

    /**
     * Ask a username and password.
     *
     * @param title title of the password form
     */
    private void getUsernameAndPasswordCommon(String title) {
	System.out.println("GraphicalInstaller.getUsernameAndPasswordCommon():  enter");
	/* IMPL_NOTE:  To implement */
    }

    /**
     * Resume the install of the suite with a password and username.
     */
    private void resumeInstallWithPassword() {
        String username;
        String password;

	System.out.println("GraphicalInstaller.resumeInstallWithPassword():  enter");
        username = usernameField.getText();
        password = passwordField.getText();
        if (username == null || username.length() == 0) {
	    Dialog.show(Resource.getString(     //title
			    ResourceConstants.ERROR),
			Resource.getString(ResourceConstants.	//text
				      AMS_GRA_INTLR_ID_NOT_ENTERED),
			new Command[]{okCmd},//commands
			Dialog.TYPE_ERROR,//type
			null,//icon
			ALERT_TIMEOUT,//timeout,
			dialogTransition);//transition
	    passwordForm.show();
	    return;
	}

        if (password == null || password.length() == 0) {
	    Dialog.show(Resource.getString(     //title
		    ResourceConstants.ERROR),
		    Resource.getString(ResourceConstants.	//text
				      AMS_GRA_INTLR_PWD_NOT_ENTERED),
		    new Command[]{okCmd},//commands
		    Dialog.TYPE_ERROR,//type
		    null,//icon
		    ALERT_TIMEOUT,//timeout,
		    dialogTransition);//transition
	    passwordForm.show();
	    return;
        }

        // redisplay the progress form
        //progressForm.show();

        if (backgroundInstaller.proxyAuth) {
            backgroundInstaller.installState.setProxyUsername(username);
            backgroundInstaller.installState.setProxyPassword(password);
        } else {
            backgroundInstaller.installState.setUsername(username);
            backgroundInstaller.installState.setPassword(password);
        }

        backgroundInstaller.continueInstall = true;
        synchronized (backgroundInstaller) {
            backgroundInstaller.notify();
        }
    }

    /**
     * Confirm the JAR download with the user.
     *
     * @param state current state of the install.
     */
    private boolean displayDownloadConfirmation(InstallState state) {
	System.out.println("GraphicalInstaller.displayDownloadConfirmation():  enter");
        String suiteName;
	String confirmStr = new String();
        String[] values = new String[1];

        suiteName = state.getSuiteName();

        try {
            values[0] = suiteName;

	    confirmStr += Resource.getString
		      (ResourceConstants.AMS_GRA_INTLR_WANT_INSTALL,
		       values) + "\n";

	    /* add untrusted warning */
	    if (!installer.isJadSigned()) {
                // The MIDlet suite is not signed, therefore will be untrusted
		confirmStr += Resource.getString(ResourceConstants.WARNING) + ":" +
			    Resource.getString(ResourceConstants.AMS_GRA_INTLR_UNTRUSTED_WARN) + "\n";
            }

            /* get size */
	    confirmStr += Resource.getString(ResourceConstants.AMS_SIZE) +
		": " + state.getJarSize() + " K" + "\n";

	    /* get version */
	    confirmStr += Resource.getString(ResourceConstants.AMS_VERSION) + ": " +
		state.getAppProperty(MIDletSuite.VERSION_PROP) + "\n";

	    /* get vendor */
	    confirmStr += Resource.getString(ResourceConstants.AMS_VENDOR) + ": "  +
		state.getAppProperty(MIDletSuite.VENDOR_PROP) + "\n";

	    /* get description */
	    String desc = state.getAppProperty(MIDletSuite.DESC_PROP);
            if (desc != null) {
		confirmStr += Resource.getString
                             (ResourceConstants.AMS_DESCRIPTION) + ": " + desc + "\n";
            }

	    /* get website */
	    confirmStr += Resource.getString(ResourceConstants.AMS_WEBSITE) + ": " +
		state.getJarUrl() + "\n";

	    /* show dialog */
	    Command cmd =
		Dialog.show(Resource.getString(     //title
			    ResourceConstants.AMS_GRA_INTLR_INSTALL_WARNING),
			    confirmStr,			//component body
			    new Command[]{okCmd, cancelCmd},//commands
			    Dialog.TYPE_WARNING,	//type
			    null,//icon
			    0,//infinite timeout,
			    dialogTransition);//transition

	    if (okCmd == cmd) {
		System.out.println("GraphicalInstaller.displayDownloadConfirmation():  Ok command selected.");
		return true;
	    }
	    else {
		System.out.println("GraphicalInstaller.displayDownloadConfirmation():  Cancel command selected.");
		return false;
	    }

        } catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(suiteName);
            sb.append("\n");
            sb.append(Resource.getString(ResourceConstants.EXCEPTION));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString
                             (ResourceConstants.AMS_CANT_ACCESS),
                             sb.toString());
	    return false;
        }
    }

    /**
     * Ask the user during an update if they want to keep the old RMS data.
     *
     * @param state current state of the install.
     */
    private boolean displayKeepRMSForm(InstallState state) {
	System.out.println("GraphicalInstaller.displayKeepRMSForm():  enter");
        Container c = new Container();
        String name;
        StringBuffer value = new StringBuffer(40);
        String[] values = new String[1];

        name = state.getAppProperty(MIDletSuite.SUITE_NAME_PROP);

        try {
            values[0] = name;
            value.append(Resource.getString
                         (ResourceConstants.AMS_GRA_INTLR_NEW_OLD_VERSION,
                          values));
            c.addComponent(new Label(value.toString()));

	    Command cmd = Dialog.show(Resource.getString(     //title
			ResourceConstants.AMS_CONFIRMATION),
			c,			//component body
			new Command[]{keepRMSCmd, removeRMSCmd},//commands
			Dialog.TYPE_CONFIRMATION,	//type
			null,//icon
			0,//infinite timeout,
			dialogTransition);//transition

	    if (cmd == keepRMSCmd) {
		backgroundInstaller.continueInstall = true;
	    }
	    else {
		backgroundInstaller.continueInstall = false;
	    }

	    synchronized (backgroundInstaller) {
		backgroundInstaller.notify();
	    }
	    return backgroundInstaller.continueInstall;


        } catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(name);
            sb.append("\n");
            sb.append(Resource.getString(ResourceConstants.EXCEPTION));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString
                             (ResourceConstants.AMS_CANT_ACCESS),
                             sb.toString());
	    return false;
	}
    }

    /**
     * Confirm the authorization path with the user.
     *
     * @param state current state of the install.
     */
    private boolean displayAuthPathConfirmation(InstallState state) {
	System.out.println("GraphicalInstaller.displayAuthPathConfirmation():  enter");
        Container c = new Container();
        String name;
        String values[] = new String[1];
        Label item;
        String authPath[];
        String temp;
        StringBuffer label = new StringBuffer(40);

        name = state.getAppProperty(MIDletSuite.SUITE_NAME_PROP);

        try {
	    Image icon = AppManagerUIImpl.convertImage(
		TrustedMIDletIcon.getIcon());

	    Label l = new Label(icon);

            values[0] = name;
            label.setLength(0);
            label.append(Resource.getString(
                ResourceConstants.AMS_GRA_INTLR_TRUSTED, values));
            label.append(": ");

            authPath = state.getAuthPath();
            temp = label.toString();
            for (int i = 0; i < authPath.length; i++) {
                item = new Label(temp + authPath[i]);
                c.addComponent(item);
                temp = " -> ";
            }

	    Command cmd = Dialog.show(Resource.getString(     //title
			ResourceConstants.AMS_CONFIRMATION),
			c,	//component body
			new Command[]{continueCmd, cancelCmd},//commands
			Dialog.TYPE_CONFIRMATION,	//type
			null,//icon
			0,//infinite timeout,
			dialogTransition);//transition

	    if (cmd == continueCmd) {
		System.out.println("GraphicalInstaller.displayAuthPathConfirmation():  continueCmd command chosen.");
		startJarDownload();
		return true;
	    }
	    else {
		System.out.println("GraphicalInstaller.displayAuthPathConfirmation():  cancelCmd command chosen.");
		return false;
	    }


        } catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(Resource.getString(ResourceConstants.EXCEPTION));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString
                             (ResourceConstants.AMS_CANT_ACCESS),
                             sb.toString());
	    return false;
        }
    }

    /**
     * Confirm redirection with the user.
     *
     * @param state current state of the install.
     * @param newLocation new url of the resource to install.
     */
    private boolean displayRedirectConfirmation(InstallState state,
                                             String newLocation) {
	System.out.println("GraphicalInstaller.displayRedirectConfirmation():  enter");
	Container c = new Container();
        StringBuffer value = new StringBuffer(40);
        String[] values = new String[1];

        try {

	    values[0] = newLocation;
            value.append(Resource.getString(
                             ResourceConstants.AMS_GRA_INTLR_CONFIRM_REDIRECT,
                                 values));
            c.addComponent(new Label(value.toString()));

	    Command cmd = Dialog.show(Resource.getString(     //title
			ResourceConstants.AMS_CONFIRMATION),
			c,	//component body
			new Command[]{continueCmd, cancelCmd},//commands
			Dialog.TYPE_CONFIRMATION,	//type
			null,//icon
			0,//infinite timeout,
			dialogTransition);//transition

	    if (cmd == continueCmd) {
		System.out.println("GraphicalInstaller.displayRedirectConfirmation():  continueCmd command chosen.");
		startJarDownload();
		return true;
	    }
	    else {
		System.out.println("GraphicalInstaller.displayRedirectConfirmation():  cancelCmd command chosen.");
		return false;
	    }
        } catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(Resource.getString(ResourceConstants.EXCEPTION));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString(
                                 ResourceConstants.AMS_CANT_ACCESS),
                                     sb.toString());
	    return false;
        }
    }

    /**
     * Resume the install to start the JAR download.
     */
    private void startJarDownload() {
	System.out.println("GraphicalInstaller.startJarDownload():  enter");

        backgroundInstaller.continueInstall = true;
        synchronized (backgroundInstaller) {
            backgroundInstaller.notify();
        }
    }

    /** Confirm the JAR only download with the user. */
    private boolean displayJarOnlyDownloadConfirmation() {
	System.out.println("GraphicalInstaller.displayJarOnlyDownloadConfirmation():  enter");
        Container c = new Container();
        Label item;
        StringBuffer label = new StringBuffer(40);
        StringBuffer value = new StringBuffer(40);
        String[] values = new String[1];

        try {
            values[0] = backgroundInstaller.name;
            item = new Label(Resource.getString(
                                  ResourceConstants.AMS_GRA_INTLR_WANT_INSTALL) +
                                  values);
            c.addComponent(item);

            label.append(Resource.getString(ResourceConstants.AMS_WEBSITE));
            label.append(": ");
            item = new Label(label.toString() + backgroundInstaller.url);
            c.addComponent(item);

            value.append(" \n");
            value.append(Resource.getString
                         (ResourceConstants.AMS_GRA_INTLR_NO_INFO));
            c.addComponent(new Label(value.toString()));

	    Command cmd = Dialog.show(Resource.getString(     //title
			ResourceConstants.AMS_CONFIRMATION),
			c,	//component body
			new Command[]{continueCmd, cancelCmd},//commands
			Dialog.TYPE_CONFIRMATION,	//type
			null,//icon
			0,//infinite timeout,
			dialogTransition);//transition

	    if (cmd == continueCmd) {
		System.out.println("GraphicalInstaller.displayJarOnlyDownloadConfirmation():  continueCmd command chosen.");
		startJarDownload();
		return true;
	    }
	    else {
		System.out.println("GraphicalInstaller.displayJarOnlyDownloadConfirmation():  cancelCmd command chosen.");
		return false;
	    }

	} catch (Exception ex) {
            StringBuffer sb = new StringBuffer();

            sb.append(backgroundInstaller.name);
            sb.append("\n");
            sb.append(Resource.getString(ResourceConstants.EXCEPTION));
            sb.append(": ");
            sb.append(ex.toString());
            displayException(Resource.getString
                             (ResourceConstants.AMS_CANT_ACCESS),
                             sb.toString());
	    return false;
        }
    }


    /**
     * Alert the user that an action was successful.
     *
     * @param successMessage message to display to user
     */
    private void displaySuccessMessage(String successMessage) {
	System.out.println("GraphicalInstaller.displaySuccessMessage():  enter");
        /* TODO:  refactor */
	Image icon = getLwuitImageFromInternalStorage("_dukeok8");

        lastDisplayChange = System.currentTimeMillis();

	Dialog.show(Resource.getString(     	//title
			ResourceConstants.INFO),
			successMessage,		//text
			new Command[]{okCmd},	//commands
			Dialog.TYPE_INFO,	//type
			null,			//icon
			ALERT_TIMEOUT,		//timeout,
			dialogTransition);	//transition
    }


    /**
     * Display an exception to the user, with a done command.
     *
     * @param title exception form's title
     * @param message exception message
     */
    private void displayException(String title, String message) {
	System.out.println("GraphicalInstaller.displayException():  enter");

	Dialog dlg = new Dialog(title);
	dlg.setScrollable(false);
	dlg.setLayout(new BorderLayout());
	dlg.addComponent(BorderLayout.CENTER, new TextArea(message));
	dlg.addCommand(okCmd);
	dlg.show();


//         Dialog.show(title,      //title
//                     message,    //message
//                     new Command[]{okCmd},//commands
//                     Dialog.TYPE_ERROR, //type
//                     null, //icon
//                     0,    //infinite timeout,
//                     dialogTransition);//transition
    }


    /** A class to install a suite in a background thread. */
    private class BackgroundInstaller implements Runnable, InstallListener {
        /** Parent installer. */
        private GraphicalInstaller parent;
        /** URL to install from. */
        private String url;
        /** Name of MIDlet suite. */
        private String name;
        /** ID of the storage where the new midlet suite will be installed. */
        private int storageId;

        /**
         * Message for the user after the current install completes
         * successfully.
         */
        private String successMessage;
        /** Flag to update the current suite. */
        private boolean update;
        /** Flag for user confiramtion. */
        private boolean noConfirmation;
        /** State of the install. */
        InstallState installState;
        /** Signals that the user wants the install to continue. */
        boolean continueInstall;
        /** Signals that the suite only has JAR, no JAD. */
        boolean jarOnly;
        /** Signals that a proxyAuth is needed. */
        boolean proxyAuth;

        /**
         * Construct a BackgroundInstaller.
         *
         * @param theParent parent installer of this object
         * @param theJadUrl where to get the JAD.
         * @param theName name of the MIDlet suite
         * @param theStorageId id of the storage to install to
         * @param theSuccessMessage message to display to user upon success
         * @param updateFlag if true the current suite should be
         *                      overwritten without asking the user.
         * @param noConfirmationFlag if true the current suite should be
         *                      installed without asking the user.
         */
        private BackgroundInstaller(GraphicalInstaller theParent,
                String theJadUrl, String theName, int theStorageId,
                String theSuccessMessage, boolean updateFlag,
                boolean noConfirmationFlag) {
	    System.out.println("BackgroundInstaller():  enter");
            parent = theParent;
            url = theJadUrl;
            name = theName;
            storageId = theStorageId;
            successMessage = theSuccessMessage;
            update = updateFlag;
            noConfirmation = noConfirmationFlag;
        }

        /**
         * Run the installer.
         */
        public void run() {
	    System.out.println("BackgroundInstaller.run():  enter");
	    // ID of the suite that was just installed
            int lastInstalledMIDletId = MIDletSuite.UNUSED_SUITE_ID;

            try {
                // a flag indicating that an attempt of installation must
                // be repeated, but now using the jar URL instead of jad
                boolean tryAgain;
                // title of the window displaying an error message
                String title;
                // an error message to display
                String msg;

                // repeat while(tryAgain)
                do {
                    tryAgain = false;
                    msg = null;

                    try {
                        if (jarOnly) {
                            lastInstalledMIDletId =
                                parent.installer.installJar(url, name,
                                    storageId, update, false, this);
                        } else {
                            lastInstalledMIDletId =
                                parent.installer.installJad(url, storageId,
                                    update, false, this);
                        }

                        // Let the manager know what suite was installed
                        GraphicalInstaller.saveSettings(null,
                                                        lastInstalledMIDletId);

			//IMPL_NOTE:  ENABLE BACK
			//parent.displaySuccessMessage(successMessage);


                        parent.exit(true);
                    } catch (InvalidJadException ije) {
                        int reason = ije.getReason();
                        if (reason == InvalidJadException.INVALID_JAD_TYPE) {
                            // media type of JAD was wrong, it could be a JAR
                            String mediaType = (String)ije.getExtraData();

                            if (Installer.JAR_MT_1.equals(mediaType) ||
                                    Installer.JAR_MT_2.equals(mediaType)) {
                                // re-run as a JAR only install
                                if (noConfirmation || confirmJarOnlyDownload()) {
                                    jarOnly = true;
                                    installState = null;
                                    tryAgain = true;
                                    continue;
                                }

                                displayListAfterCancelMessage();
                                break;
                            }
                        } else if
                            (reason == InvalidJadException.ALREADY_INSTALLED ||
                             reason == InvalidJadException.OLD_VERSION ||
                             reason == InvalidJadException.NEW_VERSION) {
                            // user has canceled the update operation,
                            // don't display an error message
                            break;
                        }

                        msg = translateJadException(ije, name, null, null, url);
                    } catch (MIDletSuiteLockedException msle) {
                        String[] values = new String[1];
                        values[0] = name;
                        if (!update) {
                            msg = Resource.getString(
                              ResourceConstants.AMS_DISC_APP_LOCKED, values);
                        } else {
                            msg = Resource.getString(
                              ResourceConstants.AMS_GRA_INTLR_LOCKED, values);
                        }
                    } catch (IOException ioe) {
                        if (parent.installer != null &&
                                parent.installer.wasStopped()) {
                            displayListAfterCancelMessage();
                            break;
                        } else {
                            msg = Resource.getString(
                                ResourceConstants.AMS_GRA_INTLR_CONN_DROPPED);
                        }
                    } catch (Throwable ex) {
                        if (Logging.TRACE_ENABLED) {
                            Logging.trace(ex, "Exception caught " +
                                "while installing");
                        }

                        msg = ex.getClass().getName() + ": " + ex.getMessage();
                    }

                } while (tryAgain);

                // display an error message, if any
                if (msg != null) {
                    title = Resource.getString(
                        ResourceConstants.AMS_GRA_INTLR_INSTALL_ERROR);
                    // go back to the app list
                    displayException(title, msg);
                }
            } finally {
                if (lastInstalledMIDletId == MIDletSuite.UNUSED_SUITE_ID) {
                    // Reset an ID of the last successfully installed midlet
                    // because an error has occured.
		    try {
			GraphicalInstaller.saveSettings(null,
			    MIDletSuite.UNUSED_SUITE_ID);

		    } catch ( Exception e ) {
			//IMPL_NOTE: indicate error
		    }
                }

            }
        }

        /**
         * Called with the current state of the install so the user can be
         * asked to override the warning. Calls the parent to display the
         * warning to the user and then waits on the state object for
         * user's response.
         *
         * @param state current state of the install.
         *
         * @return true if the user wants to continue,
         *         false to stop the install
         */
        public boolean warnUser(InstallState state) {
	    System.out.println("BackgroundInstaller.warnUser():  enter");
            installState = state;

            InvalidJadException e = installState.getLastException();

            int reason = e.getReason();
            if (noConfirmation) {
                if (update) {
                    /* no confirmation is needed */
                    return true;
                } else {
                    /* confirmation is needed only for update */
                    if((reason != InvalidJadException.OLD_VERSION) &&
                       (reason != InvalidJadException.ALREADY_INSTALLED) &&
                       (reason != InvalidJadException.NEW_VERSION)) {
                        /* no confirmation is needed since it's not an update */
                        return true;
                    }
                }
            }

            switch (e.getReason()) {
            case InvalidJadException.UNAUTHORIZED:
                proxyAuth = false;
                parent.getUsernameAndPassword();
                break;

            case InvalidJadException.PROXY_AUTH:
                proxyAuth = true;
                parent.getProxyUsernameAndPassword();
                break;

            case InvalidJadException.OLD_VERSION:
	    case InvalidJadException.ALREADY_INSTALLED:
		Command cmd =
		    Dialog.show(Resource.getString(     //title
				ResourceConstants.WARNING),
				translateJadException(e, name,
				    state.getAppProperty(MIDletSuite.VENDOR_PROP),
				    state.getAppProperty(MIDletSuite.VENDOR_PROP),
				    url),//message
				new Command[]{cancelCmd, okCmd},//commands
				Dialog.TYPE_WARNING,//type
				null,//icon
				0,//timeout,
				dialogTransition);//transition
		if (okCmd == cmd) {
		    /* indicate user wants to continue */
		    return true;
		}
		else {
		    return false;
		}
		/* indicate user does not want to continue */

            case InvalidJadException.NEW_VERSION:
                // this is now an update
                update = true;

                // fall through
	    default:
		System.out.println("BackgroundInstaller.warnUser():  calling parent.warnUser()");
                return parent.warnUser(name,
                    state.getAppProperty(MIDletSuite.VENDOR_PROP),
                    state.getAppProperty(MIDletSuite.VERSION_PROP),
                    url, e);
            }

            return true;
        }

        /**
         * Called with the current state of the install so the user can be
         * asked to confirm the jar download.
         * If false is returned, the an I/O exception thrown and
         * {@link Installer#wasStopped()} will return true if called.
         *
         * @param state current state of the install.
         *
         * @return true if the user wants to continue, false to stop the
         *         install
         */
        public boolean confirmJarDownload(InstallState state) {
	    System.out.println("BackgroundInstaller.confirmJarDownload():  enter");
            if (update || noConfirmation) {
                // this an update, no need to confirm.
                return true;
            }

            installState = state;
            url = state.getJarUrl();

            return  parent.displayDownloadConfirmation(state);
	}
        /**
         * Called with the current state of the install so the user can be
         * asked to confirm if the RMS data should be kept for new version of
         * an updated suite.
         *
         * @param state current state of the install.
         *
         * @return true if the user wants to keep the RMS data for the next
         * suite
         */
        public boolean keepRMS(InstallState state) {
	    System.out.println("BackgroundInstaller.keepRMS():  enter");
            installState = state;

            return parent.displayKeepRMSForm(state);
        }


        /**
         * Called with the current state of the install so the user can be
         * asked to confirm the authentication path.
         * If false is returned, the an I/O exception thrown and
         * {@link Installer#wasStopped()} will return true if called.
         *
         * @param state current state of the install.
         *
         * @return true if the user wants to continue, false to stop the
         *         install
         */
        public boolean confirmAuthPath(InstallState state) {
	    System.out.println("BackgroundInstaller.confirmAuthPath():  enter");
            return parent.displayAuthPathConfirmation(state);
        }

        /**
         * Called with the current state of the install and the URL where the
         * request is attempted to be redirected so the user can be asked
         * to confirm if he really wants to install from the new location.
         * If false is returned, the an I/O exception thrown and
         * {@link Installer#wasStopped()} will return true if called.
         *
         * @param state       current state of the install.
         * @param newLocation new url of the resource to install.
         *
         * @return true if the user wants to continue, false to stop the install
         */
        public boolean confirmRedirect(InstallState state, String newLocation) {
	    System.out.println("BackgroundInstaller.confirmRedirect():  enter");
            return parent.displayRedirectConfirmation(state, newLocation);
        }

        /**
         * Called with the current state of the install so the user can be
         * asked to confirm the jar only download.
         *
         * @return true if the user wants to continue, false to stop the
         *         install
         */
        private boolean confirmJarOnlyDownload() {
	    System.out.println("BackgroundInstaller.confirmJarOnlyDownload():  enter");
            if (update) {
                // this an update, no need to confirm.
                return true;
            }

            return parent.displayJarOnlyDownloadConfirmation();
        }


        /**
         * Called with the current status of the install.
         * Changes the status alert box text based on the status.
         *
         * @param status current status of the install.
         * @param state current state of the install.
         */
        public void updateStatus(int status, InstallState state) {
	    System.out.println("BackgroundInstaller.updateStatus():  enter");
            parent.updateStatus(status, state);
        }

        /**
         * Wait for the cancel message to be displayed to prevent flashing
         * and then display the list of suites.
         */
        private void displayListAfterCancelMessage() {
	    System.out.println("BackgroundInstaller.displayListAfterCancelMessage():  enter");
            // wait for the parent to display "cancelled"
            synchronized (parent) {

                // go back to app list
                parent.exit(false);
            }
        }
    }

}
