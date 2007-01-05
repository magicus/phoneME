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

import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.module.installer.JUMPInstallerModule;
import java.net.URL;
import java.util.Map;

/**
 * <code>JUMPInstallerModule</code> provides the ability to install
 * content.
 * Installers have to implement this interface. They can optionally derive from
 * {@link com.sun.jump.module.contentstore.JUMPContentStore} in their
 * implementation for flexible storage options and abstractions.
 */
public class MIDLETInstallerImpl implements JUMPInstallerModule {
    
    public void unload() {
//        System.err.println("*** MIDPInstallerImpl unload() unimplemented ***");
    }
    
    public void load(Map map) {
//        System.err.println("*** MIDPInstallerImpl load() unimplemented ***");
    }
    
    /**
     * Install content specified by the given descriptor and location.
     * @return the installed content
     */
    public JUMPContent[] install(URL location, JUMPDownloadDescriptor desc){
//        System.err.println("*** MIDPInstallerImpl install() unimplemented ***");
        return null;
    };
    
    /**
     * Uninstall content
     */
    public void uninstall(JUMPContent content) {
//        System.err.println("*** MIDPInstallerImpl uninstall() unimplemented**");
    };
    
    /**
     * Update content from given location
     */
    public void update(JUMPContent content, URL location, JUMPDownloadDescriptor desc) {
//        System.err.println("*** MIDPInstallerImpl update() unimplemented**");
    };
    
    /**
     * Get all installed content
     */
    public JUMPContent[] getInstalled() {
//        System.err.println("*** MIDPInstallerImpl getInstalled() unimplemented**");
        return null;
    };
}