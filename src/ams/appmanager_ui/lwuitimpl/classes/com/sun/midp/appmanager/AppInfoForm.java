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


public class AppInfoForm extends Form implements ActionListener {

	private MIDletSuiteStorage midletSuiteStorage;

	private Transition in, out;

	private final int MAX_COLS = 7;
	private final int largestW = 0;
	private final int runSpeed = 500;

	private int numberOfMidlets;

	private Form mainForm;
	private Font font;

	private Image suiteIcon;
	private Image midletIcon;

	/** Command object for "Back" command for AMS */
	private static final Command backCommand =
	new Command(Resource.getString(ResourceConstants.BACK),
				ResourceConstants.BACK);

	public AppInfoForm(Form mainForm){
		this.mainForm = mainForm;
		addCommand(backCommand);
		setCommandListener(this);
		setLayout(new BoxLayout(BoxLayout.Y_AXIS));

		createIcons();

		out = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, true, runSpeed);
		in = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, false, runSpeed);
		font = Font.createSystemFont(Font.FACE_SYSTEM,
									 Font.STYLE_BOLD, Font.SIZE_SMALL);
		setTransitionOutAnimator(out);
		setTransitionInAnimator(in);
	}


	public void setContents(int suiteId) throws Throwable {

		MIDletSuiteImpl midletSuite = null;
		InstallInfo installInfo; /** Installation information of the suite. */
		String displayName;
		Label tmpLabel;
		TextArea tmpArea;

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
		tmpLabel = new Label();
		tmpLabel.setIcon(getSuiteIcon());
		tmpLabel.getStyle().setBgTransparency(0x00);
		tmpLabel.setBorderPainted(false);
		addComponent(createPair(displayName, tmpLabel));

		/* set size */
		StringBuffer tmpBuf = new StringBuffer(40);
		tmpLabel = new Label();
		tmpLabel.getStyle().setBgTransparency(0x00);
		tmpLabel.setBorderPainted(false);

		tmpBuf.setLength(0);
		tmpBuf.append(
					 Integer.toString((MIDletSuiteStorage.getMIDletSuiteStorage().
									   getStorageUsed(midletSuite.getID()) +
									   1023) / 1024));
		tmpBuf.append(" K");
		tmpLabel.setText(tmpBuf.toString());
		addComponent(createPair(Resource.getString(ResourceConstants.AMS_SIZE),
								tmpLabel));

		/* set version */
		tmpLabel = new Label(midletSuite.getProperty(
													MIDletSuite.VERSION_PROP));
		tmpLabel.getStyle().setBgTransparency(0x00);
		tmpLabel.setBorderPainted(false);
		addComponent(createPair(Resource.getString(ResourceConstants.AMS_VERSION),
								tmpLabel));

		/* set vendor */
		tmpArea = new TextArea(midletSuite.getProperty(
													  MIDletSuite.VENDOR_PROP));
		tmpArea.getStyle().setBgTransparency(0x00);
		tmpArea.setBorderPainted(false);
		tmpArea.setFocusable(false);
		if (midletSuite.isTrusted()) {
			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_MGR_AUTH_VENDOR),
									tmpArea));
		} else {
			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_MGR_VENDOR),
									tmpArea));
		}

		/* set description */
		String descProp = midletSuite.getProperty(MIDletSuite.DESC_PROP);
		if (descProp != null) {
			tmpArea = new TextArea(descProp);
			tmpArea.setFocusable(false);
			tmpArea.getStyle().setBgTransparency(0x00);
			tmpArea.setBorderPainted(false);
			tmpArea.setRows(2);

			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_DESCRIPTION), tmpArea));
		}

		/* set contents */
		String amsContents = null;
		/* IMPL_NOTE:  uncomment when AMS_CONTENTS is available */
		/* amsContents = midletSuite.getProperty(MIDletSuite.AMS_CONTENTS); */
		if (amsContents != null) {
			tmpLabel = new Label(amsContents);
			tmpLabel.getStyle().setBgTransparency(0x00);
			tmpLabel.setBorderPainted(false);

			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_CONTENTS), tmpLabel));
		}

		/* set website */
		String downloadUrl = installInfo.getDownloadUrl();
		if (downloadUrl != null) {
			tmpArea = new TextArea(downloadUrl);
			tmpArea.setFocusable(false);
			tmpArea.getStyle().setBgTransparency(0x00);
			tmpArea.setBorderPainted(false);
			tmpArea.setRows(2);
			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_WEBSITE), tmpArea));
		}

		/* set advanced separator */
		tmpLabel = new Label("");
		tmpLabel.getStyle().setBgTransparency(0x00);
		tmpLabel.setBorderPainted(false);

		addComponent(createPair(Resource.getString
								(ResourceConstants.AMS_ADVANCED), tmpLabel));

		/* set is trusted */
		CheckBox checkBox = new CheckBox();
		checkBox.getStyle().setBgTransparency(0x00);
		checkBox.setBorderPainted(false);
		checkBox.setSelected(midletSuite.isTrusted());
		checkBox.setFocusable(false);
		addComponent(createPair(Resource.getString
								(ResourceConstants.AMS_MGR_TRUSTED),
								checkBox));
		/* set is verified */
		if (Constants.VERIFY_ONCE && midletSuite.isVerified()) {
			tmpLabel = new Label("");
			tmpLabel.getStyle().setBgTransparency(0x00);
			tmpLabel.setBorderPainted(false);
			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_VERIFIED_CLASSES), tmpLabel));
		}

		/* set authorized by */
		String[] authPath = installInfo.getAuthPath();
		if (authPath != null) {
			Container c = new Container();
			for (int i = 0; i < authPath.length; i++) {
				tmpLabel = new Label(authPath[i]);
				tmpLabel.getStyle().setBgTransparency(0x00);
				tmpLabel.setBorderPainted(false);
			}

			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_AUTHORIZED_BY), c));
		}

		/* set list connections */
		String listConnections = PushRegistryInternal.listConnections(
																	 midletSuite.getID(), false);
		if (listConnections != null) {
			tmpLabel = new Label(listConnections);
			tmpLabel.getStyle().setBgTransparency(0x00);
			tmpLabel.setBorderPainted(false);

			addComponent(createPair(Resource.getString
									(ResourceConstants.AMS_AUTO_START_CONN),
									tmpLabel));
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
	protected Container createPair(String label, Component c) {
		Container pair = new Container(new BorderLayout());
		int rows = 1;
		if (label.length() > MAX_COLS) {
			rows = 2;
		}
		TextArea t = new TextArea(label, rows, MAX_COLS);
		t.getStyle().setBgTransparency(0x00);
		t.getStyle().setFont(font);
		t.setBorderPainted(false);
		t.setEditable(false);
		pair.addComponent(BorderLayout.WEST,t);
		pair.addComponent(BorderLayout.CENTER, c);
		return pair;
	}


	/**
	* Gets the single MIDlet suite icon from storage.
	*
	* @return icon image
	*/
	private Image getSuiteIcon() {
		if (numberOfMidlets == 1) {
			return midletIcon;
		}
		return suiteIcon;
	}

	private void createIcons() {
		javax.microedition.lcdui.Image sourceImage;

		/* create midlet icon */
		sourceImage = GraphicalInstaller.getImageFromInternalStorage(new String("_single8"));
		midletIcon = AppManagerUIImpl.convertImage(sourceImage);

		/* create midlets suite icon */
		sourceImage = GraphicalInstaller.getImageFromInternalStorage(new String("_suite8"));
		suiteIcon = AppManagerUIImpl.convertImage(sourceImage);
	}
}
