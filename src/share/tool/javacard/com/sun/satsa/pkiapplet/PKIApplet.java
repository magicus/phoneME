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

package com.sun.satsa.pkiapplet;

import javacard.framework.*;
import javacard.security.*;
import javacardx.crypto.Cipher;

/**
 * Card side application for PKI implementation. Supports subset of
 * WIM functionality.
 */
public class PKIApplet extends Applet {

    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x0 = 0;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x1 = 1;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x2 = 2;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x3 = 3;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x4 = 4;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x5 = 5;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x6 = 6;
    /** Constant that is used to avoid '(short) x' notation. */
    static final byte x8 = 8;

    /** INS byte for command APDU. */
    static final byte INS_VERIFY    = (byte) 0x20;
    /** INS byte for command APDU. */
    static final byte INS_SELECT    = (byte) 0xa4;
    /** INS byte for command APDU. */
    static final byte INS_READ      = (byte) 0xb0;
    /** INS byte for command APDU. */
    static final byte INS_UPDATE    = (byte) 0xd6;
    /** INS byte for command APDU. */
    static final byte INS_MSE       = (byte) 0x22;
    /** INS byte for command APDU. */
    static final byte INS_PSO       = (byte) 0x2a;

    /** INS byte for command APDU. */
    static final byte INS_NEW       = (byte) 0xBC;

    /** DigestInfo structure size for RSA signature. */
    static final short digestLength = 35;
    /** Temporaru buffer for RSA signature. */
    static byte[] signBuffer = new byte[digestLength];

    /** If false, applet always report that PINs are validated. */
    static boolean verifyPINs = true;

    /** If false, key generation is disabled. */
    static boolean supportKeyGeneration = true;


    /** PIN identifiers. */
    byte[] PIN_REFs;
    /** PIN objects. */
    OwnerPIN[] PINs;
    /** Private keys. */
    PrivateKey[] keys;
    /** Root DF for entire file structure. */
    DFile top;
    /**
     * Root DF for WIM application. All relative paths start from
     * here. */
    DFile base;
    /** Currently selected file. */
    File current;
    /** Flag that indicates that SE is restored. */
    boolean isSERestored;
    /** Flag that indicates that private key path was set properly. */
    boolean isKeyFileSet;
    /** Private key number for signature generation. */
    short keyNum;
    /** Cipher object. */
    Cipher cipher;
    /** MessageDigest object for key hash calculation. */
    MessageDigest digest;
    /** Number of unused keys. */
    static short unusedKeys = 0;

    /** Constructor. */
    PKIApplet() {

        if (Data.PINs == null) {
            CardRuntimeException.throwIt(x0);
        }
        cipher = Cipher.getInstance(Cipher.ALG_RSA_PKCS1, false);
        if (supportKeyGeneration) {
            digest = MessageDigest.getInstance(MessageDigest.ALG_SHA,
                        false);
        }
        register();
    }

    /**
     * To create an instance of the Applet subclass, the JCRE will call
     * this static method first.
     * @param bArray the array containing installation parameters
     * @param bOffset the starting offset in bArray
     * @param bLength the length in bytes of the parameter data in bArray
     */
    public static void install(byte[] bArray, short bOffset,
                               byte bLength) {
        new PKIApplet();
    }

    /**
     * Called by the JCRE to inform this applet that it has been
     * selected. When invoked first time initialises the file system.
     * @return true
     */
    public boolean select() {

        if (Data.PINs != null) {
            init();
        }
        current = base;
        isSERestored = false;
        return true;
    }

    /**
     * Called by the JCRE to inform this applet that it has been
     * deselected. When invoked PIN-G should be reset.
     */
    public void deselect() {
        if (PINs != null) {
            PINs[0].reset();
        }
    }
    
