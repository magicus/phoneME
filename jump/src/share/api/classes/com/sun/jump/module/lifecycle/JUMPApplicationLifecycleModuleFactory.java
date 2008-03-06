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

package com.sun.jump.module.lifecycle;

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPApplicationLifecycleModuleFactory</code> is a factory for 
 * <code>JUMPApplicationLifecycleModule</code>
 */
public abstract class JUMPApplicationLifecycleModuleFactory extends JUMPModuleFactory {
    private static JUMPApplicationLifecycleModuleFactory INSTANCE = null;
    
    public static JUMPApplicationLifecycleModuleFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPApplicationLifecycleModuleFactory
     */
    protected JUMPApplicationLifecycleModuleFactory() {
        synchronized (JUMPApplicationLifecycleModuleFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns a <code>JUMPApplicationLifecycleModule</code> for the
     * lifecycle policy specified. This policy is one of the stated policies
     * below.
     * 
     * @param lifecyclePolicy the lifecycle policy for which an appropriate
     *        lifecycle module should be returned.
     */
    public abstract JUMPApplicationLifecycleModule 
	getModule(String lifecyclePolicy);
    

    /**
     * This policy descriptor allows only one live instance of an app
     * at any given time
     */
    public static final String POLICY_ONE_LIVE_INSTANCE_ONLY = 
	"lifecycle/one-live-instance-only";
}
