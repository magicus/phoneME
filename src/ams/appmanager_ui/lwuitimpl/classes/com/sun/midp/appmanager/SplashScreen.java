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

import java.util.Timer;
import java.util.TimerTask;

import com.sun.midp.installer.GraphicalInstaller;

import com.sun.lwuit.Image;
import com.sun.lwuit.Display;
import com.sun.lwuit.Form;
import com.sun.lwuit.Label;
import com.sun.lwuit.Container;

import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.animations.Transition3D;
import com.sun.lwuit.animations.CommonTransitions;


/** Implements the splash screen */
class SplashScreen extends Form {

    private final int SPLASH_SCREEN_DELAY = 2000;

    /** Splash screen image */
    private Image splashScreen;
    /** The form to be displayed after SplashScreen is dismissed. */
    private Form mainForm;
    /* Task to switch to main form */
    private TimerTask timerTask;
    /* A timer to schedule the timeout task */
    private Timer timeoutTimer;
    /* splash screen image */
    private Image splashImage;

    private Label splashLabel;

    private Container splashContainer;

    private int runSpeed = 500;

    private Transition in, out;

    SplashScreen(Form mainForm) {
	int width, height;
	javax.microedition.lcdui.Image sourceImage = null;

	this.mainForm = mainForm;

	width = Display.getInstance().getDisplayWidth();
	height = Display.getInstance().getDisplayHeight();

	timerTask = new TimeoutTask();
	timeoutTimer = new Timer();
	splashImage = null;

	out = Transition3D.createFlyIn(runSpeed);
	in = Transition3D.createFlyIn(runSpeed);

	setTransitionOutAnimator(out);
	setTransitionInAnimator(in);



//TODO:  this block causes exception
//         sourceImage = GraphicalInstaller.getImageFromInternalStorage("
// 		splash_screen_" + width + "x" + height);

	splashImage = AppManagerUIImpl.convertImage(sourceImage);

	if (splashImage != null) {
	    splashLabel = new Label(splashImage);
	}
	else {
	    splashLabel = new Label("Welcome to Java Wireless Client!");
	}

	addComponent(splashLabel);
    }

    /* Set a timer to switch to the App Manager Screen. */
    public void showNotify() {
	timeoutTimer.schedule(timerTask, SPLASH_SCREEN_DELAY);
    }

    /**
     * This method is called when available area of
     * the Displayable has been changed.
     */
    protected  void  sizeChanged(int w, int h) {
	/* TODO:  resize image */
    }

    /**
     * Override hideNotify  to cancel timer task.
     */
    public void hideNotify() {
	timerTask.cancel();
    }

    /**
     * A TimerTask subclass which will switch to the App Manager after
     * a time out time set.
     */
    private class TimeoutTask extends TimerTask {
        /* Switch to main form */
        public void run() {
	    mainForm.show();
        }
    }
}
