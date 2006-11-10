/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.impl;

import java.util.*;
import com.sun.appmanager.apprepository.AppModule;
import com.sun.appmanager.mtask.Client;
import com.sun.appmanager.appmodel.AppModelController;
import com.sun.appmanager.appmodel.XletAppModelController;
import com.sun.appmanager.mtask.TaskListener;

public class CDCAmsAppController implements TaskListener {

    // Currently window-managed set of apps
    private Vector windowManagedApps = new Vector();
    // Topmost active app
    private String topmostApp = null;
    // A mapping of running appId to AppModule
    private Hashtable appIdToAppModule = new Hashtable(13);

    protected Client mtaskClient = null;

    private XletAppModelController xletController = null;
    private ResourceBundle bundle = null;

    public CDCAmsAppController(Client mtaskClient) {
        this.mtaskClient = mtaskClient;
	this.xletController = (XletAppModelController)
	    AppModelController.getAppModelController(AppModelController.XLET_APP_MODEL, mtaskClient);
    }

    public String launchApp(AppModule app) {
        return launchApp(app, true);
    }

    public String launchApp(AppModule app, boolean isSwitchable) {
        // About to launch an app.
        // First, deactivate the current top task
        if (isSwitchable) {
            deactivateTopTask();
        }
        String appId = app.launch();
        if (appId == null) {
            System.out.println(getString("LaunchAppFailed"));
            if (isSwitchable) {
                reactivateTopTask();
            }
            return null;
        }
//        System.err.println(getString("LaunchAppOK") + appId);
        associateAppModule(appId, app);

        if (!isSwitchable) {
            return appId;
        }

        addWindowManagedApp(appId);
        setTopmostApp(appId);

        return appId;
    }

    public boolean pauseTask(String taskId) {
        return xletController.xletPause(taskId);
    }

    public boolean startTask(String taskId) {
        return xletController.xletStart(taskId);
    }

    public String[] listRunningApps() {
        return mtaskClient.list();
    }

    public boolean killTask(String taskId) {
        AppModule module = getAppModule(taskId);

        boolean result = false;

        if (module.getType().equals("XLET")) {
            result = xletController.xletDestroy(taskId);
        }
        else {
            result = mtaskClient.kill(taskId);
        }

        if (result) {
        // this method does the "switching" to another app after the removal
        // takes place.  Note, the disassociateAppModule takes place in
        // the getDeadApps() area.
            removeWindowManagedApp(taskId);
        }

        return result;
    }

    public boolean killAllTasks() {
        return mtaskClient.killall();
    }

    private String getTopmostApp() {
        return topmostApp;
    }

    public String getCurrentApp() {
        return getTopmostApp();
    }

    private String[] getAppsArray(Vector apps) {
        String[] l = new String[apps.size()];
        apps.copyInto(l);
        return l;
    }

    public String[] getWindowManagedApps() {
	return getAppsArray(windowManagedApps);
    }

    public void activate(String appId) {
        if (appId == null) {
            return;
        }
        if (!appId.equals(topmostApp)) {
//            System.err.println(getString("Activating") + appId);
            if (topmostApp != null) {
                deactivateTopTask(); // deactivate if it exists
            }
	    xletController.xletActivate(appId);
            setTopmostApp(appId);
        }
    }

    public void deactivate(String appId) {
        if (appId == null) {
            return;
        }
//        System.err.println(getString("Deactivating") + appId);
	xletController.xletDeactivate(appId);

        if (appId.equals(topmostApp)) {
            // We were at the top
            // There is no top anymore
            setTopmostApp(null);
        }
    }

    public void deactivateTopTask() {
        deactivate(getTopmostApp());
    }

    public void reactivateTopTask() {
	setTopTaskAndActivate();
    }

    public void setXletDimensions(int x, int y, int w, int h,
                                  int wOffset, int hOffset) {
        mtaskClient.source(new String[] {
                          "sun.mtask.xlet.XletFrame",
                          String.valueOf(x),
                          String.valueOf(y),
                          String.valueOf(w),
                          String.valueOf(h),
                          String.valueOf(wOffset),
                          String.valueOf(hOffset)
        });
    }

    public void reboot() {
        mtaskClient.killall();
        mtaskClient.exit();
        System.exit(0);
    }

    public Client getMtaskClient() {
        return mtaskClient;
    }

    private void associateAppModule(String appId, AppModule module) {
        appIdToAppModule.put(appId, module);
    }

    private void disassociateAppModule(String appId) {
        appIdToAppModule.remove(appId);
    }

    public AppModule getAppModule(String appId) {
        return (AppModule) appIdToAppModule.get(appId);
    }

    private void addWindowManagedApp(String appId) {
        windowManagedApps.addElement(appId);
    }

    private void removeWindowManagedApp(String appId) {
        if (appId == null) {
            return;
        }
	// Someone beat us to it. That's OK.
	if (!windowManagedApps.contains(appId)) {
	    return;
	}
        windowManagedApps.removeElement(appId);
        if (appId.equals(topmostApp)) {
            // We were at the top
            // There is no top anymore
            setTopmostApp(null);
        }
    }

    private void setTopmostApp(String appId) {
        // Shuffle active apps to remember latest guy
        if (appId != null) {
            windowManagedApps.removeElement(appId);
            windowManagedApps.addElement(appId);
        }

        topmostApp = appId;
    }

    private void setTopTaskAndActivate() {
        if (windowManagedApps.size() == 0) {
            setTopmostApp(null);
            return;
        }

        // Get the most recently activated guy
        String appId = (String) windowManagedApps.lastElement();
        activate(appId);
    }

    // We are getting a string of the form: PID=%d COMMAND=".."
    String nameOf(String listItem) {
        int start = listItem.indexOf(' ');
        start += 9; // past COMMAND=
        return listItem.substring(start);
    }

    /*
    // We are getting a string of the form: PID=%d COMMAND="J<type>.."
    // Extract type substring following the J command
    public String typeOf(String listItem) {
        String name = nameOf(listItem);
        int end = name.indexOf(' ');
        int start = 2; // past the quote and the 'J'
        return name.substring(start, end);
    }
    */

    // App list parsing
    // We are getting a string of the form: PID=%d COMMAND=".."
    public String appIdOf(String listItem) {
        int end = listItem.indexOf(' ');
        int start = 4; // Right past PID=
        return listItem.substring(start, end);
    }

    /**
     * This method returns a string from the demo's resource bundle.
     */
    private String getString(String key) {
        String value = null;

        try {
            value = getResourceBundle().getString(key);
        }
        catch (MissingResourceException e) {
            System.out.println("java.util.MissingResourceException: " +
                               "Couldn't find value for: " + key);
        }

        //System.err.println("getString("+key+") = "+value);

        return value;
    }

    /**
     * Returns the resource bundle associated with this demo. Used
     * to get accessable and internationalized strings.
     */
    private ResourceBundle getResourceBundle() {
        if (bundle == null) {
            bundle = ResourceBundle.getBundle(
                "com.sun.appmanager.resources.AppManagerResources");
            System.out.println("bundle");
        }

        return bundle;
    }

   // CDCAmsAppControllerListener's handler for
   // AppManager's task_killed events.
   public void taskEvent(String appID, int what) {
       if (what == TaskListener.CDCAMS_TASK_KILLED) {
	   removeWindowManagedApp(appID);
	   disassociateAppModule(appID);
       }
   }
}