    /**
     * Initialises the WIM data structures.
     */
    void init() {

        Parser.init(Data.PINs);
        short cnt = Parser.getByte();

        PIN_REFs = new byte[(short) (cnt + Data.freePINSlots)];
        PINs = new OwnerPIN[(short) (cnt + Data.freePINSlots)];

        for (short i = 0; i < cnt; i++) {

            PIN_REFs[i] = Parser.getByte();
            byte len = Parser.getByte();
            PINs[i] = new OwnerPIN(x3, x8);
            PINs[i].update(Data.PINs, Parser.offset, len);
            Parser.skip(len);
        }

        Parser.init(Data.PrivateKeys);
        cnt = Parser.getByte();
        keys = new PrivateKey[(short) (cnt + Data.freeKeySlots)];
        short keyPos = 0;
        // calculate start of first key in a file
        short privKeyStart = 
            (short) (Data.PrKDFOffset + Data.newPrivKeyOffset -
                                  Data.privKeyRecordSize * cnt);
        short pubKeyStart = 
            (short) (Data.PuKDFOffset + Data.newPubKeyOffset -
                                  Data.pubKeyRecordSize * cnt);
        for (short i = 0; i < cnt; i++) {
            PrivateKey key = new PrivateKey(this);
            if (key.value != null) {
                keys[keyPos++] = key;
            } else { 
                // This key is not supported by card
                byte[] files = Data.Files;
                short tail = (short)(cnt + Data.freeKeySlots - keyPos - 1);
                if (tail > 0) {
                    // Shift up private keys
                    short privOffset = 
                        (short)(privKeyStart + 
                                           Data.privKeyRecordSize * keyPos);
                    Util.arrayCopyNonAtomic(files, 
                            (short)(privOffset + Data.privKeyRecordSize), 
                            files, privOffset, 
                            (short)(Data.privKeyRecordSize * tail)); 
                    privOffset = 
                        (short)((privKeyStart + 
                                    Data.privKeyRecordSize * (keyPos + tail)));

                    // Shift up public keys
                    short pubOffset = 
                        (short)(pubKeyStart + 
                                           Data.pubKeyRecordSize * keyPos);
                    Util.arrayCopyNonAtomic(files, 
                            (short)(pubOffset + Data.pubKeyRecordSize), 
                             files, pubOffset, 
                             (short)(Data.pubKeyRecordSize * tail));
                    pubOffset = 
                        (short)((pubKeyStart + 
                                    Data.pubKeyRecordSize * (keyPos + tail)));
                }
            }
        }
        Data.newPrivKeyOffset = (short)(privKeyStart - 
                Data.PrKDFOffset + keyPos * Data.privKeyRecordSize);
        Data.newPubKeyOffset = (short)(pubKeyStart - 
                Data.PuKDFOffset + keyPos * Data.pubKeyRecordSize);

        unusedKeys = (short)(keys.length - Data.freeKeySlots - keyPos);
        Parser.init(Data.Files);
        top = (DFile) readFile(null);

        if (base == null) {
            ISOException.throwIt((short) 0x9001);
        }

        Data.PINs = null;
        Data.PrivateKeys = null;

        // IMPL_NOTE: debug check - remove
        if (Parser.offset != Data.Files.length) {
            ISOException.throwIt((short) 0x9001);
        }
    }

    /**
     * Creates new file object.
     * @param parent parent DF for this file
     * @return the new file object
     */
    File readFile(DFile parent) {

        short id = Parser.getShort();
        short type = Parser.getByte();
        short length = Parser.getShort();

        if ((type & File.DIR) == 0) {

            EFile f;
            if ((type & File.EMPTY) == 0) {
                f = new EFile(parent, id, type, Parser.offset, length,
                              Data.Files);
                Parser.skip(length);
            } else {
                type &= ~File.EMPTY;
                byte[] data = new byte[length];
                short dlen = Parser.getShort();
                Util.arrayCopyNonAtomic(Data.Files, Parser.offset, data,
                        (short) 0, dlen);
                f = new EFile(parent, id, type, (short) 0, length, data);
                Parser.skip(dlen);
            }
            return f;
        }

        DFile f = new DFile(parent, id, type);

        File[] files = new File[length];
        for (short i = 0; i < length; i++) {
            files[i] = readFile(f);
        }

        f.files = files;

        if (type == File.WIM) {
            base = f;
        }

        return f;
    }

