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

/*
 * @(#)AdapterOMA.java	1.7 05/10/20
 */


package com.sun.provisioning.adapters.oma;

import java.io.*;
import java.net.*;
import java.util.*;
import java.security.GeneralSecurityException;

import javax.provisioning.*;
import javax.provisioning.adapter.*;

/**
 * OMA adapter 
 */
public class AdapterOMA extends Adapter {

    private AdapterContext adapterContext;
    private String baseURI;
    private String adapterName;
    private String descriptorFileMimeType;

    // directory names used in URLs given to client
    
    // descriptor directory
    static final String DD_DIR	    = "dd";
    // virtual directory from which the icon will be available
    static final String ICON_DIR    = "icon";
    // virtual directory from which the object will be available
    static final String OBJECT_DIR	= "object";
    // directory for the notifications
    static final String NOTIFY_DIR  = "notify";
    

    // notification codes
    public static final short SC_SUCCESS		= 900;
    public static final short SC_INSUFFICIENT_MEMORY	= 901;
    public static final short SC_USER_CANCELLED		= 902;
    public static final short SC_LOSS_OF_SERVICE	= 903;
    public static final short SC_ATTRIBUTE_MISMATCH	= 904;
    public static final short SC_INVALID_DESCRIPTOR	= 905;
    public static final short SC_INVALID_DDVERSION	= 951;
    public static final short SC_DEVICE_ABORTED		= 952;
    public static final short SC_NON_ACCEPTABLE_CONTENT = 953;
    public static final short SC_LOADER_ERROR		= 954;
    
    /**
     * Initialization method
     */
    public void init(AdapterConfig adapterConfig) {

        adapterContext = adapterConfig.getAdapterContext();
        baseURI = adapterConfig.getBaseURI();
        adapterName = adapterConfig.getAdapterName();
	descriptorFileMimeType = adapterConfig.getDescriptorFileMimeType();

        log("AdapterOMA.init()");
    }


    /**
     * Clean-up method
     */
    public void destroy() {
        log("AdapterOMA.destroy()");
    }


    /**
     * Return a "Fulfillment URI" that can be used by a client device to
     * initiate the download of a bundle (which may involve downloading
     * multiple files).  The returned URI may be absolute, or else it
     * begins with a "/" and is relative to the current context root.
     * <p>
     *
     * It is adapter-specific how much of the information in the 
     * fulfillmentTask is encoded into the Fulfillment URI.At minimum,
     * the returned URI must contain the fulfillmentID.<p>
     *
     * An adapter may choose to include a Message Authentication Code
     * to prevent clients from creating their own Fulfillment URIs.
     */
    public String createFulfillmentURI(FulfillmentTask fft) {
        String fulfillmentID = fft.getFulfillmentID();
	return baseURI + "/" + URLEncoder.encode(fulfillmentID) + "/" + DD_DIR;
    }


    public ClientRequest fromExternalURL(String pathInfo,
					 DeliveryContext deliveryContext)
	throws AdapterException {

	return ClientRequest.getRequestObject(pathInfo);
    }
	
    /**
     * Called by the descriptor parser. Returns the delivery URL for the
     * bundle with specified content-id
     * @param contentID bundle content id
     * @return bundle delivery URL
     */
    String getDeliveryURLForCID(final String contentID, DeliveryContext context) {

	MatchPolicy mp = new MatchPolicy() {
	    public float doMatch(BundleDescriptor bd) {
		if (bd.getContentID().equals(contentID)) {
		    return 1.0f;
		}
		return 0.0f;
	    }
	};
	List matchPolicies = new ArrayList(1);
        matchPolicies.add(mp);
	Collection bundles;
	try {
	    bundles = adapterContext.getBundleRepository().getBundlesFor(null, 
		matchPolicies, true, true);
	} catch (IOException e) {
	    log("Failed to get the bundles", e);
	    return null;
	}
	if (bundles.size() == 0) {
	    return null;
	}

	try {
	    BundleDescriptor bundle = null;
	    FulfillmentTask ft = null;

	    if (bundles.size() > 1) {
		log("WARNING: found more than one bundle with content-id="+contentID);
		Iterator it = bundles.iterator();
		while (it.hasNext()) {
		    bundle = (BundleDescriptor)it.next();
		    ft = adapterContext.createFulfillmentTask(bundle, context);
		    if (ft.getAdapterName().equals(adapterName)) {
			log("WARNING: forcing the OMA version");
			break;
		    }
		}
	    } else {
		bundle = (BundleDescriptor)bundles.toArray()[0];
		ft = adapterContext.createFulfillmentTask(bundle, context);
	    }

	    return ft.getDeliveryURI();
	} catch (ProvisioningException pe) {
	    log("Failed to create fulfillment task", pe);
	    return null;
	}
    }


    private DescriptorFileDD 
	getDescriptorFile(BundleDescriptor bundleDescriptor) 
	throws AdapterException
    {
	DescriptorFileDD result = null;
	try {
	    URL url = bundleDescriptor.getDescriptorFile().getURL();
	    result = (DescriptorFileDD) createDescriptorFile(url);

	} catch (Exception ex) {
	    // E.g. null pointer exception if bundle has no descriptor file
	    error("Cannot get descriptor file for bundle " +
		  bundleDescriptor.getBundleID(),
		  ex);
	}
	return result;
    }


    /**
     * Returns a DescriptorFile instance corresponding to the 
     * specified descriptor file.
     */
    public DescriptorFile createDescriptorFile(URL descriptorFile) 
	throws AdapterException
    {
	DescriptorFile result = null;
	try {
	    result = new DescriptorFileDD(descriptorFile, 
					   descriptorFileMimeType);
	} catch (Exception ex) {
	    error("Cannot create descriptor file", ex);
	}
	return result;
    }


    public void log(String msg) {
        adapterContext.getServletContext().log(msg);
    }

    public void log(String msg, Throwable t) {
        adapterContext.getServletContext().log(msg, t);
    }


    private void error(String message) 
	throws AdapterException
    {
	error(message, null);
    }


    private void error(String message, Throwable cause) 
	throws AdapterException
    {
	throw new AdapterException(adapterName, message, cause);
    }


}
