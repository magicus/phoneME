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

/**
 * @version @(#)CDCAmsOTAFactory.java	1.4 05/10/20
 */

package com.sun.appmanager.impl.ota;

import com.sun.appmanager.ota.OTA;
import com.sun.appmanager.ota.OTAFactory;

import java.lang.reflect.Constructor;

/**
 * A CDCAms-specific implementation of OTAFactory, which knows
 * how to instantiate CDCAms impl classes to provide different
 * OTA protocol support. 
 */
public class CDCAmsOTAFactory extends OTAFactory
{
    private static final String OTA_PKG =
        "com.sun.appmanager.impl.ota";

    public OTA getImpl( String impl )
    {
        if ( impl == null )
        {
            return null;
        }

        // Look for the correct class based on our implementation's
        // package and a classname incorporating the protocol
        // specified.
        String otaClassname =  OTA_PKG + "." + impl + "OTA";
        try
        {
            Class ota = Class.forName( otaClassname );
            Constructor def = ota.getConstructor( new Class[0] );
            return (OTA)def.newInstance( new Object[0] );
        }
        catch ( Exception e )
        {
            // For debug:
            //            e.printStackTrace();
        }

        // Unsuccessful so far; try instantiating the impl treating
        // it as a complete classname.
        try
        {
            Class ota = Class.forName( impl );
            Constructor def = ota.getConstructor( new Class[0] );
            return (OTA)def.newInstance( new Object[0] );
        }
        catch ( Exception e )
        {
            // For debug:
            //            e.printStackTrace();
        }

        // No OTA available.
        return null;
    }

}
