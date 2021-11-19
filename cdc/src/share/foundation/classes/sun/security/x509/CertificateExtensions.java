/*
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package sun.security.x509;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.security.cert.CertificateException;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.List;
import java.util.ArrayList;

import sun.security.util.*;

/**
 * This class defines the Extensions attribute for the Certificate.
 *
 * @author Amit Kapoor
 * @author Hemma Prafullchandra
 * @version 1.19
 * @see CertAttrSet
 */
public class CertificateExtensions implements CertAttrSet {
    /**
     * Identifier for this attribute, to be used with the
     * get, set, delete methods of Certificate, x509 type.
     */
    public static final String IDENT = "x509.info.extensions";
    /**
     * name
     */
    public static final String NAME = "extensions";

    private Hashtable map = new Hashtable(11);
    private boolean unsupportedCritExt = false;
    private List unparseableExtensions;

    /**
     * Default constructor.
     */
    public CertificateExtensions() { }

    /**
     * Create the object, decoding the values from the passed DER stream.
     *
     * @param in the DerInputStream to read the Extension from.
     * @exception IOException on decoding errors.
     */
    public CertificateExtensions(DerInputStream in) throws IOException {
        init(in);
    }

    /**
     * Decode the extensions from the InputStream.
     *
     * @param in the InputStream to unmarshal the contents from.
     * @exception IOException on decoding or validity errors.
     */
    public void decode(InputStream in) throws IOException {
        DerValue val = new DerValue(in);
        init(val.data);
    }

    // helper routine
    private void init(DerInputStream in) throws IOException {

        DerValue[] exts = in.getSequence(5);

        for (int i = 0; i < exts.length; i++) {
            Extension ext = new Extension(exts[i]);
            parseExtension(ext);
        }
    }

    // Parse the encoded extension
    private void parseExtension(Extension ext) throws IOException {
        try {
            Class extClass = OIDMap.getClass(ext.getExtensionId());
            if (extClass == null) {   // Unsupported extension
                if (ext.isCritical())
                    unsupportedCritExt = true;
                if (map.put(ext.getExtensionId().toString(), ext) == null)
                    return;
                else
                    throw new IOException("Duplicate extensions not allowed");
            }
            Class[] params = { Boolean.class, Object.class };
            Constructor cons = extClass.getConstructor(params);

            byte[] extData = ext.getExtensionValue();
            int extLen = extData.length;
	    Object value = Array.newInstance(byte.class, extLen);

	    for (int i = 0; i < extLen; i++)
	        Array.setByte(value, i, extData[i]);
	    Object[] passed = new Object[] {new Boolean(ext.isCritical()),
                                                        value};
            CertAttrSet certExt = (CertAttrSet)cons.newInstance(passed);
	    if (map.put(certExt.getName(), certExt) != null)
                throw new IOException("Duplicate extensions not allowed");

        } catch (InvocationTargetException invk) {
            Throwable e = invk.getTargetException();
            if (ext.isCritical() == false) {
                // ignore errors parsing non-critical extensions
                // remember extension for toString()
                if (unparseableExtensions == null) {
                    unparseableExtensions = new ArrayList();
                }
                unparseableExtensions.add(new UnparseableExtension(ext, e));
                return;
            }
            if (e instanceof IOException) {
                throw (IOException)e;
            } else {
                throw (IOException)new IOException(e.toString()).initCause(e);
            }
	} catch (Exception e) {
            throw (IOException)new IOException(e.toString()).initCause(e);
        }
    }

    public List getUnparseableExtensions() {
        if (unparseableExtensions == null) {
            return Collections.EMPTY_LIST;
        } else {
            return unparseableExtensions;
        }
    }

    /**
     * Encode the extensions in DER form to the stream, setting
     * the context specific tag as needed in the X.509 v3 certificate.
     *
     * @param out the DerOutputStream to marshal the contents to.
     * @exception CertificateException on encoding errors.
     * @exception IOException on errors.
     */
    public void encode(OutputStream out)
    throws CertificateException, IOException {
        encode(out, false);
    }

