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
 */

package com.sun.satsa.pki;

import com.sun.j2me.dialog.Dialog;
import com.sun.j2me.dialog.MessageDialog;
import com.sun.j2me.dialog.PinMessageDialog;
import com.sun.cdc.io.j2me.apdu.APDUManager;
import com.sun.cdc.io.j2me.apdu.Handle;
import com.sun.satsa.acl.ACLPermissions;
import com.sun.satsa.acl.PINAttributes;
import com.sun.satsa.acl.PINEntryDialog;
import com.sun.satsa.util.*;

import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;

import javax.microedition.pki.UserCredentialManager;
import javax.microedition.pki.UserCredentialManagerException;
import javax.microedition.securityservice.CMSMessageSignatureService;
import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import java.io.IOException;
import java.util.Vector;
import java.util.Calendar;
import java.util.TimeZone;
import java.util.Date;

import com.sun.j2me.security.Token;

/**
 * This class provides interface to WIM card application.
 */
class WIMApplication {

    /** Operation result constant (skip this SE and try next). */
    static final int SKIP    = 0;
    /** Operation result constant. */
    static final int SUCCESS = 1;
    /** Operation result constant. */
    static final int CANCEL  = 2;
    /** Operation result constant. */
    static final int ERROR   = 3;

    /** INS byte for APDU command. */
    private static final byte INS_VERIFY    = (byte) 0x20;
    /** INS byte for APDU command. */
    private static final byte INS_MSE       = (byte) 0x22;
    /** INS byte for APDU command. */
    private static final byte INS_PSO       = (byte) 0x2a;
    /** INS byte for command APDU. */
    static final byte INS_NEW               = (byte) 0xBC;

    /** PIN status constant. */
    private static final int PIN_BLOCKED    = 0;
    /** PIN status constant. */
    private static final int PIN_DISABLED   = 1;
    /** PIN status constant. */
    private static final int PIN_REQUIRED   = 2;
    /** PIN status constant. */
    private static final int PIN_CANCELLED  = 3;

    /** ODF path. */
    private static final short ODFPath        = 0x5031;
    /** EF(TokenInfo) path. */
    private static final short TokenInfoPath  = 0x5032;
    /** EF(UnusedSpace) path. */
    private static final short UnusedSpace    = 0x5033;

    /** ODF entry tag. */
    private static final int PRIVATE_KEYS_TAG           = 0xa0;
    /** ODF entry tag. */
    private static final int PUBLIC_KEYS1_TAG           = 0xa1;
    /** ODF entry tag. */
    private static final int PUBLIC_KEYS2_TAG           = 0xa2;
    /** ODF entry tag. */
    private static final int USEFUL_CERTIFICATES_TAG    = 0xa4;
    /** ODF entry tag. */
    private static final int TRUSTED_CERTIFICATES_TAG   = 0xa5;
    /** ODF entry tag. */
    private static final int USER_CERTIFICATES_TAG      = 0xa6;
    /** ODF entry tag. */
    private static final int PINS_TAG                   = 0xa8;

    /** APDUs that must be used for WIM application selection. */
    private static final byte[][] selectAPDUs =
            {{0, (byte) 0xa4, 4, Constants.P2, 12, (byte) 0xA0, 0, 0, 0, 0x63,
              0x50, 0x4B, 0x43, 0x53, 0x2D, 0x31, 0x35, 0x7f},
             {0, (byte) 0xa4, 4, Constants.P2, 12, (byte) 0xA0, 0, 0, 0, 0x63,
              0x57, 0x41, 0x50, 0x2D, 0x57, 0x49, 0x4D, 0x7f}};


    /** Binary representation of WIM_GENERIC_RSA SE OID, 2.23.43.1.1.2 */
    private static final byte[] WIM_GENERIC_RSA_OID =
                                         {0x67, 0x2B, 0x01, 0x01, 0x02};

    /** Binary representation of WAP_WSG_WIMPATH OID, 2.23.43.1.3 */
    private static final byte[] WAP_WSG_WIMPATH =
                                               {0x67, 0x2B, 0x01, 0x03};

    /** RSA digest header. */
    private static final byte[] DigestInfoHeader = {
        0x30, 0x21,         // DigestInfo ::= SEQUENCE {
        0x30, 0x09,         // AlgorithmIdentifier ::= SEQUENCE {
        0x06, 0x05,         // id-SHA1 OBJECT IDENTIFIER ::=
        0x2b, 0x0e, 0x03, 0x02, 0x1a,   // { 1 3 14 3 2 26 } ,
        0x05, 0x00,         // parameters NULL } ,
        0x04, 0x14};         // digest OCTET STRING


    /** Connection object. */
    private Connection apdu;

    /** File system object. */
    private WimFileSystem files;

    /** Identifier of WIM_GENERIC_RSA SE */
    private int WIM_GENERIC_RSA_ID;

    /** Identifier of RSA algorithm in EF(TokenInfo) and PrKDFs. */
    private int RSA_ALGORITHM_ID;

    /** If true this WIM application doesn't allow to modify data. */
    private boolean readOnly;

    /** Serial number of this WIM card. */
    private String serialNumber;

    /** This vector contains parsed objects from DF(ODF). */
    private Vector ODF;

    /** Private keys. */
    private PrivateKey[] PrKeys;
    /** Public keys. */
    private PublicKey[] PuKeys;
    /** PINs. */
    private PINAttributes[] PINs;
    /** Certificates. */
    private Certificate[] Certificates;
    /** Vector contains identifiers of existing certificates. */
    private Vector certificateIDs;
    /** This vector contains objects loaded from directory file. */
    private Vector loaderObjects;
    /** This vector contains locations of loaded objects. */
    private Vector loaderLocations;
    /**
     * This vector contains locations of free space in directory
     * files. */
    private Vector loaderFreeBlocks;
    /** Full path to EF(UnusedSpace) file. */
    private short[] UnusedSpacePath;


    /**
     * Creates connection with WIM application (WIM spec, 11.3.3) on
     * card in specified slot. Doesn't throw exceptions.
     * @param slotNum the slot number
     * @param securityElementID identifies the security element
     * @param readOnly if true WIM data can be protected
     * @return WIMApplication object or null.
     */
    public static WIMApplication getInstance(int slotNum,
                        String securityElementID, boolean readOnly) {

        for (int i = 0; i < selectAPDUs.length; i++) {

            Handle h;
            APDUManager.initACL(slotNum);
            try {
                h = APDUManager.selectApplication(
                        selectAPDUs[i], (byte) slotNum);
            } catch (IOException e) {
                continue;
            }

            WIMApplication w = new WIMApplication(h);
            if (w.init(securityElementID) &&
                (readOnly || (! w.readOnly))) {
                return w;
            }
            w.done();
        }
        return null;
    }

    /**
     * Constructs a new WIMApplication object.
     * @param h the APDU connection handle
     */
    private WIMApplication(Handle h) {
        this.apdu = new Connection(h);
        files = new WimFileSystem(apdu);
    }

    /**
     * Reads configuration (EF(TokenInfo) file).
     * Doesn't throw exceptions.
     * @param securityElementID identifies the security element
     * @return true if successful
     */
    private boolean init(String securityElementID) {

        try {
            if (! readTokenInfo(securityElementID)) {
                return false;
            }

            readODF();
            loadPINs();
            UnusedSpacePath = files.makePath(TLV.createOctetString(
                                      Utils.shortToBytes(UnusedSpace)));
            return true;
        } catch (IOException te) {
            done();
        }

        return false;
    }

    /**
     * Safely closes the connection.
     */
    public void done() {
        if (apdu != null) {
            apdu.done();
        }
        apdu = null;
    }

