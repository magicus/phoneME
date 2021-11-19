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

package wim_data;

import java.io.*;
import java.security.*;
import java.security.cert.CertificateException;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;

/**
 * This class represents WIM file system generator. It produces file
 * Data.java that can be used with WIM emulator. File system parameters
 * are defined using fields of this class.
 */
public class Main {

    /** WIM root DF. */
    short[] WIMDF         = {0x3f00, 0x5015}; // MUST be long name

    /** DF for PIN verification. */
    short[] PINDF         = {0x5300};

    /** Path to ODF. */
    short[] ODF           = {0x5031};
    /** Path to EF(TokenInfo). */
    short[] TokenInfo     = {0x5032};
    /** Path to EF(UnusedSpace) */
    short[] UnusedSpace   = {0x5033};
    /** Path to PrKDF. */
    short[] PrKDF         = {0x5200};
    /** Path to PuKDF. */
    short[] PuKDF         = {0x5201};
    /** Path to CDF for trusted certificates. */
    short[] TrustedCDF    = {0x5202};
    /** Path to CDF for useful certificates. */
    short[] UsefulCDF     = {0x5203};
    /** Path to AODF. */
    short[] AODF          = {0x5204};
    /** Path to file with trusted certificates. */
    short[] TrustedCerts  = {0x5205};
    /** Path to empty space. */
    short[] DataFile      = {0x5206};
    /** Path to CDF for user certificates. */
    short[] UserCDF       = {0x5207};

    /** Size in bytes of CDF files for user and useful certificates. */
    short CDFSpace         = 2048;
    /** Size of empty space. */
    short FreeSpace        = 4096;

    /** WIM GENERIC RSA security environment ID. */
    byte WIM_GENERIC_RSA_ID = 0x44;
    /** RSA algorithm ID. */
    byte RSA_ALGORITHM_ID = 0x55;

    /** If true, EF(ODF) contains entry with root directory path. */
    boolean IncludeRoot = true;

    /** PINs. */
    PIN[] PINs = {
        new PIN("PIN 1",                     2, 11, PINDF, "1234"),
        new PIN("Non repudiation key 1 PIN", 6, 22, PINDF, "2345"),
        new PIN("Non repudiation key 2 PIN", 7, 33, PINDF, "3456")};

    /** Pre-generated key pairs. */
    Key[] Keys = {
        new Key("NR key 1",             512, 1, true,  5,
                new short[] {0x5301}, new short[] {0x5281}, PINs),
        new Key("NR key 2",             512, 2, true,  7,
                new short[] {0x5302}, new short[] {0x5282}, PINs),
        new Key("Authentication key 1", 512, 0, false, 8,
                new short[] {0x5303}, new short[] {0x5283}, PINs),
        new Key("Authentication key 2", 512, 0, false, 20,
                new short[] {0x5304}, new short[] {0x5284}, PINs)
    };

    /** WIM token label. */
    String tokenLabel = "WIM 1.01 SATSA RI";

    /** WIM manifacturer ID. */
    String manufacturerID = "SATSA RI";

    /** Serial number. */
    byte[] SerialNumber = {0x15, (byte) 0x97, 0x52, 0x22, 0x25,
                           0x15, 0x40, 0x12, 0x40};

    /** The number of keys that can be generated. */
    int freeKeySlots = 4;
    /** Identifier of file for new key. */
    short newFileID = 0x5400;
    /** Identifier for new key. */
    byte newKeyID = 40;

    /** The number of PINs that can be added. */
    int freePINSlots = 4;
    /** Identifier of new PIN. */
    int newPINID = 80;
    /** Reference of new PIN. */
    int newPINRef = 60;

    /** File system. */
    FileSystem fs;
    /** Stream for debug output. */
    PrintStream log;
    /** Stream for results. */
    PrintStream src;

    /** Output directory */
    private String outputDataDir = "./output/";
    private String outputDir = "./output/";

    /**
     * Main entry point.
     * @param args command line arguments
     */
    public static void main(String[] args) throws Exception {
        new Main().run(args);
    }

