/*
 *   
 *
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

package com.sun.jsr135;

import com.sun.cdc.config.PropertyProvider;

/**
 * This class provides values for the following dynamic properties:
 * <ul>
 *   <li>supports.mixing</li>
 * </ul>
 */
public class DynamicProperties implements PropertyProvider {

    /** The only instance of this class. */
    private static DynamicProperties instance = null;

    /**
     * Does not let anyone instantiate this class.
     */
    private DynamicProperties() { }

    /**
     * Returns one and only instance of this class.
     * This method does not need to be synchronized because it will be called
     * only sequentially during isolate initialization.
     *
     * @return <code>DynamicProperties</code> instance
     */
    public static DynamicProperties getInstance() {
        if (instance == null) {
            instance = new DynamicProperties();
        }
        return instance;
    }

    /**
     * Returns current value for the dynamic property corresponding to the
     * given key. This method is called upon retrieval of any of the properties
     * supported by this class.
     *
     * @param key key for the property being retrieved.
     * @param fromCache indicates whether property value should be taken from
     *        internal cache. It can be ignored if properties caching is not
     *        supported by underlying implementation.
     * @return current property value
     */
    public String getValue(String key, boolean fromCache) {
        return nGetPropertyValue(key, fromCache);
    }

    /**
     * Returns current value for the dynamic property corresponding to the
     * given key.
     *
     * @param key key for the property being retrieved.
     * @param fromCache indicates whether property value should be taken from
     *        internal cache. It can be ignored if properties caching is not
     *        supported by underlying implementation.
     * @return current property value
     */
    private static native String nGetPropertyValue(String key, boolean fromCache);

    /**
     * Tells underlying implementation to cache values of all the properties
     * corresponding to this particular class. This call can be ignored if
     * property caching is not supported.
     *
     * @return <code>true</code> on success, <code>false</code> otherwise
     */
    public native boolean cacheProperties();

}
