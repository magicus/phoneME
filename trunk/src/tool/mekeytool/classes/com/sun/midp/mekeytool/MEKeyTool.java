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

package com.sun.midp.mekeytool;

import java.util.*;
import java.io.*;

import java.security.*;
import java.security.cert.*;
import java.security.interfaces.RSAPublicKey;
import java.math.BigInteger;

import com.sun.midp.publickeystore.*;

/**
 * Manages the initial public keystore needed to bootstrap this MIDP
 * security implementation. It provides both a Java and a command line interface.
 * <p>
 * The anchor of trust on an ME (mobile equipment) are the public keys
 * loaded on it by the manufacturer, in MIDP implementation this is known
 * as the <i>ME keystore</i>. This tool does for the MIDP implementation 
 * what the manufacturer must do for the ME so that trusted MIDP 
 * applications can be authenticated.
 * @see #main(String[])
 */
public class MEKeyTool {
    /** default MIDP application directory, see Utility.c getStorageRoot() */
    private final static String defaultAppDir = "appdb";
    
    /** default ME Keystore can be set via this property */
    private static final String DEFAULT_KEYSTORE_PROPERTY = 
                                                        "default.keystore";
    
    /** default keystore can be set via this property */
    private static final String DEFAULT_MEKEYSTORE_PROPERTY = 
                                                        "default.mekeystore";

    /** default ME keystore filename, see com.sun.midp.Main.java */
    private final static String defaultKeystoreFilename = "_main.ks";

    /**
     * Maps byte codes that follow id-at (0x55 0x04) to corresponding name
     * component tags (e.g. Common Name, or CN, is 0x55, 0x04, 0x03 and
     * Country, or C, is 0x55, 0x04, 0x06). See getName. See X.520 for
     * the OIDs and RFC 1779 for the printable labels. Place holders for
     * unknown labels have a -1 as the first byte.
     */
    private static final String[] AttrLabel = {
        null,
        null,
        null,
        "CN",     // Common name: id-at 3
        "SN",     // Surname: id-at 4
        null,
        "C",      // Country: id-at 6
        "L",      // Locality: id-at 7
        "ST",      // State or province: id-at 8
        "STREET", // Street address: id-at 9
        "O",      // Organization: id-at 10
        "OU",     // Organization unit: id-at 11
    };

    /** Email attribute label. */
    private static final String EMAIL_ATTR_LABEL = "EmailAddress";

    /** Email attribute object identifier. */
    private static final byte[] EMAIL_ATTR_OID = {
        (byte)0x2a, (byte)0x86, (byte)0x48, (byte)0x86, (byte)0xf7, 
        (byte)0x0d, (byte)0x01, (byte)0x09, (byte)0x01
    };

    /** read-writable ME keystore that does not depend on SSL */
    private PublicKeyStoreBuilderBase keystore;

    /** the state for getFirstKey and getNextKey */
    private int nextKeyToGet;

    /**
     * Performs the command specified in the first argument.
     * <p>
     * Exits with a 0 status if the command was successful.
     * Exits and prints out an error message with a -1 status if the command
     * failed.</p>
     * <p><pre>
     *MEKeyTool supports the following commands:
     *
     *  no args  - same has -help
     *  -import  - import a public key from a JCE keystore
     *              into a ME keystore
     *  -delete  - delete a key from a ME keystore
     *  -help    - print a usage summary
     *  -list    - list the owner and validity period of each
     *              key in a ME keystore
     *
     *Parameters for (commands):
     *
     *  -MEkeystore &lt;filename of the ME keystore&gt; (optional for all)
     *  -keystore   &lt;filename of the JCA keystore&gt; (optional import)
     *  -storepass  &lt;password for the JCA keystore&gt; (optional import)
     *  -alias      &lt;short string ID of a key in a JCA keystore&gt; (import)
     *  -domain     &lt;security domain of the ME key&gt; (optional import)
     *  -owner      &lt;name of the owner of a ME key&gt; (delete)
     *  -number     &lt;key number starting a 1 of a ME key&gt; (delete)
     *
     *Defaults:
     *
     *  -MEkeystore appdir/main.ks or set via -Ddefault.mekeystore=
     *  -keystore   &lt;user's home dir&gt;/.keystore or set via 
     * -Ddefault.keystore=
     *  -domain     untrusted
     * </pre>
     * @param args command line arguments
     */
    public static void main(String[] args) {
        File meKeystoreFile = null;

        if (args.length == 0) {
            System.out.println("\n  Error: No command given");
            displayUsage();
            System.exit(-1);
        }

        if (args[0].equals("-help")) {
            // user just needs help with the arguments
            displayUsage();
            System.exit(0);
        }


        // start with the default keystore file
        String defaultMeKeystore = 
                System.getProperty(DEFAULT_MEKEYSTORE_PROPERTY);
        if (defaultMeKeystore != null) {
            meKeystoreFile = new File(defaultMeKeystore);
        } else {
            meKeystoreFile = new File(defaultAppDir, defaultKeystoreFilename);
        }

        try {
            if (args[0].equals("-import")) {
                importCommand(meKeystoreFile, args);
                System.exit(0);
            }
            
            if (args[0].equals("-delete")) {
                deleteCommand(meKeystoreFile, args);
                System.exit(0);
            }

            if (args[0].equals("-list")) {
                listCommand(meKeystoreFile, args);
                System.exit(0);
            }

            throw new UsageException("  Invalid command: " + args[0]);
        } catch (Exception e) {
            System.out.println("\n  Error: " + e.getMessage());

            if (e instanceof UsageException) {
                displayUsage();
            }

            System.exit(-1);
        }
    }

