/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.ota;

import com.sun.appmanager.AppManager;

import java.io.Serializable;
import java.lang.reflect.Constructor;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Iterator;
import java.util.Vector;

/**
 * This class describes downloadable content.
 * This class is populated during descriptor parsing.
 * Upon the completion of parsing, the {@link #checkOut} method must be
 * called to verify the consistency of the descriptor. If {@link #checkOut}
 * succeeds (i.e. doesn't throw an exception), then the descriptor must
 * be taken as valid.
 *
 * @version @(#)Descriptor.java	1.21 05/09/23
 */
public class Descriptor implements Serializable
{
    // Establish some Appmanager types for different downloads.

    /**
     * An identifier used to denote a media object which is
     * an executable application.
     */
    public static int TYPE_APP = 1;

    /**
     * An identifier used to denote a media object which is
     * binary data of some sort (i.e., not executable).
     */
    public static int TYPE_DATA = 2;

    /**
     * An identifier used to denote a media object which is
     * a library object for use by other applications.
     */
    public static int TYPE_LIB = 3; // library

    /**
     * The user-readable name of the media object, which identifies
     * it to the user.
     */
    protected String name = null;

    /**
     * A displayable name of this media object for use on the
     * device, if desired.
     */
    protected String displayName = null;

    /**
     * The version of this descriptor.
     */
    protected String version = null;

    /*
     * The number of bytes (non-negative integer) to
     * download from the media object's download URI.
     */
    protected int size = -1;

    /**
     * The MIME type of the media object.
     */
    protected String type = null;

    /**
     * The Appmanager type of the media object.
     * Normally Appmanager treats downloads as
     * either applications {@link #TYPE_APP}, 
     * libraries {@link #TYPE_LIB} or binary data
     * of some sort {@link #TYPE_DATA}.
     */
    protected int appManagerType = 0;

    /**
     * The URI (usually URL) from which the object can
     * be downloaded. This must be accessible via an
     * HTTP GET in order to allow the client agent to
     * perform a download.
     */
    protected String objectURI = null;

    /**
     * A URI to notify after a successful
     * download and installation.
     */
    protected String installNotifyURI = null;

    /**
     * A short, textual description of the media
     * object. There are no particular semantics associated
     * with the description, but it should be displayed
     * to the user prior to installation.
     */
    protected String description = null;

    /**
     * The organization which provides the media
     * object.
     */
    protected String vendor = null;

    /**
     * The URI of an icon object which can
     * be used by the client to represent the
     * media object.
     */
    protected String iconURI = null;

    /**
     * The security level which the Appmanager will associate
     * with this media object.
     */
    protected String securityLevel = null;

    /**
     * For application media objects, a classpath.
     */
    protected String classpath = null;

    /**
     * A vector of applications contained in this media
     * object.
     */
    public Vector applications = null;

    /**
     * The mimetype of a data object.
     */
    protected String data = null;

    /**
     * The URI of a data object.
     */
    protected String href;

    /**
     * An indicator of whether the media object is
     * a library.
     */
    protected boolean isLibrary = false;

    /**
     * An indicator of whether the media object
     * is a native executable.
     */
    protected boolean isNative = false;

    protected String schema;
    protected String source;

    /**
     * A vector of dependencies upon which this
     * media object relies.
     */
    public Vector dependencies = null;

    private static final Class[] PARAMS1 = { String.class };

    private Descriptor() {}

    public Descriptor( String schema,
                       String source ) {
        this.schema = schema;
        this.source = source;
    }

    /**
     * Set the name of the download object.
     * @param name The name to use for this descriptor.
     * @throws SyntaxException if the name is already set.
     */
    public void setName(String name) throws SyntaxException {
        checkNull(this.name);
        this.name = name;
        return;
    }

    /**
     * Set the version of this download.
     * @param version The version of this download.
     * @throws SyntaxException if the version is already set.
     */
    public void setVersion( String version ) throws SyntaxException
    {
        checkNull( this.version );
        this.version = version;
        return;
    }

    /**
     * Set the display name of this download.
     * @param s The name for display use to identify the download.
     */
    public void setDisplayName(String s) {
        displayName = s;
        return;
    }

    /**
     * Set the size of this download.
     * @param size The download size in bytes.
     * @throws SyntaxException if the size has already been set,
     *         or if the size value cannot be properly parsed into
     *         a non-negative, integer value.
     */
    public void setSize(String size) throws SyntaxException {
        if (this.size >= 0) {
            throw new SyntaxException(
	       AppManager.getResourceBundle().getString( "DuplicateSize" ) );
        }

        int sz = -1;

        try {
            sz = Integer.parseInt(size);
        } catch (Throwable e) {
            throw new SyntaxException(
               AppManager.getResourceBundle().getString( "SizeNotInteger" ) );
        }
        if (sz < 0) {
            throw new SyntaxException(
	       AppManager.getResourceBundle().getString( "NegativeSize" ) );
        }
        this.size = sz;
        return;
    }

