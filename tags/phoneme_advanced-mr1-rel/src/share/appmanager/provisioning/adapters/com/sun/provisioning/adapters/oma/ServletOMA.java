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
 * @(#)ServletOMA.java	1.10 05/10/20
 */


package com.sun.provisioning.adapters.oma;

import java.io.*;
import java.net.*;
import java.util.*;

import javax.servlet.*;
import javax.servlet.http.*;

import javax.provisioning.*;
import javax.provisioning.adapter.*;


/**
 * OMA adapter servlet
 */
public class ServletOMA extends HttpServlet {

    private static final String ADAPTER_NAME =
        "javax.provisioning.adapter.name";
    private static final String ADAPTER_CONTEXT =
        "javax.provisioning.ProvisioningContext";

    private AdapterContext adapterContext;
    private String adapterName;
    private AdapterOMA adapter;
    private DeliveryComponent deliveryComponent;


    public void init(ServletConfig servletConfig) 
	throws ServletException 
    {
	super.init(servletConfig);

        ServletContext servletContext = servletConfig.getServletContext();
        adapterContext = (AdapterContext) 
            servletContext.getAttribute(ADAPTER_CONTEXT);
        adapterName = 
            servletConfig.getInitParameter(ADAPTER_NAME);
        adapter = (AdapterOMA) adapterContext.getAdapter(adapterName);

        deliveryComponent = adapterContext.getDeliveryComponent();
    }


    /**
     * The POST methos is used by Download Agent for sending the notifications
     */
    public void doPost(HttpServletRequest request,
		       HttpServletResponse response)
        throws IOException, ServletException
    {
	log("doGet: pathInfo=" + request.getPathInfo() + 
	    "\n                    servletPath=" + request.getServletPath() +
            "\n                    contextPath=" + request.getContextPath() +
	    "\n                    serverName=" + request.getServerName());

	ClientRequest clientRequest = null;

	try {
	    clientRequest = decode(request);
	    if (!clientRequest.isInstallNotifyRequest()) {
		throw new IllegalArgumentException();
	    }
	    BufferedReader br = request.getReader();
	    String nLine = br.readLine();
	    String msg = null;
	    short code = -1;
	    for (int pos = 0, lLen = nLine.length(); pos<lLen; pos++) {
		if (Character.isWhitespace(nLine.charAt(pos))) {
		    if (code == -1) {
			// first whitespace - getting the notification code
			code = Short.parseShort(nLine.substring(0, pos));
		    } 
		} else {
		    if (code != -1) {
			// first non-whitespace character after the
			// notification code
			msg = nLine.substring(pos);
			break;
		    }
		}
	    }

	    handleNotification(code, msg, clientRequest, request, response);

	} catch (IllegalArgumentException e) {
	    // incorrect URL - 404
	    httpError(response, HttpServletResponse.SC_NOT_FOUND, 
		"Invalid URL used with POST method");
	} catch (Exception ex) {
	    // another error - 400
	    httpError(response, HttpServletResponse.SC_BAD_REQUEST, 
		"Invalid notification request");
	    error(ex, response);
	}

    }


    /**
     * Decode PathInfo, and forward request to deliveryComponent;
     */
    public void doGet(HttpServletRequest request,
		      HttpServletResponse response)
	throws ServletException, IOException
    {
	log("doGet: pathInfo=" + request.getPathInfo() + 
	    "\n                    servletPath=" + request.getServletPath() +
            "\n                    contextPath=" + request.getContextPath() +
	    "\n                    serverName=" + request.getServerName());

	ClientRequest clientRequest = null;

	try {
	    clientRequest = decode(request);
	    handleClientRequest(clientRequest, request, response);

	} catch (Exception ex) {

	    error(ex, response);
	}
    }

    private void handleClientRequest(ClientRequest clientRequest, 
				     HttpServletRequest request,
				     HttpServletResponse response)
	throws Exception 
    {

	
	DeliveryContext deliveryContext =
	    adapterContext.getDeliveryContext(request);
	String fid = clientRequest.getFulfillmentId();

	if (clientRequest.isInstallNotifyRequest()) {
	    // Handle notification
	} else {
	    handleObjectRequest(clientRequest, fid, deliveryContext, request, response);
	}
    }

    private DescriptorFileDD getDescriptor(FulfillmentTask ft) 
	throws AdapterException, IOException {

	BundleDescriptor dd = ft.getBundleDescriptor();
	Deliverable deliverable = dd.getDescriptorFile();

	if (deliverable == null) {
	    log("No descritor file was found for bundle id="+dd.getBundleID());
	    return null;
	} 

	URL dfURL = deliverable.getURL();
	return (DescriptorFileDD)adapter.createDescriptorFile(dfURL);
    }

    private void handleObjectRequest(ClientRequest req, String fid, 
	DeliveryContext context, HttpServletRequest request,
	HttpServletResponse response) {

	FulfillmentTask ft = null;

	try {
	    // trying to get a fulfillment task object with specified id
	    ft = adapterContext.getFulfillmentTask(fid, context, adapterName);
	} catch (InvalidFulfillmentIDException e) {
	    // invalid ID 
	    sendErrorEvent(e, DeliveryEvent.INVALID_FULFILLMENT_ID,
		fid, context, response);
	} catch (AdapterException e) {
	    // another error
	    reportError(e.getMessage(), response);
	}

	try {
	    DescriptorFileDD descriptorFile = getDescriptor(ft);

	    if (req.isDescriptorRequest()) {
		// descriptor request
		// rewriting the descriptor
		deliveryComponent.handleDescriptorFile(descriptorFile, ft);
		log("Rewriting OMA descriptor");
		String adapterURL = getAdapterURL(request);
		descriptorFile.rewriteDeliveryURLs(ft, adapterURL);
		descriptorFile.rewriteDependencies(adapter, context, 
			getContextPath(request));
		deliver(descriptorFile, response);
	    } else if (req.isIconRequest()) {
		// icon file or
		String iconRef = descriptorFile.getIconURI();
		deliver(descriptorFile.getIcon(), response);
	    } else if (req.isObjectRequest()) {
		// object request
		deliver(descriptorFile.getObject(), response);
	    }
	} catch (Exception e) {
	    sendErrorEvent(e, DeliveryEvent.FAILED,
		fid, context, response);
	}
    }

    private void deliver(Deliverable deliverable, HttpServletResponse response) 
	throws IOException {

	// setting the correct content type
	response.setContentType(deliverable.getMimeType());

	// writing the deliverable to servlet's output stream
	OutputStream os = response.getOutputStream();
	deliverable.writeContents(os);
    }

    private void handleNotification(short code, String message,
				    ClientRequest notification,
				    HttpServletRequest request,
				    HttpServletResponse response)
	throws Exception 
    {
	String fulfillmentID = notification.getFulfillmentId();
	DeliveryContext deliveryContext =
	    adapterContext.getDeliveryContext(request);
	int type;

	if (code == AdapterOMA.SC_SUCCESS) {
	    type = DeliveryEvent.CONFIRMED;
	} else {
	    type = DeliveryEvent.FAILED;
	}

	try {
	    FulfillmentTask ft = adapterContext.getFulfillmentTask(fulfillmentID, 
		deliveryContext, adapterName);
	} catch (InvalidFulfillmentIDException e) {
	    type = DeliveryEvent.INVALID_FULFILLMENT_ID; 
	} catch (AdapterException e) {
	    // adapter failure - failed to process the notification request
	    type = DeliveryEvent.OTHER;
	    httpError(response, HttpServletResponse.SC_INTERNAL_SERVER_ERROR, 
		"Adapter failure");
	    return;
	}
	
	DeliveryEvent deliveryEvent = 
	    new DeliveryEvent(deliveryContext,
                              adapterName,
			      type,
			      fulfillmentID,
			      code,
			      message);
	try {
	    deliveryComponent.handleDeliveryEvent(deliveryEvent);

	} catch (ProvisioningException pe) {
	    log("Exception handling DeliveryEvent", pe);
	}

	response.setStatus(HttpServletResponse.SC_OK);
    }

    /*
     * It would be nice to cache this BUT remember that the serverName can
     * be different on different requests...
     */
    private String getAdapterURL(HttpServletRequest request) {
	return	getContextPath(request) + request.getServletPath();
    }

    private String getContextPath(HttpServletRequest request) {
	return
	    request.getScheme() + "://" +
	    request.getServerName() + ":" +
	    request.getServerPort() +
	    request.getContextPath();
    }


    private ClientRequest decode(HttpServletRequest request) 
	throws Exception 
    {
	String pathInfo = request.getPathInfo();

	DeliveryContext deliveryContext = 
		adapterContext.getDeliveryContext(request);

	return adapter.fromExternalURL(pathInfo, deliveryContext);
    }


    private void error(Exception ex, HttpServletResponse response) 
	throws ServletException, IOException
    {
        log("ServletOMA exception", ex);
	
	PrintWriter pw = new PrintWriter(response.getOutputStream());
	ex.printStackTrace(pw);
	pw.flush();
	
	throw new ServletException(ex);
    }

    private void httpError(HttpServletResponse response, int responseCode, 
	String msg) throws ServletException, IOException {

        log("ServletOMA error : "+msg);

	response.setStatus(responseCode);
	PrintWriter pw = new PrintWriter(response.getOutputStream());
	pw.println(msg);
	pw.flush();
    }


    private void reportError(String msg, HttpServletResponse response) {
	log(msg);
    }


    private void sendErrorEvent(Exception exception, int type, String fulfillmentID,
	DeliveryContext deliveryContext, HttpServletResponse response) {

	DeliveryEvent deliveryEvent = 
	    new DeliveryEvent(deliveryContext,
                              adapterName,
			      type,
			      fulfillmentID,
			      -1,
			      exception.getMessage());
	try {
	    deliveryComponent.handleDeliveryEvent(deliveryEvent);

	} catch (ProvisioningException pe) {
	    log("Exception handling DeliveryEvent", pe);
	}
    }

}
