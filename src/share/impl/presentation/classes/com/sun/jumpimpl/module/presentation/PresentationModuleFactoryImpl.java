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

package com.sun.jumpimpl.module.presentation;

import com.sun.jump.module.presentation.JUMPPresentationModule;
import com.sun.jump.module.presentation.JUMPPresentationModuleFactory;

import java.util.Map;

public class PresentationModuleFactoryImpl extends JUMPPresentationModuleFactory {
    private JUMPPresentationModule simpleBasisAMS = null;
    private Map configMap;
    
    /**
     * load this module with the given properties
     * @param map properties of this module
     */
    public void load(Map map) {
        this.configMap = map;
    }
    /**
     * unload the module
     */
    public void unload() {
    }
    
    /**
     * Returns a <code>JUMPPresentationModule</code> for the specified
     *   presentation mode.
     * @param presentation the application model for which an appropriate
     *        installer module should be returned.
     * @return installer module object
     */
    public JUMPPresentationModule getModule(String presentation) {
        
        if (presentation.equals(JUMPPresentationModuleFactory.SIMPLE_BASIS_AMS)) {
            return getSimpleBasisAMS();
        } else {
            try {
                return (JUMPPresentationModule)Class.forName(presentation).newInstance();
            } catch(Exception e) {
                e.printStackTrace();
            }
            throw new IllegalArgumentException("Illegal JUMP Presentation Mode: " + presentation);
        }
    }
    
    private JUMPPresentationModule getSimpleBasisAMS() {
        synchronized(PresentationModuleFactoryImpl.class) {
            if (simpleBasisAMS == null) {
                simpleBasisAMS = (JUMPPresentationModule)new com.sun.jumpimpl.presentation.simplebasis.SimpleBasisAMS();
                simpleBasisAMS.load(configMap);
            }
            return simpleBasisAMS;
        }
    }
}

