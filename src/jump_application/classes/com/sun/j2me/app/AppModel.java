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

package com.sun.j2me.app;

//import com.sun.jump.common.JUMPAppModel;

/**
 * Abstraction for application model
 */
public class AppModel{

    /** Guard from 'new' operator */
    private AppModel() {
    }

    public final static int MIDLET = 0;
    public final static int XLET = 1;
    public final static int UNKNOWN_MODEL = 2;

    private final static int appModel = retrieveAppModel();
    
    public static int getAppModel() {
        return appModel;
    }
    private static int retrieveAppModel() {
/* 
 * TODO: 
 * meantime implementation is only temporary:
 * check if it's a Midlet by reading the property "microediton.profiles"
 * in case of xlet it generates AccessPermissionException
 * 
 * must be changed later to the code below to get run-time information about the application model 
 * 

   
            JUMPIsolateProcess myProcess = JUMPIsolateProcess.getInstance();
            JUMPAppModel appModel = myProcess.getAppModel();
            if (appModel == JUMPAppModel.XLET)
            {
                return XLET;
            }
            else
                if (appModel == JUMPAppModel.MIDLET)
                {
                    return MIDLET;
                }
            return UNKNOWN_MODEL;
     */


        try
        {
            String outStr = System.getProperty("microedition.profiles");
            System.out.println(" DEBUG: microedition.profiles " + outStr);
            return MIDLET;
        }
        catch (Exception e) {
            return XLET;
        }
    }
}
