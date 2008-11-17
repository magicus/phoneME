/*
 *
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

package com.sun.cdc.config;

/**
 * This interface must be implemented by every class that performs dynamic
 * resolution of one or more system properties.
 * <code>System.getProperty()</code> and <code>System.getProperties()</code>
 * methods will use this interface every time to retrieve dynamic property
 * values.
 */
public interface PropertyProvider {
    /**
     * Returns the current value of the dynamic property corresponding to this
     * <code>PropertyProvider</code>.
     *
     * @param key key for the dynamic property.
     * @param fromCache indicates whether property value should be taken from
     *        internal cache. It can be ignored if properties caching is not
     *        supported by underlying implementation.
     * @return the value that will be returned by
     * <code>System.getProperty()</code> call for the corresponding dynamic
     * property.
     */
    public String getValue(String key, boolean fromCache);

    /**
     * Tells underlying implementation to cache values of all the properties
     * corresponding to this particular class. This call can be ignored if
     * property caching is not supported.
     *
     * @return <code>true</code> on success, <code>false</code> otherwise
     */
    public boolean cacheProperties();
}
