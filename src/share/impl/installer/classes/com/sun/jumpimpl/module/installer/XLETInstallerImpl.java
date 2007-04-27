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

package com.sun.jumpimpl.module.installer;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPApplication;
//import com.sun.jump.executive.JUMPUserInputManager;
import com.sun.jump.module.contentstore.JUMPContentStore;
import com.sun.jump.module.contentstore.JUMPData;
import com.sun.jump.module.contentstore.JUMPNode;
import com.sun.jump.module.contentstore.JUMPStore;
import com.sun.jump.module.contentstore.JUMPStoreFactory;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.module.installer.JUMPInstallerModule;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
import java.util.Vector;
import java.util.jar.JarFile;
import java.util.zip.ZipEntry;

/**
 * XLETInstallerImpl contains the implementation of the JUMPInstallerModule
 * for XLET applications within JUMP.
 * Note: XLET and Main installations within JUMP in CDC behave almost the same.
 * For this reason, MAINInstallerImpl simply subclasses XLETInstallerImpl
 * and overrides a small set of methods to define specific Main application
 * behavior.  In the future, if the behavior of XLET and Main application
 * installation start to differ much more, it may be wise to extract a base
 * class out of this class and create a subclass each for XLETs and Main
 * applications.
 */
public class XLETInstallerImpl extends JUMPContentStore implements JUMPInstallerModule {
    /**
     * The filename extention for application descriptor files.
     */
    private final static String APP_DESCRIPTOR_EXTENSION = ".app";
    /**
     * The name of the directory to hold XLET and Main applications
     */
    private final static String REPOSITORY_APPS_DIRNAME = "./apps";
    /**
     * The name of the directory to hold XLET and Main icons
     */
    private final static String REPOSITORY_ICONS_DIRNAME = "./icons";
    /**
     * The name of the directory to hold XLET and Main application descriptor files
     */
    private final static String REPOSITORY_DESCRIPTORS_DIRNAME = "./descriptors";
    /**
     * Handle to the content store
     */
    private JUMPStoreHandle storeHandle = null;
    /**
     * The root directory of the content store
     */
    protected String contentStoreDir = null;
    /**
     * Holds the app id -> JUMPApplication object relationship
     */
    private Hashtable installedAppIdHashtable = null;
    private int installedAppIdHashtableKey = 1;
    
    /**
     * The keys below are used within application descriptor files only.
     * These keys are not the sames keys as the ones used in the download module.
     */
    private final static String DESCRIPTOR_BUNDLENAME_KEY = "bundle";
    private final static String DESCRIPTOR_APPMODEL_KEY = "type";
    private final static String DESCRIPTOR_JARPATH_KEY = "path";
    private final static String DESCRIPTOR_TITLE_KEY = "title";
    private final static String DESCRIPTOR_ICON_KEY = "icon";
    private final static String DESCRIPTOR_SECURITYLEVEL_KEY = "icon";
    private final static String DESCRIPTOR_ID_KEY = "id";
    
    protected final static String DESCRIPTOR_INITIALCLASS_KEY = "xletName";
    
    /**
     * Print out messages
     */
    private boolean verbose = false;
    
    /**
     *
     */
    private static final boolean ALLOW_MULTIPLE_APP_INSTALLS = false;
    
    
    private String getAvailableAppIdHashKey() {
        int tempHashKey = installedAppIdHashtableKey;
        while (installedAppIdHashtable.containsKey(Integer.toString(tempHashKey))) {
            if (tempHashKey == Integer.MAX_VALUE) {
                tempHashKey = 1;
            } else {
                tempHashKey++;
            }
            
            // For the very, very unlikely case that MAX_VALUE applications
            // are running and we cycled through all integer values.
            if (tempHashKey == installedAppIdHashtableKey) {
                return null;
            }
        }
        installedAppIdHashtableKey = tempHashKey;
        return Integer.toString(tempHashKey);
    }
    
    private void addInstalledAppIdEntry(Object key, Object app) {
        // Sanity check
        if (key == null || app == null) {
            return;
        }
        
        int keyVal = Integer.parseInt((String)key);
        if (keyVal < 1) {
            return;
        } else {
            installedAppIdHashtableKey = keyVal;
        }
        installedAppIdHashtable.put(key, app);
    }
    
    private void removeInstalledAppIdEntry(Object key) {
        if (key == null) {
            return;
        }
        Object app = installedAppIdHashtable.remove(key);
    }
    
