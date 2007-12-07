/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.cdc.io.j2me.jcrmi;

import javax.microedition.jcrmi.RemoteStub;

import sun.misc.CVM;
import sun.misc.MIDletClassLoader;
import sun.misc.MIDPConfig;
import sun.misc.CDCAppClassLoader;

/**
 * Creation of the Remote stub object.
 */
class RemoteStubCreation {

    /**
     * Creates stub using application's class loader.
     * @return new stub object
     * @throws ClassNotFoundException if stub class wasn't found        
     * @throws IllegalAccessException if access to stub class is denied        
     * @throws InstantiationException if it was impossible to create stub object
     */
    static RemoteStub createStub(String className) throws ClassNotFoundException,
                     IllegalAccessException, InstantiationException {
        
        /* 
         * IMPL_NOTE Thread.currentThread().getContextClassLoader() method shall be
         * used in future
         */      
        ClassLoader classLoader;
            
        for (int i = 0; ; i++) {
            Class clazz = CVM.getCallerClass(i);
                
            if (clazz == null) {
                throw new IllegalStateException("Application's class loader was not found");
            }

            /* 
             * IMPL_NOTE: CDCAppClassLoader is planned to use for CDC application.
             * CDC application now is not supported
             */
            classLoader = clazz.getClassLoader();
            if ((classLoader instanceof CDCAppClassLoader) || (classLoader instanceof MIDletClassLoader)) {
                break;
            }
        }

        return (RemoteStub)
            (Class.forName(className, true, classLoader).newInstance());
    }
}
