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
import com.sun.jump.module.installer.*;
import com.sun.jumpimpl.module.download.DownloadDestinationImpl;
import com.sun.jumpimpl.module.download.OTADiscovery;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.HashMap;
import java.io.File;
import java.util.Iterator;
import java.util.Properties;

/**
 * This class contains test programs for the installer module.
 * The JUMPInstallerTool class is also a useful tool for testing.
 */
public class InstallerTests {
    JUMPInstallerModule xletInstaller = null;
    JUMPInstallerModule midletInstaller = null;
    JUMPInstallerModule mainInstaller = null;
    String repository = null;
    
    /**
     * Constructor class, runs the tests.
     */
    public InstallerTests() {
        checkReturn(setup(), "Setup");
        checkReturn(InstallerTest001(), "InstallerTest001");
    }
    
    private boolean setup() {
        repository = System.getProperty("contentstore.root");
        if (repository == null) {
            return false;
        }
        
        if (JUMPExecutive.getInstance() == null) {
            // This one line should be called by the executive, but doing it here for time being.
            new com.sun.jumpimpl.module.installer.InstallerFactoryImpl();
            
            // This one line should be called by the executive, but doing it here for time being.
            new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();
        }
        
        // test setup, make a repository root
        File file = new File(repository);
        if (!file.exists()) {
            System.out.println(repository + " directory not found");
            return false;
        }
        
        xletInstaller = createInstaller(JUMPAppModel.XLET);
        midletInstaller = createInstaller(JUMPAppModel.MIDLET);
        mainInstaller = createInstaller(JUMPAppModel.MAIN);
        return true;
    }
    
    private void checkReturn(boolean returnCode, String description) {
        if (returnCode) {
            System.out.println("*** InstallerTest: " + description + " PASSED. ***");
        } else {
            System.out.println("*** InstallerTest: " + description + " FAILED. ***");
            System.exit(0);
        }
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
    
    /**
     * Contacts a JSR 124 server expecting applications to be installed.
     * @return resulting value
     */
    public boolean InstallerTest001() {
        DownloadTool test = new DownloadTool();
        test.startTool(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
        JUMPDownloadDescriptor desc = test.getDescriptor();
        URL url = test.getURL();
        
        if (desc != null && url != null) {
            System.out.println("About to install...");
            Properties apps[] = desc.getApplications();
            if (apps == null) {
                return false;
            }
            String appType = apps[0].getProperty("JUMPApplication_appModel");
            JUMPInstallerModule installer = null;
            if (appType.equals("xlet")) {
                installer = xletInstaller;
            } else if (appType.equals("main")) {
                installer = mainInstaller;
            } else if (appType.equals("midlet")) {
                installer = midletInstaller;
            } else {
                return false;
            }
            JUMPContent installedApps[] = installer.install(url, desc);
            // Print installed apps.
            for(int i = 0; i < apps.length; i++) {
                System.out.println("Installed : " + ((JUMPApplication)installedApps[i]).getTitle());
            }
            return true;
        }
        return false;
    } 
    
    /**
     * Class representing a tool that contacts and downloads from a JSR 124 server
     */
    public class DownloadTool {
        
        private final String DU_PROP =
                "com.sun.jumpimpl.module.download.discoveryUrl";
        private final String OF_PROP =
                "com.sun.jumpimpl.module.download.outputFile";
        
        private final String omaSubDirectory = "oma";
        private final String midpSubDirectory = "jam";
        
        private boolean debug = false;
        private boolean useIndicator = false;
        
        private boolean downloadFinished = false;
        private boolean downloadAborted = false;
        
        String outputFile = null;
        String discoveryUrl = null;
        
        byte[] buffer = null;
        int bufferIndex = 0;
        
        private JUMPDownloadDescriptor descriptor = null;
        private URL url = null;
        
        /**
         * Constructor class
         */
        public DownloadTool() {
            setup();
            //startTool(JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
            //startTool(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
        }
        
        void setup() {
            
            // This one line should be called by the executive in real impl
            new com.sun.jumpimpl.module.download.DownloadModuleFactoryImpl();
            
            // Determine the discovery URL
            discoveryUrl = System.getProperty( DU_PROP );
            if (discoveryUrl == null) {
                System.out.println("The property com.sun.jumpimpl.module.download.discoveryUrl needs to be set for the test to run.");
                System.exit(0);
            }
            System.out.println( "using discovery URL: " + discoveryUrl );
        }
        
        /**
         * URL of downloaded content to be used for installation.
         * @return URL of downloaded content
         */
        public URL getURL() {
            return url;
        }
        
        /**
         * Get a download descriptor object to be used during installation
         * @return download descriptor object
         */
        public JUMPDownloadDescriptor getDescriptor() {
            return descriptor;
        }
        
        void startTool(String protocol) {
            
            HashMap applist = null;
            
            System.out.println("Starting the test run for protocol " + protocol);
            
            if (protocol == JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA) {
                
                applist = new OTADiscovery().discover(discoveryUrl + "/" + omaSubDirectory);
            } else if (protocol == JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA) {
                applist = new OTADiscovery().discover(discoveryUrl + "/" + midpSubDirectory);
            } else {
                System.out.println("Unknown protocol, aborting");
            }
            
            String[] downloads = new String[ applist.size() ];
            String[] downloadNames = new String[ applist.size() ];
            int i = 0;
            for ( Iterator e = applist.keySet().iterator(); e.hasNext(); ) {
                String s = (String)e.next();
                downloads[ i ] = s;
                downloadNames[ i ] = (String)applist.get( s );
                //System.out.println( "key " + downloads[ i ] +
                //       ", value " +
                //       downloadNames[ i ] );
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
            startDownload( downloads[ chosenDownload ], protocol);
            
            // Wait for either failure or success
            while ( downloadFinished == false &&
                    downloadAborted == false ) {
                System.out.println( "waiting for download" );
                try {
                    Thread.sleep( 100 );
                } catch ( java.lang.InterruptedException ie ) {
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
     * main point of entry for the tool
     * @param args
     */
    public static void main(String[] args) {
        new InstallerTests();
    }
}