    /**
     * Set the size of this download.
     * @param size The download size in bytes.
     * @throws SyntaxException if the size has already been set,
     *         or if the size value is not a non-negative value.
     */
    public void setSize(int size) throws SyntaxException {
        if (this.size >= 0) {
            throw new SyntaxException(
	       AppManager.getResourceBundle().getString( "DuplicateSize" ) );
        }
        if (size < 0) {
            throw new SyntaxException(
               AppManager.getResourceBundle().getString( "NegativeSize" ) );
        }
        this.size = size;
        return;
    }

    /**
     * Set the type of this download.
     * @param type The download type.
     * @throws SyntaxException if the type has already been set.
     */
    public void setType(String type) throws SyntaxException {
        checkNull(this.type);
        this.type = type;
        return;
    }

    /**
     * Set the Appmanager type of this media object. Currently downloads
     * recognized by Appmanager are either applications, libraries
     * or data downloads.
     * @param type The Appmanager type
     * @throws SyntaxException if the Appmanager type has already
     *                         been set.
     */
    public void setAppManagerType( int type ) throws SyntaxException {
        checkType( type );
        this.appManagerType = type;
        return;
    }

    /**
     * Set the security level of this media object. Security levels
     * recognized by Appmanager are <code>SecurityConstrained</code>
     * and <code>SecurityTrusted</code>.
     * @param level The security level to set
     * @throws SyntaxException if the specified security level is
     *                         unknown.
     */
    public void setSecurityLevel( String level ) throws SyntaxException {
        level = level.toUpperCase();
        if (!level.equals(AppManager.getResourceBundle().getString( "SecurityConstrained")) &&
            !level.equals(AppManager.getResourceBundle().getString( "SecurityTrusted"))) {
            throw new SyntaxException(
                 AppManager.getResourceBundle().getString( "UnknownSecurityLevel" ) );
        }
        this.securityLevel = level;
        return;
    }

    /**
     * Set the objectURI for this media object, from which the
     * download may be performed.
     * @param uri The URI (URL) to contact.
     * @throws SyntaxException if the objectURI has already been
     *                         set, or if the parameter is an
     *                         invalid URL.
     */
    public void setObjectURI(String uri) throws SyntaxException {
        checkNull(objectURI);
        checkURL(uri);
        objectURI = uri;
        return;
    }

    /**
     * Set the installNotifyURI for this media object. This URI
     * should be notified once download and installation are
     * complete.
     * @param uri The URI to contact
     * @throws SyntaxException if the URI is invalid.
     */
    public void setInstallNotifyURI(String uri) throws SyntaxException {
        checkURL(uri);
        installNotifyURI = uri;
        return;
    }

    /**
     * Set the description for the media object.
     * @param description The new description string.
     * @throws SyntaxException if the description has already
     *                         been set.
     */
    public void setDescription(String description) throws SyntaxException {
        checkNull( this.description );
        this.description = description;
        return;
    }

    /**
     * Set the vendor which provided this media object.
     * @param vendor The vendor identifier.
     * @throws SynxtaxException If the vendor has already
     *                          been set.
     */
    public void setVendor(String vendor) throws SyntaxException {
        checkNull(this.vendor);
        this.vendor = vendor;
        return;
    }

    /**
     * Set the iconURI for this media object.
     * @param uri The URI (URL) which will provide an icon
     *            which will provide an icon for this media
     *            object.
     * @throws SyntaxException If the URI is invalid.
     */
    public void setIconURI(String uri) throws SyntaxException {
        checkURL(uri);
        iconURI = uri;
        return;
    }

    /**
     * Add a ddx:application entry to the list of applications
     * in this media object.
     * @param app An {@link Application} object constructed from
     *            information provided in the application's entry.
     */
    public void addApplication( Application app ) {
        if (applications == null) {
            applications = new Vector();
        }
        applications.add( app );
        return;
    }

    /**
     * For a data object, record a mimetype and href.
     * @param mimeType A MIME type for the data
     * @param href An href indicating where to find
     *             the data (may be null, in which case
     *             the data will be assumed to be local).
     * @throws SyntaxException if data has already been set,
     *                         or if there is an apparent conflict
     *                         in the download type.
     */
    public void addData(String mimeType, String href) throws SyntaxException {
        checkNull(data);
        checkFalse(isLibrary);
        checkNull(applications);

        data = mimeType;
        type = mimeType;
        this.href = href;
        return;
    }