    /**
     * Display the usage text to standard output.
     */
    private static void displayUsage() {
        System.out.println("\n  MEKeyTool argument combinations:\n\n" +
            "    -help\n" +
            "    -import [-MEkeystore <filename>] " +
            "[-keystore <filename>]\n" +
            "            [-storepass <password>] -alias <key alias> " +
            "[-domain <domain>]\n" +
            "    -list [-MEkeystore <filename>]\n" +
            "    -delete [-MEkeystore <filename>]\n" +
            "            (-owner <owner name> | -number <key number>)\n" +
            "\n" +
            "  The default for -MEkeystore is \"" + 
            System.getProperty(DEFAULT_MEKEYSTORE_PROPERTY, "appdb/_main.ks") +
            "\".\n" +
            "  The default for -keystore is \"" + 
            System.getProperty(DEFAULT_KEYSTORE_PROPERTY, "$HOME/.keystore") + 
            "\".\n");
    }

    /**
     * Process the command line arguments for the import command and
     * then imports a public key from a JCA keystore to ME keystore.
     * This method assumes the first argument is the import command
     * and skips it.
     * @param meKeystoreFile ME keystore abstract file name
     * @param args command line arguments
     * @exception Exception if an unrecoverable error occurs
     */
    private static void importCommand(File meKeystoreFile, String[] args)
            throws Exception {
        String jcaKeystoreFilename = null;
        String keystorePassword = null;
        String alias = null;
        String domain = "identified";
        MEKeyTool keyTool;

        for (int i = 1; i < args.length; i++) {
            try {
                if (args[i].equals("-MEkeystore")) {
                    i++;
                    meKeystoreFile = new File(args[i]); 
                } else if (args[i].equals("-keystore")) {
                    i++;
                    jcaKeystoreFilename = args[i]; 
                } else if (args[i].equals("-storepass")) {
                    i++;
                    keystorePassword = args[i]; 
                } else if (args[i].equals("-alias")) {
                    i++;
                    alias = args[i];
                } else if (args[i].equals("-domain")) {
                    i++;
                    domain = args[i];
                } else {
                    throw new UsageException(
                        "Invalid argument for import command: " + args[i]);
                }
            } catch (ArrayIndexOutOfBoundsException e) {
                throw new UsageException("Missing value for " + args[--i]);
            }
        }

        if (jcaKeystoreFilename == null) {
            jcaKeystoreFilename = System.getProperty(
                    DEFAULT_KEYSTORE_PROPERTY, 
                    System.getProperty("user.home") + 
                    File.separator + ".keystore");
        }
        
        if (alias == null) {
            throw new Exception("J2SE key alias was not given");
        }

        try {
            keyTool = new MEKeyTool(meKeystoreFile);
        } catch (FileNotFoundException fnfe) {
            keyTool = new MEKeyTool();
        }

        keyTool.importKeyFromJcaKeystore(jcaKeystoreFilename,
                                      keystorePassword,
                                      alias, domain);
        keyTool.saveKeystore(meKeystoreFile);
    }

