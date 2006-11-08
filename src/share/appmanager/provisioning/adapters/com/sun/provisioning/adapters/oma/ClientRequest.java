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

import javax.provisioning.*;
import java.net.URLDecoder;

/** 
 * Class representing a client request. The request may be for a
 * particular file (deliverable) in the context of a fulfillment
 * task.  Alternatively, it may be an install-notify or delete-notify
 * message.
 */
class ClientRequest {

    protected String fid;
    private String reqType;
    private String args;

    private ClientRequest(String fid, String type, String args) { 
	this.fid = fid;
	this.reqType = type;
	this.args = args;
    }

    static ClientRequest getRequestObject(String url) {
	int fidSlash = url.indexOf('/', 2);
	String fid = URLDecoder.decode(url.substring(1, fidSlash));
	String rest = url.substring(fidSlash+1);
	int nextSlash = rest.indexOf('/');
	if (nextSlash == -1) {
	    // creating the request object without an argument
	    return new ClientRequest(fid, rest, null);
	} else {
	    // creating the request object with an argument
	    return new ClientRequest(fid, rest.substring(0, nextSlash), 
		rest.substring(nextSlash+1));
	}
    }

    public String getFulfillmentId() {
	return fid;
    }

    public boolean isDescriptorRequest() {
	return reqType.equals(AdapterOMA.DD_DIR);
    }

    public boolean isIconRequest() {
	return reqType.equals(AdapterOMA.ICON_DIR);
    }

    public boolean isObjectRequest() {
	return reqType.equals(AdapterOMA.OBJECT_DIR);
    }

    public boolean isInstallNotifyRequest() {
	return reqType.equals(AdapterOMA.NOTIFY_DIR);
    }

    public String toString() {
	return "Client Request : FID="+fid+", type="+reqType+", args="+args;
    }

    public String getArgs() {
	return args;
    }
}

