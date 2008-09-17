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

import java.util.*;
import java.util.Hashtable;

import javax.microedition.io.*;

import javax.microedition.midlet.*;

import javax.microedition.rms.*;

import com.sun.midp.configurator.Constants;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.midlet.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.lwuit.*;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.events.FocusListener;
import com.sun.lwuit.layouts.*;
import com.sun.lwuit.list.DefaultListModel;
import com.sun.lwuit.list.ListModel;
import com.sun.lwuit.plaf.Style;
import com.sun.lwuit.plaf.UIManager;
import com.sun.lwuit.util.Resources;

import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;


/**
 * The Graphical MIDlet suite Discovery Application.
 * <p>
 * Let the user install a suite from a list of suites
 * obtained using an HTML URL given by the user. This list is derived by
 * extracting the links with hrefs that are in quotes and end with ".jad" from
 * the HTML page. An href in an extracted link is assumed to be an absolute
 * URL for a MIDP application descriptor. The selected URL is then passed to
 * graphical Installer.
 */
public class DiscoveryApp extends Form implements ActionListener {

    /** Contains the default URL for the install list. */
    private String defaultInstallListUrl = "http://";
    /** Contains the URL the user typed in. */
    private TextArea urlTextArea;
    /** Main AMS form */
    private Form mainForm;
    /** Form to contain the URL the user typed in */
    private Form urlTextForm;
    /** Displays the progress of the install. */
    private Form progressForm;
    /** Displays a list of suites to install to the user. */
    private Form installListForm;
    /** Gauge for progress form index. */
    private int progressGaugeIndex;
    /** URL for progress form index. */
    private int progressUrlIndex;
    /** Keeps track of when the display last changed, in milliseconds. */
    private long lastDisplayChange;
    /** Contains a list of suites to install. */
    private Vector installList;

    /** Command object for URL screen to go and discover available suites. */
    private Command discoverCmd =
        new Command(Resource.getString(ResourceConstants.GOTO),
                    ResourceConstants.GOTO);


    /** Command object for "Back" command in the suite list form. */
    private Command backCmd = new Command(Resource.getString
					  (ResourceConstants.BACK),
                                          ResourceConstants.BACK);
    /** Command object for URL screen to save the URL for suites. */
    private Command saveCmd =
        new Command(Resource.getString(ResourceConstants.SAVE),
                    ResourceConstants.SAVE);

    /** Command object for "Exit" command for main AMS menu */
    private Command exitCommand =
	new Command(Resource.getString(ResourceConstants.EXIT),
		    ResourceConstants.EXIT);

    /** Command object for "Ok" command for dialogs form. */
    private Command okCmd =
	new Command(Resource.getString(ResourceConstants.OK),
		    ResourceConstants.OK);

    /** Command object for "Ok" command for dialogs form. */
    private Command installCmd =
	new Command(Resource.getString(ResourceConstants.INSTALL),
		    ResourceConstants.INSTALL);


    /* Mapping button -> button suite index */
    private Hashtable itemsHash;

    private Transition right, left, dialogTransition;
    /* transition speed */
    private final int runSpeed = 1000;

