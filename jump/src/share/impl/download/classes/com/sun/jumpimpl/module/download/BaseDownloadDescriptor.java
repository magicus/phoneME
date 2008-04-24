/*
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


package com.sun.jumpimpl.module.download;

import com.sun.jump.module.download.JUMPDownloadDescriptor;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Properties;
import java.util.Vector;

/**
 * <code>JUMPDownloadDescriptor</code> contains the metadata associated with
 * any content that can be downloaded.
 */

public class BaseDownloadDescriptor extends JUMPDownloadDescriptor {
    
    /** Creates a new instance of BaseDownloadDescriptor */
    public BaseDownloadDescriptor(String schema, String source) {
        super(schema, source);
    }    
    
    public void setName(String name) {
        this.name = name;
    }
    
    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }
    
    public void setVersion(String version) {
        this.version = version;
    }
    
    public void setSize(int size) {
        this.size = size;
    }
    
    public void setMimeType(String mimeType) {
        this.mimeType = mimeType;
    }
    
    public void setType(String type) {
        this.type = type;
    }
    
    public void setObjectURI(String objectURI) {
        this.objectURI = getFullURI( objectURI );
    }
    
    public void setInstallNotifyURI(String installNotifyURI) {
        this.installNotifyURI = installNotifyURI;
    }
    
    public void setDescription(String description) {
        this.description = description;
    }
    
    public void setVendor(String vendor) {
        this.vendor = vendor;
    }
    
    public void setIconURI(String iconURI) {
        this.iconURI = getFullURI( iconURI );
    }
    
    public void setSecurityLevel(String securityLevel) {
        this.securityLevel = securityLevel;
    }
    
    public void setClasspath(String classpath) {
        this.classpath = classpath;
    }
    
    public void setApplications(Properties applications[]) {
        this.applications = applications;
    }
    
    public void setData(String data) {
        this.data = data;
    }
       
    public void setSchema(String schema) {
        this.schema = schema;
    }
    
    public void setSource(String source) {
        this.source = source;
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

    private void checkType( String downloadType ) throws SyntaxException {
        if ( downloadType != TYPE_APPLICATION &&
             downloadType != TYPE_DATA &&
             downloadType != TYPE_LIBRARY )
        {
            throw new SyntaxException( "InvalidDownloadType" ) ;
        }
        return;
    }

    public void checkOut() throws SyntaxException {
        // check vital tags are present
        checkNN(name);
        checkFalse(size < 0);
        checkNN(vendor);
        checkNN(version);

        // check any application/daemon/library/data tags are present.
        checkFalse( (applications == null) &&
                (data == null) && !isLibrary());

        // check there is an object uri, and positive size if
        // it's either daemon or application.
        if (applications != null) {
            checkFalse( (objectURI == null)  || (size <= 0));
        }

        // Check each application for internal consistency.
        //if (applications != null) {
        //    Iterator i = applications.iterator();
        //    while ( i.hasNext() ) {
            //    ( (JUMPApplication)i.next() ).check();
            //}
        //}
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
    
    private String getFullURI( String targetURI ) {
        String result = targetURI;
        // check if the targetURI is relative or absolute.
        // According to RFC 2396 we may just check if it starts with protocol
        int protocolLength = getSource().indexOf(':');
        if (protocolLength >= 0) {
            String protocol = getSource().substring(0, protocolLength + 1);
            if (!targetURI.startsWith( protocol )) {
                int lastSlash = getSource().lastIndexOf('/');
                if (lastSlash == -1)
                    result = getSource() + "/" + targetURI;
                else
                    result = getSource().substring(0, lastSlash + 1) + targetURI;
            }
        }
        return result;
    }

}
