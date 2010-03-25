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

package com.sun.jump.module.installer;

import com.sun.jump.module.JUMPModule;
import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.common.JUMPContent;
import java.net.URL;

/**
 * <code>JUMPInstallerModule</code> provides the ability to install 
 * content.
 * Installers have to implement this interface. They can optionally derive from
 * {@link com.sun.jump.module.contentstore.JUMPContentStore} in their
 * implementation for flexible storage options and abstractions.
 */
public interface JUMPInstallerModule extends JUMPModule {
    /**
     * Install content specified by the given descriptor and location.
     * @return an array of installed content
     */
    public JUMPContent[] install(URL location, JUMPDownloadDescriptor desc);

    /** 
     * Uninstall content 
     */
    public void uninstall(JUMPContent content);

    /** 
     * Update content from given location 
     */
    public void update(JUMPContent content, 
		       URL location,
		       JUMPDownloadDescriptor desc);

    /**
     * Get all installed content
     */
    public JUMPContent[] getInstalled();
}