    /**
     * Main entry point.
     * @param apdu command APDU
     */
    public void process(APDU apdu) {

        byte[] data = apdu.getBuffer();
        byte CLA = (byte) (data[ISO7816.OFFSET_CLA] & 0xF0);
        byte INS = data[ISO7816.OFFSET_INS];

        if (CLA == 0 &&
            INS == (byte)(0xA4) &&
            data[ISO7816.OFFSET_P1] == 4) {
            return;
        }

        if (CLA != (byte) 0x80) {
            ISOException.throwIt(ISO7816.SW_CLA_NOT_SUPPORTED);
        }
        switch (INS) {

            case INS_SELECT:
                selectFile(apdu);
                return;

            case INS_READ:
                read(apdu);
                return;

            case INS_UPDATE:
                update(apdu);
                return;

            case INS_VERIFY:
                verify(apdu);
                return;

            case INS_MSE:
                manageSE(apdu);
                return;

            case INS_PSO:
                sign(apdu);
                return;

            case INS_NEW:
                if (supportKeyGeneration) {
                    newKey(apdu);
                    return;
                }
                break;
        }
    
        ISOException.throwIt(ISO7816.SW_INS_NOT_SUPPORTED);
    }

    /**
     * Handles SELECT FILE APDU.
     * @param apdu command APDU
     */
    void selectFile(APDU apdu) {

        byte[] data = apdu.getBuffer();

        if (Util.getShort(data, x2) != 0) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        checkDataSize(x2, apdu);

        File f = select(Util.getShort(data, x5));

        if (f == null) {
            ISOException.throwIt(ISO7816.SW_FILE_NOT_FOUND);
        }

        current = f;

        if (current.isDF()) {

            Util.setShort(data, x0, (short) 0x6f00);
            apdu.setOutgoingAndSend(x0, x2);
        } else {

            Util.setShort(data, x0, (short) 0x6f04);
            Util.setShort(data, x2, (short) 0x8002);
            Util.setShort(data, x4, ((EFile) current).length);
            apdu.setOutgoingAndSend(x0, x6);
        }
    }

    /**
     * Selects the file specified by file identifier.
     * @param id file identifier
     * @return selected file
     */
    File select(short id) {

        DFile f;
        if (current.isDF()) {
            f = (DFile) current;
        } else  {
            f = current.parent;
        }

        File x = f.getFile(id);
        if (x != null) {
            return x;
        }

        f = f.parent;

        if (f == null) {
            return null;
        }

        return f.getFile(id);
    }

    /**
     * Handles READ BINARY APDU.
     * @param apdu command APDU
     */
    void read(APDU apdu) {

        if (current.isDF()) {
            ISOException.throwIt(ISO7816.SW_COMMAND_NOT_ALLOWED);
        }

        EFile f = (EFile) current;

        byte[] data = apdu.getBuffer();

        short offset = Util.getShort(data, x2);
        if (offset < 0 || offset > f.length) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        short len = (short) (data[x4] & 0xff);
        if ((short)(offset + len) > f.length) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }

