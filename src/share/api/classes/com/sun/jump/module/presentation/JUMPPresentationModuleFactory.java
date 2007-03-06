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

package com.sun.jump.module.presentation;

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPPresentationModuleFactory</code> is a factory for 
 * <code>JUMPPresentationModule</code>
 */
public abstract class JUMPPresentationModuleFactory extends JUMPModuleFactory {
    
    private static JUMPPresentationModuleFactory INSTANCE = null;
    
    public static JUMPPresentationModuleFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPPresentationModuleFactory
     */
    protected JUMPPresentationModuleFactory() {
        synchronized (JUMPPresentationModuleFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns a <code>JUMPPresentationModule</code> 
     * 
     * @param presentation A fully qualified presentation module class name.
     * @return  a newly created <code>JUMPPresentationModule</code>, or
     *  an already created <code>JUMPPresentationModule</code> 
     *  corresponding to the class name if it's found.  
     *  Null if class cannot be instanciated.
     */
    public abstract JUMPPresentationModule getModule(String presentation);

}