    /**
     * Process the command line arguments for the delete command and
     * then delete a public key from a ME keystore.
     * This method assumes the first argument is the delete command
     * and skips it.
     * @param meKeystoreFile ME keystore abstract file name
     * @param args command line arguments
     * @exception Exception if an unrecoverable error occurs
     */
    private static void deleteCommand(File meKeystoreFile, String[] args)
            throws Exception {
        String owner = null;
        int keyNumber = -1;
        boolean keyNumberGiven = false;
        MEKeyTool keyTool;

        for (int i = 1; i < args.length; i++) {
            try {
                if (args[i].equals("-MEkeystore")) {
                    i++;
                    meKeystoreFile = new File(args[i]); 
                } else if (args[i].equals("-owner")) {
                    i++;
                    owner = args[i];
                } else if (args[i].equals("-number")) {
                    keyNumberGiven = true;
                    i++;
                    try {
                        keyNumber = Integer.parseInt(args[i]);
                    } catch (NumberFormatException e) {
                        throw new UsageException(
                            "Invalid number for the -number argument: " +
                            args[i]);
                    }
                } else {
                    throw new UsageException(
                        "Invalid argument for the delete command: " + args[i]);
                }
            } catch (ArrayIndexOutOfBoundsException e) {
                throw new UsageException("Missing value for " + args[--i]);
            }
        }

        if (owner == null && !keyNumberGiven) {
            throw new UsageException(
                "Neither key -owner or -number was not given");
        }

        if (owner != null && keyNumberGiven) {
            throw new UsageException("-owner and -number cannot be used " +
                "together");
        }

        keyTool = new MEKeyTool(meKeystoreFile);

        if (owner != null) {
            if (!keyTool.deleteKey(owner)) {
                throw new UsageException("Key not found for: " + owner);
            }
        } else {
            try {
                keyTool.deleteKey(keyNumber - 1);
            } catch (ArrayIndexOutOfBoundsException e) {
                throw new UsageException("Invalid number for the -number " +
                                    "delete option: " + keyNumber);
            }                
        }

        keyTool.saveKeystore(meKeystoreFile);
    }

    /**
     * Process the command line arguments for the list command and
     * then list the public keys of a ME keystore.
     * This method assumes the first argument is the list command
     * and skips it.
     * @param meKeystoreFile ME keystore abstract file name
     * @param args command line arguments
     * @exception Exception if an unrecoverable error occurs
     */
    private static void listCommand(File meKeystoreFile, String[] args)
            throws Exception {
        MEKeyTool keyTool;
        PublicKeyInfo key;

        for (int i = 1; i < args.length; i++) {
            try {
                if (args[i].equals("-MEkeystore")) {
                    i++;
                    meKeystoreFile = new File(args[i]); 
                } else {
                    throw new UsageException("Invalid argument for the list " +
                                             "command: " + args[i]);
                }
            } catch (ArrayIndexOutOfBoundsException e) {
                throw new UsageException("Missing value for " + args[--i]);
            }
        }

        keyTool = new MEKeyTool(meKeystoreFile);
        key = keyTool.getFirstKey();
        for (int i = 1; key != null; i++) {
            System.out.println("Key " + Integer.toString(i));
            System.out.println(formatKeyInfo(key));
            key = keyTool.getNextKey();
        }

        System.out.println("");
    }

    /**
     * Constructs a MEKeyTool with an empty keystore.
     */
    public MEKeyTool() {
        keystore = new PublicKeyStoreBuilderBase();
    }

    /**
     * Constructs a MEKeyTool and loads its keystore using a filename.
     * @param meKeystoreFilename serialized keystore file
     * @exception FileNotFoundException if the file does not exist, is a
     * directory rather than a regular file, or for some other reason
     * cannot be opened for reading.
     * @exception IOException if the key storage was corrupted
     */
    public MEKeyTool(String meKeystoreFilename)
        throws FileNotFoundException, IOException {

        FileInputStream input;

        input = new FileInputStream(new File(meKeystoreFilename));

        try {
            keystore = new PublicKeyStoreBuilderBase(input);
        } finally {
            input.close();
        }
    };

    /**
     * Constructs a MEKeyTool and loads its keystore from a file.
     * @param meKeystoreFile serialized keystore file
     * @exception FileNotFoundException if the file does not exist, is a
     * directory rather than a regular file, or for some other reason
     * cannot be opened for reading.
     * @exception IOException if the key storage was corrupted
     */
    public MEKeyTool(File meKeystoreFile)
        throws FileNotFoundException, IOException {

        FileInputStream input;

        input = new FileInputStream(meKeystoreFile);

        try {
            keystore = new PublicKeyStoreBuilderBase(input);
        } finally {
            input.close();
        }
    };

