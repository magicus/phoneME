/*
 *  
 *
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

package com.sun.tck.wma;
import java.util.Properties;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Tool for load properties from environment or property object.
 */
public class PropLoader {

    /** 
     * Load string property value from environment or
     * from connections.prop.
     *
     * When environment setting is defined, variable receives the value from it.
     * Else it is initialized from connections.prop.
     *
     * @param valDefault  the default value is assigned when neither
     * environment variable nor property is defined
     * @param envVar  the name of environment variable
     * @param propsName  the properties file name
     * @param propVar the name of property variable
     *
     * @return string property value
     */
    protected String getProp(String valDefault, 
                            String envVar, 
                            String propsName, 
                            String propVar) {
        String retValue = System.getProperty(envVar);
        if (retValue == null) { // get from properties
            try {
                Properties props = new Properties();
                props.load(new FileInputStream(propsName));
                retValue = props.getProperty(propVar);
                if (retValue == null) { // get default value
                    retValue = valDefault;
                }
            } catch (IOException ioe) { // no properties
                retValue = valDefault;
            }
        }
        return retValue;
    }

    /** 
     * Load integer property value from environment or
     * from connections.prop.
     *
     * When environment setting is defined, variable receives the value from it.
     * Else it is initialized from connections.prop.
     *
     * @param valDefault  the default value is assigned when neither
     * environment variable nor property is defined
     * @param envVar  the name of environment variable
     * @param propsName  the properties file name
     * @param propVar the name of property variable
     *
     * @return string property value
     */
    protected int getIntProp(int valDefault, 
                            String envVar, 
                            String propsName, 
                            String propVar) {
        int retValue;
        String strVal = getProp("D", envVar, propsName, propVar);
        try {
            retValue = Integer.parseInt(strVal);
        } catch (NumberFormatException nfe) { // save default value
            retValue = valDefault;
        }
        return retValue;
    }
}
