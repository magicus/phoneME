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

package com.sun.appmanager.ota;

import com.sun.appmanager.AppManager;
import java.util.Hashtable;
import java.util.ResourceBundle;

/**
 * A representation of downloaded executable application content.
 * Because we can have different app types (e.g., XLET v. MIDLET),
 * use this class to hold relevant information needed to start
 * an application properly.
 */
abstract public class Application
{
    public static int MAIN_APP = 1;
    public static int XLET_APP = 2;
    public static int MIDLET_APP = 3;

    private String mainClass = null; // class to execute
    private String iconPath = null; // path to icon for display
    private String name = null; // application name
    private String classpath = null; // specified classpath, if any
    protected int appType = 0;    // application type (e.g., XLET)
    private Hashtable props = null; // additional properties

    // ResourceBundle for localization
    private ResourceBundle rb = AppManager.getResourceBundle();

    /**
     * Create an instance of an application.
     * @param name The application's name
     * @param iconPath The location of the application's icon
     * @param clazz The main class of the application.
     */
    public Application( String name, String iconPath, String clazz,
                        String classpath )
    {
        this.name = name;
        this.iconPath = iconPath;
        this.mainClass = clazz;
        this.classpath = classpath;
    }

    /**
     * Determine the type of this application.
     * @return One of Application's defined application types,
     *         <code>MAIN_APP</code>, <code>XLET_APP</code>, or
     *         <code>MIDLET_APP</code>.
     */
    public int getAppType()
    {
        return appType;
    }

    /**
     * Validate the application and make sure that it is
     * internally consistent.
     * @throws SyntaxException In case of inconsistency.
     */
    public void check() throws SyntaxException
    {
        return;
    }

    /**
     * Get the application's name.
     * @return The application's name.
     */
    public String getName()
    {
        return name;
    }

    /**
     * Set the application's name.
     *
     */
    public void setName( String name )
    {
        this.name = name;
        return;
    }

    /**
     * Get the path to the application's icon.
     * @return A String defining the path to the icon in
     *         the downloaded content.
     */
    public String getIconPath()
    {
        return iconPath;
    }

    /**
     * Set the path to the application's icon.
     */
    public void setIconPath( String path )
    {
        this.iconPath = path;
        return;
    }

    /**
     * Get the application's main class.
     * @return A String indicating the application's
     *         main class, or <code>null</code> if
     *         there is no main class.
     */
    public String getMainClass()
    {
        return mainClass;
    }

    /**
     * Get the applications's specified classpath
     * @return A String indicating the application's specified classpath,
     *         or <code>null</code> if one is not specified
     */
    public String getClasspath()
    {
        return classpath;
    }

    /**
     * Add a key/value pair to the application's list
     * of properties.
     * @param name - A key value.
     * @param value - To be associated with the key.
     * @throws SyntaxException - If either name or value is
     *                           <code>null</code>.
     */
    public void addProperty( String name, String value ) throws SyntaxException
    {
        if ( name == null || value == null )
        {
            throw new SyntaxException( rb.getString( "BadProperty" ) );
        }
        if ( props == null )
        {
            props = new Hashtable();
        }
        props.put( name, value );
        return;
    }

    /*
     * Validate the application type.
     */
    private void checkAppType( int type ) throws SyntaxException
    {
        if ( type != MAIN_APP &&
             type != XLET_APP &&
             type != MIDLET_APP )
        {
            throw new SyntaxException( rb.getString( "InvalidAppType" ) );
        }
        return;
    }
}

