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
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.installer.DynamicComponentInstaller;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midletsuite.DynamicComponentStorage;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.amsservices.ComponentInfo;

import javax.microedition.lcdui.*;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;
import java.io.IOException;

/**
 * Component Manager
 *
 * Implements a built-in MIDlet that manages the dynamically installed components.
 */
public class ComponentManager extends MIDlet {

    /**
     * Constructor: launches the component view screen.
     */
    public ComponentManager() {
        new ComponentView(MIDletSuite.INTERNAL_SUITE_ID, this, true);
    }

    /**
     * Signals the <code>MIDlet</code> that it has entered the
     * <em>Active</em> state.
     * In the <em>Active</EM> state the <code>MIDlet</code> may
     * hold resources.
     * The method will only be called when
     * the <code>MIDlet</code> is in the <em>Paused</em> state.
     * <p/>
     * Two kinds of failures can prevent the service from starting,
     * transient and non-transient.  For transient failures the
     * <code>MIDletStateChangeException</code> exception should be thrown.
     * For non-transient failures the <code>notifyDestroyed</code>
     * method should be called.
     * <p/>
     * If a Runtime exception occurs during <code>startApp</code> the
     * MIDlet will be
     * destroyed immediately.  Its <code>destroyApp</code> will be
     * called allowing
     * the MIDlet to cleanup.
     *
     * @throws javax.microedition.midlet.MIDletStateChangeException
     *          is thrown
     *          if the <code>MIDlet</code>
     *          cannot start now but might be able to start at a
     *          later time.
     */
    protected void startApp() throws MIDletStateChangeException {
    }

    /**
     * Signals the <code>MIDlet</code> to enter
     * the <em>Paused</em> state.
     * In the <em>Paused</em> state the <code>MIDlet</code> must
     * release shared resources
     * and become quiescent. This method will only be called
     * called when the <code>MIDlet</code> is in the <em>Active</em> state. <p>
     * <p/>
     * If a Runtime exception occurs during <code>pauseApp</code> the
     * MIDlet will be destroyed immediately.  Its
     * <code>destroyApp</code> will be called allowing
     * the MIDlet to cleanup.
     */
    protected void pauseApp() {
    }

    /**
     * Signals the <code>MIDlet</code> to terminate and enter the
     * <em>Destroyed</em> state.
     * In the destroyed state the <code>MIDlet</code> must release
     * all resources and save any persistent state. This method may
     * be called from the <em>Paused</em> or
     * <em>Active</em> states. <p>
     * <code>MIDlet</code>s should
     * perform any operations required before being terminated, such as
     * releasing resources or saving preferences or
     * state. <p>
     * <p/>
     * <strong>Note:</strong> The <code>MIDlet</code> can request that
     * it not enter the <em>Destroyed</em>
     * state by throwing an <code>MIDletStateChangeException</code>. This
     * is only a valid response if the <code>unconditional</code>
     * flag is set to <code>false</code>. If it is <code>true</code>
     * the <code>MIDlet</code> is assumed to be in the <em>Destroyed</em> state
     * regardless of how this method terminates. If it is not an
     * unconditional request, the <code>MIDlet</code> can signify that it
     * wishes to stay in its current state by throwing the
     * <code>MIDletStateChangeException</code>.
     * This request may be honored and the <code>destroy()</code>
     * method called again at a later time.
     * <p/>
     * <p>If a Runtime exception occurs during <code>destroyApp</code> then
     * they are ignored and the MIDlet is put into the <em>Destroyed</em>
     * state.
     *
     * @param unconditional If true when this method is called, the
     *                      <code>MIDlet</code> must cleanup and release all resources.  If
     *                      false the <code>MIDlet</code> may throw
     *                      <CODE>MIDletStateChangeException</CODE> to indicate it does not
     *                      want to be destroyed at this time.
     * @throws javax.microedition.midlet.MIDletStateChangeException
     *          is thrown
     *          if the <code>MIDlet</code> wishes to continue to
     *          execute (Not enter the <em>Destroyed</em> state).
     *          This exception is ignored if <code>unconditional</code>
     *          is equal to <code>true</code>.
     */
    protected void destroyApp(boolean unconditional) throws MIDletStateChangeException {
    }

    /**
     * A screen with a list of MIDlet components.
     *
     * Can provide read-only or full access to the user.
     * May serve as the main screen of a MIDlet (in which case the MIDlet
     * will terminate when the user exits the component list screen) or
     * be called from some other screen (in which case that screen will
     * be shown after exiting the the component list screen).
     */
    static class ComponentView implements CommandListener, ItemCommandListener {

        /** Command object for "Back" command in the suite list form. */
        private Command backCmd = new Command(Resource.getString
                                              (ResourceConstants.BACK),
                                              Command.BACK, 1);