    /**
     * Reads and parses EF(TokenInfo).
     * @param securityElementID identifies the security element
     * specified by user
     * @return true if successful and WIM application is found
     * @throws IOException if the file is not found or there is some
     * other IO error
     * @throws RuntimeException if there is an error parsing the file
     * (e.g. its non-WIM PKCS#15 application and some mandatory for WIM
     * fields are absent)
     */
    private boolean readTokenInfo(String securityElementID)
            throws IOException, TLVException {

        files.select(TokenInfoPath);
        TLV t = new TLV(files.readFile(), 0);

        /*
         *  PKCS15TokenInfo ::= SEQUENCE {
         *      version        INTEGER {v1(0)} (v1,...),
         *      serialNumber   OCTET STRING,
         *      -manufacturerID PKCS15Label OPTIONAL,
         *      +label          [0] PKCS15Label OPTIONAL,
         *      +tokenflags     PKCS15TokenFlags,
         *      +seInfo         SEQUENCE OF PKCS15SecurityEnvironmentInfo
         *                      OPTIONAL,
         *      -recordInfo     [1] PKCS15RecordInfo OPTIONAL,
         *      +supportedAlgorithms [2]SEQUENCE OF PKCS15AlgorithmInfo
         *                      OPTIONAL,
         *      ... -- For future extensions
         *  }
         */

        t = t.child;    // version

        // it must be integer and it must be 0 (version 1)
        if (t.getInteger() != 0) {
            return false;
        }

        t = t.next;     // serial number
        if (t.type != TLV.OCTETSTR_TYPE) {
            return false;
        }
        serialNumber = Utils.hexNumber(t.data, t.valueOffset, t.length);

        // skip optional manufacturerID
        t = t.next.skipOptional(TLV.UTF8STR_TYPE);

        // it must be label
        if (t.type != 0x80) {
            return false;
        }

        String label = t.getUTF8();

        if (! (label.startsWith("WIM 1.01") &&
              (label.length() == 8 || label.charAt(8) == ' '))) {
            return false;
        }

        if (securityElementID != null &&
            label.indexOf(securityElementID) == -1) {
            return false;
        }

        t = t.next;     // Token flags. Check if this card is read-only.
        readOnly = t.checkFlag(0);

        t = t.next;     // seInfo
        // check if WIM_GENERIC_RSA SE is supported
        WIM_GENERIC_RSA_ID = -1;
        TLV v = t.child;
        while (v != null) {
            if (v.child.next.valueEquals(WIM_GENERIC_RSA_OID)) {
                WIM_GENERIC_RSA_ID = v.child.getInteger();
                break;
            }
            v = v.next;
        }

        if (WIM_GENERIC_RSA_ID == -1) {
            return false;
        }

        // skip otional recordInfo
        t = t.next.skipOptional(0xa1).child;

        // check if RSA signature is supported
        boolean supportsSignature = false;
        while (t != null) {
            TLV m = t.child;
            if (m.next.getInteger() == 0) {     // 0 - RSA
                RSA_ALGORITHM_ID = m.getInteger();
                supportsSignature = m.next.next.next.checkFlag(1);
                break;
            }
            t = t.next;
        }

        return supportsSignature;
    }

    /**
     * Reads ODF and sets WIM root directory if necessary.
     * @throws IOException if I/O error occurs
     */
    private void readODF() throws IOException, TLVException {

        ODF = new Vector();
        resetLoader(ODF, null, null);
        parseDF(new short[] {ODFPath});

        for (int i = 0; i < ODF.size(); i++) {

            TLV t = (TLV) ODF.elementAt(i);

            if (t.type != 0xa7) {
                continue;
            }

            t = t.child;
            if (t.type != 0xa0) {   // not [0]SEQUENCE OF ObjectType
                continue;
            }

            t = t.child.child;      // 1st of SEQUENCE OF PKCS15Data

            while (t != null) {

                if (t.type != TLV.SEQUENCE_TYPE) {      // not opaqueDO
                    t = t.next;
                    continue;
                }

                TLV m = t.child.next.child.skipOptional(TLV.UTF8STR_TYPE);
                if (m.type != TLV.OID_TYPE ||
                    ! m.valueEquals(WAP_WSG_WIMPATH)) {
                    t = t.next;
                    continue;
                }

                m = t.child.next.next.child;

                short[] root = new short[m.length / 2];

                for (int j = 0; j < root.length; j++) {
                    root[j] = Utils.getShort(m.data,
                                             m.valueOffset + j * 2);
                }
                files.setRoot(root);
                break;
            }
        }
    }

    /**
     * Initialises object loader.
     * @param objects vector for loaded objects or null
     * @param locations vector for object locations or null
     * @param freeBlocks vector for unused block locations or null
     */
    private void resetLoader(Vector objects,
                             Vector locations,
                             Vector freeBlocks) {
        loaderObjects = objects;
        loaderLocations = locations;
        loaderFreeBlocks = freeBlocks;
    }

    /**
     * Finds all the files for specified type, reads and parses them.
     * @param tag tag of ODF entry for this type of objects
     * @throws IOException if IO error occurs
     */
    private void loadObjects(int tag) throws IOException, TLVException {

        for (int i = 0; i < ODF.size(); i++) {
            TLV t = (TLV) ODF.elementAt(i);
            if (t.type == tag) {
                parseDF(files.makePath(t.child.child));
            }
        }
    }

    /**
     * Parses directory file. Places results into vectors specified
     * <code>resetLoader</code> method.
     * @param path path to directory file
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void parseDF(short[] path) throws TLVException, IOException {
        files.select(path);
        doParseDF(files.readFile(), path,
                      loaderObjects, loaderLocations, loaderFreeBlocks);
    }

    /**
     * Loads all RSA private keys. Keys are stored in
     * <code>PrKeys</code> array.
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void loadPrivateKeys() throws IOException, TLVException {

        Vector v = new Vector();
        resetLoader(v, null, null);
        loadObjects(PRIVATE_KEYS_TAG);

        Vector k = new Vector();

        for (int i = 0; i < v.size(); i++) {

            TLV t = (TLV) v.elementAt(i);

            if (t.type != TLV.SEQUENCE_TYPE) {      // non-RSA key
                continue;
            }

            PrivateKey key = new PrivateKey();

            t = t.child;        // commonObjectAttributes

            key.label = t.child.getUTF8().trim();
            key.authId = t.child.next.next.getId();

            t = t.next;

            TLV m = t.child;
            key.id = m.getValue();

            m = m.next;
            key.authentication = m.checkFlag(2);
            key.nonRepudiation = m.checkFlag(9);

            m = m.next.skipOptional(TLV.BOOLEAN_TYPE);
            key.keyReference = m.getInteger() & 0xff;

            // skip PKCS15CommonPrivateKeyAttributes
            t = t.next.skipOptional(0xa0);

            t = t.child.child;
            key.path = files.makePath(t.child);
            t = t.next;
            key.modulusLength = t.getInteger();

            t = t.next;
            if (t != null &&
                t.type == TLV.INTEGER_TYPE &&
                t.getInteger() != RSA_ALGORITHM_ID) {
                continue;
            }

            k.addElement(key);
        }

        PrKeys = new PrivateKey[k.size()];
        k.copyInto(PrKeys);
    }

    /**
     * Loads all RSA public keys. Stores keys in <code>PuKeys</code>
     * array.
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void loadPublicKeys() throws IOException, TLVException {

        Vector v = new Vector();
        resetLoader(v, null, null);
        loadObjects(PUBLIC_KEYS1_TAG);
        loadObjects(PUBLIC_KEYS2_TAG);

        Vector k = new Vector();

        for (int i = 0; i < v.size(); i++) {

            TLV t = (TLV) v.elementAt(i);

            if (t.type != TLV.SEQUENCE_TYPE) {      // non-RSA key
                continue;
            }

            PublicKey key = new PublicKey();

            t = t.child;        // commonObjectAttributes
            t = t.next;         // CommonKeyAttributes

            key.id = t.child.getValue();
            TLV m = t.child.next.next;

            if (m.type != TLV.BOOLEAN_TYPE ||
                m.data[m.valueOffset] != 0) {
                continue;       // native, useless for CSR generation
            }

            // skip PKCS15CommonPublicKeyAttributes
            t = t.next.skipOptional(0xa0);

            key.body = files.pathToLocation(t.child.child);

            k.addElement(key);
        }

        PuKeys = new PublicKey[k.size()];
        k.copyInto(PuKeys);
    }

    /**
     * Loads PIN objects and places them into <code>PINs</code> array.
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void loadPINs() throws IOException, TLVException {

        Vector v = new Vector();
        resetLoader(v, null, null);
        loadObjects(PINS_TAG);

        Vector k = new Vector();

        for (int i = 0; i < v.size(); i++) {

            TLV t = (TLV) v.elementAt(i);

            if (t.type != TLV.SEQUENCE_TYPE) {      // not a PIN object
                continue;
            }

            PINAttributes pin = new PINAttributes();
            k.addElement(pin);

            t = t.child;        // commonObjectAttributes
            pin.label = t.child.getUTF8().trim();

            t = t.next;         // CommonAuthenticationObjectAttributes
            pin.id = t.child.getId();

            t = t.next.child.child;   // PinAttributes.pinFlags

            if (t.checkFlag(0)) {
                pin.pinFlags = PINAttributes.FLAG_CASE_SENSITIVE;
            }
            if (t.checkFlag(5)) {
                pin.pinFlags = PINAttributes.FLAG_NEEDS_PADDING;
            }

            t = t.next;
            pin.pinType = t.getEnumerated();

            t = t.next;
            pin.minLength = t.getInteger();

            t = t.next;
            pin.storedLength = t.getInteger();

            t = t.next;
            if (t.type == TLV.INTEGER_TYPE) {
                pin.maxLength = t.getInteger();
                t = t.next;
            } else {
                pin.maxLength = pin.storedLength;
            }

            // this entry is optional, default value is 0
            if (t.type == 0x80) {
                pin.pinReference = t.getInteger();
                t = t.next;
            }

            pin.padChar = t.getId();

            t = t.next.skipOptional(TLV.GEN_TIME_TYPE);
            pin.path = files.makePath(t.child);
        }

        if (k.size() == 0) {
            throw new IOException("PINs not found");
        }

        PINs = new PINAttributes[k.size()];
        k.copyInto(PINs);
    }

    /**
     * Loads attributes of X.509 certificates. Places results into
     * <code>Certificates</code> array. Places identifiers of all
     * certificates into <code>certificateIDs</code> vector.
     * @param loadValues if true loads also the certificates
     * @param loadTrusted if true load trusted certificates
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void loadCertificates(boolean loadValues,
                                  boolean loadTrusted)
            throws IOException, TLVException {

        Vector objects = new Vector();
        Vector locations = new Vector();
        resetLoader(objects, locations, null);
        if (loadTrusted) {
            loadObjects(TRUSTED_CERTIFICATES_TAG);
        }
        loadObjects(USEFUL_CERTIFICATES_TAG);
        loadObjects(USER_CERTIFICATES_TAG);

        Vector k = new Vector();
        certificateIDs = new Vector();

        for (int i = 0; i < objects.size(); i++) {

            TLV t = (TLV) objects.elementAt(i);

            // is it x.509 certificate?
            if (t.type != TLV.SEQUENCE_TYPE) {
                certificateIDs.addElement(t.child.next.child.getValue());
                continue;
            }

            Certificate cert = new Certificate();
            k.addElement(cert);

            t = t.child;        // commonObjectAttributes

            cert.label = t.child.getUTF8().trim();

            t = t.next;

            cert.id = t.child.getValue();
            certificateIDs.addElement(cert.id);

            TLV v = t.child.next;
            if (v != null) {
                v = v.skipOptional(TLV.BOOLEAN_TYPE);
                if (v != null) {
                    cert.requestId = v.getValue();
                }
            }

            t = t.next.child.child;

            cert.body = files.pathToLocation(t);

            if (loadValues) {
                cert.cert = files.loadObject(cert.body);
            }

            cert.header = (Location) locations.elementAt(i);
        }

        Certificates = new Certificate[k.size()];
        k.copyInto(Certificates);
    }

    /**
     * Verifies PIN status.
     * @param pin object containing PIN attributes
     * @return PIN status.
     */
    private int getPINStatus(PINAttributes pin) {

        try {
            files.select(pin.path);
            apdu.resetCommand().
            sendCommand(INS_VERIFY, pin.pinReference, 0, false);
        } catch (IOException e) {
            return PIN_BLOCKED;
        }

        if (apdu.lastSW == 0x9000) {
            return PIN_DISABLED;
        }

        if (apdu.lastSW == 0x6983) {
            return PIN_BLOCKED;
        }

        return PIN_REQUIRED;
    }

