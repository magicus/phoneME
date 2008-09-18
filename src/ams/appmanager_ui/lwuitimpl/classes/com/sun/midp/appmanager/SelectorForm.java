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

import java.util.Hashtable;

import com.sun.lwuit.*;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.TextArea;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.events.FocusListener;
import com.sun.lwuit.geom.Dimension;
import com.sun.lwuit.layouts.BorderLayout;
import com.sun.lwuit.layouts.*;


import com.sun.midp.configurator.Constants;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.installer.GraphicalInstaller;
import com.sun.midp.io.j2me.push.*;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midletsuite.*;


/**
 * Form to display several midlets from within the same midlet suite
 */
public class SelectorForm extends Form implements ActionListener  {
    /**
     * Information needed to display a list of MIDlets.
     */
    private RunningMIDletSuiteInfo msi;
    /**
     * Number of midlets in minfo.
     */
    private int mcount;
    /**
     * MIDlet information, class, name, icon; one per MIDlet.
     */
    private MIDletInfo[] minfo;


    /** Command object for "Back" command for AMS */
    private static final Command backCommand =
	new Command(Resource.getString(ResourceConstants.BACK),
		    ResourceConstants.BACK);

    /** Command object for "Launch" */
    private Command launchCmd =
	new Command(Resource.getString(ResourceConstants.LAUNCH),
		    ResourceConstants.LAUNCH);

    /* mapping:  button->suite info */
    private Hashtable midletsHash;
    /* mapping:  suite info->button */
    private Hashtable reverseHash;
    /* midlets buttons container */
    private Container buttonsContainer;

    /* Main AMS form */
    private Form mainForm;

    private Transition right, left, dialogTransition;

    private ApplicationManager manager;

    private final int MAX_MIDLETS_IN_SUITE = 20;
    private final int SMALL_ICON_SIZE = 20;
    private final int LIST_ROW_HEIGHT = 20;
    private final int RUN_SPEED = 500;

    private int displayHeight;
    private int displayWidth;
    private int rows = 5;


    /**
     * Ctor.  Initialize midlet suite independent logic.
     *
     * @param mainForm:  main AMS form
     * @param manager:  application manager used to run midlets
     */
    public SelectorForm(Form mainForm, ApplicationManager manager) {
	this.mainForm = mainForm;
	this.manager = manager;

	left = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, true, RUN_SPEED);
	right = CommonTransitions.createSlide(CommonTransitions.SLIDE_HORIZONTAL, false, RUN_SPEED);

	setTransitionOutAnimator(left);
	setTransitionInAnimator(right);

	displayHeight = com.sun.lwuit.Display.getInstance().getDisplayHeight();
	displayWidth = com.sun.lwuit.Display.getInstance().getDisplayWidth();
	rows = displayHeight / LIST_ROW_HEIGHT;
	setLayout(new FlowLayout());

