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

package com.sun.appmanager.apprepository;

import java.util.*;
import java.net.*;
import com.sun.appmanager.AppManager;
import com.sun.appmanager.store.PersistentStore;

public class AppModule {
    /**
     * Path to the module's icon image
     */
    private String iconPath = null;

    /**
     * Path to the module's depressed icon image
     */
    private String iconDepressedPath = null;

    /**
     * URL representation of the path to the icon image
     */
    protected URL iconURL = null;

    /**
     * URL representation of the path to the depressed icon image
     */
    protected URL iconDepressedURL = null;

    /**
     * Invocation command to lauch the AppModule
     */
    private String invocation = null;

    /**
     * The title/name of the AppModule
     */
    private String title = null;

    /**
     * Type of program represented by the AppModule.
     * Values can be XLET, APP, SUBLIST.
     */
    private String type = null;

    /**
     * Path to application represented by the AppModule
     */
    private String path = null;

    /*
     * Parameters passed to the VM
     */
    private String vm_params = null;

    /*
     * Parameters passed to the application
     */

    private String app_params = null;

    /*
     * Use absolute or relative path names
     */

    boolean absolutePaths = false;
    /**
     * Name of a main program
     */
    private String mainName = null;

    /**
     * Indicates if the AppModule should be launched
     * upon startup of the application manager
     */
    private String startup = null;

    /**
     * Name of the menu if the AppModule represents a menu
     * instead of an application.
     */
    private String menu = null;

    /**
     * Security level associated with the module.  Currently, the values
     * are TRUSTED and CONSTRAINED.
     */
    private String securityLevel = null;

    /**
     * boolean value determining if this is a system app or user app
     */
    private boolean systemApp = false;

    /**
     * Indicates if the AppModule represents a switchable type of
     * application, like XLET
     */
    private boolean isSwitchable = false;

    /*
     * The (downloadable) bundle that this app belongs to.
     */
    private String bundle = null;

    /**
     * Handle to the system-wide persistent store object
     */
    private PersistentStore ps = null;

    /**
     * Handle to the AppManager ResourceBundle.
     */
    private ResourceBundle rb = null;