    /**
     * Constructs a MEKeyTool and loads its keystore from a stream.
     * @param meKeystoreStream serialized keystore stream
     * @exception IOException if the key storage was corrupted
     */
    public MEKeyTool(InputStream meKeystoreStream)
            throws IOException {
        keystore = new PublicKeyStoreBuilderBase(meKeystoreStream);
    };
    
    /**
     * Copies a key from a Standard Edition keystore into the ME keystore.
     * @param jcakeystoreFilename name of the serialized keystore
     * @param keystorePassword password to unlock the keystore
     * @param alias the ID of the key in the SE keystore
     * @param domain security domain of any application authorized
     *               with the corresponding private key
     */
    public void importKeyFromJcaKeystore(String jcakeystoreFilename,
                                      String keystorePassword,
                                      String alias, String domain)
            throws IOException, GeneralSecurityException {
        FileInputStream keystoreStream;
        KeyStore jcaKeystore;

         // Load the keystore
        keystoreStream = new FileInputStream(new File(jcakeystoreFilename));

        try {
            jcaKeystore = KeyStore.getInstance(KeyStore.getDefaultType());

            if (keystorePassword == null) {
                jcaKeystore.load(keystoreStream, null);
            } else {
                jcaKeystore.load(keystoreStream,
                                 keystorePassword.toCharArray());
            }
        } finally {
            keystoreStream.close();
        }

        importKeyFromJcaKeystore(jcaKeystore,
                                 alias,
                                 domain);
    }

    /**
     * Copies a key from a Standard Edition keystore into the ME keystore.
     * @param jcaKeystore loaded JCA keystore
     * @param alias the ID of the key in the SE keystore
     * @param domain security domain of any application authorized
     *               with the corresponding private key
     */
    public void importKeyFromJcaKeystore(KeyStore jcaKeystore,
                                         String alias, String domain)
            throws IOException, GeneralSecurityException {
        X509Certificate cert;
        byte[] der;
        TLV tbsCert;
        TLV subjectName;
        RSAPublicKey rsaKey;
        String owner;
        long notBefore;
        long notAfter;
        byte[] rawModulus;
        int i;
        int keyLen;
        byte[] modulus;
        byte[] exponent;
        Vector keys;

        // get the cert from the keystore
        try {
            cert = (X509Certificate)jcaKeystore.getCertificate(alias);
        } catch (ClassCastException cce) {
            throw new CertificateException("Certificate not X.509 type");
        }

        if (cert == null) {
            throw new CertificateException("Certificate not found");
        }

        /*
         * J2SE reorders the attributes when building a printable name
         * so we must build a printable name on our own.
         */

        /*
         * TBSCertificate  ::=  SEQUENCE  {
         *   version         [0]  EXPLICIT Version DEFAULT v1,
         *   serialNumber         CertificateSerialNumber,
         *   signature            AlgorithmIdentifier,
         *   issuer               Name,
         *   validity             Validity,
         *   subject              Name,
         *   subjectPublicKeyInfo SubjectPublicKeyInfo,
         *   issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
         *                        -- If present, version shall be v2 or v3
         *   subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
         *                        -- If present, version shall be v2 or v3
         *   extensions      [3]  EXPLICIT Extensions OPTIONAL
         *                        -- If present, version shall be v3
         * }
         */
        der = cert.getTBSCertificate();
        tbsCert = new TLV(der, 0);

        // walk down the tree of TLVs to find the subject name
        try {
            // Top level a is Sequence, drop down to the first child
            subjectName = tbsCert.child;

            // skip the version if present.
            if (subjectName.type == TLV.VERSION_TYPE) {
                subjectName = subjectName.next;
            }

            // skip the serial number
            subjectName = subjectName.next;
            
            // skip the signature alg. id.
            subjectName = subjectName.next;
            
            // skip the issuer
            subjectName = subjectName.next;
            
            // skip the validity
            subjectName = subjectName.next;

            owner = parseDN(der, subjectName);
        } catch (NullPointerException e) {
            throw new CertificateException("TBSCertificate corrupt 1");
        } catch (IndexOutOfBoundsException e) {
            throw new CertificateException("TBSCertificate corrupt 2");
        }

        notBefore = cert.getNotBefore().getTime();
        notAfter = cert.getNotAfter().getTime();

        // get the key from the cert
        try {
            rsaKey = (RSAPublicKey)cert.getPublicKey();
        } catch (ClassCastException cce) {
            throw new RuntimeException("Key in certificate is not an RSA key");
        }

        // get the key parameters from the key
        rawModulus = rsaKey.getModulus().toByteArray();

        /*
         * the modulus is given as the minimum positive integer,
         * will not padded to the bit size of the key, or may have a extra
         * pad to make it positive. SSL expects the key to be signature
         * bit size. but we cannot get that from the key, so we should
         * remove any zero pad bytes and then pad out to a multiple of 8 bytes
         */
        for (i = 0; i < rawModulus.length && rawModulus[i] == 0; i++);

        keyLen = rawModulus.length - i;
        keyLen = (keyLen + 7) / 8 * 8;
        modulus = new byte[keyLen];

        int k, j;
        for (k = rawModulus.length - 1, j = keyLen - 1;
                 k >= 0 && j >= 0; k--, j--) {
            modulus[j] = rawModulus[k];
        }

        exponent = rsaKey.getPublicExponent().toByteArray();

        // add the key
        keys = keystore.findKeys(owner);
        if (keys != null) {
            boolean duplicateKey = false;

            for (int n = 0; !duplicateKey && n < keys.size(); n++) {
                PublicKeyInfo key = (PublicKeyInfo)keys.elementAt(n);

                if (key.getOwner().equals(owner)) {
                    byte[] temp = key.getModulus();

                    if (modulus.length == temp.length) {
                        duplicateKey = true;
                        for (int m = 0; j < modulus.length && m < temp.length;
                                 m++) {
                            if (modulus[m] != temp[m]) {
                                duplicateKey = false;
                                break;
                            }
                        }
                    }
                }
            }
                
            if (duplicateKey) {
                throw new CertificateException(
                    "Owner already has this key in the ME keystore");
            }
        }
                 
        keystore.addKey(new PublicKeyInfo(owner, notBefore, notAfter,
                                          modulus, exponent, domain));
    }

