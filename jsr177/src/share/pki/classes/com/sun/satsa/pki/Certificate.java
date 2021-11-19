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

package com.sun.satsa.pki;

import java.util.Calendar;
import java.util.TimeZone;
import java.util.Date;
import com.sun.satsa.util.*;

import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;

/**
 * This class represents WIM certificate.
 */
class Certificate {

    /** Certificate label. */
    String label;

    /** Certificate ID. */
    byte[] id;

    /** Issuer certificate ID. */
    byte[] requestId;

    /** Parsed X.509 certificate or null if its not loaded. */
    TLV cert;

    /**
     * Location (path, offset, length) of certificate DF entry on
     * card.
     */
    Location header;

    /** Location (path, offset, length) of certificate on card. */
    Location body;

    /**
     * Verifies if this certificate is expired.
     * @return true if the certificate is expired.
     */
    boolean isExpired() {

        Calendar c = Calendar.getInstance(TimeZone.getTimeZone("GMT"));
        c.setTime(new Date());
        try {
            return c.before(getValidity(cert, true)) ||
                   c.after(getValidity(cert, false));
        } catch (TLVException e) {
            return true;
        }
    }

    /**
     * Verifies if this certificate was issued by subject of the
     * specified certificate. Returns false for self-signed certificates.
     * @param next certificate
     * @return true if the certificate was issued by subject of the
     * specified certificate.
     */
    boolean isIssuedBy(Certificate next) {
        TLV issuer = getIssuer();
        return (requestId != null) &&
               Utils.byteMatch(requestId, next.id) &&
               (! RFC2253Name.compare(issuer, getSubject())) &&
               RFC2253Name.compare(issuer, next.getSubject());
    }

    /**
     * Returns serial number of this certificate. The result can be used
     * only for comparison, not for the construction of new DER
     * structure.
     * @return TLV object containing serial number.
     */
    TLV getSerialNumber() {
        return cert.child.child.skipOptional(0xa0);
    }

    /**
     * Returns issuer name for this certificate which can be used
     * only for comparison.
     * @return TLV object containing issuer name.
     */
    TLV getIssuer() {
        return cert.child.child.skipOptional(0xa0).next.next;
    }

    /**
     * Returns subject name for this certificate which can be used
     * only for comparison.
     * @return TLV object containing subject name.
     */
    TLV getSubject() {
        return getIssuer().next.next;
    }

    /**
     * Returns new IssuerAndSerialNumber TLV object which can be used
     * as element of new data structure.
     * @return IssuerAndSerialNumber TLV object
     */
    TLV getIssuerAndSerialNumber() {

        TLV t = TLV.createSequence();
        t.setChild(getIssuer().copy()).
            setNext(getSerialNumber().copy());
        return t;
    }

    /**
     * Returns key algorithm identifier for this certificate which can
     * be used as element of new data structure.
     * @return key algorithm identifier
     */
    TLV getKeyAlgorithmID() {
        return getSubject().next.child.copy();
    }

    /**
     * Returns notBefore or notAfter date for the given certificate.
     * @param cert parsed x.509 certificate.
     * @param notBefore if true returns notBefore field value,
     * otherwise - notAfter.
     * @return the requested date
     * @throws TLVException in case of parsing error
     */
    static Calendar getValidity(TLV cert, boolean notBefore)
            throws TLVException {

        try {
            TLV t = cert.child.child.skipOptional(0xa0).next.next.next.child;
            if (! notBefore) {
                t = t.next;
            }
            return t.getTime();
        } catch (NullPointerException npe) {
            throw new TLVException("Invalid certificate");
        }
    }

    /**
     * Returns description of given certificate (subject, issuer, serial
     * number and validity).
     * @param cert parsed certificate
     * @return certificate description or null if any error occurs
     */
    static String getInfo(TLV cert) {

        try {
            TLV sn = cert.child.child.skipOptional(0xa0);
            TLV issuer = sn.next.next;
            TLV subject = issuer.next.next;

            String result;
            result = 
                // "Subject" 
                Resource.getString(
                    ResourceConstants.JSR177_CERTIFICATE_SUBJECT) + 
                ": " + RFC2253Name.NameToString(subject);
            result += 
                "\n\n" + 
                // "Issuer" 
                Resource.getString(
                    ResourceConstants.JSR177_CERTIFICATE_ISSUER) +
                ": " + RFC2253Name.NameToString(issuer);

            String s;
            try {
                s = "" + sn.getInteger();
            } catch (TLVException e) {
                s = "0x" + Utils.hexNumber(sn.data, sn.valueOffset, sn.length);
            }
            result += 
                "\n\n" + 
                // "Serial number"
                Resource.getString(ResourceConstants.JSR177_CERTIFICATE_SN) +
                ": " + s;

            return 
                result + "\n\n" + 
                // "Valid from"
                Resource.getString(
                    ResourceConstants.JSR177_CERTIFICATE_VALIDFROM) +
                " " +
                Utils.calendarToString(getValidity(cert, true)) +
                " " + 
                // "through"
                Resource.getString(
                    ResourceConstants.JSR177_CERTIFICATE_VALIDTILL) +
                " " + 
                Utils.calendarToString(getValidity(cert, false));
        } catch (TLVException tlve) { // ignored
        } catch (IllegalArgumentException iae) { // ignored
        } catch (NullPointerException npe) { // ignored
        }
        return null;
    }

    /*
     * IMPL_NOTE remove after debugging
    /**
     * Debug output.
     * /
    void print() {
        System.out.println("----------------------------");
        System.out.println("Certificate");
        System.out.println("label " + label);
        System.out.println("id " +
                Utils.hexEncode(id, 0, id.length, 9999));
        System.out.print("header ");
        header.print();
        System.out.print("body   ");
        body.print();
        System.out.println("");
    }
*/
}
