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

import java.util.Hashtable;
import java.util.Properties;
import java.util.HashSet;
import java.util.Enumeration;

/**
 * This class holds information necessary for dynamic properties resolution.
 */
public class DynamicProperties {
    /** Table that holds providers for dynamic system properties. */
    private static Hashtable propProviders = new Hashtable();

    /**
     * Checks whether there are any property providers set.
     *
     * @return <code>true</code> if at least one provider is defined,
     *         <code>false</code> otherwise.
     */
    public static boolean isEmpty() {
        return propProviders.isEmpty();
    }

    /**
     * Returns provider for the property corresponding to the key.
     *
     * @param key key for the dynamic property.
     * @return <code>PropertyProvider</code> instance that corresponds to
     *         the given property key.
     */
    public static PropertyProvider get(String key) {
        return (PropertyProvider)propProviders.get(key);
    }

    /**
     * Adds all dynamic properties in their current state to the given
     * <code>Properties</code> object.
     *
     * @param props set of properties that dynamic property values will be
     *              added to.
     */
    public static void addSnapshot(Properties props) {
        if (propProviders.isEmpty()) {
             return;
        }

        /*
         * Cache all dynamic properties that support caching.
         */
        Object[] providers = new HashSet(propProviders.values()).toArray();
        Hashtable provCached = new Hashtable();
        for (int i = 0; i < providers.length; i++) {
            PropertyProvider pp = (PropertyProvider)providers[i];
            provCached.put(pp, new Boolean(pp.cacheProperties()));
        }

        Enumeration provKeys = propProviders.keys();
        while (provKeys.hasMoreElements()) {
            String key = (String)provKeys.nextElement();
            PropertyProvider prov = (PropertyProvider)propProviders.get(key);
            String value = prov.getValue(key, ((Boolean)provCached.get(prov)).booleanValue());
            if (value != null) {
                props.put(key, value);
            }
        }
    
    }

    /**
     * Sets the specified <code>PropertyProvider</code> object to be called
     * for dynamic property resolution. This method should be used at the time
     * of properties initialization.
     *
     * @param key key for the dynamic property.
     * @param provider <code>PropertyProvider</code> instance that will be used
     *                 to resolve the dynamic property with the specified key.
     */
    public static void put(String key, PropertyProvider provider) {
        propProviders.put(key, provider);
    }
}
