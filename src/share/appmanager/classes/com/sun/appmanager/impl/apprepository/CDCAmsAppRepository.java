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

package com.sun.appmanager.impl.apprepository;

import java.util.*;
import java.io.File;
import java.io.FilenameFilter;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.*;
import java.util.jar.*;
import java.util.zip.*;
import com.sun.appmanager.apprepository.*;
import com.sun.appmanager.ota.*;
import com.sun.appmanager.store.PersistentStore;
import com.sun.appmanager.AppManager;

public class CDCAmsAppRepository
    implements AppRepository {

    private final String pathSeparator = System.getProperty("path.separator");

    private PersistentStore ps;
    private ResourceBundle rb = null;

    public CDCAmsAppRepository() {
        this.ps = AppManager.getPersistentStore();
        this.rb = AppManager.getResourceBundle();
    }

    /**
     * Return an array of AppModules representing available applications
     * on the device for the given user.
     * @return AppModule[] array of application modules for each
     *   available application and menu
     */
    public AppModule[] getAppList() {

        String[] files = new File(ps.absolutePathOf("menu")).list(new
            ApplicationsFilenameFilter());

        AppModule[] apps = new AppModule[files.length];

        for (int i = 0; i < files.length; i++) {
            try {
                BufferedReader in = new BufferedReader(new FileReader(
                    ps.absolutePathOf("menu/" + files[i])));
                String line = null;
                Hashtable hash = new Hashtable();
                while ( (line = in.readLine()) != null) {
                    if (line.startsWith("#")) { // comment
                        continue;
                    }
                    int index = line.indexOf('=');
                    if (index == -1) {
                        continue;
                    }
                    String key = line.substring(0, index);
                    String value = line.substring(index + 1);
                    hash.put(key, value);
                }
                in.close();
                apps[i] = new AppModule(hash);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }

        return apps;
    }

    class ApplicationsFilenameFilter
        implements FilenameFilter {
        public boolean accept(File dir, String name) {
            if (name.endsWith(".app") || (name.endsWith(".menu"))) {
                return true;
            }
            else {
                return false;
            }
        }
    }

    /**
     * Create an application descriptor for the given application
     * values.  The application descriptor gets saved into the
     * application repository's menu directory.
     * @param bundleName the bundleName of the app descriptor file
     * @param applicationName title of the application
     * @param applicationType type of application, i.e. "XLET", "APP"
     * @param mainClass the main class of the application
     * @param applicationPath the directory containing the application, relative
     * to the application repository's app directory.
     * @param icon the name of the icon file
     * @param menu the menu in which the application's icon should be installed
     * into.  The main menu is represented by the value "main".
     */
    public boolean createAppDescriptor(String bundleName,
                                       String applicationName,
                                       String applicationType,
                                       String mainClass,
                                       String applicationPath,
                                       String icon,
                                       String menu,
                                       String securityLevel) {

        BufferedWriter out = null;
        try {
            out = new BufferedWriter(new FileWriter(
                ps.absolutePathOf("menu/" + applicationName + ".app")));

            String str;
            str = "bundle=" + bundleName;
            out.write(str, 0, str.length());
            out.newLine();

            str = "type=" + applicationType;
            out.write(str, 0, str.length());
            out.newLine();

            if (applicationType.equals("XLET")) {
                str = "xletName=" + mainClass;
            }
            else {
                str = "mainClass=" + mainClass;
            }

            out.write(str, 0, str.length());
            out.newLine();

            str = "path=" + applicationPath;
            out.write(str, 0, str.length());
            out.newLine();

            str = "title=" + applicationName;
            out.write(str, 0, str.length());
            out.newLine();

            str = "icon=" + icon;
            out.write(str, 0, str.length());
            out.newLine();

            str = "menu=" + menu;
            out.write(str, 0, str.length());
            out.newLine();

            if (securityLevel != null) {
                str = "securityLevel=" + securityLevel;
                out.write(str, 0, str.length());
                out.newLine();
            }

            out.flush();
            out.close();

            return true;
        }
        catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * Retrieve an instance of AppModule for the given application or menu.
     * Given the application or menu base filename, it's application descriptor
     * is read and the values are then used to create the AppModule.
     *
     * @param basename the basename of the app descriptor file
     * @param isMenu true if the module is for a menu, false for an application
     * @return AppModule instance of AppModule from the given information, null
     *   if an instance cannot be created from the given information.
     */
    private AppModule createAppModule(String basename, boolean isMenu) {

        File file = null;
        if (isMenu) {
            file = new File(ps.absolutePathOf("menu/" + basename + ".menu"));

        }
        else {
            file = new File(ps.absolutePathOf("menu/" + basename + ".app"));
        }
        if (!file.exists()) {
            System.err.println(rb.getString("AppDescriptorNotFound") +
                               basename + ".");
            return null;
        }

        AppModule module = null;
        try {
            BufferedReader in = new BufferedReader(new FileReader(file));
            String line = null;
            Hashtable hash = new Hashtable();
            while ( (line = in.readLine()) != null) {
                if (line.startsWith("#")) { // comment
                    continue;
                }
                int index = line.indexOf('=');
                String key = line.substring(0, index);
                String value = line.substring(index + 1);
                hash.put(key, value);
            }
            in.close();
            module = new AppModule(hash);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        return module;
    }

    private boolean removeAppDescriptor(String applicationName,
                                        String applicationType) {

        File file = null;
        String root = "menu/" + applicationName;
        if (applicationType.equals("SUBLIST")) {
            file = new File(ps.absolutePathOf(root + ".menu"));
        }
        else {
            file = new File(ps.absolutePathOf(root + ".app"));
        }

        boolean result = false;
        try {
            if (file.exists()) {
                result = file.delete();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    private boolean removeIcon(String iconFile) {
        File file = new File(ps.absolutePathOf("icons/" + iconFile));

        boolean result = false;
        try {
            if (file.exists()) {
                result = file.delete();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    /**
     *
     * @param descriptor Descriptor
     * @return Destination
     */
    public Destination getDestination(Descriptor descriptor) {

        CDCAmsDestination cd = new CDCAmsDestination(descriptor);
        return cd;
    }

    /**
     *
     * @param app Application
     * @param jarFile String
     * @param menu String
     * @return boolean
     */
    public AppModule[] installApplication(Destination destination, String menu) {
        CDCAmsDestination cd = (CDCAmsDestination) destination;

        Descriptor descriptor = cd.getDescriptor();
        if (descriptor == null) {
            return null;
        }

// This is the all important "name" value.  This is the value retrieved
// from the <name> in the Descriptor file.  This value will be used
// as the bundle name and is also used to name the jarfile and parent
// directory in the repository.  We need to restrict the characters used
// within this name value, which means that all of the characters in the name
// must be valid filename characters.  This name value is not intended
// for any display value purposes.  For that, use <ddx:display>.
        String name = descriptor.getName();
        if (name == null) {
            return null;
        }

        Vector appModuleVector = new Vector();

// We need to replace spaces because apparently java doesn't like
// jarfiles with spaces in the name.
        name = name.replace(' ', '_');

        System.err.println(rb.getString("Installing") + name);
        String path = "apps/" + name + "/" + name + ".jar";
        String jarFile = ps.absolutePathOf(path);
        File f = new File(jarFile);
// We need to make sure the proposed jarFile name doesn't already exist.
// and keep trying a new name if the name already exists.
        while (f.exists()) {
            name = name +
                String.valueOf(System.currentTimeMillis());
            path = "apps/" + name + "/" + name + ".jar";
            jarFile = ps.absolutePathOf(path);
            f = new File(jarFile);
        }

        byte[] buffer = cd.getBuffer();

        File parentDir = null;

        try {

            // need to make sure the parent directory exists
            String parent = f.getParent();
            parentDir = new File(parent);
            if (!parentDir.exists()) {
                parentDir.mkdirs();
            }

            FileOutputStream fos = new FileOutputStream(f);
            fos.write(buffer);
            fos.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
            parentDir.delete();
            return null;
        }

        for (Enumeration e = descriptor.getApplications().elements();
             e.hasMoreElements(); ) {

            Application app = (Application) e.nextElement();

            // Retrieve the filename of the icon
            String iconFileName = app.getIconPath();

            // extract the icon image from the jar file and place it in
            // the icons/ directory within the app repository
            File iconFile = extractIconFromJar(jarFile, iconFileName);

            if (iconFile == null) {
                f.delete();
                parentDir.delete();
                return null;
            }

            // create an app descriptor file in the menu/ directory for
            // the new app so that the appmanager can recognize it.
            if (app != null) {
                String appType = null;
                if (app.getAppType() == Application.XLET_APP) {
                    appType = "XLET";
                }
                else if (app.getAppType() == Application.MAIN_APP) {
                    appType = "APP";
                }
                else {
                    return null;
                }

                if (app.getClasspath() != null) {
                    String cdcamsHome = System.getProperty("cdcams.home");
                    if (cdcamsHome == null) {
                        throw new RuntimeException(rb.getString(
                            "HomeNotSpecified"));
                    }
                    path = path.concat(System.getProperty("path.separator") +
                                       app.getClasspath());
                }
                boolean result = false;

                result = createAppDescriptor(name,
                                             app.getName(),
                                             appType,
                                             app.getMainClass(),
                                             path,
                                             iconFile.getName(),
                                             menu,
                                             descriptor.getSecurityLevel());
                if (result) {
                    AppModule module = createAppModule(app.getName(), false);
                    if (module != null) {
                        appModuleVector.add(module);
                    }
                }
            }
        }

        if (appModuleVector.size() > 0) {
            int size = appModuleVector.size();
            AppModule module[] = new AppModule[size];
            Object moduleObjects[] = appModuleVector.toArray();
            for (int i = 0; i < size; i++) {
                module[i] = (AppModule) moduleObjects[i];
            }
            return module;
        }
        else {
            return null;
        }
    }

    /**
     *
     * @param module AppModule
     * @return boolean
     */
    public boolean removeApplication(AppModule module[]) {
        if (module == null || module[0] == null) {
            return false;
        }

        String bundle = module[0].getBundle();

        // Check that these modules live in the same bundle.
        if (module.length > 0) {
            for (int i = 1; i < module.length; i++) {
              String bundle2 = module[i].getBundle();
              if (!bundle2.equals(bundle)) {
                  System.err.println(rb.getString("AppsNotInSameBundle"));
                  return false;
              }
            }
        }

        System.out.println("Attempting to remove app bundle: " + bundle);

        // Get the path to the app bundle's jar file, which is assumed
        // to be the first entry in the classpath.  We can look at index 0
        // since all
        String modulePath = module[0].getPath();
        String jarPath = null;
        int index = modulePath.indexOf(pathSeparator);
        if (index == -1) {
            jarPath = modulePath;
        }
        else {
            jarPath = modulePath.substring(0, index);
        }

        // Check to see that the jar file exists, and if so, start
        // removing things.
        File jarFile = new File(jarPath);
        if (jarFile.exists()) {
            boolean jarFileDelete = false;
            boolean jarFileParentDirDelete = false;
            File jarFileParent = null;
            try {
                jarFileParent = jarFile.getParentFile();
                jarFileDelete = jarFile.delete();
                jarFileParentDirDelete = jarFileParent.delete();
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            // Print out a message if we cannot remove the parent directory,
            // but continue on...
            if (!jarFileParentDirDelete) {
                System.err.println(rb.getString("CouldNotRemoveParentDir"));
            }

            if (jarFileDelete) {
                boolean returnVal = true;

                // Remove the icon and app descriptor for each app
                for (int i = 0; i < module.length; i++) {
                    boolean result1 = removeAppDescriptor(module[i].getTitle(),
                        module[i].getType());
                    boolean result2 = removeIcon(module[i].getIconPath());
                    if (result1 && result2) {
                        System.err.println(rb.getString("ApplicationRemoved") +
                                           module[i].getTitle());
                    }
                    else {
                        System.err.println(
                            rb.getString("ProblemRemovingApplication") +
                            module[i].getTitle());
                        returnVal = false;
                    }
                }
                return returnVal;
            }
        }
        else {
            System.out.println(rb.getString("CantRemoveApplication") + bundle);
        }

        return false;
    }

    /**
     *
     * @param jarFile String
     * @param iconFile String
     * @return File
     */
    private File extractIconFromJar(String jarFile, String iconFile) {

        String iconFileName = null;
        String iconFilePath = null;

        JarFile jar = null;
        try {
            jar = new JarFile(jarFile);
        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }

        ZipEntry entry = jar.getEntry(iconFile);
        int index = iconFile.lastIndexOf('/');
        iconFileName = iconFile.substring(index + 1,
                                          iconFile.length());
        iconFilePath = ps.absolutePathOf("icons/" + iconFileName);

        // extract resources and put them into the hashtable.
        try {

            InputStream zis = jar.getInputStream(entry);

            int size = (int) entry.getSize();
            // -1 means unknown size.
            if (size == -1) {
                System.out.println(
                    rb.getString("IconFileSizeError"));
                return null;
            }

            byte[] b = new byte[size];
            int rb = 0;
            int chunk = 0;
            while ( (size - rb) > 0) {
                chunk = zis.read(b, rb, size - rb);
                if (chunk == -1) {
                    break;
                }
                rb += chunk;

            }
            File f = new File(iconFilePath);
            FileOutputStream fos = new FileOutputStream(f);
            fos.write(b);
            fos.close();
            return f;

        }
        catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}

class CDCAmsDestination
    implements Destination {

        // Begin Destination implementation
        byte[] buffer;
        int bufferIndex = 0;
        Descriptor descriptor = null;
        Application app = null;
        String jarFile = null;

        public CDCAmsDestination(Descriptor descriptor) {
            this.descriptor = descriptor;
            app = (Application) descriptor.getApplications().get(0);
            buffer = new byte[descriptor.getSize()];
        }

        public byte[] getBuffer() {
            return buffer;
        }

        public Descriptor getDescriptor() {
            return descriptor;
        }

        public String getJarFile() {
            return jarFile;
        }

        public void acceptMimeType(String mimeType) throws OTAException {
            trace("saying we handle mimetype " + mimeType);
            return;
        }

        public void start(String sourceURL,
                          String mimeType) throws OTAException, IOException {
            trace("download is about to start from " + sourceURL +
                  ", of type " + mimeType);
            return;
        }

        public int receive(InputStream in, int desiredLength) throws
            OTAException, IOException {

            trace("receiving data ");

            int numRead = in.read(buffer, bufferIndex, desiredLength);
            if (numRead > 0) {
                bufferIndex += numRead;
            }
            return numRead;

        }

        public void finish() throws OTAException, IOException {

            trace("download succeeded. save the results");

            return;
        }

        public void abort() {
            trace("download aborted");
            return;
        }

        public int getMaxChunkSize() {
            trace("saying we'll take any chunksize");
            return 0;
        }

        static void trace(String s) {
            if (false) {
                System.out.println(s);
            }
            return;
    }
        // End Destination implementation
}