    private final String pathSeparator = System.getProperty("path.separator");
    private final String fileSeparator = System.getProperty("file.separator");

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));

    /**
     *
     * @param hash table containing AppModule properties
     */
    public AppModule(Hashtable hash) {


        this.ps = AppManager.getPersistentStore();
        this.rb = AppManager.getResourceBundle();

        // Type of app; i.e. APP, XLET, SUBMENU,
        this.type = (String) hash.get("type");

        this.path = (String) hash.get("path");
        if (this.path == null && !this.type.equals("SUBLIST")) {
            if (verbose) {
               System.out.println(rb.getString("PathNotSpecified"));
            }
            return;
        }
// Title of app which is usually displayed in an icon
        this.title = (String) hash.get("title");

// If defined, the menu this object belongs to.
        this.menu = (String) hash.get("menu");

// VM parameters
        this.vm_params = (String) hash.get("vm_params");

// Application parameters
        this.app_params = (String) hash.get("app_params");

// Security level; i.e. TRUSTED, CONSTRAINED
        this.securityLevel = (String) hash.get("securityLevel");

// Should the app launch upon startup of the appmanager; true/false
        this.startup = (String) hash.get("startup");

// Determine the bundle that this app belongs to.  If a bundle isn't
// specified, the default is to use the name of the jarfile
// and parent directory, which are both equal.  We need to do some
// String manipulations in that case to get the name.
        if (!this.type.equals("SUBLIST")) {
            String bundleStr = (String) hash.get("bundle");
            if (bundleStr == null) {
                String tmpPath = this.path;
                int index = tmpPath.indexOf(pathSeparator);
                if (index != -1) {
                    tmpPath = tmpPath.substring(0, index);
                }
                index = tmpPath.indexOf(".jar");
                if (index != -1) {
                    tmpPath = tmpPath.substring(0, index);
                }
                index = tmpPath.lastIndexOf(fileSeparator);
                if (index != -1) {
                    tmpPath = tmpPath.substring(index + 1, tmpPath.length());
                }
                if (tmpPath != null) {
                    this.bundle = tmpPath;
                }
            }
            else {
                this.bundle = bundleStr;
            }
        }

// Determination between a system and a user app
        String value = (String) hash.get("systemApp");
        if (value == null) {
            this.systemApp = false;
        }
        else if (value.toLowerCase().equals("true") ||
                 value.toLowerCase().equals("yes")) {
            this.systemApp = true;
        }
        else {
            this.systemApp = false;
        }

// Should the app look for files using absolute paths or paths relative
// to the repository directory; true/false
        String useAbsolutePaths = (String) hash.get("absolutePaths");
        if (useAbsolutePaths == null) {
            this.absolutePaths = false;
        }
        else if (useAbsolutePaths.toLowerCase().equals("true") ||
                 useAbsolutePaths.toLowerCase().equals("yes")) {
            this.absolutePaths = true;
        }
        else {
            this.absolutePaths = false;
        }

// Icon image that represents this object
        this.iconPath = (String) hash.get("icon");
        try {
            this.iconURL = new URL("file", "", -1,
                                   ps.absolutePathOf("icons/" + iconPath));
        }
        catch (Exception e) {
            e.printStackTrace();
        }

// Depressed icon image that represents this object
        this.iconDepressedPath = (String) hash.get("icon.depressed");
        try {
            this.iconDepressedURL = new URL("file", "", -1,
                                            ps.absolutePathOf("icons/" +
                iconDepressedPath));
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        if (this.type.equals("APP")) {
            appInvocation(hash);
        }
        else if (this.type.equals("SUBLIST")) {
            if (verbose) {
                System.out.println(rb.getString("SublistTitle") +
                               this.title);
            }
        }
        else if (this.type.equals("XLET")) {
            xletInvocation(hash);
        }
    }

    private String adjustPath(String path) {

        String str = new String();
        StringTokenizer st = new StringTokenizer(path, pathSeparator);
        int numTokens = st.countTokens();
        for (int i = 0; i < numTokens; i++) {
            if (i == 0) {
                str = ps.absolutePathOf(st.nextToken());
            }
            else {
                str = str + pathSeparator +
                    ps.absolutePathOf(st.nextToken());
            }
        }
        return str;
    }

    private void appInvocation(Hashtable hash) {

        if (!absolutePaths) {
            this.path = adjustPath(this.path);
        }

        String mainClass = (String) hash.get("mainClass");

        this.invocation = "";

        if (this.vm_params != null) {
            this.invocation = this.invocation.concat(this.vm_params + " ");
        }

        this.invocation = this.invocation.concat("-cp " + this.path + " " +
                                                 mainClass);

        if (this.app_params != null) {
            this.invocation = this.invocation.concat(" " + this.app_params);
        }

        if (verbose) {
            System.err.println(rb.getString("AppTitle") +
                           this.title +
                           rb.getString("Invocation") +
                           this.invocation);
        }
    }

    /**
     *
     * @param path path to application files
     * @param name name of main xlet class
     * @return invocation command to launch the xlet
     */
    private void xletInvocation(Hashtable hash) {

// Fix up the path after determining to use absolute or relative paths
        if (!absolutePaths) {
            this.path = adjustPath(this.path);
        }

        this.mainName = (String) hash.get("xletName");

        String inv = "";

        inv += " -cp " + this.path;

        // Add security stuff
        if (this.securityLevel != null) {
            String policyFile = null;

            String cdcamsHome = System.getProperty("cdcams.home");
            if (cdcamsHome == null) {
                throw new RuntimeException("Need cdcams.home");
            }
            if (this.securityLevel.equals(AppManager.getResourceBundle().
                                          getString(
                                              "SecurityConstrained"))) {
                policyFile = cdcamsHome + fileSeparator + "lib" +
                    fileSeparator + "security" + fileSeparator +
                    "appmanager.security.constrained";
            }
            else if (this.securityLevel.equals(AppManager.getResourceBundle().
                                               getString("SecurityTrusted"))) {
                policyFile = cdcamsHome + fileSeparator + "lib" +
                    fileSeparator + "security" + fileSeparator +
                    "appmanager.security.permissive";
            }
            else {
                throw new RuntimeException("Unrecognized security level: " +
                                           this.securityLevel);
            }
            inv += " -Djava.security.policy=" + policyFile;
        }

        inv += " sun.mtask.xlet.PXletRunner -name " + this.mainName + " -path "
               + this.path;

        if (this.vm_params != null) {
            inv = this.vm_params + " " + inv;
        }
        if (this.app_params != null) {
            inv = inv.concat(" -args" + this.app_params);
        }

        this.invocation = inv;

        if (verbose) {
            System.err.println(rb.getString("XletTitle") +
                           this.title + rb.getString("Invocation") +
                           this.invocation);
        }

    }

    /**
     *
     * @return title and invocation command of the module
     */
    public String toString() {
        return "[AppModule: title=" + getProgramTitle() + ", invocation=" +
            getInvocation() + "]";
    }

    /**
     *
     * @param s string to print
     * @return printable version of the string to print
     */
    private String getPrintable(String s) {
        if (s == null) {
            return "<null>";
        }
        else {
            return "\"" + s + "\"";
        }
    }

    /**
     *
     * @return printable version of the title of the module
     */
    private String getProgramTitle() {
        return getPrintable(title);
    }

    /**
     *
     * @return printable version of the invocation command of the module
     */
    public String getInvocation() {
        return getPrintable(invocation);
    }

    /**
     * Launch the app and return a string handle
     * @return string representation of application id.
     */
    public String launch() {

        if (type.equals("SUBLIST")) {
            if (verbose) {
                System.out.println(rb.getString("GoingToLaunch") +
                               this);
            }
            return null;
        }

        if (invocation == null) {
            if (verbose) {
                System.err.println(rb.getString("DontKnowHow") +
                               this);
            }
            return null;
        }

        // Do we need a Resource lookup for " "?
        StringTokenizer st = new StringTokenizer(invocation, " ");
        int numTokens = st.countTokens();
        String[] args = new String[numTokens];
        for (int i = 0; i < numTokens; i++) {
            args[i] = st.nextToken();
        }
        return AppManager.getMtaskClient().launch("J" + type, args);
    }

    /**
     *
     * @return title of application module
     */
    public String getTitle() {
        return title;
    }

    /**
     *
     * @return type of application module
     */
    public String getType() {
        return type;
    }

    /**
     *
     * @return true if the module is a submenu, false otherwise
     */
    public boolean isSublist() {
        if (type.equals("SUBLIST")) {
            return true;
        }
        else {
            return false;
        }
    }

    /**
     *
     * @return boolean if this app is a system app
     */
    public boolean isSystemApp() {
        return systemApp;
    }

    /**
     *
     * @return path to application files of the module
     */
    public String getPath() {
        return path;
    }

    /**
     *
     * @return path to the module's icon image
     */
    public String getIconPath() {
        return iconPath;
    }

    /**
     *
     * @return path to the module's depressed icon image
     */
    public String getIconDepressedPath() {
        return iconDepressedPath;
    }

    /**
     *
     * @return true if a special-case ticker module, false otherwise
     */
    public boolean getIsSwitchable() {
        return isSwitchable;
    }

    /**
     *
     * @return URL representation of the path to the module's
     *  icon image
     */
    public URL getIconURL() {
        return this.iconURL;
    }

    /**
     *
     * @return URL representation of the path to the module's depressed
     *  icon image
     */
    public URL getIconDepressedURL() {
        return this.iconDepressedURL;
    }

    /**
     *
     * @return true if this is a startup application, false otherwise
     */
    public boolean getIsStartup() {
        if (startup == null) {
            return false;
        }
        else if (startup.toLowerCase().equals("yes") ||
                 startup.toLowerCase().equals("true")) {
            return true;
        }
        else {
            return false;
        }
    }

    /**
     *
     * @return name of the menu if this module represents a menu, null
     * if the module does not represent a menu.
     */
    public String getMenu() {
        return menu;
    }

    /**
     *
     * @return The security level setting for this module
     */
    public String getSecurityLevel() {
        return securityLevel;
    }

    /**
     *
     * @return The bundle this app belongs to.
     */
    public String getBundle() {
        return bundle;
    }

}