        /** Command object for "Back" command in the message form. */
        private Command messageBackCmd = new Command(Resource.getString
                                                     (ResourceConstants.BACK),
                                                     Command.BACK, 1);

        /** Command object for "Launch". */
        private Command openCmd =
            new Command(Resource.getString(ResourceConstants.OPEN),
                        Command.ITEM, 1);

        /** Command object for "Install" command for the suite list form. */
        private Command installCmd =
            new Command(Resource.getString(ResourceConstants.INSTALL),
                        Command.SCREEN, 1);

        /** Command object for "Back" command in the install screen. */
        private Command installNoCmd = new Command(Resource.getString
                                              (ResourceConstants.BACK),
                                              Command.BACK, 1);

        /** Command object for "Yes, Install" command for the install screen. */
        private Command installYesCmd =
            new Command(Resource.getString(ResourceConstants.INSTALL),
                        Command.SCREEN, 1);

        /** Command object for "Remove" in the suite list form. */
        private Command removeCmd =
            new Command(Resource.getString(ResourceConstants.REMOVE),
                        Command.ITEM, 3);

        /**
         * The Display object corresponding to the parent MIDlet.
         * If not null, the current displayable is saved in the constructor
         * to be restored when the user selects the "Back" command in
         * the component list Form.
         */
        private Display display;

        /**
         * If not null, the Displayable to be restored
         * when the user selects the "Back" command in the component list Form.
         */
        private Displayable parentDisplayable;

        /**
         * The parent MIDlet. If not null, this MIDlet receives NotifyDestroyed()
         * when the user selects the "Back" command in the component list Form.
         */
        private MIDlet midlet;

        /** ID of the MIDlet suite whose components we are interested in. */
        private int suiteId;

        /** false for read-only access, true for full access. */
        private boolean mayModify;

        /** The component list Form */
        private Form compList = new Form(null);

        /** The "install component from url" Form */
        private Form installUrlForm = null;

        /** The component url text field for installUrlForm */
        private TextField installUrlField = null ;

        /** The component name text field for installUrlForm */
        private TextField nameField = null ;

        /** A form for displaying a message */
        private Form messageForm = null;

        /**
         * All information about suite components is kept in this
         * array of ComponentInfo objects.
         */
        private ComponentInfo[] ci;

        /**
         * Use this constructor from a MIDlet having no screen
         * other than the ComponentView screen. Such wrapper MIDlet will
         * terminate when the user leaves the component list screen.  
         * @param suiteId0 the ID of the suite of interest
         * @param midlet0 the MIDlet that must be terminated when the user leaves the component list screen
         * @param canModify0 true for full access, false for read-only access
         */
        ComponentView(int suiteId0, MIDlet midlet0, boolean canModify0) {
            suiteId = suiteId0;
            midlet = midlet0;
            mayModify = canModify0;
            display = Display.getDisplay(midlet);
            refresh();
        }

        /**
         * Use this constructor when there is a screen where to return
         * when the user leaves the component list screen.
         * @param suiteId0 the ID of the suite of interest
         * @param display0 the Display object currently in use.
         * @param canModify0 true for full access, false for read-only access
         */
        ComponentView(int suiteId0, Display display0, boolean canModify0) {
            suiteId = suiteId0;
            display = display0;
            mayModify = canModify0;
            parentDisplayable = display.getCurrent();
            refresh();
        }

        /**
         * Displays a message
         * @param message a message to display
         * @param title form title; if null, "Error" is used
         */
        private void showMessage(String message, String title) {
            if (messageForm == null) {
                messageForm = new Form("");
            }

            if (title != null) {
                messageForm.setTitle(title);
            }

            messageForm.deleteAll();
            messageForm.append(message);
            messageForm.addCommand(messageBackCmd);
            messageForm.setCommandListener(this);

            display.setCurrent(messageForm);
        }

        /**
         * Create a Form for the "Install Component" dialog.
         * The form is stored into the installUrlForm field.
         */
        private void makeInstallDialog() {
            if (installUrlForm == null) {
                installUrlForm = new Form(
                        Resource.getString(
                            ResourceConstants.AMS_CMGR_INSTALL_COMPONENT));

                String componentUrl = Constants.AMS_CMGR_DEFAULT_COMPONENT_URL;

                installUrlField = new TextField(
                        Resource.getString(
                            ResourceConstants.AMS_CMGR_ENTER_URL_TO_INSTALL_FROM),
                        componentUrl, 1024,
                        TextField.URL);

                nameField = new TextField(
                        Resource.getString(
                            ResourceConstants.AMS_CMGR_ENTER_DESCRIPTIVE_NAME),
                        "", 1024,
                        TextField.ANY);

                installUrlForm.append(installUrlField);
                installUrlForm.append(nameField);
            }

            installUrlForm.addCommand(installNoCmd);
            installUrlForm.addCommand(installYesCmd);
            display.setCurrent(installUrlForm);
            installUrlForm.setCommandListener(this);
        }

