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

package com.sun.jump.common;

//import java.util.ResourceBundle;
import java.util.HashMap;
import java.net.URL;
import com.sun.jump.common.JUMPContent;

/**
 * A representation of executable application content in the jump environment.
 * Because we can have different app types (e.g., XLET v. MIDLET),
 * use this class to hold relevant information needed to start
 * an application properly.
 */
public class JUMPApplication
    implements java.io.Serializable, JUMPContent
{
    /**
     * The type of this JUMPApplication - XLET, MIDLET or MAIN?
     */
    public JUMPAppModel appType;

    /*
    * App type in String, for serialization
    */
    //private String appTypeInString;

    protected String  initialClass = null; // class to execute
    protected URL     iconPath = null; // path to icon for display
    protected String  title = null; // application title
    protected URL     classpath = null; // specified classpath, if any
    protected HashMap props = null; // additional properties

    // ResourceBundle for localization
    // protected ResourceBundle rb;

    /**
     * Create an instance of an application.
     * @param clazz The class name of the application
     * @param classpath The path to the application
     * @param title The application's title, can be null
     * @param iconPath The location of the application's icon in, can be null 
     * @param type The application's type
     */
    public JUMPApplication( String clazz, URL classpath, 
                           String title, URL iconPath, 
                           JUMPAppModel type  )
    {
        this.initialClass = clazz;
        this.classpath = classpath;
        this.title = title;
        this.iconPath = iconPath;
        this.appType = type;
        //this.appTypeInString = type.toString();
    }

    /**
     * Determine the type of this application.
     * 
     * @return One of JUMPApplication's defined application types,
     *         as defined in JUMPAppModel.
     */
    public JUMPAppModel getAppType()
    {
        return appType;
    }

    /**
     * Validate the application and make sure that it is
     * internally consistent.
     * @throws SyntaxException In case of inconsistency.
     */
    //public void check() throws SyntaxException
    //{
    //    return;
    //}

    /**
     * Get the application's title.
     * @return The application's title.
     */
    public String getTitle()
    {
        return title;
    }

    /**
     * Set the application's title.
     *
     */
    public void setTitle( String title )
    {
        this.title = title;
        return;
    }

    /**
     * Get the path to the application's icon.
     * @return A URL defining the path to the icon in
     *         the downloaded content.
     */
    public URL getIconPath()
    {
        return iconPath;
    }

    /**
     * Set the path to the application's icon.
     */
    public void setIconPath( URL path )
    {
        this.iconPath = path;
        return;
    }

    /**
     * Get the application's main class.
     * @return A String indicating the application's
     *         initial class, or <code>null</code> if
     *         there is no initial class.
     */
    public String getInitialClass()
    {
        return initialClass;
    }

    /**
     * Get the applications's specified classpath
     * @return A String indicating the application's specified classpath,
     *         or <code>null</code> if one is not specified
     */
    public URL getClasspath()
    {
        return classpath;
    }

    /**
     * Add a key/value pair to the application's list
     * of properties.
     * @param key - A key value.
     * @param value - An object be associated with the key.
     * @throws NullPointerException - If either key or value is
     *                           <code>null</code>.
     */
    public void addProperty( String key, String value ) //throws SyntaxException
    {
        if ( key == null || value == null )
        {
            throw new NullPointerException("null key or value");
        }
        if ( props == null )
        {
            props = new HashMap();
        }
        props.put( key, value );

        return;
    }

    /**
     * Get a key/value pair to the application's list
     * of properties.
     * @param key - A key to search the properties for.
     * @throws NullPointerException - If key is
     *                           <code>null</code>.
     */
    public String getProperty( String key ) //throws SyntaxException
    {
        if ( props != null )
        {
            return (String) props.get(key);
        }
   
        return null;
    }

    /**
     * Returns the names of this JUMPApplication's property entries as an 
     * Iterator of String objects, or an empty Iterator if 
     * the JUMPApplication have no properties associated.
     */
    public java.util.Iterator getPropertyNames() {
        if ( props != null )
        {
            return props.keySet().iterator();
        }

        return null;
    }

    public String getContentType() {
	return "Application";
    }

    /*
     * Validate the application type.
     *  better to move to JUMPAppModel class?
     */
//    private void checkAppType( JUMPAppModel type )// throws SyntaxException
//    {
//        if ( type != JUMPAppModel.MAIN &&
//             type != JUMPAppModel.XLET &&
//             type != JUMPAppModel.MAIN )
//        {
//            throw new RuntimeException( rb.getString( "InvalidAppType" ) );
//        }
//        return;
//    }
}