    /**
     * Verifies the PIN if necessary.
     * @param pin object containing PIN attributes
     * @return PIN_DISABLED - if the PIN is verified or not required;
     * PIN_BLOCKED - if the PIN is blocked; PIN_CANCELLED - if user
     * cancelled PIN entry dialog
     */
    private int checkPIN(PINAttributes pin, Token token) {

        while (true) {

            int status = getPINStatus(pin);

            if (status == PIN_DISABLED) {
                return PIN_DISABLED;
            }

            if (status == PIN_BLOCKED) {
                try {
                    MessageDialog
			.showMessage(Resource.getString(ResourceConstants
							.ERROR),
				     Resource
				     .getString(ResourceConstants
						.JSR177_WIM_PIN_BLOCKED) + ":\n" +
				     pin.label,
                     false, token);
                } catch (InterruptedException e) {}
                return PIN_BLOCKED;
            }

            // verification is required
            PINEntryDialog dialog;
            try {
                dialog = new PINEntryDialog(ACLPermissions.CMD_VERIFY,
                        pin, null, token);
            } catch (InterruptedException e) {
                return PIN_CANCELLED;
            }

            Object[] pins = dialog.getPINs();

            if (pins == null) {
                return PIN_CANCELLED;
            }

            boolean ok = true;
            try {
                byte[] tmp = (byte[]) pins[0];
                apdu.resetCommand().
                     putBytes(tmp, 0, tmp.length).
                     sendCommand(INS_VERIFY, pin.pinReference, 127, false);
            } catch (IOException e) {
                ok = false;
            }

            if (ok & (apdu.lastSW == 0x9000)) {
                return PIN_DISABLED;
            }

            try {
                MessageDialog
		    .showMessage(Resource
				 .getString(ResourceConstants
					    .ERROR),
				 Resource
				 .getString(ResourceConstants
					    .JSR177_WIM_PIN_NOT_VERIFIED),
                 false, token);
            } catch (InterruptedException e) {}
        }
    }