    /**
     * Deletes the first public key matching the owner's distinguished name.
     * @param owner name of the key's owner
     * @return true, if the key was deleted, else false
     */
    public boolean deleteKey(String owner) {
        PublicKeyInfo key;

        for (int i = 0; i < keystore.numberOfKeys(); i++) {
            key = keystore.getKey(i);
            if (key.getOwner().equals(owner)) {
                keystore.deleteKey(i);
                return true;
            }
        }

        return false;
    };

    /**
     * Deletes a key by key number, 0 being the first public key.
     *
     * @param number number of the key
     *
     * @exception  ArrayIndexOutOfBoundsException  if an invalid number was
     *             given.
     */
    public void deleteKey(int number) {
        keystore.deleteKey(number);
    };

    /**
     * Gets the first key in the keystore.
     * @return all the information related to the first key
     */
    protected PublicKeyInfo getFirstKey() {
        nextKeyToGet = 0;
        return getNextKey();
    };

    /**
     * Gets the next key after the previous one returned by
     * {@link #getFirstKey} or this method. If getFirstKey is not called
     * before the first call to this method, null will be returned.
     * @return all the information related to the next key, or null if
     *        there are no more keys
     */
    protected PublicKeyInfo getNextKey() {
        PublicKeyInfo key;

        try {
            key = keystore.getKey(nextKeyToGet);
        } catch (ArrayIndexOutOfBoundsException e) {
            return null;
        }

        nextKeyToGet++;

        return key;
    };

    /**
     * Saves the keystore to a file.
     * @param meKeystoreFile serialized keystore file
     */
    public void saveKeystore(File meKeystoreFile) throws IOException {
        FileOutputStream output;

        output = new FileOutputStream(meKeystoreFile);

        keystore.serialize(output);
        output.close();
    }

    /**
     * Gets the read-write keystore this tool is manipulating.
     * For advanced users.
     * @return read-write keystore 
     */
    public PublicKeyStoreBuilderBase getKeystore() {
        return keystore;
    }

    /**
     * Creates a string representation of a key that is displayed to a
     * user during a list command. The string does not include the modulus
     * and exponent.
     * @param keyInfo key to display
     * @return printable representation of the key
     */
    public static String formatKeyInfo(PublicKeyInfo keyInfo) {
        return "  Owner: " + keyInfo.getOwner() +
            "\n  Valid from " +
            (new Date(keyInfo.getNotBefore())).toString() +
            " to " + (new Date(keyInfo.getNotAfter())).toString() +
            "\n  Security Domain: " + keyInfo.getDomain() +
            "\n  Enabled: " + keyInfo.isEnabled();
    };

