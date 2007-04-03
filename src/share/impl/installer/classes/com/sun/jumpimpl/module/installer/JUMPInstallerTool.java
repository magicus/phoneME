/*
 * %W% %E%
 *
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
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
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
 *    list, install, install_all, uninstall, uninstall_all
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
 *     <command> can currently be list, install, install_all, uninstall, and uninstall_all
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
     * The current root of content store where applications are located
     */
    private String repository = null;
    /**
     * The property name holding the root of content store
     */
    private static final String repositoryProperty = "contentstore.root";
    
    /**
     * Creates a new instance of JUMPInstallerTool
     * @param hash properties
     */
    public JUMPInstallerTool(Hashtable hash) {
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
        
        trace("JUMPInstallerTool Settings:");
        trace("    Command: " + Command);
        trace(    "ProvisioningServerURL: " + ProvisioningServer);
        trace("");
        
        if (!setup()) {
            System.exit(-1);
        };
        
        doCommand();
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
        
        if (Command == null) {
            System.out.println("ERROR: No command specified.");
            usage();
            System.exit(0);
        }
        
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
            module = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.MIDLET);     }
        
        if (module == null)  {
            return null;
        }
        
        return module;
    }
    
    private void doCommand() {
        if (Command.equals("install")) {
            if (ContentURL != null && DescriptorURI != null && Protocol != null) {
                doInstall(ContentURL, DescriptorURI, Protocol);
            } else {
                doInstall(ProvisioningServer, true);
            }
        } else if (Command.equals("install_all")) {
            doInstall(ProvisioningServer, false);
        } else if (Command.equals("list")) {
            doList();
        } else if (Command.equals("uninstall")) {
            doUninstall(true);
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
    
    private void doInfo() {
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
                        System.out.println("  Bundle: " + xlet.getBundle());
                        System.out.println("     Jar: " + xlet.getClasspath());
                        System.out.println("      ID: " + xlet.getId());
                        System.out.println("    Icon: " + xlet.getIconPath());
                        System.out.println("   Model: " + JUMPAppModel.XLET.toString());
                    } else if (model == JUMPAppModel.MAIN) {
                        MAINApplication main = (MAINApplication)app;
                        System.out.println("  Bundle: " + main.getBundle());
                        System.out.println("     Jar: " + main.getClasspath());
                        System.out.println("      ID: " + main.getId());
                        System.out.println("    Icon: " + main.getIconPath());
                        System.out.println("   Model: " + JUMPAppModel.MAIN.toString());                        
                    } else if (model == JUMPAppModel.MIDLET) {
                        
                    }
                    
                    System.out.println("");
                }
            }
        }
        
        System.out.println("");
    }
    
    
    private void doInstall(String url, String uri, String protocol) {
        JUMPDownloadModule module = null;
        
        if (protocol.equals(JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA)) {
            module = JUMPDownloadModuleFactory.getInstance().getModule(JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
        } else if (protocol.equals(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA)) {
            module = JUMPDownloadModuleFactory.getInstance().getModule(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
        } else {
            System.err.println("Error: Unknown protocol: " + protocol);
            error();
            return;
        }
        
        trace( "doInstall - Creating descriptor for " + uri );
        JUMPDownloadDescriptor descriptor = null;
        try {
            descriptor = module.createDescriptor( uri );
        } catch (JUMPDownloadException ex) {
            ex.printStackTrace();
        }
        if (descriptor == null) {
            trace("doInstall - Could not create descriptor.");
            error();
            return;
        }
        
        URL contentURL = null;
        try {
            if (url.startsWith("file://")) {
                url = url.substring(7);
            } else if (url.startsWith("http://")) {
                System.err.println
                        ("ERROR: The tool does not support descriptors using the http:// protocol.");
                return;
            }
            contentURL = new URL("file", null, url);
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        JUMPDownloadDescriptor[] descriptors = new JUMPDownloadDescriptor[1];
        descriptors[0] = descriptor;
        
        URL[] contentURLs = new URL[1];
        contentURLs[0] = contentURL;
        install(contentURLs, descriptors);
    }
    
    private void install(URL url[], JUMPDownloadDescriptor desc[]) {
        
        if (url.length != desc.length) {
            System.err.println("ERROR: Number of URLs to install does not equal the number of given download descriptors.");
            error();
            return;
        }
        for (int i = 0; i < url.length; i++) {
            if (desc != null && url != null) {
                System.out.println("");
                System.out.println("==> Installing: " + desc[i].getName());
                Properties apps[] = desc[i].getApplications();
                if (apps == null) {
                    trace("ERROR: Could not install. Descriptor contains no information on application.");
                    error();
                    return;
                }
                String appType = apps[0].getProperty("JUMPApplication_appModel");
                JUMPInstallerModule installer = null;
                if (appType.equals("xlet")) {
                    installer = createInstaller(JUMPAppModel.XLET);
                } else if (appType.equals("main")) {
                    installer = createInstaller(JUMPAppModel.MAIN);
                } else if (appType.equals("midlet")) {
                    //System.out.println("NOTE: MIDP installations are currently not supported.");
                    installer = createInstaller(JUMPAppModel.MIDLET);
                } else {
                    trace("ERROR: Unknown application type: " + appType);
                    error();
                    return;
                }
                JUMPContent installedApps[] = installer.install(url[i], desc[i]);
                if (installedApps != null) {
                    // Print installed apps.
                    for(int j = 0; j < installedApps.length; j++) {
                        System.out.println("Application Installed: " + ((JUMPApplication)installedApps[j]).getTitle());
                    }
                } else {
                    System.out.println("ERROR: No applications were installed for: " + desc[i].getName() + ".");
                    error();
                }
                System.out.println("==> Finished Installing: " + desc[i].getName());
                System.out.println("");
            }
        }
    }
    
    private void doInstall(String provisioningServerURL, boolean userInteractive) {
        DownloadTool downloadTool = new DownloadTool(provisioningServerURL);
        downloadTool.startTool(userInteractive);
        install(downloadTool.getURLs(), downloadTool.getDescriptors());
    }
    
    private void doUninstall(boolean userInteractive) {
        if (userInteractive) {
            userInteractiveUninstall();
        } else {
            nonInteractiveUninstall();
        }
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
        
        int chosenUninstall = -1;
        
        while ( true ) {
            System.out.print( "Enter choice (-1 to exit) [-1]: " );
            BufferedReader in =
                    new BufferedReader( new InputStreamReader( System.in ) );
            String answer;
            
            try {
                answer = in.readLine();
            } catch ( java.io.IOException ioe ) {
                continue;
            }
            
            if ( "".equals( answer ) ) {
                break;
            }
            
            try {
                chosenUninstall = Integer.parseInt( answer );
                break;
            } catch ( Exception e ) {
                e.printStackTrace();
                // bad input
            }
        }
        
        // If no valid choice, quit
        if ( chosenUninstall < 0 ) {
            System.exit( 0 );
        }
        
        System.out.println( chosenUninstall );
        
        JUMPApplication app = (JUMPApplication)appsVector.get(chosenUninstall);
        
        JUMPApplication[] chosenApps = new JUMPApplication[1];
        chosenApps[0] = app;
        uninstall(chosenApps);
    }
    
    private void nonInteractiveUninstall() {
        
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
        
        JUMPApplication[] apps = (JUMPApplication[])appsVector.toArray(new JUMPApplication[]{});
        uninstall(apps);
    }
    
    private void doList() {
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
        
        public DownloadTool(String provisioningServerURL) {
            this.provisioningServerURL = provisioningServerURL;
            setup();
        }
        
        void setup() {
            
            // Determine the discovery URL
            if (provisioningServerURL != null) {
                System.out.println( "Using provisioning server URL at: " + provisioningServerURL );
            } else {
                System.out.println("A provisioning server url needs to be supplied.");
                System.out.println("Please run again with an value set to the -ProvisioningServerURL flag.");
                System.exit(0);
            }
            
            descriptorVector = new Vector();
            urlVector = new Vector();
        }
        
        public URL[] getURLs() {
            return (URL[])urlVector.toArray(new URL[]{});
        }
        
        public JUMPDownloadDescriptor[] getDescriptors() {
            return (JUMPDownloadDescriptor[])descriptorVector.toArray(new JUMPDownloadDescriptor[]{});
        }
        
        void startTool() {
            startTool(true);
        }
        
        void startTool(boolean userInteractive) {
            
            String[] downloads = null;
            String[] downloadNames = null;
            
            if (provisioningServerURL == null) {
                System.err.println("ERROR: A provisioning URL needs to be specified.");
                System.exit(0);
            }
            
            // Check if we're using an apache-based server
            if (provisioningServerURL.endsWith("showbundles.py")) {
                HashMap applist = new OTADiscovery().discover(provisioningServerURL);
                
                downloads = new String[ applist.size() ];
                downloadNames = new String[ applist.size() ];
                
                int i = 0;
                for ( Iterator e = applist.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloads[ i ] = s;
                    downloadNames[ i ] = (String)applist.get( s );
                    i++;
                }
                // Check if we're using a JSR 124 server
            } else if (provisioningServerURL.endsWith("ri-test")) {
                
                HashMap applistOMA = new OTADiscovery().discover(provisioningServerURL + "/" + omaSubDirectory);
                HashMap applistMIDP = new OTADiscovery().discover(provisioningServerURL + "/" + midpSubDirectory);
                
                downloads = new String[ applistOMA.size() + applistMIDP.size() ];
                downloadNames = new String[ applistOMA.size() + applistMIDP.size() ];
                
                int i = 0;
                for ( Iterator e = applistOMA.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloads[ i ] = s;
                    downloadNames[ i ] = (String)applistOMA.get( s );
                    i++;
                }
                
                for ( Iterator e = applistMIDP.keySet().iterator(); e.hasNext(); ) {
                    String s = (String)e.next();
                    downloads[ i ] = s;
                    downloadNames[ i ] = (String)applistMIDP.get( s );
                    i++;
                }
            } else {
                System.out.println("ERROR:  Bad Provisioning Server URL: " + provisioningServerURL);
                System.exit(0);
            }
            
            if (userInteractive) {
                userInteractiveDownload(downloads, downloadNames);
            } else {
                nonInteractiveDownload(downloads, downloadNames);
            }
            
        }
        
        void nonInteractiveDownload(String[] downloads, String[] downloadNames) {
            for (int i = 0; i < downloads.length; i++) {
                trace("Downloading: " + downloadNames[i]);
                doDownload(downloadNames[i], downloads[i]);
            }
        }
        
        void userInteractiveDownload(String[] downloads, String[] downloadNames) {
            
            // Show what is available and read input for a choice.
            System.out.println( "download choices: " );
            for (int i = 0; i < downloadNames.length ; i++ ) {
                System.out.println( "(" + i + "): " + downloadNames[ i ] );
            }
            
            int chosenDownload = -1;
            
            while ( true ) {
                System.out.print( "Enter choice (-1 to exit) [-1]: " );
                BufferedReader in =
                        new BufferedReader( new InputStreamReader( System.in ) );
                String answer;
                
                try {
                    answer = in.readLine();
                } catch ( java.io.IOException ioe ) {
                    continue;
                }
                
                if ( "".equals( answer ) ) {
                    break;
                }
                
                try {
                    chosenDownload = Integer.parseInt( answer );
                    break;
                } catch ( Exception e ) {
                    e.printStackTrace();
                    // bad input
                }
            }
            
            // If no valid choice, quit
            if ( chosenDownload < 0 ) {
                System.exit( 0 );
            }
            
            System.out.println( chosenDownload + ": " + downloads[ chosenDownload ] );
            boolean rv = doDownload(downloadNames[chosenDownload], downloads[chosenDownload]);
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
                    System.out.println("Descriptor is NULL");
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
     *
     * @param args
     */
    public static void main(String[] args) {
        
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
            }
        }
        
        new JUMPInstallerTool(argTable);
    }
}
