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

package com.sun.midp.pki.ocsp;

import javax.microedition.pki.Certificate;
import java.util.Vector;

/**
 * Class representing OCSP Request.
 */
public class OCSPRequest {

    /**
     * Version of the OCSP request. Default is 0 (means v1).
     */
    private int version;

    /**
     * Name of the requestor. Optional.
     */
    private String requestorName;

    /**
     * List of requests to server.
     */
    private Vector requestList = new Vector();

    /**
     * List of extensions.
     */
    private Vector requestExtensions = new Vector();

    /**
     * Signature (optional).
     */
    private byte[] signature;

    /** */
    private class Request {
        /** Hash algorithm identifier */
        String hashAlgorithm;
        /** Hash of Issuer's DN */
        String issuerNameHash;
        /** Hash of Issuer's public key */
        String issuerKeyHash;
        /** Certificate serial number */
        String certSerialNumber;
        /** This request extensions, optional */
        private Vector extensions = new Vector();

        /**
         * Returns string respresentation of this request. Useful for debug.
         *
         * @return string respresentation of this request
         */
        public String toString() {
            String str = "(" +
                    "\n  hashAlgorithm = " + hashAlgorithm +
                    "\n  issuerNameHash = " + issuerNameHash +
                    "\n  certSerialNumber = " + certSerialNumber;

            for (int i = 0; i < extensions.size(); i++) {
                str += "\n Extension " + i + ": \n" +
                        extensions.elementAt(i).toString();
            }

            str += "\n)";

            return str;
        }
    }

    private class Extension {
        /** ID of this extension */
        String id;
        /** true is this extension is critical, false otherwise */
        boolean isCritical;
        /** Value of the extension (octet string) */
        String value;

        /**
         * Returns string respresentation of this extension. Useful for debug.
         *
         * @return string respresentation of this extension
         */
        public String toString() {
            return "[" +
                    "\n  id = " + id +
                    "\n  isCritical = " + isCritical +
                    "\n  value = " + value +
                    "\n]";
        }
    }

    /**
     *
     */
    public OCSPRequest(Certificate cert) {

    }

    /**
     *
     * @return this request encoded as an array of bytes
     */
    public byte[] getRequestAsByteArray() {
        byte[] reqBytes = new byte[1];
        
        return reqBytes;
    }

    /**
     * Returns string respresentation of this request. Useful for debug.
     *
     * @return string respresentation of this request
     */
    public String toString() {
        String str = "version = " + version +
                     "\nrequestorName = " + requestorName + "\n";

        for (int i = 0; i < requestList.size(); i++) {
            str += "\n Request " + i + ": \n" +
                    requestList.elementAt(i).toString();
        }

        for (int i = 0; i < requestExtensions.size(); i++) {
            str += "\n Extension " + i + ": \n" +
                    requestExtensions.elementAt(i).toString();
        }

        return str;
    }
}