    /**
     * Parses a DER TLV tree into a printable distinguished name.
     *
     * @param buffer DER buffer
     * @param dn sequence of TLV nodes.
     *
     * @return printable name.
     *
     * @exception NullPointerException if the name is corrupt
     * @exception IndexOutOfBoundsException if the name is corrupt
     */
    private String parseDN(byte[] buffer, TLV dn) {
        TLV attribute;
        TLV type;
        TLV value;
        StringBuffer name = new StringBuffer(256);

        /*
         * Name ::= CHOICE { RDNSequence } # CHOICE does not encoded
         *
         * RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
         *
         * RelativeDistinguishedName ::= SET OF AttributeTypeAndValue
         *
         *   AttributeTypeAndValue ::= SEQUENCE {
         *     type     AttributeType,
         *     value    AttributeValue }
         *
         * AttributeType ::= OBJECT IDENTIFIER
         *
         * AttributeValue ::= ANY DEFINED BY AttributeType
         *
         * basically this means that each attribute value is 3 levels down
         */

        // sequence drop down a level
        attribute = dn.child;
        
        while (attribute != null) {
            if (attribute != dn.child) {
                name.append(";");
            }

            /*
             * we do not handle relative distinguished names yet
             * which should not be used by CAs anyway
             * so only take the first element of the sequence
             */

            type = attribute.child.child;

            /*
             * At this point we tag the name component, e.g. C= or hex
             * if unknown.
             */
            if ((type.length == 3) && (buffer[type.valueOffset] == 0x55) &&
                    (buffer[type.valueOffset + 1] == 0x04)) {
                // begins with id-at, so try to see if we have a label
                int temp = buffer[type.valueOffset + 2] & 0xFF;
                if ((temp < AttrLabel.length) &&
                        (AttrLabel[temp] != null)) {
                    name.append(AttrLabel[temp]);
                } else {
                    name.append(TLV.hexEncode(buffer, type.valueOffset,
                                              type.length, -1));
                }
            } else if (TLV.byteMatch(buffer, type.valueOffset,
                                     type.length, EMAIL_ATTR_OID,
                                     0, EMAIL_ATTR_OID.length)) {
                name.append(EMAIL_ATTR_LABEL);
            } else {
                name.append(TLV.hexEncode(buffer, type.valueOffset,
                                          type.length, -1));
            }

            name.append("=");

            value = attribute.child.child.next;
            if (value.type == TLV.PRINTSTR_TYPE ||
                    value.type == TLV.TELETEXSTR_TYPE ||
                    value.type == TLV.UTF8STR_TYPE ||
                    value.type == TLV.IA5STR_TYPE ||
                    value.type == TLV.UNIVSTR_TYPE) {
                try {
                    name.append(new String(buffer, value.valueOffset,
                                           value.length, "UTF-8"));
                } catch (UnsupportedEncodingException e) {
                    throw new RuntimeException(e.toString());
                }
            } else {
                name.append(TLV.hexEncode(buffer, value.valueOffset,
                                          value.length, -1));
            }

            attribute = attribute.next;
        }

        return name.toString();
    }
}

/**
 * Used to represent each Type, Length, Value structure in a DER buffer.
 */
class TLV {
    /** ASN context specific flag used in types (0x80). */
    static final int CONTEXT = 0x80;
    /** ASN constructed flag used in types (0x20). */
    static final int CONSTRUCTED = 0x20;
    /** ASN constructed flag used in types (0x20). */
    static final int EXPLICIT = CONSTRUCTED;

    /** ANY_STRING type used as a place holder. [UNIVERSAL 0] */
    static final int ANY_STRING_TYPE = 0x00; // our own impl