    /**
     * Generates CSR. See UserCredentialManager.generateCSR for details.
     * The calling method must load a vector that contains IDs of keys
     * for which CSRs were generated earlier and save it after successful
     * CSR generation.
     * @param nameInfo certificate subject name
     * @param keyLen key length
     * @param keyUsage key usage
     * @param forceKeyGen if set to true a new key must be generated
     * @param keyIDs IDs of keys for which CSRs were generated earlier.
     * If the new CSR is generated, the key ID is added into this vector.
     * @param token security token
     * @return the new CSR or null if operation cancelled
     * @throws UserCredentialManagerException if key is not found
     * @throws CMSMessageSignatureServiceException if CSR generation
     * failed
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public byte[] generateCSR(String nameInfo, int keyLen, int keyUsage,
                              boolean forceKeyGen, Vector keyIDs, Token token)
            throws UserCredentialManagerException,
            CMSMessageSignatureServiceException {

        // check the name

        if (nameInfo == null) {
            nameInfo = "CN=" + serialNumber;
        }

        TLV name;

        try {
            name = new TLV(RFC2253Name.toDER(nameInfo), 0);
        } catch (TLVException e) {
            throw new IllegalArgumentException("Invalid name");
        }

        try {
            if (MessageDialog.showMessage(
                Resource.getString(ResourceConstants.AMS_CONFIRMATION),
                Resource.getString(ResourceConstants.
                    JSR177_CERTIFICATE_GENERATED) +
                    "\n\n" + 
                    // "Name: "
                    Resource.getString(ResourceConstants.
                        JSR177_CERTIFICATE_SUBJECT) + ": " +
                    nameInfo +
                    "\n\n" + 
                    // "Key usage: "
                    Resource.getString(ResourceConstants.
                        JSR177_CERTIFICATE_KEYUSAGE) + ": " +
                    ((keyUsage ==
                        UserCredentialManager.KEY_USAGE_AUTHENTICATION) ?
                        // "authentication"
                        Resource.getString(ResourceConstants.
                            JSR177_CERTIFICATE_KEYUSAGE_AUTH) :
                        // "non-repudiation"
                        Resource.getString(ResourceConstants.
                            JSR177_CERTIFICATE_KEYUSAGE_NR)) +
                    "\n" + 
                    // "Algorithm: "
                    Resource.getString(ResourceConstants.
                        JSR177_CERTIFICATE_ALGORITHM) + ": " +
                    "RSA" + 
                    "\n" + 
                    // "Key length: "
                    Resource.getString(ResourceConstants.
                        JSR177_CERTIFICATE_KEYLENGTH) + ": " +
                    keyLen,
                true, token) == Dialog.CANCELLED) {
                return null;
            }
        } catch (InterruptedException e) {
            return null;
        }

        int keyId = -1;
        if (forceKeyGen) {
            try {
                keyId = generateKey(keyLen, keyUsage, token);
            } catch (IOException ioe) { // ignored
            } catch (InterruptedException ie) { // ignored
            }

            if (keyId == -1) {
                throw new UserCredentialManagerException(
                       UserCredentialManagerException.SE_NO_KEYGEN);
            }
            if (keyId == -2) {
                throw new UserCredentialManagerException(
                       UserCredentialManagerException.SE_NO_KEYS);
            }
        }

        // load info about keys

        try {
            if (keyId != -1) {
                loadPINs();
            }
            loadCertificates(false, true);
            loadPrivateKeys();
            loadPublicKeys();
        } catch (IOException e) {
                throw new UserCredentialManagerException(
                             UserCredentialManagerException.SE_NO_KEYS);
        }

        // find the 'best' key

        PrivateKey key = null;
        int keyType = 3;        // 0 - no certificate or CSR
                                // 1 - CSR
                                // 2 - certificate
                                // 3 - empty
        TLV keyValue = null;

        for (int i = 0; i < PrKeys.length; i++) {

            if (keyId != -1) {
                if (keyId == PrKeys[i].keyReference) {
                    key = PrKeys[i];
                    keyValue = getPublicKey(PrKeys[i].id);
                    break;
                }
                continue;
            }

            // check key size
            if (PrKeys[i].modulusLength != keyLen) {
                continue;
            }

            // check key usage
            if (! (keyUsage ==
                   UserCredentialManager.KEY_USAGE_AUTHENTICATION ?
                   PrKeys[i].authentication :
                   PrKeys[i].nonRepudiation)) {
                continue;
            }

            // check if certificate or CSR for this key exists
            int type = 0;

            if (getIDIndex(keyIDs, PrKeys[i].id) != -1) {
                type = 1;
            }

            if (getIDIndex(certificateIDs, PrKeys[i].id) != -1) {
                type = 2;
            }

            // is this key is better than the previous?
            if (type >= keyType) {
                continue;
            }

            // if PIN doesn't exist or blocked, find another key
            PINAttributes pin = getPIN(PrKeys[i].authId);
            if (pin == null || getPINStatus(pin) == PIN_BLOCKED) {
                continue;
            }

            // if the public key can't be retrieved, find another key
            TLV t = getPublicKey(PrKeys[i].id);
            if (t == null) {
                continue;
            }

            // the best key so far
            key = PrKeys[i];
            keyValue = t;
            keyType = type;
        }

        if (key == null) {
            throw new UserCredentialManagerException(
                             UserCredentialManagerException.SE_NO_KEYS);
        }

        // the key is found and loaded
        // check PIN for signature operation
        int pinStatus = checkPIN(getPIN(key.authId), token);

        if (pinStatus == PIN_CANCELLED) {
            return null;
        }

        if (pinStatus == PIN_BLOCKED) {
            throw new SecurityException("PIN blocked");
        }

        if (pinStatus != PIN_DISABLED) {
            // IMPL_NOTE: need warning message?
            throw new CMSMessageSignatureServiceException(
                 CMSMessageSignatureServiceException.SE_CRYPTO_FAILURE);
        }

        // PIN is verified, create the CSR

        TLV CRInfo = TLV.createSequence();

        CRInfo.setChild(TLV.createInteger(0)).
            setNext(name).
            setNext(keyValue).
            setNext(new TLV(TLV.SET_TYPE).setTag(0xa0)).
            setChild(TLV.createSequence()).
            setChild(TLV.createOID("1.2.840.113549.1.9.14")).
            setNext(new TLV(TLV.SET_TYPE)).
            setChild(TLV.createSequence()).
            setChild(TLV.createSequence()).
            setChild(TLV.createOID("2.5.29.15")).
            setNext(new TLV(TLV.BOOLEAN_TYPE, new byte[] {(byte) 255})).
            setNext(new TLV(TLV.OCTETSTR_TYPE,
                keyUsage == UserCredentialManager.KEY_USAGE_AUTHENTICATION ?
                new byte[] {3, 2, 7, (byte) 0x80} :
                new byte[] {3, 2, 6, 0x40}));

        byte[] sign;

        try {
            sign = signData(key, CRInfo.getDERData());
        } catch (IOException e) {
            throw new CMSMessageSignatureServiceException(
                 CMSMessageSignatureServiceException.SE_CRYPTO_FAILURE);
        }

        TLV alg = CRInfo.child.next.next.child.copy();

        TLV OID = TLV.createOID("1.2.840.113549.1.1.5");
        TLV params = alg.child.next;

        alg = TLV.createSequence();
        alg.setChild(OID).setNext(params);

        TLV request = TLV.createSequence();
        request.setChild(CRInfo).
                setNext(alg).
                setNext(new TLV(TLV.BITSTRING_TYPE, sign));

        // add to the vector of IDs of keys for which CSRs are generated
        keyIDs.addElement(key.id);
        return request.getDERData();
    }

    /**
     * Generates new key.
     * @param keyLen key length
     * @param keyUsage key usage
     * @param token security token
     * @return key reference or -1 if the key generation is not
     * supported or -2 if key cannot be generated
     * @throws IOException if I/O error occurs
     * @throws InterruptedException if interrupted
     */
    int generateKey(int keyLen, int keyUsage, Token token) throws IOException,
            InterruptedException {

        boolean nonRepudiation = (keyUsage ==
                UserCredentialManager.KEY_USAGE_NON_REPUDIATION);

        byte[] tmp = apdu.resetCommand().
            putByte(nonRepudiation ? 1 : 0).
            putShort(keyLen).
            sendCommand(INS_NEW, 0x0100, 240, false);

        if (apdu.lastSW == 0x9001) {
            return -2;
        }

        if (tmp.length != 6 ||
            Utils.getShort(tmp, 0) != 0x1234 ||
            Utils.getShort(tmp, 2) != 0x4321) {
            return -1;
        }

        apdu.resetCommand().
            putByte(nonRepudiation ? 1 : 0).
            putShort(keyLen);

        if (nonRepudiation) {
            // must create new PIN
            String[] pinInfo = PinMessageDialog.enterNewPIN(token);
            if (pinInfo == null) {
                return -1;
            }

            tmp = pinInfo[1].getBytes();
            int len = tmp.length;
            apdu.putBytes(tmp, 0, len);
            while (len++ < 8) {
                apdu.putByte(0xff);
            }

            tmp = Utils.stringToBytes(pinInfo[0]);
            len = tmp.length < 32 ? tmp.length : 32;
            apdu.putBytes(tmp, 0, len);
            while (len++ < 32) {
                apdu.putByte(0x20);
            }
        }

        tmp = apdu.sendCommand(INS_NEW, 0x0000, 240, false);
        return apdu.lastSW == 0x9000 ? tmp[0] & 0xff : -2;
    }

    /**
     * Sign given data using given key.
     * @param key private key
     * @param data data to be signed
     * @return signature prepended with one zero byte.
     * @throws IOException if I/O or crypto error occurs
     */
    private byte[] signData(PrivateKey key, byte[] data)
            throws IOException {

        // calculate SHA-1 digest
        byte[] tmp = Utils.getHash(data, 0, data.length);

        // MSE - RESTORE
        apdu.resetCommand().
             sendCommand(INS_MSE, 0xf300 | WIM_GENERIC_RSA_ID, 0, true);

        // MSE - SET
        apdu.resetCommand().
            putByte(0x84).      // key reference tag
            putByte(0x1).       // length
            putByte(key.keyReference).  // value
            putByte(0x81).      // private key path tag
            putByte(key.path.length * 2);   // length
        for (int i = 0; i < key.path.length; i++) {     // value
            apdu.putShort(key.path[i]);
        }
        apdu.sendCommand(INS_MSE, 0x41b6, 0, true);

        // sign the data
        tmp = apdu.resetCommand().
                putBytes(DigestInfoHeader, 0, DigestInfoHeader.length).
                putBytes(tmp, 0, 20).
                sendCommand(INS_PSO, 0x9e9a);

        byte[] sign = new byte[tmp.length - 1];
        System.arraycopy(tmp, 0, sign, 1, tmp.length - 2);
        return sign;
    }

    /**
     * Returns index of given identifier in the vector or -1 if not
     * found.
     * @param IDs vector containing identifiers
     * @param id identifier
     * @return index of given identifier or -1
     */
    private int getIDIndex(Vector IDs, byte[] id) {

        for (int j = 0; j < IDs.size(); j++) {
            if (Utils.byteMatch((byte[]) IDs.elementAt(j), id)) {
                return j;
            }
        }
        return -1;
    }

    /**
     * Returns PIN attributes for given authId.
     * @param authId identifier of PIN
     * @return PIN attributes or null
     */
    private PINAttributes getPIN(int authId) {

        for (int i = 0; i < PINs.length; i++) {
            if (PINs[i].id == authId) {
                return PINs[i];
            }
        }
        return null;
    }

    /**
     * Returns TLV that contains SubjectPublicKeyInfo structure for
     * public key.
     * @param id key identifier
     * @return TLV that contains SubjectPublicKeyInfo structure or null
     */
    private TLV getPublicKey(byte[] id) {

        // try to obtain the key from certificate

        for (int i = 0; i < Certificates.length; i++) {
            if (! Utils.byteMatch(Certificates[i].id, id)) {
                continue;
            }
            try {
                TLV t = files.loadObject(Certificates[i].body);
                return t.child.child.skipOptional(0xa0).next.next.next.
                        next.next.copy();
            } catch (IOException e) {
                continue;
            }
        }

        /*
            there is no certificate for this private key, try to
            read public key
        */
        for (int i = 0; i < PuKeys.length; i++) {
            if (! Utils.byteMatch(PuKeys[i].id, id)) {
                continue;
            }

            try {
                files.select(PuKeys[i].body.path);
                if (PuKeys[i].body.length == -1) {
                    PuKeys[i].body.length = files.getCurrrentFileSize();
                }
                byte[] tmp = files.readData(1, PuKeys[i].body.length,
                                            PuKeys[i].body.offset);

                TLV subjectPKInfo = TLV.createSequence();

                TLV alg = TLV.createSequence();
                subjectPKInfo.setChild(alg);

                alg.setChild(TLV.createOID("1.2.840.113549.1.1.1")).
                             setNext(new TLV(TLV.NULL_TYPE));

                alg.setNext(new TLV(TLV.BITSTRING_TYPE, tmp));
                return subjectPKInfo;
            } catch (IOException e) {
                break;
            }
        }
        return null;
    }

