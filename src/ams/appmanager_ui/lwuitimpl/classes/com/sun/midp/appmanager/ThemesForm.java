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

import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.Button;
import com.sun.lwuit.ButtonGroup;
import com.sun.lwuit.Component;
import com.sun.lwuit.Command;
import com.sun.lwuit.Container;
import com.sun.lwuit.Dialog;
import com.sun.lwuit.Display;
import com.sun.lwuit.Form;
import com.sun.lwuit.Label;
import com.sun.lwuit.RadioButton;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.layouts.BoxLayout;
import com.sun.lwuit.layouts.FlowLayout;
import com.sun.lwuit.plaf.Style;
import com.sun.lwuit.plaf.UIManager;
import com.sun.lwuit.util.Resources;

import java.io.IOException;
import java.io.ByteArrayInputStream;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.configurator.Constants;
import com.sun.midp.io.j2me.storage.File;
import com.sun.midp.io.j2me.storage.RandomAccessStream;

import javax.microedition.io.Connector;

/**
 * Simple demo showing off how a theme can be manipulated in LWUIT. Once the demo
 * is executed the theme changes completely for the rest of the application...
 *
 * @author Dima Gomon.  Based on code by Shai Almog
 */
public class ThemesForm extends Form implements ActionListener {

    /**
     * The full path to the storage location of the themes
     */

    /**
     * Names of the themes in the storage and display
     */
    private static final String[] THEMES = {"java", "business", "star", "OceanFish"};
    private static final String[] THEME_LABELS = {"Java", "Business", "Star", "Ocean Fish"};


    /** Command object for "Back" command for AMS */
    private static final Command backCommand =
	new Command(Resource.getString(ResourceConstants.BACK),
				ResourceConstants.BACK);


    private Form mainForm;
    private AppManagerUIImpl appManagerUIImpl;

    private ButtonGroup group1;
    private ButtonGroup group2;

    /* Animations speed */
    private final int RUN_SPEED = 500;

    private Transition in, out;

    public String getName() {
        return "Themes";
    }