    /** ASN BOOLEAN type used in certificate parsing. [UNIVERSAL 1] */
    static final int BOOLEAN_TYPE    = 1;
    /** ASN INTEGER type used in certificate parsing. [UNIVERSAL 2] */
    static final int INTEGER_TYPE    = 2;
    /** ASN BIT STRING type used in certificate parsing. [UNIVERSAL 3] */
    static final int BITSTRING_TYPE  = 3;
    /** ASN OCTET STRING type used in certificate parsing. [UNIVERSAL 4] */
    static final int OCTETSTR_TYPE   = 4;
    /** ASN NULL type used in certificate parsing. [UNIVERSAL 5] */
    static final int NULL_TYPE       = 5;
    /** ASN OBJECT ID type used in certificate parsing. [UNIVERSAL 6] */
    static final int OID_TYPE        = 6;
    /** ASN UTF8String type used in certificate parsing. [UNIVERSAL 12] */
    static final int UTF8STR_TYPE    = 12;
    /**
     * ASN SEQUENCE type used in certificate parsing.
     * [UNIVERSAL CONSTRUCTED 16]
     */
    static final int SEQUENCE_TYPE   = CONSTRUCTED + 16;
    /**
     * ASN SET type used in certificate parsing.
     * [UNIVERSAL CONSTRUCTED 17]
     */
    static final int SET_TYPE        = CONSTRUCTED + 17;
    /** ASN PrintableString type used in certificate parsing. [UNIVERSAL 19] */
    static final int PRINTSTR_TYPE   = 19;
    /** ASN TELETEX STRING type used in certificate parsing. [UNIVERSAL 20] */
    static final int TELETEXSTR_TYPE = 20;
    /** ASN IA5 STRING type used in certificate parsing. [UNIVERSAL 22] */
    static final int IA5STR_TYPE     = 22;  // Used for EmailAddress
    /** ASN UCT time type used in certificate parsing [UNIVERSAL 23] */
    static final int UCT_TIME_TYPE   = 23;
    /**
     * ASN Generalized time type used in certificate parsing.
     * [UNIVERSAL 24]
     */
    static final int GEN_TIME_TYPE   = 24;
    /**
     * ASN UniversalString type used in certificate parsing.
     * [UNIVERSAL 28].
     */
    static final int UNIVSTR_TYPE    = 28;
    /** ASN BIT STRING type used in certificate parsing. [UNIVERSAL 30] */
    static final int BMPSTR_TYPE  = 30;

    /**
     * Context specific explicit type for certificate version. 
     * [CONTEXT EXPLICIT 0]
     */
    static final int VERSION_TYPE    = CONTEXT + EXPLICIT + 0;
    /**
     * Context specific explicit type for certificate extensions.
     * [CONTEXT EXPLICIT 3]
     */
    static final int EXTENSIONS_TYPE = CONTEXT + EXPLICIT + 3;


