/*
 *   
 *
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

package com.sun.satsa.acl;

import com.sun.satsa.acl.AccessControlManager;
import com.sun.satsa.util.Utils;

import java.util.Vector;

/**
 * This class represents JCRMI permission.
 */
public class JCRMIPermission {

    /**
     * Hash modifier value.
     */
    private String hashModifier;
    /**
     * The list of class names.
     */
    private String[] classes;
    /**
     * The list of method identifiers.
     */
    private int[] methods;

    /**
     * Constructor.
     * @param hashModifier hash modifier string.
     * @param classList the list of classes.
     * @param methodList the list of method names and signatures.
     */
    public JCRMIPermission(String hashModifier, Vector classList,
                           Vector methodList) {

        this.hashModifier = hashModifier;

        classes = new String[classList.size()];
        classList.copyInto(classes);
        
        if (! methodList.isEmpty()) {
            methods = new int[methodList.size()];
            for (int i = 0; i < methodList.size(); i++) {
                if (methodList.elementAt(i) instanceof String) {
                    methods[i] = getMethodId(hashModifier,
                                             (String) methodList.elementAt(i));
                } else {
                    methods[i] = 
                        Utils.getInt((byte[])methodList.elementAt(i), 0);
                }
            }
        }
    }

    /**
     * Verify if this permission allows to invoke this method.
     * @param className the name of class.
     * @param method method name and signature.
     * @return true if this permission allows to invoke this method.
     */
    public boolean checkAccess(String className, String method) {

        boolean found = false;
        
        for (int i = 0; i < classes.length; i++) {

            if (classes[i].equals(className)) {
                found = true;
                break;
            }
        }

        if (! found) {
            return false;
        }

        if (methods == null) {
            return true;
        }

        int id = getMethodId(hashModifier, method);

        for (int i = 0; i < methods.length; i++) {
            if (id == methods[i])
                return true;
        }
        return false;
    }

    /**
     * Calculates method ID for given hash modifier and method name.
     * @param hashModifier hash modifier value.
     * @param method method name and signature.
     * @return the identifier.
     */
    private static int getMethodId(String hashModifier, String method) {

        if (hashModifier != null) {
            method = hashModifier + method;
        }
        byte data[] = Utils.stringToBytes(method);
        data = AccessControlManager.getHash(data, 0, data.length);
        return Utils.getInt(data, 0);
    }
}
