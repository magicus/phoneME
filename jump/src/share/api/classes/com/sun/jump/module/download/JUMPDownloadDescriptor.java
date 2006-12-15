/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jump.module.download;

import java.util.Properties;

/**
 * <code>JUMPDownloadDescriptor</code> contains the metadata associated with
 * any content that can be downloaded.
 */
public abstract class JUMPDownloadDescriptor {
    public static final String TYPE_APPLICATION = "app";
    public static final String TYPE_LIBRARY = "library";
    public static final String TYPE_DATA    = "data";
    
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
    protected String mimeType = null;

    /**
     * The content type of the media object.
     * Normally Appmanager treats downloads as
     * either applications {@link #TYPE_APPLICATION}, 
     * libraries {@link #TYPE_LIBRARY} or binary data
     * of some sort {@link #TYPE_DATA}.
     */
    protected String type = null;

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
    protected Properties applications[] = null;

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
    protected boolean isLibraryType = false;

    /**
     * An indicator of whether the media object
     * is a native executable.
     */
    protected boolean isNativeType = false;

    /**
     * The download application protocol used.
     */
    protected String schema;
    
    /**
     * The source of the content.
     */
    protected String source;
    
    /**
     * Creates a new instance of JUMPDownloadDescriptor
     */
    protected JUMPDownloadDescriptor(String schema, String source) {
        this.source = source;
        this.schema = schema;
    }

    public String getName() {
        return name;
    }

    public String getDisplayName() {
        return displayName;
    }

    public String getVersion() {
        return version;
    }

    public int getSize() {
        return size;
    }

    public String getMimeType() {
        return mimeType;
    }

    public String getType() {
        return type;
    }

    public String getObjectURI() {
        return objectURI;
    }

    public String getInstallNotifyURI() {
        return installNotifyURI;
    }

    public String getDescription() {
        return description;
    }

    public String getVendor() {
        return vendor;
    }

    public String getIconURI() {
        return iconURI;
    }

    public String getSecurityLevel() {
        return securityLevel;
    }

    public String getClasspath() {
        return classpath;
    }

    public Properties[] getApplications() {
        return applications;
    }

    public String getData() {
        return data;
    }

    public String getHref() {
        return href;
    }

    public boolean isLibrary() {
        return isLibraryType;
    }

    public boolean isNative() {
        return isNativeType;
    }

    public String getSchema() {
        return schema;
    }

    public String getSource() {
        return source;
    }
    
}