    /**
     * Ctor.  Initialize the form
     */
    public ThemesForm(Form mainForm, AppManagerUIImpl appManagerUIImpl) {
	System.out.println(">>>ThemesForm()");
	this.mainForm = mainForm;
	this.appManagerUIImpl = appManagerUIImpl;

	out = CommonTransitions.createSlide(
	    CommonTransitions.SLIDE_HORIZONTAL, true, RUN_SPEED);
	in = CommonTransitions.createSlide(
	    CommonTransitions.SLIDE_HORIZONTAL, false, RUN_SPEED);
	setTransitionOutAnimator(out);
	setTransitionInAnimator(in);

	addCommand(backCommand);

	setTitle(Resource.getString(ResourceConstants.AMS_CHANGE_STYLE_TITLE));
	setLayout(new BoxLayout(BoxLayout.Y_AXIS));

        group1 = new ButtonGroup();
        Label title = new Label("Please choose a theme:");
        title.getStyle().setMargin(0, 0, 0, 0);
        title.getStyle().setBgTransparency(70);
        addComponent(title);
        for(int iter = 0 ; iter < THEME_LABELS.length ; iter++) {
            RadioButton rb = new RadioButton(THEME_LABELS[iter]);
            Style s = rb.getStyle();
            s.setMargin(0, 0, 0, 0);
            s.setBgTransparency(70);
            group1.add(rb);
            addComponent(rb);
        }
        group1.setSelected(getSelectedThemeIndex());

        Button updateButton = new Button("Update Theme");
        updateButton.setAlignment(Button.CENTER);
        updateButton.getStyle().setPadding(5, 5, 7, 7);
	updateButton.addActionListener(new ActionListener() {

	    public void actionPerformed(ActionEvent evt) {
		System.out.println(">>>actionPerformed()");
		int newSelectedIndex = group1.getSelectedIndex();
		System.out.println("actionPerformed():  selected index is " + newSelectedIndex);
		String themeName = THEMES[newSelectedIndex];
		System.out.println("actionPerformed():  themeName is " + themeName);
		if(themeName != null) {
		    themeName = themeName.toLowerCase();
		    System.out.println("<<<actionPerformed():  calling initTheme().");
		    initTheme(themeName);
		}
	    }
	});

        Container buttonPanel = new Container(new FlowLayout(Component.CENTER));
        buttonPanel.addComponent(updateButton);
        addComponent(buttonPanel);

	group2 = new ButtonGroup();
	Label layoutLabel = new Label("Please choose a layout:");
	layoutLabel.getStyle().setMargin(0, 0, 0, 0);
	layoutLabel.getStyle().setBgTransparency(70);
	addComponent(layoutLabel);
	for(int iter = 0 ; iter < AppManagerUIImpl.LAYOUTS.length ; iter++) {
            RadioButton rb = new RadioButton(AppManagerUIImpl.LAYOUTS[iter]);
            Style s = rb.getStyle();
            s.setMargin(0, 0, 0, 0);
            s.setBgTransparency(70);
            group2.add(rb);
            addComponent(rb);
        }
	group2.setSelected(getSelectedLayoutIndex());
	Button updateLayoutButton = new Button("Update Layout");
	updateLayoutButton.setAlignment(Button.CENTER);
	updateLayoutButton.getStyle().setPadding(5, 5, 7, 7);
	updateLayoutButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		System.out.println(">>>actionPerformed()");
		int newSelectedIndex = group2.getSelectedIndex();
		System.out.println("actionPerformed():  selected index is " + newSelectedIndex);
		String layoutName = AppManagerUIImpl.LAYOUTS[newSelectedIndex];
		System.out.println("actionPerformed():  layout name is " + layoutName);
		if(layoutName != null) {
		    System.out.println(">>>actionPerformed():  setting layout.");
		    setIconsLayout(layoutName);
		}
	    }
	});
	Container layoutButtonPanel = new Container(new FlowLayout(Component.CENTER));
	layoutButtonPanel.addComponent(updateLayoutButton);
	addComponent(layoutButtonPanel);

	System.out.println("<<<ThemesForm()");
    }

     /**
     * Initializes lwuit theme
     *
     * IMPL_NOTE:  make theme name dynamic
     */
    private void initTheme(String themeName) {
	System.out.println(">>>initTheme().  themeName is " + themeName);
	try {
	    RandomAccessStream storage = new RandomAccessStream();
	    storage.connect(File.getStorageRoot(Constants.INTERNAL_STORAGE_ID) +
                themeName + "Theme" + ".res", Connector.READ);
	    int length = storage.getSizeOf();
	    byte[] resourceData = new byte[length];
	    storage.readBytes(resourceData, 0,length);
	    storage.disconnect();
	    Resources r = Resources.open(new ByteArrayInputStream(resourceData));
	    UIManager.getInstance().setThemeProps(r.getTheme(themeName + "Theme"));
	    Display.getInstance().getCurrent().refreshTheme();
	} catch (IOException ioe) {
	    ioe.printStackTrace();
	}
	System.out.println("<<<initTheme()");
    }

    /** Sets icons layout in appmanager.
     *
     * This method is needed in order to be able to access appmanager from
     * internal class defined in actionPerformed.
     */
    private void setIconsLayout(String layoutName) {
	appManagerUIImpl.setIconsStyle(layoutName);
    }


    private int getSelectedThemeIndex() {
	System.out.println(">>>getSelectedThemeIndex()");
        int selectedThemeIndex = 0;
        String themeName = UIManager.getInstance().getThemeName();
        if(themeName == null) {
            return 0;
        }
        themeName = themeName.toLowerCase();
        for(int i=0; i<THEMES.length; i++) {
            if (THEMES[i].equals(themeName)) {
                selectedThemeIndex = i;
            }
        }
	System.out.println("<<<getSelectedThemeIndex().  Index is " + selectedThemeIndex);
        return selectedThemeIndex;
    }

    private int getSelectedLayoutIndex() {
	System.out.println(">>>getSelectedLayoutIndex(). Returning  " +
			   appManagerUIImpl.getCurrentStyle());
	return appManagerUIImpl.getCurrentStyle();
    }

    /**
     * Commands handler dispatcher.
     *
     * @param evt: action event with the command ID
     */
    public void actionPerformed(ActionEvent evt) {
	System.out.println("<<<ThemesForm.actionPerformed()");
	Command cmd = evt.getCommand();

	switch (cmd.getId()) {
	case ResourceConstants.BACK:
		mainForm.show();
		break;
	}
    }

}
