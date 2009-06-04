/*
 *   
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.orientation;

import com.sun.midp.orientation.OrientationHandler;
import com.sun.midp.orientation.OrientationListener;

/**
 * Special class to generate orientation handlers.
 */
public class OrientationFactory {

    /**
     * Generates the new orientation handler.
     *
     * @param name the name of orientation service
     * @return the orientation handler or null when the name is wrong
     */
    public static OrientationHandler createOrientHandler(String name) {
        OrientationHandler retV = null;
        if (name != null) {
            try {
                retV = (OrientationHandler)(Class.forName(name).newInstance());
            } catch (ClassNotFoundException exc) {
            } catch (InstantiationException exc) {
            } catch (IllegalAccessException exc) {
            }
        }
        return retV;
    }
}