    /**
     * Create and initialize a new discovery application MIDlet.
     * The saved URL is retrieved and the list of MIDlets are retrieved.
     */
    public DiscoveryApp(Form mainForm) {

	System.out.println("DiscoveryApp():  enter");
	this.mainForm = mainForm;

	itemsHash = new Hashtable();

	left = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, true, runSpeed);
	right = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, false, runSpeed);
	dialogTransition = CommonTransitions.createSlide(CommonTransitions.SLIDE_VERTICAL, true, runSpeed);

        GraphicalInstaller.initSettings();
	/* get amsUrl and defaultInstallListUrl */
	restoreSettings();
        /* do not call createForms before restoreSettings */
        createForms();

    }

    public void showMainForm() {
        urlTextForm.show();
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

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */


    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();

	switch (cmd.getId()) {
	case ResourceConstants.GOTO:
	    // user wants to discover the suites that can be installed
	    urlTextForm.setTransitionInAnimator(left);
	    urlTextForm.setTransitionOutAnimator(right);
            discoverSuitesToInstall(urlTextArea.getText());
	    break;
	case ResourceConstants.BACK:
	    /* update transitions */
	    urlTextForm.setTransitionInAnimator(left);
	    urlTextForm.setTransitionOutAnimator(right);
            urlTextForm.show();
	    break;
	case ResourceConstants.SAVE:
	    saveURLSetting();
	    break;
	case ResourceConstants.EXIT:
	    urlTextForm.setTransitionInAnimator(right);
     	    urlTextForm.setTransitionOutAnimator(left);
	    mainForm.show();
	    break;
	case ResourceConstants.INSTALL:
	    handlerInstall();
	    break;
	case ResourceConstants.END:
	    /* IMPL_NOTE:  uncomment in separate midlet configuration */
	    //notifyDestroyed();
	    break;
	}
    }



    /**
     * Get the settings the Manager saved for the user.
     */
    private void restoreSettings() {
        ByteArrayInputStream bas;
        DataInputStream dis;
        byte[] data;
        RecordStore settings = null;

	System.out.println("restoreSettings():  enter");

        /**
         * ams.url = "" or null when running OTA from command line /
         *           OTA provisioning
         * ams.url = <some url> when running OTA from KToolbar */
        String amsUrl = System.getProperty("ams.url");
        if (amsUrl != null && !amsUrl.equals("")) {
            defaultInstallListUrl = amsUrl.trim();
            return;
        }

        try {
            settings = RecordStore.openRecordStore(
                       GraphicalInstaller.SETTINGS_STORE, false);

            data = settings.getRecord(1);
            if (data != null) {
                bas = new ByteArrayInputStream(data);
                dis = new DataInputStream(bas);
                defaultInstallListUrl = dis.readUTF();
            }

        } catch (RecordStoreException e) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                               "restoreSettings threw a RecordStoreException");
            }
        } catch (IOException e) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                               "restoreSettings threw an IOException");
            }
        } finally {
            if (settings != null) {
                try {
                    settings.closeRecordStore();
                } catch (RecordStoreException e) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "closeRecordStore threw a RecordStoreException");
                    }
                }
            }
        }
    }

    /**
     * Save the URL setting the user entered in to the urlTextBox.
     */
    private void saveURLSetting() {
        String temp;
        Exception ex;
	System.out.println("saveURLSetting():  enter");

        temp = urlTextArea.getText();

	try {
	    GraphicalInstaller.saveSettings(temp, MIDletSuite.INTERNAL_SUITE_ID);
	} catch ( Exception e ) {
	    Dialog.show(Resource.getString
				 (ResourceConstants.EXCEPTION),	//title
			    e.toString(),	//message
			    new Command[]{okCmd},//commands
			    Dialog.TYPE_ERROR,//type
			    null,//icon
			    0,	//infinite timeout,
			    dialogTransition);//transition
            urlTextForm.show();
	    return;
	}

        defaultInstallListUrl = temp;

	Dialog.show(Resource.getString(	//title
		ResourceConstants.AMS_INFORMATION),

		    Resource.getString(//text
		ResourceConstants.AMS_MGR_SAVED),
	    new Command[]{okCmd},//commands
	    Dialog.TYPE_INFO,//type
	    null,//icon
	    0,//timeout,
	    dialogTransition);//transition

    }


    /**
     * Let the user select a suite to install. The suites that are listed
     * are the links on a web page that end with .jad.
     *
     * @param url where to get the list of suites to install.
     */
    private void discoverSuitesToInstall(String url) {
	System.out.println("discoverSuitesToInstall():  enter");
        new Thread(new BackgroundInstallListGetter(this, url)).start();
    }


    /**
     * Install a suite.
     *
     * @param selectedSuite index into the installList
     */
    private void installSuite(int selectedSuite) {
        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();
        MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();
        SuiteDownloadInfo suite;
        String displayName;

	System.out.println("installSuite():  enter.  selected suite is "  + selectedSuite);

        suite = (SuiteDownloadInfo)installList.elementAt(selectedSuite);

        midletSuite.setTempProperty(null, "arg-0", "I");
        midletSuite.setTempProperty(null, "arg-1", suite.url);
        midletSuite.setTempProperty(null, "arg-2", suite.label);


	displayName =
            Resource.getString(ResourceConstants.INSTALL_APPLICATION);
        try {
	    System.out.println("Starting midlet " + displayName);
            midletStateHandler.startMIDlet(
                "com.sun.midp.installer.GraphicalInstaller", displayName);
            /*
             * Give the create MIDlet notification 1 second to get to
             * AMS.
             */
            Thread.sleep(1000);
            //notifyDestroyed();
        } catch (Exception e) {

	    Dialog.show(Resource.getString(	//title
		    ResourceConstants.ERROR),
		e.toString(),			//text
		new Command[]{okCmd},		//commands
		Dialog.TYPE_ERROR,		//type
		null,	//icon
		0,	//infinite timeout,
		dialogTransition);//transition

	    urlTextForm.show();
        }
    }


    private void handlerInstall() {
	System.out.println("handlerInstall():  enter");
	System.out.println("installListForm.getFocused() returns " +
			   installListForm.getFocused());
	int index = ((Integer)itemsHash.get(installListForm.getFocused())).intValue();

	installSuite(index);
    }

    private void createForms() {
	/* urlTextForm */
	urlTextArea = new TextArea(defaultInstallListUrl);
	urlTextArea.getStyle().setBgTransparency(0x00);
	urlTextForm = new Form(Resource.getString(ResourceConstants.
                                          AMS_DISC_APP_WEBSITE_INSTALL));
	urlTextForm.setLayout(new FlowLayout());
	urlTextForm.addComponent(urlTextArea);
	urlTextForm.addCommand(exitCommand);
	urlTextForm.addCommand(saveCmd);
	urlTextForm.addCommand(discoverCmd);

	urlTextForm.setTransitionInAnimator(right);
	urlTextForm.setTransitionOutAnimator(left);
	urlTextForm.setCommandListener(this);

	/* progressForm */
	progressForm = new Form();
	progressForm.setLayout(new FlowLayout());

	/* installListForm */
	installListForm = new Form(Resource.getString
                                     (ResourceConstants.
                                      AMS_DISC_APP_SELECT_INSTALL));
	installListForm.setLayout(new FlowLayout());
	installListForm.addCommand(backCmd);
	installListForm.addCommand(installCmd);
	installListForm.setCommandListener(this);
    }

    /** A class to get the install list in a background thread. */
    private class BackgroundInstallListGetter implements Runnable {
        /** Parent application. */
        private DiscoveryApp parent;
        /** URL of the list. */
        private String url;

        /**
         * Construct a BackgroundInstallListGetter.
         *
         * @param theParent parent of this object
         * @param theUrl where to get the list of suites to install.
         */
        private BackgroundInstallListGetter(DiscoveryApp theParent,
                                            String theUrl) {
	    System.out.println("BackgroundInstallListGetter():  enter");
            parent = theParent;
            url = theUrl;
        }

        /**
         * Get the list of suites for the user to install.
         * The suites that are listed
         * are the links on a web page that end with .jad.
         */
        public void run() {
            StreamConnection conn = null;
            InputStreamReader in = null;
            String errorMessage;

	    System.out.println("run():  enter");

            try {
		System.out.println("Opening connection");
                conn = (StreamConnection)Connector.open(url, Connector.READ);
                in = new InputStreamReader(conn.openInputStream());

		System.out.println("Calling getDownloadInfoFromPage");
		parent.installList =
		    SuiteDownloadInfo.getDownloadInfoFromPage(in);

		if (parent.installList.size() > 0) {
		    /* Add suites */
		    System.out.println("Adding suites");
		    parent.installListForm.removeAll();
		    for (int i = 0; i < parent.installList.size(); i++) {
			SuiteDownloadInfo suite =
			    (SuiteDownloadInfo)installList.elementAt(i);
			Button button = new Button(suite.label);
			button.getStyle().setBgTransparency(0x00);
			System.out.println("Adding suite " + i);
			itemsHash.put(button, new Integer(i));
			parent.installListForm.addComponent(button);
		    }

		    System.out.println("Calling installListForm.show()");
		    parent.installListForm.show();
		    return;
		}

            } catch (Exception e) {
		Dialog.show(Resource.getString(	//title
			ResourceConstants.ERROR),
			Resource.getString	//text
				(ResourceConstants.AMS_DISC_APP_CONN_FAILED_MSG),
			new Command[]{okCmd},//commands
			Dialog.TYPE_ERROR,//type
			null,//icon
			0,   //infinite timeout,
			dialogTransition);//transition
		urlTextForm.show();
            } finally {
                try {
                    conn.close();
                    in.close();
                } catch (Exception e) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                                      "close threw an Exception");
                    }
                }
            }
        }
    }

}