    /**
     * Generates the data.
     * @throws KeyStoreException if this exception occurs
     * @throws CertificateException if this exception occurs
     * @throws IOException if this exception occurs
     * @throws NoSuchAlgorithmException if this exception occurs
     */
    public void run(String[] args) throws KeyStoreException, CertificateException,
            IOException, NoSuchAlgorithmException {
        
        if (args.length >= 1) {
            outputDataDir = args[0];
        }

        if (args.length >= 2) {
            outputDir = args[1];
        }

        fs = new FileSystem(WIMDF);
        fs.addFile(PINDF, FileSystem.PIN, null);

        log = new PrintStream(new FileOutputStream(outputDir + "listing.txt"));
        src = new PrintStream(new FileOutputStream(outputDataDir + "Data.java"));

        src.print("/*\n" +
                  " *   \n" +
                  " *\n" +
                  " * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.\n" +
                  " * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n" +  
                  " *\n */\n\npackage com.sun.satsa.pkiapplet;\n\n/**\n" +
                  " * This class contains WIM PINs, private keys and file " +
                  "system.\n" +
                  " */\npublic class Data {\n\n    /** Identifier for RSA generic " +
                  "SE. */\n" +
                  "    static final byte WIM_GENERIC_RSA_ID = " + WIM_GENERIC_RSA_ID +
                  ";\n\n");

        src.println("    /** The number of keys that can be generated. " +
                "*/\n    static short freeKeySlots = " + freeKeySlots +
                ";");
        src.println("    /** Identifier of file for new key. */\n    " +
                "static short newFileID = (short) " + newFileID + ";");
        src.println("    /** Identifier for new key. */\n    static " +
                "byte newKeyID = " + newKeyID + ";");

        src.println("    /** The number of PINs that can be added. " +
                "*/\n    static short freePINSlots = " + freePINSlots +
                ";");
        src.println("    /** Identifier of new PIN. */\n    static " +
                "byte newPINID = (byte) " + newPINID + ";");
        src.println("    /** Reference of new PIN. */\n    static " +
                "byte newPINRef = (byte) " + newPINRef + ";");
        src.println("    /** Identifier of PIN-G. */\n    static " +
                "final byte PIN_G_ID = (byte) " + PINs[0].id + ";");

        ODF();
        TokenInfo();
        Unused();
        TrustedCerts();
        UserCerts();
        AODF();
        Keys();

        byte[] data = fs.getEncoded();

        src.println("    /** AODF data offset. */\n    static final " +
                "short AODFOffset = (short) " + fs.getOffset("AODF") + ";");
        src.println("    /** PrKDF data offset. */\n    static final " +
                "short PrKDFOffset = (short) " + fs.getOffset("PrKDF") + ";");
        src.println("    /** PuKDF data offset. */\n    static final " +
                "short PuKDFOffset = (short) " + fs.getOffset("PuKDF") + ";");

        src.println();

        src.print("    /** Files. */");
        Utils.writeDataArray(src, "Files", data);
        src.println("}");

        log.close();
        src.close();
    }