    /**
     * Checks if two byte arrays match.
     * <P />
     * @param a first byte array
     * @param aOff starting offset for comparison within a
     * @param aLen number of bytes of a to be compared
     * @param b second byte array
     * @param bOff starting offset for comparison within b
     * @param bLen number of bytes of b to be compared
     * @return true if aLen == bLen and the sequence of aLen bytes in a
     * starting at
     * aOff matches those in b starting at bOff, false otherwise
     */ 
    static boolean byteMatch(byte[] a, int aOff, int aLen,
                             byte[] b, int bOff, int bLen) {
        if ((aLen != bLen) || (a.length < aOff + aLen) ||
                (b.length < bOff + bLen)) {
            return false;
        }
        
        for (int i = 0; i < aLen; i++) {
            if (a[i + aOff] != b[i + bOff]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Converts a subsequence of bytes into a printable OID,
     * a string of decimal digits, each separated by a ".". 
     *
     * @param buffer byte array containing the bytes to be converted
     * @param offset starting offset of the byte subsequence inside b
     * @param length number of bytes to be converted
     *
     * @return printable OID
     */ 
    static String OIDtoString(byte[] buffer, int offset, int length) {
        StringBuffer result;
        int end;
        int t;
        int x;
        int y;

        if (length == 0) {
            return "";
        }

        result = new StringBuffer(40);

        end = offset + length;

        /*
         * first byte (t) always represents the first 2 values (x, y).
         * t = (x * 40) + y;
         */
        t = buffer[offset++] & 0xff;
        x = t / 40;
        y = t - (x * 40);

        result.append(x);
        result.append('.');
        result.append(y);

        x = 0;
        while (offset < end) {
            // 7 bit per byte, bit 8 = 0 means the end of a value
            x = x << 7;

            t = buffer[offset++];
            if (t >= 0) {
                x += t;
                result.append('.');
                result.append(x);
                x = 0;
            } else {
                x += t & 0x7f;
            }
        }

        return result.toString();
    }

    /** Hexadecimal digits. */
    static char[] hc = {
        '0', '1', '2', '3', '4', '5', '6', '7', 
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    /**
     * Converts a subsequence of bytes in a byte array into a 
     * corresponding string of hexadecimal digits, each separated by a ":". 
     *
     * @param b byte array containing the bytes to be converted
     * @param off starting offset of the byte subsequence inside b
     * @param len number of bytes to be converted
     * @param max print a single "+" instead of the bytes after max,
     *        -1 for no max.
     * @return a string of corresponding hexadecimal digits or
     * an error string
     */ 
    static String hexEncode(byte[] b, int off, int len, int max) {
        char[] r;
        int v;
        int i;
        int j;
        
        if ((b == null) || (len == 0)) {
            return "";
        }

        if ((off < 0) || (len < 0)) {
            throw new ArrayIndexOutOfBoundsException();
        }

        r = new char[len * 3];

        for (i = 0, j = 0; ; ) {
            v = b[off + i] & 0xff;
            r[j++] = hc[v >>> 4];
            r[j++] = hc[v & 0x0f];

            i++;
            if (i >= len) {
                break;
            }

            if (i == max) {
                r[j++] = ' ';
                r[j++] = '+';
                break;
            }
                
            r[j++] = ':';
        }

        return (new String(r, 0, j));
    }

    /** Raw DER type. */
    int type;
    /** Number of bytes that make up the value. */ 
    int length;
    /** Offset of the value. */
    int valueOffset;
    /** Non-null for constructed types, the first child TLV. */
    TLV child;
    /** The next TLV in the parent sequence. */
    TLV next;

    /**
     * Construct a TLV structure, recursing down for constructed types.
     *
     * @param buffer DER buffer
     * @param offset where to start parsing
     *
     * @exception IndexOutOfBoundException if the DER is corrupt
     */
    TLV(byte[] buffer, int offset) {
        boolean constructed;
        int size;
        
        type = buffer[offset++] & 0xff;

        // recurse for constructed types, bit 6 = 1
        constructed = (type & 0x20) == 0x20;

        if ((type & 0x1f) == 0x1f) {
            // multi byte type, 7 bits per byte, only last byte bit 8 as zero
            type = 0;
            for (; ; ) {
                int temp = buffer[offset++];
                type = type << 7;
                if (temp >= 0) {
                    type += temp;
                    break;
                }

                // strip off bit 8
                temp = temp & 0x7f;
                type += temp;
            }
            
        }

        size = buffer[offset++] & 0xff;
        if (size >= 128) {
            int sizeLen = size - 128;
            
            // NOTE: for now, all sizes must fit int two bytes
            if (sizeLen > 2) {
                throw new RuntimeException("TLV size to large");
            }

            size = 0;
            while (sizeLen > 0) {
                size = (size << 8) + (buffer[offset++] & 0xff);
                sizeLen--;
            }
        }

        length = size;
        valueOffset = offset;

        if (constructed) {
            int end;
            TLV temp;

            end = offset + length;

            child = new TLV(buffer, offset);
            temp = child;
            for (; ; ) {
                offset = temp.valueOffset + temp.length;
                if (offset >= end) {
                    break;
                }

                temp.next = new TLV(buffer, offset);
                temp = temp.next;
            }
        }
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     *
     * @param buffer DER buffer
     */
    void print(byte[] buffer) {
        print(buffer, 0);
    }

    /**
     * Print the a TLV structure, recursing down for constructed types.
     *
     * @param buffer DER buffer
     * @param level what level this TLV is at
     */
    private void print(byte[] buffer, int level) {
        for (int i = 0; i < level; i++) {
            System.out.print(' ');
        }

        if (child == null) {
            System.out.print("Type: 0x" + Integer.toHexString(type) +
                             " length: " + length + " value: ");
            if (type == PRINTSTR_TYPE ||
                type == TELETEXSTR_TYPE ||
                type == UTF8STR_TYPE ||
                type == IA5STR_TYPE ||
                type == UNIVSTR_TYPE) {
                try {
                    System.out.print(new String(buffer, valueOffset, length,
                                                "UTF-8"));
                } catch (UnsupportedEncodingException e) {
                    // ignore
                }
            } else if (type == OID_TYPE) {
                System.out.print(OIDtoString(buffer, valueOffset, length));
            } else {
                System.out.print(hexEncode(buffer, valueOffset, length, 14));
            }

            System.out.println("");
        } else {
            if (type == SET_TYPE) {
                System.out.println("Set:");
            } else if (type == VERSION_TYPE) {
                System.out.println("Version (explicit):");
            } else if (type == EXTENSIONS_TYPE) {
                System.out.println("Extensions (explicit):");
            } else {
                System.out.println("Sequence:");
            }

            child.print(buffer, level + 1);
        }

        if (next != null) {
            next.print(buffer, level);
        }
    }
}

/**
 * This exception is used to signal a usage error.
 */
class UsageException extends Exception {

    /**
     * Constructs a UsageException.
     */
    UsageException() {
    }

    /**
     * Constructs a UsageException.
     *
     * @param message exception message
     */
    UsageException(String message) {
        super(message);
    }
}