    /**
     * Adds a user certificate or certificate URI to a certificate store.
     * See UserCredentialManager.addCredential for details. Calling
     * method must remove leading and trailing spaces in label.
     * @param label the user friendly name associated with the
     * certificate
     * @param top chain of certificates from pkiPath
     * @param keyIDs vector that contains identifiers of keys for which
     * certificates are expected
     * @param token security token
     * @return  operation result
     * @throws IllegalArgumentException if certificate parsing error
     * occurs or label is not unique or user credential exists already
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public int addCredential(String label, TLV top, Vector keyIDs, Token token) {

        // load existing certificates
        try {
            loadPrivateKeys();
            loadCertificates(true, true);
        } catch (IOException e) {
            return SKIP;
        }

        // put certificates into array
        Vector u = new Vector();
        while (top != null) {
            u.addElement(top);
            top = top.next;
        }
        TLV path[] = new TLV[u.size()];
        u.copyInto(path);

        // verify certificates encoding and calculate identifier
        // the purpose of the check is to ensure that new certificates
        // will not cause runtime exceptions, not to verify X.509
        // compliance
        byte[][] IDs = new byte[path.length][];
        try {
            TLV Issuer = null;
            for (int i = 0; i < path.length; i++) {
                TLV t = path[i].child.child.skipOptional(0xa0).next.next;
                RFC2253Name.compare(t, t);  // issuer
                if (Issuer != null && ! RFC2253Name.compare(Issuer, t)) {
                    throw new IllegalArgumentException();
                }
                t = t.next;                 // validity
                t.child.getTime();          // notBefore
                t.child.next.getTime();     // notAfter
                t = t.next;                 // subject
                RFC2253Name.compare(t, t);
                Issuer = t;
                IDs[i] = getKeyHash(path[i]);   // subjectPublicKeyInfo
            }
        } catch (IOException e) {
            throw new IllegalArgumentException("Invalid pkiPath");
        } catch (NullPointerException npe) {
            throw new IllegalArgumentException("Invalid pkiPath");
        }
        
        // check if this WIM contains corresponding private key
        if (getPrivateKey(IDs[path.length - 1]) == null) {
            return SKIP;
        }

        // check that the label is unique for this card
        if (getCertificate(label) != null) {
            throw new IllegalArgumentException(label);
        }

        // eliminate certificates that already present on the card
        for (int i = 0; i < path.length; i++) {
            TLV t = path[i].child.child.skipOptional(0xa0);
            if (getCertificate(t.next.next, t) != null) {
                path[i] = null;
            }
        }

        if (path[path.length - 1] == null) {
            throw new IllegalArgumentException("credential exists");
        }

        // if the 1st certificate is self-signed we don't need to save it
        if (path.length > 1 && path[0] != null) {
            TLV t = path[0].child.child.skipOptional(0xa0).next.next;
            if (RFC2253Name.compare(t, t.next.next)) {
                path[0] = null;
            }
        }

        // find place for every certificate and generate CDF records

        startUpdate();

        Location[] locations;
        try {
            locations = putObjects(path);
        } catch (IOException e) {
            return ERROR;
        }
        if (locations == null) { // if no enough space
            return ERROR;
        }

        Vector headers = new Vector();
        int labelNum = 0;

        for (int i = 0; i < path.length; i++) {

            if (path[i] == null) {
                continue;
            }

            // generate header for certificate

            // create unique label if necessary
            String t_label = label;
             if (i < path.length - 1) {
                 for (int k = 0; k < 100000; k++) {
                     t_label = "certificate # " + labelNum++;
                     if (! label.equals(t_label) &&
                           getCertificate(t_label) == null) {
                         break;
                     }
                 }
            }

            // find identifier of the previous certificate
            byte[] prevID;
            if (i == 0) {
                TLV t = path[i].child.child.skipOptional(0xa0).next.next;
                Vector v = getCertsBySubject(t);
                if (v.size() == 0) {
                    prevID = new byte[20];
                } else {
                    prevID = ((Certificate) v.elementAt(0)).id;
                }
            } else {
                prevID = IDs[i - 1];
            }

            TLV commonAttrs = TLV.createSequence();
            commonAttrs.
                setChild(createLabel(t_label)).
                setNext(new TLV(TLV.BITSTRING_TYPE, new byte[2]));

            TLV commonCertAttrs = TLV.createSequence();
            commonCertAttrs.
                setChild(TLV.createOctetString(IDs[i])).
                setNext(TLV.createOctetString(prevID));

            Location l = locations[i];

            TLV x509Attrs = new TLV(0xa1);
            x509Attrs.setChild(TLV.createSequence()).
                    setChild(createPath(l.path, l.offset, l.length));

            TLV cdf = TLV.createSequence();
            cdf.setChild(commonAttrs).
                setNext(commonCertAttrs).
                setNext(x509Attrs);

            headers.addElement(cdf);
        }

        // find free space for headers in CDFs
        if (! putHeaders(headers)) {
            return ERROR;
        }

        // check PINs
        for (int i = 0; i < updatePIN.length; i++) {
            if (updatePIN[i]) {
                int pinStatus = checkPIN(PINs[i], token);
                if (pinStatus == PIN_CANCELLED) {
                    return CANCEL;
                }
                if (pinStatus == PIN_BLOCKED) {
                    throw new SecurityException("PIN blocked");
                }
                if (pinStatus != PIN_DISABLED) {
                    return ERROR;
                }
            }
        }

        // update
        try {
            doUpdate();
        } catch (IOException e) {
            return ERROR;
        }

        // remove key identifier from the list of expected certificates
        for (int i = 0; i < keyIDs.size(); i++) {
            if (Utils.byteMatch(IDs[path.length - 1],
                                (byte[]) keyIDs.elementAt(i))) {
                keyIDs.removeElementAt(i);
                break;
            }
        }

//        typeInfo("AddCredential");
        return SUCCESS;
    }

    /**
     * Finds place for new certificates and registers necessary file
     * updates.
     * @param path array containing certificates
     * @return array that contains locations for new certificates.
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private Location[] putObjects(TLV[] path) throws IOException,
            TLVException {

        // find the free space where certificates can be stored
        files.select(UnusedSpacePath);

        Vector freeSpace = new Vector();
        doParseDF(files.readFile(), UnusedSpacePath,
                freeSpace, null, null);

        Location[] blocks = new Location[freeSpace.size()];
        TLV[] records = new TLV[freeSpace.size()];
        for (int i = 0; i < freeSpace.size(); i++) {
            records[i] = (TLV) freeSpace.elementAt(i);
            blocks[i] = files.pathToLocation(records[i].child);
        }

        Location[] result = new Location[path.length];

        for (int j = 0; j < path.length; j++) {
            if (path[j] == null) {
                continue;
            }

            byte[] data = path[j].getDERData();

            for (int i = 0; i < blocks.length; i++) {

                Location block = blocks[i];
                if (block.length < data.length) {
                    continue;
                }

                block.length -= data.length;
                result[j] = new Location(block.path,
                        block.offset + block.length,
                        data.length);

                update(result[j].path, result[j].offset, data);

                // check if PIN is required for this update
                TLV t = records[i].child.next;
                if (t != null && t.type == TLV.OCTETSTR_TYPE) {

                    int id;
                    try {
                        id = t.getId();
                    } catch (TLVException e) {
                        // should never happen
                        return null;
                    }

                    for (int k = 0; k < PINs.length; k++) {
                        if (PINs[k].id == id) {
                            updatePIN[k] = true;
                        }
                    }
                }

                // Update length of block

                t = records[i].child.child.next.next;

                update(UnusedSpacePath, t.valueOffset,
                        Utils.shortToBytes(block.length));
                break;
            }
            if (result[j] == null) {
                return null;
            }
        }
        return result;
    }

    /**
     * Finds place for new certificates directory entries and registers
     * all necessary file updates.
     * @param headers vector containing new CDF entries
     * @return true if successful
     */
    private boolean putHeaders(Vector headers) {

        Vector holes = null;

        for (int i = 0; i < headers.size(); i++) {

            try {
                if (i == headers.size() - 1) {
                    holes = new Vector();
                    resetLoader(null, null, holes);
                    loadObjects(USEFUL_CERTIFICATES_TAG);
                } else
                if (i == 0) {
                    holes = new Vector();
                    resetLoader(null, null, holes);
                    loadObjects(USER_CERTIFICATES_TAG);
                }
            } catch (IOException e) {
                // should never happen
                return false;
            }

            TLV header = (TLV) headers.elementAt(i);
            byte[] data = header.getDERData();

            boolean found = false;
            for (int k = 0; k < holes.size(); k++) {

                Location l = (Location) holes.elementAt(k);
                if (l.length < data.length) {
                    continue;
                }

                l.length -= data.length;
                update(l.path, l.offset + l.length, data);

                // now update the free space after the new record
                if (l.length != 0) {
                    update(l.path, l.offset,
                                        getEmptySpaceHeader(l.length));
                    found = true;
                }
                break;
            }
            if (! found) {
                return false;
            }
        }
        return true;
    }

