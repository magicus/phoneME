/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.jump;

import java.net.MalformedURLException;
import java.net.URL;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPApplication;


/**
 * Representation of an MIDlet application.
 */
public class MIDletApplication extends JUMPApplication {

    public static final String SUITE_KEY = "MIDletApplication_suiteid";
    public static final String CLASSNAME_KEY = "MIDletApplication_classname";

    public static int getMIDletSuiteID(JUMPApplication app) {
        return Integer.parseInt(app.getProperty(SUITE_KEY));
    }

    public static String getMIDletClassName(JUMPApplication app) {
        return app.getProperty(CLASSNAME_KEY);
    }

    /**
     * Create an instance of an application.
     *
     * @param title The application's title, can be null
     * @param iconPath The location of the application's icon in, can be null
     * @param suiteID the MIDlet suite id which this midlet belongs to.
     * @param classname the MIDlet class name.
     */
    public MIDletApplication(String title, URL iconPath, int suiteID, 
			     String classname, int midletID) {
        
        super(title, iconPath, JUMPAppModel.MIDLET, 
	      computeApplicationID(suiteID, midletID));

        addProperty(SUITE_KEY, Integer.toString(suiteID)); 	 
        addProperty(CLASSNAME_KEY, classname); 	 
    }

    public int getMIDletSuiteID() {
        return Integer.parseInt(getProperty(SUITE_KEY));
    }

    public String getMIDletClassName() {
        return getProperty(CLASSNAME_KEY);
    }

    public String toString() {
        return (super.toString() + " MIDletSuiteID("
		 + getMIDletSuiteID() + ")");
    }

    private static int computeApplicationID(int suiteId, int midletNumber) {
        return (suiteId << 8 | (midletNumber & 0x00ff));
    }

    public static int convertToSuiteID(int applicationID) {
        return (applicationID >> 8);
    }
    
    public URL getIconPath() {
        String file = getProperty(ICONPATH_KEY);
        URL url = null;
        if (file == null) {
            return null;
        }        
        try {
            url = new URL("jar", "", file);            
        } catch (MalformedURLException ex) {
        }
        return url;
    }    
}
