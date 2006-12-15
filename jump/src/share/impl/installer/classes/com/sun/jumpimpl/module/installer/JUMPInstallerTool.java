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
 *    list, install, uninstall
 *
 * Usage:
 *   <cvm> -Dinstaller.repository=<repository dir> <options> -cp <classpath> com.sun.jumpimpl.module.installer.JUMPInstallerTool -command <command>
 *     <command> can currently be list, install, and uninstall
 *     <options> can be none or any of the following:
 *        -ProvisioningServerURL <url of provisioning server>
 *        -OMAdiscoverURL <discover url for OMA downloads>
 *        -MIDPdiscoverURL <discover url for MIDP downloads>
 *
 * Ex:
 *   cvm -Dinstaller.repository=/my/repository -cp $JUMP_LIBDIR/jump-api.jar:$JUMP_LIBDIR/jump-impl.jar com.sun.jumpimpl.module.installer.JUMPInstallerTool -command list
 *   cvm -Dinstaller.repository=/my/repository -cp $JUMP_LIBDIR/jump-api.jar:$JUMP_LIBDIR/jump-impl.jar com.sun.jumpimpl.module.installer.JUMPInstallerTool -command install
 *   cvm -Dinstaller.repository=/my/repository -cp $JUMP_LIBDIR/jump-api.jar:$JUMP_LIBDIR/jump-impl.jar com.sun.jumpimpl.module.installer.JUMPInstallerTool -command uninstall
 *
 * Note: This tool must be run with the cvm only.
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
     * URL used for OMA content
     */
    protected String OMAdiscoverURL = null;
    /**
     * URL used MIDP content
     */
    protected String MIDPdiscoverURL = null;
    /**
     * The current command to be run
     */
    protected String Command = null;
    /**
     * Sub-values for the current command to be run
     */
    protected String Value = null;
    /**
     * The current root of content store where applications are located
     */
    private String repository = null;
    /**
     * The property name holding the root of content store
     */
    private static final String repositoryProperty = "installer.repository";
    
    /**
     * Creates a new instance of JUMPInstallerTool
     * @param hash properties
     */
    public JUMPInstallerTool(Hashtable hash) {
        this.ProvisioningServer = (String)hash.get("ProvisioningServer");
        this.OMAdiscoverURL = (String)hash.get("OMAdiscoverURL");
        this.MIDPdiscoverURL = (String)hash.get("MIDPdiscoverURL");
        this.Command = (String)hash.get("Command");
        this.Value = (String)hash.get("Value");
        
        if (!setup()) {
            System.exit(-1);
        };
        
        doCommand();
    }
    
    private void usage() {
        System.out.println("Usage:");
        System.out.println("  <cvm> -Dinstaller.repository=<repository dir> -cp <classpath> com.sun.jumpimpl.module.installer.JUMPInstallerTool -command <command>");
        System.out.println("Available commands that can be used are:  list, install, uninstall");
        System.out.println("");
        System.out.println("Ex:");
        System.out.println("  cvm -Dinstaller.repository=/my/repository -cp $JUMP_LIBDIR/jump-api.jar:$JUMP_LIBDIR/jump-impl.jar com.sun.jumpimpl.module.installer.JUMPInstallerTool -command list");
        System.out.println("");
        System.out.println("Note: This tool must be run with the cvm only.");
    }
    
    private boolean setup() {
        
        if (Command == null) {
            System.out.println("ERROR: No command specified.");
            usage();
            System.exit(0);
        }
        
        repository = System.getProperty(repositoryProperty);
        if (repository == null) {
            System.out.println("ERROR: The property installer.repository is not set.");
            return false;
        }
        
        // test setup, make a repository root
        File file = new File(repository);
        if (!file.exists()) {
            System.out.println("ERROR: " + repository + " directory not found");
            return false;
        }
        
        if (JUMPExecutive.getInstance() == null) {
            // This one line should be called by the executive, but doing it here for time being.
            new com.sun.jumpimpl.module.installer.InstallerFactoryImpl();
            
            // This one line should be called by the executive, but doing it here for time being.
            new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();
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
        
        // These three lines below should have happened in the executive setup,
        // but for the testing purpose, emulating load() call here.
        if (JUMPExecutive.getInstance() == null) {
            HashMap map = new HashMap();
            map.put(repositoryProperty, repository);
            module.load(map);
        }
        return module;
    }
    
    private void doCommand() {
        if (Command.equals("install")) {
            doInstall();
        } else if (Command.equals("list")) {
            doList();
        } else if (Command.equals("uninstall")) {
            doUninstall();
        }
    }
    
    private void doInstall() {
        
        DownloadTool test = new DownloadTool();
        test.startTool(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
        JUMPDownloadDescriptor desc = test.getDescriptor();
        URL url = test.getURL();
        
        if (desc != null && url != null) {
            Properties apps[] = desc.getApplications();
            if (apps == null) {
                System.out.println("ERROR: Could not install. Descriptor contains no information on application.");
                return;
            }
            String appType = apps[0].getProperty("JUMPApplication_appModel");
            JUMPInstallerModule installer = null;
            if (appType.equals("xlet")) {
                installer = createInstaller(JUMPAppModel.XLET);
            } else if (appType.equals("main")) {
                installer = createInstaller(JUMPAppModel.MAIN);
            } else if (appType.equals("midlet")) {
                System.out.println("NOTE: MIDP installations are currently not supported.");
                installer = createInstaller(JUMPAppModel.MIDLET);
            } else {
                return;
            }
            JUMPContent installedApps[] = installer.install(url, desc);
            if (installedApps != null) {
                // Print installed apps.
                for(int i = 0; i < apps.length; i++) {
                    System.out.println("Installed : " + ((JUMPApplication)installedApps[i]).getTitle());
                }
            } else {
                System.out.println("No applications were installed.");
            }
        }
    }
    
    private void doUninstall() {
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        
        Vector appsVector = new Vector();
        
        // Get all of the apps
        for (int i = 0, totalApps = 0; i < installers.length; i++) {
            
            // These three lines below should have happened in the executive setup,
            // but for the testing purpose, emulating load() call here.
            HashMap map = new HashMap();
            map.put(repositoryProperty, repository);
            installers[i].load(map);
            
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
        
        if (app == null) {
            System.out.println("  ERROR: Application not found in content store.");
        } else {
            JUMPInstallerModule installer = null;
            if (app.getAppType() == JUMPAppModel.XLET) {
                installer = createInstaller(JUMPAppModel.XLET);
            } else if (app.getAppType() == JUMPAppModel.MAIN) {
                installer = createInstaller(JUMPAppModel.MAIN);
            } else if (app.getAppType() == JUMPAppModel.MIDLET) {
                installer = createInstaller(JUMPAppModel.MIDLET);
            }
            installer.uninstall(app);
        }
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
        
        private final String omaSubDirectory = "oma";
        private final String midpSubDirectory = "jam";
        
        private boolean downloadFinished = false;
        private boolean downloadAborted = false;
        
        String outputFile = null;
        String provisioningUrl = null;
        
        byte[] buffer = null;
        int bufferIndex = 0;
        
        private JUMPDownloadDescriptor descriptor = null;
        private URL url = null;
        
        public DownloadTool() {
            setup();
        }
        
        void setup() {
            
            // This one line should be called by the executive in real impl
            new com.sun.jumpimpl.module.download.DownloadModuleFactoryImpl();
            
            // Determine the discovery URL
            if (ProvisioningServer != null) {
                provisioningUrl = ProvisioningServer;
                System.out.println( "Using provisioning server at: " + provisioningUrl );
            } else {
                System.out.println("A Provisioning server url needs to be supplied.");
                System.out.println("Please run again with an value set to the -ProvisioningServerURL flag.");
                System.exit(0);
            }
            
        }
        
        public URL getURL() {
            return url;
        }
        
        public JUMPDownloadDescriptor getDescriptor() {
            return descriptor;
        }
        
        void startTool(String protocol) {
            
            HashMap applistOMA = null;
            HashMap applistMIDP = null;
            
            if (OMAdiscoverURL != null) {
                applistOMA = new OTADiscovery().discover(OMAdiscoverURL);
            } else {
                applistOMA = new OTADiscovery().discover(provisioningUrl + "/" + omaSubDirectory);
            }
            if (MIDPdiscoverURL != null) {
                applistMIDP = new OTADiscovery().discover(MIDPdiscoverURL);
            } else {
                applistMIDP = new OTADiscovery().discover(provisioningUrl + "/" + midpSubDirectory);
            }
            
            String[] downloads = new String[ applistOMA.size() + applistMIDP.size() ];
            String[] downloadNames = new String[ applistOMA.size() + applistMIDP.size() ];
            
            int i = 0;
            for ( Iterator e = applistOMA.keySet().iterator(); e.hasNext(); ) {
                String s = (String)e.next();
                downloads[ i ] = s;
                downloadNames[ i ] = (String)applistOMA.get( s );
                i++;
            }
            
            int midpIndex = i;
            
            for ( Iterator e = applistMIDP.keySet().iterator(); e.hasNext(); ) {
                String s = (String)e.next();
                downloads[ i ] = s;
                downloadNames[ i ] = (String)applistMIDP.get( s );
                i++;
            }
            
            // Show what is available and read input for a choice.
            System.out.println( "download choices: " );
            for ( i = 0; i < downloadNames.length ; i++ ) {
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
            
            System.out.println( chosenDownload );
            
            // Initiate a download. We've specified ourselves
            // as the handler of the data.
            if (chosenDownload < midpIndex) {
                startDownload( downloads[ chosenDownload ], JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
            } else {
                startDownload( downloads[ chosenDownload ], JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
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
                System.out.println( "Download failed!" );
            } else {
                System.out.println( "download succeeded. save the results" );
            }
        }
        
        void startDownload( String uri, String protocol ) {
            downloadFinished = false;
            downloadAborted = false;
            
            System.out.println( "creating descriptor for " + uri );
            
            JUMPDownloadModule module =
                    JUMPDownloadModuleFactory.getInstance().getModule(protocol);
            
            try {
                
                descriptor = module.createDescriptor( uri );
                
                JUMPDownloader downloader = module.createDownloader(descriptor);
                
                JUMPDownloadDestination destination = new DownloadDestinationImpl(descriptor);
                
                // Trigger the download
                url = downloader.start( destination );
                System.out.println( "download returns " + url);
                
                downloadFinished = true;
            } catch ( JUMPDownloadException o ) {
                System.out.println( "download failed for " + uri +
                        ": " + o.getMessage() );
                o.printStackTrace();
                downloadAborted = true;
            } catch ( Exception o ) {
                System.out.println( "download failed for " + uri +
                        ": " + o.getMessage() );
                o.printStackTrace();
                downloadAborted = true;
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
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-OMAdiscoverURL")) {
                arg = args[++i];
                argTable.put("OMAdiscoverURL", arg);
            } else if (args[i].equals("-MIDPdiscoverURL")) {
                arg = args[++i];
                argTable.put("MIDPdiscoverURL", arg);
            } else if (args[i].equals("-ProvisioningServerURL")) {
                arg = args[++i];
                argTable.put("ProvisioningServerURL", arg);
            } else if (args[i].equals("-command")) {
                arg = args[++i];
                argTable.put("Command", arg);
                if ((i + 1)  < args.length) {
                    arg = args[++i];
                    argTable.put("Value", arg);
                }
            }
        }
        
        new JUMPInstallerTool(argTable);
    }
}
