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

package com.sun.midp.appmanager;

import com.sun.midp.configurator.Constants;

import com.sun.midp.main.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import javax.microedition.lcdui.Displayable;


interface TaskManagerUI {

    /**
     * The AppManager manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemAppended callback when new item is appended to the list.
     *  
     * @param suiteInfo
     */
    void itemAppended(RunningMIDletSuiteInfo suiteInfo);

    /**
     * The AppManager manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemRemoved callback when item is removed from the list.
     *
     * @param suiteInfo
     */
    void itemRemoved(RunningMIDletSuiteInfo suiteInfo);

    /**
     *  Called when a new internal midlet was launched
     * @param midlet proxy of a newly launched MIDlet
     */
    void notifyInternalMidletStarted(MIDletProxy midlet);

    /**
     * Called when a new not internal midlet was launched.
     *
     * @param si corresponding midlet suite info
     */
    void notifyMidletStarted(RunningMIDletSuiteInfo si);

    /**
     * Called when state of a running midlet has changed.
     *
     * @param si corresponding midlet suite info
     */
    void notifyMidletStateChanged(RunningMIDletSuiteInfo si);

    /**
     * Called when a running internal midlet exited.
     * @param midlet
     */
    void notifyInternalMidletExited(MIDletProxy midlet);

    /**
     * Called when a running non internal midlet exited.
     * @param si corresponding midlet suite info
     */
    void notifyMidletExited(RunningMIDletSuiteInfo si);

    /**
     * Called by ApplicationManager after a MIDlet suite
     * is successfully installed on the device,
     * to ask the user whether or not to launch
     * a MIDlet from the suite. 
     * @param si corresponding suite info
     */
    void notifySuiteInstalled(RunningMIDletSuiteInfo si);

    /**
     * Called when a new MIDlet suite is installed externally.
     * @param si corresponding suite info
     */
    void notifySuiteInstalledExt(RunningMIDletSuiteInfo si);

    /**
     * Called when a MIDlet suite has been removed externally.
     * @param si corresponding suite info
     */
    void notifySuiteRemovedExt(RunningMIDletSuiteInfo si);

    /**
     * Called when MIDlet suite being enabled
     * @param si corresponding suite info
     */
    void notifyMIDletSuiteEnabled(RunningMIDletSuiteInfo si);

    /**
     * Called when MIDlet suite icon hase changed
     * @param si corresponding suite info
     */
    void notifyMIDletSuiteIconChaged(RunningMIDletSuiteInfo si);

    /**
     * Called when a midlet could not be launched.
     *
     * @param suiteId suite ID of the MIDlet
     * @param className class name of the MIDlet
     * @param errorCode error code
     * @param errorDetails error code details
     */
    void notifyMidletStartError(int suiteId, String className, int errorCode,
                                String errorDetails);

    void notifyMIDletSuiteStateChaged(RunningMIDletSuiteInfo si,
                                             RunningMIDletSuiteInfo newSi);

    void setCurrentItem(RunningMIDletSuiteInfo item);

    /**
     * Called to determine MidletSuiteInfo of the last selected Item.
     * Is used to restore selection in the task manager.
     *
     * @return last selected MidletSuiteInfo
     */
    RunningMIDletSuiteInfo getSelectedMIDletSuiteInfo();

    /**
     * Called when midlet selector needed.
     *
     * @param onlyFromLaunchedList true if midlet should
     *        be selected from the list of already launched midlets,
     *        if false then possibility to launch midlet is needed.
     */
    void showMidletSwitcher(boolean onlyFromLaunchedList);

    /**
     * Called by Manager when destroyApp happens to clean up data.
     */
    void cleanUp();

    Displayable getMainDisplayable();
    


}