    /**
     * Indicate that this media object contains a library.
     * @param java Whether or not the library is Java (as opposed
     *             to native).
     * @throws SyntaxException if we have already recorded a data
     *                         or application media object.
     */
    public void addLibrary(boolean java) throws SyntaxException {
        checkNull(data);
        checkFalse(isLibrary);
        checkNull(applications);

        isLibrary = true;
        isNative = !java;
        return;
    }

    /**
     * Validate the descriptor by verifying that all of the
     * necessary fields have been populated, and that no
     * contradictory information was included. As part of
     * the process, each {@link Application} identified as
     * part of the media object is also checked for
     * internal consistency.
     * @throws SyntaxException If mandantory fields are missing,
     *                         the descriptor contains contradictory
     *                         information, or any of the
     *                         Applications in the media object
     *                         are internally inconsistent.
     */
    public void checkOut() throws SyntaxException {
        // check vital tags are present
        checkNN(name);
        checkFalse(size < 0);
        checkNN(vendor);
        checkNN(version);

        // check any application/daemon/library/data tags are present.
        checkFalse( (applications == null) &&
                (data == null) && !isLibrary);

        // check there is an object uri, and positive size if
        // it's either daemon or application.
        if (applications != null) {
            checkFalse( (objectURI == null)  || (size <= 0));
        }

        // Check each application for internal consistency.
        if (applications != null) {
            Iterator i = applications.iterator();
            while ( i.hasNext() ) {
                ( (Application)i.next() ).check();
            }
        }
    }

    private void checkType( int downloadType ) throws SyntaxException {
        if ( downloadType != TYPE_APP &&
             downloadType != TYPE_DATA &&
             downloadType != TYPE_LIB )
        {
            throw new SyntaxException(
               AppManager.getResourceBundle().getString( "InvalidDownloadType" ) );
        }
        return;
    }

    private static void checkNull( Object o ) throws SyntaxException {
        if ( o != null ) {
            throw new SyntaxException();
        }
    }

    private static void checkNN( Object o ) throws SyntaxException {
        if ( o == null ) {
            throw new SyntaxException();
        }
    }

    private static void checkFalse(boolean val) throws SyntaxException {
        if ( val ) {
            throw new SyntaxException();
        }
    }

    /**
     * Check a provided URL or URI for valid syntax.
     * @param uri The URI or URI to check.
     * @throws SyntaxException If there is a problem with the URI.
     */
    protected static void checkURL(String uri) throws SyntaxException {
        try {
            URL u = new URL(uri);
        } catch ( MalformedURLException mfue ) {
            throw new SyntaxException( "InvalidURI" );
        }
    }

    /**
     * Retrieve the vendor of this media object (may be null).
     */
    public String getVendor() {
        return vendor;
    }

    /**
     * Retrieve the version of this media object (may be null).
     */
    public String getVersion() {
        return version;
    }

    /**
     * Return the URI of this media object to be used for
     * actual download.
     */
    public String getObjectURI() {
        return objectURI;
    }

    /** 
     * Return the size (a non-negative value) of the media 
     * object for download.
     */
    public int getSize() {
        return size;
    }

    /**
     * Return the URI of this media object's icon (may be null).
     */
    public String getIconURI() {
        return iconURI;
    }

    /**
     * Return the URI of this media object (possibly null)
     * which should be contacted after installation.
     */
    public String getInstallNotifyURI() {
        return installNotifyURI;
    }

    /**
     * Return the name of this media object (possibly null).
     */
    public String getName() {
        return name;
    }

    /** 
     * Return the description of this media object (possibly
     * null).
     */
    public String getDescription() {
        return description;
    }

    /**
     * Return the MIME type of this media object.
     */
    public String getMimeType() {
        return type;
    }

    /**
     * Indicate whether or not this media object is a
     * native library.
     */
    public boolean isNativeLibrary() {
        return isNative && isLibrary;
    }

    /**
     * Indicate whether or not this media object is
     * a Java library.
     */
    public boolean isJavaLibrary() {
        return !isNative && isLibrary;
    }

    /**
     * Indicate whether or not this media object is
     * binary data.
     */
    public boolean isData() {
        return (data != null);
    }

    /**
     * Retrieve the name of this media object for
     * display. If the display name is not set, return
     * the media object's name sttribute.
     */
    public String getDisplayName() {
        if (displayName == null) {
            return name;
        }
        return displayName;
    }

    /**
     * Return a Vector containing all of the {@link Application}
     * objects from this media object. If this is not an application
     * download, the Vector will be null.
     */ 
    public Vector getApplications() {
        return applications;
    }

    /**
     * Return the data href (URI) for this media object. If this is
     * not a data download, the href will be null.
     */
    public String getDataRef() {
        return href;
    }

    public String getSource()
    {
        return source;
    }

    /**
     * Return the security level of this media object, possibly
     * null.
     */
    public String getSecurityLevel()
    {
        return securityLevel;
    }
}
