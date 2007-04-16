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

import com.sun.midp.i3test.TestCase;

import javax.microedition.pki.*;
import javax.microedition.securityservice.*;

import com.sun.cardreader.CardDeviceException;
import com.sun.cardreader.SlotFactory;

/**
 * This test case tests PKIManager class.
 */
public class TestPKI extends TestCase {

    /**
     * Tests CER generation.
     */
    private void testOne()
            throws java.io.IOException, CardDeviceException {
        // Parameters for certificate request message.
        String nameInfo = null;
        String algorithm = UserCredentialManager.ALGORITHM_RSA;
        int keyUsage = UserCredentialManager.KEY_USAGE_AUTHENTICATION;
        int keyLength = 512;
        String securityElementID = null;
        String securityElementPrompt = "Please insert PKI security element";
        boolean forceKeyGen = true;
        boolean stub_flag = false;
        
        try {
            SlotFactory.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }

        if (!stub_flag) {
            byte[] enrollmentRequest = null;
    
            // Obtain a certificate enrollment request message.
            try {
                enrollmentRequest = UserCredentialManager.generateCSR(nameInfo,
                                        algorithm,
                                        keyLength,
                                        keyUsage,
                                        securityElementID,
                                        securityElementPrompt,
                                        forceKeyGen);
                assertTrue(true);
            } catch (UserCredentialManagerException e1) {
                assertTrue(e1.toString() + " Reason: " + e1.getReason(), false);
            } catch (CMSMessageSignatureServiceException e2) {
                assertTrue(e2.toString() + " Reason: " + e2.getReason(), false);
            } catch (IllegalArgumentException e3) {
                assertTrue(e3.toString(), false);
            }
        } else {
            assertTrue(true);
        }
    }

    /**
     * Tests signing.
     */
    private void testTwo()
            throws java.io.IOException, CardDeviceException {
        // perform test case
        byte[] authSignature = null;
        String dataToSign = "JSR 177 Approved";
        String[] caNames = null;
        String securityElementPrompt = "Please insert PKI security element";
        int options = 0;
        boolean stub_flag = false;
        
        try {
            SlotFactory.init();
        } 
        catch (CardDeviceException e) {
            if (e.getMessage().equals("stub")) {
                stub_flag = true;
            } else {
                throw e;
            }
        }

        if (!stub_flag) {
            try {
                authSignature = CMSMessageSignatureService.sign
                                    (dataToSign,
                                     options,
                                     caNames,
                                 securityElementPrompt + " for sign testing");
            } catch (CMSMessageSignatureServiceException e1) {
                assertTrue(e1.toString(), false);
            } catch (UserCredentialManagerException e2) {
                assertTrue(e2.toString(), false);
            }
            if (authSignature == null) {
                assertTrue("sign returns null", false);
            }
            assertTrue(true);
        } else {
            assertTrue(true);
        }
    }
    
    /**
     * Run tests.
     */
    public void runTests() {
        try {
            declare("testOne");
            testOne();
            
            declare("testTwo");
            testTwo();
        }
        catch (Throwable t) {
            fail("" + t);
        }
    }

}