        /**
         * Find an item in the component list form.
         * @param item the item to search for
         * @return the index of the item in the form, or -1 if not found.
         */
        protected int findItem(Item item) {
            for (int i=0; i<compList.size(); i++) {
                if (compList.get(i) == item) {
                    return i;
                }
            }
            return -1;
        }

        /**
         * Given an item, find the corresponding ComponentInfo object.
         * @param item the item to search for
         * @return the corresponding ComponentInfo object.
         */
        protected ComponentInfo findComponent(Item item) {
            int i = findItem(item);
            return i == -1 ? null : ci[i];
        }

        /**
         * Update content and show the component list screen.
         */
        private void refresh() {
             updateContent();
             compList.setCommandListener(this);
             display.setCurrent(compList);
         }

        /**
         * Re-read the information about installed components, update
         * the form compList and the array ci.
         */
        private void updateContent() {
             DynamicComponentStorage dcs =
                     DynamicComponentStorage.getComponentStorage();

             try {
                 ci = dcs.getListOfSuiteComponents(suiteId);
             } catch (IllegalArgumentException iae) {
                 iae.printStackTrace();
                 ci = null;
             }

             compList.setTitle(ci == null ? "Components: none" :
                                            "Components: " + ci.length);
             if (ci != null) {
                 for (int i = 0; i < ci.length; i++) {
                     if (i >= compList.size()) {
                         compList.append(new StringItem(
                             null,ci[i].getDisplayName() + "\n"));
                     } else {
                         ((StringItem)compList.get(i)).setText(
                             ci[i].getDisplayName()+"\n");
                     }

                     //compList.get(i).addCommand(backCmd);
                     compList.get(i).setDefaultCommand(openCmd);
                     if (mayModify) {
                        compList.get(i).addCommand(removeCmd);
                     }
                     compList.get(i).setItemCommandListener(this);
                 }
                 for (int i = compList.size() - 1; i >= ci.length; i--) {
                     compList.delete(i);
                 }
             } else {
                 compList.deleteAll();
             }

             compList.addCommand(backCmd);

             if (mayModify) {
                compList.addCommand(installCmd);
             }
         }

        /**
         * Indicates that a command event has occurred on
         * <code>Displayable d</code>.
         *
         * @param c a <code>Command</code> object identifying the
         *          command. This is either one of the
         *          applications have been added to <code>Displayable</code> with
         *          {@link javax.microedition.lcdui.Displayable#addCommand(javax.microedition.lcdui.Command)
         *          addCommand(Command)} or is the implicit
         *          {@link javax.microedition.lcdui.List#SELECT_COMMAND SELECT_COMMAND} of
         *          <code>List</code>.
         * @param d the <code>Displayable</code> on which this event
         *          has occurred
         */
        public void commandAction(Command c, Displayable d) {
            if (c == backCmd) {
                if (parentDisplayable != null) {
                    display.setCurrent(parentDisplayable);
                } else {
                    midlet.notifyDestroyed();
                }
            } else if (c == installCmd) {
                makeInstallDialog();
            } else if (c == installNoCmd) {
                refresh();
            } else if (c == installYesCmd) {
                DynamicComponentInstaller installer = new DynamicComponentInstaller();
                try {
                    int componentId = installer.installComponent(suiteId,
                            installUrlField.getString().trim(),
                            nameField.getString().trim());
                } catch (IOException e) {
                    showMessage(e.toString(), null);
                    return;
                } catch (MIDletSuiteLockedException msle) {
                    showMessage("The component is being used now.", null);
                    return;
                }

                refresh();

            } else if (c == messageBackCmd) {
                refresh();
            }
        }


        /**
         * Called by the system to indicate that a command has been invoked on a
         * particular item.
         *
         * @param c    the <code>Command</code> that was invoked
         * @param item the <code>Item</code> on which the command was invoked
         */
        public void commandAction(Command c, Item item) {
            ComponentInfo cinfo = findComponent(item);
            if (cinfo==null) {
                return;
            }
            int componentId = cinfo.getComponentId();
            if (c == removeCmd) {
                DynamicComponentStorage dcs =
                        DynamicComponentStorage.getComponentStorage();
                
                try {
                    dcs.removeComponent(componentId);
                } catch (MIDletSuiteLockedException msle) {
                    showMessage("Component is in use: " +
                                    msle.getMessage(), null);
                    return;
                }
                refresh();
                
            } else if (c == openCmd) {
                showMessage("component info: " + cinfo.toString(), "Info");
            }
        }

     }
}
