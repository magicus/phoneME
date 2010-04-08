/*
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

package com.sun.jumpimpl.module.installer;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.module.JUMPModuleFactory;
import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.module.download.JUMPDownloadDestination;
import com.sun.jump.module.download.JUMPDownloadException;
import com.sun.jump.module.download.JUMPDownloadModule;
import com.sun.jump.module.download.JUMPDownloadModuleFactory;
import com.sun.jump.module.download.JUMPDownloader;
import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jumpimpl.module.download.DownloadDestinationImpl;
import com.sun.jumpimpl.module.download.OTADiscovery;
import java.io.File;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Properties;
import java.util.Vector;

/**
 * This class is an installation tool for downloading,
 * installing, uninstalling, and listing content in the
 * content store application repository.
 *
 * This class should be routinely modified with more
 * features as development continues.
 *
 * The current supported commands are:
 *    list, info, install, install_all, uninstall, uninstall_all
 *
 * The commands install and uninstall will provide the user with an interactive
 * way to choose files to be installed or uninstalled.  The command install_all
 * and uninstall_all will install or uninstall all content without interactive
 * with the user.
 *
 * Usage:
 *   <cvm>  <system properties> -cp <classpath> com.sun.jumpimpl.module.installer.JUMPInstallerTool -ProvisioningServerURL <url of provisioning server> <options> -command <command>
 *     <system properties> is optional, but it should be known that contentstore.root
 *         can be overridden here if desired.
 *         For example, -Dcontentstore.root=<repository dir> can be specified
 *     <command> can currently be list, info, install, install_all, uninstall, and uninstall_all
 *     <options>
 *        -verbose:  print debugging messages
 *
 * Ex:
 *   cvm -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -command list
 *   cvm -Dcontentstore.root=data2 -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -command install
 *   cvm -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -command uninstall
 *   cvm -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -verbose -command install_all
 *   cvm -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -command uninstall_all
 *
 */
public class JUMPInstallerTool {
    
