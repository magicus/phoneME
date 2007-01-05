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

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPDownloadModuleFactory</code> is a factory for 
 * <code>JUMPDownloadModule</code>
 */
public abstract class JUMPDownloadModuleFactory extends JUMPModuleFactory {
    public static final String PROTOCOL_MIDP_OTA  = "ota/midp";
    public static final String PROTOCOL_OMA_OTA   = "ota/oma";
    
    private static JUMPDownloadModuleFactory INSTANCE = null;
    
    public static JUMPDownloadModuleFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPDownloadModuleFactory
     */
    protected JUMPDownloadModuleFactory() {
        synchronized (JUMPDownloadModuleFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns a <code>JUMPDownloadModule</code> for the protocol specified.
     * 
     * @param protocol the download protocol. The supported values are
     *        <ul>
     *          <li>{@link #PROTOCOL_MIDP_OTA}</li>
     *          <li>{@link #PROTOCOL_OMA_OTA}</li>
     *        </ul>
     * @throws java.lang.IllegalArgumentException if the protocol is not 
     *         not supported by the factory.
     */
    public abstract JUMPDownloadModule getModule(String protocol);
}