    /**
     * Encode the extensions in DER form to the stream.
     *
     * @param out the DerOutputStream to marshal the contents to.
     * @param isCertReq if true then no context specific tag is added.
     * @exception CertificateException on encoding errors.
     * @exception IOException on errors.
     */
    public void encode(OutputStream out, boolean isCertReq)
    throws CertificateException, IOException {
        DerOutputStream extOut = new DerOutputStream();
        Collection allExts = map.values();
        Object[] objs = allExts.toArray();

        for (int i = 0; i < objs.length; i++) {
            if (objs[i] instanceof CertAttrSet)
                ((CertAttrSet)objs[i]).encode(extOut);
            else if (objs[i] instanceof Extension)
                ((Extension)objs[i]).encode(extOut);
            else
                throw new CertificateException("Illegal extension object");
        }

        DerOutputStream seq = new DerOutputStream();
        seq.write(DerValue.tag_Sequence, extOut);

        DerOutputStream tmp;
        if (!isCertReq) { // certificate
            tmp = new DerOutputStream();
            tmp.write(DerValue.createTag(DerValue.TAG_CONTEXT, true, (byte)3),
                  seq);
        } else
            tmp = seq; // pkcs#10 certificateRequest

        out.write(tmp.toByteArray());
    }

    /**
     * Set the attribute value.
     * @param name the extension name used in the cache.
     * @param obj the object to set.
     * @exception IOException if the object could not be cached.
     */
    public void set(String name, Object obj) throws IOException {
        if (obj instanceof Extension)
            map.put(name, obj);
        else
            throw new IOException("Unknown extension type.");
    }

    /**
     * Get the attribute value.
     * @param name the extension name used in the lookup.
     * @exception IOException if named extension is not found.
     */
    public Object get(String name) throws IOException {
        Object obj = map.get(name);
        if (obj == null) {
            throw new IOException("No extension found with name " + name);
        }
        return (obj);
    }

    /**
     * Delete the attribute value.
     * @param name the extension name used in the lookup.
     * @exception IOException if named extension is not found.
     */
    public void delete(String name) throws IOException {
        Object obj = map.get(name);
        if (obj == null) {
            throw new IOException("No extension found with name " + name);
        }
        map.remove(name);
    }

    /**
     * Return an enumeration of names of attributes existing within this
     * attribute.
     */
    public Enumeration getElements() {
        return (map.elements());
    }

    /**
     * Return a collection view of the extensions.
     * @return a collection view of the extensions in this Certificate.
     */
    public Collection getAllExtensions() {
        return (map.values());
    }

    /**
     * Return the name of this attribute.
     */
    public String getName() {
        return (NAME);
    }

    /**
     * Return true if a critical extension is found that is
     * not supported, otherwise return false.
     */
    public boolean hasUnsupportedCriticalExtension() {
        return (unsupportedCritExt);
    }

    /**
     * Compares this CertificateExtensions for equality with the specified
     * object. If the <code>other</code> object is an
     * <code>instanceof</code> <code>CertificateExtensions</code>, then
     * all the entries are compared with the entries from this.
     *
     * @param other the object to test for equality with this CertificateExtensions.
     * @return true iff all the entries match that of the Other,
     * false otherwise.
     */
    public boolean equals(Object other) {
        if (this == other)
            return true;
        if (!(other instanceof CertificateExtensions))
            return false;
        Collection otherC = ((CertificateExtensions)other).getAllExtensions();
        Object[] objs = otherC.toArray();

        int len = objs.length;
        if (len != map.size())
            return false;

        Extension otherExt, thisExt;
        String key = null;
        for (int i = 0; i < len; i++) {
            if (objs[i] instanceof CertAttrSet)
                key = ((CertAttrSet)objs[i]).getName();
            otherExt = (Extension)objs[i];
            if (key == null)
                key = otherExt.getExtensionId().toString();
            thisExt = (Extension)map.get(key);
            if (thisExt == null)
                return false;
            if (! thisExt.equals(otherExt))
                return false;
        }
        return this.getUnparseableExtensions().equals(
                ((CertificateExtensions)other).getUnparseableExtensions());
    }

    /**
     * Returns a hashcode value for this CertificateExtensions.
     *
     * @return the hashcode value.
     */
    public int hashCode() {
        return map.hashCode() + getUnparseableExtensions().hashCode();
    }

    /**
     * Returns a string representation of this <tt>CertificateExtensions</tt>
     * object in the form of a set of entries, enclosed in braces and separated
     * by the ASCII characters "<tt>,&nbsp;</tt>" (comma and space).
     * <p>Overrides to <tt>toString</tt> method of <tt>Object</tt>.
     *
     * @return  a string representation of this CertificateExtensions.
     */
    public String toString() {
        return map.toString();
    }
}