    /**
     * xlet installer module object
     */
    protected JUMPInstallerModule xletInstaller = null;
    /**
     * midlet installer module object
     */
    protected JUMPInstallerModule midletInstaller = null;
    /**
     * main installer module object
     */
    protected JUMPInstallerModule mainInstaller = null;
    /**
     * holds download module object
     */
    protected JUMPDownloadModule downloadModule = null;
    /**
     * URL used for Provisioning Server location
     */
    protected String ProvisioningServer = null;
    /**
     * The current command to be run
     */
    protected String Command = null;
    /**
     * Sub-values for the current command to be run
     */
    protected String Value = null;
    /**
     * Whether or not to print debug messages
     */
    protected boolean Verbose = false;
    /**
     * URL containing the content to be installed.
     */
    protected String ContentURL = null;
    /**
     * URI of the descriptor file of the content to be installed
     */
    protected String DescriptorURI = null;
    /**
     * The protocol of the content.  The value should be either:
     *   ota/midp or ota/oma
     */
    protected String Protocol = null;
    /**
     * The application type of an installed content
     */
    protected String Type = null;
    /**
     * The id of an installed content.
     */
    protected String Id = null;
    /**
     * The current root of content store where applications are located
     */
    private String repository = null;
    /**
     * The property name holding the root of content store
     */
    private static final String repositoryProperty = "contentstore.root";
    
    
    private Hashtable parseArgs(String args[]) {
        if (args == null) {
            return null;
        }
        Hashtable argTable = new Hashtable();
        String arg = null;
        
        // The options -ContentURL, -DescriptorURI, and -Protocol are not
        // yet functional yet as our download implementation's createDescriptor
        // methods assume an http connection.
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-ProvisioningServerURL")) {
                arg = args[++i];
                argTable.put("ProvisioningServerURL", arg);
            } else if (args[i].equals("-command")) {
                arg = args[++i];
                argTable.put("Command", arg);
            } else if (args[i].equals("-verbose")) {
                System.setProperty("installer.verbose", "true");
                argTable.put("Verbose", "true");
            } else if (args[i].equals("-ContentURL")) {
                arg = args[++i];
                argTable.put("ContentURL", arg);
            } else if (args[i].equals("-DescriptorURI")) {
                arg = args[++i];
                argTable.put("DescriptorURI", arg);
            } else if (args[i].equals("-Protocol")) {
                arg = args[++i];
                argTable.put("Protocol", arg);
            } else if (args[i].equals("-id")) {
                arg = args[++i];
                argTable.put("Id", arg);
            } else if (args[i].equals("-type")) {
                arg = args[++i];
                argTable.put("Type", arg);
            }
        }
        return argTable;
    }
    
    /**
     * Creates a new instance of JUMPInstallerTool
     * @param args arguments for the tool
     */
    public JUMPInstallerTool(String[] args) {
        Hashtable hash = parseArgs(args);
        this.Command = (String)hash.get("Command");
        String verbose = (String)hash.get("Verbose");
        if (verbose != null && verbose.equals("true")) {
            this.Verbose = true;
        }
        
        this.ProvisioningServer = (String)hash.get("ProvisioningServerURL");
        
        // The three lines of code below, along with usage of the fields,
        // is for a future case where the tool will allow local installations.
        this.ContentURL = (String)hash.get("ContentURL");
        this.DescriptorURI = (String)hash.get("DescriptorURI");
        this.Protocol = (String)hash.get("Protocol");
        this.Type = (String)hash.get("Type");
        this.Id = (String)hash.get("Id");
        
        trace("");
        trace("=============================================");
        trace("JUMPInstallerTool Settings");
        trace("--------------------------");
        trace("");
        trace("   Command: " + Command);
        trace("App Server: " + ProvisioningServer);
        trace("        ID: " + Id);
        trace("      Type: " + Type);
        trace("=============================================");
        trace("");
        
        if (!setup()) {
            System.exit(-1);
        };
        
        if (Command != null) {
            doCommand();
        }
    }
    
    private void usage() {
        System.out.println("Usage:");
        System.out.println("  <cvm> <system properties> -cp <classpath> com.sun.jumpimpl.module.installer.JUMPInstallerTool <options>  -command <command>");
        System.out.println("Available commands that can be used are:  list, install, install_all, uninstall, and uninstall_all.");
        System.out.println("Available options: -verbose");
        System.out.println("");
        System.out.println("Ex:");
        System.out.println("  cvm -cp $JUMP_JARS com.sun.jumpimpl.module.installer.JUMPInstallerTool -verbose -command list");
        System.out.println("");
    }
    
    private void trace(String str) {
        if (this.Verbose) {
            System.out.println(str);
        }
    }
    
    private boolean setup() {
        
        repository = System.getProperty(repositoryProperty);
        if (repository != null) {
            // test setup, make a repository root
            File file = new File(repository);
            if (!file.exists()) {
                System.out.println("ERROR: " + repository + " directory not found.");
                return false;
            }
        }
        
        if (JUMPExecutive.getInstance() == null) {
            JUMPModuleFactory factory = null;
            factory = new com.sun.jumpimpl.module.installer.InstallerFactoryImpl();
            factory.load(com.sun.jumpimpl.process.JUMPModulesConfig.getProperties());
            factory = new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();
            factory.load(com.sun.jumpimpl.process.JUMPModulesConfig.getProperties());
            factory = new com.sun.jumpimpl.module.download.DownloadModuleFactoryImpl();
            factory.load(com.sun.jumpimpl.process.JUMPModulesConfig.getProperties());
        }
        
        return true;
    }
    
    private JUMPInstallerModule createInstaller(JUMPAppModel type) {
        
        JUMPInstallerModule module = null;
        
        if (type == JUMPAppModel.MAIN) {
            module = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.MAIN);
        } else if (type == JUMPAppModel.XLET) {
            module = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.XLET);
        } else if (type == JUMPAppModel.MIDLET) {
            module = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.MIDLET);
        }
        
        if (module == null)  {
            return null;
        }
        
        return module;
    }
    
    private void doCommand() {
        if (Command.equals("install")) {
            if (DescriptorURI != null) {
                doInstall(DescriptorURI);
            } else {
                doInstall(ProvisioningServer, true);
            }
        } else if (Command.equals("install_all")) {
            doInstall(ProvisioningServer, false);
        } else if (Command.equals("list")) {
            doList();
        } else if (Command.equals("uninstall")) {
            if (Id != null && Type != null) {
                doUninstall(JUMPAppModel.fromName(Type), Integer.parseInt(Id));
            } else {
                doUninstall(true);
            }
        } else if (Command.equals("uninstall_all")) {
            doUninstall(false);
        } else if (Command.equals("info")) {
            doInfo();
        }
    }
    
    private void error() {
        System.out.println("ERROR: Could not install.");
        if (!this.Verbose) {
            System.out.println("==> Please run with -verbose for more information.");
        }
    }
    
    /**
     * Print out information pertaining to each application in a list.
     */
    public void doInfo() {
        System.out.println("");
        System.out.println("---------------------------------");
        System.out.println("Applications Within Content Store");
        System.out.println("---------------------------------");
        System.out.println("");
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        
        int numApps = 0;
        
        for (int i = 0; i < installers.length; i++) {
            
            JUMPContent[] content = installers[i].getInstalled();
            if (content != null) {
                for(int j = 0; j < content.length; j++) {
                    numApps++;
                    JUMPApplication app = (JUMPApplication)content[j];
                    System.out.println("App #" + numApps + ": " + app.getTitle());
                    JUMPAppModel model = app.getAppType();
                    if (model == JUMPAppModel.XLET) {
                        XLETApplication xlet = (XLETApplication)app;
                        System.out.println("    Bundle: " + xlet.getBundle());
                        System.out.println("       Jar: " + xlet.getClasspath());
                        System.out.println("Install ID: " + xlet.getId());
                        URL iconPath = xlet.getIconPath();
                        if (iconPath == null) {
                            System.out.println("      Icon: <none>");
                        } else {
                            System.out.println("      Icon: " + iconPath.getFile());
                        }
                        System.out.println("     Model: " + JUMPAppModel.XLET.toString());
                    } else if (model == JUMPAppModel.MAIN) {
                        MAINApplication main = (MAINApplication)app;
                        System.out.println("    Bundle: " + main.getBundle());
                        System.out.println("       Jar: " + main.getClasspath());
                        System.out.println("Install ID: " + main.getId());
                        URL iconPath = main.getIconPath();
                        if (iconPath == null) {
                            System.out.println("      Icon: <none>");
                        } else {
                            System.out.println("      Icon: " + iconPath.getFile());
                        }
                        System.out.println("     Model: " + JUMPAppModel.MAIN.toString());
                    } else if (model == JUMPAppModel.MIDLET) {
                        System.out.println("  Suite ID: " + app.getProperty("MIDletApplication_suiteid"));
                        System.out.println("Install ID: " + app.getId());
                        URL iconPath = app.getIconPath();
                        if (iconPath == null) {
                            System.out.println("      Icon: <none>");
                        } else {
                            if (iconPath.getProtocol().equals("jar")) {
                                System.out.println("      Icon: jar:" + iconPath.getFile());
                            } else {
                                System.out.println("      Icon: " + iconPath.getFile());
                            }
                        }
                        System.out.println("     Model: " + JUMPAppModel.MIDLET.toString());
                    }
                    
                    System.out.println("");
                }
            }
        }
        
        System.out.println("");
    }
    
    private String getProtocol(String url) {
        if (url.endsWith(".jad")) {
            return JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA;
        } else if (url.endsWith(".dd")) {
            return JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA;
        } else {
            return null;
        }
    }    
    
    /**
     * Install the content described by the specified content descriptor file.
     * The content will be automatically downloaded before being installed.
     *
     * @param descriptorFileUrl A URL to a content desciptor file.
     * @return the installed content
     */
    public JUMPContent[] doInstall(String descriptorFileUrl) {
        return doInstall("<no title>", descriptorFileUrl);
    }
    
    /**
     * Install the content described by the specified content descriptor files.
     * The content will be automatically downloaded before being installed.
     *
     * @param descriptorFileUrl A URL to a content desciptor file.
     * @return the installed content
     */
    public JUMPContent[] doInstall(String descriptorFileUrl[]) {
        Vector contentVector = new Vector();
        for (int i = 0; i < descriptorFileUrl.length; i++) {
            JUMPContent content[] = doInstall("<no title>", descriptorFileUrl[i]);
            for (int j = 0; j < content.length; j++)     {
                contentVector.add(content[j]);
            }
        }
        return (JUMPContent[])contentVector.toArray(new JUMPContent[]{});
    }
    
    /**
     * Install content given a list of names and URIs
     * to content descriptor files.
     * @param downloadNames The title of the content.
     * The index of the array for this value
     * relates to the same index in teh
     * array for downloadURIs.
     * @param downloadURIs The URIs to the content descriptor
     * files.
     *
     * The index of the array for this value
     * relates to the same index in teh
     * array for downloadNames.
     * @return the installed content
     */
    public JUMPContent[] doInstall(String downloadNames[], String downloadURIs[]) {
        DownloadTool downloadTool = new DownloadTool();
        downloadTool.startTool(downloadNames, downloadURIs);
        URL contentURLs[] = downloadTool.getURLs();
        JUMPDownloadDescriptor[] descriptors = downloadTool.getDescriptors();
        JUMPContent[] content = install(contentURLs, descriptors);
        cleanup(contentURLs, descriptors);
        return content;
    }
    
    /**
     * Install content given a name and a URI
     * to a content descriptor file.
     * @param downloadName The name of the content
     * @param downloadURI The URI to the content descriptor file
     * @return the installed content
     */
    public JUMPContent[] doInstall(String downloadName, String downloadURI) {
        DownloadTool downloadTool = new DownloadTool();
        downloadTool.startTool(downloadName, downloadURI);
        URL contentURLs[] = downloadTool.getURLs();
        JUMPDownloadDescriptor[] descriptors = downloadTool.getDescriptors();
        JUMPContent[] content = install(contentURLs, descriptors);
        cleanup(contentURLs, descriptors);
        return content;
    }
    
    /**
     * Given a provisioning server URL, install content
     * @param provisioningServerURL The URL to a provisioning server
     * @param userInteractive When true, the user chooses an
     * application to install among a list
     * of available content.
     *
     * When false, all available content is
     * automatically installed.
     * @return the installed content
     */
    public JUMPContent[] doInstall(String provisioningServerURL, boolean userInteractive) {
        DownloadTool downloadTool = new DownloadTool(provisioningServerURL);
        downloadTool.startTool(userInteractive);
        URL contentURLs[] = downloadTool.getURLs();
        JUMPDownloadDescriptor[] descriptors = downloadTool.getDescriptors();
        JUMPContent[] content = install(contentURLs, descriptors);
        cleanup(contentURLs, descriptors);
        return content;
    }    
        
    private void cleanup(URL urls[], JUMPDownloadDescriptor[] descriptors) {
        // Remove locally downloaded content, i.e. tmp local jar files
        for (int i = 0; i < urls.length; i++) {
            File contentFile = new File(urls[i].getFile());
            if (contentFile.exists()) {
                System.out.println("*** Cleaning up tmp download content: " + contentFile.toString());
                contentFile.delete();
            }
        }
        // Remove locally downloaded jad files
        for (int i = 0; i < descriptors.length; i++) {
            Properties prop = descriptors[i].getApplications()[0];
            String localJadFile = prop.getProperty("JUMPApplication_localJadUrl");
            if (localJadFile == null) {
                continue;
            }
            File localJad = new File(localJadFile);
            if (localJad.exists()) {
                System.out.println("*** Cleaning up tmp jad: " + localJad.toString());
                localJad.delete();
            }
        }
    }
    
    /**
     * Uninstall content given the content type and
     * installed application id.
     * @param model the content type, i.e.,
     *  midlet, xlet, or main
     * @param id the installed application id
     */
    public void doUninstall(JUMPAppModel model, int id) {
        JUMPInstallerModule installer = createInstaller(model);
        JUMPContent content[] = installer.getInstalled();
        int i = 0;
        for (i = 0; i < content.length; i++) {
            JUMPApplication app = (JUMPApplication)content[i];
            if (app.getId() == id) {
                break;
            }
        }
        if (i < content.length) {
            uninstall((JUMPApplication)content[i]);
        }
    }
    
    /**
     * Uninstall content
     * @param app the application to uninstall
     */
    public void doUninstall(JUMPApplication app) {
        uninstall(app);
    }
    
    /**
     * Uninstall content
     * @param apps the content to uninstall
     */
    public void doUninstall(JUMPApplication apps[]) {
        uninstall(apps);
    }
    
    /**
     * Uninstall content
     * @param userInteractive if true, the user chooses an application
     * among a list of all installed content
     * to uninstall.
     *
     * if false, all installed content in
     * uninstalled.
     */
    public void doUninstall(boolean userInteractive) {
        if (userInteractive) {
            userInteractiveUninstall();
        } else {
            nonInteractiveUninstall();
        }
    }
    
    private void uninstall(JUMPApplication app) {
        JUMPApplication apps[] = new JUMPApplication[1];
        apps[0] = app;
        uninstall(apps);
    }
    
    private void uninstall(JUMPApplication[] apps) {
        if (apps == null) {
            trace("ERROR: No apps specified to uninstall.");
            error();
            return;
        }
        
        for (int i = 0; i < apps.length; i++) {
            System.out.println("");
            System.out.println("==> Uninstalling: " + apps[i].getTitle());
            if (apps[i] == null) {
                System.out.println("ERROR: " + apps[i].getTitle() + " not found in content store.");
            } else {
                JUMPInstallerModule installer = null;
                if (apps[i].getAppType() == JUMPAppModel.XLET) {
                    installer = createInstaller(JUMPAppModel.XLET);
                } else if (apps[i].getAppType() == JUMPAppModel.MAIN) {
                    installer = createInstaller(JUMPAppModel.MAIN);
                } else if (apps[i].getAppType() == JUMPAppModel.MIDLET) {
                    installer = createInstaller(JUMPAppModel.MIDLET);
                }
                installer.uninstall(apps[i]);
            }
            System.out.println("==> Finished Uninstalling: " + apps[i].getTitle());
            System.out.println("");
        }
    }
    
    private void userInteractiveUninstall() {
        System.setProperty("jump.installer.interactive", "true");
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        
        Vector appsVector = new Vector();
        
        // Get all of the apps
        for (int i = 0, totalApps = 0; i < installers.length; i++) {
            JUMPContent[] content = installers[i].getInstalled();
            if (content != null) {
                for(int j = 0; j < content.length; j++) {
                    appsVector.add(totalApps, content[j]);
                    totalApps++;
                }
            }
        }
        
        if (appsVector.size() == 0) {
            System.out.println("No applications are installed in the content store.");
            return;
        }
        
        // Show what is available and read input for a choice.
        System.out.println( "uninstall choices: " );
        Object apps[] = appsVector.toArray();
        for (int i = 0; i < apps.length ; i++ ) {
            System.out.println( "(" + i + "): " + ((JUMPApplication)apps[i]).getTitle());
        }
        
        String message = "Enter choice (-1 to exit) [-1]: ";
        String choice = Utilities.promptUser(message);
        int chosenUninstall = Integer.parseInt(choice);
        
        System.out.println( chosenUninstall );
        
        JUMPApplication app = (JUMPApplication)appsVector.get(chosenUninstall);
        
        JUMPApplication[] chosenApps = new JUMPApplication[1];
        chosenApps[0] = app;
        uninstall(chosenApps);
    }
    
    private void nonInteractiveUninstall() {
        System.setProperty("jump.installer.interactive", "false");
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        
        for (int i = 0; i < installers.length; i++) {
            JUMPContent[] content = installers[i].getInstalled();
            while (content != null && content.length > 0) {
                if (content[0] != null) {
                    System.out.println("");
                    System.out.println("==> Uninstalling: " + ((JUMPApplication)content[0]).getTitle());
                    installers[i].uninstall(content[0]);
                    System.out.println("==> Finished Uninstalling: " + ((JUMPApplication)content[0]).getTitle());
                    System.out.println("");
                }
                content = installers[i].getInstalled();
            }
        }
    }
    
    
    /**
     *  Install JUMP content
     *  url - URL of content to install, must be a file protocol URL
     *  desc - download descriptor object
     */
    private JUMPContent[] install(URL url[], JUMPDownloadDescriptor desc[]) {
        Vector contentVector = new Vector();
        
        if (url.length != desc.length) {
            System.err.println("ERROR: Number of URLs to install does not equal the number of given download descriptors.");
            error();
            return null;
        }
        for (int i = 0; i < url.length; i++) {
            if (desc != null && url != null) {
                if (!url[i].getProtocol().equals("file")) {
                    System.out.println("ERROR: Invalid protocol for:" + url[i].toString());
                    continue;
                }
                System.out.println("");
                System.out.println("==> Installing: " + desc[i].getName() + " from: " + url[i].toString());
                Properties apps[] = desc[i].getApplications();
                if (apps == null) {
                    trace("ERROR: Could not install. Descriptor contains no information on application.");
                    error();
                    return null;
                }
                String appType = apps[0].getProperty("JUMPApplication_appModel");
                JUMPInstallerModule installer = null;
                if (appType.equals("xlet")) {
                    installer = createInstaller(JUMPAppModel.XLET);
                } else if (appType.equals("main")) {
                    installer = createInstaller(JUMPAppModel.MAIN);
                } else if (appType.equals("midlet")) {
                    installer = createInstaller(JUMPAppModel.MIDLET);
                } else {
                    trace("ERROR: Unknown application type: " + appType);
                    error();
                    return null;
                }
                JUMPContent installedApps[] = installer.install(url[i], desc[i]);
                if (installedApps != null) {
                    // Print installed apps.
                    for(int j = 0; j < installedApps.length; j++) {
                        System.out.println("Application Installed: " + ((JUMPApplication)installedApps[j]).getTitle());
                        contentVector.add(installedApps[j]);
                    }
                } else {
                    System.out.println("ERROR: No applications were installed for: " + desc[i].getName() + ".");
                    error();
                }
                System.out.println("==> Finished Installing: " + desc[i].getName());
                System.out.println("");
            }
        }
        
        return (JUMPContent[])contentVector.toArray(new JUMPContent[]{});
    }
    
    /**
     * Print out a list of all installed content.
     */
    public void doList() {
        System.out.println("");
        System.out.println("---------------------------------");
        System.out.println("Applications Within Content Store");
        System.out.println("---------------------------------");
        System.out.println("");
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        
        int numApps = 0;
        
        for (int i = 0; i < installers.length; i++) {
            
            JUMPContent[] content = installers[i].getInstalled();
            if (content != null) {
                for(int j = 0; j < content.length; j++) {
                    numApps++;
                    System.out.println("App #" + numApps + ": " + ((JUMPApplication)content[j]).getTitle());
                }
            }
        }
        
        System.out.println("");
    }
    
    class DownloadTool {
        
        private String provisioningServerURL = null;
        
        // Values only used for a JSR 124 server
        private final String omaSubDirectory = "oma";
        private final String midpSubDirectory = "jam";
        
        private boolean downloadFinished = false;
        private boolean downloadAborted = false;
        
        String outputFile = null;
        
        byte[] buffer = null;
        int bufferIndex = 0;
        
        private Vector descriptorVector = null;
        private Vector urlVector = null;
        
        
        private String downloadNames[] = null;
        private String downloadURIs[] = null;
        
        public DownloadTool() {
            setup();
        }
        
        public DownloadTool(String provisioningServerURL) {
            this.provisioningServerURL = provisioningServerURL;
            setup();
        }
        
        void setup() {
            descriptorVector = new Vector();
            urlVector = new Vector();
        }
        
        public URL[] getURLs() {
            return (URL[])urlVector.toArray(new URL[]{});
        }
        
        public JUMPDownloadDescriptor[] getDescriptors() {
            return (JUMPDownloadDescriptor[])descriptorVector.toArray(new JUMPDownloadDescriptor[]{});
        }
        
        public void startTool(String downloadName, String downloadURI) {
            String tmpDownloadNames[] = new String[1];
            String tmpDownloadURIs[] = new String[1];
            tmpDownloadNames[0] = downloadName;
            tmpDownloadURIs[0] = downloadURI;
            startTool(tmpDownloadNames, tmpDownloadURIs);
        }
        
        public void startTool(String downloadNames[], String downloadURIs[]) {
            this.downloadNames = downloadNames;
            this.downloadURIs = downloadURIs;
            nonInteractiveDownload(downloadURIs, downloadNames);
        }
        
        public void startTool(boolean userInteractive) {
            // Determine the discovery URL
            if (provisioningServerURL != null) {
                System.out.println( "Using provisioning server URL at: " + provisioningServerURL );
            } else {
                System.out.println("A provisioning server url needs to be supplied.");
                System.out.println("Please run again with an value set to the -ProvisioningServerURL flag.");
                System.exit(0);
            }
            
            // Check if we're using a JSR 124 server
            if (provisioningServerURL.endsWith("ri-test")) {
                
                HashMap applistOMA = new OTADiscovery().discover(provisioningServerURL + "/" + omaSubDirectory);
                HashMap applistMIDP = new OTADiscovery().discover(provisioningServerURL + "/" + midpSubDirectory);
                
                downloadURIs = new String[ applistOMA.size() + applistMIDP.size() ];
                downloadNames = new String[ applistOMA.size() + applistMIDP.size() ];
                
                int i = 0;
                for ( Iterator e = applistOMA.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloadURIs[ i ] = s;
                    downloadNames[ i ] = (String)applistOMA.get( s );
                    i++;
                }
                
                for ( Iterator e = applistMIDP.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloadURIs[ i ] = s;
                    downloadNames[ i ] = (String)applistMIDP.get( s );
                    i++;
                }
            } else {
                // we're using an apache-based server
                HashMap applist = new OTADiscovery().discover(provisioningServerURL);
                
                downloadURIs = new String[ applist.size() ];
                downloadNames = new String[ applist.size() ];
                
                int i = 0;
                for ( Iterator e = applist.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloadURIs[ i ] = s;
                    downloadNames[ i ] = (String)applist.get( s );
                    i++;
                }
            }
            
            if (userInteractive) {
                userInteractiveDownload(downloadURIs, downloadNames);
            } else {
                nonInteractiveDownload(downloadURIs, downloadNames);
            }
            
        }
        
        void nonInteractiveDownload(String[] downloadURIs, String[] downloadNames) {
            for (int i = 0; i < downloadURIs.length; i++) {
                trace("Downloading: " + downloadNames[i]);
                doDownload(downloadNames[i], downloadURIs[i]);
            }
        }
        
        void userInteractiveDownload(String[] downloadURIs, String[] downloadNames) {
            
            // Show what is available and read input for a choice.
            System.out.println( "download choices: " );
            for (int i = 0; i < downloadNames.length ; i++ ) {
                System.out.println( "(" + i + "): " + downloadNames[ i ] );
            }
            String message = "Enter choice (-1 to exit) [-1]: ";
            String choice = Utilities.promptUser(message);
            System.out.println("--> choice: " + choice);
            int chosenDownload = Integer.parseInt(choice);
            System.out.println("--> chosenDownload: " + chosenDownload);
            // If no valid choice, quit
            if ( chosenDownload < 0 ) {
                System.exit( 0 );
            }
            
            System.out.println( chosenDownload + ": " + downloadURIs[ chosenDownload ] );
            boolean rv = doDownload(downloadNames[chosenDownload], downloadURIs[chosenDownload]);
        }
        
        private boolean doDownload(String name, String uri) {
            // Initiate a download. We've specified ourselves
            // as the handler of the data.
            if (uri.endsWith(".dd")) {
                startDownload(name, uri, JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
            } else if (uri.endsWith(".jad")) {
                startDownload(name, uri, JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
            } else {
                System.out.println("ERROR: Unknown URI type: " + uri);
                System.exit(0);
            }
            
            // Wait for either failure or success
            while ( downloadFinished == false &&
                    downloadAborted == false ) {
                System.out.println( "waiting for download" );
                try {
                    Thread.sleep( 100 );
                } catch ( java.lang.InterruptedException ie ) {
                    ie.printStackTrace();
                    // Eat it
                }
            }
            
            // Some resolution
            if ( ! downloadFinished ) {
                trace( "Download failed!" );
                return false;
            } else {
                trace( "Download succeeded!" );
                return true;
            }
        }
        
        void startDownload(String name, String uri, String protocol) {
            downloadFinished = false;
            downloadAborted = false;
            
            System.out.println("");
            System.out.println("==> Downloading: " + name);
            
            trace( "Creating descriptor for " + uri );
            
            JUMPDownloadModule module =
                    JUMPDownloadModuleFactory.getInstance().getModule(protocol);
            
            try {
                
                JUMPDownloadDescriptor descriptor = module.createDescriptor( uri );
                if (descriptor == null) {
                    System.out.println("ERROR: Returned descriptor is NULL for " + uri);
                    System.exit(0);
                }
                
                JUMPDownloader downloader = module.createDownloader(descriptor);
                JUMPDownloadDestination destination = new DownloadDestinationImpl(descriptor);
                
                // Trigger the download
                URL url = downloader.start( destination );
                trace( "Download returns url: " + url);
                
                downloadFinished = true;
                
                descriptorVector.add(descriptor);
                urlVector.add(url);
            } catch ( JUMPDownloadException o ) {
                System.out.println( "Download failed for " + uri);
                if (Verbose) {
                    o.printStackTrace();
                }
                downloadAborted = true;
            } catch ( Exception o ) {
                System.out.println( "Download failed for " + uri);
                if (Verbose) {
                    o.printStackTrace();
                }
                downloadAborted = true;
            } finally {
                System.out.println("==> Finished Downloading: " + name);
                System.out.println("");
            }
        }
    }
    
    /**
     * The main method when used as a standalone tool.
     * @param args program args.  See docs for details.
     */
    public static void main(String[] args) {
        new JUMPInstallerTool(args);
    }
}
