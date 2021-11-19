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

/**
 * This class represents a set of JCRMI permissions.
 */
public class JCRMIPermissions extends ACLPermissions {

    /**
     * Constructs new object.
     * @param parent parent ACFile object.
     */
    public JCRMIPermissions(ACSlot parent) {
        super(parent);
    }

    /**
     * Verifies that the MIDlet have permission for this remote method.
     * @param className the name of class.
     * @param method method name and signature.
     * @throws SecurityException if access denied
     */
    public void checkPermission(String className, String method) {

        if (type == ALLOW) {
            return;
        }

        if (type == DISALLOW) {
            throw new SecurityException("Access denied");
        }

        for (int i = 0; i < permissions.size(); i++) {

            JCRMIPermission p = (JCRMIPermission) permissions.elementAt(i);
            if (p.checkAccess(className, method)) {
                return;
            }
        }
        throw new SecurityException("Access denied: " + className +
                " " + method);
    }

    /**
     * Initializes internal variables, verifies that operation is
     * supported and permitted, returns method name and signature for
     * given PIN operation.
     * @param pinID PIN identifier.
     * @param unblockID unblocking PIN identifier.
     * @param action PIN operation.
     * @param className the name of class.
     * @return method name and signature.
     * @throws java.lang.SecurityException if access denied
     */
    public String preparePIN(int pinID, int unblockID, int action,
                             String className) {

        checkPINOperation(pinID, unblockID, action);
        String result = (String) getPINCommand(pinID, action);
        if (result == null) {
            throw new SecurityException();
        }
        checkPermission(className, result);
        return result;
    }
}
