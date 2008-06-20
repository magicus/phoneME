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
import com.sun.jump.module.download.JUMPDownloadException;

import java.net.URL;
import java.net.MalformedURLException;

public class OMADownloadDescriptor extends BaseDownloadDescriptor {
    /**
     * A URI (URL) used to provide further
     * information describing the media object.
     */
    protected String infoURI = null;

    /**
     * Whether user interaction is allowed.
     */
    protected boolean userInteraction = true;

    /**
     * The URI (URL) to which the client should
     * navigate if the user elects to invoke a browsing
     * action after the download is done. This allows
     * the download service to continue user interaction
     * after the download is complete (whether successful
     * or not).
     */
    protected String nextURI = null;

    public OMADownloadDescriptor( String schema, String source ) {
        super( schema, source);
    }

    /**
     * Return the URI from which to request more information
     * about this media object (may be null).
     */
    public String getInfoURI() {
        return infoURI;
    }

    /**
     * Indicate whether or not user interaction is allowed
     * for this media object.
     */
    public boolean getInteraction() {
        return userInteraction;
    }

    /**
     * Return the nextURI value for this media object
     * (may be null).
     */
    public String getNextURI() {
        return nextURI;
    }

    /**
     * Set the infoURI for this media object.
     * @param uri The URI (URL) which provides more information
     *            about the media object.
     * @throws SyntaxException if the parameter is an invalid
     *                         URL.
     */
    public void setInfoURI( String uri ) throws SyntaxException {
        checkURL( uri );
        infoURI = uri;
        return;
    }

    /**
     * Set option of whether or not to allow user interaction.
     * @param user True or false.
     */
    public void setInteraction( boolean user ) throws SyntaxException {
        userInteraction = user;
        return;
    }

    /**
     * Set the nextURI value for this media object, to which the
     * client should navigate after installation (whether successful
     * or not) if the user selects a browsing option.
     * @param uri the URI to which to navigate
     * @throws SyntaxException if the URI is invalid.
     */
    public void setNextURI( String uri ) throws SyntaxException {
        checkURL( uri );
        nextURI = uri;
        return;
    }
}