    /**
     * Creates PKCS#15 path.
     * @param path card file path
     * @param offset offset in the file
     * @param length length of data in the file
     * @return TLV object that represents PKCS#15 path
     */
    private TLV createPath(short[] path, int offset, int length) {

        TLV t = TLV.createSequence();
        t.setChild(TLV.createOctetString(Utils.shortsToBytes(path))).
            setNext(TLV.createInteger(Utils.shortToBytes(offset))).
            setNext(TLV.createInteger(Utils.shortToBytes(length)).
                    setTag(0x80));
        return t;
    }

    /**
     * Removes credential.
     * @param label the user friendly name associated with the
     * certificate.
     * @param isn the DER encoded ASN.1 structure that contains the
     * certificate issuer and serial number
     * @param token security token
     * @return operation result
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public int removeCredential(String label, TLV isn, Token token) {

        // load existing certificates (excluding trusted - can't delete
        // them)
        try {
            loadCertificates(true, false);
        } catch (IOException e) {
            return SKIP;
        }

        Certificate cert = getCertificate(isn.child, isn.child.next);

        if (cert == null) {
            // there is no such certificate
            return SKIP;
        }

        // IMPL_NOTE: should there be a warning?
        // if (! cert.label.trim().equals(label)) {}

        // find the certificate chain
        Vector chain = getChain(cert, null, false);

        // this chain can have common certificates with some other
        // chains, in this case only part of the chain should be deleted
        int count = chain.size();
        check:
        for (int i = 1; i < chain.size(); i++) {
            for (int k = 0; k < Certificates.length; k++) {
                if (Certificates[k] != chain.elementAt(i - 1) &&
                    Certificates[k].isIssuedBy((Certificate)
                        chain.elementAt(i))) {
                    count = i;
                    break check;
                }
            }
        }

        try {
            if (MessageDialog.showMessage(
                Resource.getString(ResourceConstants.AMS_CONFIRMATION),
                Resource.getString(ResourceConstants
                       .JSR177_CERTIFICATE_DELETED) +
                    "\n\n" + 
                    // "Label: " 
                    Resource.getString(ResourceConstants.
                        JSR177_CERTIFICATE_LABEL) + ": " +
                    cert.label + "\n\n" +
                    Certificate.getInfo(cert.cert) + "\n\n",
                true, token) == Dialog.CANCELLED) {
                return CANCEL;
            }
        } catch (InterruptedException e) {
            return CANCEL;
        }

        startUpdate();
        try {
            doRemove(chain, count);
            int pinStatus = checkPIN(PINs[0], token);
            if (pinStatus == PIN_CANCELLED) {
                return CANCEL;
            }
            if (pinStatus == PIN_BLOCKED) {
                throw new SecurityException("PIN blocked");
            }
            if (pinStatus != PIN_DISABLED) {
                return ERROR;
            }
            doUpdate();
        } catch (IOException e) {
            return ERROR;
        }

//        typeInfo("RemoveCredential");
        return SUCCESS;
    }

    /**
     * Register all necessary file updates for certificate removal.
     * @param chain certificate chain to be removed
     * @param count the number of certificates to be removed
     * @throws TLVException if parsing error occurs
     * @throws IOException if I/O error occurs
     */
    private void doRemove(Vector chain, int count) throws IOException,
            TLVException {

        // Load and parse UnusedSpace.
        files.select(UnusedSpacePath);

        Vector v_free = new Vector();     // records in UnusedSpace
        Vector v_location = new Vector(); // their offsets
        Vector v_hole = new Vector();     // empty space in the file
        doParseDF(files.readFile(), UnusedSpacePath,
                     v_free, v_location, v_hole);

        TLV[] free = new TLV[v_free.size()];
        v_free.copyInto(free);

        Location[] block = new Location[free.length];
        for (int i = 0; i < free.length; i++) {
            block[i] = files.pathToLocation(free[i].child);
        }

        for (int i = 0; i < count; i++) {

            Certificate cert = (Certificate) chain.elementAt(i);

            // remove the certificate header from CDF
            update(cert.header.path, cert.header.offset,
                    getEmptySpaceHeader(cert.header.length));

            // Now area in data file must be marked as unused.
            // try to append/prepend the new block to existing blocks

            Location hole = cert.body;

            int head = -1;
            int tail = -1;
            int empty = -1;

            for (int k = 0; k < block.length; k++) {
                if (block[k] == null ||
                    ! comparePaths(hole.path, block[k].path)) {
                    continue;
                }
                if (block[k].length == 0) {
                    empty = k;
                    continue;
                }
                if (doesFollow(block[k], hole)) {
                    head = k;
                    continue;
                }
                if (doesFollow(hole, block[k])) {
                    tail = k;
                }
            }

            if (head != -1 && tail != -1) {
                block[head].length = block[head].length + hole.length +
                                     block[tail].length;
                setBlockLength(free[head], block[head].length);

                Location newHole = (Location) v_location.elementAt(tail);
                deleteBlock(newHole);
                v_hole.addElement(newHole);
                block[tail] = null;
                continue;
            }

            if (head != -1) {
                block[head].length += hole.length;
                setBlockLength(free[head], block[head].length);
                continue;
            }

            if (tail != -1) {
                block[tail].offset -= hole.length;
                block[tail].length += hole.length;
                setBlockOffset(free[tail], block[tail].offset);
                setBlockLength(free[tail], block[tail].length);
                continue;
            }

            if (empty != -1) {
                block[empty].offset = hole.offset;
                block[empty].length = hole.length;
                setBlockOffset(free[empty], block[empty].offset);
                setBlockLength(free[empty], block[empty].length);
                continue;
            }

            // this is a new block, have to allocate new entry

            // generate new record with PIN-G authId
            TLV n = TLV.createSequence();
            TLV t = n.setChild(createPath(hole.path, hole.offset,
                        hole.length));
            t.setNext(TLV.createOctetString(new byte[] {(byte) PINs[0].id}));
            byte[] data = n.getDERData();

            // find space for new entry
            Location l = null;
            for (int k = 0; k < v_hole.size(); k++) {
                Location loc = (Location) v_hole.elementAt(k);
                if (loc.length >= data.length) {
                    l = loc;
                    break;
                }
            }

            if (l == null) {
                throw new IOException(
                        "Can't allocate new entry in EF(UnusedSpace)");
            }

            // update data
            update(UnusedSpacePath, l.offset, data);
            l.offset += data.length;
            l.length -= data.length;
            if (l.length != 0) {
                update(UnusedSpacePath, l.offset,
                        getEmptySpaceHeader(l.length));
            }
        }
    }

    /**
     * Modify offset in record of EF(UnusedSpace).
     * @param t TLV object that represents the record
     * @param offset the new offset value
     */
    private void setBlockOffset(TLV t, int offset) {

        t = t.child.child.next;
        update(UnusedSpacePath,
               t.valueOffset, Utils.shortToBytes(offset));
    }

    /**
     * Modify length of block in record of EF(UnusedSpace).
     * @param t TLV object that represents the record
     * @param length the new length value
     */
    private void setBlockLength(TLV t, int length) {

        t = t.child.child.next.next;
        update(UnusedSpacePath,
               t.valueOffset, Utils.shortToBytes(length));
    }

    /**
     * Mark record of EF(UnusedSpace) as unused.
     * @param l location of the record
     */
    private void deleteBlock(Location l) {
        update(UnusedSpacePath, l.offset, getEmptySpaceHeader(l.length));
    }

    /**
     * Verifies if the second block starts right after the fist.
     * @param a1 location of the first block
     * @param a2 location of the second block
     * @return true if the second block starts right after the fist
     */
    private static boolean doesFollow(Location a1, Location a2) {
        return (a1.offset + a1.length == a2.offset);
    }

    /**
     * Compares two paths.
     * @param path1 the first path
     * @param path2 the second path
     * @return true if the paths are equal
     */
    private static boolean comparePaths(short[] path1, short[] path2) {

        if (path1.length != path2.length) {
            return false;
        }

        for (int i = 0; i < path1.length; i++) {
            if (path1[i] != path2[i]) {
                return false;
            }
        }
        return true;
    }

    /** Vector contains locations of blocks that must be updated. */
    private Vector updateLocations;
    /** Vector contains values that must be written. */
    private Vector updateData;
    /**
     * Flags that indicate which PINs must be verified before the
     * update.
     */
    private boolean[] updatePIN;

    /** Initialises the new update. */
    private void startUpdate() {
        updateLocations = new Vector();
        updateData = new Vector();
        updatePIN = new boolean[PINs.length];
        updatePIN[0] = true;
    }

    /**
     * Registers file modification.
     * @param path file path
     * @param offset offset in the file
     * @param data data to be written
     */
    private void update(short[] path, int offset, byte[] data) {
        updateLocations.addElement(new Location(path, offset, 0));
        updateData.addElement(data);
    }

    /**
     * Performs all the registered updates.
     * @throws IOException if I/O error occurs
     */
    private void doUpdate() throws IOException {

        for (int i = 0; i < updateLocations.size(); i++) {
            Location l = (Location) updateLocations.elementAt(i);
            files.select(l.path);
            byte[] d = (byte[]) updateData.elementAt(i);
            files.writeData(d, 0, d.length, l.offset);
        }
        updateLocations = null;
        updateData = null;
    }


