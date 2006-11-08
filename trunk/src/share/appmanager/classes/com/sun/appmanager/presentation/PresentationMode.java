/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.presentation;

import com.sun.appmanager.AppManager;
import com.sun.appmanager.apprepository.AppModule;

public abstract class PresentationMode
    implements Presentation {

    /**
     * Return the presentation mode's implementation class name of the
     * Presentation interface.  The returned string will contain of
     * the following format represent by this example of a presentation
     * mode called "AwtPDA":
     *
     * com.sun.appmanager.impl.presentation.AwtPDAPresentationMode
     *
     * @return String
     */
    public static String getPresentationClassName() {
        return System.getProperty("cdcams.presentation");
    }

    public static PresentationMode createPresentation() {

        PresentationMode m = null;
        String className = getPresentationClassName();
        try {
            Class c = Class.forName(className);
            m = (PresentationMode) c.newInstance();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return m;
    }

    /**
     * Add an application or menu represented by an AppModule to this
     * presentation mode.  It is highly recommended that this method
     * be used within the implemented loadApps() method of the Presentation
     * interface.
     * @param module the module to be added
     */
    public void addApp(AppModule module) { 
        System.out.println(AppManager.getResourceBundle().getString("PMAddAppNotDef"));
   }

    /**
     * Launch the application upon startup of the application manager.
     * It is highly recommended that this method be used within the
     * implemented runStartupApps() method of the Presentation interface.
     * @param module AppModule
     */
    public void runStartupApp(AppModule module) {
        System.out.println(AppManager.getResourceBundle().getString("PMRSAppNotDef"));
    }

}