    /**
     * Generates ODF file.
     */
    public void ODF() {

        /*
        PKCS15Objects ::= CHOICE {
                privateKeys         [0] PKCS15PrivateKeys,
                publicKeys          [1] PKCS15PublicKeys,
                trustedCertificates [5] PKCS15Certificates,
                usefulCertificates  [6] PKCS15Certificates,
                authObjects         [8] PKCS15AuthObjects,
                dataObjects         [7] PKCS15DataObjects
         }

         dataObjects : objects {
             opaqueDO : {
                 commonObjectAttributes {},
                 classAttributes {
                     applicationOID {wap-wsg-3}
                  },
                  typeAttributes direct : OCTET STRING:'3F00 5015'H
             }
         }

        */

        TLV odf = TLV.createSequence();

        TLV t;
        TLV current = odf.setChild(new TLV(TLV.NULL_TYPE));

        if (PrKDF != null) {
            t = new TLV(0xa0);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(PrKDF)));
            current = current.setNext(t);
        }

        if (PuKDF != null) {
            t = new TLV(0xa1);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(PuKDF)));
            current = current.setNext(t);
        }

        if (TrustedCDF != null) {
            t = new TLV(0xa5);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(TrustedCDF)));
            current = current.setNext(t);
        }

        if (UsefulCDF != null) {
            t = new TLV(0xa6);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(UsefulCDF)));
            current = current.setNext(t);
        }

        if (UserCDF != null) {
            t = new TLV(0xa4);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(UserCDF)));
            current = current.setNext(t);
        }

        if (AODF != null) {
            t = new TLV(0xa8);
            t.setChild(TLV.createSequence()).
                    setChild(TLV.createOctetString(
                            Utils.shortToBytes(AODF)));
            current = current.setNext(t);
        }

        if (IncludeRoot) {
            t = new TLV(0xa7);
            current.setNext(t);

            t = t.setChild(new TLV(0xa0)).
                    setChild(TLV.createSequence()).
                    setChild(TLV.createSequence());

            t = t.setChild(TLV.createSequence());
            t = t.setNext(TLV.createSequence());
            t.setChild(TLV.createOID("2.23.43.1.3"));
            t.setNext(new TLV(0xa1)).setChild(
                    TLV.createOctetString(Utils.shortToBytes(WIMDF)));
        }

        odf.child = odf.child.next;

        fs.addFile(ODF, FileSystem.READ, odf.getValue());

        log.println("********* ODF");
        Utils.writeHex(log, odf.getValue());
        log.println();
        odf.child.print(log);
        log.println("");
    }

    /**
     * Generates EF(TokenInfo) file.
     */
    public void TokenInfo() {

        /*
           PKCS15TokenInfo ::= SEQUENCE {
               version        INTEGER {v1(0)} (v1,...),
               serialNumber   OCTET STRING,
               manufacturerID PKCS15Label OPTIONAL,
               label          [0] PKCS15Label OPTIONAL,
               tokenflags     PKCS15TokenFlags,
               seInfo         SEQUENCE OF PKCS15SecurityEnvironmentInfo
                              OPTIONAL,
               recordInfo     [1] PKCS15RecordInfo OPTIONAL,
               supportedAlgorithms [2] SEQUENCE OF PKCS15AlgorithmInfo
                              OPTIONAL,
               ... -- For future extensions
            }
        */

        TLV t;

        TLV info = TLV.createSequence();

        t = info.setChild(TLV.createInteger(0));

        t = t.setNext(TLV.createOctetString(SerialNumber));

        t = t.setNext(TLV.createUTF8String(manufacturerID));

        t = t.setNext(TLV.createUTF8String(tokenLabel)).setTag(0x80);

        t = t.setNext(new TLV(TLV.BITSTRING_TYPE, new byte[] {0}));

        t = t.setNext(TLV.createSequence());

        t.setChild(TLV.createSequence()).
                       setChild(TLV.createInteger(WIM_GENERIC_RSA_ID)).
                       setNext(TLV.createOID("2.23.43.1.1.2"));

        t = t.setNext(new TLV(0xa2));

        t.setChild(TLV.createSequence()).
                       setChild(TLV.createInteger(RSA_ALGORITHM_ID)).
                       setNext(TLV.createInteger(0)).
                       setNext(new TLV(TLV.NULL_TYPE)).
                       setNext(new TLV(TLV.BITSTRING_TYPE,
                               Utils.shortToBytes(0x0640)));

        fs.addFile(TokenInfo, FileSystem.READ, info.getDERData());

        log.println("********* TokenInfo");
        Utils.writeHex(log, info.getDERData());
        log.println();
        info.print(log);
        log.println();
    }

    /**
     * Generates UnusedSpace file.
     */
    public void Unused() {

        /*
           PKCS15UnusedSpace ::= SEQUENCE {
               path   PKCS15Path,
               authId PKCS15Identifier OPTIONAL
           }
        */

        TLV m = TLV.createSequence();
        TLV v = m.setChild(TLV.createSequence());

        TLV t = v.setChild(Utils.createPath(DataFile, 0, FreeSpace));
        t.setNext(TLV.createOctetString(new byte[] {(byte) PINs[0].id}));

        byte[] data = new byte[t.getDERSize() * 19 - 4];

        v.setNext(new TLV(0, data));

        fs.addFile(UnusedSpace, FileSystem.UPDATE | FileSystem.EMPTY,
                   m.getValue());

        log.println("********* UnusedSpace");
        Utils.writeHex(log, m.getValue());
        log.println();
        m.child.print(log);
        log.println();
    }

    /**
     * Generates file with trusted certificates.
     * @throws IOException if this exception occurs
     * @throws KeyStoreException if this exception occurs
     * @throws CertificateException if this exception occurs
     * @throws NoSuchAlgorithmException if this exception occurs
     */
    public void TrustedCerts() throws IOException, KeyStoreException,
            CertificateException, NoSuchAlgorithmException {

        byte[] data = CACertificate;

        fs.addFile(TrustedCerts, FileSystem.READ, data);

        log.println("********* TrustedCerts");
        Utils.writeHex(log, data);
        log.println();
        new TLV(data, 0).print(log);
        log.println();

        /*
            PKCS15CertificateObject {CertAttributes} ::= PKCS15Object {
                                      PKCS15CommonCertificateAttributes,
                                      NULL,
                                      CertAttributes}

            PKCS15Object {ClassAttributes, SubClassAttributes,
                          TypeAttributes} ::=
               SEQUENCE {
               commonObjectAttributes PKCS15CommonObjectAttributes,
               classAttributes        ClassAttributes,
               subClassAttributes     [0] SubClassAttributes OPTIONAL,
               typeAttributes         [1] TypeAttributes
            }

            PKCS15CommonObjectAttributes ::= SEQUENCE {
                + label   PKCS15Label OPTIONAL,
                + flags   PKCS15CommonObjectFlags OPTIONAL,
                authId  PKCS15Identifier OPTIONAL,
                ... -- For future extensions
            }

            PKCS15CommonObjectFlags ::= BIT STRING {
                private(0),
                modifiable (1)
            }

            PKCS15CommonCertificateAttributes ::= SEQUENCE {
                + iD                  PKCS15Identifier,
                + authority           BOOLEAN DEFAULT FALSE,
                + requestId           PKCS15KeyIdentifier OPTIONAL,
                thumbprint          [0] PKCS15OOBCertHash OPTIONAL,
                ... -- For future extensions
            }

            PKCS15X509CertificateAttributes ::= SEQUENCE {
                value        PKCS15ObjectValue { PKCS15X509Certificate },
                subject      Name OPTIONAL,
                issuer       [0] Name OPTIONAL,
                serialNumber CertificateSerialNumber OPTIONAL,
                ... -- For future extensions
            }
        */

        // Parse CA certificate
        TLV CACertPointer = new TLV(data, 0);

        CACertPointer = CACertPointer.child.child;
        if (CACertPointer.type == TLV.VERSION_TYPE) {
            CACertPointer = CACertPointer.next;
        }

        // CACertPointer is at SerialNumber field

        byte[] modulus = new TLV(CACertPointer.next.next.next.next.next.
                                 child.next.getValue(), 1).child.getValue();
        byte[] id = Utils.getHash(modulus, 0, modulus.length);


        TLV commonAttrs = TLV.createSequence();
        commonAttrs.
            setChild(Utils.createLabel("CA CERTIFICATE")).
            setNext(new TLV(TLV.BITSTRING_TYPE, Utils.shortToBytes(0)));

        TLV commonCertAttrs = TLV.createSequence();
        commonCertAttrs.
            setChild(TLV.createOctetString(id)).
            setNext(new TLV(TLV.BOOLEAN_TYPE, new byte[] {(byte) 255}));

        TLV x509Attrs = new TLV(0xa1);
        x509Attrs.setChild(TLV.createSequence()).
                  setChild(Utils.createPath(TrustedCerts, 0, data.length));

        TLV cdf = TLV.createSequence();
        cdf.setChild(commonAttrs).
            setNext(commonCertAttrs).
            setNext(x509Attrs);

        byte[] cdfData = cdf.getDERData();

        fs.addFile(TrustedCDF, FileSystem.READ, cdfData);

        log.println("********* TrustedCDF");
        Utils.writeHex(log, cdfData);
        log.println();
        new TLV(cdfData, 0).print(log);
        log.println();
    }

    /**
     * Generates user certificate files.
     */
    public void UserCerts() {

        byte[] UsefulCDFData = new TLV(0, new byte[CDFSpace]).getDERData();

        // There are two CDF files, one of them for 'useful', other
        // for user certificates. Both have the same size and content,
        // initially they are empty.
        fs.addFile(UsefulCDF, FileSystem.UPDATE | FileSystem.EMPTY,
                   UsefulCDFData);
        fs.addFile(UserCDF, FileSystem.UPDATE | FileSystem.EMPTY,
                   UsefulCDFData);

        log.println("********* UsefulCDF");
        Utils.writeHex(log, UsefulCDFData);
        log.println();
        new TLV(UsefulCDFData, 0).print(log);
        log.println();

        fs.addFile(DataFile, FileSystem.UPDATE | FileSystem.EMPTY,
                   new byte[FreeSpace]);
    }

    /**
     * Generates AODF file.
     * @throws IOException if this exception occurs
     */
    public void AODF() throws IOException {

        /*
            PKCS15Authentication ::= CHOICE {
                pin       PKCS15AuthenticationObject { PKCS15PinAttributes },
                ... -- For future extensions, e.g. biometric authentication
                -- objects
            }

            PKCS15AuthenticationObject {AuthObjectAttributes} ::=
                    PKCS15Object {
                PKCS15CommonAuthenticationObjectAttributes,
                NULL,
                AuthObjectAttributes
            }

            PKCS15Object {ClassAttributes, SubClassAttributes,
                   TypeAttributes} ::=
               SEQUENCE {
               commonObjectAttributes PKCS15CommonObjectAttributes,
               classAttributes        ClassAttributes,
               subClassAttributes     [0] SubClassAttributes OPTIONAL,
               typeAttributes         [1] TypeAttributes
            }

            PKCS15CommonObjectAttributes ::= SEQUENCE {
                + label   PKCS15Label OPTIONAL,
                + flags   PKCS15CommonObjectFlags OPTIONAL,
                authId  PKCS15Identifier OPTIONAL,
                ... -- For future extensions
            }

            PKCS15CommonAuthenticationObjectAttributes ::= SEQUENCE {
                + authId PKCS15Identifier,
                 ... –- For future extensions
             }

            PKCS15PinAttributes ::= SEQUENCE {
                + pinFlags      PKCS15PinFlags,
                + pinType       PKCS15PinType,
                + minLength     INTEGER (pkcs15-lb-minPinLength..
                                         pkcs15-ub-minPinLength),
                + storedLength  INTEGER (pkcs15-lb-minPinLength..
                                         pkcs15-ub-storedPinLength),
                maxLength     INTEGER OPTIONAL,
                + pinReference  [0] PKCS15Reference OPTIONAL,
                + padChar       OCTET STRING (SIZE(1)) OPTIONAL,
                lastPinChange GeneralizedTime OPTIONAL,
                + path          PKCS15Path OPTIONAL,
                ... -- For future extensions
            }

            PKCS15PinFlags ::= BIT STRING {
                case-sensitive   (0),        80
                local            (1),        40
                change-disabled  (2),        20     *
                unblock-disabled (3),        10
                initialized      (4),        08     *
                needs-padding    (5),        04     *
                unblockingPin    (6),        02
                soPin            (7),        01
                disable-allowed  (8)
            }

            PKCS15PinType ::= ENUMERATED {bcd, ascii-numeric, utf8, ...
               -- bcd = one nibble contains one digit
               -- ascii-numeric = one byte contains one ASCII digit
               -- utf8 = password is stored in UTF8 encoding
            }
        */

        TLV[] t = new TLV[PINs.length + freePINSlots];
        for (int i = 0; i < PINs.length; i++) {
            t[i] = PINs[i].toTLV();
        }

        int PINRecordSize = 0;
        int PINLabelOffset = 0;

        for (int i = 0; i < freePINSlots; i++) {
            PIN pin = new PIN("", newPINID + i, newPINRef + i, PINDF, "1");
            t[PINs.length + i] = pin.toTLV();
            if (i == 0) {
                PINRecordSize = t[PINs.length + i].getDERSize();
                PINLabelOffset = t[PINs.length + i].copy().child.child.
                                                            valueOffset;
            } else {
                if (PINRecordSize != t[PINs.length + i].getDERSize()) {
                    System.out.println(
                            "Error - PIN placeholder size changed.");
                    System.exit(1);
                }
            }
            t[PINs.length + i].type = 0;
        }

        for (int i = 0; i < t.length - 1; i++) {
            t[i].next = t[i + 1];
        }

        TLV aodf = TLV.createSequence();
        aodf.setChild(t[0]);
        byte[] data = aodf.getValue();
        fs.addFile(AODF, FileSystem.READ, data, "AODF");

        src.println("    /** PIN record size. */\n    static final " +
                "short PINRecordSize = " + PINRecordSize + ";");
        src.println("    /** PIN label offset. */\n    static final " +
                "short PINLabelOffset = " + PINLabelOffset + ";");
        src.println("    /** New PIN offset in AODF. */\n    static " +
                "short newPINOffset = " + (data.length -
                                   freePINSlots * PINRecordSize) + ";");

        log.println("********* AODF");
        Utils.writeHex(log, data);
        log.println();
        aodf.child.print(log);
        log.println();

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        out.writeByte(PINs.length);
        for (int i = 0; i < PINs.length; i++) {
            out.writeByte(PINs[i].ref);
            byte[] PINData = PINs[i].getData();
            out.writeByte(PINData.length);
            out.write(PINData);
        }
        src.print("\n    /** PINs. */");
        Utils.writeDataArray(src, "PINs", baos.toByteArray());
    }

    /**
     * Generates all necessary data structures for keys.
     * @throws NoSuchAlgorithmException if this exception occurs
     * @throws IOException if this exception occurs
     */
    public void Keys() throws NoSuchAlgorithmException, IOException {

        /*
            PKCS15PrivateKeyObject {KeyAttributes} ::= PKCS15Object {
                PKCS15CommonKeyAttributes,
                PKCS15CommonPrivateKeyAttributes,
                KeyAttributes
            }

            PKCS15Object {ClassAttributes, SubClassAttributes,
                          TypeAttributes} ::=
               SEQUENCE {
               commonObjectAttributes PKCS15CommonObjectAttributes,
               classAttributes        ClassAttributes,
               subClassAttributes     [0] SubClassAttributes OPTIONAL,
               typeAttributes         [1] TypeAttributes
            }

            PKCS15CommonObjectAttributes ::= SEQUENCE {
                + label   PKCS15Label OPTIONAL,
                + flags   PKCS15CommonObjectFlags OPTIONAL,
                + authId  PKCS15Identifier OPTIONAL,
                ... -- For future extensions
            }

            PKCS15CommonKeyAttributes ::= SEQUENCE {
                + iD            PKCS15Identifier,
                + usage         PKCS15KeyUsageFlags,
                native        BOOLEAN DEFAULT TRUE,
                accessFlags   PKCS15KeyAccessFlags OPTIONAL,
                + keyReference  PKCS15Reference OPTIONAL,
                startDate     GeneralizedTime OPTIONAL,
                endDate       [0] GeneralizedTime OPTIONAL,
                ... -- For future extensions
            }

            PKCS15KeyUsageFlags ::= BIT STRING {
                encrypt         (0),    80
                decrypt         (1),    40
                sign            (2),    20
                signRecover     (3),    10
                wrap            (4),    08
                unwrap          (5),    04
                verify          (6),    02
                verifyRecover   (7),    01
                derive          (8),       80
                nonRepudiation  (9)        40
            }

            PKCS15PrivateRSAKeyAttributes ::= SEQUENCE {
                value         PKCS15ObjectValue {PKCS15RSAPrivateKey},
                modulusLength INTEGER, -- modulus length in bits, e.g. 1024
                keyInfo       PKCS15KeyInfo {PKCS15RSAParameters,
                              PKCS15PublicKeyOperations} OPTIONAL,
                ... –- For future extensions
            }

            PKCS15KeyInfo {ParameterType, OperationsType} ::= CHOICE {
                reference PKCS15Reference,
                paramsAndOps SEQUENCE {
                parameters          ParameterType,
                supportedOperations OperationsType OPTIONAL}
            }

            PKCS15PublicKeyObject {KeyAttributes} ::= PKCS15Object {
                PKCS15CommonKeyAttributes,
                PKCS15CommonPublicKeyAttributes,
                KeyAttributes}

            PKCS15CommonPublicKeyAttributes ::= SEQUENCE {
                subjectName Name OPTIONAL,
                ... -- For future extensions
             }

            PKCS15PublicRSAKeyAttributes ::= SEQUENCE {
                value          PKCS15ObjectValue {PKCS15RSAPublicKey},
                modulusLength  INTEGER, -- modulus length in bits, e.g. 1024
                keyInfo        PKCS15KeyInfo {PKCS15RSAParameters,
                              PKCS15PublicKeyOperations} OPTIONAL,
                ... –- For future extensions
            }

            PKCS15RSAPublicKey ::= SEQUENCE {
                modulus         INTEGER, -- n
                publicExponent  INTEGER  -- e
            }

        */

        TLV[] PrK = new TLV[Keys.length + freeKeySlots];
        TLV[] PuK = new TLV[Keys.length + freeKeySlots];

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        out.writeByte(Keys.length);

        for (int i = 0; i < Keys.length; i++) {

            Keys[i].init();

            RSAPrivateKey priv = Keys[i].priv;
            RSAPublicKey pub =   Keys[i].pub;

            int offset;
            int len;

            out.writeByte(Keys[i].id);
            out.writeByte(Keys[i].pinIndex);
            out.writeByte(Keys[i].nonRepudiation ? 0 : 1);

            byte[] x = priv.getModulus().toByteArray();
            offset = 0;
            len = x.length;
            while (x[offset] == 0) {
                offset++;
                len--;
            }
            short keyLen = (short)(len * 8);
            out.writeShort(keyLen);
            out.writeShort(len);
            out.write(x, offset, len);

            x = priv.getPrivateExponent().toByteArray();
            offset = 0;
            len = x.length;
            while (x[offset] == 0) {
                offset++;
                len--;
            }
            out.writeShort(len);
            out.write(x, offset, len);

            PrK[i] = Keys[i].getPrivate(RSA_ALGORITHM_ID);
            PuK[i] = Keys[i].getPublic();

            byte[] keyData = new TLV(new TLV(pub.getEncoded(), 0).child.
                                       next.getValue(), 1).getDERData();

            fs.addFile(Keys[i].PublicPath, FileSystem.READ, keyData);
            fs.addFile(Keys[i].PrivatePath, FileSystem.PrK,
                       new byte[] {(byte) Keys[i].id});
        }


        int privKeyRecordSize = 0;
        int privPINIDOffset = 0;
        int privHashOffset = 0;
        int privUsageOffset = 0;
        int privKeyLengthOffset = 0;

        int pubKeyRecordSize = 0;
        int pubHashOffset = 0;
        int pubKeyLengthOffset = 0;

        short FileId = newFileID;
        for (int i = 0; i < freeKeySlots; i++) {

            Key k = new Key("New key " + (i + 1), 1024, 0, true, newKeyID + i,
                new short[] {FileId},
                new short[] {(short) (FileId + 1)}, PINs);
            FileId += 2;

            TLV prk = k.getPrivate(RSA_ALGORITHM_ID);

            TLV puk = k.getPublic();

            if (i == 0) {
                TLV t = prk.copy();
                privKeyRecordSize =   t.getDERSize();
                privPINIDOffset =     t.child.child.next.next.valueOffset;
                privHashOffset =      t.child.next.child.valueOffset;
                privUsageOffset =     t.child.next.child.next.valueOffset;
                privKeyLengthOffset = t.child.next.next.child.child.next.
                                        valueOffset;

                t = puk.copy();
                pubKeyRecordSize = t.getDERSize();
                pubHashOffset =    t.child.next.child.valueOffset;
                pubKeyLengthOffset = t.child.next.next.child.child.next.
                                       valueOffset;
            }

            prk.type = 0;
            PrK[Keys.length + i] = prk;

            puk.type = 0;
            PuK[Keys.length + i] = puk;

            fs.addFile(k.PublicPath, FileSystem.READ, new byte[0]);
            fs.addFile(k.PrivatePath, FileSystem.PrK,
                       new byte[] {(byte) k.id});
        }

        for (int i = 1; i < PrK.length; i++) {
            PrK[i - 1].setNext(PrK[i]);
            PuK[i - 1].setNext(PuK[i]);
        }

        byte[] data;

        TLV prkdf = TLV.createSequence();
        prkdf.setChild(PrK[0]);
        data = prkdf.getValue();
        fs.addFile(PrKDF, FileSystem.READ, data, "PrKDF");

        src.println("    /** Offset from start of the first empty entry " +
                "in PrKDF. */\n    static short newPrivKeyOffset = " +
                (data.length - freeKeySlots * privKeyRecordSize) + ";");
        src.println("    /** The size of record in PrKDF. */\n    " +
                "static final short privKeyRecordSize = " +
                privKeyRecordSize + ";");
        src.println("    /** Offset of PIN identifier data in PrKDF " +
                "record. */\n    static final short privPINIDOffset = " +
                privPINIDOffset + ";");
        src.println("    /** Offset of key identifier data in PrKDF " +
                "record. */\n    static final short privHashOffset = " +
                privHashOffset + ";");
        src.println("    /** Offset of usage field data in PrKDF " +
                "record. */\n    static final short privUsageOffset = " +
                privUsageOffset + ";");
        src.println("    /** Offset of key length in PrKDF record. " +
                "*/\n    static final short privKeyLengthOffset = " +
                privKeyLengthOffset + ";");
        src.println();

        log.println("********* PrKDF");
        Utils.writeHex(log, data);
        log.println();
        prkdf.child.print(log);
        log.println();



        TLV pukdf = TLV.createSequence();
        pukdf.setChild(PuK[0]);
        data = pukdf.getValue();
        fs.addFile(PuKDF, FileSystem.READ, data, "PuKDF");

        src.println("    /** Offset from start of the first empty entry " +
                "in PrKDF. */\n    static short newPubKeyOffset = " +
                (data.length - freeKeySlots * pubKeyRecordSize) + ";");
        src.println("    /** The size of record in PuKDF. */\n    static" +
                " final short pubKeyRecordSize = " + pubKeyRecordSize + ";");
        src.println("    /** Offset of key identifier data in PuKDF " +
                "record. */\n    static final short pubHashOffset = " +
                pubHashOffset + ";");
        src.println("    /** Offset of key length in PuKDF record. */\n" +
                "    static final short pubKeyLengthOffset = " +
                pubKeyLengthOffset + ";");
        src.println();

        log.println("********* PuKDF");
        Utils.writeHex(log, data);
        log.println();
        pukdf.child.print(log);
        log.println();
        src.print("    /** Private keys. */");

        Utils.writeDataArray(src, "PrivateKeys", baos.toByteArray());
    }

    /**
     * CA certificate (MIDP 2.0 RI keystore, alias dummyca).
     */
    static byte[] CACertificate = { 48, -126, 3, 61, 48, -126, 2, 37, 2, 4,
        61, 62, -50, -118, 48, 13, 6, 9, 42, -122, 72, -122, -9, 13, 1, 1, 4,
        5, 0, 48, 99, 49, 16, 48, 14, 6, 3, 85, 4, 3, 19, 7, 116, 104, 101,
        104, 111, 115, 116, 49, 12, 48, 10, 6, 3, 85, 4, 11, 19, 3, 74, 67,
        84, 49, 17, 48, 15, 6, 3, 85, 4, 10, 19, 8, 100, 117, 109, 109, 121,
        32, 67, 65, 49, 20, 48, 18, 6, 3, 85, 4, 7, 19, 11, 83, 97, 110, 116,
        97, 32, 67, 108, 97, 114, 97, 49, 11, 48, 9, 6, 3, 85, 4, 8, 19, 2,
        67, 65, 49, 11, 48, 9, 6, 3, 85, 4, 6, 19, 2, 85, 83, 48, 30, 23, 13,
        48, 50, 48, 55, 50, 52, 49, 53, 53, 56, 48, 50, 90, 23, 13, 49, 50,
        48, 55, 50, 49, 49, 53, 53, 56, 48, 50, 90, 48, 99, 49, 16, 48, 14,
        6, 3, 85, 4, 3, 19, 7, 116, 104, 101, 104, 111, 115, 116, 49, 12, 48,
        10, 6, 3, 85, 4, 11, 19, 3, 74, 67, 84, 49, 17, 48, 15, 6, 3, 85, 4,
        10, 19, 8, 100, 117, 109, 109, 121, 32, 67, 65, 49, 20, 48, 18, 6, 3,
        85, 4, 7, 19, 11, 83, 97, 110, 116, 97, 32, 67, 108, 97, 114, 97, 49,
        11, 48, 9, 6, 3, 85, 4, 8, 19, 2, 67, 65, 49, 11, 48, 9, 6, 3, 85, 4,
        6, 19, 2, 85, 83, 48, -126, 1, 34, 48, 13, 6, 9, 42, -122, 72, -122,
        -9, 13, 1, 1, 1, 5, 0, 3, -126, 1, 15, 0, 48, -126, 1, 10, 2, -126,
        1, 1, 0, -32, -30, -97, -62, 117, 76, 16, 83, -69, 72, -53, 84, 35,
        -28, -111, 23, -94, -20, 89, -97, 111, 87, 127, -101, 106, 31, -109,
        94, 105, -15, -44, 86, -71, 101, -98, 20, 39, -72, -79, -75, -99,
        -22, -42, -17, -62, 3, 78, -101, 40, 30, 27, 8, 26, 5, 77, -9, -75,
        -25, -110, -51, 58, 89, -40, -74, -74, 32, -13, -56, 43, -8, 30, 56,
        -39, -76, -12, 35, -64, 3, -55, 2, 113, 122, -84, 64, 37, 103, -2,
        -62, 106, -46, 59, 37, 20, 41, -11, -103, -116, -17, 81, 37, -92, 55,
        -38, -79, 101, -74, 73, -9, -99, 30, 90, 52, 14, 23, -14, 80, -110,
        -123, -69, 28, 108, -82, 106, -28, -32, 41, -27, -3, -51, 16, 26,
        -85, 7, -57, -92, 50, -41, -67, 112, 36, -58, 83, 115, 51, -107, 98,
        -124, -103, -75, 59, -125, -112, 14, -68, -111, 88, -16, -107, -106,
        21, 15, -19, 104, -70, 70, 5, 34, -103, 85, 30, 57, -66, -11, 52,
        -51, -71, 67, -34, 28, -21, -16, 121, -18, -99, 96, -91, 80, 120,
        -32, 56, -7, 40, -106, -81, 7, -103, -42, -50, 124, -68, 59, 4, -3,
        13, 9, 112, -79, -83, -49, -91, 70, -56, 65, 92, 7, -40, -101, -53,
        -41, -53, 92, -60, -106, 14, 65, -124, 59, 40, -111, 7, -59, -36,
        -98, 113, 120, 16, 65, -115, 5, 61, 54, 63, 120, -95, -100, -77, 55,
        -127, 42, -91, -48, 37, -83, -2, 113, 7, 2, 3, 1, 0, 1, 48, 13, 6, 9,
        42, -122, 72, -122, -9, 13, 1, 1, 4, 5, 0, 3, -126, 1, 1, 0, 19,
        -115, 51, 67, 74, -32, -104, -44, -31, 73, -70, -60, -128, -106, -88,
        -95, -33, -2, -100, -48, -98, -43, -110, 43, 4, 77, -63, -11, 119,
        -60, 121, 91, -42, -28, -61, -116, 104, 58, 120, -88, -28, 12, 64,
        -82, -94, 36, 27, -38, -4, 108, -20, 96, -94, 96, 82, 40, -114, -80,
        -65, 31, -96, -53, -90, -55, 37, 102, -79, -12, 125, 91, 102, -8, 10,
        -95, 26, -82, -11, 83, -31, 102, -96, -114, -40, 0, 99, -9, -34, 112,
        21, 54, 105, -92, 121, 101, 5, 90, 76, -10, -119, 117, -87, 65, -54,
        17, -6, 62, -36, -60, 78, -78, 45, 9, 27, -40, -78, -91, 74, -113,
        80, -118, -21, 71, 88, 24, 39, -74, -40, 86, -23, -79, 68, -3, 99,
        36, 122, 26, -72, 91, 55, 113, -39, -29, 84, -79, 85, 100, -93, 118,
        26, 46, -86, -35, -99, 100, 113, -97, -65, -94, 47, 115, -43, 81, 16,
        -23, -52, 52, 88, -54, -120, 123, -25, 33, 114, 81, 24, 67, 113, 41,
        105, -126, 100, 75, -90, -57, -69, 75, 96, 111, 88, 113, -18, -94,
        63, 62, -105, 5, 57, -22, -34, -81, 57, -84, -121, -39, 65, 122, 100,
        -75, -22, -69, -92, -61, 14, -36, 126, 86, -41, 115, -25, 64, 44, 31,
        -124, -7, -12, -117, 83, 109, -125, -86, -46, 79, 114, -37, 107, 96,
        -14, -83, 101, 53, 120, 27, 52, 46, -84, -96, -21, 93, 44, 2, 33, 21,
        89, -93, -128, -31};
}
