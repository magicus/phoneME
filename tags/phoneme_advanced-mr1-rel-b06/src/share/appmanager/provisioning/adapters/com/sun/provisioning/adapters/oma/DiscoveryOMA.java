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

package com.sun.provisioning.adapters.oma;

import java.io.*;
import java.net.*;
import java.util.*;

import javax.servlet.*;
import javax.servlet.http.*;

import javax.provisioning.*;

/**
 * This servelt provides a startup page for OMA application manager
 */
public class DiscoveryOMA extends HttpServlet {

    private static final String PROVISIONING_CONTEXT =
        "javax.provisioning.ProvisioningContext";

    private ServletContext servletContext;
    private ProvisioningContext provisioningContext;
    private BundleRepository repository;

    public void init(ServletConfig servletConfig) throws ServletException {
	super.init(servletConfig);
        servletContext = servletConfig.getServletContext();
        provisioningContext = (ProvisioningContext)
            servletContext.getAttribute(PROVISIONING_CONTEXT);

        repository = provisioningContext.getBundleRepository();
    }

    public void destroy() {
	servletContext = null;
	provisioningContext = null;
	repository = null;
    }

    private static final String PLATFORM = 
	Constants.SoftwarePlatform_JavaPlatform;

    /**
     * Handling GET requests.  
     */
    public void doGet(HttpServletRequest request,
		      HttpServletResponse response)
        throws IOException, ServletException
    {
	try {
	    String pathInfo = request.getPathInfo();
	    log("Pathinfo="+pathInfo);
	    if (pathInfo != null && !pathInfo.equals("/")) {
		// the extra info (i.e. query string) is specified. Performing
		// the fulfillment request
		fulfillRequest(pathInfo, request, response);
	    } else {

		MatchPolicy mp = new MatchPolicy() {
			public float doMatch(BundleDescriptor bd) {
			    try {
				Deliverable d = bd.getDescriptorFile();
				if (d != null) {
				    String uri = d.getURL().getFile();
				    // filtering out all applications except
				    // OMA ones
				    if (uri.endsWith(".dd")) {
					return 1.0F;
				    }
				}		
			    } catch (Exception ignored) {
			    }		    
			    return 0.0F;
			}
		    };	
		List matchPolicies = new ArrayList(1);
		matchPolicies.add(mp);
				
		Collection bundles = 
		    repository.getBundlesFor(null, matchPolicies);

		showBundles(request, response, bundles);
	    }
	} catch (Exception e) {
	    log("Error in JavaWS servlet", e);
	    error(response, e.toString());
	}
    }

    private void fulfillRequest(String pathInfo, 
				HttpServletRequest request, 
				HttpServletResponse response) 
	throws Exception
    {
	String bundleID = getBundleID(pathInfo);
	log("pathInfo=" + pathInfo + ", bundleID=" + bundleID);
	String deliveryURI = getDeliveryURI(bundleID, request);
	log("deliveryURI is "+deliveryURI);

	RequestDispatcher dispatcher =
	    request.getRequestDispatcher(deliveryURI);
	dispatcher.forward(request, response);
    }
	
    private boolean isAbsolute(String uri) {
	int colon = uri.indexOf(':');
	return 0 < colon && colon < uri.indexOf('/');
    }

    private String getBundleID(String pathInfo) {
	int i = pathInfo.indexOf('/');
	int j = pathInfo.lastIndexOf(".dd");
	if (i != 0 || j <= i) {
	    throw new IllegalArgumentException("bad OMA descriptor file name: " + pathInfo);
	}
	return pathInfo.substring(i+1,j);
    }


    private String getDeliveryURI(String bundleID,
				  HttpServletRequest request) 
	throws ProvisioningException, IOException
    {
	BundleDescriptor bundleDescriptor = 
	    repository.getBundleByID(bundleID);
	DeliveryContext deliveryContext = 
	    provisioningContext.getDeliveryContext(request);
	FulfillmentTask ft = 
	    provisioningContext.createFulfillmentTask(bundleDescriptor,
						      deliveryContext);
	return ft.getDeliveryURI();
    }


    private void showBundles(HttpServletRequest request,
			     HttpServletResponse response, 
			     Collection bundles) 
	throws IOException, ProvisioningException
    {
	response.setHeader("Content-type", "text/html");
	PrintWriter out = response.getWriter();

	if (bundles.size() == 0) {
	    error(response, "No matching bundles");

	} else {
	    showBundles2(out, request, bundles);
	}
	out.close();
    }


    private void showBundles2(PrintWriter out, 
			      HttpServletRequest request,
			      Collection bundles) 
	throws IOException, ProvisioningException
    {
	String title = "OMA Objects";
	out.println("<title>" + title + "</title>");
	out.println("<h1>" + title + "</h1>");

	out.println("This page may be accessed via OMA-compliant download manager.<p> ");

	out.println("<ul>");

	Locale l = Locale.getDefault();
	String requestURL = new String(request.getRequestURL());

	Iterator it = bundles.iterator();
	while (it.hasNext()) {
	    BundleDescriptor bd = (BundleDescriptor) it.next();

	    // This version for creating a self-link which will then
	    // dispatch to the deliveryURI.
	    String omaDD_URI = requestURL + "/" + bd.getBundleID() + ".dd";

	    String name = bd.getDisplayName(null);
	    out.println("<li><a href=\"" + omaDD_URI + "\">" + 
			name + "</a></li>"); 
	}
	out.println("</ul>");
    }

    private void error(HttpServletResponse response, String message) 
	throws IOException 
    {
	response.setHeader("Content-type", "text/html");
	PrintWriter out = response.getWriter();
        out.println("<h3>Error!</h3>");
        out.println(message);
        out.close();
    }
}

