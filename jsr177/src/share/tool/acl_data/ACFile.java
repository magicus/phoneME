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

package acl_data;

import java.util.Vector;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.FileOutputStream;

/**
 * This class represents access control file that describes permissions for one
 * card slot.
 */
//public class ACFile implements ImplicitlyTrustedClass {
public class ACFile {

    /** Path to ODF. */
    public static final short ODF = 0x5031;
    /** Path to DODF. */
    public static final short DODF    = 0x5207;
    /** Path to AODF. */
    public static final short AODF    = 0x5208;
    /** Path to ACIFILE. */
    public static final short ACIFILE = 0x5300;
    /** Path to ACFILE. */
    public static final short ACFILE  = 0x5310;

    /**
     * Value of OID from the spceification (A.4.2.1 Location of Access
     * Control Files)
     */
    private byte[] ACIFOID = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x2a, 0x02, 0x6e,
                             0x03, 0x01, 0x01, 0x01};

    /** File name */
    private static String fileName;
    /** Output directory */
    private static String OUT_DIR;
    /** Generated files */
    Vector files = new Vector();
    /**
     * Constructs an instance of an access control file object.
     */
    public ACFile() {
    }

    /**
     * Load access control information.
     * @param fileName file name.
     * @param out_dir output directory.
     * @return ACFile object
     */
    public static ACFile load(String fileName, String out_dir) {
        InputStream permIS;
        OUT_DIR = out_dir;
        try {
            permIS = new FileInputStream(fileName);
        } catch (IOException e) {
            System.err.println("Error during opening file " + e);
            return null;
        }

        try {
            ACFile f = new ACFile();
            f.init(new ACLFileReader(new InputStreamReader(permIS)));
            return f;
        } catch (Exception e) {
            System.out.println("Error reading ACList " + e);
        }
        return null;
    }


    /**
     * The list of ACL objects.
     */
    private Vector ACLists = new Vector();
    /**
     * The list of PIN data objects.
     */
    private Vector PINAttrs = new Vector();

    /**
     * Initializes ACF object.
     * @param r reader for permissions file.
     * @throws IOException if I/O error occurs.
     */
    private void init(ACLFileReader r) throws IOException {
        short n = ACFILE;
        ACList acl;
        while (true) {
            try {
                String s = r.readWord();
                if (s == null) {
                    break;
                }
                if (s.equals("acf")) {
                    ACLists.addElement(new ACList(r, n++));
                } else
                if (s.equals("pin_data")) {
                    PINAttrs.addElement(new PINAttributes(r));
                } else {
                    throw new Exception();
                }

            } catch (Exception e) {
                throw new IOException("Line " + r.lineNumber);
            }
        }
        TLV acif = TLV.createSequence();
        TLV current = acif.setChild(new TLV(TLV.NULL_TYPE));
        for (short i = 0; i < ACLists.size(); i++) {
            acl = (ACList)ACLists.elementAt(i);
            current = current.setNext(acl.getACIFRec());
/* ACF creating */
            Vector v = acl.getACEntries();
            TLV acf = TLV.createSequence();
            TLV acf_current = acf.setChild(new TLV(TLV.NULL_TYPE));
            for (int j = 0; j < v.size(); j++) {
                ACEntry ace = (ACEntry)v.elementAt(j);
                acf_current = acf_current.setNext(ace.getACERec());
            }
            acf.child = acf.child.next;
            TLV acf_p = TLV.createSequence();
            acf_p.setChild(acf);
            addFile(acl.getNum(), acf_p.child.getValue());
        }
        acif.child = acif.child.next;
        acif.getValue();
        addFile(ACIFILE, acif.getValue());

        TLV pattr = TLV.createSequence();
        current = pattr.setChild(new TLV(TLV.NULL_TYPE));
        PINAttributes pat;
        for (int i = 0; i < PINAttrs.size(); i++) {
            pat = (PINAttributes)PINAttrs.elementAt(i);
            current = current.setNext(pat.getPINRec());
        }
        pattr.child = pattr.child.next;
        pattr.getValue();
        TLV pattr_p = TLV.createSequence();
        pattr_p.setChild(pattr);
        addFile(AODF, pattr.getValue());
    }

    /**
     * Adds a new file to the file system.
     * @param path file path
     * @param data file body
     * @throws IOException in case if something wrong
     */
    void addFile(short path, byte[] data)
                                     throws IOException {
        String sPath = OUT_DIR;
        sPath += Integer.toHexString(path);
        FileOutputStream fos;
        try {
            fos = new FileOutputStream(sPath);
            fos.write(data, 0, data.length);
            fos.close();
            files.add(sPath);
        } catch (IOException e) {
            System.out.println("Add file error: " + e);
            return;
        }
    }

    /**
     * Creates ODF file
     * @throws IOException in case if something wrong
     */
    void createODF() throws IOException {
        TLV odfRec = TLV.createSequence(); // this SEQUENCE will be ignored by getValue()
        TLV current;
        
        current = odfRec.setChild(new TLV(ACEntry.CONTEXT_CONSTRUCTED_7)); // dataObjects [7] CHOICE {
        current.setChild(TLV.createSequence()). // path SEQUENCE {
            setChild(TLV.createOctetString(Utils.shortToBytes(DODF))); // path
        // } -- path
        // } -- dataObjects

        current = current.setNext(new TLV(ACEntry.CONTEXT_CONSTRUCTED_8)); // authObjects [8] CHOICE {
        current.setChild(TLV.createSequence()). // path SEQUENCE {
            setChild(TLV.createOctetString(Utils.shortToBytes(AODF))); // path
        // } -- path
        // } -- authObjects

        addFile(ODF, odfRec.getValue());
    }
    
    /**
     * Creates DODF file
     * @throws IOException in case if something wrong
     */
    void createDODF() throws IOException {
        // DataType ::= CHOICE {
        TLV dodfRec = TLV.createSequence().
            setTag(ACEntry.CONTEXT_CONSTRUCTED_1); // [1] DataObject{OidDO} ::= SEQUENCE {
        TLV current = dodfRec.setChild(TLV.createSequence()); // commonObjectAttributes SEQUENCE {}
        current.setNext(TLV.createSequence()). // classAttributes SEQUENCE {
                setChild(TLV.createUTF8String("ACF Converter")); // applicationName
        // } -- classAttributes
        current = current.next;
        current.setNext(new TLV(ACEntry.CONTEXT_CONSTRUCTED_1)). // typeAttributes [1] 
                setChild(TLV.createSequence()). // SEQUENCE {
                setChild(new TLV(TLV.OID_TYPE, ACIFOID)). // id
                setNext(TLV.createSequence()). // value -> path SEQUENCE {
                setChild(TLV.createOctetString(Utils.shortToBytes(ACIFILE))); // path
                // } -- path
        // } -- typeAttributes
        // } -- DataObject
        // } -- DataType
        
        addFile(DODF, dodfRec.getDERData());
    }

}

