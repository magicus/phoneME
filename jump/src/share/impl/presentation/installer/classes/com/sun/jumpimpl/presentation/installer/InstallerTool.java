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

package com.sun.jumpimpl.presentation.installer;

import com.sun.jump.module.presentation.JUMPPresentationModule;
import com.sun.jumpimpl.module.installer.JUMPInstallerTool;
import java.util.Map;

/**
 * This JUMP Presentation mode contains support for installing,
 * uninstalling, and listing installed content.
 */
public class InstallerTool implements JUMPPresentationModule {
    
    static private boolean verbose;
    private String installToolArgs[] = null;
    
    /**
     * Create an instance of InstallerTool
     */
    public InstallerTool() {
    }
    
    /**
     * This method converts the executive properties into
     * application arguments that the JUMPInstallerTool recognizes.
     */
    private String[] parseToolProperties(Map map) {
        String str[] = null;
        
        String arg1 = (String)map.get("jump.presentation.installer.arg1");
        String arg2 = (String)map.get("jump.presentation.installer.arg2");
        String arg3 = (String)map.get("jump.presentation.installer.arg3");
        String verbose = (String)map.get("jump.installer.verbose");
        
        trace("********************************");
        trace("parseToolProperties() arg1: " + arg1);
        trace("parseToolProperties() arg2: " + arg2);
        trace("parseToolProperties() arg3: " + arg3);
        trace("********************************");
        
        String command = arg1;
        if (command.equals("install")) {
            if (arg2 != null) {
                str = new String[4];
                str[0] = "-command";
                str[1] = command;
                str[2] = "-DescriptorURI";
                str[3] = arg2;
            } else {
                System.err.println("ERROR: A content descriptor file has not been specified.");
                return null;
            }
        } else if (command.equals("install_all")) {        
            str = new String[6];
            str[0] = "-command";
            str[1] = command;
            str[2] = "-DescriptorURI";
            str[3] = arg2;
            str[4] = "-ProvisioningServerURL";
            str[5] = (String)map.get("jump.installer.provisionURL");
        } else if (command.equals("uninstall")) {
            str = new String[6];
            str[0] = "-command";
            str[1] = command;
            str[2] = "-type";
            str[3] = arg2;
            str[4] = "-id";
            str[5] = arg3;            
        } else if (command.equals("info") || command.equals("list") || command.equals("uninstall_all"))    {
            str = new String[2];
            str[0] = "-command";
            str[1] = command;
        } else {
            System.err.println("ERROR: Unknown command given: " + command);
            return null;
        }
        
        // Append the -verbose option if needed.
        String newStr[] = null;
        if (verbose != null && verbose.toLowerCase().equals("true")) {
            newStr = new String[str.length + 1];
            System.arraycopy(str, 0, newStr, 0, str.length);
            newStr[str.length] = "-verbose";
            return newStr;
        }
        
        return str;
    }
    
    /**
     * load the presentation module
     * @param map the configuration data required for loading this module.
     */
    public void load(Map map) {
        // check if verbose mode is used
        String verboseStr = System.getProperty("jump.presentation.verbose");
        if (verboseStr == null && map != null) {
            verboseStr = (String) map.get("jump.presentation.verbose");
        }
        if (verboseStr != null && verboseStr.toLowerCase().equals("true")) {
            verbose = true;
        }
        
        // convert the properties into application arguments for the tool
        installToolArgs = parseToolProperties(map);
    }
    
    /**
     * Stop this Presentation Mode from running.
     */
    public void stop() {
    }
    
    /**
     * Unload this module
     */
    public void unload() {
    }
    
    static void trace(String str) {
        if (verbose) {
            System.out.println(str);
        }
    }
    
    /**
     * Implementation of the interface's start() method.
     */
    public void start() {
        System.out.println("*** Starting InstallerTool ***");
        new JUMPInstallerTool(installToolArgs);
        // shut down the server
        new com.sun.jumpimpl.os.JUMPOSInterfaceImpl().shutdownServer();
        System.exit(0);
    }
}