        apdu.setOutgoing();
        apdu.setOutgoingLength(len);
        apdu.sendBytesLong(f.data, (short) (f.offset + offset), len);
    }

    /**
     * Handles UPDATE BINARY apdu.
     * @param apdu command APDU
     */
    void update(APDU apdu) {

        if (current.isDF()) {
            ISOException.throwIt(ISO7816.SW_COMMAND_NOT_ALLOWED);
        }

        EFile f = (EFile) current;

        if (!(f.type == File.UPDATE && isValidated(PINs[0]))) {
            ISOException.throwIt(ISO7816.SW_SECURITY_STATUS_NOT_SATISFIED);
        }

        byte[] data = apdu.getBuffer();

        short offset = Util.getShort(data, x2);
        if (offset < 0) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        short len = (short) (data[x4] & 0xff);

        if ((short)(offset + len) > f.length) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }

        short l = apdu.setIncomingAndReceive();
        short off = 5;

        while (l > 0) {
            Util.arrayCopyNonAtomic(data, off,
                                f.data, (short) (f.offset + offset), l);
            offset += l;
            l = apdu.receiveBytes(x0);
            off = 0;
        }
    }

    /**
     * Handles PIN related APDUs.
     * @param apdu command APDU
     */
    void verify(APDU apdu) {

        if (current.type != File.PIN) {
            ISOException.throwIt(ISO7816.SW_FILE_NOT_FOUND);
        }

        byte[] data = apdu.getBuffer();

        short pin_num = -1;
        for (short i = 0; 
                i < (short)(PIN_REFs.length - Data.freePINSlots); i++) {
            if (PIN_REFs[i] == data[x3]) {
                pin_num = i;
                break;
            }
        }

        if (pin_num == -1 || data[x2] != 0) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        OwnerPIN pin = PINs[pin_num];

        if (data[x4] == 0) {

            if (isValidated(pin)) {
                return;     // SW = 0x9000
            }

            if (pin.getTriesRemaining() == 0) {
                ISOException.throwIt(ISO7816.SW_FILE_INVALID);
            }

            ISOException.throwIt((short) (0x6300 | pin.getTriesRemaining()));
        }

        short len = apdu.setIncomingAndReceive();
        if (len > 8) {  // too long, set to 0 to update PIN
            len = 0;
        }
        pin.check(data, x5, (byte) len);

        if (isValidated(pin)) {
            return;     // SW = 0x9000
        }

        ISOException.throwIt((short) 0x6300);
    }

    /**
     * Hanldes MANAGE SECURITY ENVIRONMENT APDUs.
     * @param apdu command APDU
     */
    void manageSE(APDU apdu) {

        byte[] data = apdu.getBuffer();

        if (data[x2] == (byte) 0xf3) {

            if (data[x3] != Data.WIM_GENERIC_RSA_ID) {
                ISOException.throwIt((short) 0x6600);
            }

            isSERestored = true;
            isKeyFileSet = false;
            keyNum = -1;
            return;
        }

        if (! isSERestored || Util.getShort(data, x2) != (short) 0x41b6) {
            ISOException.throwIt((short) 0x6600);
        }

        File keyFile = current;

        short len = apdu.setIncomingAndReceive();
        short index = 5;
        len += index;

        try {
            while (index < len) {

                byte tag = data[index++];
                byte l = data[index++];

                if (l <= 0 || l > 32) {
                    ISOException.throwIt((short) 0x6600);
                }

                if (tag == (byte) 0x84) {   // private key reference

                    if (l != 1) {
                        ISOException.throwIt((short) 0x6600);
                    }

                    for (short i = 0;
                         i < (short)(keys.length - Data.freeKeySlots - unusedKeys); i++) {

                        if (data[index] == keys[i].id) {
                            keyNum = i;
                            break;
                        }
                    }
                } else
                if (tag == (byte) 0x81) {   // private key DF path
                    keyFile = getFile(data, index, l);
                } else {
                    ISOException.throwIt((short) 0x6A80);   // invalid tag
                }

                // path (id, relative or complete)
                index += l;
            }
        }
        catch (ArrayIndexOutOfBoundsException e) {
            keyNum = -1;
        }

        if (keyNum == -1 ||
            keyFile == null ||
            keyFile.type != File.PrivateKeyFile) {
            ISOException.throwIt((short) 0x6600);
        }

        EFile f = (EFile) keyFile;
        isKeyFileSet = (f.data[f.offset] == keys[keyNum].id);

        if (! isKeyFileSet) {
            ISOException.throwIt((short) 0x6600);
        }
    }

    /**
     * Allocates new key and, if necessary, PIN.
     * @param apdu the command APDU. APDU data contains key type
     * (byte, 0 - authenticatiuon, 1 - non-repudiation), key lenght in
     * bits (2 byte value). For non-repudiation key after that goes
     * initial PIN value (8 bytes) and label (32 bytes). If p1 = 1
     * the command just verifies that a new key can be generated.
     */
    void newKey(APDU apdu) {

        apdu.setIncomingAndReceive();
        byte[] data = apdu.getBuffer();

        boolean nonRepudiation = (data[x5] == 1);

        if (Data.freeKeySlots == 0 ||
            (nonRepudiation && Data.freePINSlots == 0)) {
            ISOException.throwIt((short) 0x9001);
        }

        short keyLen = Util.getShort(data, x6);

        if (keyLen > (short)((data.length - 1) * 8)) {
            ISOException.throwIt((short) 0x9001);
        }
        
        if (data[x2] == 1) {
            Util.setShort(data, x0, (short) 0x1234);
            Util.setShort(data, x2, (short) 0x4321);
            apdu.setOutgoingAndSend(x0, x4);
            return;
        }

/*
 * IMPL_NOTE: For testing purposes return existing key instead of new one
        for (short i = 0; i < (short)(keys.length - Data.freeKeySlots - unusedKeys); i++) {
            if (keys[i].keyLen == keyLen 
                    && keys[i].nonRepudiation == nonRepudiation) {
                data[0] = (byte)keys[i].id;
                apdu.setOutgoingAndSend(x0, x1);
                return;
            }
        }
*/

        try {
            short pinIndex = 0;

            if (nonRepudiation) {
                // new PIN must be allocated
                pinIndex = (short) (PINs.length - Data.freePINSlots);

                OwnerPIN pin = new OwnerPIN((byte) 3, (byte)8);
                pin.update(data, x8, (byte) 8);
                pin.check(data, x8, (byte) 8);
                PINs[pinIndex] = pin;
                PIN_REFs[pinIndex] = Data.newPINRef;

                Util.arrayCopy(data, (short) 16, Data.Files,
                        (short) (Data.AODFOffset + Data.newPINOffset +
                        Data.PINLabelOffset), (short) 32);
            }

            // check if keyLen & RSA algorithm are supported by card 
            KeyPair p = new KeyPair(KeyPair.ALG_RSA, keyLen);
            p.genKeyPair();

            PrivateKey key = new PrivateKey(this, Data.newKeyID, 
                    PINs[pinIndex], 
                    nonRepudiation, keyLen, (RSAPrivateKey) p.getPrivate());

            RSAPublicKey pk = (RSAPublicKey) p.getPublic();

            EFile f = (EFile) base.getFile(Data.newFileID);
            f.data = encodePublicKey(pk, data);
            f.offset = 0;
            f.length = (short) (f.data.length);

            byte[] hash = getKeyHash(pk, data);
            byte[] files = Data.Files;

            short offset = (short) (Data.PuKDFOffset + Data.newPubKeyOffset);
            Util.arrayCopy(hash, x0, files,
                    (short) (offset + Data.pubHashOffset), (short) 20);

            Util.setShort(files, (short) (offset + Data.pubKeyLengthOffset),
                    keyLen);

            offset = (short) (Data.PrKDFOffset + Data.newPrivKeyOffset);

            files[(short) (offset + Data.privPINIDOffset)] =
                    nonRepudiation ? Data.newPINID : Data.PIN_G_ID;

            Util.arrayCopy(hash, x0, files,
                    (short) (offset + Data.privHashOffset), (short) 20);

            Util.setShort(files, (short) (offset + Data.privKeyLengthOffset),
                    keyLen);
            offset += Data.privUsageOffset;
            if (nonRepudiation) {
                files[offset++] = 6;
                files[offset++] = 0;
                files[offset++] = 0x40;
            } else {
                files[offset++] = 7;
                files[offset++] = 0x20;
                files[offset++] = 0;
            }

            data[0] = Data.newKeyID;
            apdu.setOutgoingAndSend(x0, x1);

            JCSystem.beginTransaction();

            if (nonRepudiation) {
                Data.freePINSlots--;
                Data.newPINID++;
                Data.newPINRef++;
                files[(short) (Data.AODFOffset + Data.newPINOffset)] = 0x30;
                Data.newPINOffset += Data.PINRecordSize;
            }

            keys[(short) (keys.length - Data.freeKeySlots - unusedKeys)] = key;
            Data.freeKeySlots--;
            Data.newFileID += 2;
            Data.newKeyID++;
            files[(short) (Data.PrKDFOffset + Data.newPrivKeyOffset)] = 0x30;
            files[(short) (Data.PuKDFOffset + Data.newPubKeyOffset)] = 0x30;
            Data.newPrivKeyOffset += Data.privKeyRecordSize;
            Data.newPubKeyOffset += Data.pubKeyRecordSize;

            JCSystem.commitTransaction();
        } catch (ISOException ie) {
            throw ie;
        } catch (Exception e) {
            ISOException.throwIt((short) 0x9001);
        }
    }

    /**
     * Generates DER encoded RSA public key.
     * @param pk the key
     * @param data temporary data buffer
     * @return DER encoded RSA public key
     */
    static byte[] encodePublicKey(RSAPublicKey pk, byte[] data) {

        short modulusLength = pk.getModulus(data, (short) 0);
        boolean padModulus = ((data[0] & 0x80) != 0);

        if (padModulus) {
            modulusLength++;
        }

        short size = getDERSize(modulusLength);

        short exponentLength = pk.getExponent(data, (short) 0);
        boolean padExponent = ((data[0] & 0x80) != 0);

        if (padExponent) {
            exponentLength++;
        }

        size += getDERSize(exponentLength);

        byte[] der = new byte[getDERSize(size)];

        // generate public key record

        short offset = 0;
        der[offset++] = 0x30;
        offset = putLength(der, offset, size);
        der[offset++] = 2;
        offset = putLength(der, offset, modulusLength);
        if (padModulus) {
            offset++;
        }
        offset += pk.getModulus(der, offset);

        der[offset++] = 2;
        offset = putLength(der, offset, exponentLength);
        if (padExponent) {
            offset++;
        }
        pk.getExponent(der, offset);

        return der;
    }

    /**
     * Calculates identifier for public RSA key.
     * @param pk the key
     * @param data temporary data buffer
     * @return the identifier
     */
    byte[] getKeyHash(RSAPublicKey pk, byte[] data) {

        short offset = 1;
        short len = pk.getModulus(data, offset);

        if ((data[1] & 0x80) != 0) {
            offset--;
            len++;
            data[offset] = 0;
        }
        byte[] hash = new byte[(short) 20];
        digest.doFinal(data, offset, len, hash, x0);
        return hash;
    }

    /**
     * Returns the size of DER object for given value size.
     * @param i the value size
     * @return the size of DER object
     */
    static short getDERSize(short i) {

        if (i < 128) {
            return (short) (i + 2);
        }
        return (short) (i + 3);
    }

    /**
     * Places encoded length of DER object into the buffer.
     * @param data the buffer
     * @param offset offset in the buffer where the length must be
     * placed
     * @param length the length to be placed
     * @return the new offset
     */
    static short putLength(byte[] data, short offset, short length) {
        if (length >= 128) {
            data[offset++] = (byte) 0x81;
        }
        data[offset++] = (byte) length;
        return offset;
    }

    /**
     * Returns file object specified by path in the buffer.
     * @param data the buffer
     * @param index path offset
     * @param l path length
     * @return file object or null if not found
     */
    File getFile(byte[] data, short index, short l) {

        // path must contain even number of bytes
        if (l < 2 || l % 2 != 0) {
            return null;
        }
        l = (short) (l / 2);

        short id = Util.getShort(data, index);

        File x = null;
        
        if (l == 1) {
            x = base;
        } else {
            
            if (id == top.id) {
                x = top;
            } else {
                if (id == (short)0x3fff) {
                    x = base;
                } else {
                    return null;
                }
            }
            index += 2;
            l--;
        }
        
        while (l != 0) {

            if (! x.isDF()) {
                return null;
            }
            File f = ((DFile) x).getFile(Util.getShort(data, index));
            if (f == null || f.parent != x) {
                return null;
            }
            x = f;
            index += 2;
            l--;
        }

        return x;
    }

    /**
     * Handles PSO-CDS APDU command.
     * @param apdu command APDU
     */
    void sign(APDU apdu) {

        byte[] data = apdu.getBuffer();

        if (Util.getShort(data, x2) != (short) 0x9e9a) {
            ISOException.throwIt(ISO7816.SW_INCORRECT_P1P2);
        }

        if (! isSERestored || keyNum == -1 || ! isKeyFileSet) {
            ISOException.throwIt((short) 0x6600);
        }

        checkDataSize(digestLength, apdu);

        if (! keys[keyNum].checkAccess()) {
            ISOException.throwIt(ISO7816.SW_SECURITY_STATUS_NOT_SATISFIED);
        }

        cipher.init(keys[keyNum].value, Cipher.MODE_ENCRYPT);
        Util.arrayCopyNonAtomic(data, x5, signBuffer, x0, digestLength);
        short len = cipher.doFinal(signBuffer, x0, digestLength, data, x0);

        short expected = (short) (keys[keyNum].value.getSize() >> 3);
        if (len != expected) {
            Util.arrayFillNonAtomic(data, x0, (short) (expected - len), x0);
            cipher.doFinal(signBuffer, x0, digestLength,
                           data, (short) (expected - len));
        }

        apdu.setOutgoingAndSend(x0, expected);
    }

    /**
     * Verifies that APDU contains correct number of data bytes.
     * @param expectedSize expected data size
     * @param apdu APDU object
     */
    private void checkDataSize(short expectedSize, APDU apdu) {
        if (expectedSize != apdu.setIncomingAndReceive()) {
            ISOException.throwIt(ISO7816.SW_WRONG_LENGTH);
        }
    }

    /**
     * Verifies that PIN is validated.
     * @param pin the PIN
     * @return true if the PIN is validated or PIN verification is disabled.
     */
    public boolean isValidated(OwnerPIN pin) {
        return verifyPINs ? pin.isValidated() : true;
    }
}

