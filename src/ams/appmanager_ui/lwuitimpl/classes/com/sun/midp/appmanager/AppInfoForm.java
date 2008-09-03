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

package com.sun.midp.appmanager;


import com.sun.lwuit.*;
import com.sun.lwuit.TextArea;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.geom.Dimension;
import com.sun.lwuit.layouts.BorderLayout;
import com.sun.lwuit.layouts.BoxLayout;
import com.sun.lwuit.layouts.FlowLayout;
import com.sun.midp.configurator.Constants;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.installer.GraphicalInstaller;
import com.sun.midp.io.j2me.push.*;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midletsuite.*;

import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.animations.CommonTransitions;


public class AppInfoForm extends Form implements ActionListener  {

    private MIDletSuiteStorage midletSuiteStorage;

    private Transition in, out;

    private final int MAX_COLS = 6;
    private int largestW = 0;
    private int runSpeed = 500;
    private int numberOfMidlets;

    private Form mainForm;

    private Image suiteIcon;

    /** Command object for "Back" command for AMS */
    private static final Command backCommand =
	new Command(Resource.getString(ResourceConstants.BACK),
		    ResourceConstants.BACK);

    public AppInfoForm(Form mainForm){
	this.mainForm = mainForm;
	addCommand(backCommand);
	setCommandListener(this);
	setLayout(new BoxLayout(BoxLayout.Y_AXIS));

	out = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, true, runSpeed);
	in = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, false, runSpeed);
	setTransitionOutAnimator(out);
	setTransitionInAnimator(in);
    }


    public void setContents(int suiteId) throws Throwable {

	MIDletSuiteImpl midletSuite = null;
	InstallInfo installInfo; /** Installation information of the suite. */
	javax.microedition.lcdui.Image tmpIcon;
	String displayName;

	suiteIcon = null; /* reset suite icon */

	midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
	midletSuite = midletSuiteStorage.getMIDletSuite(suiteId, false);
	numberOfMidlets = midletSuite.getNumberOfMIDlets();
	installInfo = midletSuite.getInstallInfo();


	try {
	    midletSuiteStorage.getMIDletSuite(suiteId, false);
	} catch ( Exception e ) {
	    throw(e);
	}


	/* clear components from previous invocations */
	removeAll();

	/* set title */
	if (numberOfMidlets == 1) {
            String value = midletSuite.getProperty("MIDlet-1");
            MIDletInfo temp = new MIDletInfo(value);
            displayName = temp.name;
        } else {
            displayName =
                midletSuite.getProperty(MIDletSuiteImpl.SUITE_NAME_PROP);
        }
	setTitle("Info " + displayName);

	/* set midlet icon */
	Label iconLabel = new Label();
	iconLabel.setIcon(getSuiteIcon());
	iconLabel.getStyle().setBgTransparency(0xff);
	addComponent(createPair(displayName, iconLabel, largestW));

	/* set size */
	StringBuffer tmpBuf = new StringBuffer(40);
	Label size = new Label();
	
	tmpBuf.setLength(0);
	tmpBuf.append(
	    Integer.toString((MIDletSuiteStorage.getMIDletSuiteStorage().
		getStorageUsed(midletSuite.getID()) +
		1023) / 1024));
	tmpBuf.append(" K");
	size.setText(tmpBuf.toString());
	addComponent(createPair(Resource.getString(ResourceConstants.AMS_SIZE),
			size, largestW));

	/* set version */
	Label tmpLabel = new Label(midletSuite.getProperty(
				    MIDletSuite.VERSION_PROP));
	tmpLabel.setFocusable(true);
	addComponent(createPair(Resource.getString(ResourceConstants.AMS_VERSION),
				tmpLabel, largestW));

	/* set vendor */
	if (midletSuite.isTrusted()) {
	    addComponent(createPair(Resource.getString
                          (ResourceConstants.AMS_MGR_AUTH_VENDOR),
				new Label(midletSuite.getProperty(
				    MIDletSuite.VENDOR_PROP)),
				    largestW));
	}
	else {
	    addComponent(createPair(Resource.getString
                (ResourceConstants.AMS_MGR_VENDOR),
		new Label(midletSuite.getProperty( MIDletSuite.VENDOR_PROP)),
				    largestW));
	}

	/* set description */
	String descProp = midletSuite.getProperty(MIDletSuite.DESC_PROP);
	if (descProp != null) {
	    addComponent(createPair(Resource.getString
	        (ResourceConstants.AMS_DESCRIPTION),
		new Label(descProp),
		largestW));
	}

	/* set contents */
	String amsContents = null;
	/* IMPL_NOTE:  uncomment when AMS_CONTENTS is available */
	/* amsContents = midletSuite.getProperty(MIDletSuite.AMS_CONTENTS); */
	if (amsContents != null) {
	    addComponent(createPair(Resource.getString
		(ResourceConstants.AMS_CONTENTS),
		new Label(amsContents),
		largestW));
	}

	/* set website */
	String downloadUrl = installInfo.getDownloadUrl();
	if (downloadUrl != null) {
	    Label downloadUrlLable = new Label(downloadUrl);
	    downloadUrlLable.setFocusable(true);
	    addComponent(createPair(Resource.getString
		 (ResourceConstants.AMS_WEBSITE), downloadUrlLable, largestW));
	}

//         if (downloadUrl != null) {
//             addComponent(createPair(Resource.getString
//                  (ResourceConstants.AMS_WEBSITE),
//                  new Label(downloadUrl),
//                  largestW));
//         }

	/* set advanced separator */
	addComponent(createPair(Resource.getString
		     (ResourceConstants.AMS_ADVANCED),
			    new Label(""),
				largestW));


	/* set is trusted */
	CheckBox checkBox = new CheckBox();
	checkBox.setSelected(midletSuite.isTrusted());
	addComponent(createPair(Resource.getString
		     (ResourceConstants.AMS_MGR_TRUSTED),
				checkBox,
				largestW));

	/* set is verified */
       if (Constants.VERIFY_ONCE && midletSuite.isVerified()) {
	   addComponent(createPair(Resource.getString
			(ResourceConstants.AMS_VERIFIED_CLASSES),
				   new Label(""),
				   largestW));
	}

       /* set authorized by */
       String[] authPath = installInfo.getAuthPath();
       if (authPath != null) {
	   List authList = new List();
	   for (int i = 0; i < authPath.length; i++) {
	       authList.addItem(new Label(authPath[i]));
	   }

	   addComponent(createPair(Resource.getString
			(ResourceConstants.AMS_AUTHORIZED_BY),
				   authList,
				   largestW));
       }

       /* set list connections */
       String listConnections = PushRegistryInternal.listConnections(
                       midletSuite.getID(), false);
       if (listConnections != null) {
	   addComponent(createPair(Resource.getString
			(ResourceConstants.AMS_AUTO_START_CONN),
				   new Label(listConnections),
				   largestW));
       }

    }

    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();

	switch (cmd.getId()) {
	case ResourceConstants.BACK:
	    mainForm.show();
	    break;
	}
    }


    /**
     * Helper method that allows us to create a pair of components label and the given
     * component in a horizontal layout with a minimum label width
     */
    protected Container createPair(String label, Component c, int minWidth) {
	Container pair = new Container(new BorderLayout());
	//Label l =  new Label(label);
	int rows = 1;
	if (label.length() > MAX_COLS) {
	    rows = 2;
	}
	TextArea t = new TextArea(label, rows, MAX_COLS);
	t.setEditable(false);
	//Dimension d = l.getPreferredSize();
	//d.setWidth(Math.max(d.getWidth(), minWidth));
	//l.setPreferredSize(d);
	//l.getStyle().setBgTransparency(100);
	pair.addComponent(BorderLayout.WEST,t);
	//pair.addComponent(BorderLayout.WEST,l);
	pair.addComponent(BorderLayout.CENTER, c);
	return pair;
    }

     /**
     * Helper method that allows us to create a pair of components label and the given
     * component in a horizontal layout
     */
     protected Container createPair(String label, Component c) {
         return createPair(label,c,0);
     }

    /**
    * Gets the single MIDlet suite icon from storage.
    *
    * @return icon image
    */
    private Image getSuiteIcon() {

	javax.microedition.lcdui.Image sourceImage;
	String srcIconName;

	if (suiteIcon != null) {
	    return suiteIcon;
	}

	if (numberOfMidlets == 1) {
	    srcIconName = new String("_single8");
	}
	else {
	    srcIconName = new String("_suite8");
	}

	System.out.println("srcIconName is" + srcIconName);
	sourceImage = GraphicalInstaller.getImageFromInternalStorage(srcIconName);
	System.out.println("Source image is " + sourceImage);
	suiteIcon = AppManagerUIImpl.convertImage(sourceImage);
	System.out.println("suiteIcon is " + suiteIcon);

	return suiteIcon;
    }
}
