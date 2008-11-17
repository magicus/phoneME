/*
 * @(#)PI18n.java	1.8 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package text;

import java.util.*;

/**
 ** This class is to get the resource file with the current locale
 ** set by JDK. Use ResourceBundle objects to isolate locale-sensitive
 ** date, such as translatable text. The second argument passed to
 ** getBundle method identify which properties file we want to access.
 ** The first argument refers to the actual properties files.
 ** When the locale was created, the language code and country code
 ** were passed to its constructor. The property files are named 
 ** followed by the language and country code.   
 **/
public class PI18n {
    public ResourceBundle bundle = null;
    public String propertyName = null;
    
    public PI18n(String str) {
	propertyName = new String(str);
    }

    public String getString(String key){
       Locale currentLocale = java.util.Locale.getDefault();

       if (bundle == null) {
           try{
               bundle = ResourceBundle.getBundle(propertyName, currentLocale);
           } catch(java.util.MissingResourceException e){
               System.out.println("Could not load Resources");
               System.exit(0);
           }
        }
        String value = new String("");
        try{
            value = bundle.getString(key);
        } catch (java.util.MissingResourceException e){
            System.out.println("Could not find " + key);}
        return value;
    }
}