/**
 * This class represents private key.
 */
class PrivateKey {
    /** Owner applet. */
	PKIApplet applet;
    /** Key identifier. */
    short id;
    /** PIN object that protects this key. */
    OwnerPIN PIN;
    /** True if its non-repudiation key. */
    boolean nonRepudiation;
    /** Length of the key. */
    short keyLen;
    /** The key object. */
    RSAPrivateKey value;

    /**
     * Constructs new key object using hardcoded data.
     * @param app The applet object
     */
    PrivateKey(PKIApplet app) {
        applet = app;
        id = Parser.getByte();
        PIN = applet.PINs[Parser.getByte()];
        nonRepudiation = (Parser.getByte() == 0);
        keyLen = Parser.getShort();
        boolean notSupported = false;
        try {
            value = (RSAPrivateKey) KeyBuilder.buildKey(
                       KeyBuilder.TYPE_RSA_PRIVATE, keyLen, false);
        } catch (CryptoException e) {
            notSupported = true;
        }
        if (value == null) {
            notSupported = true;
        }
        if (notSupported) {
            value = null;
            short len = Parser.getShort();
            Parser.skip(len);
            len = Parser.getShort();
            Parser.skip(len);
        } else {
            short len = Parser.getShort();
            value.setModulus(Data.PrivateKeys, Parser.offset, len);
            Parser.skip(len);
            len = Parser.getShort();
            value.setExponent(Data.PrivateKeys, Parser.offset, len);
            Parser.skip(len);
        }
    }

    /**
     * Constructs new key object.
     * @param app the applet object
     * @param id key identifier
     * @param PIN PIN object that protects this key
     * @param nonRepudiation is it non-repudiation key?
     * @param keyLen length of key
     * @param value the key object
     */
    PrivateKey(PKIApplet app, short id, OwnerPIN PIN, boolean nonRepudiation,
               short keyLen, RSAPrivateKey value) {
        this.applet = app;
        this.id = id;
        this.PIN = PIN;
        this.nonRepudiation = nonRepudiation;
        this.keyLen = keyLen;
        this.value = value;
    }


    /**
     * Verifies that user has access to this key.
     * @return true if PIN is validated
     */
    boolean checkAccess() {

        boolean result = applet.isValidated(PIN);
        if (nonRepudiation) {
            PIN.reset();
        }
        return result;
    }
}

