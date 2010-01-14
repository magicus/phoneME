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

package com.sun.jump.module.download;

import com.sun.jump.module.JUMPModule;

/**
 * <code>JUMPDownloadModule</code> provides the ability to download 
 * content.
 */
public interface JUMPDownloadModule extends JUMPModule {
    /**
     * Creates the descriptor pointed by the uri.
     * 
     * @throws JJUMPDownloadExceptionif there are problems creating the
     *         descriptor from the uri specified
     */
    public JUMPDownloadDescriptor createDescriptor(String uri)
        throws JUMPDownloadException;
    
    /**
     * Create a downloader using the information from the descriptor
     * passed.
     * 
     * @throws JJUMPDownloadExceptionif the descriptor does not contain all
     *         the information needed to create the action.
     */
    public JUMPDownloader createDownloader(
        JUMPDownloadDescriptor descriptor)
        throws JUMPDownloadException;
}