    /**
     * Returns private key for given ID.
     * @param id key identifier
     * @return the key or null if not found
     */
    private PrivateKey getPrivateKey(byte[] id) {

        for (int i = 0; i < PrKeys.length; i++) {
            if (Utils.byteMatch(id, PrKeys[i].id)) {
                return PrKeys[i];
            }
        }
        return null;
    }

    /**
     * Returns certificates for given subject.
     * @param subject TLV structure that represents RFC 2253 name.
     * @return certificates for given subject
     */
    private Vector getCertsBySubject(TLV subject) {

        Vector v = new Vector();
        for (int i = 0; i < Certificates.length; i++) {
            if (RFC2253Name.compare(subject,
                                    Certificates[i].getSubject())) {
                v.addElement(Certificates[i]);
            }
        }
        return v;
    }

    /**
     * Returns certificate for given label.
     * @param label the user friendly certificate label
     * @return the certificate object or null if not found
     */
    private Certificate getCertificate(String label) {

        for (int i = 0; i < Certificates.length; i++) {
            if (Certificates[i].label.equals(label)) {
                return Certificates[i];
            }
        }
        return null;
    }

    /**
     * Returns certificate for given issuer and serial number.
     * @param issuer TLV structure that represents RFC 2253 name.
     * @param serialNumber certificate serial number
     * @return the certificate object or null if not found
     */
    private Certificate getCertificate(TLV issuer, TLV serialNumber) {

        for (int i = 0; i < Certificates.length; i++) {
            TLV t = Certificates[i].cert.child.child.skipOptional(0xa0);
            if (t.match(serialNumber) &&
                t.next.next.match(issuer)) {
                return Certificates[i];
            }
        }
        return null;
    }

    /**
     * Calculates public key hash for given X.509 certificate.
     * @param cert TLV structure that represents X.509 certificate
     * @return public key hash
     * @throws TLVException if parsing error occurs
     */
    private static byte[] getKeyHash(TLV cert) throws TLVException {

        TLV t = cert.child.child.skipOptional(0xa0).
                next.next.next.next.next.child;
        // t is at subjectPublicKeyInfo.algorithm field

        byte[] data;
        int offset;
        int length;

        TLV m = t.child;
        t = t.next;
        if (m.match(TLV.createOID("1.2.840.113549.1.1.1"))) {
            // RSA
            data = (new TLV(t.data, t.valueOffset + 1).child).getValue();
            offset = 0;
            length = data.length;
        } else
        if (m.match(TLV.createOID("1.2.840.10045.2.1"))) {
            // ECDSA
            data = t.data;
            offset = t.valueOffset + 2;
            if (t.data[t.valueOffset + 1] == 4) {
                // uncompressed form
                length = (t.length - 2) / 2;
            } else {
                // compressed form
                length = (t.length - 2);
            }
        } else {
            // PKCS#15 doesn't say anything about this case
            // just calculate the hash of subjectPublicKey data
            data = t.data;
            offset = t.valueOffset + 1;
            length = t.length - 1;
        }

        return Utils.getHash(data, offset, length);
    }

    /**
     * Pads the string to 32 bytes as recommended in WIM and returns TLV
     * value that can be used as label.
     * @param label label string
     * @return TLV object for new label
     */
    private static TLV createLabel(String label) {
        int len = Utils.stringToBytes(label).length;
        while (len < 32) {
            label = label + " ";
            len++;
        }
        return TLV.createUTF8String(label);
    }

    /**
     * Creates block header that can be used to mark empty space in
     * directory file.
     * @param length free block length
     * @return block header
     */
    private static byte[] getEmptySpaceHeader(int length) {

        if (length == 1) {
            return new byte[] {(byte) 0xff};
        }

        if (length < 130) {
            return new byte[] {0, (byte) (length - 2)};
        }

        if (length < 259) {
            return new byte[] {0, (byte) 0x81, (byte) (length - 3)};
        }

        length -= 4;
        return new byte[] {0, (byte) 0x82, (byte) (length >> 8),
                           (byte) length};
    }

    /**
     * Parses EF(DF).
     * @param data data to be parsed
     * @param path file path
     * @param objects method places objects from file into this vector.
     *        Can be null. Contains values of TLV type
     * @param locations method places location of objects into this
     *        vector. Can be null. Contains values of type Location.
     * @param freeBlocks method places locations of free memory in
     *        EF(DF) into this vector. Can be null. Contains values of
     *        type Location.
     * @throws TLVException if parsing error occurs
     */
    private static void doParseDF(byte[] data, short[] path,
         Vector objects, Vector locations, Vector freeBlocks)
            throws TLVException {

        int start = 0;

        int current = 0;
        while (current < data.length) {

            // free space - skip
            if (data[current] == (byte) 0xff) {
                current++;
                continue;
            }

            // TLV object
            TLV t = new TLV(data, current);

            // empty one - skip
            if (t.type == 0) {
                current = t.valueOffset + t.length;
                continue;
            }

            // real object

            if (objects != null) {
                objects.addElement(t);
            }

            if (locations != null) {
                locations.addElement(new Location(path, current,
                                   t.valueOffset + t.length - current));
            }

            if (freeBlocks != null && start < current) {
                freeBlocks.addElement(
                            new Location(path, start, current - start));
            }

            current = t.valueOffset + t.length;
            start = current;
        }
        if (start < current && freeBlocks != null) {
            freeBlocks.addElement(
                            new Location(path, start, current - start));
        }
    }

