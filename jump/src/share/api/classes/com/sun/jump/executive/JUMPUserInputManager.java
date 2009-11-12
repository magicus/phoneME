/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package com.sun.jump.executive;

import java.net.URL;

/**
 * <Code>JUMPUserInputManager</code> defines methods that allow JUMP 
 * modules to request user feedback as part of their functionality. Examples
 * that need user feedback are asking the user if a particular application
 * can be downloaded or not. The implementation of the manager can present 
 * an appropriate User Interface (GUI or CLI) to get the user feedback. 
 * An instance of <code>JUMPUserInputManager</code> is got from 
 * {@link JUMPExecutive#getUserInputManager()}
 * <p>
 * All the methods block till the user performs some action with the UI
 * presented. All the strings that are passed as arguments that are shown by
 * the dialog MUST be a localized string. The implementation of the API will 
 * simply render those strings.
 * <p>
 * The following sample code shows how the download module can ask the
 * user whether to allow installing an unsigned application.
 * <pre>
 *
 *    public boolean allowUnsignedApp() {
 *       JUMPExecutive executive = JUMPExecutive.getInstance();
 *       JUMPUserInputManager uiManager = executive.getUserInputManager();
 *       return uiManager.showDialog("Allow installing of unsigned app ..."
 *          "OK",
 *          "Cancel");
 *    }
 *
 * </pre>
 */
public abstract class JUMPUserInputManager {
    
    
    /** Creates a new instance of JUMPUserInputManager */
    protected JUMPUserInputManager () {
    }
    
    /**
     * Show the <i>message</i> in a dialog and wait for the user to select
     * <b>OK</b> or <b>Cancel</b> and return the user selection.
     *
     * @param message the message to be displayed in the dialog box
     * @param okString the string that is used to display the OK action. 
     * @param cancelString the string that is used to display the OK action. 
     *
     * @return <code>true</code> if the user pressed OK and <Code>false</code>
     *         if the user pressed cancel or dismissed the dialog.
     */
    public abstract boolean showDialog(String message, 
        URL imageURL,
        String okString, 
        String cancelString);
    
    /**
     * Show the <i>message</i> in a dialog and wait for the user to select
     * <b>OK</b> or <b>Cancel</b> and return the user selection.
     *
     * @param title the message to be displayed in the choice box
     * @param choices a list of strings to show as the choices for the user
     *        to select
     * @param defaultChoices a list of strings (typically a subset of
     *        <i>choices</i>) that should be selected as the default choice.
     * @param isMultiSelection indicates if the user can select one or more
     *        choices.
     * @param okString the string that is used to display the OK action. 
     * @param cancelString the string that is used to display the OK action. 
     * @return <Code>null</code>
     *         if the user pressed cancel or dismissed the choice box or the
     *         list of choices the user selected, which can have one or more
     *         items depending on the multiselection option.
     */
    public abstract String[] showChoices(String title, 
        URL imageURL,
        String[] choices,
        String[] defaultChoices,
        boolean isMultiSelection,
        String okString, 
        String cancelString);
}
