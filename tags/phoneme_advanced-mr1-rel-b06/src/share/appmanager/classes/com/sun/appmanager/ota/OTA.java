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

/**
 * @version @(#)OTA.java	1.23 05/03/21
 */

package com.sun.appmanager.ota;

import com.sun.appmanager.AppManager;

import java.util.Hashtable;
import java.util.ResourceBundle;
import java.lang.reflect.Constructor;

/**
 * Abstract class specifying the provisioning client agent.
 */
public abstract class OTA
{
    /**
     * Status constant that indicates to the service that the media
     * object was downloaded and installed successfully.
     */
    public static final String ST_SUCCESS = "900";

    /**
     * Status constant that indicates to the service that the device
     * could not accept the media object as it did not have
     * enough storage space on the device. This event may occur
     * before ar oafter the retrieval of the media object.
     */
    public static final String ST_INSUFFICIENTMEMORY = "901";

    /**
     * Status constant that indicates that the used does not want
     * to go trhough with the download operation. The event may occur
     * after the analyses of the Download Descriptor,
     * or instead of the sedning of the Installation Notification (i.e.
     * the user cancelled the download while th retrieval/installation
     * of the media object was in process)
     */
    public static final String ST_USERCANCELLED = "902";

    /**
     * Status constant that indicates the the client device lost
     * network service while retrieveing the Media Object.
     */
    public static final String ST_LOSSOFSERVICE = "903";

    /**
     * Statis constant that indicates that the downloaded media object
     * size was not the same as stated in the descriptor.
     */
    public static final String ST_SIZEMISMATCH = "904";

    /**
     * Status constant that indicates that the media object does
     * not match the attributes defined in the Download Descriptor,
     * and that the device therefore rejects the media object.
     */
    public static final String ST_ATTRIBUTEMISMATCH = "905";

    /**
     * Status constant that indicates that the device could not
     * interpret the Download Descriptor. This typically means a
     * syntactic error.
     */
    public static final String ST_INVALIDDESCRIPTOR = "906";

    /**
     * Status constant that indicates that the device was not
     * compatible with the "major" version of the Download Descriptor,
     * as indicated in the Attribute Version (that is a parameter
     * to the attribute Media).
     */
    public static final String ST_INVALIDDDVERSION = "951";

    /**
     * Status constant that indicates that the device interrupted, or
     * cancelled, the retrieval of the medai object despite the fact that
     * the content should have been executable on the device.
     * This is thus a different case from "Insuficient Memory" and 
     * "Non-Acceptable Content" (where the device has concluded that it
     * cannot use the content).
     */
    public static String ST_DEVICEABORTED = "952";

    /**
     * Status constant that indicates that after the retrieval of the media
     * object, buf before sedning the installtion notification, the
     * Download Agent concluded that the device cannot use the
     * media object.
     */
    public static String ST_NONACCEPTABLECONTENT = "953";

    /**
     * Status constant that indicates that the URL that was to be
     * used for the retrieval of the Medai Object did not provide access to
     * the Media Object. This may represent for example errors of
     * type server down, incorrect URL and service errors.
     */
    public static String ST_LOADERERROR = "954";

    /**
     * Handle of the AppManager's ResourceBundle, for localization.
     */
    private static ResourceBundle rb = AppManager.getResourceBundle();

    /**
     * Discover applications at given URL.
     * Downloads the page from the url and stores application
     * name and descriptor in the hashtable, where urls are keys
     * and application names are values.
     * @param url url to discover from
     * @return list of discovered applications. If returned hashtable
     * is empty, either no applications were discovered, or there was
     * some sort of an error during the discovery.
     */
    public Hashtable discover( String url ) 
    {
        throw new OTANotSupportedException(
	   rb.getString( "DiscoverNotSupported" ) );
    }

    /**
     * Returns a decriptor information of an application.
     * @param uri uri to get the descriptor from, usually returned
     * by {@link #discover(String) discover()} method.
     * Upon encountering an error in the descriptor, this method
     * will throw an {@link OTAException OTAException} exception,
     * and if url to notify server of problems is known, report the
     * corresponding error to the server. It will not though send
     * the success status, the application is responsible for doing
     * that, since it may encounter a descriptor error itself.
     * @throws OTAException if there are troubles downloading or
     * parsing the descriptor.
     */
    public Descriptor createDescriptor( String uri ) throws OTAException
    {
        throw new 
	    OTANotSupportedException(
	        rb.getString( "CreateDescriptorNotSupported" ) );
    }
  
    /**
     * Download the data identified by the associated Descriptor.
     * @param des the Descriptor object identifying the download,
     * which is provided by the 
     * {@link #createDescriptor(String) createDescriptor()} method.
     * @param store implementation instance of Destination interface to
     * be used to store downloaded bits.
     * @param report class implementing {@link DLIndicator} interface.
     * If null, reporting is not done.
     * @return false if download was cancelled. See {@link DLIndicator}
     * to learn how to cancel a download
     * @throws OTAException if an error occured during download.
     */
    public boolean download( Descriptor des,
            Destination store, DLIndicator report ) throws OTAException
    {
        throw new OTANotSupportedException(
		     rb.getString( "DownloadNotSupported" ) );
    }

    /**
     * Send installation notification to the server, either reporting
     * a success or a failure. Note, that some of the errors will
     * already be reported by {@link #download(Descriptor, Destination, DLIndicator)
     * download()} method.
     * So, if an exception was thrown from
     * {@link #download(Descriptor, Destination, DLIndicator)
     * download()} method, you do not need to report another failure.
     * @param uri uri to send the status to. Usually this uri is
     * taken from the descriptor information provided by the
     * {@link #createDescriptor(String) createDescriptor()} method.
     * @param statusCode Status code to return. Since codes may
     * be different in different OTA implementations, the special
     * constants defined by this class should be used for that.
     * The list of constants starts from {@link #ST_SUCCESS ST_SUCCESS}.
     * Note, that not all of the status may be supported by all of
     * the protocols. If the protocol does not support a certain
     * constants, some default will be used.
     */
    public boolean sendNotify( String uri, String statusCode,
            String statusMsg )
    {
        throw new OTANotSupportedException(
	   rb.getString( "SendNotifyNotSupported" ) );
    }

    /**
     * Returns schema implemented by this instance.
     */
    public abstract String getSchema();
}