    /**
     * Generates a signature.
     * @param nonRepudiation if true, the non-repudiation key must be
     * used, otherwise - authentication key
     * @param data the data to be signed
     * @param options signature content options
     * @param caNames array that contains parsed names of certificate
     * authorities
     * @param token security token
     * @return the DER encoded signature, null if the signature
     * generation was cancelled by the user before completion
     * @throws CMSMessageSignatureServiceException if an error occurs
     * during signature generation
     */
    public byte[] generateSignature(boolean nonRepudiation, byte[] data,
                                    int options, TLV[] caNames, Token token)
            throws CMSMessageSignatureServiceException {

        // load existing certificates
        try {
            loadPrivateKeys();
            loadCertificates(true, true);
        } catch (IOException e) {
            throw new CMSMessageSignatureServiceException(
                    CMSMessageSignatureServiceException.SE_FAILURE);
        }

        // find certificate chains
        Vector chains = getChains(nonRepudiation, caNames);
        if (chains.size() == 0) {
            throw new CMSMessageSignatureServiceException(
             CMSMessageSignatureServiceException.CRYPTO_NO_CERTIFICATE);
        }

        // select certificate
        Vector chain = selectChain(chains, token);
        if (chain == null) {
            return null;
        }

        Certificate cert = (Certificate) chain.elementAt(0);
        PrivateKey key = getPrivateKey(cert.id);
        PINAttributes pin = getPIN(key.authId);

        int pinStatus = checkPIN(pin, token);

        if (pinStatus == PIN_BLOCKED) {
            throw new SecurityException();
        }

        if (pinStatus == PIN_CANCELLED) {
            return null;
        }

        TLV signedAttrs = new TLV(TLV.SET_TYPE);
        TLV t = signedAttrs.setChild(TLV.createSequence());
        t.setChild(TLV.createOID("1.2.840.113549.1.9.3")). // ContentType
          setNext(new TLV(TLV.SET_TYPE)).
          setChild(TLV.createOID("1.2.840.113549.1.7.1"));  // data

        t.next = TLV.createSequence();
        t = t.next;

        Calendar calendar = Calendar.getInstance(TimeZone.getTimeZone("GMT"));
        calendar.setTime(new Date());        

        t.setChild(TLV.createOID("1.2.840.113549.1.9.5")). // signingTime
          setNext(new TLV(TLV.SET_TYPE)).
          setChild(TLV.createUTCTime(calendar));

        t.next = TLV.createSequence();
        t = t.next;

        t.setChild(TLV.createOID("1.2.840.113549.1.9.4")). // messageDigest
          setNext(new TLV(TLV.SET_TYPE)).
          setChild(TLV.createOctetString(Utils.getHash(data, 0, data.length)));

        // generate signature
        byte[] signature;
        try {
            signature = signData(key, signedAttrs.getDERData());
        } catch (IOException e) {
            throw new CMSMessageSignatureServiceException(
             CMSMessageSignatureServiceException.CRYPTO_FAILURE);
        }

        // format the signature
        /*
         *   ContentInfo ::= SEQUENCE {
         *      contentType ContentType,
         *      content [0] EXPLICIT ANY DEFINED BY contentType }
         *
         *  ContentType ::= OBJECT IDENTIFIER
         *
         *  id-signedData OBJECT IDENTIFIER ::= { iso(1) member-body(2)
         *      us(840) rsadsi(113549) pkcs(1) pkcs7(7) 2 }
         */

        TLV ContentInfo = TLV.createSequence();
        t = ContentInfo.
            setChild(TLV.createOID("1.2.840.113549.1.7.2")).
            setNext(new TLV(0xa0)).
            setChild(TLV.createSequence());

        /*
         *   t - SignedData
         *  SignedData ::= SEQUENCE {
         *    version CMSVersion,
         *     digestAlgorithms DigestAlgorithmIdentifiers,
         *    encapContentInfo EncapsulatedContentInfo,
         *    certificates [0] IMPLICIT CertificateSet OPTIONAL,
         *    - crls [1] IMPLICIT CertificateRevocationLists OPTIONAL,
         *    signerInfos SignerInfos }
         *
         *    DigestAlgorithmIdentifiers ::= SET OF
         *                                    DigestAlgorithmIdentifier
         */

        t = t.
            setChild(TLV.createInteger(1)).  // version
            setNext(new TLV(TLV.SET_TYPE));  // digestAlgorithms

        TLV SHAAlgId = TLV.createSequence();
        SHAAlgId.setChild(TLV.createOID("1.3.14.3.2.26"));   // SHA-1

        t.setChild(SHAAlgId.copy());

        /*
         *   EncapsulatedContentInfo ::= SEQUENCE {
         *    eContentType ContentType,
         *    eContent [0] EXPLICIT OCTET STRING OPTIONAL }
         *
         *    ContentType ::= OBJECT IDENTIFIER
         *
         *    id-data OBJECT IDENTIFIER ::= { iso(1) member-body(2)
         *        us(840) rsadsi(113549) pkcs(1) pkcs7(7) 1 }
         */

        t = t.setNext(TLV.createSequence());

        TLV m = t.setChild(TLV.createOID("1.2.840.113549.1.7.1"));

        if ((options &
            CMSMessageSignatureService.SIG_INCLUDE_CONTENT) != 0) {
            m.setNext(new TLV(0xa0)).
                setChild(TLV.createOctetString(data));
        }

        /*
         *   certificates [0] IMPLICIT CertificateSet OPTIONAL,
         *
         *  CertificateSet ::= SET OF CertificateChoices
         *
         * CertificateChoices ::= CHOICE {
         *   certificate Certificate,                 -- See X.509
         *  extendedCertificate [0] IMPLICIT ExtendedCertificate,
         *                                           -- Obsolete
         *  attrCert [1] IMPLICIT AttributeCertificate }
         *
         */

        if ((options &
             CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE) != 0) {
            t = t.setNext(new TLV(0xa0));
            TLV n = t.setChild(cert.cert);
            for (int i = 1; i < chain.size(); i++) {
                n = n.setNext(((Certificate) chain.elementAt(i)).cert);
            }
        }

        /*
         *   signerInfos SignerInfos }
         *  SignerInfos ::= SET OF SignerInfo
         *
         *  SignerInfo ::= SEQUENCE {
         *    version CMSVersion,
         *    sid SignerIdentifier,
         *    digestAlgorithm DigestAlgorithmIdentifier,
         *    signedAttrs [0] IMPLICIT SignedAttributes OPTIONAL,
         *    signatureAlgorithm SignatureAlgorithmIdentifier,
         *    signature SignatureValue,
         *    - unsignedAttrs [1] IMPLICIT UnsignedAttributes OPTIONAL }
         *
         *    SignerIdentifier ::= CHOICE {
         *      issuerAndSerialNumber IssuerAndSerialNumber,
         *      - subjectKeyIdentifier [0] SubjectKeyIdentifier }
         *
         *      IssuerAndSerialNumber ::= SEQUENCE {
         *        issuer Name,
         *        serialNumber CertificateSerialNumber }
         *
         */

        t = t.setNext(new TLV(TLV.SET_TYPE)).
              setChild(TLV.createSequence()).
              setChild(TLV.createInteger(1)).
              setNext(cert.getIssuerAndSerialNumber()).
              setNext(SHAAlgId).      // SHA-1
              setNext(signedAttrs).setTag(0xa0).
              setNext(cert.getKeyAlgorithmID()).
              setNext(new TLV(TLV.OCTETSTR_TYPE, signature, 1));

        return ContentInfo.getDERData();
    }


    /**
     * Finds certificate chains for specified operation.
     * @param nonRepudiation if true, the non-repudiation key must be
     * used, otherwise - authentication key
     * @param caNames array that contains parsed names of certificate
     * authorities
     * @return vector of certificate chains
     */
    private Vector getChains(boolean nonRepudiation, TLV[] caNames) {

        // find the chains
        Vector chains = new Vector();
        for (int i = 0; i < Certificates.length; i++) {
            PrivateKey key = getPrivateKey(Certificates[i].id);
            if ((key != null) && (nonRepudiation ? key.nonRepudiation :
                                                   key.authentication)) {
                Vector chain = getChain(Certificates[i], caNames, true);
                if (chain != null) {
                    chains.addElement(chain);
                }
            }
        }
        return chains;
    }

    /**
     * Builds certificate chain for given certificate.
     * @param cert user certificate
     * @param caNames array that contains parsed names of certificate
     * authorities
     * @param checkValidity check validity of certificates
     * @return vector containing certificate chain or null
     */
    private Vector getChain(Certificate cert, TLV[] caNames,
                            boolean checkValidity) {

        Vector chain = new Vector();

        while (true) {

            if (checkValidity && cert.isExpired()) {
                // the certificate is expired, can't build the chain
                return null;
            }

            chain.addElement(cert);
            TLV issuer = cert.getIssuer();

            // if this is the certificate issued by one of the CAs in
            // the list then the chain is complete
            if (caNames != null) {
                // check if we need any additional certificates
                for (int i = 0; i < caNames.length; i++) {
                    if (RFC2253Name.compare(issuer, caNames[i])) {
                        return chain;
                    }
                }
            }

            // search for the issuer certificate
            boolean found = false;
            for (int i = 0; i < Certificates.length; i++) {
                Certificate next = Certificates[i];
                found = cert.isIssuedBy(next) &&
                        ! chain.contains(next);
                if (found) {
                    cert = next;
                    break;
                }
            }

            if (! found) {
                // issuer certificate was not found
                // if caNames is null, the chain is good enough,
                // otherwise the chain is not found
                return caNames == null ? chain : null;
            }
        }
    }

    /**
     * Allows user to select certificate or cancel signature operation.
     * @param chains array of certifcate chains
     * @param token security token
     * @return user selected certificate chain
     */
    private Vector selectChain(Vector chains, Token token) {

        String[] labels = new String[chains.size()];
        for (int i = 0; i < chains.size(); i++) {
            labels[i] = ((Certificate)
                ((Vector) chains.elementAt(i)).elementAt(0)).label;
        }

        int chainIndex = -1;
        try {
            if (chains.size() == 1) {
                // if only one chain is found show certificate label to
                // the user

                if (MessageDialog.showMessage(
                        Resource.getString(ResourceConstants
					   .AMS_CONFIRMATION),
                        Resource.getString(ResourceConstants
					   .JSR177_CERTIFICATE_USED) +
                        labels[0], true, token) != Dialog.CANCELLED) {
                    chainIndex = 0;
                }
            } else {
                // if more than one chain is found ask user to choose
                // one of them
                chainIndex = MessageDialog.chooseItem(
                        Resource.getString(ResourceConstants
					   .JSR177_SELECT_CERTIFICATE),
                        Resource.getString(ResourceConstants
					   .JSR177_CERTIFICATES_AVAILABLE),
                        labels, token);
            }
        } catch (InterruptedException e) {}

        return (chainIndex == -1) ? null :
               (Vector) chains.elementAt(chainIndex);
    }

    // IMPL_NOTE: delete after debugging
    /**
     * Debug output.
     * @param Title title text
     * /
    private void typeInfo(String Title) {

        System.out.println("***********************************");
        System.out.println("debug " + Title);
        System.out.println("***********************************");

        try {
            files.select(UnusedSpacePath);

            Vector v_free = new Vector();     // records in UnusedSpace
            Vector v_location = new Vector(); // their offsets
            Vector v_hole = new Vector();     // empty space in the file
            doParseDF(files.readFile(), UnusedSpacePath,
                         v_free, v_location, v_hole);

            System.out.println("-------------------------------------");
            System.out.println("Unused space entries " + v_free.size());
            System.out.println();

            for (int i = 0; i < v_free.size(); i++) {

                ((Location) v_location.elementAt(i)).print();
                ((TLV) v_free.elementAt(i)).print();
                System.out.println();
            }

            System.out.println("-------------------------------------");
            System.out.println("Holes in UnusedSpace " + v_hole.size());
            System.out.println();

            for (int i = 0; i < v_hole.size(); i++) {
                ((Location) v_hole.elementAt(i)).print();
                System.out.println();
            }

            loadCertificates(true, true);

            System.out.println("-------------------------------------");
            System.out.println("Certificates " + Certificates.length);
            for (int i = 0; i < Certificates.length; i++) {
                Certificates[i].print();
            }

        } catch (Exception e) {
            System.out.println("Exception in typeInfo " + e);
        }
    }
*/
}
