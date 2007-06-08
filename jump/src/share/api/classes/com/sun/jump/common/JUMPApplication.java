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

import java.net.MalformedURLException;
import java.util.Properties;
import java.net.URL;
import com.sun.jump.common.JUMPContent;
import java.util.Enumeration;
import java.io.ByteArrayOutputStream;
import java.io.ByteArrayInputStream;

/**
 * A representation of executable application content in the jump environment.
 * Because we can have different app types (e.g., XLET v. MIDLET),
 * use this class to hold relevant information needed to start
 * an application properly.
 */
public class JUMPApplication
        implements java.io.Serializable, JUMPContent {
    
    protected Properties props = null; // additional properties
    
    public static final String ICONPATH_KEY = "JUMPApplication_iconPath";
    public static final String TITLE_KEY = "JUMPApplication_title";
    public static final String APPMODEL_KEY = "JUMPApplication_appModel";
    public static final String ID_KEY = "JUMPApplication_id";
    /**
     * A hint to the WindowingModule regarding the area of the screen 
     * this JUMPApplication requires. 
     * The value should be in the syntax of "x,y-wxh", for example "0,50-640x430".
     **/
    public static final String ID_SCREEN_BOUNDS = "JUMPApplication_screenBounds";
    
    /**
     * Create an instance of an application.
     * @param title The application's title, can be null
     * @param iconPath The location of the application's icon in, can be null
     * @param type The application's type
     * @param id The installation id of the application
     */
    public JUMPApplication(String title, URL iconPath, JUMPAppModel type, int id) {
        
        if (title != null) {
            addProperty(TITLE_KEY, title);
        }
        
        if (iconPath != null) {
            addProperty(ICONPATH_KEY, iconPath.getFile());
        }
        
        if (type != null) {
            addProperty(APPMODEL_KEY, type.getName());
        }
        
        addProperty(ID_KEY, Integer.toString(id));        

    }
    
    /**
     * Create an instance of an application from a Properties object
     * @param props The properties that correspond to this application
     */
    private JUMPApplication(Properties props) {
        this.props = props;

	if (props.getProperty(ID_KEY) == null ||
	    props.getProperty(APPMODEL_KEY) == null)
		throw new IllegalArgumentException("Properties do not include " + 
				APPMODEL_KEY + " or " + ID_KEY);
    }
    
    /**
     * Create a binary representation of this JUMPApplication object
     */
    public byte[] toByteArray() {
        try {
           ByteArrayOutputStream baos = new ByteArrayOutputStream();
           props.store(baos, "");
           return baos.toByteArray();
        } catch (java.io.IOException e) {
           System.err.println("Error serializing this JUMPApplication object");
           e.printStackTrace();
	   return null;
        }
    }
    
    /**
     * Create a JUMPApplication from its binary representation
     */
    public static JUMPApplication fromByteArray(byte[] propBytes) {
         try {
            Properties p = new Properties();
            ByteArrayInputStream bais = new ByteArrayInputStream(propBytes);
            p.load(bais);
            return new JUMPApplication(p);
         } catch (java.io.IOException e) {
           System.err.println("Error deserializing an JUMPApplication object");
           e.printStackTrace();
	   return null;
        }
    }
    
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append("[app type=\""+getAppType()+"\",");
        sb.append(" iconPath=\""+getIconPath()+"\",");
        sb.append(" title=\""+getTitle()+"\",");
        sb.append(" props=["+getPropsAsString()+"] ]");
        return sb.toString();
    }
    
    private String getPropsAsString() {
        if (props == null) {
            return "null";
        }
        
        StringBuffer sb = new StringBuffer();
        for (Enumeration e = getPropertyNames(); e.hasMoreElements(); ) {
            String name = (String)e.nextElement();
            String value = getProperty(name);
            sb.append(name+"="+value+", ");
        }
        return sb.toString();
    }
    
    /**
     * Determine the type of this application.
     *
     * @return One of JUMPApplication's defined application types,
     *         as defined in JUMPAppModel.
     */
    public JUMPAppModel getAppType() {
        String type = getProperty(APPMODEL_KEY);
        if (type.equals(JUMPAppModel.XLET.getName())) {
            return JUMPAppModel.XLET;
        } else if (type.equals(JUMPAppModel.MAIN.getName())) {
            return JUMPAppModel.MAIN;
        } else if (type.equals(JUMPAppModel.MIDLET.getName())) {
            return JUMPAppModel.MIDLET;
        }
        return null;
    }
    
    /**
     * Get the application's title.
     * @return The application's title.
     */
    public String getTitle() {
        return getProperty(TITLE_KEY);
    }
    
    /**
     * Set the application's title.
     *
     */
    public void setTitle( String title ) {
        addProperty(TITLE_KEY, title);
        return;
    }
    
    /**
     * Obtain the installed application id for this object.
     * @return The installed application id.
     */
    public int getId() {
        return Integer.parseInt(getProperty(ID_KEY));
    }
    
    /**
     * Set the installed application id for this object.
     * @param id The installed application id.
     */
    public void setId(String id) {
        addProperty(ID_KEY, id);
    }
    
    /**
     * Get the path to the application's icon.
     * @return A URL defining the path to the icon in
     *         the downloaded content.
     */
    public URL getIconPath() {
        String file = getProperty(ICONPATH_KEY);
        URL url = null;
        if (file == null) return null;
        
        try {
            url = new URL("file", null, file);
        } catch (MalformedURLException ex) {
            ex.printStackTrace();
        }
        return url;
    }
    
    /**
     * Set the path to the application's icon.
     */
    public void setIconPath( URL path ) {
        addProperty(ICONPATH_KEY, path.getFile());
        return;
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
        if ( key == null || value == null ) {
            throw new NullPointerException("null key or value");
        }
        if ( props == null ) {
            props = new Properties();
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
        if ( props != null ) {
            return (String) props.get(key);
        }
        
        return null;
    }
    
    /**
     * Returns the names of this JUMPApplication's property entries as an
     * Enumeration of String objects, or an empty Enumeration if
     * the JUMPApplication have no properties associated.
     */
    public Enumeration getPropertyNames() {
        if ( props != null ) {
            return props.keys();
        }
        
        return null;
    }
    
    public String getContentType() {
        return "Application";
    }

    /**
     * Returns true if two JUMPApplications' APPMODEL_KEY and
     * ID_KEY properties hold the same value.
     */
    public boolean equals(Object obj) {
        if (!(obj instanceof JUMPApplication)) return false;

	JUMPApplication other = (JUMPApplication) obj;
	return (getProperty(APPMODEL_KEY).equals(other.getProperty(APPMODEL_KEY))
               && getProperty(ID_KEY).equals(other.getProperty(ID_KEY)));	
    }

    /**
     * Returns the value of ID_KEY property.
     */
    public int hashCode() {
        return Integer.parseInt(getProperty(ID_KEY));
    }
}