    /**
     * Returns an instance of the content store to be used with the installer.
     * @return Instance of JUMPStore
     */
    protected JUMPStore getStore() {
        return JUMPStoreFactory.getInstance().getModule(JUMPStoreFactory.TYPE_FILE);
    }
    
    /**
     * Implementation of JUMPInstaler.unload()
     */
    public void unload() {
        contentStoreDir = null;
        verbose = false;
        if (storeHandle != null) {
            closeStore(storeHandle);
        }
        installedAppIdHashtable.clear();
        installedAppIdHashtable = null;
    }
    
    /**
     * load the installer module
     * @param map the configuration data required for loading this service.
     */
    public void load(Map map) {
        // check if verbose mode is used
        String verboseStr = System.getProperty("installer.verbose");
        if (verboseStr == null && map != null) {
            verboseStr = (String) map.get("installer.verbose");
        }
        if (verboseStr != null && verboseStr.toLowerCase().equals("true")) {
            verbose = true;
        }
        
        // the repository directory should be passed in as a system property
        contentStoreDir = System.getProperty("contentstore.root");
        if (contentStoreDir == null && map != null) {
            contentStoreDir = (String)map.get("contentstore.root");
        }
        if (contentStoreDir != null) {
            // remove any ending /'s'
            if (!contentStoreDir.endsWith("/")) {
                contentStoreDir = contentStoreDir.concat("/");
            }
        } else {
            // default to the current directory
            contentStoreDir = "./";
        }
        
        // Get the store handle from the JUMPContentStoreSubClass.
        storeHandle = openStore(true);
        
        // Make sure apps/, icons/, descriptors/ exist under the root
        // of content store.  This is inherited CDC-specific behavior.
        try {
            // Create the directories within the repository if they don't exist'
            if (storeHandle.getNode(REPOSITORY_APPS_DIRNAME) == null) {
                storeHandle.createNode(REPOSITORY_APPS_DIRNAME);
            }
            if (storeHandle.getNode(REPOSITORY_ICONS_DIRNAME) == null) {
                storeHandle.createNode(REPOSITORY_ICONS_DIRNAME);
            }
            if (storeHandle.getNode(REPOSITORY_DESCRIPTORS_DIRNAME) == null) {
                storeHandle.createNode(REPOSITORY_DESCRIPTORS_DIRNAME);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        closeStore(storeHandle);
        
        // Populate the installedAppIdHashtable to keep track of the installed
        // ids of all currently installed applications of this type
        installedAppIdHashtableKey = 1;
        if (installedAppIdHashtable != null) {
            installedAppIdHashtable.clear();
        }
        installedAppIdHashtable = new Hashtable();
        JUMPContent[] content = getInstalled();
        if (content != null) {
            for(int j = 0; j < content.length; j++) {
                JUMPApplication app = ((JUMPApplication)content[j]);
                int installedId = app.getId();
                addInstalledAppIdEntry(Integer.toString(installedId), app);
            }
        }
    }
    
    /**
     * install content specified by the given descriptor and location.
     * @return the installed content
     * @param location URL of content to be installed
     * @param desc object describing the content to be installed
     */
    public JUMPContent[] install(URL location, JUMPDownloadDescriptor desc) {
        // sanity check
        if (location == null || desc == null ||
                !location.getProtocol().equals("file") ||
                desc.getType() != JUMPDownloadDescriptor.TYPE_APPLICATION) {
            return null;
        }
        
        // This is the all important "name" value.  This is the value retrieved
        // from the <name> in the Descriptor file.  This value will be used
        // as the bundle name and is also used to name the jarfile and parent
        // directory in the repository.  We need to restrict the characters used
        // within this name value, which means that all of the characters in the name
        // must be valid filename characters.  This name value is not intended
        // for any display value purposes.  For that, use <ddx:display>.
        // Here's an example.  Let's say the name value is CasinoGames.  Then
        // the resulting jarfile name will be: apps/CasinoGames/CasinoGames.jar.
        // If there is already a pathname that is the same, the jarfile name changes
        // a bit.  We currently do not overwrite the existing duplicate file.  Read
        // below for more details.
        String bundleName = desc.getName();
        if (bundleName == null) {
            return null;
        }
        
        // We need to replace spaces because apparently java doesn't like
        // jarfiles with spaces in the name. Any further string substitutions
        // should be done here.
        bundleName = bundleName.replace(' ', '_');
        
        trace(getString("Installing") + bundleName);
        
        String jarPathWithinStore = REPOSITORY_APPS_DIRNAME + '/' + bundleName + '/' + bundleName + ".jar";
        String jarPath = null;
        if (ALLOW_MULTIPLE_APP_INSTALLS) {
            // createUniquePathName ensures that we don't overwrite an existing file by
            // retrieving a unique name for saving.  If there exists a file of the same
            // name, using the example above the new name will look something like this:
            // apps/CasinoGames(2)/CasinoGames(2).jar.
            jarPath = createUniquePathName(contentStoreDir + jarPathWithinStore);
        } else {
            jarPath = contentStoreDir + jarPathWithinStore;
            if (new File(jarPath).exists()) {
                System.out.println("*** Error installing bundle: A bundle with this name is already installed.");
                return null;
            }
        }
        
        trace(getString("AttemptingToSave") + jarPath);
        
        // Because of the possibility of the filename being modified because of
        // an already exiting file, the bundle name will also need to change.
        // The new bundle name is within the filename itself, so extract the bundle
        // name.  For the example above, the new bundle name will be CasinoGames(2),
        // not CasinoGames.  Again, this is only in the rare case of duplicates.
        // This should be changed if the policy is to overwrite existing filenames.
        int dotindex = jarPath.lastIndexOf('.');
        int fileSeparatorIndex = jarPath.lastIndexOf('/');
        if (dotindex == -1 || fileSeparatorIndex == -1) {
            return null;
        }
        
        // The bundleName because the fileName minus the .jar
        bundleName = jarPath.substring(fileSeparatorIndex + 1, dotindex);
        String parentDir = jarPath.substring(0, fileSeparatorIndex);
        
        try {
            // This is the location of the file passed into the install method
            File origFile = new File(location.getFile());
            if (!origFile.exists()) {
                trace(getString("CannotAccessFile") + ": " + location.getFile());
                return null;
            }
            
            // First, create the parent directory for the saved application
            File destDir = new File(parentDir);
            destDir.mkdir();
            if (!destDir.exists()) {
                trace(destDir.toString() + " " + getString("DoesNotExist"));
                return null;
            }
            
            // Move the file from the passed-in location
            File destFile = new File(jarPath);
            if (destFile == null) {
                return null;
            }
            boolean result = origFile.renameTo(destFile.getCanonicalFile());
            if (!result) {
                trace(getString("CannotMoveFile"));
                
                // The file move didn't work.  A reason for this could be that
                // the original file and destination file are in two different
                // filesystems.  Try copying a buffer from the URL input stream.
                trace(getString("UsingURLInputStream"));
                byte[] buffer = copyBuffer(location.openStream(), desc.getSize());
                if (buffer == null) {
                    trace(getString("CannotSaveFile"));
                    return null;
                }
                
                // Write out to a file within the repository
                FileOutputStream fos = new FileOutputStream(destFile);
                if (fos == null) {
                    trace(getString("CannotSaveFile"));
                    return null;
                }
                fos.write(buffer);
                fos.close();
                origFile.delete();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            try {
                storeHandle.deleteNode(parentDir);
            } catch (IOException e) {
                e.printStackTrace();
            }
            return null;
        }
        
        // Now it is time to install the applications into the system
        Properties apps[] = desc.getApplications();
        if (apps == null) {
            return null;
        }
        
        Hashtable installedContentHashtable = new Hashtable();
        for (int i = 0; i < apps.length; i++) {
            Properties app = apps[i];
            
            // sanity check
            if (app == null) {
                continue;
            }
            
            String appTitle = app.getProperty("JUMPApplication_title");
            if (appTitle == null) {
                return null;
            }
            
            // Properties object to hold application properties to be written to .app file
            // The key values in this properties object should match the key values
            // defined for application descriptor files.
            Properties appProperties = new Properties();
            
            String appIDHashKey = getAvailableAppIdHashKey();
            if (appIDHashKey == null) {
                trace("ERROR: Could not obtain an id hash key value.");
                return null;
            } else {
                appProperties.setProperty(DESCRIPTOR_ID_KEY, appIDHashKey);
            }
            
            // Retrieve the filename of the icon
            //String iconFileName = app.getIconPath().getFile();
            String iconFileName = app.getProperty("JUMPApplication_iconPath");
            
            // extract the icon image from the jar file and place it in
            // the icons/ directory within the app repository
            String iconPath = extractIconFromJar(jarPath, iconFileName.trim(), appIDHashKey);
            
            // create an app descriptor file in the menu/ directory for
            // the new app so that the appmanager can recognize it.
            // make sure the descriptor pathname is uniqe and doesn't exist.
            String descriptorsDir = REPOSITORY_DESCRIPTORS_DIRNAME + '/';
            
            appProperties.setProperty(DESCRIPTOR_BUNDLENAME_KEY, bundleName);
            appProperties.setProperty(DESCRIPTOR_APPMODEL_KEY, app.getProperty("JUMPApplication_appModel"));
            appProperties.setProperty(getInstallerInitialClassKey(), app.getProperty(getPropertyInstallerInitialClassKey()));
            appProperties.setProperty(DESCRIPTOR_JARPATH_KEY, jarPath);
            appProperties.setProperty(DESCRIPTOR_TITLE_KEY, appTitle);
            if (iconPath != null) {
                appProperties.setProperty(DESCRIPTOR_ICON_KEY, iconPath);
            }
            String securityLevel = desc.getSecurityLevel();
            if (securityLevel != null) {
                appProperties.setProperty(DESCRIPTOR_SECURITYLEVEL_KEY, securityLevel);
            }
            
            String appDescriptorPath = descriptorsDir + appTitle + '-' + appIDHashKey + APP_DESCRIPTOR_EXTENSION;
            if (new File(appDescriptorPath).exists()) {
                System.out.println("*** Error installing bundle: A descriptor with this name is already installed.");
                return null;
            }
            // create application descriptor file
            boolean result = createAppDescriptor(appDescriptorPath, appProperties);
            // create JUMPApplication object for the app
            if (result) {
                JUMPApplication module = createJUMPApplication(appDescriptorPath);
                trace("--> createJUMPApplication returns: " + module.toString());
                if (module != null) {
                    installedContentHashtable.put(appIDHashKey, module);
                    addInstalledAppIdEntry(appIDHashKey, module);
                }
            }
        }
        
        // return installed content
        int size = installedContentHashtable.size();
        if (size > 0) {
            Vector jumpAppObjectsVector = new Vector();
            JUMPContent content[] = new JUMPContent[size];
            for (Enumeration e = installedContentHashtable.keys() ; e.hasMoreElements() ;) {
                Object key = e.nextElement();
                Object app = installedContentHashtable.get(key);
                jumpAppObjectsVector.add(app);
            }
            return (JUMPContent[])jumpAppObjectsVector.toArray(new JUMPContent[]{});
        } else {
            return null;
        }
    };
    
    /**
     * Uninstall content.
     * @param content the object to be uninstalled
     */
    public void uninstall(JUMPContent content) {
        // sanity check
        if (content == null) {
            return;
        }
        
        JUMPApplication app = (JUMPApplication)content;
        
        String bundleName = getBundleName(app);
        JUMPApplication[] apps = getAppsInBundle(bundleName);
        
        if (apps == null || bundleName == null) {
            return;
        }
        
// Currently, calling JUMPExecutive.getInstance returns null.  Therefore, we
// cannot use the JUMPUserInputManager APIs until this is fixed.  When this is
// fixed, the following code can be uncommented.
        
//        if (apps.length > 1) {
//            JUMPExecutive executive = JUMPExecutive.getInstance();
//            if (executive == null) {
//                System.out.println("ERROR: The JUMP Executive instance is null.");
//                return;
//            }
//            JUMPUserInputManager uiManager = executive.getUserInputManager();
//            String str = "This application belongs to the bundle: " + bundleName + " which contains multiple applications.  Do you wish to remove the bundle and all of its applications?";
//            boolean rc = uiManager.showDialog(str, null, "OK", "Cancel");
//            if (!rc) {
//                return;
//            }
//        }
        
        if (apps.length > 1) {
            System.out.print(app.getAppType().toString());
            System.out.print(": " + app.getTitle() + " is contained within a bundle" );
            System.out.println("that contains the following applications:" );
            System.out.print("  ");
            for (int i = 0; i < apps.length; i++) {
                JUMPApplication application = (JUMPApplication)apps[i];
                System.out.print(application.getTitle());
                if (i < (apps.length - 1)) {
                    System.out.print(", ");
                }
            }
            System.out.println("");
//            String value = System.getProperty("jump.installer.interactive");
//            if (value.toLowerCase().equals("true")) {
//                System.out.println("Deleting this bundle will remove all of the applications.");
//                while ( true ) {
//                    String message = "Do you wish to proceed? [y/n]: ";
//                    String answer = Utilities.promptUser(message);
//                    if (answer.toLowerCase().equals("y")) {
//                        break;
//                    } else if (answer.toLowerCase().equals("n")){
//                        return;
//                    } else {
//                        System.out.println("ERROR: Illegal response.");
//                    }
//                }
//                
//            } else {
                System.out.println("All applications will be removed.");
//            }
        }
        
        trace(getString("AttemptingToRemove") + bundleName);
        
        // Get the path to the app bundle's jar file, which is assumed
        // to be the first entry in the classpath.
        String jarPath = getAppClasspath(app);
        boolean result1 = removeJarFile(jarPath);
        if (!result1) {
            trace(getString("CannotRemoveApplicationJar") + ": " + jarPath);
        }
        
        // Remove the icon and app descriptor for each app in the bundle
        for (int i = 0; i < apps.length; i++) {
            
            int appId = apps[i].getId();
            removeInstalledAppIdEntry(Integer.toString(appId));
            
            // Remove the icon and app descriptor for each app
            boolean result2 = removeAppDescriptor(apps[i]);
            if (!result2) {
                trace(getString("CouldNotRemoveAppDescriptor") + apps[i].getTitle());
            }
            
            boolean result3 = removeIcon(apps[i].getIconPath());
            if (!result3) {
                trace(getString("CouldNotRemoveIcon") + apps[i].getTitle());
            }
            
            if (result1 && result2 && result3) {
                System.out.println(getString("SuccessfulUninstall") + apps[i].getTitle());
            }
        }
    };
    
    /**
     * Update content from given location.  For XLET and Main applications, the behavior is to uninstall the current bundle and install the new bundle.
     * @param content object to be updated
     * @param location URL location of content to update with
     * @param desc object describing the bundle to update with
     */
    public void update(JUMPContent content, URL location, JUMPDownloadDescriptor desc) {
        uninstall(content);
        install(location, desc);
    };
    
    /**
     * Get all installed content of type XLET
     * @return Array of JUMPApplication objects that are XLETs
     */
    public JUMPContent[] getInstalled() {
        
        Vector nodeVector = new Vector();
        
        storeHandle = openStore(true);
        
        // get the listing of all nodes starting at the root.
        JUMPNode.List list = null;
        try {
            list = (JUMPNode.List) storeHandle.getNode(REPOSITORY_DESCRIPTORS_DIRNAME);
        } catch (IOException e) {
            trace("Exception in getNode(): " + e.toString());
        }
        
        closeStore(storeHandle);
        
        if (list == null) {
            return null;
        }
        
        for (Iterator itn = list.getChildren(); itn.hasNext(); ) {
            JUMPNode node = (JUMPNode) itn.next();
            JUMPApplication app = createJUMPApplication(node.getURI());
            
            // Identify only the xlets or main apps, not both at the same time
            if (app != null && app.getAppType() == getInstallerAppModel()) {
                nodeVector.add(app);
            }
        }
        return (JUMPApplication[])nodeVector.toArray(new JUMPApplication[]{});
    };
    
    /**
     * Given the application object, return the name of the bundle the application belongs to
     * @param app application object
     * @return the names of the bundle this application belongs to
     */
    protected String getBundleName(JUMPApplication app) {
        XLETApplication xletApp = (XLETApplication)app;
        return xletApp.getBundle();
    }
    
    /**
     * Given the bundle name, return the application objects within the bundle
     * @param bundle name of content bundle
     * @return the application objects belonging to the bundle
     */
    protected JUMPApplication[] getAppsInBundle(String bundle) {
        JUMPApplication[] apps = (JUMPApplication[]) getInstalled();
        Vector appsVector = new Vector();
        for (int i = 0; i < apps.length; i++) {
            XLETApplication xletApp = (XLETApplication)apps[i];
            if (xletApp.getBundle().equals(bundle)) {
                appsVector.add(apps[i]);
            }
        }
        Object[] objs = appsVector.toArray();
        JUMPApplication[] bundleApps = new JUMPApplication[objs.length];
        for (int i = 0; i < objs.length; i++ ) {
            bundleApps[i] = (JUMPApplication)objs[i];
        }
        return bundleApps;
    }
    
    private String extractIconFromJar(String jarFile, String iconFile) {
        return (extractIconFromJar(jarFile, iconFile, null));
    }
    
    private String extractIconFromJar(String jarFile, String iconFile, String id) {
        
        String iconFileName = null;
        String iconFilePath = null;
        
        JarFile jar = null;
        
        try {
            jar = new JarFile(jarFile);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        
        trace("extractIconFromJar(): jarfile: " + jarFile + "  icon: " + iconFile);
        
        ZipEntry entry = jar.getEntry(iconFile);
        if (entry == null) {
            trace(getString("CouldNotExtract") + iconFile);
            return null;
        }
        
        int index = iconFile.lastIndexOf('/');
        if (index != -1) {
            iconFileName = iconFile.substring(index + 1,
                    iconFile.length());
        } else {
            iconFileName = iconFile;
        }
        
        if (id != null) {
            // Get extention of file
            int dotindex = iconFileName.lastIndexOf('.');
            if (dotindex == -1) {
                return null;
            }
            
            // The path up until the extention.
            String pathToExtention = iconFileName.substring(0, dotindex);
            
            // The extention
            String extention = iconFileName.substring(dotindex);
            
            iconFileName = pathToExtention + '-' + id + extention;
        }
        
        String iconsDir = contentStoreDir + REPOSITORY_ICONS_DIRNAME + '/';
        iconFilePath = iconsDir + iconFileName;
        if (new File(iconFilePath).exists()) {
            System.out.println("*** Warning installing bundle: An icon with this name is already installed.");
            return iconFilePath;
        } else {
            trace("Saving icon file: " + iconFilePath);
        }
        
        try {
            
            InputStream zis = jar.getInputStream(entry);
            
            int size = (int) entry.getSize();
            // -1 means unknown size.
            if (size == -1) {
                trace(getString("IconFileSizeError"));
                return null;
            }
            
            byte[] buffer = copyBuffer(zis, size);
            
            File f = new File(iconFilePath);
            FileOutputStream fos = new FileOutputStream(f);
            fos.write(buffer);
            fos.close();
            
            return iconFilePath;
            
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    
    /**
     * This is necessary to avoid issues when downloading and installing the
     * same app on the device, or at least an app with the same exact name.
     * Simply concat a number, starting with 2, to the end of the path until
     * a path is found that doesn't already exist.
     */
    private String createUniquePathName(String original) {
        
        int NUM = 2;
        int LIMIT  = 1000;
        
        // Get extention of file
        int dotindex = original.lastIndexOf('.');
        if (dotindex == -1) {
            return null;
        }
        
        // The path up until the extention.
        String pathToExtention = original.substring(0, dotindex);
        
        // The extention
        String extention = original.substring(dotindex);
        
        // Get the file name minus the extention
        int fileSeparatorIndex = original.lastIndexOf('/');
        String fileName = null;
        if (fileSeparatorIndex != -1) {
            fileName = original.substring(fileSeparatorIndex + 1, dotindex);
        } else {
            return null;
        }
        
        // First, check if the original name is unique
        String testPath = original;
        File testPathFile = new File(testPath);
        while (testPathFile.exists() && NUM < LIMIT) {
            String EXTRA = '(' + Integer.toString(NUM) + ')';
            testPath = pathToExtention + EXTRA + extention;
            testPathFile = new File(testPath);
            
            // Need to change the parent directory of the file if this
            // is a jarfile
            if (testPath.endsWith(".jar")){
                String parentPath = testPathFile.getParent();
                testPath = parentPath + EXTRA + '/' + fileName + EXTRA + extention;
                testPathFile = new File(testPath);
            }
            NUM++;
        }
        
        if (NUM == LIMIT) {
            return null;
        }
        
        return testPath;
    }
    
    /**
     * Create an application descriptor for the given application
     * values.  The application descriptor gets saved into the
     * application repository's menu directory.
     * @param descriptorPath the path within the content store to store the application descriptor
     * @param props object containing the properties of the application descriptor
     * @return boolean value indicating success or failure
     */
    private boolean createAppDescriptor(String descriptorPath, Properties props) {
        
        // Convert the paths to the relative path from the content store root,
        // we don't want to store the absolute path to the persistant store
        removeContentStorePath(props, DESCRIPTOR_ICON_KEY);
        removeContentStorePath(props, DESCRIPTOR_JARPATH_KEY);
        
        JUMPData propData = new JUMPData(props);
        
        try {
            storeHandle.createDataNode(descriptorPath, propData);
        } catch (RuntimeException re) {
            re.printStackTrace();
            return false;
        } catch (IOException ioe) {
            ioe.printStackTrace();
            return false;
        }
        
        return true;
    }
    
    /**
     * Retrieve an instance of JUMPApplication for the given application or menu.
     */
    private JUMPApplication createJUMPApplication(String descriptorPath) {
               
        storeHandle = openStore(true);
        JUMPNode appDescriptorNode = null;
        try {
            appDescriptorNode = storeHandle.getNode(descriptorPath);
        } catch (IOException e) {
            e.printStackTrace();
        }
        closeStore(storeHandle);
        
        if (appDescriptorNode == null) {
            trace(getString("AppDescriptorNotFound") + descriptorPath);
            return null;
        }
        
        JUMPData appDescriptorData = null;
        if (appDescriptorNode.containsData()) {
            appDescriptorData = ((JUMPNode.Data)appDescriptorNode).getData();
        } else {
            return null;
        }
        
        Properties appDescriptorProps = (Properties)appDescriptorData.getValue();
        JUMPApplication module = null;
        
        // First, make sure the app type is correct
        String appType = (String)appDescriptorProps.getProperty(DESCRIPTOR_APPMODEL_KEY);
        if (appType == null || !appType.equals(getInstallerAppModel().getName())) {
            return null;
        }
        
        // Convert back the path to the absolute path
        addContentStorePath(appDescriptorProps, DESCRIPTOR_ICON_KEY);
        addContentStorePath(appDescriptorProps, DESCRIPTOR_JARPATH_KEY);        
        
        // Now, obtain the values for specific keys
        String classPath = (String)appDescriptorProps.getProperty(DESCRIPTOR_JARPATH_KEY);
        if (classPath == null) {
            return null;
        }
        
        URL classPathURL = null;
        try {
            classPathURL = new URL("file", null, classPath);
            if (classPathURL == null) {
                return null;
            }
        } catch (MalformedURLException ex) {
            ex.printStackTrace();
            return null;
        }
        
        String bundleName = (String)appDescriptorProps.getProperty(DESCRIPTOR_BUNDLENAME_KEY);
        if (bundleName == null) {
            return null;
        }
        
        String title = (String)appDescriptorProps.getProperty(DESCRIPTOR_TITLE_KEY);
        if (title == null) {
            return null;
        }
        
        String iconPath = (String)appDescriptorProps.getProperty(DESCRIPTOR_ICON_KEY);
        URL iconPathURL = null;
        if (iconPath != null) {
            try {
                iconPathURL = new URL("file", null, iconPath);
                if (iconPathURL == null) {
                    return null;
                }
            } catch (MalformedURLException ex) {
                ex.printStackTrace();
                return null;
            }
        } else {
            trace("Application: " + title + " doesn't have an icon.");
        }

        String clazz = (String)appDescriptorProps.getProperty(getInstallerInitialClassKey());
        
        String id = (String)appDescriptorProps.getProperty("id");
        
        module = createJUMPApplicationObject(bundleName, clazz, classPathURL, title, iconPathURL, Integer.parseInt(id));
 
        return module;
    }
    
    /**
     * Removes the content store dir path from the property represented by the key.
     * Used when creating .app file from the JUMPApplication properties list,
     * so that .app file in the content sore will not have a full path to store,
     * allowing the store to be moved from one location to another.
     */
    private void removeContentStorePath(Properties props, String key) {
        String value = (String) props.remove(key);
        if (value == null) return;
        props.put(key, value.substring(contentStoreDir.length()));
    }
    
    /**
     * Prepends the content store dir path to the property represented by the key.
     * Used when creating JUMPApplication properties from the .app file
     * in the content store, so that the JUMPApplication created have
     * proper absolute path corresponding to the current run's contentstore
     * root location.
     */
    private void addContentStorePath(Properties props, String key) {
        String value = (String) props.remove(key);
        if (value == null) return;
        props.put(key, contentStoreDir + value);
    }
    
    /**
     * Create an instance of JUMPApplication
     * @param bundle Name of application bundle that this application belongs to
     * @param clazz Initial class of application
     * @param classPathURL URL of the classpath of this application
     * @param title The user visible title of this application
     * @param iconPathURL URL to the path of the icon for this application
     * @return application object
     */
    protected JUMPApplication createJUMPApplicationObject(String bundle,
            String clazz, URL classPathURL, String title, URL iconPathURL, int id) {
        return new XLETApplication(contentStoreDir, bundle, clazz, classPathURL, title, iconPathURL, id);
    }
    
    /**
     * Return the application type that this installer module can install
     * @return the application type
     */
    protected JUMPAppModel getInstallerAppModel() {
        return JUMPAppModel.XLET;
    }
    
    /**
     * Get the key value used to specify an application's initial class
     * @return initial class key value
     */
    protected String getInstallerInitialClassKey() {
        return "xletName";
    }
    
    /**
     * Get the key value used to specify an application's initial class from Properties object
     * @return initial class key value
     */
    protected String getPropertyInstallerInitialClassKey() {
        return XLETApplication.INITIAL_CLASS_KEY;
    }
    
    /**
     * Remove the jar file from the content store.
     * The current implementation uses java.util.File methods.
     * @param jarPath path to the jar file within the content store
     * @return boolean value indicating success or failure
     */
    private boolean removeJarFile(String jarPath) {
        
        // Check to see that the jar file exists, and if so, start
        // removing things.
        File jarFile = new File(jarPath);
        trace("removeJarFile(): " + jarFile);
        if (jarFile != null && jarFile.exists()) {
            trace("jarFile: " + jarFile + " exists!!! ***");
            boolean jarFileDelete = false;
            boolean jarFileParentDirDelete = false;
            File jarFileParent = null;
            try {
                jarFileParent = jarFile.getParentFile();
                jarFileDelete = jarFile.delete();
                if (jarFileParent != null) {
                    jarFileParentDirDelete = jarFileParent.delete();
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            
            if (!jarFileDelete) {
                trace(getString("CannotRemoveApplicationJar"));
            }
            // Print out a message if we cannot remove the parent directory,
            // but continue on...
            if (!jarFileParentDirDelete) {
                trace(getString("CouldNotRemoveApplicationDir"));
            }
            return true;
        } else {
            trace(getString("ApplicationJarDoesNotExist"));
        }
        
        return false;
    }
    
    /**
     * Remove application descriptor from content store
     * @param applicationName the application for which the application descriptor should be removed
     * @return boolean value indicating success or failure
     */
    private boolean removeAppDescriptor(JUMPApplication app) {
        String appTitle = app.getTitle();
        String id = Integer.toString(app.getId());
        String uri = REPOSITORY_DESCRIPTORS_DIRNAME + '/' + appTitle + '-' + id + APP_DESCRIPTOR_EXTENSION;
        storeHandle = openStore(true);
        if (storeHandle == null) {
            return false;
        }
        try {
            storeHandle.deleteNode(uri);
        } catch (IOException e) {
            return false;
        }
        closeStore(storeHandle);
        return true;
    }
    
    /**
     * Remove an icon file from within the content store
     * @param iconURL URL of icon file within the content store
     * @return boolean value indicating success or failure
     */
    private boolean removeIcon(URL iconURL) {
        String iconFileName = iconURL.getFile();
        trace("Trying to remove icon file: " + iconFileName);
        File iconFile = new File(iconFileName);
        if (iconFile != null && iconFile.exists()) {
            try {
                iconFile.delete();
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        } else {
            return false;
        }
        return true;
    }
    
    /**
     * read from an input stream into a byte buffer[]
     * @param is the input stream where the data is located
     * @param size the number of bytes to read
     * @return byte buffer of read data
     */
    private byte[] copyBuffer(InputStream is, int size) {
        if (is == null) {
            return null;
        }
        
        byte[] buffer = new byte[size];
        try {
            
            // Read data into buffer
            int rb = 0;
            int chunk = 0;
            while ( (size - rb) > 0) {
                chunk = is.read(buffer, rb, size - rb);
                if (chunk == -1) {
                    break;
                }
                rb += chunk;
            }
        } catch (IOException ex) {
            ex.printStackTrace();
            return null;
        }
        
        return buffer;
    }
    
    /**
     * Obtain the classpath value of the application object
     * @param app application object
     * @return classpath value of the application object
     */
    protected String getAppClasspath(JUMPApplication app) {
        if (app == null) {
            return null;
        }
        XLETApplication xletApp = (XLETApplication)app;
        return xletApp.getClasspath().getFile();
    }
    
    /**
     * Retrieve a String value from the module's resource bundle
     * @param key key value into resource bundle
     * @return value pertaining to the key field in the resource bundle
     */
    private String getString(String key) {
        return InstallerFactoryImpl.getString(key);
    }
    
    /**
     * Simple print command that prints messages if verbose is on
     * @param s string to print
     */
    private void trace(String s) {
        if (verbose) {
            System.out.println(s);
        }
        return;
    }
}