	addCommand(backCommand);
	addCommand(launchCmd);
	setCommandListener(this);
    }

    /**
     * Set contents of the selector form
     *
     * @param msi midlet suite
     */
    public void setContents(RunningMIDletSuiteInfo msi) {
	this.msi = msi;
	mcount = 0;

	/* IMPL_NOTE:  buttonsContainer needs to be recreated for each invocation
    	    of this method to avoid showing same midlets over and over*/
	minfo = new MIDletInfo[MAX_MIDLETS_IN_SUITE];

	setTitle(msi.displayName);
	buttonsContainer = new Container();
	buttonsContainer.setLayout(new GridLayout(rows, 1));
	midletsHash = new Hashtable();
	reverseHash = new Hashtable();

	/* add midlets from suite to container and hash tables*/
	addMidlets();
	/* clear form */
	removeAll();
	/* add buttons container to the form */
	addComponent(buttonsContainer);
    }


    /**
     * Add midlets to the midlets buttons container
     *
     */
    private void addMidlets() {
	MIDletSuiteStorage mss = MIDletSuiteStorage.getMIDletSuiteStorage();
	/* read midlet suite items to data structures */
	try {
	    readMIDletInfo(mss);
	} catch ( Throwable t ) {
	    t.printStackTrace();
	}

	/* Add  midlets from suite */
	for (int i = 0; i < mcount; i++) {
	    Image icon = null;
	    ButtonActionListener bAListener = new ButtonActionListener();
	    if (minfo[i].icon != null) {
		icon = AppManagerUIImpl.convertImage(RunningMIDletSuiteInfo.getIcon(msi.suiteId,
		    minfo[i].icon, mss));
	    }
	    /* resize image */
	    icon = icon.scaled(SMALL_ICON_SIZE, SMALL_ICON_SIZE);
	    /* create button */
	    /* build button */
	    Button button = new Button(minfo[i].name){
		public Image getPressedIcon() {
		    Image i = getIcon();
		    return i.scaled((int) (i.getWidth() * 0.8), (int) (i.getHeight() * 0.8));
		}

		public Image getRolloverIcon() {
		    Image i = getIcon();
		    return i.scaled((int) (i.getWidth() * 1.2), (int) (i.getHeight() * 1.2));
		}
	    };
	    button.setIcon(icon);
	    button.getStyle().setBgTransparency(0x00);
	    button.setBorderPainted(false);
	    button.setAlignment(Label.LEFT);
	    button.setTextPosition(Label.RIGHT);
	    button.addActionListener(bAListener);

	    /* set button width */
	    Dimension currentSize = button.getPreferredSize();
	    Dimension newSize = new Dimension(displayWidth, currentSize.getHeight());
	    button.setPreferredSize(newSize);

	    /* Add button */
            buttonsContainer.addComponent(button);
	    midletsHash.put(button, msi);
	    reverseHash.put(msi, button);
	}
    }


    /**
     * Commands handler dispatcher.
     *
     * @param evt: action event with the command ID
     */
    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();

	switch (cmd.getId()) {
	case ResourceConstants.BACK:
	    mainForm.show();
	    break;
	case ResourceConstants.LAUNCH:
	    handlerLaunch();
	    break;
	}
    }


    /**
     *	Obtains focus and launches approperiate midlet
     */
    private void handlerLaunch() {
	int index = buttonsContainer.getComponentIndex(getFocused());
	System.out.println("handlerLaunch():  enter.  index is " + index);
	manager.launchSuite(msi, minfo[index].classname);
    }

    /**
     * Add a MIDlet to the list.
     *
     * @param info MIDlet information to add to MIDlet
     */
    private void addMIDlet(MIDletInfo info) {
	System.out.println("addMIDlet():  enter.  info is " + info.toString());
        if (mcount >= minfo.length) {
            MIDletInfo[] n = new MIDletInfo[mcount+4];
            System.arraycopy(minfo, 0, n, 0, mcount);
            minfo = n;
        }

        minfo[mcount++] = info;
    }

     /**
     * Read in and create a MIDletInfo for each MIDlet
     *
     * @param mss the midlet suite storage
     *
     * @throws MIDletSuiteCorruptedException if the suite is corrupted
     * @throws MIDletSuiteLockedException if the suite is locked
     */
    private void readMIDletInfo(MIDletSuiteStorage mss)
            throws MIDletSuiteCorruptedException, MIDletSuiteLockedException {
	System.out.println("readMIDletInfo():  enter");
        MIDletSuite midletSuite = mss.getMIDletSuite(msi.suiteId, false);

            if (midletSuite == null) {
		System.out.println("midletSuite is null");
                return;
            }

            try {
                for (int n = 1; n < 100; n++) {
                    String nth = "MIDlet-"+ n;
                    String attr = midletSuite.getProperty(nth);
                    if (attr == null || attr.length() == 0){
                        break;
		    }
                    addMIDlet(new MIDletInfo(attr));
                }
            } finally {
                midletSuite.close();
            }
    }


    /**
     * Class needed to intercept button pressed events
     *
     */
    private class ButtonActionListener implements ActionListener {

        public void actionPerformed(ActionEvent evt) {
	    RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
					 midletsHash.get(evt.getSource()));

	    handlerLaunch();
        }
    }

}
